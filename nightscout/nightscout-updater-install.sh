#!/usr/bin/env bash
# Nightscout Plesk: weekly patch-only update, fresh backup, health check and rollback.
set -Eeuo pipefail
umask 077

if [[ "$(id -u)" -ne 0 ]]; then
  echo 'Bitte als root ausfuehren.' >&2
  exit 1
fi

for bin in docker curl flock sha256sum python3 systemctl; do
  command -v "$bin" >/dev/null || { echo "Fehlt: $bin" >&2; exit 1; }
done

[[ -f /opt/nightscout/docker-compose.yml ]] || { echo 'Compose-Datei nicht gefunden.' >&2; exit 1; }
[[ "$(docker inspect -f '{{.State.Running}}' nightscout 2>/dev/null)" == true ]] || { echo 'Nightscout laeuft nicht.' >&2; exit 1; }
[[ "$(docker inspect -f '{{.State.Running}}' nightscout-mongo 2>/dev/null)" == true ]] || { echo 'MongoDB laeuft nicht.' >&2; exit 1; }

cat > /usr/local/sbin/nightscout-weekly-update <<'UPDATE'
#!/usr/bin/env bash
set -Eeuo pipefail
umask 077
cd /opt/nightscout

IMAGE='nightscout/cgm-remote-monitor:latest'
ROOT='/root/nightscout-backups'
STATE='/var/lib/nightscout-weekly-updater'
COMPOSE=(docker compose -f /opt/nightscout/docker-compose.yml)
mkdir -p "$ROOT" "$STATE"
chmod 700 "$ROOT" "$STATE"
exec 9>/run/lock/nightscout-weekly-update.lock
flock -n 9 || { echo '[INFO] Update laeuft bereits'; exit 0; }

healthy() {
  curl -fsS --max-time 10 -o /dev/null http://127.0.0.1:1337/api/v1/status.json &&
  curl -fsS --max-time 10 -o /dev/null https://sweet.bopzocker.de/api/v1/status.json
}

wait_healthy() {
  local n
  for n in $(seq 1 36); do
    if [[ "$(docker inspect -f '{{.State.Running}}' nightscout 2>/dev/null || true)" == true ]] && healthy; then
      sleep 10
      healthy && return 0
    fi
    sleep 5
  done
  return 1
}

[[ "$(docker inspect -f '{{.State.Running}}' nightscout-mongo 2>/dev/null)" == true ]] || { echo '[STOP] MongoDB ist nicht aktiv'; exit 1; }
[[ "$(docker inspect -f '{{.State.Running}}' nightscout 2>/dev/null)" == true ]] || { echo '[STOP] Nightscout ist nicht aktiv'; exit 1; }
healthy || { echo '[STOP] Nightscout API vor dem Update nicht gesund; keine Aenderung'; exit 1; }
"${COMPOSE[@]}" config -q

CONFIG_IMAGE="$("${COMPOSE[@]}" config --format json | python3 -c 'import json,sys;print(json.load(sys.stdin)["services"]["nightscout"].get("image",""))')"
[[ "$CONFIG_IMAGE" == "$IMAGE" ]] || { echo "[STOP] Compose-Image ist nicht $IMAGE"; exit 1; }

OLD_ID="$(docker inspect -f '{{.Image}}' nightscout)"
OLD_VERSION="$(docker exec nightscout node -p 'require("./package.json").version')"
[[ "$OLD_VERSION" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]] || { echo '[STOP] Unbekanntes Nightscout-Versionsformat'; exit 1; }

PULLED=0
STOPPED=0
SWITCHED=0
COMPLETE=0
NEW_ID=''

recover() {
  local rc=$?
  trap - EXIT
  if [[ $COMPLETE -eq 0 ]]; then
    if [[ $PULLED -eq 1 ]]; then
      docker image tag "$OLD_ID" "$IMAGE" || true
    fi
    if [[ $SWITCHED -eq 1 ]]; then
      echo '[ROLLBACK] Alte Nightscout-Version wiederherstellen' >&2
      "${COMPOSE[@]}" up -d --no-deps --pull never --force-recreate nightscout || true
      if wait_healthy; then
        echo '[ROLLBACK] Nightscout erneut erreichbar.' >&2
      else
        echo '[ALARM] Nightscout auch nach Rollback nicht gesund! Unmittelbar pruefen.' >&2
      fi
      [[ -z "$NEW_ID" ]] || printf '%s\n' "$NEW_ID" > "$STATE/failed-image-id"
    elif [[ $STOPPED -eq 1 ]]; then
      echo '[WIEDERANLAUF] Nightscout nach Backup-Fehler starten' >&2
      docker start nightscout || true
    fi
  fi
  exit "$rc"
}
trap recover EXIT

