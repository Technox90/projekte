# Installation in ESPHome und Home Assistant

## Voraussetzungen

ESP32 ESP-WROOM-32 oder kompatibles ESP32-Dev-Board, 2,4-GHz-WLAN, ESPHome Device Builder, Home Assistant, USB-Datenkabel und ausreichende Versorgung. Zusatzmodule bis zur ersten erfolgreichen Firmwareinstallation **abgeklemmt lassen**.

## 1. Neutrale Gerätenamen

Voreinstellung in der [Firmware](../firmware/pc-steuerung.yaml):

~~~yaml
esphome:
  name: pc-steuerung
  friendly_name: PC Steuerung
~~~

Bei mehreren PCs eindeutige, ebenfalls neutrale Hostnamen benutzen, etwa pc-steuerung-01 und pc-steuerung-02. Für jedes Gerät vorzugsweise eigene API-Schlüssel und OTA-Passwörter erzeugen.

## 2. Lokale Secrets

Die [Beispieldatei](../firmware/secrets.example.yaml) enthält nur Platzhalter. Die realen Werte gehören in die **lokale** ESPHome-Secrets-Verwaltung, niemals auf GitHub.

| Secret | Inhalt |
|---|---|
| wifi_ssid | WLAN-SSID |
| wifi_password | WLAN-Kennwort |
| pc_fallback_password | Starkes Fallback-WLAN-Kennwort, mindestens acht Zeichen |
| api_encryption_key | Zufälliger, Base64-kodierter 32-Byte-Schlüssel |
| ota_password | Starkes, gerätespezifisches OTA-Passwort |

Den API-Schlüssel mit ESPHome generieren lassen oder einen sicheren Zufallszahlengenerator verwenden. Echte Passwörter nicht als Screenshots/Logs veröffentlichen.

## 3. Erstflash per USB

1. ESP32 alleine anschließen; keinen PC-PWR_SW-Kontakt verbinden.
2. YAML im ESPHome Device Builder einfügen.
3. **Konfiguration überprüfen** ausführen.
4. Firmware kompilieren.
5. Über USB (oder über https://web.esphome.io/ mit der heruntergeladenen Firmware) flashen.
6. Im seriellen Log prüfen, ob WLAN verbunden ist und eine IP vergeben wird.
7. Unter Home Assistant → Einstellungen → Geräte & Dienste → ESPHome das Gerät mit dem API-Schlüssel verbinden.
8. Nach stabiler API-Verbindung Relais und PC817 **schrittweise** gemäß Prüfprotokoll testen.

Diese Veröffentlichung wurde nicht lokal mit einer installierten ESPHome-Build-Umgebung oder realer Hardware end-to-end getestet. Die Validierung und das Kompilieren sind vor dem Einbau erforderlich.

## 4. Test des Relais ohne Mainboard

Beim Booten und Neustart darf das Relais nicht selbstständig anziehen. Die Firmware setzt ALWAYS_OFF, dies garantiert bei jeder Modulvariante jedoch noch keinen glitchfreien Boot-Vorgang. Mit geeignetem Messgerät COM–NO beobachten. Home-Assistant-Button **PC Power** drücken: Kontakt muss **nur etwa 500 ms** schließen. Auch den GPIO18-Taster separat testen.

**Erst danach** PWR_SW am Mainboard verbinden.

## 5. Statussensoren

Vor Anschluss des 12-V-Ausgangs an GPIO22 sicherstellen, dass PC817 U1 gegen ESP32-GND stets **0 bis 3,3 V** führt. In der YAML wird bei anliegenden 12 V ein **aktiv-LOW-Signal** vorausgesetzt. Verhalten mit **12 V AUS** und **12 V EIN** verifizieren.

Der Mainboard-PLED-Eingang darf **nicht ungeprüft direkt** an ein 3,6–30-V-Modul: Eingangsspannung, Polarität, Strombelastbarkeit und LED-Verhalten messen; passende Signalaufbereitung auslegen. Die beiden Ausgänge sind in HA getrennte Sensoren.

## 6. Home-Assistant-Entitäten

- PC Power: 500-ms-Power-Taster.
- PC Ausschalten erzwingen (10s): erzwungenes Herunterfahren bei geprüftem PC-EIN-Status.
- PC 12V Versorgung: geschaltete Haupt-12 V erkannt.
- PC Power LED: separat erfasste Mainboard-LED.
- Onboard LED Modus: Auswahl eines der sieben Modi.
- Ausschalten erzwingen aktiv: Warnstatus.
- ESP32 Online, WLAN Signal, ESP32 Laufzeit, ESP32 Neustart.

## 7. OTA-Updates

Nach erfolgreichem Flash und API-Verbindung per WLAN installieren. Wenn API-/OTA-Zugangsdaten wechseln, gegebenenfalls erneut per USB flashen. **Keine** Zugangsdaten aus der Beispiel-Secrets-Datei unverändert produktiv nutzen.

**Prüfhinweis:** Vor einem realen zehnsekündigen Tastendruck COM/NO ohne Mainboardanschluss auf exakt einen Impuls von ca. 10 Sekunden prüfen. Der LED-Test ohne Relais dauert unverändert 6 Sekunden.
