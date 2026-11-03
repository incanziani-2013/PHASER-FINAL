#include "GlitchEngine.h"

namespace phaser
{
    void GlitchEngine::prepare (double sampleRate)
    {
        sr = sampleRate;
        const int size = juce::nextPowerOfTwo ((int) sr);   // >= 1 s
        for (auto& b : buf) b.assign ((size_t) size, 0.0f);
        mask = size - 1;
        for (auto* s : { &sCrush, &sRedux, &sNoise, &sDigital, &sMix }) s->reset (sr, 0.02);
        for (auto* s : { &aCrush, &aRedux, &aNoise, &aDigital }) s->reset (sr, 0.01);
        first = true;
        reset();
    }

    void GlitchEngine::reset()
    {
        for (auto& b : buf) std::fill (b.begin(), b.end(), 0.0f);
        w = 0; stepPos = 0; loopPos = 0; holdCnt = 0; engaged = false; engGain = 0; env = 0;
        held[0] = held[1] = 0;
    }

    void GlitchEngine::process (float* const* ch, int numCh, int n, const GlitchParams& p)
    {
        const int nc = std::min (numCh, 2);
        if (first)
        {
            sCrush.setCurrentAndTargetValue (p.crush); sRedux.setCurrentAndTargetValue (p.redux);
            sNoise.setCurrentAndTargetValue (p.noise); sDigital.setCurrentAndTargetValue (p.digital);
            sMix.setCurrentAndTargetValue (p.mix);
            aCrush.setCurrentAndTargetValue (p.crushOn ? 1.f : 0.f); aRedux.setCurrentAndTargetValue (p.reduxOn ? 1.f : 0.f);
            aNoise.setCurrentAndTargetValue (p.noiseOn ? 1.f : 0.f);  aDigital.setCurrentAndTargetValue (p.digitalOn ? 1.f : 0.f);
            first = false;
        }
        sCrush.setTargetValue (p.crush); sRedux.setTargetValue (p.redux); sNoise.setTargetValue (p.noise);
        sDigital.setTargetValue (p.digital); sMix.setTargetValue (p.mix);
        aCrush.setTargetValue (p.crushOn ? 1.f : 0.f); aRedux.setTargetValue (p.reduxOn ? 1.f : 0.f);
        aNoise.setTargetValue (p.noiseOn ? 1.f : 0.f);  aDigital.setTargetValue (p.digitalOn ? 1.f : 0.f);

        static const float beats[6] = { 4.0f, 2.0f, 1.0f, 0.5f, 0.25f, 0.125f };
        const double bpm = std::max (20.0, std::min (400.0, p.bpm));
        const int stepSamples = std::max (256, (int) (sr * 60.0 / bpm * (double) beats[std::max (0, std::min (5, p.rate))]));
        const float engCoef = 1.0f - std::exp (-1.0f / (0.003f * (float) sr));
        const float envDecay = std::exp (-1.0f / (0.08f * (float) sr));
        const int maxLoop = (int) (0.5 * sr);
        const int fadeN = 32;

        for (int i = 0; i < n; ++i)
        {
            // ---- reloj de pasos / stutter ----
            if (++stepPos >= stepSamples)
            {
                stepPos = 0;
                engaged = p.stutterOn && p.stutter > 0.001f && rng.nextFloat() < p.stutter;
                if (engaged)
                {
                    loopLen = std::max (64, std::min (maxLoop, stepSamples / 4));
                    loopStart = (w - loopLen) & mask;
                    loopPos = 0;
                }
                jitter = p.digitalOn ? rng.nextFloat() * sDigital.getCurrentValue() : 0.0f;   // modulación glitch
            }
            if (! p.stutterOn) engaged = false;
            engGain += ((engaged ? 1.0f : 0.0f) - engGain) * engCoef;

            const float crush = sCrush.getNextValue(), redux = sRedux.getNextValue();
            const float noise = sNoise.getNextValue(), digital = sDigital.getNextValue(), mix = sMix.getNextValue();
            const float aC = aCrush.getNextValue(), aR = aRedux.getNextValue();
            const float aN = aNoise.getNextValue(), aD = aDigital.getNextValue();

            float inPeak = 0;
            for (int c = 0; c < nc; ++c) inPeak = std::max (inPeak, std::abs (ch[c][i]));
            env = std::max (inPeak, env * envDecay);

            // sample & hold compartido
            const int holdLen = 1 + (int) (redux * redux * 60.0f + jitter * 20.0f);
            const bool takeNew = (--holdCnt <= 0);
            if (takeNew) holdCnt = holdLen;

            const float bits = 16.0f - crush * 14.0f;
            const float levels = std::exp2 (bits - 1.0f);
            const float dd = 1.0f + digital * 14.0f;
            const float ddComp = 1.0f / std::sqrt (dd);
            const float nAmt = noise * noise * 0.5f * env * aN;

            // ventana del loop (evita clicks en los bordes)
            float loopWin = 1.0f;
            if (engGain > 0.0005f)
            {
                loopWin = std::min (1.0f, std::min ((float) loopPos, (float) (loopLen - 1 - loopPos)) / (float) fadeN);
                loopWin = std::max (0.0f, loopWin);
            }
            const int loopIdx = (loopStart + loopPos) & mask;

            float inS[2] = { 0.0f, 0.0f };
            for (int c = 0; c < nc; ++c)
            {
                const float in = ch[c][i];
                inS[c] = in;
                float y = in;

                if (engGain > 0.0005f)
                    y += (buf[c][(size_t) loopIdx] * loopWin - y) * engGain;

                if (takeNew) held[c] = y;
                y += (held[c] - y) * aR;

                const float q = std::round (y * levels) / levels;
                y += (q - y) * aC;

                const float dz = std::max (-1.0f, std::min (1.0f, y * dd)) * ddComp;
                y += (dz - y) * aD;

                y += (rng.nextFloat() * 2.0f - 1.0f) * nAmt;

                ch[c][i] = sanitize (in + (y - in) * mix);
            }

            if (! engaged)   // mientras el loop está activo el buffer queda congelado
                for (int c = 0; c < nc; ++c) buf[c][(size_t) w] = inS[c];
            w = (w + 1) & mask;
            if (engGain > 0.0005f && ++loopPos >= loopLen) loopPos = 0;
        }
    }
}
