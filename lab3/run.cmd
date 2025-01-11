@echo off
setlocal

set "folder=logger\build"
set "file=%folder%\logger.exe"

if exist "%folder%" (
    if exist "%file%" (
        "%file%" %*
    ) else (
        echo Error: File "logger.exe" does not exist in "%folder%"
        exit /b 1
    )
) else (
    echo Error: Folder "%folder%" does not exist.
    exit /b 1
)

endlocal
