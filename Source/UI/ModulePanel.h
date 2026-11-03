#pragma once
#include "UiUtil.h"
#include "CustomKnob.h"

namespace phaser::ui
{
    // Panel lateral con marco biselado. Se reconstruye con los controles del módulo activo.
    class ModulePanel : public juce::Component
    {
    public:
        struct Spec
        {
            enum class Type { Knob, Choice, Toggles };
            Type type = Type::Knob;
            juce::String id, label;
            std::vector<std::pair<juce::String, juce::String>> toggles;   // (paramId, texto)
        };

        ModulePanel (juce::AudioProcessorValueTreeState& s, bool rightSide) : apvts (s), right (rightSide) {}
        ~ModulePanel() override { clear(); }

        void setContent (const juce::String& title, bool cyanAccent, const juce::String& enableId, const std::vector<Spec>& specs);
        void paint (juce::Graphics&) override;
        void resized() override;

    private:
        void clear();
        juce::AudioProcessorValueTreeState& apvts;
        bool right, cyanAccent = false;
        juce::String title;
        juce::TextButton enableBtn { "ON" };
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> enableAtt;
        std::vector<std::unique_ptr<juce::Component>> rows;
    };
}
