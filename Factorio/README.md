# BBQ CHAOS – Factorio 2.0.77 · Bob's + Angel's + Space Age

**Leitlinie seit 03.10.2026:** ausschliesslich **unveraenderte Original-ZIP-Dateien aus dem offiziellen [Factorio-Modportal](https://mods.factorio.com/)**. Keine selbst erstellten BBQ-Reparaturmods, Lua-Eingriffe, gepatchten ZIPs, alternativen Downloadquellen oder grossen zusaetzlichen Planetensammlungen.

## Modpack-Zusammensetzung

- [mod-list.json](mod-list.json): **96 Eintraege – alle aktiviert**. Originalauswahl einschliesslich Bob's, Angel's und Space Age sowie bereits gewuenschter QoL-Mods, nicht pauschal um neue Overhauls erweitert.
- [MODS.md](MODS.md): alle Mods, Stati und Modportal-Links.
- [version-pins.json](version-pins.json): **sechs** festgelegte QoL-Modversionen; keine Version fuer deaktivierte Mods.
- [BBQ-UPDATE.cmd](BBQ-UPDATE.cmd): **einzige notwendige Windows-Startdatei**.
- [BBQ-GitHub-Launcher-v2.ps1](BBQ-GitHub-Launcher-v2.ps1): laedt Liste, Pins und den auf SHA-256 geprueften Downloader-Core; **keine ZIP-Nachbearbeitung**.
- [BBQ-ModDownloader-core.ps1.gz.b64](BBQ-ModDownloader-core.ps1.gz.b64): offizieller Portal-Downloader aus dem bisherigen Paket (verifiziert via SHA-256 `c029f896eb20fb81023e32266ebda0689d2b3c96004eb9cd300b43811e7b0405`).
- [BBQ-STARTEN.cmd](BBQ-STARTEN.cmd), [BBQ-NUR-PRUEFEN.cmd](BBQ-NUR-PRUEFEN.cmd), [BBQ-GitHub-Launcher.ps1](BBQ-GitHub-Launcher.ps1): alte Skripte, nicht fuer das neue Setup verwenden.

## Warum bestimmte Mods nicht mehr enthalten sind

- `angelsaddons-space-age`: Original-Kompatibilitaetsmod verursacht ungueltigen Schwefelsaeure-Referenznamen bei Quality/Battery-Recycling.
- `angelsaddons-space-age-revived` **0.0.14**: Offizielle Fork-ZIP behebt obigen Fehler, verwendet aber `bob-turbo-transport-belt`, das in Bob's Logistics 2.1.1 nicht mehr existiert. Fehler unter anderem im `turbo-transport-belt`- und `turbo-loader`-Rezept. **Unmodifiziert verwenden wir diese Fork nicht**.
- `angels-space-age-tungsten-compat`: benoetigt die deaktivierte Originalmod.
- `saplib`: Konflikt mit `cargo-ships` in `resource-autoplace.lua`, reproduziert; `cargo-ships` und `cargo-ships-graphics` bleiben aktiv. `TurboBelt`, das `saplib` und die Abwesenheit von Space Age verlangt, bleibt deaktiviert.
- 14 bisherige deaktivierte Mods sind vollstaendig aus der Liste entfernt, zusaetzlich **AutoDeconstruct** als freiwillige QoL-Mod nach fehlgeschlagener ZIP-Validierung. Eine defekte Modportal-Veroeffentlichung ist damit **nicht** nachgewiesen.

Die neue **AngelBob Space Age Rebalance** ist im Modportal verfuegbar, verlangt aber viele zusaetzliche grosse Planeten-/Grafikmods. Sie wurde **bewusst nicht ungefragt** in das Modpack aufgenommen. Ohne spezifische Space-Age-Integration sind **Spielstart und komplette Progression auf allen Planeten nicht garantiert**. Modportal-Verfuegbarkeit ist keine Garantie fuer gemeinsame Kompatibilitaet.

`AngelBob` ist ein **offizieller Modportal-Metapack-Mod**, der laut Autor selbst nichts am Spiel aendert, sondern Angel/Bob-Abhaengigkeiten buendelt. `early_construction_modified_private` traegt zwar „private“ im Namen, ist aber unter diesem **exakten ID-Namen im offiziellen Modportal** als „Early Construction“ in Version 2.0.1 veroeffentlicht. Nur unveraenderte Originaldateien zulassen.

## Umfang nach Bereinigung

Die Liste enthaelt jetzt **96 aktive** Eintraege (4 Factorio-Grundspiel- und DLC-Mods
plus **92 Original-ZIPs aus dem offiziellen Modportal**). Es gibt keine
deaktivierten Eintraege mehr; somit zeigt `BBQ-UPDATE.cmd` 96/96 an.

`AutoDeconstruct` ist eine freiwillige Komfortfunktion (erschoepfte Bohrer zur
Demontage markieren) und fuer die beabsichtigte Bob's-/Angel's-Auswahl nicht
als Kernmod eingeplant. Die AMP-Pruefung ihrer ZIP-Datei
`AutoDeconstruct_1.0.14.zip` scheiterte; wir verzichten auf die Mod, ohne ihre
Original-ZIP zu patchen oder die SHA1-Kontrolle abzuschalten.

Die bereits gewaehlten uebrigen QoL-Mods sind weiterhin in der Liste; sie
werden nicht ungefragt geloescht. Der Server-Downloader aktualisiert die
aktive AMP-Instanz erst mit `--apply`; ein fehlgeschlagener vorheriger Versuch
hat die alte Instanz nicht ersetzt.

## Windows: Neuinstallation / Update

1. **Factorio schliessen**. `%APPDATA%\Factorio\saves`, `mod-settings.dat`, `mod-list.json` und gegebenenfalls eigene Backups sichern.
2. Bereits manuell modifizierte `angelsaddons-space-age-revived_0.0.14.zip`, `bbq-chaos-belt-compat_*.zip` sowie sonstige selbstveraenderte ZIPs aus dem **aktiven** `%APPDATA%\Factorio\mods`-Verzeichnis herausnehmen (nicht blind alles loeschen); die GitHub-Modliste enthaelt den Fork nicht mehr.
3. Falls eine fruehere `BBQ-UPDATE.cmd` den v2-Launcher verwendet, ist **kein Austausch** notwendig: sie laedt bei jedem Start die aktualisierte Launcher-Datei von GitHub. Andernfalls die Datei aus diesem Ordner einmal neu beziehen.
4. `BBQ-UPDATE.cmd` doppelklicken. Der Launcher **archiviert** vor dem Download noch vorhandene `bbq-chaos-belt-compat_*`-Testmods und `angelsaddons-space-age-revived_0.0.14.zip` unter `%APPDATA%\Factorio\BBQ-CHAOS-Backups\obsolete-*`, statt ZIPs zu veraendern oder zu loeschen. Anschliessend werden aktive Mods ausschliesslich **original** aus dem offiziellen Modportal geladen und auf Portalpruefsummen geprueft. Beim Fehlschlag ist der Bericht unter `%APPDATA%\Factorio\BBQ-CHAOS-Reports` massgeblich.
5. `BBQ-UPDATE.cmd pruefen` fuer reine Pruefung, `BBQ-UPDATE.cmd test` fuer den Parser-Test. **Keine veraenderten Original-ZIPs wieder in das Verzeichnis legen.**
6. Factorio auf einem **neuen Test-Spielstand** starten und Planeten-/Produktionsketten testen, bevor derselbe Satz im Multiplayer verwendet wird.

Das **Serververzeichnis von AMP wird vom Windows-Updater nicht veraendert**. Die AMP-Instanz war zwischenzeitlich mit einer **lokal gepatchten** Angel-Fork startfaehig und laeuft damit **nicht** mit den unveraenderten Windows-Mods synchron. Vor dem naechsten Multiplayerstart **Server herunterfahren, Konfiguration und Save sichern**, alte BBQ-Testfix-ZIPs entfernen, **Originaldateien aus dem offiziellen Modportal** verwenden und die `mod-list.json` auf den neuen Stand bringen. Ein neues Spiel ist bei Aenderungen an Angel/Bob/Space-Age-Rezepten unter Umstaenden erforderlich. **Diese README ist keine Aussage, dass AMP bereits umgestellt ist.**

## AMP-Server direkt aus dem Factorio-Modportal synchronisieren

Die Windows-CMD laedt **nicht** in die Factorio01-AMP-Instanz. Fuer AMP steht jetzt
[BBQ-AMP-Portal-Sync.py](BBQ-AMP-Portal-Sync.py) bereit, eine reine
**Administrationshilfe** (keine Factorio-Mod). Die GitHub-Liste und sechs Pins
werden direkt vom Repository gelesen; ZIPs kommen nur von **mods.factorio.com**,
sind unveraendert und muessen zu den vom Modportal angegebenen SHA1-Pruefsummen
und Original-\`info.json\` passen. Es werden keine externen ZIPs und keine
Lua-Reparaturen hinzugefuegt.

1. Factorio01 **in AMP stoppen**. **Savegames separat sichern.**
2. Das GitHub-Python-Skript per \`curl\` unter \`/tmp\` speichern.
3. \`sudo python3 /tmp/BBQ-AMP-Portal-Sync.py --check\`: 2.0-Verfuegbarkeit, Pins
   und verpflichtende Mod-Abhaengigkeiten **ohne Aenderung** vorpruefen.
4. \`sudo python3 /tmp/BBQ-AMP-Portal-Sync.py --apply\`: verwendet falls vorhanden
   lokale \`player-data.json\`-Daten \`service-username\` und \`service-token\`;
   sonst werden Zugangsdaten **nur im SSH-Terminal** erfragt (Token unsichtbar).
   **Nie** Zugangsdaten an Chat/GitHub schicken.
5. Script laedt alle ZIPs zuerst in ein staging-Verzeichnis; originale bestehende
   ZIPs duerfen nur bei **korrekt verifizierter SHA1-Pruefsumme** kopiert werden.
   \`mod-settings.dat\` wird erhalten, \`mod-list.json\` von GitHub uebernommen.
   Erst wenn alle Downloads erfolgreich sind, wird das alte AMP-Verzeichnis
   nach \`mods-BBQ-backup-<Datum>\` verschoben, das neue aktiv geschaltet.
   Fehler vor dem Swap lassen die aktiven Mods unveraendert.
6. Danach Factorio01 in AMP starten und \`factorio-current.log\` pruefen.

Dieses Skript **loescht keine Welt** und veraendert keine Mod-Inhalte.
Ein alter Spielstand kann aber noch von den deaktivierten Angel-Patches
abhaengen. Fuer eine andere Rezept-/Prototypkombination ist ein **neuer
Spielstand** als erster Test empfehlenswert. Ein erfolgreicher Download
garantiert nicht, dass das gesamte Bob/Angel/Space-Age-Modpack spielbar
ist. Kein vorheriges serverseitiges \`--create\` mit geaenderten Mods
erzwingen, bevor die neue Liste fehlerfrei geladen wird.

## Sicherheit / GitHub-Checks

- Keine Accounts, privaten Dateien, Tokens, Passwoerter, `mod-settings.dat` oder Savegames auf GitHub hochladen.
- Die aktuelle CMD laedt genau festgelegte Dateien aus `Technox90/projekte`; SHA-256 verifiziert den Downloader-Core. Downloads aus dem Modportal muessen zu den vom Portal veroeffentlichten ZIP-Hashes passen.
- GitHub Actions prueft JSON, Pins, bekannte Inkompatibilitaeten und PowerShell-Syntax; sie kann **keinen vollstaendigen Spieltest oder die Funktion aller Rezepte beweisen**.
- Downloaderfehler nicht durch das Ausschalten von `quality`, `space-age` oder grosser Bob-/Angel-Kernbereiche umgehen.
