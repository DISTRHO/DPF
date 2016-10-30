@echo off
IF NOT EXIST bin GOTO NOBINDIR
CD bin
SET GEN=%~dp0\lv2_ttl_generator.exe
SET EXT=dll
for /D %%s in (.\*) do (call :generatettls "%%s")


goto :eof
:NOBINDIR
echo "Please run this script from the surface root"
goto :eof

:generatettls
pushd %1
for %%i in (*_dsp.dll) do %~dp0\lv2_ttl_generator.exe %%i
popd
goto :eof
