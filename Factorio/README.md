# 🔥 BBQ CHAOS – Factorio 2.0.77 Modpack

Verwaltete Modliste für **Bob's + Angel's + Space Age** mit QoL-Erweiterungen. Ziel: dieselbe Auswahl auf Windows-PC und AMP-Multiplayer-Server; zunächst lokal testen.

## Dateien

- [MODS.md](MODS.md) – komplette Liste aller **110 Mods**, mit Status, Zweck und Modportal-Links.
- [mod-list.json](mod-list.json) – **maßgebliche zentrale Modauswahl**: 100 aktiviert, 10 deaktiviert.
- [version-pins.json](version-pins.json) – **sechs** festgelegte QoL-Versionen (Factorio 2.0).
- **[BBQ-UPDATE.cmd](BBQ-UPDATE.cmd) – EINZIGE auf Windows benötigte Datei.** Direkt von GitHub herunterladen, doppelklicken; lädt bei jedem Lauf die aktuelle GitHub-Modliste und den geprüften Core.
- [BBQ-STARTEN.cmd](BBQ-STARTEN.cmd) – älterer Start für vollständig entpackte Ordner (nur aus Kompatibilitätsgründen behalten).
- [BBQ-NUR-PRUEFEN.cmd](BBQ-NUR-PRUEFEN.cmd) – älterer Prüflauf; bei der Ein-Datei-Lösung stattdessen `BBQ-UPDATE.cmd pruefen` verwenden.
- [BBQ-GitHub-Launcher.ps1](BBQ-GitHub-Launcher.ps1) – GitHub-Downloader/Startlogik.
- [BBQ-ModDownloader-core.ps1.gz.b64](BBQ-ModDownloader-core.ps1.gz.b64) – komprimierte, unveränderte Downloader-Core-Version aus dem zuletzt funktionierenden Paket (im Launcher SHA-256-geprüft).

## Installation auf Windows – nur eine CMD-Datei

1. **Factorio schließen**.
2. Nur die Datei **[BBQ-UPDATE.cmd](BBQ-UPDATE.cmd)** herunterladen (auf GitHub `Raw` / `Download raw file`) und eine **bereits vorhandene ältere BBQ-UPDATE.cmd einmalig ersetzen**. **Keine weiteren lokalen Skriptdateien erforderlich.**
3. `BBQ-UPDATE.cmd` doppelklicken: die CMD lädt den Launcher v2 mit Cache-Bypass und Syntaxkontrolle. Aktuelle Modliste, sechs Versionsvorgaben und Downloader-Core kommen bei jedem Lauf von `Technox90/projekte/main/Factorio` über HTTPS.
4. Der Core ist mit einer fest hinterlegten SHA-256-Prüfsumme geschützt; bei GitHub-/Integritätsfehlern Abbruch **ohne stillen Rückgriff auf einen alten Stand**.
5. Zum reinen Prüfen: `BBQ-UPDATE.cmd pruefen` über Eingabeaufforderung; Parser-Test: `BBQ-UPDATE.cmd test`.
6. Factorio.com-Benutzername/Token bleiben lokal; Mods werden vom offiziellen Factorio-Modportal geladen. Bei ungeklärten Abhängigkeiten können gültige ZIPs geladen werden, die Modliste wird aber nicht als vollständiger Satz aktiviert.
7. Neue Welt testen, erst danach auf AMP übernehmen.

**Wichtig:** Nur **eine CMD-Datei lokal** heißt nicht, dass der Code nur aus einer Datei besteht: Launcher, Modliste, Versionsvorgaben und geprüfter Core bleiben auf GitHub und werden automatisch heruntergeladen. `BBQ-STARTEN.cmd` und `BBQ-NUR-PRUEFEN.cmd` werden nicht mehr benötigt.

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
