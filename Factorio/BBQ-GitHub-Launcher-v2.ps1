# BBQ_LAUNCHER_VERSION=20261003_2
# BBQ CHAOS Factorio – aktuelle Liste und unveraenderten Core direkt von GitHub
[CmdletBinding()]
param([switch]$CheckOnly,[switch]$TestParser)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
[Net.ServicePointManager]::SecurityProtocol=[Net.SecurityProtocolType]::Tls12
$root='https://raw.githubusercontent.com/Technox90/projekte/main/Factorio'
$work=Join-Path $env:TEMP 'BBQ-CHAOS-GitHub-Download'
New-Item -ItemType Directory -Force -Path $work | Out-Null
try {
  foreach($name in @('mod-list.json','version-pins.json','BBQ-ModDownloader-core.ps1.gz.b64')){
    # Jede Anfrage hat eine neue URL: keine veralteten GitHub-CDN-Antworten.
    $url="$root/$name" + "?nocache=$([guid]::NewGuid().ToString('N'))"
    $dest=Join-Path $work $name
    Write-Host "GitHub: $name"
    Remove-Item -LiteralPath $dest -Force -ErrorAction SilentlyContinue
    Invoke-WebRequest -Uri $url -OutFile $dest -UseBasicParsing -TimeoutSec 60 -Headers @{'Cache-Control'='no-cache';Pragma='no-cache'} -ErrorAction Stop | Out-Null
    if(-not(Test-Path -LiteralPath $dest) -or (Get-Item -LiteralPath $dest).Length -lt 2){
      throw "GitHub-Datei ungueltig: $name"
    }
  }
  $listFile=Join-Path $work 'mod-list.json'
  $list=Get-Content -LiteralPath $listFile -Raw -Encoding UTF8 | ConvertFrom-Json
  # Variable Anzahl zulassen, damit spaetere Modlisten-Aenderungen ohne
  # Neuverteilung der CMD-Datei moeglich bleiben.
  if(-not $list -or -not $list.mods){throw 'GitHub-Modliste fehlt oder ist leer.'}
  $mods=@($list.mods)
  if($mods.Count -lt 4){throw 'GitHub-Modliste ist unplausibel klein.'}
  $seen=@{}
  foreach($mod in $mods){
    if(-not $mod.name -or ($mod.enabled -isnot [bool])){throw 'Ungueltiger Mod-Eintrag (name/enabled).'}
    if($seen.ContainsKey([string]$mod.name)){throw "Doppelter Mod-Eintrag: $($mod.name)"}
    $seen[[string]$mod.name]=$mod.enabled
  }
  if(-not $seen.ContainsKey('base') -or -not $seen['base']){throw 'Factorio Base muss aktiviert sein.'}
  $enabled=@($mods | Where-Object {$_.enabled -eq $true})
  if($enabled.Count -lt 1){throw 'Keine aktiven Mods in der GitHub-Liste.'}
  $pin=Get-Content -LiteralPath (Join-Path $work 'version-pins.json') -Raw -Encoding UTF8 | ConvertFrom-Json
  if($null -eq $pin){throw 'GitHub-Versionsvorgaben fehlen.'}
  foreach($p in @($pin.PSObject.Properties)){
    if(-not $seen.ContainsKey($p.Name) -or -not $seen[$p.Name]){throw "Version-Pin fuer nicht aktiven Mod: $($p.Name)"}
    if([string]$p.Value -notmatch '^[0-9]+[.][0-9]+[.][0-9]+$'){throw "Ungueltiger Version-Pin: $($p.Name)"}
  }
  $b64=[IO.File]::ReadAllText((Join-Path $work 'BBQ-ModDownloader-core.ps1.gz.b64')).Trim()
  $raw=[Convert]::FromBase64String($b64)
  $mem=[IO.MemoryStream]::new($raw)
  $gz=[IO.Compression.GZipStream]::new($mem,[IO.Compression.CompressionMode]::Decompress)
  $scriptPath=Join-Path $work 'BBQ-ModDownloader.ps1'
  try {
    $out=[IO.File]::Create($scriptPath)
    try{$gz.CopyTo($out)}finally{$out.Dispose()}
  } finally {$gz.Dispose();$mem.Dispose()}
  $expected='c029f896eb20fb81023e32266ebda0689d2b3c96004eb9cd300b43811e7b0405'
  $actual=(Get-FileHash -LiteralPath $scriptPath -Algorithm SHA256).Hash
  if($actual -ine $expected){throw 'Pruefsumme des Downloaders ist falsch. Abbruch!'}
  Write-Host ("BBQ CHAOS | GitHub geladen: {0} Mods, {1} aktiv" -f @($list.mods).Count,$enabled.Count)
  if($TestParser){ & $scriptPath -ModList $listFile -TestParser }
  elseif($CheckOnly){ & $scriptPath -ModList $listFile -CheckOnly }
  else{
    # Archive known obsolete local test mods WITHOUT editing ZIP contents.
    # Disabled Angel fork 0.0.14 is also removed; portal download cannot activate it.
    $modsDir=Join-Path $env:APPDATA 'Factorio\mods'
    if(Test-Path -LiteralPath $modsDir){
      $oldMods=@(Get-ChildItem -LiteralPath $modsDir -Force |
        Where-Object { $_.Name -like 'bbq-chaos-belt-compat_*' -or
          $_.Name -eq 'angelsaddons-space-age-revived_0.0.14.zip' })
      if($oldMods.Count -gt 0){
        $archive=Join-Path $env:APPDATA ('Factorio\BBQ-CHAOS-Backups\obsolete-'+(Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
        New-Item -ItemType Directory -Force -Path $archive | Out-Null
        foreach($item in $oldMods){
          Move-Item -LiteralPath $item.FullName -Destination (Join-Path $archive $item.Name) -ErrorAction Stop
          Write-Host "BBQ: Nicht aktives Testarchiv gesichert: $($item.Name)"
        }
        Write-Host "BBQ: Archivierte Testmods: $archive"
      }
    }
    # Unmodified official modportal archives ONLY. No ZIP edits or custom patches.
    # Keep the core in a separate process so its 'exit' cannot bypass error checking.
    Write-Host "BBQ: Lade unveraenderte Original-Mods aus dem Factorio-Modportal."
    & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $scriptPath -ModList $listFile -DownloadAvailable
    $downloadCode=[int]$LASTEXITCODE
    if($downloadCode -ne 0){
      throw "Download unvollstaendig (Code $downloadCode). Pruefe Report; keine Mods veraendern."
    }
    Write-Host "BBQ: Originalarchive geladen. Kein ZIP-Patching." -ForegroundColor Green
  }
} catch {
  Write-Host ("FEHLER: GitHub-Downloader nicht gestartet: "+$_.Exception.Message) -ForegroundColor Red
  exit 10
}