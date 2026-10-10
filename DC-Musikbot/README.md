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

## Discord Slash-Commands

Berechtigungen: Administratoren oder Mitglieder mit **Nachrichten verwalten** oder **Server verwalten** duerfen /radio steuern. Musikwunschbefehle stehen allen Mitgliedern dieses Servers offen. Moderatorenrollen ohne eines dieser Rechte muessen entsprechend berechtigt werden.

- `/radio lautstaerke prozent:10` – live Lautstaerke einstellen (0–100 %)
- `/radio lauter` und `/radio leiser` – jeweils 5 Prozentpunkte
- `/radio pause`, `/radio weiter`, `/radio neustart`
- `/radio status` – Discord-Status und Lautstaerke
- `/musik suchen suchbegriff:...` – suchbare Songs und Request-IDs anzeigen
- `/musik wuenschen song_id:...` – Song an AzuraCast uebermitteln

Der Bot registriert die globalen Befehle beim Start; die Anzeige in Discord kann daher etwas dauern. Der Bot benoetigt den `applications.commands`-Scope. Musikwunschfunktion in den AzuraCast-Sendereinstellungen aktivieren. Auf Wunsch API-Schluessel **nur lokal** in `/opt/bbq-chaos-radiobot/radio.env` hinterlegen (`AZURACAST_API_KEY="..."`). Er wird als Bearer-Token gesendet. Mit einem API-Schluessel koennen andere AzuraCast-Regeln fuer die Anfrage gelten; daher moeglichst eng eingeschraenkte Berechtigungen nutzen.

Der 10-Minuten-Cooldown pro Discord-Mitglied startet nur bei erfolgreicher Musikwunschanfrage und wird in `requests-state.json` ueber Neustarts hinweg gespeichert. Bei Ablehnung durch AzuraCast beginnt kein Cooldown. Die eigentliche Wiedergabe wird weiterhin durch AzuraCast geplant. Die private Antwort zeigt Suchergebnisse mit ihrer Song-ID; `/musik wuenschen` akzeptiert diese ID.

Die Lautstaerke startet fuer neue Installationen bei 10 % (`RADIO_VOLUME=0.1`) und wird anschliessend unabhaengig von der env-Datei in `volume-state.json` persistiert. Bei bestehenden Installationen ohne diese Datei wird zuerst die vorhandene Einstellung aus `radio.env` verwendet. Um auf die Env-Vorgabe zurueckzusetzen, die Datei `volume-state.json` bei gestopptem Bot loeschen. Die Audiosignal-Lautstaerke wird direkt zur Laufzeit angepasst.

Wichtig: Der Discord-Bot veraendert keine AzuraCast-Dienste und ueberspringt keine Lieder. `/radio neustart` startet nur die Discord-Audiowiedergabe neu.

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
- `bot.py`: 24/7-Radiobot mit Songanzeige und Slash-Commands\n- `commands.py`: Slash-Commands, Musiksuche und Request-Cooldown
- `nowplaying.py`: einzelnes persistentes Song-Embed
- `update.sh`: Update der bereits installierten Hetzner-Version
- `requirements.txt`: Python-Abhaengigkeiten
- `radio.env.example`: Musterkonfiguration ohne echte Zugangsdaten
- `.gitignore`: verhindert versehentliches Committen lokaler Geheimnisse
