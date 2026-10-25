# Kalenderstatus – StatusAnzeige 2.5.0

Die StatusAnzeige kann Termine automatisch in einen Status wie `meeting` umsetzen.

## Unterstützte Quellen

### 1. iCal / ICS
Beliebige erreichbare `.ics`-URL.

### 2. Google Calendar
Google Calendar wird über den **privaten iCal-Link** eines Kalenders eingebunden.
Dadurch ist auf dem ESP32 kein OAuth-Login erforderlich.

### 3. Home Assistant
Direkter Zugriff auf eine Home-Assistant-Kalender-Entity über die REST-API.

Benötigt:
- Home-Assistant Basis-URL, z. B. `http://homeassistant.local:8123`
- Kalender-Entity, z. B. `calendar.arbeit`
- Long-Lived Access Token

## Weboberfläche

`http://<IP>/calendar`

Dort können eingestellt werden:

- Kalenderstatus AN/AUS
- Quelle
- iCal-/Google-ICS-URL
- Home-Assistant URL
- Home-Assistant Kalender-Entity
- Home-Assistant Access Token
- Status während eines Termins
- Vorlauf in Minuten
- Nachlauf in Minuten
- Abfrageintervall
- Ganztägige Termine ignorieren
- Schlüsselwortfilter
- Termin-Titel auf dem Display anzeigen
- Verhalten nach Termin:
  - vorherigen Status wiederherstellen
  - FREI
  - Standardstatus

## Schlüsselwortfilter

Mehrere Begriffe werden mit Komma getrennt:

`Meeting, Teams, Besprechung`

Ist das Feld leer, werden alle Termine berücksichtigt.

## Test

Auf der Kalenderseite gibt es **Kalender testen**.
Damit wird die konfigurierte Quelle abgefragt und der nächste bzw. aktuell relevante Termin angezeigt.

## Statuslogik

Ein Termin gilt als aktiv:

`Terminbeginn - Vorlauf <= Jetzt <= Terminende + Nachlauf`

Beim ersten Aktivwerden merkt sich die StatusAnzeige den vorherigen Status.
Nach dem Termin wird entsprechend der gewählten Rückkehrstrategie gewechselt.

## Sicherheit

Private Google-iCal-Links und Home-Assistant-Tokens sind Zugangsdaten.
Sie werden in `/config/config.json` auf der SD-Karte gespeichert.

Der Home-Assistant-Token wird über die Web-Konfigurations-API nicht wieder ausgegeben.
