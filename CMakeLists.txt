cmake_minimum_required(VERSION 3.22)
project(PHASER VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Runtime estático en MSVC: el VST3 no necesita el redistribuible de Visual C++
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# JUCE: si existe la carpeta JUCE/ junto a este archivo se usa esa; si no, se descarga la versión fija desde el repo oficial.
set(PHASER_JUCE_TAG "8.0.4" CACHE STRING "Tag de JUCE (github.com/juce-framework/JUCE)")
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    add_subdirectory(JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG ${PHASER_JUCE_TAG}
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(PHASER
    COMPANY_NAME "PHASER Audio"
    PRODUCT_NAME "PHASER"
    VERSION 1.0.0
    BUNDLE_ID com.phaseraudio.phaser
    PLUGIN_MANUFACTURER_CODE Phzr
    PLUGIN_CODE Phsr
    FORMATS VST3
    VST3_CATEGORIES Fx
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(PHASER PRIVATE
    Source/PluginProcessor.cpp Source/PluginEditor.cpp Source/Parameters.cpp
    Source/DSP/ColorEngine.cpp Source/DSP/SpectrumEngine.cpp Source/DSP/MotionEngine.cpp
    Source/DSP/GlitchEngine.cpp Source/DSP/MasterEngine.cpp
    Source/UI/PhaserLookAndFeel.cpp Source/UI/CustomKnob.cpp Source/UI/ModulePanel.cpp Source/UI/CentralVisualizer.cpp
    Source/Presets/PresetManager.cpp)

target_include_directories(PHASER PRIVATE Source)

target_compile_definitions(PHASER PUBLIC
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_STRICT_REFCOUNTEDPOINTER=1)

target_link_libraries(PHASER
    PRIVATE juce::juce_audio_utils juce::juce_dsp
    PUBLIC  juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

# Test offline del DSP (opcional): cmake -DPHASER_BUILD_TESTS=ON
option(PHASER_BUILD_TESTS "Compilar tests offline del DSP" OFF)
if(PHASER_BUILD_TESTS)
    add_executable(phaser_dsp_test Tests/dsp_test.cpp
        Source/DSP/ColorEngine.cpp Source/DSP/SpectrumEngine.cpp Source/DSP/MotionEngine.cpp
        Source/DSP/GlitchEngine.cpp Source/DSP/MasterEngine.cpp)
    target_compile_definitions(phaser_dsp_test PRIVATE PHASER_TEST_SHIM)
    target_include_directories(phaser_dsp_test PRIVATE Tests Source/DSP)
endif()
