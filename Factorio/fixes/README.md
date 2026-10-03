# BBQ CHAOS – Turbo-Transport-Belt-Kompatibilitätsfix

**Testversion 1.0.2 · Factorio 2.0.77 · AMP Linux · Noch nicht als vollständiges Modpack getestet**

## Beobachtete Fehler

- Bob's Logistics 2.1.1 hat die ID `bob-turbo-transport-belt` zugunsten von `turbo-transport-belt` geändert.
- Factorio beim `assignID`: `Source: turbo-transport-belt (recipe)` mit nicht existierender `bob-turbo-transport-belt`-ID.
- **v1.0.0:** fand keine Zutat (ingredients=0); Fehler blieb.
- **v1.0.1:** ersetzte einen Ergebnisverweis (results=1), Fehler blieb. Log belegte, dass **reskins-library**, **reskins-bobs**, **squeak-through-2**, **reskins-angels** erst **nach** v1.0.1 ihre finalen Änderungen ausführten.

## v1.0.2 – gezielter Folgetest

- Die optionalen Abhängigkeiten `? reskins-library`, `? reskins-bobs`, `? reskins-angels`, `? squeak-through-2` ordnen die Reparatur **nach** diesen aktiven Mods ein.
- Nur im Rezept `turbo-transport-belt`: alte Bob-Referenz in Zutaten durch `express-transport-belt`, in Ergebnissen/`main_product` durch `turbo-transport-belt` ersetzen, falls das alte Item nicht existiert.
- Vorher und nachher werden alle verbliebenen direkten und verschachtelten Vorkommen der Legacy-ID im Rezept protokolliert.
- **Keine weiteren Bob-Mods deaktiviert.** Auch dies ist keine offiziell bestätigte Balancing-Regel und erfordert einen echten Spieltest.

## AMP Installation

1. `Factorio01` in AMP stoppen.
2. Das Skript [install-amp.sh](install-amp.sh) als Administrator ausführen (neueste Fassung lädt `1.0.2`). Das Skript sichert die `mod-list.json` und archiviert alte Testfix-ZIPs außerhalb des Modordners.
3. Server in AMP starten und das **neue** `factorio-current.log` prüfen.
4. Erwartetes Logging:
    - `Loading mod bbq-chaos-belt-compat 1.0.2 (data-final-fixes.lua)`
    - `[BBQ CHAOS BELT FIX v1.0.2] BEFORE legacy_references=N`
    - `[BBQ CHAOS BELT FIX v1.0.2] CHANGED ingredients=N results=N main_products=N`
    - `[BBQ CHAOS BELT FIX v1.0.2] AFTER legacy_references=N`
    - ggf. `UNRESOLVED_PATH recipe.<Feld>`

```bash
D=/home/amp/.ampdata/instances/Factorio01/factorio/server
stat -c '%y' "$D/factorio-current.log"
grep -inE 'bbq-chaos-belt-compat|BBQ CHAOS BELT FIX|Error|Saving' "$D/factorio-current.log" | tail -45
```

**Wichtig:** Die modifizierte `bbq-chaos-belt-compat`-ZIP ist derzeit ein lokaler AMP-Testfix, **nicht** Teil der öffentlichen GitHub-`mod-list.json` oder des Windows-Downloaders. Auf dem Multiplayer-Server wird ein anderer Modsatz als auf Windows geladen, bis die getestete Fassung auf beiden Seiten übernommen wurde. Den Server bitte nicht für Spieler öffnen, bevor Client und Server übereinstimmen.
