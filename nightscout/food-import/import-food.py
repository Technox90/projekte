#!/usr/bin/env python3
"""Import food CSV (German or English) into Nightscout API v3.

No changes to entries, treatments, profiles, device status or pump commands.
Default: offline validation/dry run. Writes require --apply and --confirm.
"""
import argparse
import csv
import datetime as dt
import json
import math
import os
from pathlib import Path
import sys
import tempfile
import urllib.error
import urllib.parse
import urllib.request

HEADER = {
    "de": ("Name", "Kategorie", "Unterkategorie", "Portion", "Einheit",
           "Kohlenhydrate_g", "Fett_g", "Eiweiss_g", "Energie_kJ", "GI"),
    "en": ("Name", "Category", "Subcategory", "Portion", "Unit",
           "Carbs_g", "Fat_g", "Protein_g", "Energy_kJ", "GI"),
}
FIELDS = ("name", "category", "subcategory", "portion", "unit",
          "carbs", "fat", "protein", "energy", "gi")
UNIT_ALIASES = {
    "g": "g", "ml": "ml", "pcs": "pcs", "oz": "oz",
    "stück": "pcs", "stueck": "pcs", "piece": "pcs",
    "pieces": "pcs",
}
DEFAULT_TOKEN_PATH = "/opt/nightscout-food-import/config/food-token"
DEFAULT_BASE = "http://127.0.0.1:1337"
BACKUP_PATH = "/opt/nightscout-food-import/backups"


def number(value, label, line, *, default=None, allow_zero=True, integer=False):
    raw = value.strip()
    if not raw:
        if default is not None:
            return default
        raise ValueError(f"Zeile / row {line}: {label} fehlt / missing")
    try:
        val = float(raw.replace(",", "."))
    except ValueError:
        raise ValueError(f"Zeile / row {line}: {label} ist keine Zahl / not numeric") from None
    if not math.isfinite(val) or val < 0 or (not allow_zero and val == 0):
        raise ValueError(f"Zeile / row {line}: {label} ungueltig / invalid")
    if val > 100000:
        raise ValueError(f"Zeile / row {line}: {label} zu gross / too large")
    if integer and val != int(val):
        raise ValueError(f"Zeile / row {line}: {label} muss ganzzahlig sein / must be an integer")
    return int(val) if integer else val


def make_key(record):
    parts = []
    for field in ("name", "category", "subcategory", "portion", "unit"):
        value = record.get(field, "")
        if field == "portion":
            try:
                parts.append(f"{float(str(value).replace(',', '.')):.6f}")
            except (ValueError, TypeError):
                parts.append(str(value).strip().casefold())
        else:
            parts.append(str(value or "").strip().casefold())
    return tuple(parts)


def load_food_csv(path, lang):
    rows = []
    with path.open("r", encoding="utf-8-sig", newline="") as stream:
        reader = csv.DictReader(stream, delimiter=";" if lang == "de" else ",")
        expected = HEADER[lang]
        if not reader.fieldnames or tuple(reader.fieldnames) != expected:
            raise ValueError("CSV-Spalten / CSV header falsch. Erwartet / Expected: "
                             + ("; " if lang == "de" else ", ").join(expected))
        for line, raw in enumerate(reader, start=2):
            if None in raw:
                raise ValueError(f"Zeile / row {line}: zu viele Spalten / too many columns")
            values = [(raw.get(column) or "").strip() for column in expected]
            if not any(values):
                continue
            mapped = dict(zip(FIELDS, values))
            for field in ("name", "category", "subcategory"):
                if len(mapped[field]) > 120:
                    raise ValueError(f"Zeile / row {line}: {field} zu lang / too long")
            if not mapped["name"]:
                raise ValueError(f"Zeile / row {line}: Name fehlt / missing")
            unit = UNIT_ALIASES.get(mapped["unit"].casefold())
            if not unit:
                raise ValueError(f"Zeile / row {line}: Einheit / unit muss g, ml, pcs oder oz sein")
            item = {
                "type": "food",
                "name": mapped["name"],
                "category": mapped["category"],
                "subcategory": mapped["subcategory"],
                "unit": unit,
                "portion": number(mapped["portion"], "Portion", line, allow_zero=False),
                "carbs": number(mapped["carbs"], "Carbs", line),
                "fat": number(mapped["fat"], "Fat", line, default=0),
                "protein": number(mapped["protein"], "Protein", line, default=0),
                "energy": number(mapped["energy"], "Energy", line, default=0),
                "gi": number(mapped["gi"], "GI", line, default=2, integer=True),
            }
            if item["gi"] not in (1, 2, 3):
                raise ValueError(f"Zeile / row {line}: GI muss 1, 2 oder 3 sein")
            if any(item[field] > 2000 for field in ("carbs", "fat", "protein")):
                raise ValueError(f"Zeile / row {line}: Naehrwert / nutrient > 2000 g")
            rows.append(item)
    if not rows:
        raise ValueError("Keine Lebensmittel in der Datei / No foods in CSV")
    seen = set()
    for item in rows:
        key = make_key(item)
        if key in seen:
            raise ValueError("Doppelter CSV-Eintrag / Duplicate CSV row: " + item["name"])
        seen.add(key)
    return rows


