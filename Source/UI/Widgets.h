#pragma once
#include "UiUtil.h"

namespace phaser::ui
{
    // Medidor estéreo segmentado con retención de pico, LED de clip y barra de reducción del limitador.
    class LevelMeter : public juce::Component
    {
    public:
        explicit LevelMeter (bool showReduction) : withGR (showReduction) {}
        void setLevels (float l, float r, float gr)
        {
            auto upd = [] (float& lvl, float& pk, float v) { lvl = juce::jmax (v, lvl * 0.80f); if (lvl >= pk) pk = lvl; else pk *= 0.97f; };
            upd (lv[0], pk[0], l); upd (lv[1], pk[1], r);
            if (l >= 1.0f || r >= 1.0f) clip = true;
            reduction = gr; repaint();
        }
        void mouseDown (const juce::MouseEvent&) override { clip = false; repaint(); }
        void paint (juce::Graphics& g) override
        {
            auto b = getLocalBounds().toFloat();
            const float w = b.getWidth(), h = b.getHeight();
            const int segs = 24;
            const float barW = w * (withGR ? 0.26f : 0.34f), top = h * 0.16f, bot = h * 0.86f, segH = (bot - top) / segs;
            for (int ch = 0; ch < 2; ++ch)
            {
                const float x = w * 0.08f + (float) ch * (barW + w * 0.06f);
                const float db = juce::Decibels::gainToDecibels (lv[ch], -60.0f), pdb = juce::Decibels::gainToDecibels (pk[ch], -60.0f);
                const int lit = juce::roundToInt (juce::jmap (db, -60.0f, 0.0f, 0.0f, (float) segs));
                const int peakSeg = juce::roundToInt (juce::jmap (pdb, -60.0f, 0.0f, 0.0f, (float) segs));
                for (int s = 0; s < segs; ++s)
                {
                    const float y = bot - (float) (s + 1) * segH;
                    auto col = s > segs - 3 ? danger : s > segs - 7 ? amber : (s > segs / 2 ? green : greenDark);
                    const bool on = s < lit || s == peakSeg - 1;
                    g.setColour (on ? col : dim.withAlpha (0.55f));
                    g.fillRect (x, y + 1.0f, barW, segH - 2.0f);
                }
            }
            if (withGR)
            {
                const float x = w * 0.08f + 2.0f * (barW + w * 0.06f) + w * 0.04f, bw = w * 0.14f;
                g.setColour (dim.withAlpha (0.55f)); g.fillRect (x, top, bw, bot - top);
                const float frac = juce::jlimit (0.0f, 1.0f, -reduction / 18.0f);
                g.setColour (cyan); g.fillRect (x, top, bw, (bot - top) * frac);
                g.setColour (textCol.withAlpha (0.7f)); g.setFont (font (h * 0.1f)); g.drawText ("GR", (int) x - 6, (int) (bot + 2), (int) bw + 12, (int) (h * 0.12f), juce::Justification::centred);
            }
            g.setColour (clip ? danger : dim); g.fillEllipse (w * 0.08f, h * 0.04f, h * 0.07f, h * 0.07f);
            g.setColour (textCol.withAlpha (0.7f)); g.setFont (font (h * 0.1f).withExtraKerningFactor (0.1f));
            g.drawText (withGR ? "OUT" : "IN", (int) (w * 0.25f), 0, (int) (w * 0.5f), (int) (h * 0.14f), juce::Justification::centred);
        }
    private:
        bool withGR, clip = false;
        float lv[2] { 0, 0 }, pk[2] { 0, 0 }, reduction = 0;
    };

    // Pestaña de módulo con icono, estado activo y LED de "módulo encendido".
    class ModuleTab : public juce::Button
    {
    public:
        ModuleTab (const juce::String& name, int iconIndex, juce::RangedAudioParameter* enableParam)
            : juce::Button (name), icon (iconIndex)
        {
            if (enableParam != nullptr)
            {
                att = std::make_unique<juce::ParameterAttachment> (*enableParam, [this] (float v) { ledOn = v > 0.5f; repaint(); });
                att->sendInitialUpdate();
            }
        }
        void setActive (bool a) { active = a; repaint(); }
        void paintButton (juce::Graphics& g, bool over, bool) override
        {
            const auto accent = icon == 1 || icon == 2 ? cyan : green;
            auto r = getLocalBounds().toFloat().reduced (2.0f);
            auto p = chamfered (r, r.getHeight() * 0.3f);
            g.setColour (active ? accent.withAlpha (0.16f) : juce::Colour (0xff04120b)); g.fillPath (p);
            if (over && ! active) { g.setColour (accent.withAlpha (0.08f)); g.fillPath (p); }
            glowStroke (g, p, active ? accent : accent.withAlpha (0.4f), active ? 2.0f : 1.0f, active ? 1.0f : 0.2f);

            const float h = r.getHeight(), ix = r.getX() + h * 0.7f, iy = r.getCentreY(), u = h * 0.26f;
            g.setColour (accent);
            juce::Path ic;
            switch (icon)
            {
                case 0: for (int i = 0; i < 3; ++i) { const float a = juce::MathConstants<float>::twoPi * (float) i / 3.0f - 1.57f; ic.addEllipse (ix + std::cos (a) * u * 0.5f - u * 0.75f, iy + std::sin (a) * u * 0.5f - u * 0.75f, u * 1.5f, u * 1.5f); } g.strokePath (ic, juce::PathStrokeType (1.2f)); break;
                case 1: for (int i = 0; i < 7; ++i) { const float hh = u * (0.5f + 1.2f * std::abs (std::sin ((float) i * 1.3f))); g.fillRect (ix - u + (float) i * u * 0.33f, iy - hh * 0.5f, 1.8f, hh); } break;
                case 2: ic.startNewSubPath (ix - u * 1.2f, iy); for (int i = 0; i <= 20; ++i) { const float x = (float) i / 20.0f; ic.lineTo (ix - u * 1.2f + x * u * 2.4f, iy - std::sin (x * 6.28f) * u * 0.8f); } g.strokePath (ic, juce::PathStrokeType (1.6f)); break;
                default: for (int a = 0; a < 3; ++a) for (int b = 0; b < 3; ++b) if ((a + b) % 2 == 0) g.fillRect (ix - u + (float) a * u * 0.7f, iy - u + (float) b * u * 0.7f, u * 0.65f, u * 0.65f); break;
            }
            g.setColour (active ? juce::Colours::white.interpolatedWith (accent, 0.3f) : accent.interpolatedWith (textCol, 0.4f));
            g.setFont (font (h * 0.36f).withExtraKerningFactor (0.08f));
            g.drawText (getName(), juce::Rectangle<float> (ix + u * 1.8f, r.getY(), r.getRight() - ix - u * 2.6f, h), juce::Justification::centredLeft, true);
            g.setColour (ledOn ? accent : dim); g.fillEllipse (r.getRight() - h * 0.42f, r.getCentreY() - h * 0.07f, h * 0.14f, h * 0.14f);
        }
    private:
        int icon; bool active = false, ledOn = false;
        std::unique_ptr<juce::ParameterAttachment> att;
    };
}
