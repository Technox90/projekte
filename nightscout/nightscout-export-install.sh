#!/usr/bin/env bash
# Read-only Nightscout exporter for the existing Plesk/Docker host.
# Does not change Nightscout, MongoDB, Docker, AAPS, firewall or therapy settings.
set -Eeuo pipefail
ROOT=/opt/nightscout-export
TOKEN="$ROOT/config/read-token"
SVC=nightscout-exp

msg(){ printf '\n==> %s\n' "$*"; }
fail(){ echo "[FEHLER] $*" >&2; exit 1; }
trap 'echo "[FEHLER] Setup abgebrochen in Zeile $LINENO" >&2' ERR

[[ $EUID == 0 ]] || fail "Als root ausfuehren"
for bin in python3 systemctl runuser flock; do command -v "$bin" >/dev/null || fail "$bin fehlt"; done
[[ -s "$TOKEN" && ! -L "$TOKEN" ]] || fail "Nur-Lese-Token fehlt: $TOKEN"

msg "Eigener Dienstbenutzer und geschuetzte Ordner"
id "$SVC" >/dev/null 2>&1 || useradd -r -M -d /nonexistent -s /usr/sbin/nologin "$SVC"
install -d -o root -g "$SVC" -m 750 "$ROOT"
install -d -o root -g "$SVC" -m 750 "$ROOT/config"
chown root:"$SVC" "$TOKEN"; chmod 640 "$TOKEN"
install -d -o "$SVC" -g "$SVC" -m 700 "$ROOT/data"
chown -R "$SVC":"$SVC" "$ROOT/data"
find "$ROOT/data" -type d -exec chmod 700 '{}' +
find "$ROOT/data" -type f -exec chmod 600 '{}' +

msg "Exporter-Programm installieren"
cat > "$ROOT/exporter.py" <<'PY'
#!/usr/bin/env python3
"""Nightscout read-only local export; atomic compressed JSON per UTC day."""
import argparse, datetime as dt, gzip, json, os, pathlib, sys, tempfile, time
import urllib.error, urllib.parse, urllib.request

BASE = 'http://127.0.0.1:1337'
ROOT = pathlib.Path('/opt/nightscout-export')
UTC = dt.timezone.utc
EPOCH = dt.datetime(1970,1,1,tzinfo=UTC)
LIMIT = 500
RESOURCES = ('entries','treatments','devicestatus')
FIELDS = {'entries':'date','treatments':'created_at','devicestatus':'created_at'}

def ms_timestamp(value):
    if isinstance(value,(int,float)): return int(value)
    if str(value).isdigit(): return int(value)
    date = dt.datetime.fromisoformat(str(value).replace('Z','+00:00'))
    if date.tzinfo is None: date = date.replace(tzinfo=UTC)
    delta = date.astimezone(UTC)-EPOCH
    return (delta.days*86400+delta.seconds)*1000 + delta.microseconds//1000

def iso(ms):
    return (EPOCH+dt.timedelta(milliseconds=ms)).isoformat(timespec='milliseconds').replace('+00:00','Z')