class NoRedirect(urllib.request.HTTPRedirectHandler):
    # Do not leak the authorization bearer token through an HTTP redirect.
    def redirect_request(self, request, fp, code, msg, headers, newurl):
        return None


class NightscoutClient:
    def __init__(self, base_url, token_file):
        self.base = base_url.rstrip("/")
        parsed = urllib.parse.urlparse(self.base)
        # Never send credentials to a public HTTP endpoint.
        if not (parsed.scheme == "http" and parsed.hostname in ("127.0.0.1", "localhost")
                and not parsed.username and not parsed.password):
            raise ValueError("Nur lokaler Zugriff erlaubt / local HTTP only: 127.0.0.1")
        access_token = token_file.read_text(encoding="utf-8").strip()
        if len(access_token) < 10 or "://" in access_token or "/" in access_token:
            raise ValueError("Token-Datei enthaelt keinen gueltigen einzelnen Token")
        self.access_token = access_token
        self.jwt = None

    def request(self, path, *, method="GET", payload=None, query=None, authenticate=True):
        if authenticate and not self.jwt:
            result = self.request("/api/v2/authorization/request/"
                                  + urllib.parse.quote(self.access_token, safe=""),
                                  authenticate=False)
            if not isinstance(result, dict) or not result.get("token"):
                raise RuntimeError("Tokenaustausch fehlgeschlagen / authorization failed")
            self.jwt = result["token"]
        url = self.base + path
        if query:
            url += "?" + urllib.parse.urlencode(query)
        headers = {"Accept": "application/json"}
        if authenticate:
            headers["Authorization"] = "Bearer " + self.jwt
        data = None
        if payload is not None:
            data = json.dumps(payload, ensure_ascii=False).encode("utf-8")
            headers["Content-Type"] = "application/json"
        req = urllib.request.Request(url, method=method, data=data, headers=headers)
        try:
            # Disable redirects: a redirect may expose a bearer JWT to another host.
            opener = urllib.request.build_opener(NoRedirect())
            with opener.open(req, timeout=30) as response:
                body = response.read()
                return json.loads(body.decode("utf-8")) if body else {}
        except urllib.error.HTTPError as exc:
            # Do not print any response body that might include sensitive details.
            raise RuntimeError(f"Nightscout {method} {path}: HTTP {exc.code}") from None
        except urllib.error.URLError:
            raise RuntimeError("Nightscout lokal nicht erreichbar / local API unreachable") from None

    def verify_scopes(self):
        status = self.request("/api/v3/status")
        permissions = status.get("result", {}).get("apiPermissions", {})
        food = permissions.get("food", "")
        if not ("c" in food and "r" in food):
            raise RuntimeError("Der Token braucht Food Lesen+Erstellen (r+c). "
                               "The token must have food read/create.")
        for section in ("entries", "treatments", "profile", "devicestatus"):
            rights = permissions.get(section, "")
            if any(flag in rights for flag in "cud"):
                raise RuntimeError("Unsicherer Token: Schreibrechte ausserhalb food erkannt. "
                                   "Use a dedicated restricted food token.")
        print("[OK] Tokenrechte geprueft / token scopes verified (food r+c only)")

    def get_existing_foods(self):
        all_rows = []
        limit = 500
        for skip in range(0, 20000, limit):
            response = self.request("/api/v3/food",
                                    query=[("limit", limit), ("skip", skip)])
            if isinstance(response, dict):
                rows = response.get("result")
            else:
                rows = response
            if not isinstance(rows, list):
                raise RuntimeError("Unexpected Nightscout food GET response")
            all_rows.extend(rows)
            if len(rows) < limit:
                return all_rows
        raise RuntimeError("More than 20,000 food records; import aborted")

    def create_food(self, record):
        return self.request("/api/v3/food", method="POST", payload=record)


