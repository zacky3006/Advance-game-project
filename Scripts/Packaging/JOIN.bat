@echo off
setlocal
cd /d "%~dp0"
if not exist "ArenaGame.exe" (
    echo ArenaGame.exe is missing. Keep JOIN.bat beside the packaged game executable.
    pause
    exit /b 1
)
rem Same computer: double-click JOIN.bat. LAN: JOIN.bat 192.168.1.10
set "JOIN_ADDRESS=127.0.0.1"
if not "%~1"=="" set "JOIN_ADDRESS=%~1"
start "" "%~dp0ArenaGame.exe" "%JOIN_ADDRESS%" -windowed -ResX=1280 -ResY=720 -ExecCmds="t.MaxFPS 60"
