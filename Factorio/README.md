# 🔥 BBQ CHAOS – Factorio 2.0.77 Modpack

Verwaltete Modliste für **Bob's + Angel's + Space Age** mit QoL-Erweiterungen. Ziel: dieselbe Auswahl auf Windows-PC und AMP-Multiplayer-Server; zunächst lokal testen.

## Dateien

- [MODS.md](MODS.md) – komplette Liste aller **110 Mods**, mit Status, Zweck und Modportal-Links.
- [mod-list.json](mod-list.json) – **maßgebliche zentrale Modauswahl**: 100 aktiviert, 10 deaktiviert.
- [version-pins.json](version-pins.json) – **sechs** festgelegte QoL-Versionen (Factorio 2.0).
- [BBQ-STARTEN.cmd](BBQ-STARTEN.cmd) – Windows-Start (lädt **GitHub-Liste bei jedem Lauf**).
- [BBQ-NUR-PRUEFEN.cmd](BBQ-NUR-PRUEFEN.cmd) – nur Abhängigkeiten prüfen, nichts installieren.
- [BBQ-GitHub-Launcher.ps1](BBQ-GitHub-Launcher.ps1) – GitHub-Downloader/Startlogik.
- [BBQ-ModDownloader-core.ps1.gz.b64](BBQ-ModDownloader-core.ps1.gz.b64) – komprimierte, unveränderte Downloader-Core-Version aus dem zuletzt funktionierenden Paket (im Launcher SHA-256-geprüft).

## Installation auf Windows

1. **Factorio schließen**.
2. Im GitHub-Repo oben **Code → Download ZIP** wählen und entpacken, oder nur den Unterordner `Factorio` lokal bereitstellen.
3. `BBQ-STARTEN.cmd` doppelklicken. Die aktuelle Liste, Versionsvorgaben und der Downloader-Core kommen **jedes Mal von GitHub**.
4. Das Programm lädt ZIP-Mods **nur vom offiziellen Factorio-Modportal**. Benötigt wird ein gültiger Factorio.com-Benutzername/Token; bestehende Anmeldedaten aus `%APPDATA%\Factorio\player-data.json` können lokal verwendet werden.
5. Im Programm die Installation bestätigen. Bei ungelösten Abhängigkeiten lädt es verfügbare Mods herunter, **aktiviert aber kein unvollständiges Set**.
6. Neue Welt testen; erst danach auf AMP übernehmen.

**Direkte Downloadquelle für die Liste:**
`https://raw.githubusercontent.com/Technox90/projekte/main/Factorio/mod-list.json`

## Änderungen

Für neue Mods ausschließlich die zentrale `mod-list.json` bearbeiten. Achtung: Der Factorio-Modmanager kann beim Spielen die *lokale* Liste ändern, diese Änderungen werden **nicht** automatisch nach GitHub übertragen. Beim nächsten Downloaderlauf hat wieder die GitHub-Liste Vorrang. Im Zweifelsfall zunächst `BBQ-NUR-PRUEFEN.cmd` ausführen.

## Sicherheit und bekannte Grenzen

- Niemals Account-Passwörter, API-Token, `player-data.json`, `mod-settings.dat` oder private ZIPs nach GitHub hochladen. Dieses Repository ist **öffentlich**.
- `early_construction_modified_private` muss bei fehlender Modportal-Veröffentlichung manuell besorgt werden. Der Downloader erhält dabei die bisherige Konfiguration und erzeugt einen Bericht.
- Eine veröffentlichte 2.0-Version bedeutet **nicht** automatisch, dass alle 100 aktiven Mods gemeinsam unter 2.0.77 laden. Vor Servereinsatz einschließlich aller Space-Age-Planeten prüfen.
- Der Launcher lädt **fest benannte Dateien ausschließlich aus diesem Repository**, mit SHA-256-Prüfung für den Downloader-Core. Kein stiller Fallback auf veraltete lokale Listen.
- Ein AMP-/Linux-Installationsprozess ist nicht Bestandteil dieses Windows-Downloaders.

## Modpacksprache und ursprüngliche Zusammenstellung

BBQ CHAOS · Deutsch · neue Spielwelt möglich · Space Age aktiv. Konflikt-/Experimentmods bleiben deaktiviert; sechs angeforderte QoL-Mods sind hinzugefügt und fixiert. Der Stand ist ein *Testkandidat*, keine getestete Veröffentlichung.
