# PHASER V1 PRO — Advanced Color Processor (VST3)

Plugin de efectos VST3 (Windows 10/11, 64-bit) hecho con C++17 + JUCE 8. Cuatro módulos en serie
(COLOR → SPECTRUM → MOTION → GLITCH) + sección MASTER, interfaz vectorial propia y 20 presets de fábrica.

## Estado honesto del proyecto
| Parte | Estado |
|---|---|
| Motor DSP (`Source/DSP`) | Escrito y **probado offline** con `Tests/dsp_test.cpp` (31/31 comprobaciones, resultado en `Tests/last_run.txt`). Se probó con un shim mínimo de JUCE en Linux/g++, **no** dentro de JUCE real. |
| Procesador, parámetros, presets, GUI | Escritos completos, **nunca compilados ni ejecutados** (no había JUCE, CMake, Windows ni red en el entorno de desarrollo). Es probable que el primer build muestre algún error de compilación menor; pegá el mensaje y se corrige. |
| VST3 compilado | **No incluido.** Se genera con `BUILD.bat` en tu PC. |
| FL Studio | **No probado.** Automatización, guardado de estado y presets están implementados según la API de JUCE pero sin verificar en el host. |

## Requisitos
- Windows 10/11 x64
- Visual Studio 2019/2022 o *Build Tools* con "Desarrollo para el escritorio con C++"
- CMake 3.22+ (en el PATH)
- Git (para descargar JUCE 8.0.4 automáticamente) — o clonar JUCE manualmente en `.\JUCE`
- Licencia: JUCE es AGPLv3/comercial. Si distribuís PHASER, revisá https://juce.com/juce-8-licence/

## Compilar e instalar
```
BUILD.bat      :: configura, compila Release, deja el VST3 en build\PHASER_artefacts\Release\VST3\PHASER.vst3 y Builds\VST3\
INSTALL.bat    :: copia a %CommonProgramFiles%\VST3 (admin) o a %LOCALAPPDATA%\Programs\Common\VST3
CLEAN.bat      :: borra solo build\ y Builds\VST3\
```
En FL Studio: *Options → Manage plugins → Find installed plugins* (con "Verify plugins"). Aparece como **PHASER**.

## Módulos
- **COLOR**: Drive, Tone, Saturation, Blend, Warmth, Brightness, Harmonics — saturación tanh con sesgo asimétrico (armónicos pares), bloqueador de DC, tilt/shelves, mezcla paralela.
- **SPECTRUM**: Low/Mid/High (shelf 120 Hz / peak en FREQUENCY con Q / shelf 6 kHz) + filtro Off/LP/HP/BP/Peak/LowShelf/HighShelf con Resonance; crossfade al cambiar de filtro.
- **MOTION**: Chorus o Flanger (MODE), Rate, Depth, Phase L/R, Feedback, Modulation, Stereo (feedback cruzado) y Width M/S (el mono no cambia con Width).
- **GLITCH**: Crush (bits), Redux (sample-rate), Stutter (loop de buffer sincronizado al tempo del host, RATE 1/1…1/32), Noise (gateado por envolvente), Digital (clip duro), Mix; cada efecto con botón propio.
- **MASTER**: Input, Output, Master Mix, Master Drive, Stereo, Limiter (techo −0,5 dBFS + recorte de seguridad), Bypass global con fundido, medidores IN/OUT con GR.

Todos los cambios de parámetros y on/off de módulos se suavizan (sin clicks). Latencia: 0 muestras.

## Interfaz
1100×720, redimensionable (relación fija). Doble clic en un knob = reset; doble clic en su valor = escribir un número. Tooltips. Esfera central reactiva (FFT en el hilo de mensajes, datos del audio vía FIFO lock-free).

## Presets
20 de fábrica (categorías Atmosphere, Distortion, Glitch, Motion, Texture, Lead, Bass, Vocal, Init). Guardar/Cargar/Eliminar de usuario: `%APPDATA%\PHASER\Presets\*.phpreset` (XML). El estado completo (parámetros, módulo activo, preset) se guarda en el proyecto de FL.

## Estructura
```
CMakeLists.txt  BUILD.bat  INSTALL.bat  CLEAN.bat  TODO.md
Source/  PluginProcessor|Editor, Parameters, DSP/, UI/, Presets/
Tests/   dsp_test.cpp, TestShim.h, last_run.txt
```
Test del DSP en cualquier PC con g++: `g++ -std=c++17 -O2 -DPHASER_TEST_SHIM -ITests -ISource/DSP Tests/dsp_test.cpp Source/DSP/*.cpp -o dsp_test`

## Limitaciones conocidas (v1)
- Sin oversampling en las etapas no lineales (puede haber aliasing con drive alto).
- Control MIDI: PHASER no recibe MIDI; en FL se mapea con *Link to controller* sobre los parámetros automatizables.
- El limitador no es true-peak ni usa lookahead (latencia 0).
- El stutter corre libre respecto al tempo (no alineado a la posición de compás).
