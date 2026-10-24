# StatusAnzeige To Go

Eigenständiges ESP32-S3-Projekt für das **Waveshare ESP32-S3-Touch-LCD-2 (240×320)** mit microSD, 13 adressierbaren LEDs, Akkuanzeige, Touch, WLAN, BLE, FTP, Weboberfläche, HTTP-API und optional MQTT/Home Assistant.

> **Aktueller Firmware-Stand:** 2.0.2  
> Arduino-IDE-Einstieg: `Arduino_IDE/StatusAnzeige/StatusAnzeige.ino`

## Schnellstart

1. Den Ordner `Arduino_IDE/StatusAnzeige/` herunterladen.
2. `StatusAnzeige.ino` mit Arduino IDE öffnen.
3. ESP32-Boardpaket **3.3.12** verwenden.
4. Die Einstellungen aus `Arduino_IDE/BOARD_SETTINGS.md` übernehmen.
5. Die Bibliotheken aus `Arduino_IDE/LIBRARIES.md` installieren.
6. Bei SimpleFTPServer die Datei aus `Arduino_Library_Fix/FtpServerKey.h` verwenden bzw. `STORAGE_SD` einstellen.
7. Den Inhalt von `SD_Karte/` auf eine FAT32-microSD kopieren.
8. `SD_Karte/config/config.json` anpassen oder nach dem ersten Start über die Weboberfläche konfigurieren.
9. Firmware kompilieren und flashen.

## Projektstruktur

```text
StatusAnzeige/
├── Arduino_IDE/
│   ├── README.md
│   ├── BOARD_SETTINGS.md
│   ├── LIBRARIES.md
│   └── StatusAnzeige/
│       └── StatusAnzeige.ino
├── Arduino_Library_Fix/
│   └── FtpServerKey.h
├── SD_Karte/
│   ├── README.md
│   ├── config/
│   │   ├── config.json
│   │   └── config.example.json
│   ├── images/
│   ├── firmware/
│   ├── data/
│   └── logs/
├── PlatformIO/
├── docs/
│   ├── INSTALLATION.md
│   ├── WEB.md
│   ├── HOME_ASSISTANT.md
│   ├── STREAMERBOT.md
│   ├── TOUCH.md
│   └── CHANGELOG.md
└── README.md
```

## Weboberfläche

Nach erfolgreicher WLAN-Verbindung:

- `http://statusanzeige.local/` – Status setzen
- `/wifi` – WLAN konfigurieren
- `/config` – Geräteeinstellungen
- `/statuses` – Statusfarben und LED-Effekte
- `/system` – Diagnose
- `/update` – Firmwareupdate
- `/config/backup` – Config herunterladen
- `/config/restore-page` – Config wiederherstellen

Statuswechsel und Systemaktionen werden im Hintergrund per `fetch()` ausgeführt; die Weboberfläche bleibt geöffnet.

## Hardware-Pins

| Funktion | GPIO |
|---|---:|
| LCD SCLK | 39 |
| LCD MOSI | 38 |
| LCD MISO | 40 |
| LCD CS | 45 |
| LCD DC | 42 |
| LCD Backlight | 1 |
| SD CS | 41 |
| BAT_ADC | 4 |
| LED DATA | 17 |
| Taster | 18 |
| Touch SDA | 48 |
| Touch SCL | 47 |

## Wichtige Arduino-Einstellungen

Kurzfassung:

- Board: **ESP32S3 Dev Module**
- ESP32 Core: **3.3.12**
- Flash Size: **16MB (128Mb)**
- Partition Scheme: **Huge APP (3MB No OTA/1MB SPIFFS)**
- PSRAM: **OPI PSRAM**
- USB CDC On Boot: **Enabled**
- Flash Mode: **QIO 80MHz**
- CPU Frequency: **240MHz (WiFi)**
- Upload Speed: **921600**

Alle Einstellungen stehen vollständig in `Arduino_IDE/BOARD_SETTINGS.md`.

## Sicherheit

Die Standardwerte in `config.json` sind nur Startwerte. Nach Inbetriebnahme bitte insbesondere ändern:

- API-Passcode
- FTP-Passwort
- Fallback-AP-Passwort
- WLAN-Zugangsdaten

FTP und HTTP sind für das lokale LAN gedacht. Keine Portweiterleitung aus dem Internet auf Port 21 oder 80 einrichten.


## Optionale Funktionen

Ab Firmware **2.1.0** gibt es die Seite `/features`.

Dort lassen sich vorhandene Zusatzfunktionen einzeln aktivieren oder deaktivieren. Jede Option enthält direkt eine kurze Erklärung. Noch nicht programmierte Ideen werden separat unter **Geplant** angezeigt und sind bewusst deaktiviert, damit keine wirkungslosen Schalter entstehen.

Aktuell schaltbar sind u. a. BLE, FTP, MQTT, Home Assistant Discovery, Touch, Uhr, Statusleiste, Fallback-Portal, Web-/SD-Update, letzter Status, Akku-Schutz, LED-Effekte und Bestätigungsblitz.
