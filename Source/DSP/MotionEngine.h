#pragma once
#include "DSPCommon.h"

namespace phaser
{
    // mode: 0 Chorus, 1 Flanger
    struct MotionParams { float width = 1.0f, rate = 0.5f, depth = 0.5f, phaseDeg = 90.0f, feedback = 0.2f, mod = 0.5f, stereo = 0.3f; int mode = 0; };

    // MODULE 03 - MOTION: micro-delay modulado por LFO (chorus / flanger) con fase L/R,
    // realimentación cruzada (ping-pong) y ensanchador M/S.
    class MotionEngine
    {
    public:
        void prepare (double sampleRate);
        void reset();
        void process (float* const* ch, int numCh, int n, const MotionParams& p);
    private:
        inline float readFrac (const std::vector<float>& b, float delaySamples) const noexcept;
        double sr = 44100.0, lfoPhase = 0.0;
        bool first = true;
        int w = 0, mask = 0;
        std::vector<float> buf[2];
        juce::SmoothedValue<float> sRate, sDepth, sFb, sMod, sStereo, sWidth, sPhase, sBase, sExc;
    };
}
