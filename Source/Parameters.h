#pragma once
#include "Common.h"

// Lista única de parámetros: genera los IDs, los punteros atómicos y se usa en el layout.
#define PH_PARAM_LIST(X) \
    X(inGain) X(outGain) X(mix) X(mDrive) X(limiter) X(stereo) X(bypass) \
    X(colorOn) X(cDrive) X(cTone) X(cSat) X(cBlend) X(cWarm) X(cBright) X(cHarm) \
    X(spectrumOn) X(sLow) X(sMid) X(sHigh) X(sRes) X(sFreq) X(sQ) X(sFilter) \
    X(motionOn) X(mWidth) X(mRate) X(mDepth) X(mPhase) X(mFeedback) X(mMod) X(mStereo) X(mMode) \
    X(glitchOn) X(gCrush) X(gRedux) X(gStutter) X(gNoise) X(gRate) X(gDigital) X(gMix) \
    X(gCrushOn) X(gReduxOn) X(gStutterOn) X(gNoiseOn) X(gDigitalOn)

namespace phaser
{
    namespace ids
    {
       #define X(n) inline constexpr const char* n = #n;
        PH_PARAM_LIST (X)
       #undef X
    }

    // Punteros a los valores "raw" (atómicos) de los parámetros: lectura segura en el hilo de audio.
    struct RawParams
    {
       #define X(n) std::atomic<float>* n = nullptr;
        PH_PARAM_LIST (X)
       #undef X
        void bind (juce::AudioProcessorValueTreeState& a)
        {
           #define X(n) n = a.getRawParameterValue (ids::n); jassert (n != nullptr);
            PH_PARAM_LIST (X)
           #undef X
        }
    };

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
}
