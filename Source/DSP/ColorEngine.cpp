#include "ColorEngine.h"

namespace phaser
{
    void ColorEngine::prepare (double sampleRate)
    {
        sr = sampleRate;
        for (auto* s : { &sDrive, &sTone, &sSat, &sBlend, &sWarm, &sBright, &sHarm }) s->reset (sr, 0.03);
        first = true;
        reset();
    }

    void ColorEngine::reset()
    {
        for (int c = 0; c < 2; ++c)
        {
            toneLo[c].reset(); toneHi[c].reset(); warmF[c].reset(); brightF[c].reset();
            dcX[c] = dcY[c] = 0;
        }
    }

    void ColorEngine::process (float* const* ch, int numCh, int n, const ColorParams& p)
    {
        const int nc = std::min (numCh, 2);
        if (first)
        {
            sDrive.setCurrentAndTargetValue (p.drive); sTone.setCurrentAndTargetValue (p.tone);
            sSat.setCurrentAndTargetValue (p.sat);     sBlend.setCurrentAndTargetValue (p.blend);
            sWarm.setCurrentAndTargetValue (p.warm);   sBright.setCurrentAndTargetValue (p.bright);
            sHarm.setCurrentAndTargetValue (p.harm);
            first = false;
        }
        sDrive.setTargetValue (p.drive); sTone.setTargetValue (p.tone); sSat.setTargetValue (p.sat);
        sBlend.setTargetValue (p.blend); sWarm.setTargetValue (p.warm); sBright.setTargetValue (p.bright);
        sHarm.setTargetValue (p.harm);

        const float R = 1.0f - 220.0f / (float) sr;   // bloqueador DC ~35 Hz

        int pos = 0;
        while (pos < n)
        {
            const int len = std::min (kChunk, n - pos);

            const float tone   = sTone.skip (len);
            const float warm   = sWarm.skip (len);
            const float bright = sBright.skip (len);
            const float t = (tone - 0.5f) * 2.0f;   // -1..1 (tilt: oscuro <-> brillante)

            const auto lo = makeBQ (FType::LowShelf,  sr, 400.0,  0.707, -t * 6.0);
            const auto hi = makeBQ (FType::HighShelf, sr, 3000.0, 0.707,  t * 6.0);
            const auto wm = makeBQ (FType::LowShelf,  sr, 150.0,  0.707,  warm * 6.0);
            const auto br = makeBQ (FType::HighShelf, sr, 6000.0, 0.707,  bright * 9.0);
            for (int c = 0; c < nc; ++c) { toneLo[c].c = lo; toneHi[c].c = hi; warmF[c].c = wm; brightF[c].c = br; }

            for (int i = 0; i < len; ++i)
            {
                const float drv   = sDrive.getNextValue();
                const float sat   = sSat.getNextValue();
                const float blend = sBlend.getNextValue();
                const float harm  = sHarm.getNextValue();

                const float nat  = drv * 3.45388f;                 // 0..30 dB de ganancia previa
                const float g    = std::exp (nat);
                const float comp = std::exp (-0.6f * nat);         // compensación parcial de nivel
                const float bias = 0.35f * harm;
                const float tb   = std::tanh (bias);

                for (int c = 0; c < nc; ++c)
                {
                    float* d = ch[c] + pos + i;
                    const float x = *d;

                    const float pre    = x * g;
                    const float soft   = std::tanh (pre);
                    const float asym   = std::tanh (pre + bias) - tb;     // armónicos pares
                    const float shaped = soft + harm * (asym - soft);
                    float wet = x + sat * (shaped * comp - x);

                    float y = wet - dcX[c] + R * dcY[c];                  // quita DC del waveshaper asimétrico
                    dcX[c] = wet; dcY[c] = y;

                    y = toneLo[c].process (y);
                    y = toneHi[c].process (y);
                    y = warmF[c].process (y);
                    y = brightF[c].process (y);

                    *d = sanitize (x + blend * (y - x));                  // procesamiento paralelo
                }
            }
            pos += len;
        }
    }
}
