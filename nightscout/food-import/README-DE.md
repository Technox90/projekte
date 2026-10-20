# Nightscout-Lebensmittelimport (Deutsch)

[English instructions](README-EN.md) · [CSV-Vorlage DE](template-de.csv) · [CSV template EN](template-en.csv)

Dieses **optionale** Werkzeug importiert Lebensmittel in die Nightscout-**food**-Datenbank. Die Lebensmittel erscheinen nach erfolgreicher Synchronisierung im AAPS-Reiter **ESSEN**. In **AAPS 3.4** ist dieser Reiter eine **reine Anzeige**; Lebensmittel werden dort nicht direkt im Bolusrechner übernommen.

Der Importer verändert **keine** Glukosewerte, Behandlungen, Pumpenbefehle, Profile oder Basalraten. Er ist unabhängig vom vorhandenen Nightscout-Exporter.

## 1. CSV-Vorlage ausfüllen

Datei: [template-de.csv](template-de.csv). In Excel, LibreOffice oder einem Editor öffnen und unter einem **anderen Namen** speichern, z. B. `foods-de.csv`.

| Name | Kategorie | Unterkategorie | Portion | Einheit | Kohlenhydrate_g | Fett_g | Eiweiss_g | Energie_kJ | GI |
|---|---|---|---:|---|---:|---:|---:|---:|---:|
| *Bezeichnung* | *z. B. Frühstück* | *z. B. Brot* | *Menge* | *g/ml/pcs/oz* | *KH für diese Menge* | *optional* | *optional* | *optional* | *1, 2 oder 3* |

**Die Tabellenzeile oben ist nur eine Beschreibung und kein Beispieldatensatz.** Die bereitgestellte CSV enthält ausschließlich die Kopfzeile. Eigene Nährwerte anhand Verpackung oder verlässlicher Datenquelle eintragen – keine geschätzten Kohlenhydratwerte übernehmen.

- In deutscher CSV Spaltentrenner `;`, Dezimalzahlen `12,5` oder `12.5`.
- Nightscout verwendet nur die Einheiten `g`, `ml`, `pcs` (Stück) und `oz`. Auch `Stück` wird erkannt und in `pcs` umgewandelt.
- `Kohlenhydrate_g` ist die **gesamte KH-Menge der eingetragenen Portion**, **nicht automatisch je 100 g**.
- `GI` ist in Nightscout eine Kategorie: `1` niedrig, `2` mittel, `3` hoch (nicht der klassische numerische glykämische Index). Leer = 2.
- Fett, Eiweiß und Energie dürfen leer bleiben und werden dann mit 0 gespeichert. Lebensmittelname, Portion, Einheit und KH müssen ausgefüllt werden.
- Keine medizinischen Dosierungsempfehlungen: fehlerhafte KH-Angaben können spätere Bolusberechnungen verfälschen.

## 2. Dateien auf den Plesk-Server laden

Nightscout läuft bereits unter `/opt/nightscout` und lokal unter `127.0.0.1:1337`. Der Food-Importer kommt **separat** unter `/opt/nightscout-food-import`:

```bash
install -d -m 700 /opt/nightscout-food-import/config
curl -fL --proto '=https' --tlsv1.2 \
  https://raw.githubusercontent.com/Technox90/projekte/main/nightscout/food-import/import-food.py \
  -o /opt/nightscout-food-import/import-food.py
curl -fL --proto '=https' --tlsv1.2 \
  https://raw.githubusercontent.com/Technox90/projekte/main/nightscout/food-import/template-de.csv \
  -o /opt/nightscout-food-import/foods-de.csv
python3 -m py_compile /opt/nightscout-food-import/import-food.py
```

`foods-de.csv` füllst Du danach im Editor aus oder lädst Deine bearbeitete CSV mit WinSCP an diesen Pfad.

## 3. Separaten Food-Token erstellen

Im Nightscout-Menü **Admin Tools → Roles** eine Rolle ausschließlich mit `api:food:read` und `api:food:create` anlegen; dann unter **Subjects** einen Benutzer für diese Rolle erzeugen (z. B. `food-import`). Die genauen Rollenmasken können je nach Version abweichen.

**Nicht** den vorhandenen `read-token` des Analyse-Exporters benutzen – dieser darf überhaupt nichts schreiben. **Nicht** das AAPS-Admin-Token verwenden. Das Skript lehnt Tokens ab, die Schreibrechte auf `entries`, `treatments`, `profile` oder `devicestatus` besitzen.

Token *nur lokal* speichern (nicht in GitHub, nicht hier in den Chat):

```bash
umask 077
read -r -s -p "Nightscout Food-Token: " FOOD_TOKEN; echo
printf '%s' "$FOOD_TOKEN" > /opt/nightscout-food-import/config/food-token
unset FOOD_TOKEN
chmod 600 /opt/nightscout-food-import/config/food-token
```

## 4. Erst prüfen, dann importieren

**Offline-Vorschau (keine API-Verbindung, keine Änderungen):**

```bash
python3 /opt/nightscout-food-import/import-food.py \
  --file /opt/nightscout-food-import/foods-de.csv --lang de
```

**Erst nach Kontrolle bewusst importieren:**

```bash
python3 /opt/nightscout-food-import/import-food.py \
  --file /opt/nightscout-food-import/foods-de.csv --lang de \
  --apply --confirm IMPORT_FOOD_ONLY
```

Beim echten Import prüft das Skript die Tokenrechte, lädt bestehende Lebensmittel zur Duplikatkontrolle und legt ein lokales Backup unter `/opt/nightscout-food-import/backups` ab. **Nur neue** Einträge werden per `POST /api/v3/food` angelegt. Der Import läuft **nicht** automatisch und löscht oder verändert keine vorhandenen Lebensmittel.

Prüfe anschließend über `https://sweet.bopzocker.de/food` (bei aktiviertem Nightscout-Plugin `food`) und nach der Synchronisierung im AAPS-Reiter **ESSEN**. Einträge werden nur angezeigt; Nährwertangaben vor jeder Verwendung kontrollieren.

## Sicherheit / Einschränkungen

- Die öffentliche Nightscout-API ist möglicherweise ohne Anmeldung lesbar. Dieser Food-Importer ändert die `AUTH_DEFAULT_ROLES`-Einstellung **nicht**.
- Das Programm kommuniziert aus Sicherheitsgründen nur mit `http://127.0.0.1:1337` bzw. `localhost`. Es lädt **keine** CSV-Dateien auf GitHub.
- Wenn der Import mitten im Lauf unterbrochen wird, können bereits angelegte Lebensmittel vorhanden sein; erneutes Ausführen überspringt erkennbare Duplikate.
- Der produktive Import wurde **nicht** auf Deinem Server getestet. Erst mit einer einzelnen überprüften Lebensmittelzeile beginnen und die Anzeige in Nightscout kontrollieren.

Quelle: [Nightscout API v3](https://github.com/nightscout/cgm-remote-monitor/blob/master/lib/api3/swagger.yaml) · [Nightscout Food Editor](https://github.com/nightscout/cgm-remote-monitor/blob/master/lib/food/food.js) · [AAPS 3.4 Essen](https://wiki.aaps.app/de/latest/SettingUpAaps/ConfigBuilder.html)
