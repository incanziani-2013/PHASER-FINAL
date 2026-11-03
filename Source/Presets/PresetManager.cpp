#include "PresetManager.h"

namespace phaser
{
    juce::File PresetManager::getUserFolder()
    {
        return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory).getChildFile ("PHASER").getChildFile ("Presets");
    }

    juce::StringArray PresetManager::getCategories()
    {
        return { "User", "Atmosphere", "Bass", "Distortion", "Glitch", "Lead", "Motion", "Texture", "Vocal", "Init" };
    }

    void PresetManager::rescan()
    {
        list.clear();
        int i = 0;
        for (auto& fp : getFactoryPresets())
        {
            Entry e; e.name = fp.name; e.category = fp.category; e.factory = true; e.factoryIndex = i++;
            list.push_back (e);
        }
        auto dir = getUserFolder();
        dir.createDirectory();
        auto files = dir.findChildFiles (juce::File::findFiles, false, "*.phpreset");
        files.sort();
        for (auto& f : files)
        {
            Entry e; e.name = f.getFileNameWithoutExtension(); e.category = "User"; e.factory = false; e.file = f;
            if (auto xml = juce::XmlDocument::parse (f))
                e.category = xml->getStringAttribute ("phCategory", "User");
            list.push_back (e);
        }
    }

    int PresetManager::findCurrentIndex() const
    {
        const auto name = apvts.state.getProperty ("presetName", "PHASER INIT").toString();
        for (int i = 0; i < (int) list.size(); ++i) if (list[(size_t) i].name == name) return i;
        return -1;
    }

    juce::String PresetManager::displayName (int index) const
    {
        if (index < 0 || index >= (int) list.size()) return "-- CUSTOM --";
        auto& e = list[(size_t) index];
        return e.factory ? juce::String::formatted ("%02d - ", e.factoryIndex + 1) + e.name.toUpperCase()
                         : "USER - " + e.name.toUpperCase();
    }

    juce::String PresetManager::currentDisplayName() const { return displayName (findCurrentIndex()); }

    void PresetManager::applyFactory (const FactoryPreset& p)
    {
        for (auto* prm : apvts.processor.getParameters())
            if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (prm))
                r->setValueNotifyingHost (r->getDefaultValue());

        for (auto& kv : p.values)
            if (auto* r = apvts.getParameter (juce::String (kv.first)))
                r->setValueNotifyingHost (r->convertTo0to1 (kv.second));

        apvts.state.setProperty ("presetName", juce::String (p.name), nullptr);
    }

    void PresetManager::loadIndex (int index)
    {
        if (index < 0 || index >= (int) list.size()) return;
        auto& e = list[(size_t) index];
        if (e.factory) applyFactory (getFactoryPresets()[(size_t) e.factoryIndex]);
        else           loadFromFile (e.file);
    }

    void PresetManager::step (int delta)
    {
        if (list.empty()) return;
        int cur = findCurrentIndex();
        if (cur < 0) cur = 0;
        const int n = (int) list.size();
        loadIndex (((cur + delta) % n + n) % n);
    }

    void PresetManager::resetToInit() { loadIndex ((int) getFactoryPresets().size() - 1); }

    bool PresetManager::loadFromFile (const juce::File& f)
    {
        auto xml = juce::XmlDocument::parse (f);
        if (xml == nullptr || ! xml->hasTagName (apvts.state.getType())) return false;

        const auto activeModule = apvts.state.getProperty ("activeModule", 0);
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
        apvts.state.setProperty ("activeModule", activeModule, nullptr);
        apvts.state.setProperty ("presetName", f.getFileNameWithoutExtension(), nullptr);
        return true;
    }

    bool PresetManager::saveUser (const juce::String& name, const juce::String& category)
    {
        auto clean = juce::File::createLegalFileName (name.trim());
        if (clean.isEmpty()) return false;

        apvts.state.setProperty ("presetName", clean, nullptr);
        auto state = apvts.copyState();
        state.removeProperty ("activeModule", nullptr);
        auto xml = state.createXml();
        if (xml == nullptr) return false;
        xml->setAttribute ("phCategory", category.isEmpty() ? "User" : category);

        auto dir = getUserFolder();
        if (! dir.createDirectory().wasOk()) return false;
        const bool ok = dir.getChildFile (clean + ".phpreset").replaceWithText (xml->toString());
        rescan();
        return ok;
    }

    bool PresetManager::deleteUser (int index)
    {
        if (index < 0 || index >= (int) list.size() || list[(size_t) index].factory) return false;
        const bool ok = list[(size_t) index].file.deleteFile();
        rescan();
        return ok;
    }
}
