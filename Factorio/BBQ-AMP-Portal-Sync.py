#!/usr/bin/env python3
"""BBQ CHAOS Factorio AMP: verified ORIGINAL modportal ZIPs only.
Stop Factorio01 in AMP before invoking --apply. Never upload your portal credentials.
This admin utility does not modify any mod ZIP; it stages a clean directory
and atomically swaps it into the AMP instance only after all checks succeed.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor, as_completed
from datetime import datetime
import getpass
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import sys
import tempfile
from urllib.parse import quote, urlencode
from uuid import uuid4
from urllib.request import Request, urlopen
from zipfile import ZipFile, BadZipFile

SERVER = Path("/home/amp/.ampdata/instances/Factorio01/factorio/server")
GITHUB = "https://raw.githubusercontent.com/Technox90/projekte/main/Factorio/"
PORTAL = "https://mods.factorio.com"
BUILTIN = {"base", "quality", "space-age", "elevated-rails"}
SAFE = re.compile(r"^[\w .-]{1,128}$", re.ASCII)
SHA1 = re.compile(r"^[a-fA-F0-9]{40}$")
VERSION = re.compile(r"^\d+\.\d+\.\d+$")
HEADERS = {"User-Agent": "BBQ-CHAOS-AMP-original-portal-sync/1.1", "Cache-Control": "no-cache, no-store", "Pragma": "no-cache"}


def fetch_bytes(url):
    with urlopen(Request(url, headers=HEADERS), timeout=90) as response:
        return response.read()


def fetch_json(url):
    return json.loads(fetch_bytes(url).decode("utf-8-sig"))


def check_name(name):
    if not isinstance(name, str) or not SAFE.fullmatch(name) or name.strip() != name or name in (".", ".."):
        raise ValueError("Ungueltiger Modname in Liste/Portal")
    return name


def parsed_version(value):
    if not isinstance(value, str) or not VERSION.fullmatch(value):
        raise ValueError("Ungueltige Mod-Version")
    return tuple(map(int, value.split(".")))


def sha1_file(path):
    digest = hashlib.sha1()
    with open(path, "rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest().lower()


def portal_info(name, pin):
    meta = fetch_json(PORTAL + "/api/mods/" + quote(name, safe="") + "/full")
    if meta.get("name") != name:
        raise ValueError(f"{name}: Portal liefert unerwartete Mod-ID")
    candidates = [
        r for r in meta.get("releases", [])
        if r.get("info_json", {}).get("factorio_version") == "2.0"
        and isinstance(r.get("version"), str) and VERSION.fullmatch(r["version"])
        and (pin is None or r["version"] == pin)
    ]
    if not candidates:
        raise ValueError(f"{name}: keine offizielle 2.0-Version" + (f" fuer Pin {pin}" if pin else ""))
    release = max(candidates, key=lambda r: parsed_version(r["version"]))
    if not SHA1.fullmatch(str(release.get("sha1", ""))):
        raise ValueError(f"{name}: SHA1 fehlt oder ist ungueltig")
    file_name = f'{name}_{release["version"]}.zip'
    if release.get("file_name") != file_name or not release.get("download_url", "").startswith("/download/"):
        raise ValueError(f"{name}: Dateiname oder Portal-Downloadlink ungueltig")
    return name, release


def get_login():
    player = SERVER / "player-data.json"
    creds = {}
    if player.is_file():
        creds = json.loads(player.read_text(encoding="utf-8-sig"))
    username = creds.get("service-username") or creds.get("username") or ""
    token = creds.get("service-token") or ""
    if not username:
        if not sys.stdin.isatty():
            raise SystemExit("Kein Benutzername in player-data.json; SSH-Terminal erforderlich.")
        username = input("Factorio.com-Benutzername (NICHT hier im Chat eingeben): ").strip()
    if not token:
        if not sys.stdin.isatty():
            raise SystemExit("Kein service-token in player-data.json; SSH-Terminal erforderlich.")
        token = getpass.getpass("Factorio.com-Service-Token (Eingabe unsichtbar): ").strip()
    if not username or not token:
        raise SystemExit("Modportal-Login fehlt.")
    return username, token


def archive_problem(path, name, version, sha1):
    """None if the exact original portal ZIP is valid; otherwise a precise reason.
    Official hashes are mandatory. ZIP root folder spelling is NOT a trust check:
    official releases may use a different root folder or store info.json at root.
    """
    if not path.is_file():
        return "Datei fehlt"
    actual_sha1 = sha1_file(path)
    if actual_sha1 != sha1.lower():
        return f"SHA1 falsch (Portal: {sha1.lower()}, Datei: {actual_sha1})"
    try:
        with ZipFile(path) as archive:
            candidates = [x for x in archive.namelist() if x == "info.json" or x.endswith("/info.json")]
            if not candidates:
                return "Gueltige Portal-SHA1, aber kein info.json im Archiv"
            observed = []
            for entry in candidates:
                try:
                    info = json.loads(archive.read(entry).decode("utf-8-sig"))
                except (KeyError, ValueError, UnicodeError):
                    continue
                seen_name, seen_version = info.get("name"), info.get("version")
                observed.append(f"{seen_name}@{seen_version}")
                if seen_name == name and seen_version == version:
                    return None
            return (f"Gueltige Portal-SHA1, aber info.json hat andere ID/Version "
                    f"(erwartet {name}@{version}; gefunden: {', '.join(observed) or 'nicht lesbar'})")
    except (OSError, BadZipFile, ValueError) as error:
        return f"Gueltige Portal-SHA1, aber ZIP nicht lesbar: {type(error).__name__}"


def validate_archive(path, name, version, sha1):
    return archive_problem(path, name, version, sha1) is None


def download_release(name, release, old_mods, stage, creds):
    filename = release["file_name"]
    version = release["version"]
    expected = release["sha1"].lower()
    old_zip = old_mods / filename
    staged_zip = stage / filename
    if validate_archive(old_zip, name, version, expected):
        # Copy only a byte-identical original ZIP; never reuse patched ZIPs.
        shutil.copy2(old_zip, staged_zip)
        return f"Vorhandenes Portal-Original: {filename}"

    username, token = creds
    query = urlencode({"username": username, "token": token})
    endpoint = PORTAL + release["download_url"] + "?" + query
    # Credentials are never written to output, error messages, or GitHub.
    try:
        with urlopen(Request(endpoint, headers=HEADERS), timeout=120) as response:
            with open(staged_zip, "wb") as target:
                shutil.copyfileobj(response, target, length=1024 * 1024)
    except Exception:
        staged_zip.unlink(missing_ok=True)
        raise RuntimeError(f"Modportal-Download fehlgeschlagen: {filename} (Login/Netzwerk pruefen)") from None
    problem = archive_problem(staged_zip, name, version, expected)
    if problem:
        staged_zip.unlink(missing_ok=True)
        raise ValueError(f"{filename}: {problem}")
    return f"Heruntergeladen: {filename}"


def main():
    parser = argparse.ArgumentParser(description="Synchronisiert AMP nur mit Originalmods aus mods.factorio.com")
    parser.add_argument("--check", action="store_true", help="Alle Portalversionen pruefen, nichts aendern")
    parser.add_argument("--apply", action="store_true", help="Nach vollstaendigem Download Modordner umstellen")
    args = parser.parse_args()
    if args.check == args.apply:
        parser.error("genau --check oder --apply angeben")

    old_mods = SERVER / "mods"
    if not old_mods.is_dir():
        raise SystemExit(f"AMP-Modordner nicht vorhanden: {old_mods}")
    if args.apply:
        raise_if_running()

    print("Hole offizielle GitHub-Modauswahl und Versionsvorgaben ...", flush=True)
    # Each GitHub request receives a fresh query string to defeat raw CDN caches.
    modlist = fetch_json(GITHUB + "mod-list.json?nocache=" + uuid4().hex)
    pins = fetch_json(GITHUB + "version-pins.json?nocache=" + uuid4().hex)
    entries = modlist["mods"]
    # No disabled entries or retired optional mods belong in the curated list.
    # Fail safely before touching any ZIP if an obsolete CDN response is seen.
    if any(x.get("enabled") is not True for x in entries):
        raise SystemExit("Alte/ungueltige GitHub-Liste: deaktivierte Mods enthalten. Erneut versuchen.")
    if any(x.get("name") == "AutoDeconstruct" for x in entries):
        raise SystemExit("Veraltete GitHub-Liste erkannt (AutoDeconstruct). Erneut versuchen; AMP unveraendert.")
    seen = set()
    active = []
    for item in entries:
        name = check_name(item["name"])
        if name in seen or type(item.get("enabled")) is not bool:
            raise ValueError(f"Duplikat/ungueltiger Status: {name}")
        seen.add(name)
        if item["enabled"] and name not in BUILTIN:
            active.append(name)
    if not all(item["enabled"] for item in entries if item["name"] in BUILTIN):
        raise ValueError("Basis-/Space-Age-DLC deaktiviert")
    if set(pins) - set(active):
        raise ValueError("Version-Pin fuer deaktivierte/nicht vorhandene Mod")
    for v in pins.values():
        parsed_version(v)
    if not BUILTIN.issubset(seen):
        raise ValueError("Basis-/Space-Age-Eintraege fehlen")
    print(f"GitHub aktuell: {len(entries)} von {len(entries)} aktiv, {len(active)} Portal-ZIPs; AutoDeconstruct ist NICHT enthalten.", flush=True)

    selections = {}
    failures = []
    # All public portal metadata is fetched BEFORE the old AMP mod folder is touched.
    with ThreadPoolExecutor(max_workers=5) as pool:
        futures = {pool.submit(portal_info, name, pins.get(name)): name for name in active}
        for future in as_completed(futures):
            name = futures[future]
            try:
                modname, rel = future.result()
                selections[modname] = rel
            except Exception as error:
                failures.append(f"{name}: {type(error).__name__}: {error}")
    if failures:
        print("\n".join(sorted(failures)))
        raise SystemExit(f"Abbruch: {len(failures)} Mod(s) im Portal nicht passend gefunden; AMP unveraendert.")
    # Every hard dependency must be active. Optional/conflicting declarations
    # remain game-managed; this is a protective preflight, not a playability test.
    enabled = {item["name"] for item in entries if item["enabled"]}
    missing_dependencies = []
    for name, release in selections.items():
        for raw in release.get("info_json", {}).get("dependencies", []):
            dep = str(raw).strip()
            if not dep or dep.startswith(("?", "~", "!", "(?)", "(!)")):
                continue
            if dep.startswith("^"):
                dep = dep[1:].strip()
            dependency = re.split(r"\s+[<>=]+\s*\d", dep, maxsplit=1)[0].strip()
            if dependency and dependency not in enabled:
                missing_dependencies.append(f"{name} benoetigt {dependency}")
    if missing_dependencies:
        print("\n".join(sorted(missing_dependencies)))
        raise SystemExit("Abbruch: verpflichtende Mod-Abhaengigkeiten fehlen; AMP unveraendert.")
    print(f"Vorabpruefung bestanden: {len(selections)} offizielle Versionen.")
    for name, rel in sorted(selections.items()):
        print(f"  {name}: {rel['version']}")

    if args.check:
        print("OK: --check ohne Aenderungen. Beachte: Rezepte/Dependencies nicht spielgetestet.")
        return

    creds = get_login() if any(
        not validate_archive(old_mods / rel["file_name"], name, rel["version"], rel["sha1"])
        for name, rel in selections.items()
    ) else (None, None)
    stage = Path(tempfile.mkdtemp(prefix=".bbq-portal-stage-", dir=SERVER))
    backup = SERVER / ("mods-BBQ-backup-" + datetime.now().strftime("%Y%m%d-%H%M%S"))
    try:
        stat = old_mods.stat()
        os.chmod(stage, stat.st_mode & 0o777)
        if os.geteuid() == 0:
            os.chown(stage, stat.st_uid, stat.st_gid)
        settings = old_mods / "mod-settings.dat"
        if settings.is_file():
            shutil.copy2(settings, stage / settings.name)
        for name, rel in sorted(selections.items()):
            message = download_release(name, rel, old_mods, stage, creds)
            print(message, flush=True)
        destination = stage / "mod-list.json"
        destination.write_text(json.dumps(modlist, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
        for child in stage.iterdir():
            if child.is_file():
                os.chmod(child, 0o644)
                if os.geteuid() == 0:
                    os.chown(child, stat.st_uid, stat.st_gid)
        # Swap ONLY after all ZIP hashes, info.json fields and file writes passed.
        os.replace(old_mods, backup)
        try:
            os.replace(stage, old_mods)
        except Exception:
            os.replace(backup, old_mods)
            raise
        print(f"FERTIG: {len(selections)} unveraenderte Modportal-ZIPs aktiviert.")
        print(f"ALTES MODVERZEICHNIS SICHER AUFBEWAHRT: {backup}")
        print("Jetzt Factorio01 in AMP starten und Log pruefen.")
    finally:
        if stage.exists():
            shutil.rmtree(stage)


def raise_if_running():
    # Prevent likely live server writes; also stop it explicitly through AMP first.
    try:
        for proc in Path("/proc").iterdir():
            if not proc.name.isdigit():
                continue
            try:
                cmd = (proc / "cmdline").read_bytes().replace(b"\0", b" ").decode("utf-8", "ignore")
            except (OSError, PermissionError):
                continue
            if "factorio" in cmd and (
                "--start-server" in cmd or "--create" in cmd or "--start-server-load-latest" in cmd
            ):
                raise SystemExit("Factorio-Prozess aktiv. Factorio01 in AMP STOPPEN!")
    except FileNotFoundError:
        pass


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        raise SystemExit("Abgebrochen: bisherige aktive Mods bleiben erhalten, sofern kein Swap erfolgte.")
    except Exception as error:
        # Never print HTTP URL; it can contain Factorio download token.
        raise SystemExit(f"FEHLER ({type(error).__name__}): {error}") from None
