@echo off
setlocal

set "folder=p_launcher\build"
set "file=%folder%\p_t.exe"

if exist "%folder%" (
    if exist "%file%" (
        "%file%" %*
    ) else (
        echo Error: File "p_t.exe" does not exist in "%folder%"
        exit /b 1
    )
) else (
    echo Error: Folder "%folder%" does not exist.
    exit /b 1
)

endlocal
