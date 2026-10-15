# Elektrisches Prüfprotokoll – vor Anschluss verpflichtend

Dies ist eine **leere Prüfliste**, keine Behauptung über bestandene Messungen. Ergebnisse für die realen Module dokumentieren, bei Unklarheit nicht weiter verdrahten.

## ESP32 und Versorgung

- [ ] Reale Platinenbezeichnungen GPIO23/18/22/21/2 überprüft
- [ ] ESP32 allein stabil im WLAN; keine Brownout-Meldungen
- [ ] Relaismodul benötigt tatsächlich geeignete VCC-Spannung und Stromstärke
- [ ] 3,3-V-Versorgung durch Relais-Spulenstrom nicht überlastet
- [ ] Relais bleibt beim Anlegen der Betriebsspannung, Booten und Neustart sicher AUS

## Relais

- [ ] NO, COM, NC durch **Durchgangsmessung** identifiziert
- [ ] COM/NO in Ruhe offen, bei angezogenem Relais geschlossen
- [ ] 300-ms-Impuls gemessen, ohne PC-Mainboard
- [ ] Lokaler GPIO18-Taster löst genau einen 300-ms-Impuls aus
- [ ] 6-s-Impuls separat und **ohne Mainboard** mit simulierter, überprüfter 12-V-Statusmeldung getestet
- [ ] Kein unerwünschter Relaisimpuls bei WLAN-Verlust, OTA und ESP32-Neustart

## PC817 Kanal 1: 12-V-Versorgung

- [ ] PC817-Modul für **12-V-Eingang** nachweislich geeignet
- [ ] +12 V ist die **geschaltete Hauptversorgung**, keine Standby-Versorgung
- [ ] Ein- und Ausgangsseiten bzw. Jumper auf tatsächliche Trennung durchgemessen
- [ ] Ausgang U1 gegen ESP-GND bleibt **0 bis 3,3 V** bei allen Zuständen
- [ ] GPIO22 meldet nachweislich EIN nur bei geschalteter Hauptversorgung
- [ ] GPIO22 meldet nachweislich AUS bei abgeschalteter Hauptversorgung
- [ ] Fehler bei fehlendem/defektem Sensor berücksichtigt, bevor erzwungenes Ausschalten erlaubt wird

## PC817 Kanal 2: Mainboard-Power-LED

- [ ] PLED+ und PLED− anhand des **richtigen Mainboard-Handbuchs** bestimmt
- [ ] PLED-Spannung/Polarität in EIN/AUS/Standby gemessen
- [ ] Zum gemessenen Signal passende **getrennte Eingangsschaltung** dimensioniert
- [ ] Originale Gehäuse-LED wird nicht in ihrer Funktion beeinträchtigt
- [ ] Ausgang U2 gegen ESP-GND bleibt stets **0 bis 3,3 V**
- [ ] GPIO21 meldet den tatsächlichen LED-Zustand getrennt von GPIO22

## Finale Funktionsprüfung

- [ ] Originaler Gehäusetaster funktioniert weiter
- [ ] HA-Power-Button und lokaler Taster funktionieren
- [ ] Aus-Schaltzustände bei echter PC-Hardware validiert
- [ ] Erzwungenes Ausschalten kann bei tatsächlich AUS nicht versehentlich einschalten
- [ ] GPIO2-LED auf genau diesem Board identifiziert; PWR-LED ist nicht Gegenstand der PWM-Steuerung
- [ ] LED-Modi, 10-s-Pulsieren, WLAN-Warnung und Ausschalten-erzwingen-Anzeige getestet
- [ ] Stromkreise mechanisch/elektrisch sauber isoliert und gesichert

**Keine** Freigabe aufgrund einer erfolgreichen YAML-Kompilierung allein.
