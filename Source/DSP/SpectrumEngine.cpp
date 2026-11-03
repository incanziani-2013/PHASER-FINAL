#include "SpectrumEngine.h"

namespace phaser
{
    void SpectrumEngine::prepare (double sampleRate)
    {
        sr = sampleRate;
        for (auto* s : { &sLow, &sMid, &sHigh, &sRes, &sQ }) s->reset (sr, 0.04);
        sFreq.reset (sr, 0.04);
        first = true;
        reset();
    }

    void SpectrumEngine::reset()
    {
        for (int c = 0; c < 2; ++c) { lowB[c].reset(); midB[c].reset(); highB[c].reset(); cur[c].reset(); prev[c].reset(); }
        curType = prevType = 0; fadeLeft = 0;
    }

    void SpectrumEngine::process (float* const* ch, int numCh, int n, const SpectrumParams& p)
    {
        const int nc = std::min (numCh, 2);
        if (first)
        {
            sLow.setCurrentAndTargetValue (p.low); sMid.setCurrentAndTargetValue (p.mid);
            sHigh.setCurrentAndTargetValue (p.high); sRes.setCurrentAndTargetValue (p.res);
            sQ.setCurrentAndTargetValue (p.q); sFreq.setCurrentAndTargetValue (p.freq);
            curType = p.filter; first = false;
        }
        sLow.setTargetValue (p.low); sMid.setTargetValue (p.mid); sHigh.setTargetValue (p.high);
        sRes.setTargetValue (p.res); sQ.setTargetValue (p.q); sFreq.setTargetValue (std::max (20.0f, p.freq));

        static const FType map[7] = { FType::LowPass, FType::LowPass, FType::HighPass, FType::BandPass,
                                      FType::Peak, FType::LowShelf, FType::HighShelf };
        int pos = 0;
        while (pos < n)
        {
            const int len = std::min (kChunk, n - pos);
            const float low = sLow.skip (len), mid = sMid.skip (len), high = sHigh.skip (len);
            const float res = sRes.skip (len), q = sQ.skip (len), freq = sFreq.skip (len);

            const auto lc = makeBQ (FType::LowShelf,  sr, 120.0,  0.707, low);
            const auto mc = makeBQ (FType::Peak,      sr, freq,   q,     mid);
            const auto hc = makeBQ (FType::HighShelf, sr, 6000.0, 0.707, high);
            for (int c = 0; c < nc; ++c) { lowB[c].c = lc; midB[c].c = mc; highB[c].c = hc; }

            if (p.filter != curType)
            {
                for (int c = 0; c < nc; ++c) { prev[c] = cur[c]; cur[c].reset(); }
                prevType = curType; curType = p.filter; fadeLeft = kFade;
            }
            if (curType > 0)
            {
                const double fq = 0.707 * std::pow (12.0, (double) res);   // 0.7 .. 8.5
                const auto fc = makeBQ (map[curType], sr, freq, fq, res * 12.0);
                for (int c = 0; c < nc; ++c) cur[c].c = fc;
            }

            for (int i = 0; i < len; ++i)
            {
                float t = 1.0f;
                const bool fading = fadeLeft > 0;
                if (fading) { t = 1.0f - (float) fadeLeft / (float) kFade; --fadeLeft; }

                for (int c = 0; c < nc; ++c)
                {
                    float* d = ch[c] + pos + i;
                    float y = lowB[c].process (*d);
                    y = midB[c].process (y);
                    y = highB[c].process (y);
                    const float yc = curType > 0 ? cur[c].process (y) : y;
                    if (fading)
                    {
                        const float yp = prevType > 0 ? prev[c].process (y) : y;
                        y = yp + (yc - yp) * t;
                    }
                    else y = yc;
                    *d = sanitize (y);
                }
            }
            pos += len;
        }
    }
}
