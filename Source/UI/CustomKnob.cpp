#include "CustomKnob.h"

namespace phaser::ui
{
    KnobControl::KnobControl (juce::AudioProcessorValueTreeState& state, const juce::String& paramId, const juce::String& label,
                              bool cyanAcc, Layout l)
        : name (label), layout (l), cyanAccent (cyanAcc)
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
        slider.getProperties().set ("cyan", cyanAccent);
        slider.setVelocityBasedMode (false);
        slider.setScrollWheelEnabled (true);
        addAndMakeVisible (slider);

        valueLabel.setJustificationType (layout == Layout::Row ? juce::Justification::centredRight : juce::Justification::centred);
        valueLabel.setEditable (false, true, false);                 // doble clic para escribir un valor
        valueLabel.setColour (juce::Label::textColourId, textCol);
        valueLabel.setColour (juce::Label::backgroundWhenEditingColourId, juce::Colour (0xff03100a));
        valueLabel.setColour (juce::Label::textWhenEditingColourId, green);
        valueLabel.setColour (juce::Label::outlineWhenEditingColourId, greenDark);
        valueLabel.onTextChange = [this] { slider.setValue (slider.getValueFromText (valueLabel.getText()), juce::sendNotificationSync); };
        addAndMakeVisible (valueLabel);

        slider.onValueChange = [this]
        {
            valueLabel.setText (slider.getTextFromValue (slider.getValue()), juce::dontSendNotification);
            repaint();
        };

        attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, paramId, slider);
        slider.setDoubleClickReturnValue (true, state.getParameter (paramId)->convertFrom0to1 (state.getParameter (paramId)->getDefaultValue()));
        slider.setTooltip (state.getParameter (paramId)->getName (40) + "  (doble clic: reset)");
        valueLabel.setTooltip ("Doble clic para escribir un valor");
        slider.onValueChange();
    }

    void KnobControl::resized()
    {
        auto b = getLocalBounds();
        const float h = (float) b.getHeight();
        if (layout == Layout::Row)
        {
            slider.setBounds (b.removeFromLeft (b.getHeight()).reduced ((int) (h * 0.04f)));
            valueLabel.setFont (font (h * 0.2f, false));
            valueLabel.setBounds (b.getX() + (int) (h * 0.08f), (int) (h * 0.12f), b.getWidth() - (int) (h * 0.08f), (int) (h * 0.30f));
        }
        else
        {
            const int top = (int) (h * 0.2f), bot = (int) (h * 0.2f);
            slider.setBounds (b.withTrimmedTop (top).withTrimmedBottom (bot).withSizeKeepingCentre (b.getHeight() - top - bot, b.getHeight() - top - bot));
            valueLabel.setFont (font (h * 0.15f, false));
            valueLabel.setBounds (b.getX(), b.getBottom() - bot, b.getWidth(), bot);
        }
    }

    void KnobControl::paint (juce::Graphics& g)
    {
        const auto accent = cyanAccent ? cyan : green;
        const float h = (float) getHeight();
        if (layout == Layout::Row)
        {
            const float x = h + h * 0.08f, w = (float) getWidth() - x;
            g.setColour (textCol); g.setFont (font (h * 0.2f).withExtraKerningFactor (0.06f));
            g.drawText (name, juce::Rectangle<float> (x, h * 0.12f, w * 0.7f, h * 0.30f), juce::Justification::centredLeft, true);

            auto bar = juce::Rectangle<float> (x, h * 0.58f, w, juce::jmax (3.0f, h * 0.06f));
            g.setColour (dim); g.fillRoundedRectangle (bar, bar.getHeight() * 0.5f);
            const float prop = (float) slider.valueToProportionOfLength (slider.getValue());
            auto fill = bar.withWidth (bar.getWidth() * prop);
            g.setGradientFill (juce::ColourGradient (greenDark, fill.getX(), 0, accent, fill.getRight(), 0, false));
            g.fillRoundedRectangle (fill, bar.getHeight() * 0.5f);
            g.setColour (accent.withAlpha (0.25f)); g.fillRoundedRectangle (fill.expanded (0, 1.5f), bar.getHeight());
        }
        else
        {
            g.setColour (textCol); g.setFont (font (h * 0.15f).withExtraKerningFactor (0.08f));
            g.drawText (name, juce::Rectangle<float> (0, 0, (float) getWidth(), h * 0.2f), juce::Justification::centred, true);
        }
    }
}
