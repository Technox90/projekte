# ESP32-Weboberfläche mit Passwort und einstellbaren Impulszeiten

Die PC-Steuerung kann über **Home Assistant** und über die **lokale ESPHome-Weboberfläche V3** bedient werden. Die Weboberfläche nutzt dieselben Firmware-Buttons, Sensoren und gespeicherten Zeitregler wie Home Assistant.

## 1. Neue Secrets im ESPHome Device Builder

Im ESPHome Device Builder im Bereich **Secrets** zusätzlich zu den bisherigen Zugangsdaten zwei **eigene** Werte anlegen:

~~~yaml
# NUR BEISPIELE – eigene Werte lokal eintragen, nie auf GitHub.
pc_web_username: "DEIN_WEB_BENUTZER"
pc_web_password: "DEIN_LANGES_ZUFAELLIGES_PASSWORT"
~~~

Die offizielle [Secrets-Vorlage](../firmware/secrets.example.yaml) enthält ausdrücklich keine echten Zugangsdaten.

In der [Firmware](../firmware/pc-steuerung.yaml) ist der folgende Webserver konfiguriert:

~~~yaml
web_server:
  port: 80
  version: 3
  local: true
  include_internal: false
  ota: false
  auth:
    type: digest
    username: !secret pc_web_username
    password: !secret pc_web_password
~~~

