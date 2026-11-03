#pragma once
// Cabecera común del motor DSP. El DSP no depende de la GUI ni de juce::dsp.
#ifdef PHASER_TEST_SHIM
 #include "TestShim.h"      // solo para tests offline (ver Tests/)
#else
 #include <juce_core/juce_core.h>
 #include <juce_audio_basics/juce_audio_basics.h>
#endif
#include <algorithm>
#include <cmath>
#include <vector>

namespace phaser
{
    constexpr float kPi    = 3.14159265358979323846f;
    constexpr float kTwoPi = 6.28318530717958647692f;
    constexpr int   kChunk = 32;   // las muestras se procesan en bloques de 32 para actualizar coeficientes

    inline float sanitize (float x) noexcept { return std::isfinite (x) ? x : 0.0f; }
    inline float dbToGain (float db) noexcept { return std::pow (10.0f, db * 0.05f); }
}
