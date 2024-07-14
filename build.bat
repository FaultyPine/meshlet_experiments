@echo off

call "activate_dev.bat"

setlocal

:: unpack cmd line args
for %%a in (%*) do set "%%a=1"


if "%web%"=="1" (
    tup build-web
    goto :eof
)

if "%clean%"=="1" (
    rmdir build-desktop /q /s
    rmdir build-web /q /s
    goto :eof
)

tup build-desktop

endlocal
