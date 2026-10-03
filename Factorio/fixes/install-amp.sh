#!/usr/bin/env bash
set -Eeuo pipefail
# BBQ CHAOS: vor Einsatz Factorio01 in AMP STOPPEN!
ROOT="/home/amp/.ampdata/instances/Factorio01/factorio/server/mods"
LIST="$ROOT/mod-list.json"
NAME="bbq-chaos-belt-compat"
VER="1.0.3"
# Unveraenderlicher Git-Commit, kein veraltetes Cache und kein unkontrolliertes main.
BASE="https://raw.githubusercontent.com/Technox90/projekte/470a61f4213ea2065a9f104f7ea9813d299a4796/Factorio/fixes/$NAME-$VER"
if [[ ! -f "$LIST" ]]; then echo "FEHLER: $LIST nicht vorhanden" >&2; exit 1; fi
python3 - "$ROOT" "$LIST" "$NAME" "$VER" "$BASE" <<'PY'
import datetime
import io
import json
import os
import pathlib
import shutil
import sys
import urllib.request
import zipfile
root, filename, name, version, base = sys.argv[1:]
p = pathlib.Path(filename)
zpath = pathlib.Path(root) / f"{name}_{version}.zip"
modlist = json.loads(p.read_text(encoding="utf-8-sig"))
by_name = {x["name"]: x for x in modlist["mods"]}
for required in ("base", "space-age", "boblogistics", "angelsaddons-space-age-revived"):
    if not by_name.get(required, {}).get("enabled"):
        raise SystemExit(f"FEHLER: Abhaengigkeit {required} nicht aktiv")

contents = {}
for item in ("info.json", "data-final-fixes.lua"):
    req = urllib.request.Request(f"{base}/{item}", headers={"User-Agent":"BBQ-CHAOS-Factorio-compat"})
    with urllib.request.urlopen(req, timeout=35) as result:
        contents[item] = result.read()
info = json.loads(contents["info.json"].decode("utf-8"))
if info.get("name") != name or info.get("version") != version:
    raise SystemExit("FEHLER: Unpassende GitHub Mod-Metadaten")
if not contents["data-final-fixes.lua"].decode("utf-8").find("BBQ CHAOS BELT FIX v1.0.3") >= 0:
    raise SystemExit("FEHLER: Unpassende GitHub Lua-Version")
prefix = f"{name}_{version}/"
buffer = io.BytesIO()
with zipfile.ZipFile(buffer, "w", zipfile.ZIP_DEFLATED) as archive:
    for item, data in contents.items():
        archive.writestr(prefix + item, data)
with zipfile.ZipFile(io.BytesIO(buffer.getvalue())) as test:
    assert test.testzip() is None

backup = p.with_name(p.name + "." + datetime.datetime.now().strftime("%Y%m%d-%H%M%S") + ".bak")
shutil.copy2(p, backup)
if name in by_name:
    by_name[name]["enabled"] = True
else:
    modlist["mods"].append({"name":name,"enabled":True})

# Rechte und Gruppe der AMP-mod-list.json beibehalten.
stat = p.stat()
mod_data = (json.dumps(modlist, indent=2) + "\n").encode("utf-8")
tmp = p.with_name(p.name + ".bbqtmp")
try:
    tmp.write_bytes(mod_data)
    os.chmod(tmp, stat.st_mode & 0o777)
    os.chown(tmp, stat.st_uid, stat.st_gid)
    os.replace(tmp, p)
    zpath.write_bytes(buffer.getvalue())
    os.chmod(zpath, 0o644)
    os.chown(zpath, stat.st_uid, stat.st_gid)
finally:
    if tmp.exists(): tmp.unlink()

# Ältere Versionen nur nach erfolgreicher Installation archivieren.
old_dir = pathlib.Path(root).parent / "old-mods"
old = sorted(pathlib.Path(root).glob(f"{name}_*"))
old = [q for q in old if q.name != zpath.name and
       (q.is_file() and q.suffix == ".zip" or q.is_dir())]
for f in old:
    old_dir.mkdir(parents=True, exist_ok=True)
    target = old_dir / f.name
    if target.exists():
        target = old_dir / (f.name + "." + datetime.datetime.now().strftime("%Y%m%d%H%M%S"))
    shutil.move(str(f), str(target))
    print("Archiviert: " + str(f) + " -> " + str(target))
print("OK: "+str(zpath))
print("OK: "+name+" "+version+" aktiviert. Backup: "+str(backup))
with zipfile.ZipFile(zpath) as arch:
    print("ZIP enthält: " + ", ".join(arch.namelist()))
stored = json.loads(p.read_text(encoding="utf-8"))
print("Modliste: " + str(next((m for m in stored["mods"] if m["name"] == name), "FEHLT")))
print("Jetzt Factorio01 in AMP starten. Im Log MUSS Loading mod bbq-chaos-belt-compat 1.0.3 stehen.")
PY
