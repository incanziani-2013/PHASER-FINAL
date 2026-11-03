@echo off
setlocal EnableExtensions
cd /d "%~dp0"
set "SRC=build\PHASER_artefacts\Release\VST3\PHASER.vst3"
if not exist "%SRC%" set "SRC=Builds\VST3\PHASER.vst3"
if not exist "%SRC%" goto :nosrc

net session >nul 2>&1
if errorlevel 1 goto :user
set "DEST=%CommonProgramFiles%\VST3"
goto :copy
:user
set "DEST=%LOCALAPPDATA%\Programs\Common\VST3"
echo [AVISO] Sin permisos de administrador: se instala para el usuario actual.
echo         Para instalar en Program Files, ejecuta INSTALL.bat como administrador.
:copy
if not exist "%DEST%" mkdir "%DEST%"
xcopy /E /I /Y /Q "%SRC%" "%DEST%\PHASER.vst3\" >nul
if errorlevel 1 goto :fail
echo [OK] PHASER instalado en:
echo      %DEST%\PHASER.vst3
echo No se borro ningun otro plugin. En FL Studio: Options ^> Manage plugins ^> Find installed plugins ^(Verify plugins^).
exit /b 0
:nosrc
echo [ERROR] No se encontro el VST3 compilado. Ejecuta BUILD.bat primero.
exit /b 1
:fail
echo [ERROR] Fallo la copia.
exit /b 1
