@echo off
setlocal EnableExtensions
chcp 65001 >nul
title BBQ CHAOS - Factorio Mod-Updater
rem Einzige lokal benoetigte Datei: aktuellsten GitHub-Launcher sicher laden.
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
powershell.exe -NoProfile -ExecutionPolicy Bypass -Command "$ErrorActionPreference='Stop'; [Net.ServicePointManager]::SecurityProtocol=[Net.SecurityProtocolType]::Tls12; $url='https://raw.githubusercontent.com/Technox90/projekte/main/Factorio/BBQ-GitHub-Launcher.ps1'; $file=Join-Path $env:TEMP 'BBQ-CHAOS-GitHub-Launcher.ps1'; try { Invoke-WebRequest -Uri $url -OutFile $file -UseBasicParsing -TimeoutSec 60 -ErrorAction Stop; if ((Get-Item -LiteralPath $file).Length -lt 200) { throw 'GitHub-Launcher ungueltig.' }; if ($env:BBQ_MODE -eq 'check') { & $file -CheckOnly } elseif ($env:BBQ_MODE -eq 'test') { & $file -TestParser } else { & $file }; if (-not $?) { exit 1 }; if ($LASTEXITCODE -and $LASTEXITCODE -ne 0) { exit $LASTEXITCODE }; exit 0 } catch { Write-Host ('FEHLER: ' + $_.Exception.Message) -ForegroundColor Red; exit 10 }"
set "rc=%errorlevel%"
echo.
if not "%rc%"=="0" echo FEHLER %rc% - Bericht unter AppData\Roaming\Factorio\BBQ-CHAOS-Reports pruefen.
if "%rc%"=="0" echo Vorgang beendet. Siehe Ausgabe fuer eventuelle Warnungen.
pause
exit /b %rc%
