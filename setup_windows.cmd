@echo off

set SCRIPT_DIR=%~dp0
set SCRIPT=setup_windows.ps1

net session >null 2>&1

if %errorlevel% neq 0 (
    echo Requesting Administrator privileges...
    PowerShell -NoProfile -ExecutionPolicy Bypass -Command "Start-Process PowerShell -ArgumentList '-NoProfile -ExecutionPolicy Bypass -File \"%SCRIPT_DIR%%SCRIPT%\"' -Verb RunAs"
    exit /b
)

PowerShell -NoExit -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT%"
