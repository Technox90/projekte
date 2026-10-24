# START HERE – StatusAnzeige mit Arduino IDE

## 1. Dateien

Öffne:

`Arduino_IDE/StatusAnzeige/StatusAnzeige.ino`

## 2. ESP32 Core

Version **3.3.12**.

## 3. Board

`ESP32S3 Dev Module`

Alle Menüwerte: [BOARD_SETTINGS.md](BOARD_SETTINGS.md)

## 4. Bibliotheken

Siehe [LIBRARIES.md](LIBRARIES.md).

## 5. FTP-Fix

SimpleFTPServer muss `STORAGE_SD` statt `STORAGE_FFAT` benutzen.

Referenz:

`../Arduino_Library_Fix/FtpServerKey.h`

## 6. SD-Karte

Den kompletten Inhalt von:

`../SD_Karte/`

auf die FAT32-microSD kopieren.

## 7. Konfiguration

Vor dem ersten Flashen ist keine WLAN-Konfiguration im Sketch nötig. WLAN und weitere Einstellungen liegen auf der SD-Karte in:

`/config/config.json`

Alternativ nach dem Start über den Fallback-AP **StatusAnzeige-Setup** konfigurieren.

## 8. Kompilieren

In Arduino IDE zuerst **Überprüfen**, danach **Hochladen**.

Serial Monitor: **115200 Baud**.

## 9. Weboberfläche

Nach erfolgreicher Verbindung:

`http://statusanzeige.local/`

oder über die angezeigte IP-Adresse.
