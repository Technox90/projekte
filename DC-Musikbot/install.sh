#!/usr/bin/env bash
set -Eeuo pipefail

APP_DIR=/opt/bbq-chaos-radiobot
SERVICE=bbq-chaos-radio
BOT_USER=bbqradio
REPO_BASE=https://raw.githubusercontent.com/Technox90/projekte/main/DC-Musikbot
STREAM_DEFAULT=https://stream.bopzocker.de/listen/chaos/radio.mp3

[[ $EUID -eq 0 ]] || { echo 'Bitte als root ausfuehren.' >&2; exit 1; }
command -v apt-get >/dev/null || { echo 'Nur Debian/Ubuntu mit apt unterstuetzt.' >&2; exit 1; }
if [[ -e "$APP_DIR" || -e "/etc/systemd/system/$SERVICE.service" ]]; then
  echo 'Installation existiert bereits. Nichts wird ueberschrieben.' >&2
  exit 1
fi
echo 'BBQ-Chaos Discord-Radiobot – Hetzner Direktinstallation'
echo 'Die bestehende AzuraCast-Installation wird nicht veraendert.'
read -r -p 'Discord-Sprachkanal-ID (15-22 Ziffern): ' CHANNEL_ID
[[ $CHANNEL_ID =~ ^[0-9]{15,22}$ ]] || { echo 'Ungueltige Kanal-ID' >&2; exit 1; }
read -r -p "Stream-URL [$STREAM_DEFAULT]: " STREAM_URL
STREAM_URL="${STREAM_URL:-$STREAM_DEFAULT}"
[[ $STREAM_URL =~ ^https?://[^[:space:]]+$ ]] || { echo 'Ungueltige Stream-URL' >&2; exit 1; }
read -r -s -p 'Bot-Token (verdeckt): ' BOT_TOKEN
echo
[[ -n "$BOT_TOKEN" && "$BOT_TOKEN" != *$'\n'* && "$BOT_TOKEN" != *$'\r'* ]] || { echo 'Ungueltiges Token' >&2; exit 1; }

export DEBIAN_FRONTEND=noninteractive
apt-get update
apt-get install -y --no-install-recommends python3 python3-venv python3-pip ffmpeg ca-certificates curl libopus0 libsodium23
if ! id "$BOT_USER" >/dev/null 2>&1; then
  useradd --system --home-dir "$APP_DIR" --shell /usr/sbin/nologin "$BOT_USER"
fi
install -d -m 0750 -o "$BOT_USER" -g "$BOT_USER" "$APP_DIR"
curl --fail --location --silent --show-error "$REPO_BASE/bot.py" -o "$APP_DIR/bot.py"
curl --fail --location --silent --show-error "$REPO_BASE/nowplaying.py" -o "$APP_DIR/nowplaying.py"
curl --fail --location --silent --show-error "$REPO_BASE/requirements.txt" -o "$APP_DIR/requirements.txt"
python3 -m venv "$APP_DIR/.venv"
"$APP_DIR/.venv/bin/python" -m pip install --no-cache-dir --upgrade pip
"$APP_DIR/.venv/bin/python" -m pip install --no-cache-dir -r "$APP_DIR/requirements.txt"
# EnvironmentFile uses systemd syntax; prohibit unsafe line breaks and quote characters.
if [[ "$BOT_TOKEN" == *'"'* || "$BOT_TOKEN" == *'\\'* || "$STREAM_URL" == *'"'* || "$STREAM_URL" == *'\\'* ]]; then
  echo 'Token/URL enthaelt nicht unterstuetzte Zeichen.' >&2
  exit 1
fi
cat >"$APP_DIR/radio.env" <<EOF
DISCORD_BOT_TOKEN="$BOT_TOKEN"
DISCORD_CHANNEL_ID="$CHANNEL_ID"
RADIO_STREAM_URL="$STREAM_URL"
EOF
unset BOT_TOKEN
chmod 0600 "$APP_DIR/radio.env"
chown -R "$BOT_USER:$BOT_USER" "$APP_DIR"

cat >"/etc/systemd/system/$SERVICE.service" <<EOF
[Unit]
Description=BBQ-Chaos-Deutschland 24/7 Discord-Radiobot
Wants=network-online.target
After=network-online.target

[Service]
Type=simple
User=$BOT_USER
Group=$BOT_USER
WorkingDirectory=$APP_DIR
EnvironmentFile=$APP_DIR/radio.env
ExecStart=$APP_DIR/.venv/bin/python $APP_DIR/bot.py
Restart=always
RestartSec=10
TimeoutStopSec=20
NoNewPrivileges=true
ProtectSystem=full
ProtectHome=true
PrivateTmp=true

[Install]
WantedBy=multi-user.target
EOF

"$APP_DIR/.venv/bin/python" -m py_compile "$APP_DIR/bot.py"
systemctl daemon-reload
systemctl enable --now "$SERVICE"
echo
echo "Fertig. Status: systemctl status $SERVICE"
echo "Logs: journalctl -u $SERVICE -f"
echo 'Der Dienststart garantiert noch keine Discord-Verbindung. Logs pruefen.'
