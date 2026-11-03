#pragma once
#include "DSPCommon.h"

namespace phaser
{
    // rate: 0=1/1 1=1/2 2=1/4 3=1/8 4=1/16 5=1/32
    struct GlitchParams
    {
        float crush = 0.5f, redux = 0.3f, stutter = 0.3f, noise = 0.1f, digital = 0.3f, mix = 0.5f;
        int rate = 2;
        bool crushOn = true, reduxOn = true, stutterOn = true, noiseOn = true, digitalOn = true;
        double bpm = 120.0;
    };

    // MODULE 04 - GLITCH: stutter (repetición de buffer), reducción de sample rate, bitcrusher,
    // distorsión digital y ruido gateado por la envolvente. Cada etapa se activa/desactiva con fundido.
    class GlitchEngine
    {
    public:
        void prepare (double sampleRate);
        void reset();
        void process (float* const* ch, int numCh, int n, const GlitchParams& p);
    private:
        double sr = 44100.0;
        bool first = true, engaged = false;
        int w = 0, mask = 0, stepPos = 0, loopStart = 0, loopLen = 64, loopPos = 0, holdCnt = 0;
        float engGain = 0.0f, env = 0.0f, jitter = 0.0f;
        float held[2] {};
        std::vector<float> buf[2];
        juce::Random rng { 1234 };
        juce::SmoothedValue<float> sCrush, sRedux, sNoise, sDigital, sMix;
        juce::SmoothedValue<float> aCrush, aRedux, aNoise, aDigital;
    };
}
