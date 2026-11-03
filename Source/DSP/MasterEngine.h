#pragma once
#include "DSPCommon.h"

namespace phaser
{
    struct MasterParams { float drive = 0.0f, stereo = 1.0f, outGainDb = 0.0f; bool limiter = true; };

    // PHASER MASTER: drive suave con compensación, control estéreo M/S, ganancia de salida
    // y limitador de salida (attack rápido, release 80 ms, techo -0.5 dBFS) + recorte de seguridad.
    class MasterEngine
    {
    public:
        void prepare (double sampleRate);
        void reset();
        void process (float* const* ch, int numCh, int n, const MasterParams& p);
        float getReductionDb() const noexcept { return lastReductionDb; }   // negativo o 0
    private:
        double sr = 44100.0;
        bool first = true;
        float gain = 1.0f, lastReductionDb = 0.0f;
        juce::SmoothedValue<float> sDrive, sWidth, sOut, sCeil;
    };
}
