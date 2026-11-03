@echo off
setlocal EnableExtensions
cd /d "%~dp0"
echo ==========================================
echo   PHASER V1 PRO - BUILD (Release x64)
echo ==========================================

where cmake >nul 2>&1
if errorlevel 1 goto :nocmake
for /f "delims=" %%v in ('cmake --version ^| findstr /b "cmake"') do echo [OK] %%v

if exist "JUCE\CMakeLists.txt" goto :havejuce
where git >nul 2>&1
if errorlevel 1 goto :nogit
echo [OK] git encontrado: se descargara JUCE desde github.com/juce-framework/JUCE
goto :findvs
:havejuce
echo [OK] Usando JUCE local en .\JUCE

:findvs
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" goto :novs
set "VSPATH="
for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSPATH=%%i"
if not defined VSPATH goto :novs
set "VSVER="
for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationVersion`) do set "VSVER=%%i"
for /f "tokens=1 delims=." %%a in ("%VSVER%") do set "VSMAJOR=%%a"
set "GEN="
if "%VSMAJOR%"=="16" set "GEN=Visual Studio 16 2019"
if "%VSMAJOR%"=="17" set "GEN=Visual Studio 17 2022"
if "%VSMAJOR%"=="18" set "GEN=Visual Studio 18 2026"
if not defined GEN goto :badvs
echo [OK] Visual Studio / Build Tools: %VSPATH%  ^(generador: %GEN%^)

echo.
echo [1/2] Configurando...
cmake -S . -B build -G "%GEN%" -A x64
if errorlevel 1 goto :fail

echo.
echo [2/2] Compilando Release...
cmake --build build --config Release --parallel
if errorlevel 1 goto :fail

set "OUT=build\PHASER_artefacts\Release\VST3\PHASER.vst3"
if not exist "%OUT%" goto :noout
if not exist "Builds\VST3" mkdir "Builds\VST3"
xcopy /E /I /Y /Q "%OUT%" "Builds\VST3\PHASER.vst3\" >nul
echo.
echo [LISTO] VST3 generado en:
echo     %~dp0%OUT%
echo     %~dp0Builds\VST3\PHASER.vst3
echo Ejecuta INSTALL.bat para instalarlo.
exit /b 0

:nocmake
echo [ERROR] CMake no encontrado. Instala CMake 3.22+ desde https://cmake.org/download/ y marca "Add to PATH".
goto :fail
:nogit
echo [ERROR] Git no encontrado y no existe la carpeta JUCE\. Instala Git (https://git-scm.com) o clona JUCE en .\JUCE
goto :fail
:novs
echo [ERROR] No se encontro Visual Studio ni Build Tools con el componente "Desktop development with C++".
echo         Descarga "Build Tools for Visual Studio" de https://visualstudio.microsoft.com/downloads/
goto :fail
:badvs
echo [ERROR] Version de Visual Studio no reconocida: %VSVER%
goto :fail
:noout
echo [ERROR] La compilacion termino pero no se encontro %OUT%
:fail
echo.
echo BUILD FALLIDO. Revisa los mensajes anteriores.
exit /b 1
