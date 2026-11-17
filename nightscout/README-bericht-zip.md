# Nightscout: 90-Tage-ZIP fuer Diabetesbericht

Dieses Werkzeug packt die **vorhandenen lokalen Nightscout-Datenexporte** zu einer ZIP-Datei fuer eine Diabetes-Auswertung.

- Quelle: /opt/nightscout-export/data (bestehender Nur-Lese-Dienst)
- Inhalt: entries.json, treatments.json, devicestatus.json, profile.json, manifest.json
- Zeitraum: standardmaessig **90 UTC-Kalendertage einschliesslich heute** (heute evtl. unvollstaendig)
- Ausgabe: /root/nightscout-bericht-90tage-YYYY-MM-DD-bis-YYYY-MM-DD.zip
- **Keine Gesundheitsdaten oder Tokens auf GitHub!**
- Es werden keine Pumpen-, AAPS-, Nightscout- oder Insulineinstellungen veraendert.
- Voraussetzungen: Python 3.9+ und bestehender Nightscout-Exportdienst

## Ein Befehl zum Kopieren (SSH als root)

Der Befehl laedt die aktuelle Skriptdatei aus GitHub, prueft die Python-Syntax und aktualisiert die letzten Exporttage mit dem vorhandenen Nur-Lese-Dienst, bevor er die ZIP erzeugt.

```bash
curl -fLsS --retry 3 --proto '=https' --tlsv1.2 \
  https://raw.githubusercontent.com/Technox90/projekte/main/nightscout/nightscout-bericht-zip.py \
  -o /root/nightscout-bericht-zip.py \
  && python3 -m py_compile /root/nightscout-bericht-zip.py \
  && python3 /root/nightscout-bericht-zip.py --refresh
```

Der Code ist hier einsehbar: [nightscout-bericht-zip.py](nightscout-bericht-zip.py). Das Herunterladen und Ausfuehren als root sollte nur nach Sichtpruefung erfolgen.

## Falls Tagesdateien fehlen

Es wird **kein unvollstaendiges Archiv** erzeugt. Der bereits installierte Dienst kann die letzten 90 Tage erneut lesend abrufen:

```bash
systemctl start nightscout-export-backfill.service
```

Anschliessend den GitHub-Befehl oben erneut ausfuehren.

## ZIP finden und herunterladen

```bash
ls -lh /root/nightscout-bericht-*.zip
```

Der Programmausdruck nennt den Dateipfad. Die ZIP mit MobaXterm/SFTP auf den eigenen Rechner laden und **nur im privaten Chat** zur Auswertung hochladen.

## Optionen

```bash
python3 /root/nightscout-bericht-zip.py --days 30
python3 /root/nightscout-bericht-zip.py --days 90 --refresh
python3 /root/nightscout-bericht-zip.py --help
```

Der Exportdienst speichert nach **UTC-Tagesdateien**. Fuer die Auswertung koennen die Messzeitpunkte nach Europe/Berlin umgerechnet werden. Ein geschaetzter GMI ist kein Labor-HbA1c.

Die ZIP enthaelt **sensible Gesundheitsdaten**, hat standardmaessig Dateirechte 0600 und darf nicht in ein oeffentliches Repository hochgeladen werden. Das Skript selbst enthaelt und benoetigt keine Zugangsdaten; der lokale Dienst verwaltet seine Leseberechtigung separat. Es handelt sich um ein Exportwerkzeug, nicht um eine Therapieempfehlung.
