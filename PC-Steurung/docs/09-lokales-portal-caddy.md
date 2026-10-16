# Lokales PC-Steuerungsportal unter einer HTTPS-Adresse (Caddy)

**Status: Architekturplan; die vorbereitenden Portal-Statussignale sind bereits in der GitHub-ESPHome-Firmware umgesetzt, das Caddy-Portal selbst ist noch nicht installiert.** Dieser Plan ersetzt die frühere, weltweit öffentlich erreichbare Drei-Subdomain-Variante in [08-oeffentliches-portal-caddy.md](08-oeffentliches-portal-caddy.md). Die originale ESPHome-Steuerung und ihre global passwortgeschützte Wartungsoberfläche bleiben erhalten. Die Firmware stellt zusätzlich ausschließlich lesende Statussignale für Home Assistant bereit. Es gibt **keine öffentliche Portweiterleitung** zum ESP32 oder zum Home-Assistant-System.

## Ziel

Eine **lokale** Website, die von PCs und Mobilgeräten im eigenen Heimnetz über eine einzige HTTPS-Adresse erreichbar ist. Optionaler Zugriff von außerhalb des Heimnetzes erfolgt **ausschließlich über ein privates VPN** (z. B. WireGuard auf der Fritz!Box), nicht über eine anonym weltweit erreichbare Webseite.

Vorgeschlagener lokaler Name: **https://pc.home.arpa/** (frei änderbarer Beispielname). \`home.arpa\` ist ein gemäß RFC 8375 reservierter lokaler Namensraum und erfordert **keine registrierte öffentliche Domain**.

