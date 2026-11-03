#include "CentralVisualizer.h"
#include "../PluginProcessor.h"

namespace phaser::ui
{
    CentralVisualizer::CentralVisualizer (PhaserAudioProcessor& p) : proc (p)
    {
        setInterceptsMouseClicks (false, false);
        for (auto& pt : particles)
        {
            pt.angle = rng.nextFloat() * juce::MathConstants<float>::twoPi;
            pt.radius = 0.25f + rng.nextFloat() * 0.85f;
            pt.speed = (rng.nextFloat() - 0.3f) * 0.6f;
            pt.size = 1.0f + rng.nextFloat() * 2.5f;
            pt.hue = rng.nextFloat();
        }
        proc.visualizerActive = true;
        startTimerHz (30);
    }

    CentralVisualizer::~CentralVisualizer() { proc.visualizerActive = false; stopTimer(); }

    void CentralVisualizer::analyse()
    {
        std::array<float, fftSize> tmp {};
        const int got = proc.readVisualizerSamples (tmp.data(), fftSize);
        if (got > 0)
        {
            std::copy (history.begin() + got, history.end(), history.begin());   // desplaza y añade lo nuevo
            std::copy (tmp.begin(), tmp.begin() + got, history.end() - got);
        }
        else  // sin audio: decae suavemente
            for (auto& h : history) h *= 0.9f;

        float sum = 0; for (float v : history) sum += v * v;
        level = std::sqrt (sum / (float) fftSize);

        std::copy (history.begin(), history.end(), fftData.begin());
        std::fill (fftData.begin() + fftSize, fftData.end(), 0.0f);
        window.multiplyWithWindowingTable (fftData.data(), (size_t) fftSize);
        fft.performFrequencyOnlyForwardTransform (fftData.data());

        const double sr = juce::jmax (8000.0, proc.getSampleRate());
        for (int b = 0; b < numBands; ++b)
        {
            const float f0 = 40.0f * std::pow (16000.0f / 40.0f, (float) b / numBands);
            const float f1 = 40.0f * std::pow (16000.0f / 40.0f, (float) (b + 1) / numBands);
            const int k0 = juce::jlimit (1, fftSize / 2 - 1, (int) (f0 * fftSize / sr));
            const int k1 = juce::jlimit (k0 + 1, fftSize / 2, (int) (f1 * fftSize / sr) + 1);
            float m = 0; for (int k = k0; k < k1; ++k) m = juce::jmax (m, fftData[(size_t) k]);
            const float dB = juce::Decibels::gainToDecibels (m / (fftSize * 0.25f), -90.0f);
            const float v = juce::jlimit (0.0f, 1.0f, (dB + 70.0f) / 70.0f);
            bands[(size_t) b] = juce::jmax (v, bands[(size_t) b] * 0.82f);
        }
    }

    void CentralVisualizer::timerCallback()
    {
        analyse();
        auto avg = [&] (int a, int b) { float s = 0; for (int i = a; i < b; ++i) s += bands[(size_t) i]; return s / (float) (b - a); };
        bass = avg (0, 8); mid = avg (8, 20); high = avg (20, numBands);

        time += 1.0 / 30.0;
        const float idle = 0.12f + 0.05f * std::sin ((float) time * 1.3f);
        const float target = juce::jmax (idle, juce::jlimit (0.0f, 1.0f, level * 3.5f));
        energy += (target - energy) * (target > energy ? 0.5f : 0.12f);
        rot += 0.004f + energy * 0.03f;

        if (level - prevLevel > 0.04f && rings.size() < 6) rings.push_back ({ 0.0f });   // transitorio -> anillo
        prevLevel = level;
        for (auto& r : rings) r.age += 0.035f;
        rings.erase (std::remove_if (rings.begin(), rings.end(), [] (const Ring& r) { return r.age >= 1.0f; }), rings.end());

        for (auto& p : particles) { p.angle += p.speed * (0.02f + energy * 0.12f); }
        repaint();
    }

