# Überholtes Konzept: öffentliches Internetportal mit Caddy

**ARCHIV – NICHT MEHR UMSETZEN:** Die früher gewünschte öffentlich erreichbare Drei-Subdomain-Variante wurde verworfen. Der **aktuelle** Plan ist eine einzige **lokale HTTPS-Adresse mit /login, /steuerung und /einstellungen**, ohne Veröffentlichung ins Internet: [09-lokales-portal-caddy.md](09-lokales-portal-caddy.md). Der Rest dieses Dokuments ist nur zur historischen Nachvollziehbarkeit belassen.

---

# Architekturplan: weltweit öffentliches Statusportal mit Caddy

**Status: Konzept, noch NICHT installiert oder produktiv getestet.** Keine Änderung der bestehenden ESPHome-Webserver-Authentifizierung oder Freigabe des HA-/ESPHome-Ports! Dieser Plan ist eine technische Vorlage. Vor einer Installation müssen Domain, Caddy-Laufzeitumgebung, DNS, WAN-Erreichbarkeit, Firewall-Regeln, Backends und Authentifizierung geprüft werden.

## Ziel und Abgrenzung

- **Weltweit öffentlich, ohne Anmeldung:** freigegebene, lesende PC-Statusanzeige: PC EIN/AUS/UNBEKANNT, ESP32 online/offline, optional WLAN-Qualität und Zeitpunkt der letzten Aktualisierung.
- **Nur nach starker Anmeldung:** PC Ein/Aus, PC Not-Aus (langer Power-Taster), Einstellen der zwei Impulszeiten und der Schaltsperre.
- Die öffentlichen Daten sind eine bewusst reduzierte Whitelist; **keine** Home-Assistant-API-Tokens, Geräte-IP-Adressen, MAC-Adressen, WLAN-SSIDs, laufende andere Dienste, Logs oder technische Kontrollen offenlegen.
- Der native ESPHome-Webserver V3 (HTTP Digest) bleibt **ausschließlich intern** und weiterhin **global passwortgeschützt**. Die ESPHome-/HA-Endpunkte dürfen nicht direkt veröffentlicht werden.
- **Vorhandene Steuerungslogik bleibt:** Normalkontakt 0,2–1,5 s (Standard 0,5 s), langer Kontakt 4–12 s (Standard 10 s), Software-Verriegelung, Schaltsperre 0,5–10 s (Standard 2 s), Status-Freigabe aus geschalteten 12 V ODER qualifizierter Power-LED. Die lange ATX-Taste ist kein sicherheitstechnischer Not-Aus.

## Trennung in drei HTTPS-Hosts

Alle Hostnamen sind **Beispiele**, nicht bestehende Domains und vor Benutzung zu ersetzen.

