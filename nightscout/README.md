# Nightscout-Updater (Plesk)

**Navigation:** [🏠 Alle Projekte](../README.md) · [📤 Export-Anleitung](README-export.md) · [📝 Nightscout-Prompt](../promts/module/projekte--nightscout.md) · [🔄 Updater](nightscout-updater-install.sh) · [📥 Export-Installer](nightscout-export-install.sh)

> **Kurzüberblick:** Nightscout-Patchupdates mit Sicherung sowie separater Nur-Lese-Export. Zugangsdaten und Gesundheitsdaten bleiben lokal.


**Navigation:** [🏠 Projektübersicht](../README.md) · [Dateien](./) · [Projekt-Prompt](../promts/module/projekte--nightscout.md)

### 📁 Nightscout-Werkzeuge

| Datei | Aufgabe |
|---|---|
| [nightscout-updater-install.sh](nightscout-updater-install.sh) | Installation des wöchentlichen Patch-Updaters |
| [nightscout-export-install.sh](nightscout-export-install.sh) | Installation des Nur-Lese-Exports |
| [README-export.md](README-export.md) | Ausführliche Beschreibung des Exports |

**Sicherheit:** Produktive Daten und API-Geheimnisse nicht auf GitHub ablegen. Anleitungen zur geprüften Installation folgen unten.


Woechentlicher Nightscout-Patch-Updater mit Backup, Health-Checks und Image-Rollback. **Die produktive MongoDB wird nicht aktualisiert.**

## Installation auf Plesk (root)

Nur verwenden, wenn der aktuelle Nightscout-Container und die API lokal sowie ueber HTTPS erreichbar sind.

```bash
set -e
install -d -m 700 /root/downloads
curl -fL --retry 3 -o /root/downloads/nightscout-updater-install.sh \
  https://raw.githubusercontent.com/Technox90/projekte/231c648e6658cdb34dcb92aeb4e69b4664ce8412/nightscout/nightscout-updater-install.sh
echo '48d0164a2c43417d4b2a953454742c201d798fcaf158aafcb7153bfe70188821  /root/downloads/nightscout-updater-install.sh' | sha256sum -c -
bash -n /root/downloads/nightscout-updater-install.sh
bash /root/downloads/nightscout-updater-install.sh
```

Das Skript legt den eigentlichen Updater unter `/usr/local/sbin/nightscout-weekly-update` sowie einen systemd-Dienst und einen Timer an. Der Zeitplan ist Sonntag um 11:00 Uhr (Europe/Berlin). Wenn die Nightscout-API nicht erreichbar ist, wird der Timer nicht aktiviert.

## Pruefung

```bash
systemctl list-timers --all nightscout-weekly-update.timer
journalctl -u nightscout-weekly-update.service -n 100 --no-pager
```

## Aktualisierung des Installationsskripts

Bei kuenftigen GitHub-Aenderungen das Skript erst pruefen und den neuen Commit und SHA-256-Wert festhalten. Es wird **nicht ungeprueft automatisch aus GitHub als root ausgefuehrt**. Das neue Installationsskript kann nach der Pruefung auf dieselbe Weise wie oben installiert werden.

## Grenzen

* Es werden nur Patch-Upgrades innerhalb einer Major.Minor-Reihe zugelassen (z.B. 15.0.8 -> 15.0.9); Minor-/Major-Upgrades brauchen eine manuelle Pruefung.
* Vor tatsaechlichen Updates wird eine lokale Sicherung angelegt. Sie enthaelt auch sensible Konfigurationsdaten und wird **nicht** auf GitHub gespeichert.
* Das Image-Rollback ersetzt keinen getesteten Restore der Datenbank, und ein HTTP-200-Test prueft nicht alle AAPS-Synchronisierungsvorgaenge.
* Ein produktiver Nightscout-Ausfall kann die Cloud-Synchronisierung und verknuepfte Alarme unterbrechen; CGM-/Pumpenueberwachung muss davon unabhaengig funktionieren.

---

## Nightscout-Datenexport (Nur-Lese-Dienst)

Zusätzlich zum wöchentlichen Updater gibt es jetzt einen eigenständigen Exporter.
Er ruft Glukosewerte, Behandlungen, Gerätestatus und Profile über die lokale
Nightscout-API ausschließlich lesend ab, legt JSON.gz-Dateien unter
`/opt/nightscout-export/data` ab und aktualisiert sie stündlich.

- Installer: [nightscout-export-install.sh](nightscout-export-install.sh)
- Anleitung: [README-export.md](README-export.md)

**Bei bereits installiertem Exportdienst ist keine Neuinstallation nötig.**
Tokens und Gesundheitsdaten niemals in dieses öffentliche GitHub-Repository hochladen.

---

## Lebensmittel-Datenimport / Food data import (DE / EN)

Für den AAPS-Reiter **ESSEN / FOOD** gibt es jetzt einen separaten CSV-Importer mit deutscher
und englischer Vorlage, Duplikatprüfung, Offline-Vorschau und manueller Importfreigabe.

- [Lebensmittelimport öffnen](food-import/)
- [Deutsch: Anleitung](food-import/README-DE.md) · [Vorlage DE](food-import/template-de.csv)
- [English: Instructions](food-import/README-EN.md) · [Template EN](food-import/template-en.csv)
- [Python-Importprogramm](food-import/import-food.py)

Das Programm verändert keine AAPS-Therapieprofile, Insulindosierungen oder Pumpenbefehle.
Es benötigt einen **separaten Token mit ausschließlich Leserechten und Erstellungsrechten für die Sammlung food**.
Der bestehende Nur-Lese-Exporter bleibt unverändert. Keine Tokens oder persönlichen Daten auf GitHub speichern.
