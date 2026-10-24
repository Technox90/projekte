# StatusAnzeige To Go

Firmware fuer das Waveshare **ESP32-S3-Touch-LCD-2 (SKU 29667)** als portable Statusanzeige mit 2"-Touchdisplay, microSD und 13 adressierbaren LEDs.

## Hauptfunktionen

- 240x320 ST7789T3, 180° Gehaeuseausrichtung
- PNG-Statusbilder von microSD
- 13 WS2812/kompatible LEDs mit Statusfarbe und Effekten `static`, `pulse`, `blink`
- externer Taster und CST816D-Touchbedienung
- WLAN mit maximaler Sendeleistung, Auto-Reconnect und Fallback-AP/Captive Portal
- Weboberflaeche fuer Status, WLAN und Konfiguration
- Statusfarben/Effekte direkt im Browser aendern
- Akkuanzeige ueber BAT_ADC GPIO4
- WLAN-RSSI + Akku in oberer Statusleiste
- FTP-Zugriff auf die SD-Karte
- BLE-Steuerung
- HTTP-API fuer Home Assistant, Streamer.bot und eigene Automationen
- optional MQTT + Home Assistant Discovery (PubSubClient)
- letzter Status wird gespeichert
- Config-Backup (`config.bak`) vor Aenderungen
- Config-Download im Browser
- Firmwareupdate ueber Browser oder SD-Karte
- Systemdiagnose im Browser
- Safe Mode nach wiederholten Watchdog/Panic-Resets

## Weboberflaeche

- `/` Hauptseite / Status setzen
- `/wifi` WLAN suchen und aendern
- `/config` Geraetekonfiguration
- `/statuses` LED-Farben und Effekte je Status
- `/system` Systemdiagnose
- `/update` Firmware `.bin` hochladen
- `/config/backup` aktuelle `config.json` herunterladen

Standard-API-Passcode: `123456` (bitte aendern).

## Hardware-Pins

| Funktion | GPIO |
|---|---:|
| LCD SCLK | 39 |
| LCD MOSI | 38 |
| LCD MISO | 40 |
| LCD CS | 45 |
| LCD DC | 42 |
| LCD BL | 1 |
| SD CS | 41 |
| BAT_ADC | 4 |
| externe LEDs | 17 |
| externer Taster | 18 |
| Touch I2C SDA | 48 |
| Touch I2C SCL | 47 |

## Arduino IDE

Board: `ESP32S3 Dev Module`, Flash 16 MB, PSRAM `OPI PSRAM`, USB CDC an, Partition `Huge APP`.

Libraries:

- ArduinoJson
- GFX Library for Arduino
- Adafruit NeoPixel
- PNGdec
- SimpleFTPServer
- PubSubClient (nur fuer MQTT; Firmware kompiliert ohne diese Library ebenfalls, MQTT bleibt dann deaktiviert)

Wegen SimpleFTPServer muss fuer ESP32 `DEFAULT_STORAGE_TYPE_ESP32` auf `STORAGE_SD` stehen. Eine korrigierte Referenzdatei liegt unter `Arduino_Library_Fix/FtpServerKey.h`.

## SD-Karte

Den Inhalt von `SD_Karte/` auf eine FAT32-microSD kopieren. WLAN, Passwoerter und MQTT anschliessend in `config/config.json` oder ueber die Weboberflaeche setzen.

## Sicherheit

FTP und die lokale HTTP-Weboberflaeche sind fuer ein vertrauenswuerdiges LAN gedacht. Keine Portweiterleitung aus dem Internet auf Port 21/80 einrichten.


## 2.0.1 – Weboberfläche

- Statusbuttons nutzen jetzt `fetch()` statt direkter Navigation auf `/api/status`.
- Die Startseite bleibt bei Statuswechseln und Systemaktionen geöffnet.
- `Bild neu laden`, `Config neu laden` und `Neustart` laufen im Hintergrund.
- API-Passcode kann pro Browser-Sitzung auf der Startseite gemerkt werden.
- API-Passcode ist in `/config` direkt änderbar, inklusive Bestätigungsfeld.


## 2.0.2 – ESP32-Core 3.3.12 / GCC 14 Compile-Fix

Korrigiert wurde die Statusleisten-Höhe:

```cpp
const int h = min((int)cfg.statusBarHeight, (int)gfx->height());
```

`gfx->height()` liefert `int16_t`. GCC 14 kann bei `std::min()` nicht automatisch zwischen `int` und `int16_t` ableiten. Beide Werte werden deshalb explizit als `int` übergeben.
