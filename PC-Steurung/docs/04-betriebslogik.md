# Betriebslogik, Verriegelungen und LED-Modi

## Power-Steuerung

| Aktion | Relais | Weitere Wirkung |
|---|---|---|
| HA oder Web: PC Ein/Aus | 0,2–1,5 s (Standard 0,5 s) | 10 s LED weich pulsieren |
| Physischer GPIO18-Taster | Gleiche einstellbare Normaldauer | 10 s LED weich pulsieren |
| HA oder Web: PC Not-Aus | 4–12 s (Standard 10 s) | LED 0,5 s EIN / 0,5 s AUS, bis PC aus |
| Erzwungenes Ausschalten bei weder 12V noch stabiler PLED EIN | Befehl abgelehnt | Schutz gegen versehentliches Einschalten |
| Weiterer Power-Befehl während erzwungenem Ausschalten | Gesperrt | Keine konkurrierenden Impulse |

Das **erzwungene Ausschalten** ist nur ein 4–12 Sekunden gehaltener ATX-Power-Taster, **kein normgerechter Not-Aus**. Ungespeicherte Daten können verloren gehen. Die Verriegelung akzeptiert **entweder geschaltete 12 V EIN oder die mindestens 2 Sekunden ohne Unterbrechung aktive Mainboard-Power-LED**. Beide Signale muessen elektrisch korrekt gemessen sein. Eine im S3-Standby blinkende LED ist kein verlaesslicher Betriebsindikator; lange EIN-Phasen der Standby-LED koennen dennoch fehlinterpretiert werden.

Wenn nach Ablauf der eingestellten langen Impulsdauer mindestens eine der beiden Quellen weiter PC EIN meldet, bleibt die *Warnanzeige* aktiv. Die kombinierte Erkennung geht erst auf AUS, wenn beide Quellen drei Sekunden durchgehend AUS melden. Das Relais fällt nach der eingestellten Dauer (spätestens nach 12 Sekunden) unabhängig davon ab. Nach einem ESP32-Neustart werden Not-Aus-Zustände nicht wiederhergestellt.

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

Die zuletzt ausgewählte Option wird auf dem Gerät gespeichert. Der PC-EIN/AUS-Zustand der LED-Modi basiert nun auf **geschalteter 12-V-Schiene ODER qualifizierter Mainboard-Power-LED** und nicht auf der Erreichbarkeit von Windows.

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
- Die Not-Aus-Blinkanzeige startet bei akzeptiertem erzwungenem Ausschalten **sofort**; der eigentliche Impuls bleibt weiterhin von einem validierten **PC EIN** via 12 V oder mindestens zwei Sekunden stabiler Power-LED abhaengig.
- Falls weder 12 V noch stabile Power-LED EIN melden, darf das Relais nicht für den langen Impuls anziehen; der Textsensor macht das sichtbar.
- Blinkt die zweite LED bereits beim reinen Test nicht, ist zuerst deren Verbindung mit GPIO2, Polaritaet bzw. PWM-Eignung zu pruefen.

## Kombinierte Erkennung ab Revision 3

- **PC 12V Versorgung:** GPIO22 (PC817-Kanal 1), bei EIN unmittelbar relevant.
- **PC Power LED:** GPIO21 (PC817-Kanal 2), zeigt das ungefilterte PLED-Signal.
- **PC Power LED stabil:** zusaetzliche Auswertung, nur wenn PLED mindestens **2 Sekunden ohne Unterbrechung EIN** war. Kurzes Standby-Blinken soll kein erzwungenes Ausschalten freigeben.
- **PC Betrieb erkannt:** ODER aus 12 V und stabiler PLED; beim Wechsel auf AUS **3 Sekunden Nachlauf** gegen kurze Signalunterbrechungen.
- **Relaisfreigabe:** verlangt zum Zeitpunkt der Betaetigung entweder 12 V EIN oder stabile PLED EIN – NICHT bloss einen nachlaufenden, alten Kombinationszustand.
- **Not-Aus-Anzeige:** endet nach dem einstellbaren 4–12-s-Relaisimpuls erst, wenn die kombinierte Erkennung AUS geworden ist. Die erzwungene Kontaktbetaetigung dauert immer maximal 12 Sekunden.
- **Sieben normale LED-Modi:** verwenden jetzt ebenfalls die kombinierte PC-EIN/AUS-Erkennung.

**Sicherheit:** Bei S3/Sleep/PLED-Blinken mit langen Leuchtphasen kann eine reine Power-LED-Erkennung trotz Qualifizierung falsch sein. Im Standby kann langer Power-Taster-Druck eine unerwartete Wirkung (z. B. Aufwachen) haben. Vor Aktivierung mit realer Hardware AUS, EIN und S3 pruefen. Niemals die 2-s-Qualifizierung unbedacht entfernen.

## Anpassung der Relaisschaltzeiten

Normaler Power-Befehl über HA, Web oder GPIO18: **0,2–1,5 s** (Standard **0,5 s**). Erzwungenes Ausschalten über HA/Web: **4–12 s** (Standard **10 s**). **Unverändert:** separater LED-Test ohne Relais 6 s; normale Power-LED-Pulsanimation 10 s; WLAN-Blinken 300 ms EIN/AUS; Not-Aus-Warnblinken 500 ms EIN/AUS. Der zehnsekündige Tastendruck kann je nach PC-/UEFI-Verhalten einen unerwarteten Neustart nach dem Abschalten auslösen und sollte vor Nutzung am Relaiskontakt ohne angeschlossenes Mainboard geprüft werden.

## Gespeicherte Zeitregler und Weboberfläche

- **PC Ein/Aus Impulsdauer**: 0,2–1,5 s (Schritt 0,1 s), Standard 0,5 s.
- **PC Not-Aus Impulsdauer**: 4–12 s (Schritt 0,5 s), Standard 10 s.
- Beide `number.template`-Entitäten werden auf dem Gerät mit `restore_value: true` gespeichert und sowohl in HA als auch über ESPHome Webserver V3 bedient. Die Impulszeit wird beim Start des Befehls einmal eingelesen; Änderungen währenddessen beeinflussen den laufenden Kontakt nicht.
- Die beiden Hauptbuttons heißen **PC Ein/Aus** und **PC Not-Aus (Ausschalten erzwingen)**. Zusätzliche Diagnosebuttons bleiben getrennt.
- ESPHome-Webserver auf lokalem HTTP-Port 80 mit **Digest-Authentifizierung** und lokalen Web-Secrets, ohne Web-OTA. **Keine Portfreigabe ins Internet.**
- Unverändert: LED-Test ohne Relais 6 s, weiches LED-Pulsieren nach normalem Power-Befehl 10 s, WLAN-Anzeige 0,3 s EIN/AUS und Warnblinken 0,5 s EIN/AUS.

[Einrichtung der Weboberfläche](07-weboberflaeche.md).
