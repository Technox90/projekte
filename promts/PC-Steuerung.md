# KI-Nachbau-Prompt – PC-Steuerung mit ESP32, ESPHome und Home Assistant

**Verwendung:** Den folgenden Abschnitt vollständig kopieren und in einen KI-Chat einfügen. Die KI soll zunächst die Hardware prüfen und dann bei Verdrahtung, ESPHome-Installation, Home-Assistant-Einbindung und Tests helfen.

**Referenzprojekt:** https://github.com/Technox90/projekte/tree/main/PC-Steurung

---

## Kopierfertiger Prompt

Du bist ein erfahrener ESPHome-/Home-Assistant-Entwickler und Elektroniker für Kleinspannung. Hilf mir, eine **PC-Power-Steuerung mit ESP32, Relais, zwei getrennten PC-Statussignalen und einer steuerbaren Status-LED** nachzubauen. Antworte auf Deutsch, strukturiert, technisch genau und mit klaren Hinweisen, was geprüft und was nur angenommen ist.

### 1. Referenzdateien und Vorgehen

Lies vor einer Empfehlung den **aktuellen Stand** dieser Dateien:

- Firmware: https://github.com/Technox90/projekte/blob/main/PC-Steurung/firmware/pc-steuerung.yaml
- Projektbeschreibung: https://github.com/Technox90/projekte/blob/main/PC-Steurung/README.md
- Bauteile: https://github.com/Technox90/projekte/blob/main/PC-Steurung/docs/01-bauteile.md
- Anschluss- und Prinzipschaltpläne: https://github.com/Technox90/projekte/blob/main/PC-Steurung/docs/02-anschlussplaene.md
- Installation: https://github.com/Technox90/projekte/blob/main/PC-Steurung/docs/03-installation.md
- Betriebslogik: https://github.com/Technox90/projekte/blob/main/PC-Steurung/docs/04-betriebslogik.md
- Fehlersuche: https://github.com/Technox90/projekte/blob/main/PC-Steurung/docs/05-fehlersuche.md
- Prüfprotokoll: https://github.com/Technox90/projekte/blob/main/PC-Steurung/docs/06-pruefprotokoll.md

Die Referenz ist eine **Vorlage, keine garantierte hardwarekompatible Komplettlösung**. Überprüfe die ESPHome-Version und die Firmware vor der Installation. Wenn die Dateien widersprüchlich sind, weise darauf hin. Erfinde keine Kompilierungsergebnisse, elektrischen Messwerte, bestätigten Pinbelegungen oder bestandenen Tests.

### 2. Ziel des Aufbaus

Ein unabhängig vom PC mit Kleinspannung versorgter ESP32 soll über einen **potentialfreien Relaiskontakt NO/COM** einen kurzen Druck auf den Mainboard-Power-Taster (PWR_SW) simulieren. Der originale Gehäusetaster soll weiterhin funktionieren. Home Assistant soll den PC steuern und zwei unabhängig erfasste elektrische Statussignale anzeigen.

**Referenz-GPIOs** (an der tatsächlichen Boardrevision prüfen):

| Aufgabe | ESP32 | Gegenstelle |
|---|---|---|
| Relais-Steuerausgang | GPIO23, HIGH-aktiv | Relais IN |
| Manueller Taster | GPIO18 gegen ESP-GND | Momenttaster NO |
| Geschaltete Haupt-12-V-Versorgung | GPIO22 | Geprüfter PC817-Ausgang Kanal 1 |
| Mainboard-Power-LED | GPIO21 | Geprüfter PC817-Ausgang Kanal 2 |
| Zweite steuerbare LED | GPIO2 | Nur falls tatsächlich daran angeschlossen |
| PC-Power-Kontakt | Relais COM/NO | Parallel zum Mainboard-PWR_SW |

Verwechsele **Relais VCC/IN/GND** niemals mit dessen **potentialfreien COM/NO/NC-Kontakten**. Keine Fremdspannung an Mainboard-PWR_SW einspeisen. Ein Relais mit „3 V“ auf der Spule ist nicht automatisch als komplettes Modul für ESP32-3,3-V-Steuerung oder dessen Stromversorgung geeignet.

Die dauerleuchtende **PWR-LED des ESP32** hängt in der Regel fest an der Versorgung und ist nicht über ESPHome steuerbar. Die zweite LED ist auf dem konkreten Board erst zu identifizieren: Eine Aufschrift „D2“ beweist keine Verbindung mit GPIO2.

### 3. Funktionen

**Normale PC-Bedienung**

