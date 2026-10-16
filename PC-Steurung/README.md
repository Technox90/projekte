# PC-Steuerung mit ESP32, ESPHome und Home Assistant

Generisches Projekt zur Steuerung des PC-Power-Tasters und zur getrennten Erfassung von zwei Hardware-Statussignalen. Ohne Personennamen, Kennwörter oder individuelle Geräte-IDs.

> **Wichtig:** Dies ist eine dokumentierte Firmware-Vorlage, kein elektrisch freigegebener Fertigaufbau. Die tatsächlichen Relais-/PC817-Platinen müssen vor dem Anschluss anhand ihrer Beschriftungen und durch Messungen geprüft werden. Die Firmware wurde **nicht auf einem echten Gerät kompiliert oder elektrisch getestet**. Beachte das [Prüfprotokoll](docs/06-pruefprotokoll.md).

## Funktionen

| Funktion | Ergebnis |
|---|---|
| PC Ein/Aus (Home Assistant und Web) | Einstellbarer Power-Impuls 0,2–1,5 s (Standard 0,5 s) |
| Lokaler Taster | GPIO18 gegen GND, gleicher einstellbarer Normalimpuls |
| PC Not-Aus (Home Assistant und Web) | Einstellbarer langer Tastendruck 4–12 s (Standard 10 s), nur bei geprüfter 12-V- ODER stabiler Power-LED-Freigabe |
| Relaisschutz | Gegenseitige Verriegelung; gespeicherte Nachlauf-Sperrzeit 0,5–10 s (Standard 2 s), Boot-Sperre |
| PC 12-V-Status | Separater binärer Sensor an GPIO22 über PC817 Kanal 1 |
| Mainboard-Power-LED | Separater binärer Sensor an GPIO21 über PC817 Kanal 2 (passende Eingangsstufe erforderlich) |
| LED-Betriebsarten | 7 wählbare, gespeicherte Modi |
| Normaler Power-Befehl | 10 s weich pulsierende LED |
| WLAN-Ausfall | 0,3 s EIN / 0,3 s AUS |
| Erzwungenes Ausschalten aktiv | 0,5 s EIN / 0,5 s AUS bis PC aus gemeldet |
| Diagnose | ESP32-Verbindung, WLAN-Signal, Uptime |
| Updates/Sicherheit | ESPHome OTA, verschlüsselte HA-API, Fallback-WLAN und passwortgeschützte ESP32-Weboberfläche |

**GPIO2 steuert ausschließlich eine eventuell GPIO-gesteuerte zweite Board-LED.** Die gewöhnliche **PWR-LED** ist direkt an die Spannungsversorgung angeschlossen und lässt sich nicht per ESPHome ansteuern. Die Kennzeichnung D2 auf einer Platine allein beweist nicht, dass diese LED an GPIO2 liegt.

## Inhalt des Ordners

- [Vollständige Firmware](firmware/pc-steuerung.yaml)
- [Vorlage für lokale Secrets](firmware/secrets.example.yaml)
- [Bauteile und Voraussetzungen](docs/01-bauteile.md)
- [Anschluss- und Prinzipschaltpläne](docs/02-anschlussplaene.md)
- [ESPHome- und Home-Assistant-Installation](docs/03-installation.md)
- [Lokale Weboberfläche und Zeitregler](docs/07-weboberflaeche.md)
- [Aktueller Plan: lokales HTTPS-Portal mit Caddy, /login und geschützter Steuerung](docs/09-lokales-portal-caddy.md)
- [Frühere, nicht mehr vorgesehene öffentliche Internet-Variante](docs/08-oeffentliches-portal-caddy.md)
- [LED-Modi und Schaltlogik](docs/04-betriebslogik.md)
- [Fehlerbehebung](docs/05-fehlersuche.md)
- [Elektrisches Prüfprotokoll](docs/06-pruefprotokoll.md)

## Kurzinstallation

1. Alle Zusatzmodule vom ESP32 trennen und den ESP32 per USB-Datenkabel anschließen.
2. Die [Firmware](firmware/pc-steuerung.yaml) in den ESPHome Device Builder kopieren.
3. Die benötigten Werte aus [secrets.example.yaml](firmware/secrets.example.yaml) **lokal** als echte Secrets anlegen, nicht ins Repository hochladen.
4. Konfiguration prüfen, Firmware kompilieren, über USB installieren.
5. WLAN und Home-Assistant-API testen.
6. Erst dann einzelne Module anhand der [Prinzipschaltpläne](docs/02-anschlussplaene.md) separat verdrahten und nach [Prüfprotokoll](docs/06-pruefprotokoll.md) testen.
7. Relais COM/NO erst nach erfolgreicher Kontaktprüfung parallel zum PC-POWER-SW anschließen.

## Elektrische Sicherheitsregeln

- **Keine 3,3 V, 5 V oder 12 V** auf den Mainboard-Power-Switch geben. Dort werden nur potentialfreie NO/COM-Kontakte parallel zum Taster geschlossen.
- **Keine 12 V oder 5 V** an GPIO21/GPIO22 des ESP32.
- Bei vielen PC817-Boards mit **3,6–30 V Eingangsspannung** ist die Mainboard-Power-LED mit unter Umständen nur 3,3 V **nicht direkt kompatibel**. Der Kanal bleibt bis zur Messung elektrisch offen.
- Manche PC817-Module verbinden über Jumper Eingangs- und Ausgangsmasse. Vor einer behaupteten galvanischen Trennung **Durchgang messen**.
- Die 3-V-Aufschrift einer Relais-Spule beweist **nicht** die Eignung der gesamten Platine an einem 3,3-V-Ausgang. Spulenstrom und Spannungsversorgung messen.
- Das erzwungene Ausschalten über einen einstellbar 4–12 Sekunden gehaltenen ATX-Power-Taster ist **kein sicherheitstechnischer Not-Aus** und kann Datenverlust verursachen.
- Niemals das PC-Netzteil öffnen, keine Netzspannungsleitung bearbeiten.