def save_backup(rows, directory):
    directory.mkdir(mode=0o700, parents=True, exist_ok=True)
    os.chmod(directory, 0o700)
    when = dt.datetime.now(dt.timezone.utc).strftime("%Y%m%dT%H%M%SZ")
    fd, temp_name = tempfile.mkstemp(prefix=".food-backup-", suffix=".tmp",
                                     dir=directory)
    destination = directory / f"food-backup-{when}.json"
    try:
        os.fchmod(fd, 0o600)
        with os.fdopen(fd, "w", encoding="utf-8") as stream:
            json.dump(rows, stream, indent=2, ensure_ascii=False)
        os.replace(temp_name, destination)
    finally:
        if os.path.exists(temp_name):
            os.unlink(temp_name)
    return destination


def main():
    parser = argparse.ArgumentParser(description="Nightscout food CSV importer (DE/EN)")
    parser.add_argument("--file", required=True, type=Path, help="CSV path")
    parser.add_argument("--lang", choices=("de", "en"), required=True)
    parser.add_argument("--url", default=DEFAULT_BASE, help="Local Nightscout URL")
    parser.add_argument("--token-file", type=Path, default=Path(DEFAULT_TOKEN_PATH))
    parser.add_argument("--apply", action="store_true",
                        help="Allow food CREATE calls. Without this: offline dry run")
    parser.add_argument("--confirm", default="",
                        help="For --apply, type exactly IMPORT_FOOD_ONLY")
    args = parser.parse_args()
    records = load_food_csv(args.file, args.lang)
    print(f"[OK] CSV validiert / CSV validated: {len(records)} items")
    for item in records[:10]:
        print(f"  {item['name']}: {item['portion']:g} {item['unit']}; {item['carbs']:g} g carbs")
    if not args.apply:
        print("[VORSCHAU / DRY RUN] Nichts geschrieben / no writes performed")
        return
    if args.confirm != "IMPORT_FOOD_ONLY":
        parser.error("--apply erfordert --confirm IMPORT_FOOD_ONLY")
    client = NightscoutClient(args.url, args.token_file)
    client.verify_scopes()
    existing = client.get_existing_foods()
    known = {make_key(entry) for entry in existing
             if isinstance(entry, dict) and entry.get("type") == "food"}
    pending = [item for item in records if make_key(item) not in known]
    print(f"[INFO] Bereits vorhanden / Existing: {len(existing)}")
    print(f"[INFO] Neue Lebensmittel / New foods: {len(pending)}")
    if not pending:
        print("[FERTIG / DONE] Keine neuen Eintraege / no new entries")
        return
    backup = save_backup(existing, Path(BACKUP_PATH))
    print("[OK] Food-Backup erstellt / food backup saved:", backup)
    count = 0
    for item in pending:
        client.create_food(item)
        count += 1
        print(f"[OK] {count}/{len(pending)}: {item['name']}")
    print(f"[FERTIG / DONE] {count} Lebensmittel eingefuegt / foods added")


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, RuntimeError) as exc:
        print(f"[FEHLER / ERROR] {exc}", file=sys.stderr)
        sys.exit(1)