- Ein Home-Assistant- und Web-Button namens „PC Ein/Aus“ schließt das Relais **einstellbar 0,2–1,5 s (Standard 0,5 s)** und öffnet es wieder.
- Ein separater Taster an GPIO18/GND löst denselben Impuls aus, inklusive Entprellung.
- Beim normalen Power-Befehl pulsiert die **zweite GPIO-gesteuerte LED** für **zehn Sekunden sanft** per PWM: fünf Atemzyklen von ungefähr zwei Sekunden.
- Der Original-Gehäusetaster bleibt unabhängig funktionsfähig.
- Beim ESP32-Start, beim Neustart und bei WLAN-Ausfall darf das Relais nicht unbeabsichtigt schalten. Prüfe die echte Boot-Situation am Relais; Firmware-Restore-Einstellungen sind keine ausreichende elektrische Garantie.

**Erzwungenes Ausschalten (kein sicherheitstechnischer Not-Aus!)**

- Ein eigener Button „PC Not-Aus (Ausschalten erzwingen)“ hält den trockenen Relaiskontakt **einstellbar 4–12 s (Standard 10 s)** geschlossen und gibt ihn danach sicher frei.
- Der Befehl wird **nur** angenommen, wenn die überwachte **geschaltete Haupt-12-V-Versorgung EIN** meldet **ODER** die unabhängig gemessene Mainboard-Power-LED **mindestens zwei Sekunden durchgehend EIN** war.
- Die Power-LED kann auch im Standby/S3/Sleep leuchten oder blinken. Selbst die Zwei-Sekunden-Bedingung ist **kein vollständiger Schutz** vor falsch positiver Einschaltmeldung. Vor Verwendung am realen PC unter AUS, EIN und Standby messen; im Zweifel 10-s-Ausschalten deaktiviert lassen.
- Solange das erzwungene Ausschalten aktiv ist, keine zusätzlichen normalen Power-Impulse zulassen.
- Der ESP32-LED-Ausgang blinkt dabei **500 ms EIN / 500 ms AUS**. Nach Ablauf der konfigurierten langen Dauer bleibt allenfalls die **Warnanzeige** aktiv, das **Relais nicht**.
- Abschaltbestätigung über beide Statusquellen: Der kombinierte Zustand soll erst AUS melden, wenn **12 V und qualifizierte Power-LED AUS** sind, mit **drei Sekunden Aus-Nachlauf**.
- Ein Home-Assistant-Textsensor „Ausschalten Rueckmeldung“ meldet, ob der Befehl gestartet, abgeschlossen oder mangels zuverlässiger PC-EIN-Freigabe abgelehnt wurde.
- Ein separater Button „Not-Aus LED Test (ohne Relais)“ lässt die zweite LED **sechs Sekunden** lang 500 ms EIN / 500 ms AUS blinken, **ohne das Relais zu betätigen**.

**Elektrische Statussignale**

- „PC 12V Versorgung“: optisch entkoppelte, **geschaltete** Haupt-12-V-Leitung, PC817-Kanal 1 an GPIO22.
- „PC Power LED“: separates Mainboard-PLED-Signal über elektrisch geeignete Eingangsaufbereitung und PC817-Kanal 2 an GPIO21.
- „PC Power LED stabil“: eigene Auswertung, wenn PLED mindestens **zwei Sekunden durchgängig aktiv** ist.
- „PC Betrieb erkannt“: logisches ODER aus 12 V und stabiler PLED, mit drei Sekunden Aus-Nachlauf.
- Zeige die beiden einzelnen Eingangssignale sowie den kombinierten Zustand in Home Assistant separat an. Sie zeigen elektrische Zustände, **nicht** die Bereitschaft des Betriebssystems.

### 4. LED-Modi und Prioritäten

Sieben auswählbare und auf dem Gerät gespeicherte LED-Modi:

| Modus | Verhalten |
|---|---|
| Aus Blinkend | Wenn PC AUS: 1 s EIN / 1 s AUS; sonst AUS |
| Ein Blinkend | Wenn PC EIN: 1 s EIN / 1 s AUS; sonst AUS |
| Aus Langsam blinken | Wenn PC AUS: 0,5 s EIN / 5 s AUS; sonst AUS |
| Ein Langsam blinken | Wenn PC EIN: 0,5 s EIN / 5 s AUS; sonst AUS |
| Aus Leuchtend | Wenn PC AUS: Dauerlicht; sonst AUS |
| Ein Leuchtend | Wenn PC EIN: Dauerlicht; sonst AUS |
| AUS | Normalerweise aus |

Automatische Anzeigen haben Vorrang: **(1) erzwungenes Ausschalten 0,5/0,5 s, (2) WLAN-Verlust 0,3/0,3 s, (3) Power-Befehl zehn Sekunden sanft pulsieren, (4) gewählter LED-Modus**.

