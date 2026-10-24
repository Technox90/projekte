# AUTO USB/Akku-Erkennung

Der AUTO-Modus kann zwischen USB- und Akkubetrieb umschalten, wenn USB-VBUS
über einen Spannungsteiler auf einen freien ADC-fähigen GPIO geführt wird.

## Beispiel

- USB 5 V -> 100 kOhm -> Sense-GPIO
- Sense-GPIO -> 100 kOhm -> GND
- gemeinsames GND
- Teiler-Faktor: 2.0
- USB-Erkennungsschwelle: 4.0 V

In der Weboberfläche unter **Konfiguration -> Anzeige & LEDs**:

- Versorgungsanzeige: `AUTO`
- VBUS-Sense GPIO: der tatsächlich verwendete freie ADC-GPIO
- VBUS Teiler-Faktor: `2.0`
- USB-Erkennung ab: `4.0 V`

Bei `VBUS-Sense GPIO = -1` fällt AUTO kontrolliert auf die Akkuanzeige zurück.

Wichtig: Nicht einfach irgendeinen GPIO auswählen. Der gewählte Pin muss am
konkreten Aufbau frei und ADC-fähig sein.