# Pull aktualisiert nur die Image-Referenz, nicht den laufenden Container.
docker image tag "$OLD_ID" nightscout/cgm-remote-monitor:weekly-last-good
PULLED=1
docker pull "$IMAGE"
NEW_ID="$(docker image inspect -f '{{.Id}}' "$IMAGE")"

if [[ "$NEW_ID" == "$OLD_ID" ]]; then
  echo "[OK] Kein neues Image; Nightscout $OLD_VERSION bleibt aktiv."
  COMPLETE=1
  exit 0
fi

NEW_VERSION="$(docker run --rm --network none --entrypoint node "$IMAGE" -p 'require("./package.json").version')"
if [[ ! "$NEW_VERSION" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]] ||
   [[ "${NEW_VERSION%.*}" != "${OLD_VERSION%.*}" ]] ||
   ! dpkg --compare-versions "$NEW_VERSION" gt "$OLD_VERSION"; then
  echo "[STOP] $OLD_VERSION -> $NEW_VERSION: Kein freigegebenes Patch-Upgrade; manuelle Pruefung erforderlich."
  exit 0
fi

if [[ -f "$STATE/failed-image-id" ]] && [[ "$(cat "$STATE/failed-image-id")" == "$NEW_ID" ]]; then
  echo "[STOP] Image $NEW_VERSION ist als zuvor fehlgeschlagen markiert."
  exit 0
fi

# Erst nach erfolgreicher Pruefung und Pull folgt die kurzzeitige Downtime.
BK="$ROOT/weekly-$(date +%Y-%m-%d_%H-%M-%S)"
mkdir -m 700 "$BK"
"${COMPOSE[@]}" config -q
tar -C /opt/nightscout --exclude='./mongo' -czf "$BK/config.tar.gz" .
docker inspect nightscout nightscout-mongo > "$BK/docker-inspect.json"

echo "[BACKUP] Nightscout kurz stoppen; $BK"
STOPPED=1
docker stop --timeout 60 nightscout >/dev/null
docker exec nightscout-mongo mongodump --archive --gzip > "$BK/mongodb.archive.gz"
test -s "$BK/mongodb.archive.gz"
(
 cd "$BK"
 sha256sum config.tar.gz docker-inspect.json mongodb.archive.gz > SHA256SUMS
 sha256sum -c SHA256SUMS
)
docker start nightscout >/dev/null
STOPPED=0
wait_healthy || { echo '[FEHLER] Nightscout nach Backup nicht gesund; kein Update.' >&2; exit 1; }

echo "[UPDATE] $OLD_VERSION -> $NEW_VERSION"
SWITCHED=1
"${COMPOSE[@]}" up -d --no-deps --pull never --force-recreate nightscout
wait_healthy || { echo '[FEHLER] API nach Update nicht gesund.' >&2; exit 1; }
ACTIVE_VERSION="$(docker exec nightscout node -p 'require("./package.json").version')"
[[ "$ACTIVE_VERSION" == "$NEW_VERSION" ]] || { echo '[FEHLER] Versionskontrolle fehlgeschlagen.' >&2; exit 1; }

COMPLETE=1
rm -f "$STATE/failed-image-id"
echo "[OK] Nightscout $ACTIVE_VERSION aktiv. Backup: $BK"
UPDATE

chmod 700 /usr/local/sbin/nightscout-weekly-update

cat > /etc/systemd/system/nightscout-weekly-update.service <<'SERVICE'
[Unit]
Description=Nightscout weekly patch updater with backup and rollback
Wants=network-online.target
After=network-online.target docker.service
Requires=docker.service

[Service]
Type=oneshot
ExecStart=/usr/local/sbin/nightscout-weekly-update
TimeoutStartSec=20min
SERVICE

cat > /etc/systemd/system/nightscout-weekly-update.timer <<'TIMER'
[Unit]
Description=Nightscout weekly update, Sunday 11 AM

[Timer]
OnCalendar=Sun *-*-* 11:00:00 Europe/Berlin
Persistent=false
Unit=nightscout-weekly-update.service

[Install]
WantedBy=timers.target
TIMER

systemctl daemon-reload

# Don't enable unattended changes while the previous HTTP 502 problem remains unresolved.
if curl -fsS --max-time 10 -o /dev/null http://127.0.0.1:1337/api/v1/status.json &&
   curl -fsS --max-time 10 -o /dev/null https://sweet.bopzocker.de/api/v1/status.json; then
  systemctl enable --now nightscout-weekly-update.timer
  echo '[OK] Wochen-Updater aktiviert, Sonntag 11:00 Uhr.'
else
  systemctl disable --now nightscout-weekly-update.timer 2>/dev/null || true
  echo '[STOP] API nicht gesund (evtl. 502). Updater installiert, aber TIMER NICHT AKTIVIERT.'
  echo 'Erst Fehler beheben, dann: systemctl enable --now nightscout-weekly-update.timer'
fi

systemctl list-timers nightscout-weekly-update.timer --no-pager