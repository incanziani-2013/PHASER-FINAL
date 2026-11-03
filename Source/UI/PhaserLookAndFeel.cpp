#include "PhaserLookAndFeel.h"

namespace phaser::ui
{
    PhaserLookAndFeel::PhaserLookAndFeel()
    {
        setColour (juce::Label::textColourId, textCol);
        setColour (juce::ComboBox::textColourId, textCol);
        setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff03100a));
        setColour (juce::PopupMenu::textColourId, textCol);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, greenDark);
        setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
        setColour (juce::TooltipWindow::backgroundColourId, juce::Colour (0xff03100a));
        setColour (juce::TooltipWindow::textColourId, green);
        setColour (juce::TooltipWindow::outlineColourId, greenDark);
        setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff03100a));
        setColour (juce::TextEditor::textColourId, textCol);
        setColour (juce::TextEditor::outlineColourId, greenDark);
        setColour (juce::AlertWindow::backgroundColourId, juce::Colour (0xff03100a));
        setColour (juce::AlertWindow::textColourId, textCol);
        setColour (juce::AlertWindow::outlineColourId, greenDark);
        setColour (juce::TextButton::buttonColourId, juce::Colour (0xff06180f));
        setColour (juce::TextButton::textColourOffId, textCol);
        setColour (juce::TextButton::textColourOnId, juce::Colours::white);
    }

    void PhaserLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider& s)
    {
        const auto accent = (bool) s.getProperties().getWithDefault ("cyan", false) ? cyan : green;
        auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (2.0f);
        const float d = juce::jmin (b.getWidth(), b.getHeight()), r = d * 0.5f;
        const auto c = b.getCentre();
        const float ang = a0 + pos * (a1 - a0);
        const bool active = s.isMouseOverOrDragging();

        const float ar = r * 0.92f, th = juce::jmax (2.0f, r * 0.13f);
        juce::Path track; track.addCentredArc (c.x, c.y, ar, ar, 0.0f, a0, a1, true);
        g.setColour (dim.withAlpha (0.9f));
        g.strokePath (track, juce::PathStrokeType (th, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        if (pos > 0.002f)
        {
            juce::Path v; v.addCentredArc (c.x, c.y, ar, ar, 0.0f, a0, ang, true);
            g.setColour (accent.withAlpha (active ? 0.30f : 0.18f));
            g.strokePath (v, juce::PathStrokeType (th * 2.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setColour (accent);
            g.strokePath (v, juce::PathStrokeType (th, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        const float br = r * 0.68f;
        juce::ColourGradient body (juce::Colour (0xff33423b), c.x - br * 0.4f, c.y - br * 0.6f, juce::Colour (0xff040806), c.x + br * 0.6f, c.y + br * 0.8f, false);
        g.setGradientFill (body);
        g.fillEllipse (c.x - br, c.y - br, br * 2, br * 2);
        g.setColour (juce::Colour (0xff000000).withAlpha (0.8f));
        g.drawEllipse (c.x - br, c.y - br, br * 2, br * 2, 1.5f);
        g.setColour (accent.withAlpha (0.25f));
        g.drawEllipse (c.x - br * 0.86f, c.y - br * 0.86f, br * 1.72f, br * 1.72f, 1.0f);

        const float x0 = c.x + std::sin (ang) * br * 0.28f, y0 = c.y - std::cos (ang) * br * 0.28f;
        const float x1 = c.x + std::sin (ang) * br * 0.9f,  y1 = c.y - std::cos (ang) * br * 0.9f;
        g.setColour (accent.withAlpha (0.35f)); g.drawLine (x0, y0, x1, y1, th * 1.1f);
        g.setColour (juce::Colours::white.interpolatedWith (accent, 0.4f)); g.drawLine (x0, y0, x1, y1, juce::jmax (1.5f, th * 0.45f));
    }

    void PhaserLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool hi, bool down)
    {
        auto r = b.getLocalBounds().toFloat().reduced (1.5f);
        const bool on = b.getToggleState();
        const bool isDanger = (bool) b.getProperties().getWithDefault ("danger", false);
        const auto accent = isDanger ? danger : (bool) b.getProperties().getWithDefault ("cyan", false) ? cyan : green;
        auto p = chamfered (r, juce::jmin (8.0f, r.getHeight() * 0.28f));

        g.setColour (on ? accent.withAlpha (0.22f) : juce::Colour (0xff04120b));
        g.fillPath (p);
        if (hi || down) { g.setColour (accent.withAlpha (down ? 0.25f : 0.12f)); g.fillPath (p); }
        glowStroke (g, p, on ? accent : accent.withAlpha (0.55f), 1.4f, on ? 1.0f : 0.35f);
    }

    void PhaserLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool)
    {
        const bool on = b.getToggleState();
        const bool isDanger = (bool) b.getProperties().getWithDefault ("danger", false);
        const auto accent = isDanger ? danger : (bool) b.getProperties().getWithDefault ("cyan", false) ? cyan : green;
        g.setColour (on ? juce::Colours::white.interpolatedWith (accent, 0.35f) : accent.interpolatedWith (textCol, 0.4f));
        g.setFont (font (juce::jmin (16.0f, b.getHeight() * 0.42f)).withExtraKerningFactor (0.08f));
        g.drawText (b.getButtonText(), b.getLocalBounds(), juce::Justification::centred, true);
    }

    void PhaserLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
    {
        auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (1.5f);
        auto p = chamfered (r, 6.0f);
        g.setColour (juce::Colour (0xff04120b)); g.fillPath (p);
        glowStroke (g, p, green.withAlpha (0.7f), 1.2f, 0.3f);
        juce::Path arrow;
        const float ax = (float) w - 16.0f, ay = (float) h * 0.5f;
        arrow.addTriangle (ax - 5, ay - 3, ax + 5, ay - 3, ax, ay + 4);
        g.setColour (box.isEnabled() ? green : dim); g.fillPath (arrow);
    }

    void PhaserLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int w, int h)
    {
        g.fillAll (juce::Colour (0xff03100a));
        g.setColour (greenDark); g.drawRect (0, 0, w, h, 1);
    }

    void IconButton::paintButton (juce::Graphics& g, bool over, bool down)
    {
        auto r = getLocalBounds().toFloat().reduced (1.5f);
        auto frame = chamfered (r, 6.0f);
        g.setColour (juce::Colour (0xff04120b)); g.fillPath (frame);
        if (over || down) { g.setColour (green.withAlpha (down ? 0.25f : 0.12f)); g.fillPath (frame); }
        glowStroke (g, frame, green.withAlpha (0.7f), 1.2f, over ? 0.8f : 0.3f);

        auto c = r.getCentre(); const float u = juce::jmin (r.getWidth(), r.getHeight()) * 0.28f;
        juce::Path p;
        switch (icon)
        {
            case Icon::Folder:
                p.addRoundedRectangle (c.x - u * 1.1f, c.y - u * 0.5f, u * 2.2f, u * 1.5f, 2.0f);
                p.addRectangle (c.x - u * 1.1f, c.y - u * 0.95f, u * 0.9f, u * 0.5f);
                g.setColour (green); g.strokePath (p, juce::PathStrokeType (1.5f)); break;
            case Icon::Save:
                p.addRoundedRectangle (c.x - u, c.y - u, u * 2, u * 2, 2.0f);
                p.addRectangle (c.x - u * 0.55f, c.y - u, u * 1.1f, u * 0.7f);
                p.addRectangle (c.x - u * 0.55f, c.y + u * 0.2f, u * 1.1f, u * 0.8f);
                g.setColour (green); g.strokePath (p, juce::PathStrokeType (1.5f)); break;
            case Icon::Gear:
                p.addStar (c, 8, u * 0.85f, u * 1.15f);
                p.addEllipse (c.x - u * 0.38f, c.y - u * 0.38f, u * 0.76f, u * 0.76f);
                g.setColour (green); g.strokePath (p, juce::PathStrokeType (1.4f)); break;
        }
    }
}