## Quellen

- [ESPHome WiFi](https://esphome.io/components/wifi/)
- [ESPHome LEDC PWM](https://esphome.io/components/output/ledc/)
- [ESPHome GPIO Sensor](https://esphome.io/components/binary_sensor/gpio/)
- [ESPHome Template Select](https://esphome.io/components/select/template/)
- [ESPHome Scripts](https://esphome.io/components/script/)

**Status:** Dokumentierte Vorlage – reale GPIO2-LED, Optokoppler-Ausgangslogik und Mainboard-LED-Eingang müssen noch je Board-Revision geprüft werden.

## Diagnose der Ausschalt-Warn-LED (ab Revision 2)

Die Firmware besitzt zusaetzlich den Button **Not-Aus LED Test (ohne Relais)**. Er laesst die zweite, per GPIO2 steuerbare LED fuer sechs Sekunden mit **500 ms EIN / 500 ms AUS** blinken, **ohne** den Power-Taster oder das Relais zu betaetigen.

Der Textsensor **Ausschalten Rueckmeldung** meldet, ob das erzwungene Ausschalten begonnen hat oder mangels **12 V EIN oder stabiler Power-LED EIN** abgelehnt wurde. Bei Ablehnung erfolgt kein langer Power-Taster-Druck. Der Test-Button ist deshalb der sichere Weg, den LED-Ausgang unabhaengig von der 12-V-Verdrahtung zu pruefen.

## PC-EIN-Erkennung aus zwei Quellen

**GPIO22 (12 V) ODER GPIO21 (Power-LED)** werden gemeinsam fuer die Freigabe des einstellbaren langen Power-Tastendrucks und die normalen LED-Modi ausgewertet. Die Power-LED wird nur als PC-EIN-Kriterium gewertet, wenn sie **mindestens zwei Sekunden ohne Unterbrechung leuchtet**, damit ein kurzer Standby-Blinkimpuls nicht sofort den langen Power-Tastendruck freigibt. Neue HA-Entitaeten: **PC Power LED stabil** und **PC Betrieb erkannt**. Die Abschalt-Warnanzeige endet erst, wenn beide Quellen AUS sind (drei Sekunden stabiler Nachlauf). Dies ist keine sichere Erkennung aller Standby-Varianten: **EIN, AUS und Standby am realen Rechner messen**, bevor das erzwungene Ausschalten verwendet wird. Mehr dazu unter [Betriebslogik](docs/04-betriebslogik.md).

**Einstellbare Schaltzeiten:** Normaler PC-Power-Befehl (Home Assistant, Web oder GPIO18): **0,2–1,5 s**, Standard **0,5 s**. Erzwungenes Ausschalten: **4–12 s**, Standard **10 s**. Die gewählten Werte sind auf dem ESP32 gespeichert. Der getrennte, relaisfreie LED-Test bleibt bei **6 Sekunden**, die Power-Pulsanimation bei **10 Sekunden**. Ein langer Druck kann zu Datenverlust und je nach Mainboard/BIOS zu unerwartetem Wiederanlaufen führen; vor produktiver Benutzung mit getrenntem Relaiskontakt messen.

## Lokale Weboberfläche und Zeitregler

Die Firmware enthält einen ESPHome-Webserver V3 mit **HTTP Digest** und zwei **lokalen** Secrets: `pc_web_username` und `pc_web_password`. Nach Installation unter `http://pc-steuerung.local/` oder der lokalen IP-Adresse erreichbar. Die zwei Hauptbuttons **PC Ein/Aus** und **PC Not-Aus (Ausschalten erzwingen)** sowie die beiden Power-Zeitregler sind über die Weboberfläche und Home Assistant nutzbar. Der LED-Test ist eine zusätzliche Diagnosefunktion. Nur im vertrauenswürdigen LAN betreiben: **keine öffentliche Portfreigabe**.

**Vor dem Aktualisieren:** Die beiden neuen Web-Secrets im ESPHome Device Builder lokal anlegen und [Anleitung für Weboberfläche und Regler](docs/07-weboberflaeche.md) lesen.

## Relaisschutz

Ein **Schaltvorgang verriegelt das Relais**, bis der Kontakt nach der eingestellten Dauer wieder geöffnet wurde. Währenddessen werden normale und erzwungene Power-Befehle nicht parallel ausgeführt; ein langer Power-Befehl unterbricht keinen laufenden kurzen Tastendruck mehr. Nach dem Öffnen beginnt eine einstellbare **PC Relais Schaltsperre** von 0,5–10 s (Standard **2 s**, `restore_value: true`). In dieser Zeit wird kein neuer Relaisimpuls gestartet. Auch nach dem ESP32-Neustart gilt zunächst die Standardsperre. Bei Ablehnung des langen Tastendrucks erscheint eine entsprechende Rückmeldung.

**Wichtig zur noch gewünschten öffentlichen Status-Webseite:** Der ESPHome-Webserver unterstützt nur eine **globale** Digest-Anmeldung, keine separate Anmeldung ausschließlich bei Power-Buttons. Bis eine zusätzliche sichere Status-/Steuerlösung festgelegt ist, bleibt die **gesamte native ESP32-Weboberfläche passwortgeschützt**. Ein Entfernen der Authentifizierung würde auch die Power-REST-Endpunkte freigeben. Es gibt deshalb **keine** öffentliche ESP32-Portfreigabe.
