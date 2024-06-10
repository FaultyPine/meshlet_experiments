@echo off
setlocal

:: unpack cmd line args
for %%a in (%*) do set "%%a=1"


if "%web%"=="1" (
    emrun build-web/app.html
    goto :eof
)


pushd build-desktop
pushd build-desktop
app.exe
popd
popd


