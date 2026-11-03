@echo off
cd /d "%~dp0"
echo Limpiando solo archivos generados de compilacion...
if exist build rmdir /s /q build
if exist "Builds\VST3" rmdir /s /q "Builds\VST3"
echo [OK] Eliminados: build\ y Builds\VST3\  ^(el codigo fuente y JUCE\ local no se tocan; tampoco los plugins ya instalados^)
