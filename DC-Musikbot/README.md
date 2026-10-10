# DC-Musikbot – BBQ-Chaos-Deutschland

Discord-Radiobot fuer den bestehenden **AzuraCast-Stream** auf dem **Hetzner-Server**. Ohne Proxmox, ohne zusaetzlichen Docker-Container und ohne Veraenderung der AzuraCast-Konfiguration.

**Stream:** `https://stream.bopzocker.de/listen/chaos/radio.mp3`

## Voraussetzungen

- Hetzner-Server mit Debian/Ubuntu, root-Zugang und Internetzugang
- Discord-Bot aus dem [Discord Developer Portal](https://discord.com/developers/applications)
- Auf dem Zielserver erlaubte ausgehende Verbindungen zu Discord und zum Stream
- Bot-Berechtigungen im gewuenschten Sprachkanal: Kanal ansehen, Verbinden, Sprechen
- Kanal-ID (Discord Entwicklermodus -> Rechtsklick auf Kanal -> ID kopieren)
- Token niemals auf GitHub veroeffentlichen!

## Installation

Auf dem **Hetzner-Host** als root:

```bash
install -d -m 700 /root/installer
curl -fsSL https://raw.githubusercontent.com/Technox90/projekte/main/DC-Musikbot/install.sh -o /root/installer/install-bbq-radio.sh
chmod 700 /root/installer/install-bbq-radio.sh
bash /root/installer/install-bbq-radio.sh
```

Installer fragt nach Kanal-ID, optionaler Stream-URL und Token (verdeckte Eingabe). Laedt `bot.py` und `requirements.txt` aus diesem Ordner nach `/opt/bbq-chaos-radiobot`, legt einen eigenen Systembenutzer an und aktiviert `bbq-chaos-radio.service`.

**Sicherheit:** Fuehre Downloads nur aus, wenn du dem Repository vertraust. Die echte `radio.env` bleibt auf dem Server (`/opt/bbq-chaos-radiobot/radio.env`, Rechte 0600) und gehoert nicht ins Repository.

## Verwaltung

```bash
systemctl status bbq-chaos-radio
journalctl -u bbq-chaos-radio -f
systemctl restart bbq-chaos-radio
systemctl stop bbq-chaos-radio
systemctl start bbq-chaos-radio
```


## Songanzeige im Sprachkanal-Textchat (NowPlaying)

Der Bot liest standardmaessig `https://stream.bopzocker.de/api/nowplaying/chaos` alle 20 Sekunden aus. Er sendet **ein** Discord-Embed in den Textchat des konfigurierten Sprachkanals und **bearbeitet** die bestehende Nachricht bei Song-/Status-/Hoererwechsel. Die Nachricht-ID steht dauerhaft in `/opt/bbq-chaos-radiobot/nowplaying-message.json` und uebersteht Neustarts.

Statusanzeige untereinander: **🟢 Online** (AutoDJ), **🔴 Live** (Live-DJ), **⚫ Offline** (AzuraCast meldet nicht online). Auch beim Wechsel zwischen AutoDJ und Live wird dieselbe Nachricht bearbeitet. Falls die API nicht erreichbar ist, bleibt die letzte Anzeige bestehen und im Journal erscheint ein Fehler.\n\nErforderliche **zusaetzliche** Kanalberechtigungen: **Nachrichten senden**, **Links einbetten**, **Nachrichtenverlauf lesen**. Kein Webhook und kein neuer Discord-Token noetig.

### Bestehende Hetzner-Installation aktualisieren

```bash
install -d -m 700 /root/installer
curl -fsSL https://raw.githubusercontent.com/Technox90/projekte/main/DC-Musikbot/update.sh -o /root/installer/update-bbq-radio.sh
chmod 700 /root/installer/update-bbq-radio.sh
bash /root/installer/update-bbq-radio.sh
journalctl -u bbq-chaos-radio -f
```

Das Skript legt ein Backup an und laesst `radio.env` und AzuraCast unveraendert. Wenn ein Discord-Voice-Handshake noch fehlschlaegt, muss die Audioverbindung separat diagnostiziert werden; die Songanzeige wird unabhaengig davon betrieben.

## Verhalten

- Start beim Serverboot via systemd
- Dauerhafter Aufenthalt im Sprachkanal, auch ohne Zuhoerer
- Discord-Reconnect und alle 15 Sekunden Watchdog-Pruefung
- FFmpeg mit Stream-Reconnect; bei Wiedergabeende wird eine neue FFmpeg-Quelle gestartet
- Wiederanlauf des Dienstes nach Prozessabsturz (10 Sekunden)
- Keine Veraenderung an `/var/azuracast` oder Docker

**Hinweis:** Nach Aenderungen an der Kanal-ID oder dem Token `radio.env` als root aktualisieren und Dienst neu starten. Der Installer ueberschreibt bestehende Installationen absichtlich nicht. Beim allerersten Start die Logs auf Voice-/FFmpeg-Fehler pruefen. Musik-/Senderechte muessen auch die Weitergabe ueber Discord abdecken.

## Dateien

- `install.sh`: Debian/Ubuntu-Installer
- `bot.py`: 24/7-Radiobot mit Songanzeige
- `nowplaying.py`: einzelnes persistentes Song-Embed
- `update.sh`: Update der bereits installierten Hetzner-Version
- `requirements.txt`: Python-Abhaengigkeiten
- `radio.env.example`: Musterkonfiguration ohne echte Zugangsdaten
- `.gitignore`: verhindert versehentliches Committen lokaler Geheimnisse