    void CentralVisualizer::paint (juce::Graphics& g)
    {
        const auto b = getLocalBounds().toFloat();
        const auto c = b.getCentre();
        const float R = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f * 0.98f;
        const float t = (float) time;
        const float tau = juce::MathConstants<float>::twoPi;

        // halo exterior
        g.setGradientFill (juce::ColourGradient (green.withAlpha (0.10f + 0.25f * energy), c.x, c.y, juce::Colours::transparentBlack, c.x + R, c.y, true));
        g.fillEllipse (c.x - R, c.y - R, R * 2, R * 2);

        // anillos metálicos
        g.setColour (juce::Colour (0xff04100a)); g.fillEllipse (c.x - R * 0.93f, c.y - R * 0.93f, R * 1.86f, R * 1.86f);
        g.setColour (greenDark.withAlpha (0.8f)); g.drawEllipse (c.x - R * 0.93f, c.y - R * 0.93f, R * 1.86f, R * 1.86f, 2.0f);
        g.setColour (green.withAlpha (0.35f));    g.drawEllipse (c.x - R * 0.74f, c.y - R * 0.74f, R * 1.48f, R * 1.48f, 1.0f);

        // marcas
        for (int i = 0; i < 90; ++i)
        {
            const float a = tau * (float) i / 90.0f + rot * 0.2f, r0 = R * 0.955f, r1 = R * (i % 5 == 0 ? 0.985f : 0.97f);
            g.setColour (green.withAlpha (i % 5 == 0 ? 0.55f : 0.22f));
            g.drawLine (c.x + std::sin (a) * r0, c.y - std::cos (a) * r0, c.x + std::sin (a) * r1, c.y - std::cos (a) * r1, 1.0f);
        }

        // arcos de progreso giratorios (energía)
        {
            juce::Path arc; arc.addCentredArc (c.x, c.y, R * 0.88f, R * 0.88f, 0.0f, rot, rot + 1.2f + energy * 2.2f, true);
            glowStroke (g, arc, green, R * 0.022f, 1.0f);
            juce::Path arc2; arc2.addCentredArc (c.x, c.y, R * 0.88f, R * 0.88f, 0.0f, rot + 3.4f, rot + 3.4f + 0.7f + bass * 1.6f, true);
            glowStroke (g, arc2, cyan, R * 0.016f, 0.8f);
        }

        // espectro radial
        constexpr int bars = 64;
        for (int i = 0; i < bars; ++i)
        {
            const float pos = (float) i / bars * 2.0f;                       // espejo: 0..1..0
            const float fi = (pos <= 1.0f ? pos : 2.0f - pos) * (numBands - 1);
            const int i0 = (int) fi; const float fr = fi - (float) i0;
            const float v = bands[(size_t) i0] * (1.0f - fr) + bands[(size_t) juce::jmin (numBands - 1, i0 + 1)] * fr;
            const float a = tau * (float) i / bars - juce::MathConstants<float>::halfPi;
            const float r0 = R * 0.76f, r1 = r0 + R * (0.015f + 0.11f * v);
            g.setColour (green.interpolatedWith (cyan, (float) i / bars).withAlpha (0.45f + 0.5f * v));
            g.drawLine (c.x + std::cos (a) * r0, c.y + std::sin (a) * r0, c.x + std::cos (a) * r1, c.y + std::sin (a) * r1, R * 0.012f);
        }

        // anillos de energía (transitorios)
        for (auto& r : rings)
        {
            const float rr = R * (0.30f + r.age * 0.45f);
            g.setColour (cyan.withAlpha ((1.0f - r.age) * 0.55f));
            g.drawEllipse (c.x - rr, c.y - rr, rr * 2, rr * 2, 1.5f);
        }

        // esfera
        const float S = R * 0.62f;
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff021c12), c.x, c.y, juce::Colour (0xff0a7a40).withAlpha (0.85f), c.x + S, c.y, true));
        g.fillEllipse (c.x - S, c.y - S, S * 2, S * 2);

        {
            juce::Graphics::ScopedSaveState ss (g);
            juce::Path clip; clip.addEllipse (c.x - S, c.y - S, S * 2, S * 2);
            g.reduceClipRegion (clip);

            constexpr int curves = 12;
            for (int k = 0; k < curves; ++k)
            {
                const float e = bands[(size_t) ((k * 5 + 2) % numBands)];
                const float amp = S * (0.10f + 0.30f * (0.35f * energy + 0.65f * e));
                const float f1 = 2.0f + (float) k * 0.55f, f2 = 5.0f + (float) k * 0.9f;
                const float sp1 = 0.5f + (float) k * 0.11f, sp2 = 0.9f + (float) k * 0.07f;
                juce::Path p;
                for (int i = 0; i <= 40; ++i)
                {
                    const float u = (float) i / 40.0f;
                    const float x = c.x - S + u * S * 2.0f;
                    const float env = std::sin (u * juce::MathConstants<float>::pi);
                    const float y = c.y + ((float) k - curves * 0.5f) * S * 0.14f
                                  + (std::sin (u * f1 * 3.0f + t * sp1 + (float) k) * 0.7f + std::sin (u * f2 * 2.0f - t * sp2) * 0.3f) * amp * env;
                    if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
                }
                const auto col = green.interpolatedWith (cyan, (float) k / curves);
                g.setColour (col.withAlpha (0.10f + 0.08f * energy)); g.strokePath (p, juce::PathStrokeType (4.0f + 5.0f * energy));
                g.setColour (col.withAlpha (0.55f + 0.35f * energy)); g.strokePath (p, juce::PathStrokeType (1.0f + 1.2f * bass));
            }

            // núcleo brillante
            g.setGradientFill (juce::ColourGradient (cyan.withAlpha (0.25f * energy + 0.08f), c.x, c.y, juce::Colours::transparentBlack, c.x + S * 0.7f, c.y, true));
            g.fillEllipse (c.x - S, c.y - S, S * 2, S * 2);
        }

        // partículas
        for (auto& p : particles)
        {
            const float rr = R * p.radius * (0.9f + 0.1f * std::sin (t * 2.0f + p.hue * 9.0f)) * (0.85f + 0.3f * energy);
            const float x = c.x + std::cos (p.angle) * rr, y = c.y + std::sin (p.angle) * rr;
            const float sz = p.size * (0.7f + 1.2f * high);
            g.setColour (green.interpolatedWith (cyan, p.hue).withAlpha (0.25f + 0.55f * energy));
            g.fillEllipse (x - sz * 0.5f, y - sz * 0.5f, sz, sz);
        }

        // borde de la esfera
        g.setColour (juce::Colours::white.withAlpha (0.35f + 0.25f * energy));
        g.drawEllipse (c.x - S, c.y - S, S * 2, S * 2, 1.6f);
        g.setColour (cyan.withAlpha (0.25f)); g.drawEllipse (c.x - S * 1.02f, c.y - S * 1.02f, S * 2.04f, S * 2.04f, 3.0f);
    }
}
