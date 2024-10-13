@echo off
setlocal enabledelayedexpansion

if "%1"=="pull" (
    echo Pulling...
    git pull -v origin main
) else if "%1"=="fetch" (
    echo Fetching...
    git fetch -v
) else (
    echo No choice provided. Fetching and pulling...
    git fetch -v
    git pull -v origin main
)

endlocal