Der 100-ms-Takt der Referenz verwendet diese korrekten Bedingungen:

- Normal: (t % 20) < 10
- Langsam: (t % 55) < 5
- WLAN: (t % 6) < 3
- Ausschalten erzwingen: (t % 10) < 5

Die früher vorgeschlagene Bedingung (t % 5 < 20) ist **falsch** (praktisch immer wahr). Alte Doppelblitz-Muster bitte nicht wieder einbauen.

### 5. Zwingende elektrische Prüfungen

1. **ESP32:** Exaktes Board inklusive Revision und GPIO-Bezeichnungen feststellen. PWM-LED an GPIO2 nicht aus der Beschriftung D2 herleiten.
2. **Relais:** Versorgungsspannung, Eingangsstrom, HIGH/LOW-Schaltlogik, NO/COM/NC-Kontakte und sicheres Verhalten bei Reset/Boot mit Messgerät prüfen. Der Regler des ESP32 muss die notwendige Last tragen können oder die Relaisversorgung muss geeignet getrennt bereitgestellt werden.
3. **Mainboard:** Die PWR_SW-Kontakte **ausschließlich aus dem Handbuch des konkreten Mainboards** ermitteln. Nur potentialfreien COM/NO-Kontakt parallel zum Originaltaster anschließen. Kein 3,3-V-, 5-V- oder 12-V-Signal auf die PWR_SW-Leitungen legen.
4. **12-V-Erfassung:** Nur die geschaltete Haupt-12-V-Schiene, nicht 5VSB/Standby, abgreifen. Das reale PC817-Modul muss dafür ausgelegt sein.
5. **PLED-Erfassung:** PLED-Spannung, Polarität, Blinktakt und Eingangsstrom am echten Mainboard messen. Ein als 3,6–30 V beworbenes PC817-Modul erkennt möglicherweise eine 3,3-V-LED-Leitung nicht zuverlässig. Ohne geeignete Eingangsaufbereitung **nicht direkt anklemmen**. Gehäuse-LED nicht überlasten.
6. **Ausgangslogik:** An ESP32 GPIO21/GPIO22 darf maximal **3,3 V** liegen. Aktiv-LOW-Logik, Open-Collector-Ausgänge, Pull-ups sowie mögliche Jumper zwischen Eingangs- und Ausgangsmasse messen. Galvanische Trennung nicht blind annehmen.
7. **Sicherheit:** Keine Arbeiten an Netzspannung, kein Öffnen des ATX-Netzteils. Leitungen isolieren, kurzschlussschützen und mechanisch sichern. Der lange Power-Druck **ist kein normgerechter Not-Aus** und kann Datenverlust verursachen.

Wenn eine erforderliche Messung fehlt, bitte **nicht raten und kein fertiges Anschlussbild vortäuschen**, sondern zuerst die fehlenden Werte anfragen.

### 6. Kläre mit mir meine Hardware

Frage mich zuerst nach:

- Exaktem ESP32-Modell und Boardrevision (gern Fotos und Beschriftungen).
- Exaktem Relaismodul, IN/VCC/GND- und COM/NO/NC-Kennzeichnungen.
- Exaktem PC817-Modul mit Jumpern, Anschlussnamen und Datenblatt/Eingangsspannung.
- Mainboard-Modell und dessen Power-Switch-/Power-LED-Pinbelegung.
- Dauerhafter Stromversorgung für ESP32 und Relais.
- Quelle der geschalteten PC-12-V-Versorgung.
- Gemessener Mainboard-PLED-Spannung und -Polarität sowie ihrem Verhalten bei AUS/EIN/Standby.
- Bestätigtem GPIO der zweiten LED und Anzahl der zu steuernden PCs.

Bei mehreren PCs **neutrale, eindeutige Gerätenamen** benutzen (z. B. pc-steuerung-01 und pc-steuerung-02). Verwende lokale ESPHome-Secrets für WLAN, API-Schlüssel und OTA. Frage **nie** nach Passwörtern oder Schlüsseln zur Veröffentlichung.

### 7. Gewünschte Unterstützung und Ergebnisse

Arbeite in dieser Reihenfolge:

