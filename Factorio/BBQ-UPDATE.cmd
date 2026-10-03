@echo off
setlocal EnableExtensions
chcp 65001 >nul
title BBQ CHAOS - Factorio Mod-Updater
rem Einzige lokal benoetigte Datei; Launcher v2 mit Anti-Cache und Syntaxpruefung.
rem Aufruf: Doppelklick = Download/Update; BBQ-UPDATE.cmd pruefen = nur pruefen.
set "BBQ_MODE=update"
if /I "%~1"=="pruefen" set "BBQ_MODE=check"
if /I "%~1"=="/check" set "BBQ_MODE=check"
if /I "%~1"=="test" set "BBQ_MODE=test"
if /I not "%~1"=="" if /I not "%~1"=="pruefen" if /I not "%~1"=="/check" if /I not "%~1"=="test" (
    echo Nutzung: BBQ-UPDATE.cmd [pruefen^|test]
    pause
    exit /b 2
)
echo ================================================
echo   BBQ CHAOS - Factorio Mod-Updater
echo ================================================
echo Quelle: github.com/Technox90/projekte/Factorio
echo Modus: %BBQ_MODE%
echo.
powershell.exe -NoProfile -ExecutionPolicy Bypass -Command "$ErrorActionPreference='Stop'; [Net.ServicePointManager]::SecurityProtocol=[Net.SecurityProtocolType]::Tls12; $url='https://raw.githubusercontent.com/Technox90/projekte/main/Factorio/BBQ-GitHub-Launcher-v2.ps1?nocache='+[guid]::NewGuid().ToString('N'); $file=Join-Path $env:TEMP 'BBQ-CHAOS-GitHub-Launcher-v2.ps1'; try {; Remove-Item -LiteralPath $file -Force -ErrorAction SilentlyContinue; Invoke-WebRequest -Uri $url -OutFile $file -UseBasicParsing -TimeoutSec 60 -Headers @{'Cache-Control'='no-cache';Pragma='no-cache'} -ErrorAction Stop; if ((Get-Item -LiteralPath $file).Length -lt 200) {throw 'GitHub-Launcher ist leer oder ungueltig.'}; $content=Get-Content -LiteralPath $file -Raw -Encoding UTF8; if (-not $content.StartsWith('# BBQ_LAUNCHER_VERSION=20261003_2')) {throw 'GitHub hat eine alte Launcher-Version geliefert.'}; $tokens=$null; $parseErrors=$null; [void][System.Management.Automation.Language.Parser]::ParseFile($file,[ref]$tokens,[ref]$parseErrors); if ($parseErrors.Count -gt 0) {throw ('PowerShell-Syntaxfehler: '+$parseErrors[0].Message)}; if ($env:BBQ_MODE -eq 'check') {& $file -CheckOnly} elseif ($env:BBQ_MODE -eq 'test') {& $file -TestParser} else {& $file}; if ($LASTEXITCODE -and $LASTEXITCODE -ne 0) {exit $LASTEXITCODE}; exit 0; } catch {Write-Host ('FEHLER: '+$_.Exception.Message) -ForegroundColor Red;exit 10}"
set "rc=%errorlevel%"
echo.
if not "%rc%"=="0" echo FEHLER %rc% - Bericht unter AppData\Roaming\Factorio\BBQ-CHAOS-Reports pruefen.
if "%rc%"=="0" echo Vorgang beendet. Siehe Ausgabe fuer eventuelle Warnungen.
pause
exit /b %rc%