class Client:
    def __init__(self):
        token = (ROOT/'config/read-token').read_text(encoding='utf-8').strip()
        if len(token)<10 or '/' in token or '://' in token:
            raise RuntimeError('Ungueltiger Nur-Lese-Token')
        self.token=token
        self.jwt=None
        self.auth_at=0

    def get(self,path,params=(),authenticated=True):
        url=BASE+path
        if params: url+='?'+urllib.parse.urlencode(params)
        for attempt in range(4):
            if authenticated and (not self.jwt or time.monotonic()-self.auth_at>600):
                self.auth()
            headers={'Accept':'application/json'}
            if authenticated: headers['Authorization']='Bearer '+self.jwt
            req=urllib.request.Request(url,headers=headers,method='GET')
            try:
                with urllib.request.urlopen(req,timeout=45) as response:
                    result=json.load(response)
                return result
            except urllib.error.HTTPError as exc:
                if exc.code==401 and authenticated and attempt<3:
                    self.jwt=None
                    continue
                if exc.code in (429,500,502,503,504) and attempt<3:
                    time.sleep(2**attempt)
                    continue
                raise RuntimeError(f'{path}: HTTP {exc.code}') from None
            except (urllib.error.URLError, TimeoutError):
                if attempt<3:
                    time.sleep(2**attempt)
                    continue
                raise RuntimeError(f'{path}: keine Verbindung') from None
        raise RuntimeError(f'{path}: Abfrage fehlgeschlagen')

    def auth(self):
        path='/api/v2/authorization/request/'+urllib.parse.quote(self.token,safe='')
        result=self.get(path,authenticated=False)
        if not isinstance(result,dict) or not result.get('token'):
            raise RuntimeError('Tokenaustausch fehlgeschlagen')
        self.jwt=result['token']
        self.auth_at=time.monotonic()

    def check(self):
        status=self.get('/api/v3/status')
        permissions=status.get('result',{}).get('apiPermissions',{}) if isinstance(status,dict) else {}
        for name in ('entries','treatments','profile','devicestatus'):
            if permissions.get(name)!='r':
                raise RuntimeError(f'{name}: Nur-Lese-Recht r fehlt')
        print('[OK] Token: nur Leserechte',flush=True)

    def range(self,name,start,end,depth=0):
        if end<=start: return []
        field=FIELDS[name]
        a,b=(str(start),str(end)) if name=='entries' else (iso(start),iso(end))
        rows=self.get('/api/v1/'+name+'.json',[
            ('count',str(LIMIT)),
            (f'find[{field}][$gte]',a),
            (f'find[{field}][$lt]',b)])
        if not isinstance(rows,list):
            raise RuntimeError(f'{name}: keine Liste von Datensaetzen')
        for row in rows:
            if not isinstance(row,dict) or field not in row:
                raise RuntimeError(f'{name}: Zeitstempel fehlt')
            try: stamp=ms_timestamp(row[field])
            except (ValueError,TypeError,OverflowError):
                raise RuntimeError(f'{name}: ungueltiger Zeitstempel') from None
            if not start<=stamp<end:
                raise RuntimeError(f'{name}: API ignoriert Zeitfilter')
        if len(rows)>=LIMIT:
            if end-start<=1 or depth>=30:
                raise RuntimeError(f'{name}: API-Limit mit gleichen Zeitstempeln')
            midpoint=start+(end-start)//2
            return unique(self.range(name,start,midpoint,depth+1)+self.range(name,midpoint,end,depth+1))
        return rows

def unique(rows):
    seen=set(); out=[]
    for row in rows:
        key=row.get('_id') or row.get('identifier')
        if not isinstance(key,str): key=json.dumps(row,sort_keys=True,ensure_ascii=False)
        if key not in seen: seen.add(key); out.append(row)
    return out

def save(path,records):
    path.parent.mkdir(mode=0o700,parents=True,exist_ok=True)
    os.chmod(path.parent,0o700)
    fd,temp=tempfile.mkstemp(prefix='.export-',suffix='.tmp',dir=path.parent)
    try:
        os.fchmod(fd,0o600)
        with os.fdopen(fd,'wb') as stream:
            with gzip.GzipFile(fileobj=stream,mode='wb',compresslevel=6,mtime=0) as zipped:
                zipped.write(json.dumps(records,ensure_ascii=False,separators=(',',':')).encode('utf-8'))
        os.replace(temp,path)
    finally:
        if os.path.exists(temp): os.unlink(temp)

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--days',type=int,default=3)
    args=parser.parse_args()
    if not 1<=args.days<=366: parser.error('--days muss zwischen 1 und 366 liegen')
    os.umask(0o077)
    client=Client(); client.check()
    today=dt.datetime.now(UTC).date()
    totals={name:0 for name in RESOURCES}
    for offset in range(args.days-1,-1,-1):
        day=today-dt.timedelta(days=offset)
        start=int((dt.datetime.combine(day,dt.time.min,UTC)-EPOCH).total_seconds()*1000)
        counts={}
        for name in RESOURCES:
            rows=unique(client.range(name,start,start+86400000))
            save(ROOT/'data'/name/(day.isoformat()+'.json.gz'),rows)
            counts[name]=len(rows)
            totals[name]+=len(rows)
        print('[OK] '+day.isoformat()+' UTC: '+', '.join(f'{n}={v}' for n,v in counts.items()),flush=True)
    profiles=client.get('/api/v1/profile.json',[('count','1000')])
    if not isinstance(profiles,list) or len(profiles)>=1000:
        raise RuntimeError('Profile unvollstaendig oder API-Limit erreicht')
    save(ROOT/'data'/'profile'/'all.json.gz',profiles)
    print('[OK] Profile='+str(len(profiles)),flush=True)
    print('[FERTIG] '+', '.join(f'{n}={v}' for n,v in totals.items()),flush=True)

