#include "MotionEngine.h"

namespace phaser
{
    void MotionEngine::prepare (double sampleRate)
    {
        sr = sampleRate;
        const int size = juce::nextPowerOfTwo ((int) (sr * 0.06) + 8);   // 60 ms máx.
        for (auto& b : buf) b.assign ((size_t) size, 0.0f);
        mask = size - 1;
        for (auto* s : { &sRate, &sDepth, &sFb, &sMod, &sStereo, &sWidth, &sPhase }) s->reset (sr, 0.03);
        sBase.reset (sr, 0.08); sExc.reset (sr, 0.08);
        first = true;
        reset();
    }

    void MotionEngine::reset()
    {
        for (auto& b : buf) std::fill (b.begin(), b.end(), 0.0f);
        w = 0; lfoPhase = 0.0;
    }

    inline float MotionEngine::readFrac (const std::vector<float>& b, float d) const noexcept
    {
        const float rp = (float) w - d;
        const float fl = std::floor (rp);
        const float fr = rp - fl;
        const int i0 = ((int) fl) & mask, i1 = (i0 + 1) & mask;
        return b[(size_t) i0] + fr * (b[(size_t) i1] - b[(size_t) i0]);
    }

    void MotionEngine::process (float* const* ch, int numCh, int n, const MotionParams& p)
    {
        const int nc = std::min (numCh, 2);
        const float baseMs = p.mode == 1 ? 1.5f : 12.0f;
        const float excMs  = p.mode == 1 ? 1.3f : 7.0f;
        if (first)
        {
            sRate.setCurrentAndTargetValue (p.rate); sDepth.setCurrentAndTargetValue (p.depth);
            sFb.setCurrentAndTargetValue (p.feedback); sMod.setCurrentAndTargetValue (p.mod);
            sStereo.setCurrentAndTargetValue (p.stereo); sWidth.setCurrentAndTargetValue (p.width);
            sPhase.setCurrentAndTargetValue (p.phaseDeg);
            sBase.setCurrentAndTargetValue (baseMs); sExc.setCurrentAndTargetValue (excMs);
            first = false;
        }
        sRate.setTargetValue (p.rate); sDepth.setTargetValue (p.depth); sFb.setTargetValue (p.feedback);
        sMod.setTargetValue (p.mod); sStereo.setTargetValue (p.stereo); sWidth.setTargetValue (p.width);
        sPhase.setTargetValue (p.phaseDeg); sBase.setTargetValue (baseMs); sExc.setTargetValue (excMs);

        const float fbScale = p.mode == 1 ? 1.0f : 0.5f;
        const float maxD = (float) mask - 2.0f;

        for (int i = 0; i < n; ++i)
        {
            const float rate = sRate.getNextValue(), depth = sDepth.getNextValue();
            const float fb = sFb.getNextValue() * fbScale, mod = sMod.getNextValue();
            const float st = sStereo.getNextValue(), width = sWidth.getNextValue();
            const float ph = sPhase.getNextValue() / 360.0f;
            const float bms = sBase.getNextValue(), ems = sExc.getNextValue() * depth;

            lfoPhase += (double) rate / sr;
            if (lfoPhase >= 1.0) lfoPhase -= 1.0;

            const float l1 = std::sin (kTwoPi * (float) lfoPhase);
            const float l2 = std::sin (kTwoPi * ((float) lfoPhase + ph));
            const float msToS = 0.001f * (float) sr;
            const float dL = juce::jlimit (1.0f, maxD, (bms + ems * l1) * msToS);
            const float dR = juce::jlimit (1.0f, maxD, (bms + ems * l2) * msToS);

            const float xl = ch[0][i];
            const float xr = nc > 1 ? ch[1][i] : xl;
            const float yl = readFrac (buf[0], dL);
            const float yr = nc > 1 ? readFrac (buf[1], dR) : yl;

            const float crossL = (1.0f - st) * yl + st * yr;
            const float crossR = (1.0f - st) * yr + st * yl;
            buf[0][(size_t) w] = xl + fb * crossL;
            buf[1][(size_t) w] = xr + fb * crossR;
            w = (w + 1) & mask;

            const float dryG = 1.0f - 0.4f * mod, wetG = 0.7f * mod;
            float ol = xl * dryG + yl * wetG;
            float orr = xr * dryG + yr * wetG;

            if (nc > 1)
            {
                const float m = 0.5f * (ol + orr), s = 0.5f * (ol - orr) * width;   // M/S: el mono no cambia con width
                ol = m + s; orr = m - s;
                ch[0][i] = sanitize (ol); ch[1][i] = sanitize (orr);
            }
            else ch[0][i] = sanitize (ol);
        }
    }
}
