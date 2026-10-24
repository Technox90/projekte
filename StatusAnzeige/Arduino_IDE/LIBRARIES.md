# Benötigte Arduino-Bibliotheken

Über **Werkzeuge → Bibliotheken verwalten** installieren:

| Bibliothek | Autor |
|---|---|
| ArduinoJson | Benoit Blanchon |
| GFX Library for Arduino | Moon On Our Nation |
| Adafruit NeoPixel | Adafruit |
| PNGdec | Larry Bank / bitbank2 |
| SimpleFTPServer | Renzo Mischianti / xreef |
| PubSubClient | Nick O'Leary – optional für MQTT/Home Assistant |

Vom ESP32-Core bereitgestellt:

- WiFi
- WebServer
- DNSServer
- SD
- FS
- Update
- ESPmDNS
- BLE
- Wire
- SPI

## Wichtig: SimpleFTPServer

Die Bibliothek verwendet bei ESP32 standardmäßig je nach Version FFAT. Für dieses Projekt muss die normale SD-Bibliothek verwendet werden:

```cpp
#define DEFAULT_STORAGE_TYPE_ESP32 STORAGE_SD
```

Eine passende Referenz liegt unter:

`../Arduino_Library_Fix/FtpServerKey.h`

## Mehrere SD.h-Bibliotheken

Eine Arduino-Meldung wie:

```text
Mehrere Bibliotheken wurden für "SD.h" gefunden
Benutzt: ...packages\esp32\hardware\esp32\3.3.12\libraries\SD
```

ist in Ordnung, solange die **SD-Bibliothek aus dem ESP32-Core** als „Benutzt“ angezeigt wird.
