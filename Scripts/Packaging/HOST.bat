@echo off
setlocal
cd /d "%~dp0"
if not exist "ArenaGame.exe" (
    echo ArenaGame.exe is missing. Keep HOST.bat beside the packaged game executable.
    pause
    exit /b 1
)
start "" "%~dp0ArenaGame.exe" /Game/Maps/MainMenu?listen -windowed -ResX=1280 -ResY=720 -ExecCmds="t.MaxFPS 60"
