#pragma once
#include "../Common.h"
#include "FactoryPresets.h"

namespace phaser
{
    // Presets de fábrica (código) + presets de usuario (XML en %APPDATA%\PHASER\Presets\*.phpreset).
    class PresetManager
    {
    public:
        struct Entry { juce::String name, category; bool factory = true; int factoryIndex = -1; juce::File file; };

        explicit PresetManager (juce::AudioProcessorValueTreeState& s) : apvts (s) {}

        void rescan();
        const std::vector<Entry>& entries() const noexcept { return list; }
        int  findCurrentIndex() const;
        juce::String displayName (int index) const;
        juce::String currentDisplayName() const;

        void loadIndex (int index);
        void step (int delta);
        void resetToInit();
        bool loadFromFile (const juce::File& f);
        bool saveUser (const juce::String& name, const juce::String& category);
        bool deleteUser (int index);

        static juce::File getUserFolder();
        static juce::StringArray getCategories();

    private:
        void applyFactory (const FactoryPreset& p);
        juce::AudioProcessorValueTreeState& apvts;
        std::vector<Entry> list;
    };
}