1. **Bestandsaufnahme:** Hardwarekompatibilität der aktuellen Referenz und sämtliche fehlenden Messungen benennen.
2. **Stückliste:** Alle Bauteile inklusive notwendiger elektrischer Mindestwerte auflisten.
3. **Getrennte Anschlusspläne:** Relaissteuerseite, trockener Relaiskontakt zum PWR_SW, GPIO18-Taster, 12-V-Optokoppler-Eingang und ESP32-Ausgang, sowie angepasster PLED-Eingang und ESP32-Ausgang. Pins eindeutig beschriften, keine frei erfundenen Anschlussbilder.
4. **ESPHome:** Die verlinkte aktuelle YAML auf mein bestätigtes Board anpassen. Vollständige funktionierende Datei ausgeben, syntaktisch prüfen/kompilieren, sofern tatsächlich möglich. Bestehende Funktionen nicht stillschweigend entfernen.
5. **Home Assistant:** Einrichtung der ESPHome-Integration, Liste der Buttons, Statussensoren, LED-Modi und Diagnose-Entitäten erläutern.
6. **Sichere Inbetriebnahme:** Erst ESP32 alleine flashen; dann Relais **ohne Mainboardverbindung** prüfen; danach 12-V- und PLED-Sensoren bei allen PC-Zuständen messen; erst abschließend COM/NO an PWR_SW anschließen.
7. **Testprotokoll:** die einstellbaren Impulszeiten von 0,2–1,5 s und 4–12 s, Schutz bei Boot/WLAN-Ausfall, LED-Test ohne Relais, zehnsekündige Pulsanimation, WLAN-Blinken, alle sieben Modi, EIN/AUS/Standby-Status, Ablehnung bei unbekanntem Zustand und Relaisfreigabe nach der gewählten Dauer, spätestens nach zwölf Sekunden, prüfen.
8. **Transparenz:** Offene Risiken, ungeprüfte Annahmen, nötige Messungen und nicht durchgeführte Hardwaretests explizit kennzeichnen.

Führe keine Änderungen an meinem GitHub, Home Assistant, ESP32 oder anderen Geräten aus, ohne dass ich sie konkret beauftrage. Gib keine vorgetäuschten Prüf- oder Installationserfolge aus.

---

## Hinweise zum Referenzprojekt

- Dieser Prompt gehört bewusst in den **separaten Repository-Ordner promts** und nicht in den Quellcode-Ordner des Projekts.
- Lies vor jedem Nachbau die aktuelle GitHub-Version der Firmware und Dokumentation. Alte Chatbeispiele können überholt sein.
- Der Prompt ist eine Arbeitsanweisung für eine KI und **keine Freigabe** zur unkontrollierten Verdrahtung.

**Zeitabgleich:** Normales Schalten via HA, Web oder GPIO18 = **einstellbar 0,2–1,5 s (Standard 0,5 s)**. Erzwungenes Ausschalten via HA oder Web = **einstellbar 4–12 s (Standard 10 s)**. LED-Test *ohne Relais* = weiterhin **6 s**, Pulsanimation nach normalem Power-Befehl = weiterhin **10 s**. Ein dauerhaft gehaltener Power-Schalter kann je nach BIOS/UEFI nach dem Ausschalten ein unerwünschtes Wiedereinschalten bewirken; Kontaktzeit erst getrennt vom Mainboard prüfen.

## Zusatzanforderung: gespeicherte Power-Zeiten und passwortgeschützte ESP32-Weboberfläche

Die aktuelle Firmware unterstützt jetzt eine **lokale ESPHome-Weboberfläche V3** mit **HTTP Digest** auf Port 80. Die Benutzeranmeldung erfolgt ausschließlich mit **lokalen Secrets** `pc_web_username` und `pc_web_password`. Web-OTA ist deaktiviert, ESPHome OTA bleibt nutzbar. **Nicht ins öffentliche Internet exponieren:** HTTP Digest ist kein HTTPS.

Zwei gespeicherte `number.template`-Regler (`optimistic: true`, `restore_value: true`) stehen in Home Assistant und direkt auf der Weboberfläche zur Verfügung:

- `PC Ein/Aus Impulsdauer`: **0,2–1,5 s**, 0,1-s-Schritte, Standard 0,5 s.
- `PC Not-Aus Impulsdauer`: **4–12 s**, 0,5-s-Schritte, Standard 10 s.

Die beiden Hauptbuttons heißen `PC Ein/Aus` und `PC Not-Aus (Ausschalten erzwingen)`. Die Relais-Skripte verwenden eine dynamische ESPHome-`delay: !lambda` mit **Millisekunden**, die Zeit wird zu Beginn des Impulses einmalig gelesen. Ungültige Werte werden sicher durch 500 ms bzw. 10 s ersetzt. Die Statusfreigabe über 12 V oder mindestens zwei Sekunden stabile Power-LED bleibt erhalten. LED-Test (sechs Sekunden **ohne Relais**) und zehnsekündiges weiches Power-Pulsieren sind von den Zeitreglern getrennt.

