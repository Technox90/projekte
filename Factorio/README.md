# BBQ CHAOS – Factorio 2.0.77 · Bob's + Angel's + Space Age

**Leitlinie seit 03.10.2026:** ausschliesslich **unveraenderte Original-ZIP-Dateien aus dem offiziellen [Factorio-Modportal](https://mods.factorio.com/)**. Keine selbst erstellten BBQ-Reparaturmods, Lua-Eingriffe, gepatchten ZIPs, alternativen Downloadquellen oder grossen zusaetzlichen Planetensammlungen.

## Modpack-Zusammensetzung

- [mod-list.json](mod-list.json): **111 Eintraege – 97 aktiviert, 14 deaktiviert**. Originalauswahl einschliesslich Bob's, Angel's und Space Age sowie bereits gewuenschter QoL-Mods, nicht pauschal um neue Overhauls erweitert.
- [MODS.md](MODS.md): alle Mods, Stati und Modportal-Links.
- [version-pins.json](version-pins.json): **sechs** festgelegte QoL-Modversionen; keine Version fuer deaktivierte Mods.
- [BBQ-UPDATE.cmd](BBQ-UPDATE.cmd): **einzige notwendige Windows-Startdatei**.
- [BBQ-GitHub-Launcher-v2.ps1](BBQ-GitHub-Launcher-v2.ps1): laedt Liste, Pins und den auf SHA-256 geprueften Downloader-Core; **keine ZIP-Nachbearbeitung**.
- [BBQ-ModDownloader-core.ps1.gz.b64](BBQ-ModDownloader-core.ps1.gz.b64): offizieller Portal-Downloader aus dem bisherigen Paket (verifiziert via SHA-256 `c029f896eb20fb81023e32266ebda0689d2b3c96004eb9cd300b43811e7b0405`).
- [BBQ-STARTEN.cmd](BBQ-STARTEN.cmd), [BBQ-NUR-PRUEFEN.cmd](BBQ-NUR-PRUEFEN.cmd), [BBQ-GitHub-Launcher.ps1](BBQ-GitHub-Launcher.ps1): alte Skripte, nicht fuer das neue Setup verwenden.

## Was wurde deaktiviert, und warum?

- `angelsaddons-space-age`: Original-Kompatibilitaetsmod verursacht ungueltigen Schwefelsaeure-Referenznamen bei Quality/Battery-Recycling.
- `angelsaddons-space-age-revived` **0.0.14**: Offizielle Fork-ZIP behebt obigen Fehler, verwendet aber `bob-turbo-transport-belt`, das in Bob's Logistics 2.1.1 nicht mehr existiert. Fehler unter anderem im `turbo-transport-belt`- und `turbo-loader`-Rezept. **Unmodifiziert verwenden wir diese Fork nicht**.
- `angels-space-age-tungsten-compat`: benoetigt die deaktivierte Originalmod.
- `saplib`: Konflikt mit `cargo-ships` in `resource-autoplace.lua`, reproduziert; `cargo-ships` und `cargo-ships-graphics` bleiben aktiv. `TurboBelt`, das `saplib` und die Abwesenheit von Space Age verlangt, bleibt deaktiviert.
- Weitere bereits zuvor deaktivierte Mods bleiben deaktiviert, siehe Katalog.

Die neue **AngelBob Space Age Rebalance** ist im Modportal verfuegbar, verlangt aber viele zusaetzliche grosse Planeten-/Grafikmods. Sie wurde **bewusst nicht ungefragt** in das Modpack aufgenommen. Ohne spezifische Space-Age-Integration sind **Spielstart und komplette Progression auf allen Planeten nicht garantiert**. Modportal-Verfuegbarkeit ist keine Garantie fuer gemeinsame Kompatibilitaet.

`AngelBob` ist ein **offizieller Modportal-Metapack-Mod**, der laut Autor selbst nichts am Spiel aendert, sondern Angel/Bob-Abhaengigkeiten buendelt. `early_construction_modified_private` traegt zwar „private“ im Namen, ist aber unter diesem **exakten ID-Namen im offiziellen Modportal** als „Early Construction“ in Version 2.0.1 veroeffentlicht. Nur unveraenderte Originaldateien zulassen.

## Windows: Neuinstallation / Update

1. **Factorio schliessen**. `%APPDATA%\Factorio\saves`, `mod-settings.dat`, `mod-list.json` und gegebenenfalls eigene Backups sichern.
2. Bereits manuell modifizierte `angelsaddons-space-age-revived_0.0.14.zip`, `bbq-chaos-belt-compat_*.zip` sowie sonstige selbstveraenderte ZIPs aus dem **aktiven** `%APPDATA%\Factorio\mods`-Verzeichnis herausnehmen (nicht blind alles loeschen); die lokale Modliste deaktiviert den Fork ohnehin.
3. Falls eine fruehere `BBQ-UPDATE.cmd` den v2-Launcher verwendet, ist **kein Austausch** notwendig: sie laedt bei jedem Start die aktualisierte Launcher-Datei von GitHub. Andernfalls die Datei aus diesem Ordner einmal neu beziehen.
4. `BBQ-UPDATE.cmd` doppelklicken. Aktive Mods werden ausschliesslich **original** aus dem offiziellen Modportal geladen und auf Portalpruefsummen geprueft. Beim Fehlschlag ist der Bericht unter `%APPDATA%\Factorio\BBQ-CHAOS-Reports` massgeblich. Keine automatischen Eigenpatches.
5. `BBQ-UPDATE.cmd pruefen` fuer reine Pruefung, `BBQ-UPDATE.cmd test` fuer den Parser-Test. **Keine veraenderten Original-ZIPs wieder in das Verzeichnis legen.**
6. Factorio auf einem **neuen Test-Spielstand** starten und Planeten-/Produktionsketten testen, bevor derselbe Satz im Multiplayer verwendet wird.

Das **Serververzeichnis von AMP wird vom Windows-Updater nicht veraendert**. Die AMP-Instanz war zwischenzeitlich mit einer **lokal gepatchten** Angel-Fork startfaehig und laeuft damit **nicht** mit den unveraenderten Windows-Mods synchron. Vor dem naechsten Multiplayerstart **Server herunterfahren, Konfiguration und Save sichern**, alte BBQ-Testfix-ZIPs entfernen, **Originaldateien aus dem offiziellen Modportal** verwenden und die `mod-list.json` auf den neuen Stand bringen. Ein neues Spiel ist bei Aenderungen an Angel/Bob/Space-Age-Rezepten unter Umstaenden erforderlich. **Diese README ist keine Aussage, dass AMP bereits umgestellt ist.**

## Sicherheit / GitHub-Checks

- Keine Accounts, privaten Dateien, Tokens, Passwoerter, `mod-settings.dat` oder Savegames auf GitHub hochladen.
- Die aktuelle CMD laedt genau festgelegte Dateien aus `Technox90/projekte`; SHA-256 verifiziert den Downloader-Core. Downloads aus dem Modportal muessen zu den vom Portal veroeffentlichten ZIP-Hashes passen.
- GitHub Actions prueft JSON, Pins, bekannte Inkompatibilitaeten und PowerShell-Syntax; sie kann **keinen vollstaendigen Spieltest oder die Funktion aller Rezepte beweisen**.
- Downloaderfehler nicht durch das Ausschalten von `quality`, `space-age` oder grosser Bob-/Angel-Kernbereiche umgehen.
