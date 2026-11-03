// Tests offline del motor DSP de PHASER (compilados con g++ y el shim de Tests/TestShim.h).
// No sustituyen a probar el plugin real en FL Studio.
#include "ColorEngine.h"
#include "SpectrumEngine.h"
#include "MotionEngine.h"
#include "GlitchEngine.h"
#include "MasterEngine.h"
#include <chrono>
#include <cstdio>
#include <functional>
using namespace phaser;

static int failures = 0, checks = 0;
#define CHECK(cond, ...) do { ++checks; if (!(cond)) { ++failures; std::printf ("  [FAIL] "); std::printf (__VA_ARGS__); std::printf ("\n"); } else { std::printf ("  [ ok ] "); std::printf (__VA_ARGS__); std::printf ("\n"); } } while (0)

static const double SR = 44100.0;
using Buf = std::vector<float>;

static Buf sine (double f, float amp, int n) { Buf b ((size_t) n); for (int i = 0; i < n; ++i) b[(size_t) i] = amp * (float) std::sin (6.283185307179586 * f * i / SR); return b; }
static double rms (const Buf& b, int from, int to) { double s = 0; for (int i = from; i < to; ++i) s += (double) b[(size_t) i] * b[(size_t) i]; return std::sqrt (s / (to - from)); }
static double db (double x) { return 20.0 * std::log10 (std::max (x, 1e-12)); }
static double goertzel (const Buf& b, double f, int from, int to)
{
    double w = 6.283185307179586 * f / SR, re = 0, im = 0;
    for (int i = from; i < to; ++i) { re += b[(size_t) i] * std::cos (w * i); im += b[(size_t) i] * std::sin (w * i); }
    return 2.0 * std::sqrt (re * re + im * im) / (to - from);
}
static double maxAbs (const Buf& b) { double m = 0; for (float x : b) m = std::max (m, (double) std::abs (x)); return m; }
static bool finiteAll (const Buf& b) { for (float x : b) if (! std::isfinite (x)) return false; return true; }

// Procesa un buffer (estéreo con L=R=in salvo que se pase r) en bloques de tamaño bs.
template <class Fn>
static void runStereo (Buf& L, Buf& R, int bs, Fn fn)
{
    const int n = (int) L.size();
    for (int pos = 0; pos < n; pos += bs)
    {
        const int len = std::min (bs, n - pos);
        float* p[2] = { L.data() + pos, R.data() + pos };
        fn (p, 2, len);
    }
}

