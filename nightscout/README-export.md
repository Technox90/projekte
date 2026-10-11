# Nightscout-Datenexport (Plesk / Docker)

Der **read-only Nightscout-Exporter** ist ein unabhängiger Zusatzdienst zum vorhandenen Nightscout-Updater. Er verändert **nicht** Docker Compose, MongoDB, AAPS, Pumpe oder Insulinprofile.

**Wichtig:** Das öffentliche Repository darf weder Nightscout-Token noch Glukose-, Insulin- oder Gesundheitsdaten enthalten. Nur Skripte und Dokumentation einchecken.

## Bestehende Installation

- Nightscout: Docker unter `/opt/nightscout` (lokal `127.0.0.1:1337`).
- Exportdaten: `/opt/nightscout-export/data`
- Separater Nur-Lese-Token: `/opt/nightscout-export/config/read-token` (nicht in Git!)
- Benutzer: `nightscout-exp`
- Erstexport: 90 Tage, danach stündlich die letzten drei UTC-Tage erneut.
- Datenbereiche: `entries`, `treatments`, `devicestatus`, `profile`.

### Prüfen (bei bereits installiertem Exporter)

```bash
systemctl show nightscout-export-backfill.service \
  -p ActiveState -p Result -p ExecMainStatus
systemctl list-timers --all nightscout-export.timer
journalctl -u nightscout-export.service -n 30 --no-pager
du -sh /opt/nightscout-export/data
```

**Die bereits erfolgreich laufende Installation nicht unnötig erneut ausführen!** Ein erneuter Installer-Aufruf ersetzt Skript und systemd-Units und startet den 90-Tage-Export erneut.

## Erstinstallation auf einem noch nicht eingerichteten Server

Voraussetzungen: Debian/Plesk, `root`, Python 3, systemd, lokales Nightscout auf Port 1337. In Nightscout zuvor ein separates Subject mit Rolle `readable` anlegen. Nur den Token (nicht den vollständigen Link) mit Zugriff nur für root speichern:

```bash
install -d -m 700 /opt/nightscout-export/config
umask 077
read -r -s -p 'Nightscout-Nur-Lese-Token: ' NS_TOKEN; echo
printf '%s' "$NS_TOKEN" > /opt/nightscout-export/config/read-token
unset NS_TOKEN
chmod 600 /opt/nightscout-export/config/read-token
```

Installer laden, Inhalt prüfen, dann ausführen:

```bash
curl --fail --location --proto '=https' --tlsv1.2 \
  https://raw.githubusercontent.com/Technox90/projekte/main/nightscout/nightscout-export-install.sh \
  -o /root/nightscout-export-install.sh
bash -n /root/nightscout-export-install.sh
less /root/nightscout-export-install.sh
bash /root/nightscout-export-install.sh
```

Der Installer führt zunächst einen eintägigen Test aus und aktiviert Timer und 90-Tage-Erstexport nur nach erfolgreichem Test. Die Python-Bibliothek ist im Skript enthalten; zusätzliche pip-Pakete sind nicht erforderlich.

## Dateiformat

```text
/opt/nightscout-export/data/
  entries/YYYY-MM-DD.json.gz
  treatments/YYYY-MM-DD.json.gz
  devicestatus/YYYY-MM-DD.json.gz
  profile/all.json.gz
```

Die Exporte sind nach **UTC-Kalendertagen** aufgeteilt. Der Service verwendet ausschließlich lesende HTTP-GET-Anfragen mit einem JWT aus dem Nur-Lese-Token. Das Programm verwirft fehlerhafte API-Ergebnisse und verhindert parallele Läufe.

### Hinweise

- Der Export ersetzt kein MongoDB-Backup.
- Zeitfilter und API-Ergebnislimits werden geprüft; bei überfüllten Zeitfenstern erfolgt eine Aufteilung in kleinere Zeitfenster. Bei nicht eindeutig abrufbaren Einträgen stoppt das Programm, statt einen vollständigen Export vorzutäuschen.
- Profildaten werden bis zu 999 Einträgen unterstützt, ab 1000 wird abgebrochen.
- Die **öffentlich lesbare Nightscout-API ist durch dieses Skript nicht abgesichert**. Erst die AAPS- und Client-Berechtigungen prüfen, dann den öffentlichen Zugriff separat sperren.
- Gesundheitsdaten niemals ins öffentliche GitHub-Repository hochladen.
- Auswertungen sind keine automatische Dosierungssteuerung oder Ersatz für medizinische Beratung.
