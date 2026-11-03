#pragma once
#include "DSPCommon.h"
#include "Biquad.h"

namespace phaser
{
    struct ColorParams { float drive = 0.3f, tone = 0.5f, sat = 0.4f, blend = 0.5f, warm = 0.0f, bright = 0.0f, harm = 0.0f; };

    // MODULE 01 - COLOR: saturación soft-clip / waveshaping asimétrico (armónicos pares),
    // filtros de tono (tilt), calidez, brillo y mezcla en paralelo.
    class ColorEngine
    {
    public:
        void prepare (double sampleRate);
        void reset();
        void process (float* const* ch, int numCh, int n, const ColorParams& p);
    private:
        double sr = 44100.0;
        bool first = true;
        juce::SmoothedValue<float> sDrive, sTone, sSat, sBlend, sWarm, sBright, sHarm;
        BQ toneLo[2], toneHi[2], warmF[2], brightF[2];
        float dcX[2] {}, dcY[2] {};
    };
}
