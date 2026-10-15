# Anschluss- und Prinzipschaltpläne

> **Status:** Logische Schaltbilder, keine elektrisch freigegebene Fotomontage. Die konkret beschrifteten Pins des gelieferten Relais- und PC817-Moduls müssen gemessen werden. Ungeprüfte Verbindung zur Mainboard-Power-LED bleibt absichtlich offen.

## 1. ESP32-Pinliste

| ESP32 | Aufgabe | Gegenstelle |
|---|---|---|
| GPIO23 | Ausgang HIGH | Relais IN |
| GND | Steuer-Masse | Relais GND |
| geeignete Relaisversorgung | Versorgung nach Modultyp | Relais VCC |
| GPIO18 | Pull-up-Eingang | Manueller NO-Taster an GND |
| GPIO22 | Sensor aktiv-LOW | PC817 Ausgang U1 / OUT1 |
| GPIO21 | Sensor aktiv-LOW | PC817 Ausgang U2 / OUT2 |
| GPIO2 | PWM-Ausgang | Optionale zweite Board-LED, nur falls identifiziert |

## 2. Relais-Steueranschlüsse

~~~mermaid
flowchart LR
  subgraph E["ESP32 Steuerseite"]
    A["GPIO23"]
    B["ESP32 GND"]
    C["geeignete Versorgung"]
  end
  subgraph R["Relaismodul Eingang"]
    D["IN"]
    F["GND"]
    G["VCC"]
  end
  A -->|"HIGH: Relais an"| D
  B --- F
  C --- G
~~~

**Relais-COM/NO sind vollständig von VCC/IN/GND zu unterscheiden.** Zwischen COM und NO darf bei ausgeschaltetem Relais kein Durchgang bestehen. Die VCC-Spannung und der Strombedarf des Relaismoduls sind durch Messung zu bestätigen.

## 3. Relais und Gehäusetaster parallel am PC

~~~text
Mainboard PWR_SW-A                            Mainboard PWR_SW-B
        |                                             |
        +---[ Original Gehaeuse-Taster (NO) ]---------+
        |                                             |
        +---[ Relais COM ---- NO ]--------------------+
        |                                             |
        +---[ Optionaler direkter Taster (NO) ]-------+

An PWR_SW KEINE externe Spannung anlegen. Relais NC frei lassen.
~~~

Wer einen Taster an **GPIO18/GND** anschließt, nutzt stattdessen die Firmwaresteuerung. Dies ist eine Alternative zum unabhängigen Taster direkt am Mainboard, keine elektrische Verbindung zwischen GPIO18 und Mainboard-PWR_SW.

## 4. PC817 Kanal 1: geschaltete 12 V

~~~mermaid
flowchart LR
  subgraph PC["PC-Stromkreis / Hauptversorgung"]
    A["+12 V GESCHALTET"]
    B["PC GND"]
  end
  subgraph IN["PC817-Eingang 1 (nur wenn 12 V spezifiziert)"]
    C["IN1 (+)"]
    D["IN1-G / MINUS"]
  end
  A --> C
  B --- D
~~~

Der 12-V-Abgriff muss isoliert und kurzschlussgeschützt ausgeführt werden. **Nicht** die Standby-Schiene wählen, und **niemals das ATX-Netzteil öffnen**.

~~~mermaid
flowchart LR
  subgraph OUT["PC817 Ausgang 1"]
    A["U1 / OUT1"]
    B["GND der Ausgangsseite"]
  end
  subgraph E["ESP32 3,3-V-Seite"]
    C["GPIO22 mit Pull-up"]
    D["ESP32 GND"]
  end
  A --> C
  B --- D
~~~

U1 muss in allen Zuständen 0–3,3 V gegenüber ESP-GND ausgeben. **Aktiv LOW** wird in der Firmware angenommen; falls die reale Ausgangslogik anders ist, erst messen und dann gezielt die Sensorpolarität anpassen. Jumper und galvanische Trennung prüfen.

## 5. PC817 Kanal 2: Mainboard-Power-LED

~~~mermaid
flowchart LR
  subgraph MB["Mainboard"]
    A["PLED+"]
    B["PLED−"]
  end
  M["LED-Spannung/Polarität messen und passende isolierte Signalanpassung auslegen"]
  I["PC817 Eingang 2, erst nach Freigabe"]
  A --> M
  B --> M
  M -. "KEINE direkte ungeprüfte Leitung" .-> I
~~~

Ein als **3,6–30 V** beworbenes PC817-Eingangsmodul ist nicht automatisch kompatibel mit einer möglicherweise unter 3,6 V liegenden PLED-Spannung. Ein Anzapfen parallel zur originalen Power-LED kann ihre Anzeige beeinflussen. Daher **nur nach Spannungsmessung und angepasster Erfassung** verbinden.

~~~mermaid
flowchart LR
  subgraph OUT["PC817 Ausgang 2"]
    A["U2 / OUT2"]
    B["GND der Ausgangsseite"]
  end
  subgraph E["ESP32 3,3-V-Seite"]
    C["GPIO21 mit Pull-up"]
    D["ESP32 GND"]
  end
  A --> C
  B --- D
~~~

## 6. Lokaler Taster am ESP32

~~~text
ESP32 GPIO18 ----[ potentialfreier Taster NO ]---- ESP32 GND
                    intern 3,3-V-Pull-up
~~~

## 7. Farblegende der Dokumentation

Rot: 3,3 V, orange: PC +12 V, schwarz: ESP-GND, grau: PC-GND, blau: GPIO-Signale, grün: trockener PWR_SW-Schließerkontakt.

**Wichtig:** Die Eingangsmassen der PC817-PC-Seite nicht mit der ESP-Ausgangsmasse verbinden, wenn echte galvanische Trennung beabsichtigt ist. Auf manchen Breakouts überbrücken Jumper die Trennung – vor Nutzung messen.
