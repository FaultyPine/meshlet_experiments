@echo off
:: really don't like that we have to do these

WHERE emcc >nul 2>nul
IF NOT %ERRORLEVEL% == 0 (
    call emsdk.bat activate --permanent
)

:: if cl doesn't work, run vcvarsall
WHERE cl >nul 2>nul
IF NOT %ERRORLEVEL% == 0 (
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
)