@echo off
chcp 65001 >nul
cd /d "%~dp0"
title BBQ CHAOS - Parser-Test
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0BBQ-GitHub-Launcher.ps1" -TestParser
set "rc=%errorlevel%"
echo.
if not "%rc%"=="0" echo Fehlercode: %rc% - Bericht unter AppData\Roaming\Factorio\BBQ-CHAOS-Reports pruefen.
pause
exit /b %rc%