Bei der Anleitung **immer die aktuelle Firmware** und [Weboberflächen-Dokumentation](https://github.com/Technox90/projekte/blob/main/PC-Steurung/docs/07-weboberflaeche.md) verwenden, keine veralteten festen Zeiten voraussetzen. Bei Inbetriebnahme jede Zeit **mit vom Mainboard getrenntem Relaiskontakt** prüfen.

## Relaisschutz und Schutz der öffentlichen Webansicht

Die aktuelle Firmware enthält jetzt eine **gegenseitige Relaisverriegelung**: Während eines normalen Power-Tastendrucks kann das erzwungene Ausschalten den Kontakt nicht übernehmen oder verlängern; umgekehrt sind normale Impulse während erzwungenem Ausschalten gesperrt. Es gilt auch eine beim Start wirksame Boot-Schaltsperre. Nach jedem Kontaktabfall beginnt die **gespeicherte, einstellbare Schaltsperre `PC Relais Schaltsperre`** von 0,5–10 Sekunden (0,5-s-Schritte; Standard 2 s). Weitere Befehle in diesem Zeitraum werden abgewiesen. Die Relaislaufzeit bleibt auf maximal 1,5 Sekunden bzw. 12 Sekunden begrenzt. Software-Verriegelung ersetzt keine elektrische Sicherheitsmaßnahme.

**Wichtig für Web-Zugriff:** Der ESPHome-Webserver besitzt nur eine **globale** Digest-Authentifizierung und bietet **keinen getrennten Passwortschutz für nur zwei Buttons**. Daher die bestehende Webserver-Authentifizierung **nicht** entfernen. Eine wirklich anonyme, öffentliche Statusansicht muss getrennt von den authentifizierten, serverseitig abgesicherten Power-Steueraktionen bereitgestellt werden (z. B. Home-Assistant-Datenserver und Caddy). Die ESP32-Web-REST-Endpunkte sowie ESPHome-OTA-/API-Zugänge bleiben ausschließlich im vertrauenswürdigen Netz und dürfen nicht ins Internet weitergeleitet werden. Vor einer solchen Änderung den Benutzer fragen, ob er mit „öffentlich“ **ohne Login im lokalen LAN** oder eine **weltweit zugängliche Internetseite** meint.

## Aktuelle Vorgabe: lokales HTTPS-Webportal auf einer Adresse

Die frühere Idee einer öffentlich weltweit erreichbaren Statusseite mit mehreren Subdomains **wurde verworfen**. Die aktuell gewünschte Lösung ist **nur lokal**, vom Heimnetz unter einer einzigen privaten HTTPS-Adresse (z. B. `https://pc.home.arpa/`) erreichbar und bei Bedarf über **VPN** von unterwegs abrufbar. **Bestehende öffentliche Domains nicht dafür verwenden!** Keine direkten Portfreigaben ins Internet.

Pfadstruktur: `/` und `/status` ohne Anmeldung lesbar; `/login` für die Sitzung; `/steuerung` für die zwei Powerbuttons und `/einstellungen` für die drei gespeicherten Impuls-/Sperrzeiten – beide strikt **serverseitig passwortgeschützt**. Caddy terminiert `tls internal` und leitet ausschließlich an ein spezialisiertes Portal-Backend, **niemals direkt an die ungeschützte ESPHome-REST-API oder HA**. Für lokale TLS-Nutzung die Caddy-Root-CA auf Geräten vertrauen lassen. Das Portal verwendet gesicherte Sessions, Hash-Passwörter, CSRF-Schutz, feste Aktions-Whitelists und bewusste Bestätigung des langen Power-Drucks. Die originale ESPHome-Weboberfläche bleibt **global mit Digest geschützt** und nur intern zugänglich.

Authelia erfordert laut offizieller Dokumentation öffentlich zertifizierbare Cookie-Domains und ist daher **nicht** die vorausgesetzte Login-Lösung für den speziellen privaten Namensraum `home.arpa`. Stattdessen das Login im Portal-Backend planen. Der aktualisierte, maßgebliche Plan liegt unter: https://github.com/Technox90/projekte/blob/main/PC-Steurung/docs/09-lokales-portal-caddy.md . Der frühere Plan `08-oeffentliches-portal-caddy.md` ist archiviert. Alle bisher vorhandenen Firmware-Funktionen, Relais-Sperren und LED-Anzeigen bleiben bestehen. Die neuen Logs zeigen stabilen WLAN-Betrieb und funktionierende LED-Tests, aber keine bestätigten Relais-Schaltdauern.
