# Arduino IDE

Die fertige Arduino-Sketchdatei liegt hier:

`StatusAnzeige/StatusAnzeige.ino`

## Ablauf

1. Arduino IDE 2.x öffnen.
2. ESP32 Core 3.3.12 installieren.
3. Bibliotheken laut `LIBRARIES.md` installieren.
4. Boardeinstellungen laut `BOARD_SETTINGS.md` setzen.
5. `StatusAnzeige.ino` öffnen.
6. USB-Port des ESP32-S3 auswählen.
7. **Überprüfen** ausführen.
8. Danach **Hochladen**.

Die Sketchdatei muss in einem Ordner namens `StatusAnzeige` liegen. Genau deshalb ist die Struktur hier bereits Arduino-kompatibel.

## SD-Karte

Vor dem ersten Start den Inhalt von `../../SD_Karte/` auf eine FAT32-microSD kopieren.

## SimpleFTPServer

Für ESP32 muss die Bibliothek die Standard-SD-Bibliothek verwenden. Die passende Referenzdatei liegt in:

`../../Arduino_Library_Fix/FtpServerKey.h`

Dort muss gelten:

```cpp
#define DEFAULT_STORAGE_TYPE_ESP32 STORAGE_SD
```
