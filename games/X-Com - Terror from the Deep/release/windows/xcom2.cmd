@echo off
cd /D "%~dp0"
set SDL_STDIO_REDIRECT=0
SR-Geoscape.exe "0"
if errorlevel 2 goto begin
goto end
:begin
SR-Tactical.exe "1"
:geo
SR-Geoscape.exe "1"
if errorlevel 2 goto begin
:end
