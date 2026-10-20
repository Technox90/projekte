# Nightscout food import (English)

[Deutsche Anleitung](README-DE.md) · [German CSV template](template-de.csv) · [English CSV template](template-en.csv)

This optional tool imports food records into Nightscout's **food** collection. After Nightscout synchronization, these records can appear in AAPS **FOOD**. In **AAPS 3.4**, the Food tab is **display-only**; it does not directly fill the bolus wizard.

The importer **does not** write glucose entries, treatments, insulin doses, pump commands, basal rates, or therapy profiles. It runs independently of the existing Nightscout data exporter.

## 1. Review the populated food dataset

Download [template-en.csv](template-en.csv): it now contains **93 foods populated with average nutrient reference values** from Germany's BLS 4.0 database. Review and adjust them in a spreadsheet editor or text editor, then save the reviewed copy as `foods-en.csv`. These are generic averages, **not your product's label values**. See [sources and licensing](QUELLEN.md) and [sources for the 48 additions](QUELLEN-ERWEITERUNG.md).

CSV columns:

```csv
Name,Category,Subcategory,Portion,Unit,Carbs_g,Fat_g,Protein_g,Energy_kJ,GI
```

- Use a comma as the CSV delimiter and a decimal **dot** for numbers (e.g. `12.5`).
- Units accepted by the Nightscout Food Editor: `g`, `ml`, `pcs`, `oz`. You may also enter `piece` or `pieces` and they are converted to `pcs`.
- `Carbs_g` means carbohydrate grams **for the stated portion**. All 93 reference rows use **100 g or 100 ml**, so these example values are per 100 g/ml. If you change a portion size, recalculate its nutrient amounts.
- `GI` is Nightscout's 1–3 categorical scale (1 low, 2 medium, 3 high), **not** the usual numerical glycemic index. GI is **blank** in the CSV because no glycemic index was measured; the importer defaults a blank field to **2**, which must not be interpreted as a measured value.
- Fat, protein, and energy can be blank (saved as zero). Name, portion, unit, and carbohydrate amount must be supplied.
- Check nutrition values against actual packaging or reliable food data; incorrect values can affect later carb or insulin calculations.

## 2. Download the importer on your Plesk host

Your Nightscout already runs at local address `http://127.0.0.1:1337`. Use a **separate** directory:

```bash
install -d -m 700 /opt/nightscout-food-import/config
curl -fL --proto '=https' --tlsv1.2 \
  https://raw.githubusercontent.com/Technox90/projekte/main/nightscout/food-import/import-food.py \
  -o /opt/nightscout-food-import/import-food.py
curl -fL --proto '=https' --tlsv1.2 \
  https://raw.githubusercontent.com/Technox90/projekte/main/nightscout/food-import/template-en.csv \
  -o /opt/nightscout-food-import/foods-en.csv
python3 -m py_compile /opt/nightscout-food-import/import-food.py
```

Review the included reference values in `foods-en.csv`, correcting them for the foods you actually use, or upload your edited CSV with SFTP.

## 3. Create a dedicated food-only Nightscout token

In Nightscout **Admin Tools → Roles**, add a custom role with **only** the permissions `api:food:read` and `api:food:create`. Then create a Subject (e.g. `food-import`) assigned to that role. Admin Tool forms vary by Nightscout version.

Do **not** reuse the read-only exporter token; it has no write permission. Do **not** reuse the AAPS admin token. The importer rejects any token granting create/update/delete access to the entries, treatments, profile or devicestatus collections.

Store the token **on the host only**:

```bash
umask 077
read -r -s -p "Nightscout food token: " FOOD_TOKEN; echo
printf '%s' "$FOOD_TOKEN" > /opt/nightscout-food-import/config/food-token
unset FOOD_TOKEN
chmod 600 /opt/nightscout-food-import/config/food-token
```

Never commit or share tokens, CSV files containing personal data, or backups.

## 4. Preview before importing

**Offline dry run (no Nightscout connection, no writes):**

```bash
python3 /opt/nightscout-food-import/import-food.py \
  --file /opt/nightscout-food-import/foods-en.csv --lang en
```

**Only after reviewing your values, explicitly apply:**

```bash
python3 /opt/nightscout-food-import/import-food.py \
  --file /opt/nightscout-food-import/foods-en.csv --lang en \
  --apply --confirm IMPORT_FOOD_ONLY
```

Before writing, the importer verifies Nightscout token scopes, loads existing foods to identify duplicates, and creates a locally protected backup in `/opt/nightscout-food-import/backups`. It only creates missing food records using `POST /api/v3/food`, and never deletes or updates existing foods.

Check `https://sweet.bopzocker.de/food` (the Nightscout `food` plugin may need to be enabled) and then the AAPS Food tab after sync. This is an optional manual task and is never scheduled.

## Security and limitations

- Nightscout may currently be publicly readable without a token; the importer **does not** change authentication settings.
- HTTP requests are limited to `127.0.0.1` or `localhost` on the same host, without sending tokens to the public Internet.
- If an import fails halfway, already-created records remain. A retry skips detectable duplicates.
- The live import has **not been tested** on your Nightscout server. Start with one carefully checked food entry and confirm it displays correctly before importing many records.

References: [Nightscout API v3](https://github.com/nightscout/cgm-remote-monitor/blob/master/lib/api3/swagger.yaml) · [Nightscout Food Editor](https://github.com/nightscout/cgm-remote-monitor/blob/master/lib/food/food.js) · [AAPS 3.4 Food](https://wiki.aaps.app/de/latest/SettingUpAaps/ConfigBuilder.html)

## Added German dishes and everyday foods

The CSV now contains **93 foods per language**, including 24 newly added dishes (such as cheese spaetzle, potato salad, currywurst, schnitzel, filled Swabian pasta, potato dumplings and soups), and 24 further fruit, vegetable, dairy, bakery and pantry items.

**Carbohydrate-safety note:** Values for mixed dishes are BLS reference recipes, not exact values for a specific homemade meal. All amounts are **per 100 g or 100 ml**. Check the exact product/recipe and weighed portion before using values in any insulin-related workflow. No automatic dose recommendations.

Individual sources for the additions: [QUELLEN-ERWEITERUNG.md](QUELLEN-ERWEITERUNG.md).

### Refresh CSV from GitHub (dry run only)

```bash
curl -fL --proto '=https' --tlsv1.2 \
  https://raw.githubusercontent.com/Technox90/projekte/main/nightscout/food-import/template-en.csv \
  -o /opt/nightscout-food-import/foods-en.csv
python3 /opt/nightscout-food-import/import-food.py \
  --file /opt/nightscout-food-import/foods-en.csv --lang en
```

The second command validates the CSV **offline without writing to Nightscout**. Actual import still requires a dedicated food-only token and explicit `--apply --confirm IMPORT_FOOD_ONLY`.
