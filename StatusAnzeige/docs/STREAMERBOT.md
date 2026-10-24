# Streamer.bot

Die bestehende HTTP-API kann direkt aus Streamer.bot aufgerufen werden.

Beispiele:

- Stream Online -> `http://statusanzeige.local/api/status?id=live&ps=123456`
- Stream Offline -> `...id=frei...`
- BRB -> `...id=bin_gleich_zurueck...`
- Pause -> `...id=pause...`

Empfehlung: In Streamer.bot eine wiederverwendbare Action `StatusAnzeige setzen` mit Status-ID als Argument anlegen.
