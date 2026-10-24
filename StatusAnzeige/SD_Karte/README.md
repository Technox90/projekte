# microSD-Karte vorbereiten

Empfohlen: FAT32.

Den Inhalt dieses Ordners direkt in das Stammverzeichnis der microSD kopieren:

```text
/
├── config/
│   └── config.json
├── data/
├── firmware/
├── images/
└── logs/
```

## Konfiguration

`config/config.json` enthält WLAN, FTP, API, Display, LEDs, Akku, Touch, MQTT und alle Statusdefinitionen.

Die Datei kann später auch über die Weboberfläche geändert, gesichert und wiederhergestellt werden.

## Firmwareupdate über SD

Für ein Update:

1. kompilierte `firmware.bin` nach `/firmware/firmware.bin`
2. leere Datei `/firmware/update.flag` anlegen
3. Gerät neu starten

Nach erfolgreichem Update wird die Firmware zu `firmware.done.bin` umbenannt.
