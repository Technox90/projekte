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
- [ ] einstellbarer normaler Impuls von 0,2–1,5 s (Standard 0,5 s) gemessen, ohne PC-Mainboard
- [ ] Lokaler GPIO18-Taster löst genau einen Impuls mit der aktuell eingestellten normalen Dauer aus
- [ ] einstellbaren 4–12-s-Impuls separat und **ohne Mainboard** mit simulierter, überprüfter 12-V-Statusmeldung getestet
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

## OR-Erkennung bei mehreren PC-Signalen

- [ ] PC EIN: GPIO22/12 V korrekt EIN
- [ ] PC EIN: GPIO21/Power-LED korrekt EIN und mindestens 2 s stabil
- [ ] Falls nur eine Quelle angeschlossen ist: Ausschalten-erzwingen-Freigabe nachweislich korrekt
- [ ] PC AUS: Keine der beiden Quellen meldet EIN; PC Betrieb erkannt wird nach 3 s AUS
- [ ] Standby/S3: PLED-Blinkrhythmus gemessen und bestaetigt, dass die 2-s-Stabilitaetspruefung keinen unbeabsichtigten langen Power-Tastendruck freigibt
- [ ] Signalunterbrechung der PLED fuehrt nicht zum vorzeitigen Ende der erzwungenen Ausschaltanzeige
- [ ] Keine ungewollte Aktivierung durch nachlaufenden Kombinationszustand (Relaisfreigabe benutzt aktuelle Ursprungswerte)

- [ ] Separater LED-Test bleibt auf 6 Sekunden begrenzt und schaltet **niemals** das Relais
- [ ] Tatsächliche Freigabe-/Abfallzeit des Relais bei 500 ms und 10 s gemessen; Kontakt löst danach zuverlässig

## ESP32-Weboberfläche und variable Zeiten

- [ ] Individuelle Web-Zugangsdaten sind ausschließlich als lokale Secrets hinterlegt
- [ ] ESPHome-Konfiguration einschließlich Webserver V3 / Digest kompiliert
- [ ] HTTP-Weboberfläche lässt sich aus dem vertrauenswürdigen lokalen Netzwerk mit Passwort öffnen
- [ ] Ohne gültige Web-Zugangsdaten keine Bedienung möglich
- [ ] Keine WAN-Portfreigabe und keine öffentliche Web-Weiterleitung vorhanden
- [ ] Über HA und Web beide Zeitregler gleichermaßen einstellbar und synchron
- [ ] Normalimpuls: Minimum 0,2 s / Standard 0,5 s / Maximum 1,5 s mit Messgerät nachgewiesen
- [ ] Langer Impuls: Minimum 4 s / Standard 10 s / Maximum 12 s mit Messgerät nachgewiesen, **Mainboard dabei abgeklemmt**
- [ ] Relais fällt nach jedem Impuls automatisch ab und bleibt beim Neustart AUS
- [ ] Veränderte Reglerwerte bleiben nach Neustart gespeichert
- [ ] Not-Aus LED Test (ohne Relais) dauert weiterhin 6 Sekunden und betätigt den Relaiskontakt nicht

## Relaisverriegelung und Nachlauf-Sperrzeit

- [ ] Normal- und Not-Aus-Button bei laufendem Relaisimpuls gleichzeitig getestet: kein zweiter oder überlappender Kontaktimpuls
- [ ] Langer Power-Befehl unterbricht einen laufenden Normalimpuls **nicht**
- [ ] Schaltsperre von 0,5–10 s anpassbar, Standard 2 s, nach Relaisabfall gemessen
- [ ] Während der Sperre werden weitere Power-Befehle abgelehnt
- [ ] Nach dem Neustart ist das Relais AUS und die Boot-Schaltsperre wirkt
- [ ] Ungültige Zeitwerte überschreiten niemals die maximalen 1,5 s / 12 s
- [ ] Falsche PC-EIN-Erkennung führt **nicht** zur Freigabe des langen Tastendrucks
- [ ] Nach Neustart bleibt die eingestellte Sperrzeit auf dem ESP32 gespeichert
- [ ] Sämtliche Kontaktprüfungen zuerst **ohne Mainboard-Anschluss** durchgeführt

**Web-Sicherheit:** Die native ESPHome-Webseite bleibt bis zum gesonderten Aufbau einer öffentlichen Statusseite **vollständig** Digest-passwortgeschützt. Kein direktes Freischalten des ESP32-Webports für Besucher.

## Portal-Firmware 2026-10: zusätzliche Status-Entitäten prüfen

- [ ] Neue Firmware in ESPHome **Konfiguration überprüfen** und ohne Fehler kompilieren.
- [ ] Home Assistant zeigt `PC Zustand` als EIN/AUS/UNBEKANNT, bei noch nicht initialisierten Quellen UNBEKANNT.
- [ ] `PC Steuerung Status` wechselt zwischen Bereit, Relais aktiv, Schaltsperre aktiv und gegebenenfalls Ausschaltkontrolle aktiv.
- [ ] `PC Relais aktiv` bleibt nur während eines echten Impulses EIN (Softwarezustand; reale Kontaktzeit mit Messgerät prüfen).
- [ ] `PC Schaltsperre aktiv` und `PC Schaltsperre Restzeit` zeigen bei Neustart und nach dem Impuls die Sperre an; anschließend Restzeit 0.
- [ ] Elektrische PC817-Signale im PC AUS, EIN und Standby kontrollieren; kein unüberprüftes „PC AUS“ veröffentlichen.
- [ ] Interne ESPHome-Weboberfläche bleibt weiterhin vollständig mit Digest-Anmeldung geschützt.
- [ ] Der lokale Caddy-Viewer bekommt nur freigegebene read-only HA-Signale, nicht ESPHome-/HA-Zugangsdaten.
- [ ] `/login`, `/steuerung` und `/einstellungen` sind erst dann erreichbar, wenn das separate Portal-Backend installiert wurde.
- [ ] Tatsächliche HA-Entity-IDs nach dem Flashen ermitteln und nur explizit freigegebene IDs im Portal hinterlegen.
