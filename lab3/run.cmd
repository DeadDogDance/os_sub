@echo off
setlocal

set "program=logger.exe"
set "folder=build"
set "file=%folder%\%program%"

if exist "%folder%" (
    if exist "%file%" (
        "%file%" %*
    ) else (
        echo Error: File %program% does not exist in %folder%"
        exit /b 1
    )
) else (
    echo Error: Folder %folder% does not exist.
    exit /b 1
)

endlocal
