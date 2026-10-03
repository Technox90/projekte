# Nightscout-Updater (Plesk)

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
