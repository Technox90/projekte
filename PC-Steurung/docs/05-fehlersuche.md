# Fehlersuche

## Gerät verschwindet aus dem WLAN

Relais und PC817 zuerst vollständig abklemmen, ESP32 nur über USB-Datenkabel versorgen. WLAN-Logs prüfen; insbesondere Brownout, Neustartschleifen und Versorgungseinbrüche durch die Relais-Spule. Stromaufnahme der konkreten Relaisplatine messen.

## COM-Port vorhanden, Web Serial verbindet nicht

Keine konkurrierende serielle Anwendung geöffnet lassen. Im Windows-Geräte-Manager kontrollieren, ob der COM-Port beim Trennen des ESP32 verschwindet. Nur die real vorhandene BOOT-/IO0- und EN/RST-Belegung des Boards nach Herstellerangaben verwenden. Nicht beliebige Pins brücken.

## WLAN verbunden, API meldet BAD_INDICATOR

Meist fehlgeschlagener Verschlüsselungs-Handshake. In Home Assistant die ESPHome-Integration mit exakt dem API-Schlüssel der aktuellen Firmware neu konfigurieren. Eine zugewiesene IP-Adresse belegt noch keine funktionierende API.

## Relais schaltet alle fünf Sekunden

Eine alte Testkonfiguration enthält womöglich einen interval-Schaltzyklus. Die veröffentlichte Firmware schaltet das **Relais nur durch Power-Button oder manuellen Taster**; ihre periodische LED-Logik darf GPIO23 nicht betätigen.

## Relais klickt, PC reagiert nicht

COM und NO unter spannungsfreier Messung identifizieren. Am Mainboard nur PWR_SW-Kontaktpaar nach Originalhandbuch verwenden. Impulsdauer von 500 ms prüfen; die originalen Gehäusetasteranschlüsse bleiben parallel bestehen.

## Optokoppler meldet falschen 12-V-Status

Ausgangspegel U1 gegen ESP-GND im Zustand Haupt-12 V AUS/EIN messen, nicht raten. Die aktuelle Firmware nimmt einen aktiv-LOW-Ausgang an. Bei falscher Logik nicht den zehnsekündigen Ausschaltbefehl auslösen. Auch bei offenem Eingang kann der interne Pull-up einen vermeintlich definierten Zustand anzeigen.

## Mainboard-Power-LED bleibt immer aus

Bei als 3,6–30 V beworbenem Eingangsmodul ist die LED-Spannung möglicherweise zu niedrig. Spannung zwischen PLED+ und PLED− samt Polarität, Strombedarf und Verhalten im Standby bestimmen; erst dann passende galvanisch getrennte Signalaufbereitung auslegen.

## PWR-LED leuchtet dauerhaft; zweite LED blinkt nicht

Die PWR-LED wird fast immer fest von der Boardversorgung gespeist. D2 kann die **Bestückungskennung**, nicht GPIO2 sein. Hardware/Schaltplan der exakten Devboard-Revision prüfen. Falls die zweite LED nicht an GPIO2 liegt, kann die Firmware sie nicht steuern.

## Erzwungenes Ausschalten aktiv bleibt stehen

Die Anzeige endet, wenn **12 V und qualifizierte Power-LED** den PC nicht mehr als EIN erkennen (mit 3 Sekunden Aus-Nachlauf). Das Relais wird **trotzdem nach zehn Sekunden freigegeben**. Bleibt die Anzeige nach tatsächlichem PC-Aus bestehen, zuerst den 12-V-Sensor untersuchen.

## Ausschalten erzwingen: LED blinkt nicht (Diagnose)

1. **Not-Aus LED Test (ohne Relais)** in Home Assistant betaetigen. Er blinkt sechs Sekunden (0,5 s EIN / 0,5 s AUS), ohne das Relais zu schalten.
2. Falls der Test **nicht** sichtbar blinkt: Auf dieser Boardrevision ist D2 eventuell nicht an GPIO2 angeschlossen, GPIO2 ist ungeeignet oder die LED-Polaritaet ist invertiert. Vor weiteren Aenderungen elektrische Anbindung pruefen.
3. Falls der Test blinkt, aber der echte Not-Aus nicht: Den Textsensor **Ausschalten Rueckmeldung** anschauen. "Abgelehnt: 12V AUS und Power-LED nicht stabil EIN" zeigt an, dass die Sicherheitsfreigabe blockiert hat.
4. In Home Assistant pruefen, ob **PC 12V Versorgung** bei eingeschaltetem PC wirklich EIN meldet. Nicht durch Umgehen dieser Bedingung korrigieren; Optokopplerlogik und Signale messen.
5. Bei akzeptiertem Not-Aus setzt die Firmware den LED-Ausgang sofort auf EIN, danach wechselt sie 500 ms EIN / 500 ms AUS. Das Relais darf maximal zehn Sekunden angezogen bleiben.

## Kombinierte Erkennung: nur einer der beiden Sensoren meldet EIN

1. **PC 12V Versorgung** und **PC Power LED** einzeln im HA-Geraet pruefen.
2. **PC Power LED stabil** wird nur EIN, wenn die LED **2 Sekunden ununterbrochen EIN** meldet. Bei langsamer blinkender PLED wird absichtlich keine Freigabe erteilt.
3. **PC Betrieb erkannt** kombiniert beide Quellen per ODER. Erst wenn beide Quellen mindestens 3 Sekunden keine Aktivitaet mehr melden, gilt der PC als AUS.
4. Vor dem echten zehnsekündigen Ausschalten am PC die Anzeige **PC Betrieb erkannt** und die beiden Ursprungsquellen vergleichen. Bei abweichendem Zustand zuerst Sensorpolung und Optokopplerverdrahtung durchmessen.
5. Wenn PLED im Standby auch mehr als zwei Sekunden kontinuierlich leuchtet, kann sie als EIN fehlinterpretiert werden. In diesem Fall erzwungenes Ausschalten nur mit verifizierter 12-V-Erkennung verwenden bzw. PLED-Logik fuer dieses Board anpassen.
