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

COM und NO unter spannungsfreier Messung identifizieren. Am Mainboard nur PWR_SW-Kontaktpaar nach Originalhandbuch verwenden. Impulsdauer von 300 ms prüfen; die originalen Gehäusetasteranschlüsse bleiben parallel bestehen.

## Optokoppler meldet falschen 12-V-Status

Ausgangspegel U1 gegen ESP-GND im Zustand Haupt-12 V AUS/EIN messen, nicht raten. Die aktuelle Firmware nimmt einen aktiv-LOW-Ausgang an. Bei falscher Logik nicht den sechssekündigen Ausschaltbefehl auslösen. Auch bei offenem Eingang kann der interne Pull-up einen vermeintlich definierten Zustand anzeigen.

## Mainboard-Power-LED bleibt immer aus

Bei als 3,6–30 V beworbenem Eingangsmodul ist die LED-Spannung möglicherweise zu niedrig. Spannung zwischen PLED+ und PLED− samt Polarität, Strombedarf und Verhalten im Standby bestimmen; erst dann passende galvanisch getrennte Signalaufbereitung auslegen.

## PWR-LED leuchtet dauerhaft; zweite LED blinkt nicht

Die PWR-LED wird fast immer fest von der Boardversorgung gespeist. D2 kann die **Bestückungskennung**, nicht GPIO2 sein. Hardware/Schaltplan der exakten Devboard-Revision prüfen. Falls die zweite LED nicht an GPIO2 liegt, kann die Firmware sie nicht steuern.

## Erzwungenes Ausschalten aktiv bleibt stehen

Die Anzeige endet erst, wenn der separat gemessene 12-V-Sensor PC AUS meldet. Das Relais wird **trotzdem nach sechs Sekunden freigegeben**. Bleibt die Anzeige nach tatsächlichem PC-Aus bestehen, zuerst den 12-V-Sensor untersuchen.
