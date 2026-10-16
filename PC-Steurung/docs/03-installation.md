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
| pc_web_username | Individueller Benutzername für die lokale ESP32-Weboberfläche |
| pc_web_password | Langes, zufälliges Passwort für die lokale ESP32-Weboberfläche |

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

Beim Booten und Neustart darf das Relais nicht selbstständig anziehen. Die Firmware setzt ALWAYS_OFF, dies garantiert bei jeder Modulvariante jedoch noch keinen glitchfreien Boot-Vorgang. Mit geeignetem Messgerät COM–NO beobachten. Home-Assistant-Button **PC Ein/Aus** drücken: Kontakt muss die **eingestellte normale Impulsdauer** (Standard 0,5 s) schließen. Auch den GPIO18-Taster separat testen.

**Erst danach** PWR_SW am Mainboard verbinden.

## 5. Statussensoren

Vor Anschluss des 12-V-Ausgangs an GPIO22 sicherstellen, dass PC817 U1 gegen ESP32-GND stets **0 bis 3,3 V** führt. In der YAML wird bei anliegenden 12 V ein **aktiv-LOW-Signal** vorausgesetzt. Verhalten mit **12 V AUS** und **12 V EIN** verifizieren.

Der Mainboard-PLED-Eingang darf **nicht ungeprüft direkt** an ein 3,6–30-V-Modul: Eingangsspannung, Polarität, Strombelastbarkeit und LED-Verhalten messen; passende Signalaufbereitung auslegen. Die beiden Ausgänge sind in HA getrennte Sensoren.

## 6. Home-Assistant-Entitäten

- PC Ein/Aus: einstellbarer 0,2–1,5-s-Power-Taster (Standard 0,5 s).
- PC Not-Aus (Ausschalten erzwingen): einstellbarer 4–12-s-Power-Taster (Standard 10 s), nur bei geprüfter PC-EIN-Erkennung.
- PC 12V Versorgung: geschaltete Haupt-12 V erkannt.
- PC Power LED: separat erfasste Mainboard-LED.
- Onboard LED Modus: Auswahl eines der sieben Modi.
- Ausschalten erzwingen aktiv: Warnstatus.
- ESP32 Online, WLAN Signal, ESP32 Laufzeit, ESP32 Neustart.

## 7. OTA-Updates

Nach erfolgreichem Flash und API-Verbindung per WLAN installieren. Wenn API-/OTA-Zugangsdaten wechseln, gegebenenfalls erneut per USB flashen. **Keine** Zugangsdaten aus der Beispiel-Secrets-Datei unverändert produktiv nutzen.

**Prüfhinweis:** Vor einem langen Tastendruck COM/NO ohne Mainboardanschluss auf die eingestellte Dauer (4–12 s) prüfen. Der LED-Test ohne Relais dauert unverändert 6 Sekunden.

## Passwortgeschützte ESP32-Weboberfläche

Die Firmware verwendet die zusätzlichen Secrets `pc_web_username` und `pc_web_password`. Diese **vor der Installation lokal anlegen**. Nach erfolgreichem Flash im lokalen Netzwerk `http://pc-steuerung.local/` oder `http://<ESP32-IP>/` öffnen und mit den individuellen Zugangsdaten anmelden. Die Oberfläche enthält die beiden Hauptbuttons und die gespeicherten Impulszeiten. Es handelt sich um **HTTP mit Digest**, **nicht HTTPS** – keine öffentliche Portfreigabe einrichten. Alle Schritte: [Lokale Weboberfläche](07-weboberflaeche.md).

## Relais-Schaltschutz (ab aktueller Firmware)

Die Number-Entität **PC Relais Schaltsperre** bietet 0,5–10 Sekunden, Standard 2 Sekunden, auch über HA und die lokale (weiterhin geschützte) ESP32-Weboberfläche. Nach jedem Relaisabfall verhindert sie sofortige Folgebefehle; auch während eines laufenden Normalimpulses wird erzwungenes Ausschalten blockiert. Vor Anschluss an das Mainboard Relaiskontakte COM/NO getrennt testen: Normal-/Not-Aus-Impuls, Sperre, Doppelbetätigung und Neustart. `ALWAYS_OFF` ist keine Garantie gegen einen kurzen Pegelwechsel beim physischen Booten.

**Die Weboberfläche bleibt vorerst global passwortgeschützt:** ESPHome kann seine REST-Schaltaktionen nicht individuell mit einem Klick-Passwort sichern, während alle anderen Web-Endpunkte öffentlich sind. Für eine sichtbare öffentliche Statusseite mit geschützter Steuerung ist eine getrennte Status-/Authentifizierungs-Webarchitektur erforderlich, **keine direkte öffentliche Weiterleitung auf den ESP32**.
