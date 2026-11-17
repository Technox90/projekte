#!/usr/bin/env python3
"""Erstellt einen privaten 90-Tage-Nightscout-Berichtsexport aus dem bestehenden lokalen Nur-Lese-Export.

Die lokale Nightscout-API wird nur durch den bereits eingerichteten systemd-Exportdienst
kontaktiert, wenn --refresh angegeben ist. Dieses Programm selbst verwendet kein Token.
Python 3.9+; nur Standardbibliothek.
"""
from __future__ import annotations

import argparse
import datetime as dt
import gzip
import json
import os
import pathlib
import subprocess
import sys
import tempfile
import zipfile
from zoneinfo import ZoneInfo

SOURCE_DEFAULT = pathlib.Path('/opt/nightscout-export/data')
OUTPUT_DEFAULT = pathlib.Path('/root')
COLLECTIONS = ('entries', 'treatments', 'devicestatus')
TIMEZONE = ZoneInfo('Europe/Berlin')


def read_records(path: pathlib.Path) -> list[dict]:
    if path.is_symlink() or not path.is_file():
        raise RuntimeError(f'Keine regulaere Exportdatei: {path}')
    try:
        with gzip.open(path, 'rt', encoding='utf-8') as handle:
            data = json.load(handle)
    except (OSError, ValueError, UnicodeError) as exc:
        raise RuntimeError(f'Exportdatei defekt: {path} ({type(exc).__name__})') from None
    if not isinstance(data, list) or not all(isinstance(row, dict) for row in data):
        raise RuntimeError(f'Unerwartetes JSON-Format: {path} (erwartet: Liste aus Objekten)')
    return data


def export_window(days: int):
    # Das vorhandene Export-System trennt Dateien nach UTC-Kalendertagen.
    today = dt.datetime.now(dt.timezone.utc).date()
    start = today - dt.timedelta(days=days - 1)
    return start, today, [start + dt.timedelta(days=n) for n in range(days)]


def verify_paths(root: pathlib.Path, days: list[dt.date]) -> list[pathlib.Path]:
    if not root.is_dir() or root.is_symlink():
        raise RuntimeError(f'Exportordner fehlt: {root}\n'
                           'Pruefe: systemctl status nightscout-export.timer')
    required = [root / name / f'{day.isoformat()}.json.gz'
                for name in COLLECTIONS for day in days]
    required.append(root / 'profile' / 'all.json.gz')
    missing = [p for p in required if not p.is_file() or p.is_symlink()]
    if missing:
        example = '\n'.join(f'  - {p}' for p in missing[:8])
        raise RuntimeError(f'{len(missing)} von {len(required)} Exportdateien fehlen.\n{example}\n'
                           'Zur Vervollstaendigung (Nur-Lese-API):\n'
                           '  systemctl start nightscout-export-backfill.service\n'
                           'Danach denselben ZIP-Befehl erneut starten.')
    return required


def refresh():
    print('Aktualisiere die letzten Tage ueber den bestehenden Nur-Lese-Dienst ...', flush=True)
    try:
        subprocess.run(['systemctl', 'start', 'nightscout-export.service'],
                       check=True, timeout=240)
    except (subprocess.CalledProcessError, FileNotFoundError, subprocess.TimeoutExpired):
        raise RuntimeError('Aktualisierung fehlgeschlagen. Pruefe:\n'
                           '  journalctl -u nightscout-export.service -n 30 --no-pager\n'
                           'Es wurde noch keine neue ZIP-Datei erstellt.') from None


def identity(item: dict) -> str:
    identifier = item.get('_id') or item.get('identifier')
    if identifier is not None:
        return str(identifier)
    return json.dumps(item, sort_keys=True, ensure_ascii=False, separators=(',', ':'))


def write_json_collection(archive: zipfile.ZipFile, root: pathlib.Path,
                          name: str, days: list[dt.date]) -> tuple[int, int]:
    count, duplicates, seen = 0, 0, set()
    with archive.open(f'{name}.json', 'w') as stream:
        stream.write(b'[')
        first = True
        for day in days:
            rows = read_records(root / name / f'{day.isoformat()}.json.gz')
            for row in rows:
                key = identity(row)
                if key in seen:
                    duplicates += 1
                    continue
                seen.add(key)
                if not first:
                    stream.write(b',')
                stream.write(json.dumps(row, ensure_ascii=False, separators=(',', ':')).encode('utf-8'))
                count += 1
                first = False
        stream.write(b']')
    return count, duplicates


