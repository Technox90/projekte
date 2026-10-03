# BBQ_ANGELS_PATCH_VERSION=20261003_1
# BBQ CHAOS: match the proven working AMP hotfix for Angel's Space Age Revived 0.0.14.
# Always run AFTER the official Factorio mod-portal download and SHA1 verification.
[CmdletBinding()]
param([string]$ModsDir=(Join-Path $env:APPDATA 'Factorio\mods'))
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem

$name='angelsaddons-space-age-revived_0.0.14'
$target=Join-Path $ModsDir ($name+'.zip')
if(-not(Test-Path -LiteralPath $target)){
  throw "Angel's Space Age Revived 0.0.14 fehlt im Modordner: $target"
}

# The Linux server ZIP was hotfixed with these exact TWO replacements.
# Strict verification prevents accidentally patching an unknown upstream build.
$edits=@(
  [pscustomobject]@{
    Path="$name/data.lua";
    Old='OV.global_replace_item("turbo-transport-belt", "bob-turbo-transport-belt")';
    New='-- Disabled: obsolete Bob turbo-belt mapping'
  },
  [pscustomobject]@{
    Path="$name/data-updates.lua";
    Old='bobmods.lib.recipe.replace_ingredient("loader-mini5", "bob-turbo-transport-belt", "bob-ultimate-transport-belt")';
    New='bobmods.lib.recipe.replace_ingredient("loader-mini5", "turbo-transport-belt", "bob-ultimate-transport-belt")'
  }
)
$utf8=[Text.UTF8Encoding]::new($false)
$replaceCount=0
$alreadyCount=0
$tmp=Join-Path $ModsDir ('.'+$name+'.bbq-patch.tmp')
if(Test-Path -LiteralPath $tmp){ Remove-Item -LiteralPath $tmp -Force }
try{
  Copy-Item -LiteralPath $target -Destination $tmp -ErrorAction Stop
  $zip=[IO.Compression.ZipFile]::Open($tmp,[IO.Compression.ZipArchiveMode]::Update)
  try{
    foreach($edit in $edits){
      $entry=$zip.GetEntry($edit.Path)
      if($null -eq $entry){throw "ZIP-Datei fehlt: $($edit.Path)"}
      $entryName=$entry.FullName
      $reader=[IO.StreamReader]::new($entry.Open(),$utf8,$true)
      try{$body=$reader.ReadToEnd()}finally{$reader.Dispose()}
      if($body.Contains($edit.Old)){
        if($body.Split(@($edit.Old),[StringSplitOptions]::None).Count -ne 2){
          throw "Legacy-Text kommt mehrfach vor: $entryName"
        }
        $changed=$body.Replace($edit.Old,$edit.New)
        $entry.Delete()
        $newEntry=$zip.CreateEntry($entryName,[IO.Compression.CompressionLevel]::Optimal)
        $writer=[IO.StreamWriter]::new($newEntry.Open(),$utf8)
        try{$writer.Write($changed)}finally{$writer.Dispose()}
        $replaceCount++
      }elseif($body.Contains($edit.New)){
        $alreadyCount++
      }else{
        throw "Unerwarteter Inhalt in $entryName; kein Eingriff"
      }
    }
  }finally{$zip.Dispose()}
  if($replaceCount -eq 0){
    Write-Host "BBQ Angel's Revived: beide AMP-Korrekturen bereits vorhanden." -ForegroundColor Green
    return
  }
  if(($replaceCount+$alreadyCount) -ne 2){throw "Patch unvollstaendig"}
  $verify=[IO.Compression.ZipFile]::OpenRead($tmp)
  try{
    foreach($edit in $edits){
      $entry=$verify.GetEntry($edit.Path)
      if($null -eq $entry){throw "ZIP-Pruefung: Datei fehlt"}
      $reader=[IO.StreamReader]::new($entry.Open(),$utf8,$true)
      try{$body=$reader.ReadToEnd()}finally{$reader.Dispose()}
      if(-not $body.Contains($edit.New) -or $body.Contains($edit.Old)){
        throw "ZIP-Pruefung fehlgeschlagen: $($edit.Path)"
      }
    }
  }finally{$verify.Dispose()}
  $backupDir=Join-Path (Split-Path -Parent $ModsDir) 'BBQ-CHAOS-Backups'
  New-Item -ItemType Directory -Path $backupDir -Force | Out-Null
  $backup=Join-Path $backupDir ("$name-before-patch-$(Get-Date -Format 'yyyyMMdd-HHmmss-fff').zip")
  [IO.File]::Replace($tmp,$target,$backup)
  Write-Host "BBQ Angel's Revived 0.0.14: $replaceCount Quelldatei(en) identisch zum AMP-Server korrigiert." -ForegroundColor Green
  Write-Host "ZIP-Originalbackup: $backup"
}finally{
  if(Test-Path -LiteralPath $tmp){Remove-Item -LiteralPath $tmp -Force}
}
