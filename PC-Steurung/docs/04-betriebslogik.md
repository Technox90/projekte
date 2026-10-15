# Betriebslogik, Verriegelungen und LED-Modi

## Power-Steuerung

| Aktion | Relais | Weitere Wirkung |
|---|---|---|
| HA: PC Power | 300 ms schließen | 10 s LED weich pulsieren |
| Physischer GPIO18-Taster | 300 ms schließen | 10 s LED weich pulsieren |
| HA: Ausschalten erzwingen | 6 s schließen | LED 0,5 s EIN / 0,5 s AUS, bis PC aus |
| Erzwungenes Ausschalten bei PC AUS/UNBEKANNT | Befehl abgelehnt | Schutz gegen versehentliches Einschalten |
| Weiterer Power-Befehl während erzwungenem Ausschalten | Gesperrt | Keine konkurrierenden Impulse |

Das **erzwungene Ausschalten** ist nur ein sechs Sekunden gehaltener ATX-Power-Taster, **kein normgerechter Not-Aus**. Ungespeicherte Daten können verloren gehen. Die Verriegelung setzt einen korrekt gemessenen 12-V-Sensor voraus.

Wenn der 12-V-Sensor bei Ablauf der sechs Sekunden immer noch PC EIN meldet, bleibt die *Warnanzeige* aktiv. Das Relais fällt nach sechs Sekunden unabhängig davon ab. Nach einem ESP32-Neustart werden Not-Aus-Zustände nicht wiederhergestellt.

## Sieben LED-Modi

| Option | PC AUS | PC EIN |
|---|---|---|
| Aus Blinkend | 1 s EIN / 1 s AUS | aus |
| Ein Blinkend | aus | 1 s EIN / 1 s AUS |
| Aus Langsam blinken | 0,5 s EIN / 5 s AUS | aus |
| Ein Langsam blinken | aus | 0,5 s EIN / 5 s AUS |
| Aus Leuchtend | dauernd ein | aus |
| Ein Leuchtend | aus | dauernd ein |
| AUS | aus | aus |

Die zuletzt ausgewählte Option wird auf dem Gerät gespeichert. Der PC-EIN/AUS-Zustand basiert auf der geschalteten **12-V-Schiene**, nicht auf der Erreichbarkeit von Windows.

## Automatische LED-Warnungen (Priorität)

1. **Erzwungenes Ausschalten aktiv:** 500 ms EIN, 500 ms AUS.
2. **WLAN verloren:** 300 ms EIN, 300 ms AUS.
3. **Normaler Power-Befehl:** zehn Sekunden sanftes PWM-Pulsieren (fünf Perioden zu je zwei Sekunden).
4. **Normalbetrieb:** der in HA gespeicherte Modus.

Alle Animationen gelten nur für eine **über GPIO2 tatsächlich steuerbare LED**, nicht für die feste PWR-LED.

## Blink-Takte

Abtastintervall: 100 ms.

| Muster | Ausdruck | Wirkung |
|---|---|---|
| Normal | (t % 20) < 10 | 1 s ein / 1 s aus |
| Langsam | (t % 55) < 5 | 0,5 s ein / 5 s aus |
| WLAN-Ausfall | (t % 6) < 3 | 0,3 s ein / 0,3 s aus |
| Ausschalten erzwingen | (t % 10) < 5 | 0,5 s ein / 0,5 s aus |

**Falscher Ausdruck:** (t % 5 < 20) wäre immer wahr, somit Dauerlicht. Das wird in dieser Firmware bewusst nicht verwendet.

## Bedeutung der Sensoren

- PC 12V Versorgung EIN: überwachte PC-Hauptversorgung steht an – kein Beweis für Windows-Bereitschaft.
- PC Power LED: von der LED-Stufe erkanntes Signal, ggf. blinkend; der Binärsensor bildet noch keine eigene S3-Standby-Klassifikation.
- ESP32 Online: Controller ist online, nicht der PC.
- WLAN-Verlust-Anzeige: Prüft, ob der ESP32 mit einer WLAN-Station verbunden ist. **Kein** Internet- oder Home-Assistant-Verfügbarkeitstest.

Die WLAN- und API-Reboot-Timeouts sind auf null gesetzt, damit die LED bei langen Verbindungsausfällen weiterläuft. Automatischer Neustart aufgrund dieser Timeouts entfällt dadurch.

## LED-Test und Rueckmeldung

- **Not-Aus LED Test (ohne Relais):** sechs Sekunden 500 ms EIN / 500 ms AUS; kein Relaisimpuls, kein PC-Ausschalten.
- **Ausschalten Rueckmeldung:** Statusmeldung "Gestartet", "Abgelehnt", "Abgeschlossen" oder "LED-Test beendet".
- Die Not-Aus-Blinkanzeige startet bei akzeptiertem sechsekündigem Ausschalten **sofort**; der eigentliche Impuls bleibt weiterhin von einem validierten **PC EIN** an GPIO22 abhaengig.
- Falls der PC817 **AUS** oder **UNBEKANNT** meldet, darf das Relais nicht sechs Sekunden anziehen; der Textsensor macht das sichtbar.
- Blinkt die zweite LED bereits beim reinen Test nicht, ist zuerst deren Verbindung mit GPIO2, Polaritaet bzw. PWM-Eignung zu pruefen.
