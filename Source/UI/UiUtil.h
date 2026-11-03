#pragma once
#include "../Common.h"

namespace phaser::ui
{
    inline const juce::Colour bg        { 0xff020604 };
    inline const juce::Colour panelBg   { 0xff04120b };
    inline const juce::Colour green     { 0xff2dff6a };
    inline const juce::Colour greenDark { 0xff0a8a35 };
    inline const juce::Colour cyan      { 0xff19e6e0 };
    inline const juce::Colour dim       { 0xff0c3a1f };
    inline const juce::Colour textCol   { 0xffd8fff0 };
    inline const juce::Colour danger    { 0xffff3b4d };
    inline const juce::Colour amber     { 0xffffb020 };

    inline juce::Font font (float h, bool bold = true)
    {
        return juce::Font (juce::FontOptions (h, bold ? juce::Font::bold : juce::Font::plain));
    }

    // Rectángulo con esquinas cortadas (look "hardware" futurista)
    inline juce::Path chamfered (juce::Rectangle<float> r, float c)
    {
        juce::Path p;
        p.startNewSubPath (r.getX() + c, r.getY());
        p.lineTo (r.getRight() - c, r.getY());
        p.lineTo (r.getRight(), r.getY() + c);
        p.lineTo (r.getRight(), r.getBottom() - c);
        p.lineTo (r.getRight() - c, r.getBottom());
        p.lineTo (r.getX() + c, r.getBottom());
        p.lineTo (r.getX(), r.getBottom() - c);
        p.lineTo (r.getX(), r.getY() + c);
        p.closeSubPath();
        return p;
    }

    inline void glowStroke (juce::Graphics& g, const juce::Path& p, juce::Colour c, float thickness, float glow = 1.0f)
    {
        g.setColour (c.withAlpha (0.10f * glow)); g.strokePath (p, juce::PathStrokeType (thickness * 5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (c.withAlpha (0.22f * glow)); g.strokePath (p, juce::PathStrokeType (thickness * 2.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (c);                          g.strokePath (p, juce::PathStrokeType (thickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
}
