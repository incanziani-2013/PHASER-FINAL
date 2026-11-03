#pragma once
#include "UiUtil.h"

namespace phaser::ui
{
    class PhaserLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        PhaserLookAndFeel();

        void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float startAngle, float endAngle, juce::Slider&) override;
        void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool highlighted, bool down) override;
        void drawButtonText (juce::Graphics&, juce::TextButton&, bool highlighted, bool down) override;
        void drawComboBox (juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
        juce::Font getComboBoxFont (juce::ComboBox&) override { return font (13.0f); }
        juce::Font getPopupMenuFont() override { return font (14.0f); }
        void drawPopupMenuBackground (juce::Graphics&, int w, int h) override;
        juce::Font getLabelFont (juce::Label&) override { return font (13.0f); }
    };

    // Botón de icono vectorial (carpeta / disquete / engranaje)
    class IconButton : public juce::Button
    {
    public:
        enum class Icon { Folder, Save, Gear };
        IconButton (const juce::String& name, Icon i) : juce::Button (name), icon (i) {}
        void paintButton (juce::Graphics&, bool over, bool down) override;
    private:
        Icon icon;
    };
}