int main()
{
    const int N = 44100;

    std::printf ("== COLOR ==\n");
    {
        ColorEngine e; e.prepare (SR);
        ColorParams p; p.drive = 0; p.sat = 0; p.blend = 1; p.tone = 0.5f; p.warm = 0; p.bright = 0; p.harm = 0;
        Buf in = sine (2000, 0.5f, N), L = in, R = in;
        runStereo (L, R, 128, [&] (float** c, int nc, int n) { e.process (c, nc, n, p); });
        CHECK (std::abs (db (rms (L, N / 2, N)) - db (rms (in, N / 2, N))) < 0.15, "transparente con drive=0/sat=0 (dif %.3f dB)", db (rms (L, N/2, N)) - db (rms (in, N/2, N)));

        ColorEngine e2; e2.prepare (SR);
        ColorParams q; q.drive = 0.8f; q.sat = 1; q.blend = 1; q.harm = 1;
        Buf L2 = sine (1000, 0.5f, N), R2 = L2;
        runStereo (L2, R2, 256, [&] (float** c, int nc, int n) { e2.process (c, nc, n, q); });
        const double h2 = goertzel (L2, 2000, N / 2, N), h3 = goertzel (L2, 3000, N / 2, N), f1 = goertzel (L2, 1000, N / 2, N);
        CHECK (finiteAll (L2) && maxAbs (L2) < 2.0, "salida finita y acotada (max %.3f)", maxAbs (L2));
        CHECK (h2 > 0.01 * f1 && h3 > 0.01 * f1, "genera armónicos 2º/3º (%.1f dB / %.1f dB bajo fundamental)", db (h2 / f1), db (h3 / f1));
        ColorEngine e3; e3.prepare (SR); ColorParams z = q; z.blend = 0;
        Buf L3 = sine (1000, 0.5f, N), R3 = L3, ref = L3;
        runStereo (L3, R3, 64, [&] (float** c, int nc, int n) { e3.process (c, nc, n, z); });
        double md = 0; for (int i = 0; i < N; ++i) md = std::max (md, (double) std::abs (L3[(size_t) i] - ref[(size_t) i]));
        CHECK (md < 1e-6, "blend=0 devuelve la señal original (err %.2e)", md);
    }

    std::printf ("== SPECTRUM ==\n");
    {
        SpectrumParams p;
        auto gainAt = [&] (double f, SpectrumParams pp) { SpectrumEngine e; e.prepare (SR); Buf L = sine (f, 0.25f, N), R = L, in = L;
            runStereo (L, R, 100, [&] (float** c, int nc, int n) { e.process (c, nc, n, pp); }); return db (rms (L, N/2, N)) - db (rms (in, N/2, N)); };
        CHECK (std::abs (gainAt (1000, p)) < 0.05, "plano con todo a 0 dB (%.3f dB)", gainAt (1000, p));
        SpectrumParams lo = p; lo.low = 12;
        double g = gainAt (40, lo); CHECK (g > 10.5 && g < 12.5, "low shelf +12 dB @40 Hz = %.2f dB", g);
        SpectrumParams hi = p; hi.high = -12;
        g = gainAt (14000, hi); CHECK (g < -10.5 && g > -12.5, "high shelf -12 dB @14 kHz = %.2f dB", g);
        SpectrumParams mid = p; mid.mid = 9; mid.freq = 2000; mid.q = 2;
        g = gainAt (2000, mid); CHECK (std::abs (g - 9.0) < 0.3, "peaking +9 dB @2 kHz = %.2f dB", g);
        SpectrumParams lp = p; lp.filter = 1; lp.freq = 1000; lp.res = 0;
        g = gainAt (8000, lp); CHECK (g < -30, "low-pass 1 kHz atenúa 8 kHz %.1f dB", g);
        SpectrumParams hp = p; hp.filter = 2; hp.freq = 1000; hp.res = 0;
        g = gainAt (100, hp); CHECK (g < -30, "high-pass 1 kHz atenúa 100 Hz %.1f dB", g);
        SpectrumParams bp = p; bp.filter = 3; bp.freq = 1000; bp.res = 0.5f;
        CHECK (gainAt (1000, bp) > -1.0 && gainAt (100, bp) < -12, "band-pass centrado en 1 kHz");
        // clicks al cambiar de tipo de filtro y al barrer parámetros
        SpectrumEngine e; e.prepare (SR);
        Buf L = sine (1000, 0.5f, N * 2), R = L; const int bs = 64; double maxStep = 0;
        float prev = 0;
        for (int pos = 0; pos + bs <= (int) L.size(); pos += bs)
        {
            SpectrumParams pp; pp.filter = (pos / 8192) % 7; pp.low = (pos / 4096 % 2) ? 12.f : -12.f; pp.freq = 500.f + (float) (pos % 20000);
            float* ptr[2] = { L.data() + pos, R.data() + pos }; e.process (ptr, 2, bs, pp);
            for (int i = 0; i < bs; ++i) { maxStep = std::max (maxStep, (double) std::abs (L[(size_t) (pos + i)] - prev)); prev = L[(size_t) (pos + i)]; }
        }
        CHECK (finiteAll (L) && maxStep < 0.6, "sin discontinuidades al automatizar filtro/ganancia (salto máx %.3f)", maxStep);
    }

    std::printf ("== MOTION ==\n");
    {
        for (int mode = 0; mode < 2; ++mode)
        {
            MotionEngine e; e.prepare (SR);
            MotionParams p; p.mode = mode; p.mod = 0; p.width = 1;
            Buf in = sine (700, 0.5f, N), L = in, R = in;
            runStereo (L, R, 200, [&] (float** c, int nc, int n) { e.process (c, nc, n, p); });
            double md = 0; for (int i = 0; i < N; ++i) md = std::max (md, (double) std::abs (L[(size_t) i] - in[(size_t) i]));
            CHECK (md < 1e-5, "modo %d: mod=0 deja pasar la señal intacta (err %.2e)", mode, md);

            MotionEngine e2; e2.prepare (SR); MotionParams q; q.mode = mode; q.mod = 1; q.depth = 1; q.feedback = 0.85f; q.rate = 2; q.width = 2; q.phaseDeg = 180;
            Buf L2 = sine (700, 0.5f, N * 2), R2 = L2;
            runStereo (L2, R2, 77, [&] (float** c, int nc, int n) { e2.process (c, nc, n, q); });
            double diff = 0; for (int i = N; i < 2 * N; ++i) diff += std::abs (L2[(size_t) i] - R2[(size_t) i]);
            CHECK (finiteAll (L2) && finiteAll (R2) && maxAbs (L2) < 3.0, "modo %d: estable con feedback 0.85 (max %.2f)", mode, maxAbs (L2));
            CHECK (diff > 1.0, "modo %d: genera diferencia L/R (imagen estéreo)", mode);

            MotionEngine e3; e3.prepare (SR); MotionParams w = q; w.width = 0;
            Buf L3 = sine (700, 0.5f, N), R3 = sine (900, 0.5f, N);
            runStereo (L3, R3, 128, [&] (float** c, int nc, int n) { e3.process (c, nc, n, w); });
            double md2 = 0; for (int i = 0; i < N; ++i) md2 = std::max (md2, (double) std::abs (L3[(size_t) i] - R3[(size_t) i]));
            CHECK (md2 < 1e-5, "modo %d: width=0 colapsa a mono (compat. mono)", mode);
        }
    }

    std::printf ("== GLITCH ==\n");
    {
        GlitchEngine e; e.prepare (SR); GlitchParams p; p.mix = 0;
        Buf in = sine (440, 0.5f, N), L = in, R = in;
        runStereo (L, R, 100, [&] (float** c, int nc, int n) { e.process (c, nc, n, p); });
        double md = 0; for (int i = 0; i < N; ++i) md = std::max (md, (double) std::abs (L[(size_t) i] - in[(size_t) i]));
        CHECK (md < 1e-6, "mix=0 deja pasar la señal original (err %.2e)", md);

        GlitchEngine e2; e2.prepare (SR); GlitchParams c; c.mix = 1; c.crush = 1; c.crushOn = true; c.reduxOn = false; c.stutterOn = false; c.digitalOn = false; c.noiseOn = false;
        Buf L2 = in, R2 = in;
        runStereo (L2, R2, 64, [&] (float** cc, int nc, int n) { e2.process (cc, nc, n, c); });
        std::vector<float> vals; for (int i = N / 2; i < N; ++i) { bool f = false; for (float v : vals) if (std::abs (v - L2[(size_t) i]) < 1e-6f) { f = true; break; } if (! f) vals.push_back (L2[(size_t) i]); }
        CHECK (vals.size() <= 5, "bitcrush a 2 bits cuantiza a pocos niveles (%d distintos)", (int) vals.size());

        GlitchEngine e3; e3.prepare (SR); GlitchParams r; r.mix = 1; r.redux = 1; r.crushOn = false; r.stutterOn = false; r.digitalOn = false; r.noiseOn = false; r.reduxOn = true;
        Buf L3 = in, R3 = in;
        runStereo (L3, R3, 128, [&] (float** cc, int nc, int n) { e3.process (cc, nc, n, r); });
        int runs = 0; for (int i = N / 2 + 1; i < N; ++i) if (std::abs (L3[(size_t) i] - L3[(size_t) i - 1]) < 1e-6f) ++runs;
        CHECK (runs > (N / 2) * 0.9, "reducción de sample-rate mantiene muestras (%d repetidas)", runs);

        GlitchEngine e4; e4.prepare (SR); GlitchParams s; s.mix = 1; s.stutter = 1; s.stutterOn = true; s.rate = 5; s.bpm = 120;
        s.crushOn = s.reduxOn = s.digitalOn = s.noiseOn = false;
        juce::Random rr (7); Buf noise ((size_t) N); for (auto& v : noise) v = rr.nextFloat() - 0.5f;
        Buf L4 = noise, R4 = noise;
        runStereo (L4, R4, 100, [&] (float** cc, int nc, int n) { e4.process (cc, nc, n, s); });
        const int step = (int) (SR * 60.0 / 120.0 * 0.125), loop = step / 4; double md4 = 0; int cnt = 0;
        for (int t = step + 800; t < step + 1000; ++t) { md4 = std::max (md4, (double) std::abs (L4[(size_t) t] - L4[(size_t) (t + loop)])); ++cnt; }
        CHECK (md4 < 0.02, "stutter repite el buffer (periodo %d muestras, err %.4f)", loop, md4);

        GlitchEngine e5; e5.prepare (SR); GlitchParams all; all.mix = 1; all.crush = 1; all.redux = 1; all.digital = 1; all.noise = 1; all.stutter = 1; all.rate = 4;
        Buf L5 = sine (200, 0.9f, N * 3), R5 = L5;
        runStereo (L5, R5, 333, [&] (float** cc, int nc, int n) { e5.process (cc, nc, n, all); });
        CHECK (finiteAll (L5) && maxAbs (L5) < 3.0, "todo al máximo: finito y acotado (max %.2f)", maxAbs (L5));
    }

    std::printf ("== MASTER ==\n");
    {
        MasterEngine e; e.prepare (SR); MasterParams p; p.limiter = true;
        Buf L = sine (1000, 3.0f, N), R = L;
        runStereo (L, R, 128, [&] (float** c, int nc, int n) { e.process (c, nc, n, p); });
        CHECK (maxAbs (L) <= 0.9442 && maxAbs (R) <= 0.9442, "limitador respeta el techo -0.5 dBFS con +9.5 dB de entrada (max %.4f)", maxAbs (L));
        CHECK (e.getReductionDb() < -5, "reporta reducción de ganancia (%.1f dB)", e.getReductionDb());

        MasterEngine e2; e2.prepare (SR); Buf L2 = sine (1000, 0.5f, N), R2 = L2, in = L2;
        runStereo (L2, R2, 64, [&] (float** c, int nc, int n) { e2.process (c, nc, n, p); });
        double md = 0; for (int i = 0; i < N; ++i) md = std::max (md, (double) std::abs (L2[(size_t) i] - in[(size_t) i]));
        CHECK (md < 1e-6, "transparente por debajo del techo (err %.2e)", md);

        MasterEngine e3; e3.prepare (SR); MasterParams m; m.drive = 1; m.limiter = true;
        Buf L3 = sine (200, 0.9f, N), R3 = L3;
        runStereo (L3, R3, 64, [&] (float** c, int nc, int n) { e3.process (c, nc, n, m); });
        CHECK (finiteAll (L3) && maxAbs (L3) <= 0.9442, "drive máximo + limitador acotado (max %.3f)", maxAbs (L3));
    }

    std::printf ("== INDEPENDENCIA DEL TAMAÑO DE BUFFER ==\n");
    {
        auto chain = [&] (int bs, Buf& out)
        {
            ColorEngine c; SpectrumEngine s; MotionEngine m; GlitchEngine g; MasterEngine ma;
            c.prepare (SR); s.prepare (SR); m.prepare (SR); g.prepare (SR); ma.prepare (SR);
            ColorParams cp; cp.drive = 0.6f; cp.sat = 0.8f; SpectrumParams sp; sp.low = 4; sp.filter = 4; sp.freq = 3000; sp.res = 0.5f;
            MotionParams mp; mp.mod = 0.7f; GlitchParams gp; gp.stutterOn = false; gp.noiseOn = false; gp.mix = 0.5f; MasterParams mps;
            Buf L = sine (330, 0.4f, N), R = sine (550, 0.4f, N);
            runStereo (L, R, bs, [&] (float** ch, int nc, int n) { c.process (ch, nc, n, cp); s.process (ch, nc, n, sp); m.process (ch, nc, n, mp); g.process (ch, nc, n, gp); ma.process (ch, nc, n, mps); });
            out = L;
        };
        Buf a, b, d; chain (1, a); chain (64, b); chain (997, d);
        double m1 = 0, m2 = 0; for (int i = 0; i < N; ++i) { m1 = std::max (m1, (double) std::abs (a[(size_t) i] - b[(size_t) i])); m2 = std::max (m2, (double) std::abs (b[(size_t) i] - d[(size_t) i])); }
        CHECK (m1 < 1e-3 && m2 < 1e-3, "cadena completa igual con bloques de 1/64/997 muestras (dif %.2e / %.2e)", m1, m2);
    }

    std::printf ("== SAMPLE RATES Y MONO ==\n");
    {
        bool ok = true;
        for (double sr : { 22050.0, 44100.0, 48000.0, 96000.0, 192000.0 })
        {
            ColorEngine c; SpectrumEngine s; MotionEngine m; GlitchEngine g; MasterEngine ma;
            c.prepare (sr); s.prepare (sr); m.prepare (sr); g.prepare (sr); ma.prepare (sr);
            ColorParams cp; SpectrumParams sp; sp.filter = 3; MotionParams mp; GlitchParams gp; MasterParams mps;
            Buf x ((size_t) (sr / 2)); for (size_t i = 0; i < x.size(); ++i) x[i] = 0.5f * (float) std::sin (6.2831853 * 440.0 * (double) i / sr);
            for (size_t pos = 0; pos < x.size(); pos += 512) { int len = (int) std::min<size_t> (512, x.size() - pos); float* ch[1] = { x.data() + pos };
                c.process (ch, 1, len, cp); s.process (ch, 1, len, sp); m.process (ch, 1, len, mp); g.process (ch, 1, len, gp); ma.process (ch, 1, len, mps); }
            ok = ok && finiteAll (x) && maxAbs (x) < 2.0;
        }
        CHECK (ok, "mono, 22.05k..192k: salida finita y acotada");
    }

    std::printf ("== RENDIMIENTO (referencia, esta máquina) ==\n");
    {
        ColorEngine c; SpectrumEngine s; MotionEngine m; GlitchEngine g; MasterEngine ma;
        c.prepare (SR); s.prepare (SR); m.prepare (SR); g.prepare (SR); ma.prepare (SR);
        ColorParams cp; SpectrumParams sp; sp.filter = 1; MotionParams mp; GlitchParams gp; MasterParams mps;
        const int secs = 20; Buf L = sine (220, 0.4f, (int) SR * secs), R = L;
        auto t0 = std::chrono::steady_clock::now();
        runStereo (L, R, 512, [&] (float** ch, int nc, int n) { c.process (ch, nc, n, cp); s.process (ch, nc, n, sp); m.process (ch, nc, n, mp); g.process (ch, nc, n, gp); ma.process (ch, nc, n, mps); });
        double el = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
        std::printf ("  4 módulos + master, estéreo 44.1 kHz: %.1fx tiempo real (~%.1f %% de un núcleo)\n", secs / el, 100.0 * el / secs);
    }

    std::printf ("\n%d comprobaciones, %d fallos\n", checks, failures);
    return failures ? 1 : 0;
}
