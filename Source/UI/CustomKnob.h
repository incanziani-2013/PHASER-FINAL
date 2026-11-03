#pragma once
#include "UiUtil.h"

namespace phaser::ui
{
    // Control completo: knob propio + nombre + valor (doble clic en el valor = escribir) + barra de nivel.
    // Doble clic sobre el knob = reset al valor por defecto del parámetro.
    class KnobControl : public juce::Component
    {
    public:
        enum class Layout { Row, Column };
        KnobControl (juce::AudioProcessorValueTreeState& state, const juce::String& paramId, const juce::String& label,
                     bool cyanAccent = false, Layout layout = Layout::Row);
        void resized() override;
        void paint (juce::Graphics&) override;
    private:
        juce::Slider slider;
        juce::Label valueLabel;
        juce::String name;
        Layout layout;
        bool cyanAccent;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };
}
