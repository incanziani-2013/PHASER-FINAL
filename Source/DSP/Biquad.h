#pragma once
#include "DSPCommon.h"

namespace phaser
{
    // Biquad RBJ (Direct Form II Transposed). Sin asignaciones de memoria.
    enum class FType { LowPass, HighPass, BandPass, Peak, LowShelf, HighShelf };

    struct BQCoeffs { float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0; };

    inline BQCoeffs makeBQ (FType t, double sr, double f, double q, double gainDb)
    {
        f = std::min (std::max (f, 10.0), sr * 0.45);
        q = std::max (q, 0.1);
        const double w0 = 6.283185307179586 * f / sr, c = std::cos (w0), s = std::sin (w0);
        const double A = std::pow (10.0, gainDb / 40.0);
        const double alpha = s / (2.0 * q);
        const double sq = 2.0 * std::sqrt (A) * alpha;
        double b0 = 1, b1 = 0, b2 = 0, a0 = 1, a1 = 0, a2 = 0;

        switch (t)
        {
            case FType::LowPass:
                b0 = (1 - c) / 2; b1 = 1 - c; b2 = b0; a0 = 1 + alpha; a1 = -2 * c; a2 = 1 - alpha; break;
            case FType::HighPass:
                b0 = (1 + c) / 2; b1 = -(1 + c); b2 = b0; a0 = 1 + alpha; a1 = -2 * c; a2 = 1 - alpha; break;
            case FType::BandPass:   // ganancia de pico 0 dB
                b0 = alpha; b1 = 0; b2 = -alpha; a0 = 1 + alpha; a1 = -2 * c; a2 = 1 - alpha; break;
            case FType::Peak:
                b0 = 1 + alpha * A; b1 = -2 * c; b2 = 1 - alpha * A;
                a0 = 1 + alpha / A; a1 = -2 * c; a2 = 1 - alpha / A; break;
            case FType::LowShelf:
                b0 = A * ((A + 1) - (A - 1) * c + sq);
                b1 = 2 * A * ((A - 1) - (A + 1) * c);
                b2 = A * ((A + 1) - (A - 1) * c - sq);
                a0 = (A + 1) + (A - 1) * c + sq;
                a1 = -2 * ((A - 1) + (A + 1) * c);
                a2 = (A + 1) + (A - 1) * c - sq; break;
            case FType::HighShelf:
                b0 = A * ((A + 1) + (A - 1) * c + sq);
                b1 = -2 * A * ((A - 1) + (A + 1) * c);
                b2 = A * ((A + 1) + (A - 1) * c - sq);
                a0 = (A + 1) - (A - 1) * c + sq;
                a1 = 2 * ((A - 1) - (A + 1) * c);
                a2 = (A + 1) - (A - 1) * c - sq; break;
        }
        BQCoeffs r;
        r.b0 = (float) (b0 / a0); r.b1 = (float) (b1 / a0); r.b2 = (float) (b2 / a0);
        r.a1 = (float) (a1 / a0); r.a2 = (float) (a2 / a0);
        return r;
    }

    struct BQ
    {
        BQCoeffs c;
        float z1 = 0, z2 = 0;
        inline float process (float x) noexcept
        {
            const float y = c.b0 * x + z1;
            z1 = c.b1 * x - c.a1 * y + z2;
            z2 = c.b2 * x - c.a2 * y;
            return y;
        }
        void reset() noexcept { z1 = z2 = 0; }
    };
}
