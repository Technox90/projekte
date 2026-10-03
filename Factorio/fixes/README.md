# BBQ CHAOS – Bob/Space Age Turbo-Belt-Kompatibilität

**Diagnoseversion 1.0.3 · Factorio 2.0.77 · noch nicht mit vollständigem Modpack getestet**

## Beobachteter Fehlerverlauf

- `1.0.0`: Turbo-Fließband-Rezept hatte keine kaputte Zutat, aber einen ungültigen Ergebnis-Namen.
- `1.0.1`: Korrigierte ein Ergebnis (`results=1`); dann kam dieselbe fehlende ID.
- `1.0.2`: Der Fehler liegt nun **nicht mehr** im Rezept `turbo-transport-belt`, sondern im Rezept `turbo-loader`.

Fehler: `item with name 'bob-turbo-transport-belt' does not exist`. [Bob's Logistics Changelog](https://mods.factorio.com/mod/boblogistics/changelog) bestätigt: Ab Version 2.1.0 wurde der Präfix `bob-` bei Turbo-Band, Untergrundband und Verteiler entfernt.

## Funktionsweise von 1.0.3

`data-final-fixes.lua` läuft nach `loaders-modernized` und den Reskins-Mods, um die Legacy-ID `bob-turbo-transport-belt` zu beseitigen, **sofern die alte Item-ID tatsächlich nicht existiert**:

- In Rezepten außer `turbo-transport-belt` ersetzen wir Zutaten `bob-turbo-transport-belt` → `turbo-transport-belt`; somit kann z. B. `turbo-loader` das gültige Space-Age-Band verwenden.
- Nur beim Rezept `turbo-transport-belt` selbst führen wir die entsprechende Zutat auf `express-transport-belt`, um eine unbrauchbare Selbstabhängigkeit zu vermeiden.
- Ungültige Ergebnisse und `main_product` werden auf `turbo-transport-belt` angepasst.
- Sämtliche Änderungen werden **je Rezept und als SUMMARY** geloggt.
- Es werden **keine weiteren Item-IDs** verändert, Mods deaktiviert oder Spieleinstellungen zurückgesetzt.

Dies ist ein gezielter **Testfix**, keine bestätigte Originalrezeptur oder Balance der beteiligten Mods.

## Anwendung auf AMP

1. Server **Factorio01 stoppen**.
2. Das GitHub-Installationsskript `Factorio/fixes/install-amp.sh` herunterladen und mit `sudo bash` ausführen. Es sichert `mod-list.json`, schreibt die neue ZIP in den Modordner und archiviert alte Versionen außerhalb.
3. Server starten und Log nach `bbq-chaos-belt-compat`, `BBQ CHAOS BELT FIX` und `Error` prüfen.

Im Log muss `Loading mod bbq-chaos-belt-compat 1.0.3 (data-final-fixes.lua)` stehen. Danach sollte `[BBQ CHAOS BELT FIX v1.0.3] turbo-loader changed=...` erscheinen, sofern eine Legacy-Referenz in Zutaten oder Resultaten vorliegt.

**Wichtig:** Das zentrale `Factorio/mod-list.json` für Windows wird **nicht** automatisch um diese private Mod ergänzt. Erst einen vollständigen Serverstart validieren, dann Client und Server synchronisieren, damit Multiplayer keinen Mod-Mismatch bekommt.
