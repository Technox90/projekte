# Arduino IDE – Boardeinstellungen

Getestete Zielkonfiguration für **Waveshare ESP32-S3-Touch-LCD-2** und **Arduino ESP32 Core 3.3.12**.

| Einstellung | Wert |
|---|---|
| Board | ESP32S3 Dev Module |
| Port | der erkannte USB/COM-Port |
| USB CDC On Boot | Enabled |
| CPU Frequency | 240MHz (WiFi) |
| Core Debug Level | None |
| USB DFU On Boot | Disabled |
| Erase All Flash Before Sketch Upload | Disabled |
| Events Run On | Core 1 |
| Flash Mode | QIO 80MHz |
| Flash Size | 16MB (128Mb) |
| JTAG Adapter | Disabled |
| Arduino Runs On | Core 1 |
| USB Firmware MSC On Boot | Disabled |
| Partition Scheme | Huge APP (3MB No OTA/1MB SPIFFS) |
| PSRAM | OPI PSRAM |
| Upload Mode | UART0 / Hardware CDC |
| Upload Speed | 921600 |
| USB Mode | Hardware CDC and JTAG |
| Zigbee Mode | Disabled |

## Warum Huge APP?

Die Firmware enthält Weboberfläche, BLE, FTP, PNG-Decoding, Touch, MQTT-Unterstützung und weitere Funktionen. Die normale kleine App-Partition reicht dafür nicht aus.

## Serial Monitor

Baudrate:

`115200`

Bei Problemen immer den kompletten Boot-Log ab Reset mitsenden.