| Beispiel-Hostname | Sichtbarkeit | Dienst |
|---|---|---|
| \`pc.example.de\` | Weltweit ohne Login | **Nur lesendes** Statusportal, ohne Befehls- und Einstell-Endpunkte |
| \`steuerung.example.de\` | Authentifizierung zwingend | Administrationsoberfläche und streng definierte Schreib-API |
| \`login.example.de\` | Weltweit, als Login-Portal | Authelia zur Anmeldung inklusive optional/verbindlich **TOTP-2FA** |

**Wichtig:** Die Buttons dürfen auf der öffentlichen Seite zu sehen sein, aber nur **auf die geschützte Steuerseite verlinken**. Ein Klick löst **noch keinen** Power-Befehl aus. Erst nach erfolgreicher Anmeldung, anschließender expliziter Bestätigung und serverseitiger Prüfung wird ein Befehl gesendet.

## Daten- und Befehlsfluss

~~~text
                     WELTWEITES INTERNET
                              |
                    HTTPS 443 / DNS / TLS
                              |
                     Caddy Reverse Proxy
                              |
         +--------------------+---------------------+
         |                    |                     |
   pc.example.de       steuerung.example.de    login.example.de
   (ohne Login)         (Authelia-geschuetzt)   (Authelia)
         |                    |                     |
   View-Dienst          forward_auth               |
   nur GET/Status  <-----kein direkter Schreibweg    |
         |                    |                     |
         |                 Control-API              |
         |                    |                     |
         +--------- interne Netzwerkdienste --------+
                              |
                    Home Assistant intern
                              |
                    ESPHome API verschluesselt
                              |
                       ESP32 -> Relais
~~~

**Wesentliche Sicherheitsgrenze:** Der Viewer-Backenddienst und der Control-Backenddienst müssen voneinander getrennte HTTP-Endpunkte / idealerweise getrennte Prozesse und Netzwerkrechte besitzen. Der öffentliche Viewer darf **gar keine Schreib-API** implementieren. Kein ESPHome- oder Home-Assistant-Token in HTML/JavaScript.

- **Statusweg:** internes HA/ESPHome -> kleiner Statusadapter mit Whitelist -> öffentliche Read-only-Antwort, z. B. \`GET /api/status\`; kein vollständiges HA-Attributobjekt nach außen. Idealerweise wird der Status über ein intern gepushtes Snapshot-/MQTT-Read-only-Thema (mit ACL) bezogen, damit der öffentlich erreichbare Viewer keinen mächtigen HA-Token benötigt.
- **Steuerweg:** angemeldeter Nutzer -> Caddy + Authelia (TOTP) -> Backend prüft User-/Rollenrecht, CSRF-Token, Origin und genaue **POST**-Befehle -> HA \`button.press\` oder \`number.set_value\` auf einer **festen Whitelist** bekannter ESPHome-Entitäten -> HA native ESPHome-Integration.
- **Nicht zulassen:** beliebige \`entity_id\`, beliebige \`service\`-Namen, beliebige URLs, allgemeines Durchreichen von HA-REST-API-Anfragen; keine geheimen HA-Bearer-Tokens im Browser.
- Der Backenddienst sollte nach bestätigtem Kommando sofort eine Auftrags-ID zurückgeben und später den PC-Hardwarestatus mit Timeout prüfen. Erfolgreicher HA-Service-Call ist nicht automatisch eine erfolgreiche PC-Zustandsänderung.
- **Wiederholungen und Doppelklicks:** Backend zusätzlich mit Rate-Limit/Idempotenz gegen wiederholte POSTs; ESPHome besitzt bereits gegenseitige Relais-Verriegelung und Schaltsperre.
- **Gefährliche Aktion:** „PC Not-Aus“ benötigt separate explizite Bestätigung und idealerweise erneute Bestätigung des Benutzers; eine erfolgreiche Session allein kann sonst mehrere Befehle zulassen. 2FA für den geschützten Bereich wird empfohlen.
- Der Kontrollserver soll jede akzeptierte/abgelehnte Aktion mit Zeit, Benutzer, Aktion, Ergebnis und Gerätenamen protokollieren; keine Passwörter/Tokens in Logs.

## Vorschlag für Proxmox

- Bestehenden Caddy Reverse Proxy weiterverwenden (im bisherigen Aufbau Caddy-CT **106**, Adresse **192.168.178.106**; vor Deployment tatsächlich prüfen).
- Home Assistant im bisherigen Aufbau VM **101**, Adresse **192.168.178.101**; vor Deployment tatsächlich prüfen.
- Ein eigenständiger, unprivilegierter Portal-LXC/VM als **separater View- und Control-Dienst** oder zwei isolierte Container. Nur benötigte TCP-Ports vom Caddy-CT aus erlauben; interner Status-/Steuerverkehr zu HA nach Firewall-Regeln.
- Zusätzliche Authelia-Instanz für Login, Benutzerverwaltung, TOTP und die \`forward_auth\`-Prüfung. Eigene gesicherte Secrets, Storage und Backups.
- WAN-Freigabe ausschließlich zu Caddy auf TCP **443** und bei konventioneller ACME-HTTP-Prüfung ggf. **80**. **Nie** TCP 8123/6053/ESPHome-Webport 80 oder Authelia-Backendport direkt ins Internet weiterleiten. Bei CGNAT/DS-Lite erst die tatsächliche Erreichbarkeit prüfen; alternativ eine bewusst abgesicherte Tunnel-/VPS-Topologie planen.
- DNS-Einträge für die drei Subdomains zeigen auf den **öffentlichen Caddy-Einstieg**; nicht auf HA oder ESP32.
- Caddy kann bei öffentlicher DNS-Auflösung und erreichbaren Validierungskanälen öffentliche TLS-Zertifikate automatisieren.

## Beispiel-Caddyfile – nur Struktur, NICHT unverändert produktiv verwenden

Die drei Beispiel-Domains und drei **ausschließlich interne** Upstream-Adressen müssen vor Benutzung ersetzt werden. Die Variablen \`{$PUBLIC_STATUS_UPSTREAM}\`, \`{$PRIVATE_CONTROL_UPSTREAM}\` und \`{$AUTHELIA_UPSTREAM}\` sind Caddy-Laufzeitumgebungsvariablen, z. B. \`192.168.178.X:8080\`, \`192.168.178.X:8081\`, \`192.168.178.Y:9091\` (keine unbestätigten realen IP-Adressen in das Caddyfile eintragen).

~~~caddyfile
# Öffentlicher Viewer: KEIN ESPHome/HA direkt als Upstream!
pc.example.de {
    encode zstd gzip
    header {
        X-Content-Type-Options "nosniff"
        X-Frame-Options "DENY"
        Referrer-Policy "no-referrer"
    }

    # POST/PUT/PATCH/DELETE nie an den rein lesenden Viewer durchlassen.
    @write method POST PUT PATCH DELETE
    respond @write "Method Not Allowed" 405
    reverse_proxy {$PUBLIC_STATUS_UPSTREAM}
}

# Die komplette Steueroberfläche und ihre API müssen authentifiziert sein.
steuerung.example.de {
    encode zstd gzip
    header {
        X-Content-Type-Options "nosniff"
        X-Frame-Options "DENY"
        Referrer-Policy "no-referrer"
        Cache-Control "no-store"
    }

    forward_auth {$AUTHELIA_UPSTREAM} {
        uri /api/authz/forward-auth
        copy_headers Remote-User Remote-Groups
    }

    # Private Control-API, nur via Caddy erreichbar.
    reverse_proxy {$PRIVATE_CONTROL_UPSTREAM}
}

# Login-Portal
login.example.de {
    reverse_proxy {$AUTHELIA_UPSTREAM}
}
~~~

**Caddy-Konfiguration allein ist keine vollständige Applikationsautorisierung.** Der private Backenddienst muss auch bei Browser-/Header-Manipulation eine explizite Benutzer-/Rollenprüfung und Schutz gegen CSRF durchführen. Ankommende \`Remote-User\`-/\`Remote-Groups\`-Header dürfen nicht ungeprüft als vertrauenswürdig behandelt werden. Backend direkt auf der internen Firewall nur für den Proxy erreichbar halten.

**Authelia-Autorisierungsprinzip** – keine vollständige lauffähige Authelia-Konfiguration:

~~~yaml
access_control:
  default_policy: deny
  rules:
    - domain: steuerung.example.de
      policy: two_factor
~~~

Zusätzlich müssen die Authelia-Session-Domain und \`authelia_url: https://login.example.de\` passend konfiguriert werden. Die vollständige Einrichtung inklusive Session-Signing-Key, Session-Storage, Passwort-Hasher, TOTP und ggf. SMTP muss vor Produktionsstart ausgearbeitet werden.

## Bedienverhalten

1. Anonymer Besucher sieht nur vom Nutzer erlaubte Statusdaten und einen Button **Steuerung öffnen**.
2. Klick auf **PC Ein/Aus**, **PC Not-Aus** oder **Zeiten ändern** verlinkt zu \`https://steuerung.example.de/\`; vor der Anmeldung wird kein Schaltbefehl gesendet.
3. Authelia prüft Benutzername, Passwort und TOTP. Nur zugelassene Benutzer gelangen zur Steuerseite.
4. Jede ausgeführte Aktion ist ein serverseitig autorisierter **POST** mit CSRF-Schutz. Beim Not-Aus erscheint eine deutliche Bestätigung.
5. Das Backend kontrolliert Befehls-/Entitäten-Whitelist, Parametergrenzen, Schaltsperre und Sensordaten (ESPHome übernimmt seine eigene Kontrolle nochmals).
6. Andere Besucher sehen weiterhin nur die öffentliche, lesende Anzeige.

**Option „bei jedem Schaltbefehl Passwort erneut eingeben“:** Das ist eine zusätzliche Anforderung an die Control-App (Step-up-Reauthentifizierung). Eine normale Authentifizierung am Caddy-/Authelia-Portal schützt eine **Sitzung**, sie fordert das Passwort nicht zwangsläufig bei jedem Klick neu an. Falls pro Aktion neue Anmeldung gewünscht wird, muss dies explizit implementiert und getestet werden.

## Sicherheits- und Abnahmetests

- [ ] Anonymer GET auf Statusseite liefert ausschließlich whitelisted PC-Statusdaten.
- [ ] Statusseite enthält keine Geheimnisse, keine Steuerbefehlsendpunkte und keine schreibenden HA-/ESPHome-Verbindungen.
- [ ] Anonymer Zugriff auf Steuerungs-Host und Steuer-API wird blockiert.
- [ ] Fehlende/ungültige TOTP-Anmeldung blockiert sämtliche Steuerbefehle und Zeitänderungen.
- [ ] Direkter Netzwerkzugriff auf den Control-Backendport nur vom Caddy-Proxy möglich.
- [ ] HA-Port 8123, ESPHome native API, ESPHome Web V3 und OTA bleiben ausschließlich intern.
- [ ] Browser/CSRF-Tests: fremde Origin, fehlende CSRF-Token, GET statt POST, unzulässige Methoden werden verworfen.
- [ ] Manipulierte \`entity_id\`/\`service\`-Werte, ungültige Dauer und negative Werte werden vom Backend abgewiesen.
- [ ] Parallele Anfragen, Mehrfachklicks und Not-Aus während eines Normalimpulses erzeugen keinen doppelten Kontaktschluss.
- [ ] Wechsel der Impulszeiten nur nach Anmeldung, nur innerhalb 0,2–1,5 / 4–12 / Sperre 0,5–10 s.
- [ ] Not-Aus benötigt bewusste Bestätigung; Test ohne Relais bleibt unabhängig.
- [ ] Fehlgeschlagene HA-Verbindung wird im Web als Fehler/Unbekannt angezeigt, nicht als „PC AUS“.
- [ ] Caddy-TLS-Zertifikat valide, HTTP wird auf HTTPS umgeleitet; schreibende Aktionen benutzen ausschließlich HTTPS.
- [ ] Rate-Limits, Login-Sperren/Abwehr, Audit-Logs, Backups/Restore der Authelia-Konfiguration getestet.
- [ ] Test aus externem Netz (Mobilfunk): öffentliches Lesen möglich, anonyme Steuerung unmöglich.
- [ ] Keine Portfreigaben direkt zu Home Assistant, ESP32 oder Auth-Backend.

## Noch offene, für das Deployment erforderliche Angaben

1. Gewünschte **öffentliche Domain/Subdomain** (echten Wert vor dem Eintrag in Caddy klären).
2. Caddy-Installationsart (Docker Compose im CT, Caddyfile-Pfad, aktuell verwendete Hostnames/Ports) verifizieren.
3. WAN-Erreichbarkeit, DNS und ob bereits Caddy-HTTPS/Portweiterleitungen existieren.
4. Soll für **jede einzelne** Power-Aktion das Passwort nochmals abgefragt werden oder genügt eine zeitlich begrenzte Login-Session mit zusätzlicher Not-Aus-Bestätigung?
5. Sollen öffentliche Besucher **nur den PC-Zustand** oder auch WLAN-Qualität, Uptime, Schaltprotokoll und weitere Signale sehen? Weniger ist sicherer.

## Aktuelle offizielle Referenzen

- Caddy \`forward_auth\`: https://caddyserver.com/docs/caddyfile/directives/forward_auth
- Caddy \`basic_auth\`: https://caddyserver.com/docs/caddyfile/directives/basic_auth
- Caddy automatische Zertifikate: https://caddyserver.com/docs/automatic-https
- Authelia Caddy Integration: https://www.authelia.com/integration/proxies/caddy/
- Authelia Access Control (two_factor): https://www.authelia.com/configuration/security/access-control/
- Home Assistant API: https://developers.home-assistant.io/docs/api/rest/
- ESPHome Web Server: https://esphome.io/components/web_server/
