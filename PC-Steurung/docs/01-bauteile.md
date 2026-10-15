# Bauteile und Voraussetzungen

| Anzahl | Bauteil | Spezifikation / Bemerkung |
|---:|---|---|
| 1 | ESP32 ESP-WROOM-32 Dev-Board | Freie GPIO18, GPIO21, GPIO22, GPIO23; GPIO2 optional |
| 1 | 1-Kanal-Relaismodul | Potentialfreier NO/COM-Kontakt; Steuereingang HIGH-aktiv und 3,3-V-GPIO-kompatibel |
| 1 | 2-Kanal-Optokoppler mit PC817 | Nur bei bestätigten Eingangsbereichen und Ausgangspegeln |
| 1 | Optionaler manueller Schließertaster | Potentialfrei, nicht rastend |
| 1 | Dauerhafte Spannungsversorgung | Ausreichende Reserve für ESP32-WLAN und Relais |
| nach Bedarf | Frontpanel-Y-Verbindung | Zum parallelen Anschluss ans Mainboard-PWR_SW |
| nach Bedarf | Isolierte Leitungen und Strombegrenzung | Gemäß realer Schaltung, Spannung und Verlegeart |
| ggf. | Signalanpassung für Mainboard-PLED | Nach Spannungsmessung zu dimensionieren |

## Relais

Die Aufschrift SRD-03VDC-SL-C steht für eine 3-V-Relais-Spule; sie garantiert nicht automatisch einen 3,3-V-kompatiblen Steuereingang oder ausreichend Strom über den ESP32-3V3-Pin. Versorgung unter Last prüfen. NO (Schließer), NC (Öffner) und COM nicht anhand von Farben raten – per Durchgangsprüfer bestimmen.

## PC817

Ein 3,6–30-V-Eingangsmodul kann die geschaltete 12-V-Hauptspannung erfassen, **wenn das tatsächliche Modul für 12 V ausgelegt ist**. Sein Ausgang braucht einen geprüften 0–3,3-V-Pegel zur ESP32-Seite. Jumper können bei manchen Modulen die Masse galvanisch verbinden: Anschluss und Isolation real messen.

Die Mainboard-Power-LED ist kein garantiertes 3,6–30-V-Signal. Ohne Nachweis von Spannung, Polarität, Eingangsstrom und korrekter Signalaufbereitung **keine direkte Verbindung** zum PC817-Eingang herstellen.

## ESP32-LEDs

- PWR-LED: normalerweise fest an Versorgung, per GPIO nicht beeinflussbar.
- D2-LED: D2 kann eine Bestückungsbezeichnung sein, kein GPIO2-Belegungsnachweis.
- Die Firmware setzt GPIO2 nur als zu bestätigenden, möglichen LED-Ausgang voraus.

## Mainboard

Die Position des Frontpanel-Headers und die PWR_SW/PLED-Pins hängen vom **konkreten Mainboard** ab. Ausschließlich dessen offizielles Handbuch benutzen. Niemals fremde Pinnummern übernehmen.
