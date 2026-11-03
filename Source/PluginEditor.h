#pragma once
#include "PluginProcessor.h"
#include "UI/PhaserLookAndFeel.h"
#include "UI/CustomKnob.h"
#include "UI/ModulePanel.h"
#include "UI/CentralVisualizer.h"
#include "UI/Widgets.h"

class PhaserAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit PhaserAudioProcessorEditor (PhaserAudioProcessor&);
    ~PhaserAudioProcessorEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void selectModule (int index);
    void showPresetMenu();
    void showSettingsMenu();
    void showSaveDialog();
    void showLoadDialog();
    void refreshPresetText();

    PhaserAudioProcessor& proc;
    phaser::ui::PhaserLookAndFeel lnf;
    juce::TooltipWindow tooltip { this, 600 };

    phaser::ui::CentralVisualizer visualizer;
    phaser::ui::ModulePanel leftPanel, rightPanel;
    std::array<std::unique_ptr<phaser::ui::ModuleTab>, 4> tabs;
    int activeModule = 0;

    phaser::ui::KnobControl mixKnob, inKnob, driveKnob, stereoKnob, outKnob;
    juce::TextButton prevBtn { "<" }, nextBtn { ">" }, presetBtn, bypassBtn { "BYPASS" }, limiterBtn { "LIMITER" };
    phaser::ui::IconButton loadBtn { "load", phaser::ui::IconButton::Icon::Folder },
                           saveBtn { "save", phaser::ui::IconButton::Icon::Save },
                           settingsBtn { "settings", phaser::ui::IconButton::Icon::Gear };
    phaser::ui::LevelMeter inMeter { false }, outMeter { true };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAtt, limiterAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PhaserAudioProcessorEditor)
};
