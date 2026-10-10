#!/usr/bin/env bash
set -Eeuo pipefail
APP=/opt/bbq-chaos-radiobot
BASE=https://raw.githubusercontent.com/Technox90/projekte/main/DC-Musikbot
[[ $EUID -eq 0 ]] || { echo "Als root ausfuehren" >&2; exit 1; }
[[ -f "$APP/bot.py" && -f "$APP/radio.env" && -x "$APP/.venv/bin/python" ]] || {
  echo "Vorhandene Installation unvollstaendig; kein Update ausgefuehrt" >&2; exit 1;
}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
for file in bot.py nowplaying.py commands.py; do
  curl -fsSL "$BASE/$file" -o "$TMP/$file"
done
"$APP/.venv/bin/python" -m py_compile "$TMP/bot.py" "$TMP/nowplaying.py" "$TMP/commands.py"
BACKUP="$APP/backup-$(date +%Y%m%d-%H%M%S)"
mkdir -m 0700 "$BACKUP"
cp -a "$APP/bot.py" "$BACKUP/bot.py"
for file in bot.py nowplaying.py commands.py; do
  if [[ -f "$APP/$file" ]]; then cp -a "$APP/$file" "$BACKUP/$file"; fi
done
for file in bot.py nowplaying.py commands.py; do
  install -o bbqradio -g bbqradio -m 0640 "$TMP/$file" "$APP/$file"
done
systemctl restart bbq-chaos-radio
echo "Update abgeschlossen. Backup: $BACKUP"
echo "Logs: journalctl -u bbq-chaos-radio -f"
echo "Discord-Berechtigungen: Nachrichten senden, Links einbetten, Nachrichtenverlauf lesen."
