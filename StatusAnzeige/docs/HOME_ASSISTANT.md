# Home Assistant

## MQTT (empfohlen)

In `/config` MQTT aktivieren und Brokerdaten eintragen. Mit PubSubClient sendet das Geraet:

- `statusanzeige/state`
- `statusanzeige/set`
- `statusanzeige/availability`
- `statusanzeige/battery`
- `statusanzeige/rssi`

Home Assistant Discovery ist standardmaessig aktivierbar.

## REST ohne MQTT

Beispiel `configuration.yaml`:

```yaml
rest_command:
  statusanzeige_frei:
    url: "http://statusanzeige.local/api/status?id=frei&ps=123456"
  statusanzeige_busy:
    url: "http://statusanzeige.local/api/status?id=busy&ps=123456"
  statusanzeige_live:
    url: "http://statusanzeige.local/api/status?id=live&ps=123456"
```

API-Passcode vorher aendern.
