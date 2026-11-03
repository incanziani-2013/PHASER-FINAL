#include "MasterEngine.h"

namespace phaser
{
    static constexpr float kCeilOn = 0.9441f;   // -0.5 dBFS
    static constexpr float kCeilOff = 16.0f;    // solo protección ante valores absurdos

    void MasterEngine::prepare (double sampleRate)
    {
        sr = sampleRate;
        sDrive.reset (sr, 0.03); sWidth.reset (sr, 0.03); sOut.reset (sr, 0.03); sCeil.reset (sr, 0.03);
        first = true; reset();
    }

    void MasterEngine::reset() { gain = 1.0f; lastReductionDb = 0.0f; }

    void MasterEngine::process (float* const* ch, int numCh, int n, const MasterParams& p)
    {
        const int nc = std::min (numCh, 2);
        const float outG = dbToGain (p.outGainDb);
        const float ceilT = p.limiter ? kCeilOn : kCeilOff;
        if (first)
        {
            sDrive.setCurrentAndTargetValue (p.drive); sWidth.setCurrentAndTargetValue (p.stereo);
            sOut.setCurrentAndTargetValue (outG); sCeil.setCurrentAndTargetValue (ceilT); first = false;
        }
        sDrive.setTargetValue (p.drive); sWidth.setTargetValue (p.stereo);
        sOut.setTargetValue (outG); sCeil.setTargetValue (ceilT);

        const float relCoef = 1.0f - std::exp (-1.0f / (0.08f * (float) sr));
        float minGain = 1.0f;

        for (int i = 0; i < n; ++i)
        {
            const float dr = sDrive.getNextValue(), width = sWidth.getNextValue();
            const float og = sOut.getNextValue(), ceil = sCeil.getNextValue();

            if (dr > 0.0001f)
            {
                const float a = std::min (1.0f, dr * 4.0f);
                const float nat = dr * 2.3f;
                const float g = std::exp (nat), comp = std::exp (-0.5f * nat);
                for (int c = 0; c < nc; ++c)
                {
                    const float x = ch[c][i];
                    ch[c][i] = x + a * (std::tanh (x * g) * comp - x);
                }
            }

            if (nc > 1)
            {
                const float m = 0.5f * (ch[0][i] + ch[1][i]), s = 0.5f * (ch[0][i] - ch[1][i]) * width;
                ch[0][i] = m + s; ch[1][i] = m - s;
            }

            float peak = 0.0f;
            for (int c = 0; c < nc; ++c) { ch[c][i] = sanitize (ch[c][i]) * og; peak = std::max (peak, std::abs (ch[c][i])); }

            const float target = peak > ceil ? ceil / peak : 1.0f;
            if (target < gain) gain += (target - gain) * 0.6f;           // ataque rápido
            else               gain += (1.0f - gain) * relCoef;           // release 80 ms
            minGain = std::min (minGain, gain);

            for (int c = 0; c < nc; ++c)
                ch[c][i] = std::max (-ceil, std::min (ceil, ch[c][i] * gain));   // recorte de seguridad final
        }
        lastReductionDb = p.limiter ? 20.0f * std::log10 (std::max (minGain, 1.0e-4f)) : 0.0f;
    }
}
