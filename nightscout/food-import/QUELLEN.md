# Datenquellen und Nutzung / Data sources and usage

## Attribution / Quellenangabe

**Max Rubner-Institut (2025):** *Bundeslebensmittelschlüssel (BLS), Version 4.0 — Deutsche Nährstoffdatenbank*, Karlsruhe. DOI: [10.25826/Data20251217-134202-0](https://doi.org/10.25826/Data20251217-134202-0). Lizenz / license: [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/).

Offizielle Übersicht / official description: [Max Rubner-Institut](https://www.mri.bund.de/de/institute/ernaehrungsverhalten/forschungsbereiche/bundeslebensmittelschluessel-bls/).

Für die ursprünglich enthaltenen 45 Datensätze wurden je Lebensmittel die gerundeten Referenzwerte aus den auf [kalorien.info](https://kalorien.info/daten-quellen/) dargestellten BLS-4.0-Werten für Energie, Kohlenhydrate, Fett und Eiweiß übernommen. Änderungen: **Auswahl der Lebensmittel, Gruppierung in Kategorien, Kurzbenennung, Übersetzung der Namen und Spalten in Englisch sowie Standardisierung der Portionsgröße auf 100 g oder 100 ml**. Für Getränke wird die dort verwendete Näherung 100 ml ≈ 100 g übernommen.

The original 45 sample items reproduce rounded reference values (energy, carbohydrates, fat, protein) published from BLS 4.0 by kalorien.info. Changes include **selection, category grouping, simplified labels, English translations, and standard 100 g or 100 ml reference portions**. Beverage values use the source's approximate 100 ml ≈ 100 g assumption.

**Insgesamt 93 Einträge pro Sprache:** Die ursprünglichen 45 Referenzeinträge wurden um 24 Gerichte und 24 normale Lebensmittel ergänzt. Einzelquellen aller Ergänzungen, Rezept- und Umrechnungshinweise siehe [QUELLEN-ERWEITERUNG.md](QUELLEN-ERWEITERUNG.md).

## Einzelquellen / Per-food lookup

- [Apfel / Apple](https://kalorien.info/lebensmittel/apfel/)
- [Banane / Banana](https://kalorien.info/lebensmittel/banane/)
- [Orange](https://kalorien.info/lebensmittel/orange/)
- [Kiwi](https://kalorien.info/lebensmittel/kiwi/)
- [Avocado](https://kalorien.info/lebensmittel/avocado/)
- [Brokkoli / Broccoli](https://kalorien.info/lebensmittel/brokkoli/)
- [Karotte / Carrot](https://kalorien.info/lebensmittel/karotte/)
- [Tomate / Tomato](https://kalorien.info/lebensmittel/tomate/)
- [Gurke / Cucumber](https://kalorien.info/lebensmittel/gurke/)
- [Kartoffeln / Potatoes](https://kalorien.info/lebensmittel/kartoffeln/)
- [Reis / Rice](https://kalorien.info/lebensmittel/reis/)
- [Nudeln / Pasta](https://kalorien.info/lebensmittel/nudeln/)
- [Haferflocken / Oats](https://kalorien.info/lebensmittel/haferflocken/)
- [Vollkornbrot / Wholegrain bread](https://kalorien.info/lebensmittel/vollkornbrot/)
- [Toastbrot / Toast](https://kalorien.info/lebensmittel/weizentoast/)
- [Brötchen / Bread rolls](https://kalorien.info/lebensmittel/broetchen/)
- [Müsli / Muesli](https://kalorien.info/lebensmittel/muesli/)
- [Linsen / Lentils](https://kalorien.info/lebensmittel/linsen/)
- [Croissant](https://kalorien.info/lebensmittel/croissant/)
- [Pommes frites](https://kalorien.info/lebensmittel/pommes-frites/)
- [Kartoffelchips / Crisps](https://kalorien.info/lebensmittel/kartoffelchips/)
- [Milch / Milk](https://kalorien.info/lebensmittel/milch-1-5/)
- [Naturjoghurt / Plain yoghurt](https://kalorien.info/lebensmittel/naturjoghurt/)
- [Gouda](https://kalorien.info/lebensmittel/gouda/)
- [Körniger Frischkäse / Cottage cheese](https://kalorien.info/lebensmittel/koerniger-frischkaese/)
- [Ei / Egg](https://kalorien.info/lebensmittel/ei/)
- [Hähnchenbrust / Chicken breast](https://kalorien.info/lebensmittel/haehnchenbrust/)
- [Butter](https://kalorien.info/lebensmittel/butter/)
- [Walnüsse / Walnuts](https://kalorien.info/lebensmittel/walnuesse/)
- [Erdnussbutter / Peanut butter](https://kalorien.info/lebensmittel/erdnussbutter/)
- [Zucker / Sugar](https://kalorien.info/lebensmittel/zucker/)
- [Honig / Honey](https://kalorien.info/lebensmittel/honig/)
- [Fruchtgummi / Fruit gummies](https://kalorien.info/lebensmittel/fruchtgummi/)
- [Vollmilchschokolade / Milk chocolate](https://kalorien.info/lebensmittel/vollmilchschokolade/)
- [Zartbitterschokolade / Dark chocolate](https://kalorien.info/lebensmittel/zartbitterschokolade/)
- [Apfelsaft / Apple juice](https://kalorien.info/lebensmittel/apfelsaft/)
- [Orangensaft / Orange juice](https://kalorien.info/lebensmittel/orangensaft/)
- [Cola](https://kalorien.info/lebensmittel/cola/)
- [Haferdrink / Oat drink](https://kalorien.info/lebensmittel/haferdrink/)

## Wichtige Hinweise / Important notices

- **Deutsch:** Durchschnittliche Referenz-Nährwerte sind keine Herstellerangaben. Gerade Brot, Süßigkeiten, Fertigprodukte und Getränkerezepturen können abweichen. **Vor einem Import in Nightscout alle relevanten Kohlenhydratwerte gegen die konkrete Packung und tatsächlich gegessene Menge prüfen.**
- **English:** Generic reference values are not manufacturer-specific. Actual recipes and brand products may differ substantially. **Verify carbohydrate numbers against the exact food and weighed portion before importing or using them to inform insulin treatment.**
- Gekochter Reis, gekochte Nudeln und gekochte Kartoffeln sind **nicht** mit Rohgewichten austauschbar. / Cooked and dry weights are **not interchangeable**.
- Die Spalte GI bleibt leer: Es wurde **kein gemessener glykämischer Index** beigefügt. Der Importer setzt leer technisch auf `2`; das bedeutet **nicht**, dass für die Lebensmittel ein GI-Wert 2 bestimmt wurde.
- Beide Sprachdateien beschreiben dieselben 93 Lebensmittel. **Nicht beide Sprachdateien unkontrolliert importieren**, sonst entstehen doppelte Einträge unter anderen Namen. / Do **not** import both language files indiscriminately.
- Die CSV ist öffentlich und enthält **keine persönlichen Gesundheitsdaten oder Zugangsdaten**. Die Quelle wird für die Referenzwerte genannt; das ersetzt keine individuelle Ernährungs- oder Diabetesberatung.