if __name__=='__main__':
    try: main()
    except Exception as exc:
        print('[FEHLER] '+str(exc),file=sys.stderr,flush=True)
        sys.exit(1)
PY
chown root:root "$ROOT/exporter.py"
chmod 644 "$ROOT/exporter.py"
python3 -m py_compile "$ROOT/exporter.py"

msg "1-Tag-Test mit echtem Nur-Lese-Token"
runuser -u "$SVC" -- python3 "$ROOT/exporter.py" --days 1 || fail "Test fehlgeschlagen: Kein Timer aktiviert"

msg "systemd-Dienste installieren"
cat > /etc/systemd/system/nightscout-export.service <<'UNIT'
[Unit]
Description=Nightscout stundenweiser Nur-Lese-Export
Wants=network-online.target
After=network-online.target
[Service]
Type=oneshot
User=nightscout-exp
Group=nightscout-exp
UMask=0077
WorkingDirectory=/opt/nightscout-export
ExecStart=/usr/bin/flock -n -E 0 /opt/nightscout-export/data/.export.lock /usr/bin/python3 /opt/nightscout-export/exporter.py --days 3
NoNewPrivileges=yes
PrivateTmp=yes
ProtectSystem=strict
ReadWritePaths=/opt/nightscout-export/data
ProtectHome=yes
ProtectKernelTunables=yes
ProtectKernelModules=yes
ProtectControlGroups=yes
CapabilityBoundingSet=
RestrictAddressFamilies=AF_INET AF_UNIX
TimeoutStartSec=0
UNIT
cat > /etc/systemd/system/nightscout-export-backfill.service <<'UNIT'
[Unit]
Description=Nightscout einmaliger 90-Tage-Export
Wants=network-online.target
After=network-online.target
[Service]
Type=oneshot
User=nightscout-exp
Group=nightscout-exp
UMask=0077
WorkingDirectory=/opt/nightscout-export
ExecStart=/usr/bin/flock -n -E 0 /opt/nightscout-export/data/.export.lock /usr/bin/python3 /opt/nightscout-export/exporter.py --days 90
NoNewPrivileges=yes
PrivateTmp=yes
ProtectSystem=strict
ReadWritePaths=/opt/nightscout-export/data
ProtectHome=yes
ProtectKernelTunables=yes
ProtectKernelModules=yes
ProtectControlGroups=yes
CapabilityBoundingSet=
RestrictAddressFamilies=AF_INET AF_UNIX
TimeoutStartSec=0
UNIT
cat > /etc/systemd/system/nightscout-export.timer <<'UNIT'
[Unit]
Description=Stundenweiser Nightscout-Export
[Timer]
OnCalendar=hourly
Persistent=true
AccuracySec=5min
Unit=nightscout-export.service
[Install]
WantedBy=timers.target
UNIT
systemctl daemon-reload
systemctl enable --now nightscout-export.timer
systemctl start --no-block nightscout-export-backfill.service

msg "Fertig. Der Erstexport ist gestartet und der Timer aktiviert."
echo "journalctl -u nightscout-export-backfill.service -n 50 --no-pager"
echo "systemctl list-timers --all nightscout-export.timer"
echo "Daten: $ROOT/data (niemals in GitHub speichern)"
