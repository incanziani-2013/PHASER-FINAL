#pragma once
#include "DSPCommon.h"
#include "Biquad.h"

namespace phaser
{
    // filter: 0 Off, 1 LowPass, 2 HighPass, 3 BandPass, 4 Peak, 5 LowShelf, 6 HighShelf
    struct SpectrumParams { float low = 0, mid = 0, high = 0, res = 0.3f, freq = 1000.0f, q = 1.0f; int filter = 0; };

    // MODULE 02 - SPECTRUM: EQ de 3 bandas (shelf grave / peaking / shelf agudo) + etapa de filtro seleccionable.
    // Los cambios de tipo de filtro hacen crossfade de 512 muestras para evitar clicks.
    class SpectrumEngine
    {
    public:
        void prepare (double sampleRate);
        void reset();
        void process (float* const* ch, int numCh, int n, const SpectrumParams& p);
    private:
        static constexpr int kFade = 512;
        double sr = 44100.0;
        bool first = true;
        juce::SmoothedValue<float> sLow, sMid, sHigh, sRes, sQ;
        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> sFreq;
        BQ lowB[2], midB[2], highB[2], cur[2], prev[2];
        int curType = 0, prevType = 0, fadeLeft = 0;
    };
}