def create_zip(root: pathlib.Path, output_dir: pathlib.Path, days_count: int):
    start, today, days = export_window(days_count)
    verify_paths(root, days)
    if not output_dir.is_dir() or output_dir.is_symlink():
        raise RuntimeError(f'Ausgabeordner fehlt oder ist ein Symlink: {output_dir}')

    filename = f'nightscout-bericht-{days_count}tage-{start.isoformat()}-bis-{today.isoformat()}.zip'
    target = output_dir / filename
    fd, tmp_path = tempfile.mkstemp(prefix='.nightscout-bericht-', suffix='.tmp', dir=output_dir)
    os.close(fd)
    os.chmod(tmp_path, 0o600)
    try:
        with zipfile.ZipFile(tmp_path, mode='w', compression=zipfile.ZIP_DEFLATED,
                             compresslevel=6, allowZip64=True) as archive:
            counts, duplicate_counts = {}, {}
            for name in COLLECTIONS:
                counts[name], duplicate_counts[name] = write_json_collection(archive, root, name, days)
                print(f'  {name}: {counts[name]} Datensaetze', flush=True)
            profiles = read_records(root / 'profile' / 'all.json.gz')
            counts['profile'] = len(profiles)
            archive.writestr('profile.json', json.dumps(profiles, ensure_ascii=False,
                                                       separators=(',', ':')))
            print(f'  profile: {counts["profile"]} Datensaetze', flush=True)
            manifest = {
                'source': 'Lokaler Nightscout-Export (Nur-Lese-Dienst)',
                'data_directory': str(root),
                'exported_at_utc': dt.datetime.now(dt.timezone.utc).isoformat(),
                'from_utc_date': start.isoformat(),
                'through_utc_date': today.isoformat(),
                'days': days_count,
                'today_is_partial': True,
                'source_day_timezone': 'UTC',
                'analysis_timezone': str(TIMEZONE),
                'counts': counts,
                'duplicates_removed': duplicate_counts,
                'note': ('Private Gesundheitsdaten! Kein Zugangstoken enthalten. '
                         'GMI ist ein CGM-Schaetzwert und kein Labor-HbA1c. '
                         'Bitte Pumpen- und Sensorprotokolle vor Therapiebewertung abgleichen.'),
            }
            archive.writestr('manifest.json', json.dumps(manifest, ensure_ascii=False, indent=2))
        os.replace(tmp_path, target)
        os.chmod(target, 0o600)
    finally:
        if os.path.exists(tmp_path):
            os.unlink(tmp_path)
    print(f'\nFERTIG: {target}\nDateigroesse: {target.stat().st_size / 1024 / 1024:.1f} MiB')
    print('Diese ZIP-Datei ist privat (Dateirechte 600). Nicht auf GitHub hochladen!')
    print('Lade ausschliesslich die fertige ZIP-Datei hier in den Chat hoch.')
    return target


def main() -> int:
    parser = argparse.ArgumentParser(description='Nightscout-Berichts-ZIP aus lokalem Nur-Lese-Export')
    parser.add_argument('--days', type=int, default=90, help='Anzahl UTC-Tage einschliesslich heute (Standard: 90)')
    parser.add_argument('--source', type=pathlib.Path, default=SOURCE_DEFAULT,
                        help='Verzeichnis mit entries/, treatments/, devicestatus/, profile/')
    parser.add_argument('--output', type=pathlib.Path, default=OUTPUT_DEFAULT,
                        help='Zielverzeichnis fuer die ZIP-Datei (Standard: /root)')
    parser.add_argument('--refresh', action='store_true',
                        help='Vor dem Packen bestehenden stundenweisen Nur-Lese-Exportdienst ausfuehren')
    args = parser.parse_args()
    if not 1 <= args.days <= 366:
        parser.error('--days muss zwischen 1 und 366 liegen')
    os.umask(0o077)
    print('Nightscout ZIP-Bericht: lokale exportierte Daten, keine API-Zugangsdaten')
    if args.refresh:
        refresh()
    create_zip(args.source, args.output, args.days)
    return 0


if __name__ == '__main__':
    try:
        sys.exit(main())
    except (RuntimeError, OSError) as exc:
        print(f'\nFEHLER: {exc}', file=sys.stderr)
        sys.exit(1)
