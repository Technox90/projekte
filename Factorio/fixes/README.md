# Experimenteller Rezeptfix – Turbo-Bänder (AMP / Factorio 2.0.77)

**Status: noch nicht in einer gestarteten Spielwelt validiert.**

Bei **Bob's Logistics 2.1.1** wurde der Prefix `bob-` bei Turbo-Bändern entfernt. In der bisherigen Kombination mit Space Age / Angel's Space Age Compatibility Revived erzeugt das Rezept `turbo-transport-belt` dennoch eine Zutat `bob-turbo-transport-belt`, die nicht existiert. Siehe [Bob's Logistics Changelog](https://mods.factorio.com/mod/boblogistics/changelog) (2.1.0).

## Wirkung

Die kleine Mod `bbq-chaos-belt-compat` 1.0.0 ändert **nur** im Rezept `turbo-transport-belt` einen fehlenden `bob-turbo-transport-belt`-Eingang zu `express-transport-belt`. Sie ändert nichts, wenn die Bob-Variante tatsächlich existiert. Diese Zuordnung ist ein pragmatischer Testfix, keine offiziell verifizierte Balancing-Anpassung.

## Installation / Tests

- Factorio01 in AMP **stoppen**.
- Unter `Factorio/fixes/bbq-chaos-belt-compat` liegen `info.json` und `data-final-fixes.lua` für eine normale Factorio-2.0-Mod.
- Beide Dateien in ZIP `bbq-chaos-belt-compat_1.0.0.zip` packen, dabei muss der ZIP-Inhalt den Wurzelordner `bbq-chaos-belt-compat_1.0.0/` besitzen.
- ZIP in `.../factorio/server/mods/`, Eintrag `{"name":"bbq-chaos-belt-compat","enabled":true}` in die **serverseitige** `mod-list.json` ergänzen.
- AMP neu starten, alle Ladephasen bis zur erfolgreichen Kartenanlage prüfen.
- Für Clients muss derselbe Modstand separat bereitgestellt werden. **Der Windows-Updater installiert dieses Testpaket noch nicht automatisch.**

**Kein Downgrade von Bob's Logistics, kein pauschales Entfernen der anderen Bob-Mods.** Erst nach erfolgreichen Tests in die zentrale Liste und den Downloader übernehmen.
