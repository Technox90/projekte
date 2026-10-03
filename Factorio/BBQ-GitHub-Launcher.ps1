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
    $url="$root/$name"
    $dest=Join-Path $work $name
    Write-Host "GitHub: $name"
    # Bei HTTP-Fehler kein stiller Fallback auf alte Daten.
    Invoke-WebRequest -Uri $url -OutFile $dest -UseBasicParsing -TimeoutSec 60 -ErrorAction Stop | Out-Null
    if(-not(Test-Path -LiteralPath $dest) -or (Get-Item -LiteralPath $dest).Length -lt 2){
      throw "GitHub-Datei ungueltig: $name"
    }
  }
  $listFile=Join-Path $work 'mod-list.json'
  $list=Get-Content -LiteralPath $listFile -Raw -Encoding UTF8 | ConvertFrom-Json
  if(@($list.mods).Count -ne 110){throw 'GitHub-Modliste hat unerwartete Anzahl von Mods.'}
  $enabled=@($list.mods | Where-Object {$_.enabled -eq $true})
  if($enabled.Count -ne 100){throw 'GitHub-Modliste hat unerwartete Anzahl aktiver Mods.'}
  $pin=Get-Content -LiteralPath (Join-Path $work 'version-pins.json') -Raw -Encoding UTF8 | ConvertFrom-Json
  if(@($pin.PSObject.Properties).Count -ne 6){throw 'GitHub-Versionsvorgaben unvollstaendig.'}
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
  else{ & $scriptPath -ModList $listFile -DownloadAvailable }
} catch {
  Write-Host ("FEHLER: GitHub-Downloader nicht gestartet: "+$_.Exception.Message) -ForegroundColor Red
  exit 10
}