| Route | Sichtbarkeit im Heimnetz | Funktion |
|---|---|---|
| \`/\` | Ohne Anmeldung | Einfacher PC- und Controller-Status |
| \`/status\` | Ohne Anmeldung | Freigegebene Diagnose- und Hardwarestatusdaten |
| \`/login\` | Ohne Anmeldung erreichbar | Passwortanmeldung |
| \`/logout\` | Mit aktiver Sitzung | Sitzung beenden |
| \`/steuerung\` | Nur nach Anmeldung | **PC Ein/Aus**, **PC Not-Aus** |
| \`/einstellungen\` | Nur nach Anmeldung | Normalzeit, Not-Aus-Zeit, Relais-Nachlauf-Sperre |
| \`/api/status\` | Nur lesend, ohne Anmeldung | Stark eingeschränkte, nicht vertrauliche Statusdaten |
| \`/api/control/... \` | Nur nach Anmeldung + serverseitiger Prüfung | Konkrete whitelisted Power-Aktionen |
| \`/api/settings/... \` | Nur nach Anmeldung + serverseitiger Prüfung | Konkrete whitelisted Zeitänderungen |

Die Buttons dürfen auf der anonymen Statusseite optisch angezeigt werden, **aber dort keinen Schaltbefehl ausführen**. Ein Klick führt erst zur \`/login\`-Seite bzw. zur authentifizierten \`/steuerung\`-Route. Selbst bei Manipulation der Benutzeroberfläche dürfen nicht angemeldete Requests **serverseitig** keinen Power-Befehl auslösen.

## 1. Netz und DNS

- Pi-hole/Unbound oder der im Heimnetz tatsächlich verwendete DNS-Server löst **pc.home.arpa** auf die **interne Caddy-Adresse** auf. Die tatsächlichen IP-Adressen erst vor der Einrichtung ermitteln; bestehende CT-/VM-Nummern nicht ungeprüft übernehmen.
- \`home.arpa\` ist privat und wird **nicht** im öffentlichen DNS als globale Website verfügbar gemacht.
- Caddy Port 443 muss von vertrauenswürdigen Heimnetz-Clients und gegebenenfalls VPN-Clients aus erreichbar sein. **Keine WAN-Portfreigabe** für diesen Hostnamen, ESP32-Webport 80, ESPHome API/OTA oder HA-Port 8123.
- Für Zugriff von unterwegs WireGuard-VPN oder vergleichbares privates VPN einrichten und dabei **DNS für pc.home.arpa über das Heimnetz** bereitstellen. Ohne VPN ist die Website von außerhalb nicht erreichbar.
- Home Assistant und ESPHome bleiben intern. Der bereits vorhandene ESPHome-Webserver V3 mit globaler Digest-Anmeldung bleibt als lokale **Wartungsoberfläche** aktiv, wird jedoch **nicht** per Caddy öffentlich gespiegelt.

## 2. Lokales HTTPS

Caddy kann mit **\`tls internal\`** ein Zertifikat aus seiner lokalen CA ausstellen. Dann wird auf jedem Client, der die Website ohne Browser-Warnung öffnen soll, **das öffentliche Root-CA-Zertifikat** der lokalen Caddy-CA als vertrauenswürdig installiert. Den **privaten CA-Schlüssel niemals auf Endgeräte kopieren**. Diese Zertifikate sind nicht öffentlich signiert.

Beispiel-Caddyfile (nur Architektur, nicht direkt einspielen!):

~~~caddyfile
pc.home.arpa {
    tls internal
    encode zstd gzip
    header {
        X-Content-Type-Options "nosniff"
        X-Frame-Options "DENY"
        Referrer-Policy "no-referrer"
        Strict-Transport-Security "max-age=31536000"
    }
    # PC_PORTAL_UPSTREAM muss als interne Umgebungsvariable gesetzt sein,
    # z. B. auf den internen HTTP-Port einer dedizierten Portal-VM/CT.
    # Niemals HA oder den ESPHome-Webserver als direktes Ziel eintragen!
    reverse_proxy {$PC_PORTAL_UPSTREAM}
}
~~~

Beispiel: Der Reverse Proxy führt nicht selbst die Power-Kommandos aus, sondern leitet ausschließlich zum **dedizierten Portal-Backend**. Die Zugangskontrolle für die Routen ist dort zentral und **serverseitig** implementiert. Kein blindes Proxying von Benutzer-URLs zur Home-Assistant-API.

## 3. Login im selben Host unter /login

Da eine **echte Login-Seite** \`/login\` auf derselben Adresse gewünscht ist, wird dafür ein Portal-Backend mit Benutzerdatenbank und **sessionbasierter Authentifizierung** geplant:

- Passwörter nur als starker, gesalzener Hash (z. B. Argon2id), niemals Klartext speichern.
- Erfolgreicher Login setzt ein **Secure**, **HttpOnly**, **SameSite=Lax/Strict** Session-Cookie. Serverseitige Session-Validierung, Ablauffrist und echte Logout-Invalidierung.
- Loginversuche begrenzen, Fehlversuche protokollieren, sichere Passwort-Rücksetzung vorsehen; zusätzliche TOTP-2FA optional für Steuerungsaktionen.
- Jeder schreibende Request erfordert gültige Sitzung, **CSRF-Schutz**, Prüfung von \`Origin\`, korrekte HTTP-Methode **POST** und die explizite Berechtigung zur Aktion. GET führt niemals einen Power-Befehl aus.
- Der lange Tastendruck \`PC Not-Aus\` erfordert zusätzlich eine **bewusste Bestätigung**. Ein reines Anzeigen oder Klicken auf einen nicht freigeschalteten Button ist kein Autorisierungsschutz.
- Das Backend hält den benötigten HA-Token **nur serverseitig** und bietet eine **feste Whitelist** für genau die benötigten ESPHome-Buttons und Number-Entitäten. Keine allgemeine Weiterleitung von HA-Service-/Entity-Namen aus Browseranfragen zulassen.
- Das Portal-Backend darf nur von Caddy und definierten Administrationssystemen erreichbar sein; Home Assistant/ESPHome dürfen vom Internet nicht direkt angesprochen werden.
- Status-Endpunkte geben höchstens freigegebene PC-Zustände, Verbindung und WLAN-Qualität aus. Keine WLAN-Passwörter, Tokens, privaten Netzdetails, vollständigen HA-Attribute oder vertrauliche Logs.
- Fail closed: HA/ESPHome nicht erreichbar -> **Unbekannt/Offline**, nicht fälschlich \`PC AUS\`. Akzeptierter API-Befehl bedeutet nicht zwingend erfolgreiche physische Zustandsänderung.

**Warum nicht einfach den nativen ESPHome-Webserver ohne Passwort spiegeln?** Sein globaler Digest-Schutz kann nicht einzelne Power-REST-Endpunkte gezielt freigeben. Ohne globale Anmeldung wären Power-URLs direkt aufrufbar, auch wenn man die Buttons versteckt.

**Warum kein Authelia als Voraussetzung für \`pc.home.arpa\`?** Die aktuelle Authelia-Dokumentation setzt für Cookie-Domains einen regulären, öffentlich zertifizierbaren Domainnamen voraus. Für einen rein privaten Sondernamen ist daher ein gesondertes Portal-Backend mit korrekt implementierten eigenen Sessions die kontrollierbarere Planung. Falls stattdessen Authelia gewünscht ist, muss sein unterstütztes Domain-/TLS-Modell vorher gesondert geprüft und gegebenenfalls ein anderer Namensraum gewählt werden.

## 4. Vorhandene ESPHome-Funktionen unverändert lassen

- Normaler Power-Impuls: 0,2–1,5 Sekunden, Standard 0,5 s.
- Langer Power-Impuls („Not-Aus“ im UI, **kein normgerechter Not-Aus**): 4–12 Sekunden, Standard 10 s.
- Einstellbare \`PC Relais Schaltsperre\`: 0,5–10 s, Standard 2 s; gegenseitige Verriegelung der Relaisbefehle.
- Hardwarestatus: geschaltete 12-V-Schiene oder mindestens zwei Sekunden stabile Mainboard-Power-LED (im Standby zu prüfen).
- LED-Modi, LED-Animationen, WLAN-Warnung und separater relaisfreier LED-Test bleiben bestehen.
- Änderungen von Zeiten erfolgen im geschützten \`/einstellungen\` ausschließlich über erlaubte HA-Number-Entitäten.

## 5. Prüfung vor Inbetriebnahme

1. Lokale Domain-Auflösung \`pc.home.arpa\` vom Heimnetz und VPN prüfen.
2. Caddy-\`tls internal\`-Zertifikat und **Root-CA-Vertrauen** auf Testgeräten prüfen.
3. Anonymer Client: \`/\`, \`/status\`, \`/api/status\` lesbar, **keine schreibende Aktion möglich**.
4. Anonymer Client: \`/steuerung\`, \`/einstellungen\` und sämtliche Schreib-API-Endpunkte nachweislich abgewiesen.
5. Authentifizierter Client: Buttons und Zeitänderungen nur innerhalb zulässiger Werte und nur über Whitelist.
6. CSRF-, Session-, Cookie-, Expiry-, Doppelbefehls-, Rate-Limit- und Not-Aus-Bestätigungsprüfung bestehen.
7. Bei HA/ESPHome-Ausfall zeigt Status \`Unbekannt\`, nicht \`AUS\`.
8. Browsernetzwerkanalyse: keine HA-/ESPHome-Secrets und keine direkten internen HA/ESPHome-URLs im JavaScript.
9. Alle Schaltbefehle zuerst mit **vom PC-Mainboard getrenntem Relais** messen.
10. Kein externer Zugriff auf Caddy-Host oder ESP32 ohne authentifizierten VPN-Tunnel.

## 6. Log-Auswertung der ersten ESP32-Tests

Aus dem vom Nutzer vorgelegten Log:
- WLAN überwiegend **−42 bis −51 dBm**: guter bis sehr guter Empfang.
- \`Boot seems successful\` bedeutet normaler erfolgreicher ESPHome-Start, keine Fehlermeldung.
- ESP32-Uptime steigt bis **932 s**; in diesem Logauszug ist **kein Neustart erkennbar**.
- Zwei LED-Tests: \`LED-Test gestartet\` -> \`LED-Test beendet\` nach ungefähr **6 s**; dies prüft den LED-Ausgang, **nicht** den tatsächlichen Relaiskontakt.
- \`PC Ein/Aus Impulsdauer\` wurde auf **1,00 s**, danach **0,70 s** verändert, ESPHome bestätigt später einen Präferenz-Speichervorgang \`1 written, 0 failed\`.
- Die Meldungen \`Roam check skipped, signal good\` bzw. \`Roam scan\` zeigen lediglich übliche WLAN-Überprüfungen bei ordentlichem Signal.
- Diese Logs beweisen **nicht**, dass die aktuelle Firmwareversion mit Relaissperre installiert ist, dass der Relaiskontakt tatsächlich die eingestellte Dauer geschlossen bleibt oder dass die 12-V-/Power-LED-Erkennung am realen PC validiert ist.

## Offene Details für die spätere Realisierung

- Namen bestätigen: \`pc.home.arpa\` oder anderer **privater** Hostname.
- Bestehende Caddy-Installation und ihr tatsächlicher CT-/VM-Standort/Port prüfen.
- DNS-Weg: Pi-hole/Unbound tatsächlich von Endgeräten verwendet?
- Mobiler Fernzugriff ausschließlich über VPN gewünscht oder **überhaupt nicht**?
- Soll \`/login\` Passwort allein verlangen oder zusätzlich TOTP?
- Eventuell zusätzliche Sichtbarkeit von 12-V-Sensor, PLED, RSSI, ESP32-Uptime ohne Anmeldung.

**Dies ist ein Plan.** Noch kein neues Portal, keine Caddy-/DNS-/VPN-Änderung und keine neue Login-Datenbank wurden installiert.

## Dokumentation

- RFC 8375 für \`home.arpa\`: https://www.rfc-editor.org/rfc/rfc8375
- Caddy lokale HTTPS-CA: https://caddyserver.com/docs/running#local-https-with-systemd
- Caddy TLS: https://caddyserver.com/docs/caddyfile/directives/tls
- Authelia Cookie-Domains (Einschränkungen): https://www.authelia.com/configuration/session/introduction/
- Caddy Reverse Proxy: https://caddyserver.com/docs/caddyfile/directives/reverse_proxy
- ESPHome Webserver: https://esphome.io/components/web_server/

## 7. Ergänzte ESPHome-Signale für den Statusadapter

Die aktuelle [ESPHome-Firmware](../firmware/pc-steuerung.yaml) enthält jetzt fünf vorbereitende Signale. Sie werden über die verschlüsselte native API an Home Assistant übertragen; für den lokalen Viewer erfolgt **nur eine ausdrücklich erlaubte, lesende Ausgabe** aus einem getrennten Backend. Die Statussignale selbst sind **keine HTTP-API mit anonymen Schreibrechten**.

| ESPHome/HA-Anzeigename | Erlaubte Verwendung im Portal | Quellbedingung |
|---|---|---|
| `PC Zustand` | Öffentlicher PC-Status EIN/AUS/UNBEKANNT | Bestehender kombinierter Hardwarestatus `PC Betrieb erkannt`; beim Start zunächst UNBEKANNT |
| `PC Steuerung Status` | Anzeige „Bereit“, „Relais aktiv“, „Schaltsperre aktiv“ oder „Ausschaltkontrolle aktiv“ | ESP32-Skriptausführung / Relaisverriegelung |
| `PC Relais aktiv` | Optionaler Laufindikator | Softwarevariable für einen laufenden Impuls |
| `PC Schaltsperre aktiv` | Hinweis, warum gerade nicht geschaltet werden kann | Nachlauf-/Boot-Sperrzeit |
| `PC Schaltsperre Restzeit` | Countdown in Sekunden | Lokaler ESP32-`millis()`-Zähler |

Hinzu kommen die bisherigen Diagnosewerte `ESP32 Online` und `WLAN Signal` (nur falls für anonyme Besucher gewünscht). Die im Home Assistant **tatsächlich vergebenen Entity-IDs** sind erst nach Firmware-Update im HA-Geräteregister zu prüfen und dann als **feste Whitelist** im Portal-Backend einzutragen. Keine Entity-IDs aus diesen Anzeigenamen blind annehmen.

**Wichtig für die Authentisierung:** Drei bestehende Number-Entitäten (`PC Ein/Aus Impulsdauer`, `PC Not-Aus Impulsdauer`, `PC Relais Schaltsperre`) und zwei Power-Buttons sind **Schreibfunktionen**. Sie dürfen nur durch die angemeldete Steuerungs-API unter `/steuerung` oder `/einstellungen` angesprochen werden. Ein eigenes `/login` entsteht **nicht** durch ESPHome-`web_server` und muss unabhängig im Backend realisiert und getestet werden.

**Statusgrenzen:** Wenn beide PC817-Eingänge zwar elektrische Pegel liefern, aber falsch oder überhaupt nicht angeschlossen sind, kann der ESP32 ein falsches `AUS` ausgeben. Deswegen müssen die Signale mit dem realen PC im Zustand EIN, AUS und Standby vor Veröffentlichung freigegeben werden.
