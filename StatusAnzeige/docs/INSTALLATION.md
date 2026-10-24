# Installation

1. microSD als FAT32 formatieren und `SD_Karte/*` auf die Karte kopieren.
2. In `config/config.json` WLAN eintragen oder Fallback-AP benutzen.
3. Arduino IDE: ESP32 Core 3.3.x, Board `ESP32S3 Dev Module`, 16 MB Flash, OPI PSRAM, Huge APP.
4. Benoetigte Libraries installieren (siehe README).
5. Bei SimpleFTPServer die mitgelieferte `Arduino_Library_Fix/FtpServerKey.h` als Vergleich verwenden und fuer ESP32 `STORAGE_SD` setzen.
6. `Arduino_IDE/StatusAnzeige/StatusAnzeige.ino` kompilieren und flashen.
7. Danach `http://statusanzeige.local/` oder die angezeigte IP aufrufen.

Wenn das bekannte WLAN nicht erreichbar ist, startet der Fallback-AP `StatusAnzeige-Setup`. Das Captive Portal leitet auf `/wifi`.
