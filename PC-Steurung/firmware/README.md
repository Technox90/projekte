# Firmware – PC-Steuerung

[⬅ PC-Steuerung](../README.md) · [🏠 Projektübersicht](../../README.md) · [Dokumentation](../docs/README.md) · [Firmware-Prompt](../../promts/module/projekte--PC-Steurung--firmware.md)

ESPHome-Konfiguration für einen ESP32 zur PC-Power-Taster-Steuerung mit Statussignalen und lokaler Weboberfläche.

| Datei | Zweck |
|---|---|
| [pc-steuerung.yaml](pc-steuerung.yaml) | ESPHome-Firmware, API, WLAN, Webserver, Statussensoren und Schaltlogik |
| [secrets.example.yaml](secrets.example.yaml) | Vorlage für lokale Zugangsdaten – enthält keine produktiven Passwörter |

## Vor der Verwendung

1. [Hardware und Pinbelegung prüfen](../docs/02-anschlussplaene.md).
2. Lokale ESPHome-Secrets anhand der Vorlage anlegen; **keine echten Zugangsdaten committen**.
3. [Installation](../docs/03-installation.md), [Betriebslogik](../docs/04-betriebslogik.md) und [Prüfprotokoll](../docs/06-pruefprotokoll.md) beachten.
4. YAML mit ESPHome validieren und Firmware erst nach elektrischer Prüfung testen.

**Wichtig:** Ein langer Power-Tasterimpuls ist kein normgerechter sicherheitstechnischer Not-Aus. Keine ungetestete Schalthardware anschließen.
