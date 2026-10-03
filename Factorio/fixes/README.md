# Experimenteller Turbo-Band-Rezeptfix (AMP / Factorio 2.0.77)

**Version: 1.0.1 – noch nicht mit dem vollständigen Modpack im Spiel getestet.**

### Ausgangsfehler

Nach dem erfolgreichen Laden von Quality und Cargo Ships tritt bei der Endprüfung auf:

```text
Error in assignID: item with name 'bob-turbo-transport-belt' does not exist.
Source: turbo-transport-belt (recipe).
```

Bob's Logistics 2.1.0/2.1.1 verwendet für Turbo-Bänder andere IDs als ältere Integrationen. Der fehlerhafte Rezeptverweis kann eine **Zutat oder ein Ergebnis** sein; die ursprüngliche Testversion 1.0.0 korrigierte nur Zutaten.

### Wirkung von 1.0.1

Die Testmod `bbq-chaos-belt-compat` greift ausschließlich bei `data.raw.recipe["turbo-transport-belt"]` ein und nur, falls das alte Item nicht existiert:

- Fehlende **Zutaten** `bob-turbo-transport-belt` werden zu `express-transport-belt`.
- Fehlende **Ergebnisse** und `main_product` werden zu `turbo-transport-belt`.
- Im Log wird protokolliert: `[BBQ CHAOS BELT FIX v1.0.1] recipe=... ingredients=N results=N main_products=N`.
- Bestehende Items und andere Rezepte bleiben unverändert.

Das ist ein gezielter **Testfix**, keine bestätigte offizielle Rezeptbalance.

### Installation auf Factorio01

1. **Factorio01 in AMP stoppen.**
2. Im SSH-Terminal die Datei `Factorio/fixes/install-amp.sh` ausführen (als root/sudo). Erzeugt vor Änderungen ein Backup von `mod-list.json`.
3. Es wird `bbq-chaos-belt-compat_1.0.1.zip` mit dem korrekten Mod-Ordner ins AMP-Verzeichnis geschrieben und die Mod aktiviert.
4. Server in AMP starten, **vollständigen** Start-Log lesen. Im Log muss `Loading mod bbq-chaos-belt-compat 1.0.1 (data-final-fixes.lua)` stehen und eine eigene `[BBQ CHAOS BELT FIX v1.0.1]`-Zeile erscheinen.

**Diagnose auf AMP:**

```bash
D=/home/amp/.ampdata/instances/Factorio01/factorio/server
grep -inE 'bbq-chaos-belt-compat|BBQ CHAOS BELT FIX|Error in assignID' "$D/factorio-current.log" | tail -25
grep -A2 -B2 'bbq-chaos-belt-compat' "$D/mods/mod-list.json"
ls -lh "$D/mods"/bbq-chaos-belt-compat*
```

Wenn `Loading mod` fehlt, liegt das Problem an der tatsächlichen AMP-Modauswahl, nicht an der Rezeptkorrektur. Fehlt dagegen die alte ID in Zutaten und Ergebnissen, ist der Ursprung möglicherweise in einem anderen Feld oder einer späteren Änderung zu suchen; dann ist der vollständige Log nötig.

### Wichtig

Die GitHub-Standardliste und der **Windows-Downloader installieren diesen privaten Testfix nicht**. Vor produktivem Multiplayer müssen Server und Clients dieselbe gültige, getestete Version der Mod verwenden. Danach kann der Fix in den gemeinsamen GitHub-Download integriert werden.