Die lokale Oberfläche lässt sich auch ohne externe CSS- oder JavaScript-Server anzeigen (\`local: true\`), verbraucht dafür etwas mehr Speicher auf dem ESP32.

**Sicherheit:** Der Zugriff erfolgt über **HTTP** mit **Digest-Authentifizierung**, nicht HTTPS. Digest verhindert, dass das Passwort bei jedem Request als einfaches Base64 übertragen wird, verschlüsselt aber nicht die gesamte Kommunikation. **Keine Portfreigabe ins Internet, keine öffentliche Weiterleitung oder Reverse-Proxy-Freigabe**; nur im vertrauenswürdigen LAN. Web-OTA ist deaktiviert. Firmware-Updates laufen weiterhin mit dem ESPHome-OTA-Passwort.

## 2. Adresse und Anmeldung

Nachdem die Firmware über USB bzw. ESPHome OTA installiert ist und der ESP32 eine lokale IP-Adresse erhalten hat, folgende Adresse im Browser im selben Netz öffnen:

- \`http://pc-steuerung.local/\` – wenn mDNS funktioniert.
- \`http://<LOKALE-ESP32-IP>/\` – zuverlässiger, wenn \`.local\` nicht aufgelöst wird.

Bei mehreren Controllern heißen die Hostnamen z. B. \`pc-steuerung-01.local\` und \`pc-steuerung-02.local\`; jeder benötigt **eigene**, lokale Web-Zugangsdaten.

Bei der Passwortabfrage die selbst gewählten Werte für \`pc_web_username\` und \`pc_web_password\` eingeben.

## 3. Die beiden Hauptbuttons

| Button | Wirkung |
|---|---|
| **PC Ein/Aus** | Ein normaler Power-Tasterdruck am Mainboard-PWR_SW, Dauer über den Regler einstellbar. |
| **PC Not-Aus (Ausschalten erzwingen)** | Ein langer Power-Tasterdruck am Mainboard-PWR_SW, nur wenn die geprüfte geschaltete 12-V-Hauptversorgung **ODER** eine mindestens zwei Sekunden stabile Mainboard-Power-LED den PC als EIN melden. |

Die normale Power-Taste ist **ein Toggle-Impuls**, kein sicher getrenntes „Nur einschalten“ beziehungsweise „Nur ausschalten“. Ob er den PC startet, herunterfährt, in Standby versetzt oder nichts tut, hängt vom jeweiligen PC und Betriebssystem ab.

Ein erzwungenes Ausschalten über langen Tastendruck ist **KEIN normgerechter Not-Aus**, kann ungespeicherte Daten zerstören und bei bestimmten PCs nach langem Gedrückthalten ein ungewolltes Wiederanlaufen bewirken. Die Freigabe über PLED muss unter EIN/AUS/S3 geprüft werden. Bei Unsicherheit nicht verwenden.

**Weitere Diagnosefunktionen:** \`Not-Aus LED Test (ohne Relais)\` bleibt erhalten, ist aber kein dritter Power-Button: Er lässt nur GPIO2 sechs Sekunden lang blinken. Außerdem gibt es den ESP32-Neustart.

## 4. Zwei gespeicherte Zeitregler

| Regler | Einstellbereich | Schritt | Standard |
|---|---|---|---|
| **PC Ein/Aus Impulsdauer** | 0,2–1,5 Sekunden | 0,1 s | **0,5 s** |
| **PC Not-Aus Impulsdauer** | 4–12 Sekunden | 0,5 s | **10 s** |

Beide Regler erscheinen als **Number-Entitäten** in Home Assistant **und** in der ESP32-Weboberfläche. Nach einem Wechsel wirkt die neue Zeit beim **nächsten** Power-Befehl. Die Werte werden auf dem ESP32 gespeichert (\`restore_value: true\`) und überstehen einen Neustart.

Der jeweilige Wert wird beim Impulsstart einmalig durch eine dynamische ESPHome-\`delay: !lambda\` (Millisekunden) übernommen. Ungültige/noch nicht initialisierte Werte fallen auf **0,5 s** bzw. **10 s** zurück. Änderungen während eines laufenden Kontaktschlusses ändern die bereits gestartete Laufzeit nicht.

Die **10 Sekunden weiche LED-Pulsanimation** nach einem normalen Power-Befehl und der **6 Sekunden lange LED-Test ohne Relais** bleiben von den beiden Power-Zeitreglern unabhängig.

## 5. Installation und erste Prüfung

1. Die zwei neuen Secrets **nur lokal** eintragen.
2. [Firmware](../firmware/pc-steuerung.yaml) im ESPHome Device Builder aktualisieren.
3. **Konfiguration überprüfen** und Firmware kompilieren.
4. ESP32 über USB oder ESPHome OTA neu flashen und mit Home Assistant verbinden.
5. Im Browser die lokale ESP32-IP öffnen und Benutzername/Passwort prüfen. Ohne Anmeldung darf die Oberfläche nicht bedienbar sein.
6. **Relaiskontakte COM/NO vom Mainboard trennen.** Mit Messgerät die Impulsdauer bei minimalem, normalem und maximalem Wert prüfen.
7. Zeitregler über die Weboberfläche ändern und in Home Assistant den gleichen Wert kontrollieren; anschließend umgekehrt.
8. Neustarten, gespeicherte Zeitwerte prüfen; während eines laufenden Impulses Regler nicht zur Nachsteuerung verwenden.
9. Die beiden PC817-Eingänge auf **richtiges PC AUS/EIN/Standby** validieren, bevor der lange Power-Button an das Mainboard angeschlossen wird.
10. Testen, ob die beiden GPIO2-LED-Animationen sowie das normale WLAN-Blinken unverändert funktionieren.

## 6. Fehlersuche

- **Kein Webzugriff:** Direkte IP aus dem Router oder ESPHome-Log verwenden; \`.local\` benötigt funktionierendes mDNS. Port 80 und lokale Netzsegmentierung prüfen.
- **Passwort falsch:** Die in der lokal gespeicherten ESPHome-Secrets-Datei gesetzten Werte gelten nach dem erneuten Flash. Einen anderen Browser bzw. ein neues privates Fenster zum erneuten Digest-Login testen.
- **Regler fehlen:** Firmware mit den neuen \`number.template\`-Einträgen muss tatsächlich installiert sein. HA-Gerät und Browser aktualisieren.
- **Website erscheint unformatiert:** \`local: true\` beibehalten, Firmware-Kompilierung und verfügbaren Flash prüfen.
- **Not-Aus abgelehnt:** Ursprungswerte \`PC 12V Versorgung\` und \`PC Power LED stabil\` prüfen. **Nicht** einfach die Freigabe entfernen.
- **Langer Tastendruck führt zu erneutem Start:** Die eingestellte lange Dauer ist möglicherweise zu groß für das konkrete Mainboard. Verhalten kontrolliert untersuchen, bevor das Relais erneut angeschlossen wird.

## Quellen

- https://esphome.io/components/web_server/
- https://esphome.io/components/number/template/
- https://esphome.io/automations/actions/

**Hinweis:** Die Firmwaredatei wurde erstellt, aber ein vollständiger ESPHome-Build und elektrische Hardwaretests sind vor produktiver Nutzung weiterhin erforderlich.
