#include "PluginEditor.h"

using namespace phaser;
using namespace phaser::ui;
using Spec = ModulePanel::Spec;

namespace
{
    constexpr float W = 1100.0f, H = 720.0f;

    Spec knob (const char* id, const char* label) { Spec s; s.type = Spec::Type::Knob; s.id = id; s.label = label; return s; }
    Spec choice (const char* id, const char* label) { Spec s; s.type = Spec::Type::Choice; s.id = id; s.label = label; return s; }
    Spec toggles (std::initializer_list<std::pair<const char*, const char*>> l)
    {
        Spec s; s.type = Spec::Type::Toggles;
        for (auto& t : l) s.toggles.push_back ({ t.first, t.second });
        return s;
    }

    const char* moduleNames[4]   = { "COLOR", "SPECTRUM", "MOTION", "GLITCH" };
    const char* enableIds[4]     = { ids::colorOn, ids::spectrumOn, ids::motionOn, ids::glitchOn };

    std::vector<Spec> leftSpecs (int m)
    {
        switch (m)
        {
            case 0:  return { knob (ids::cDrive, "DRIVE"), knob (ids::cTone, "TONE"), knob (ids::cSat, "SATURATION"), knob (ids::cBlend, "BLEND") };
            case 1:  return { knob (ids::sLow, "LOW"), knob (ids::sMid, "MID"), knob (ids::sHigh, "HIGH"), knob (ids::sRes, "RESONANCE") };
            case 2:  return { knob (ids::mWidth, "WIDTH"), knob (ids::mRate, "RATE"), knob (ids::mDepth, "DEPTH"), knob (ids::mPhase, "PHASE") };
            default: return { knob (ids::gCrush, "CRUSH"), knob (ids::gRedux, "REDUX"), knob (ids::gStutter, "STUTTER"), knob (ids::gNoise, "NOISE") };
        }
    }
    std::vector<Spec> rightSpecs (int m)
    {
        switch (m)
        {
            case 0:  return { knob (ids::cWarm, "WARMTH"), knob (ids::cBright, "BRIGHTNESS"), knob (ids::cHarm, "HARMONICS") };
            case 1:  return { knob (ids::sFreq, "FREQUENCY"), knob (ids::sQ, "Q"), choice (ids::sFilter, "FILTER") };
            case 2:  return { knob (ids::mFeedback, "FEEDBACK"), knob (ids::mMod, "MODULATION"), knob (ids::mStereo, "STEREO"), choice (ids::mMode, "MODE") };
            default: return { knob (ids::gDigital, "DIGITAL"), knob (ids::gMix, "MIX"), choice (ids::gRate, "RATE"),
                              toggles ({ { ids::gCrushOn, "CRUSH" }, { ids::gReduxOn, "REDUX" }, { ids::gStutterOn, "STUT" }, { ids::gNoiseOn, "NOISE" }, { ids::gDigitalOn, "DIGI" } }) };
        }
    }
}

PhaserAudioProcessorEditor::PhaserAudioProcessorEditor (PhaserAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p), visualizer (p),
      leftPanel (p.apvts, false), rightPanel (p.apvts, true),
      mixKnob (p.apvts, ids::mix, "MIX", false, KnobControl::Layout::Column),
      inKnob (p.apvts, ids::inGain, "INPUT", false, KnobControl::Layout::Column),
      driveKnob (p.apvts, ids::mDrive, "DRIVE", false, KnobControl::Layout::Column),
      stereoKnob (p.apvts, ids::stereo, "STEREO", true, KnobControl::Layout::Column),
      outKnob (p.apvts, ids::outGain, "OUTPUT", true, KnobControl::Layout::Column)
{
    setLookAndFeel (&lnf);

    addAndMakeVisible (visualizer);
    addAndMakeVisible (leftPanel); addAndMakeVisible (rightPanel);
    for (int i = 0; i < 4; ++i)
    {
        tabs[(size_t) i] = std::make_unique<ModuleTab> (moduleNames[i], i, p.apvts.getParameter (enableIds[i]));
        tabs[(size_t) i]->onClick = [this, i] { selectModule (i); };
        tabs[(size_t) i]->setTooltip (juce::String ("Módulo 0") + juce::String (i + 1) + " - " + moduleNames[i]);
        addAndMakeVisible (*tabs[(size_t) i]);
    }
    for (auto* k : { &mixKnob, &inKnob, &driveKnob, &stereoKnob, &outKnob }) addAndMakeVisible (*k);

    for (auto* b : { &prevBtn, &nextBtn, &presetBtn, &bypassBtn, &limiterBtn }) addAndMakeVisible (*b);
    for (auto* b : { (juce::Button*) &loadBtn, (juce::Button*) &saveBtn, (juce::Button*) &settingsBtn }) addAndMakeVisible (*b);
    addAndMakeVisible (inMeter); addAndMakeVisible (outMeter);

    bypassBtn.setClickingTogglesState (true); bypassBtn.getProperties().set ("danger", true);
    limiterBtn.setClickingTogglesState (true);
    bypassAtt  = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, ids::bypass, bypassBtn);
    limiterAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, ids::limiter, limiterBtn);

    prevBtn.onClick = [this] { proc.presets.step (-1); refreshPresetText(); };
    nextBtn.onClick = [this] { proc.presets.step (1);  refreshPresetText(); };
    presetBtn.onClick = [this] { showPresetMenu(); };
    loadBtn.onClick = [this] { showLoadDialog(); };
    saveBtn.onClick = [this] { showSaveDialog(); };
    settingsBtn.onClick = [this] { showSettingsMenu(); };

    prevBtn.setTooltip ("Preset anterior"); nextBtn.setTooltip ("Preset siguiente"); presetBtn.setTooltip ("Elegir preset");
    loadBtn.setTooltip ("Cargar preset desde archivo"); saveBtn.setTooltip ("Guardar preset de usuario");
    settingsBtn.setTooltip ("Ajustes"); bypassBtn.setTooltip ("Bypass global (con fundido)"); limiterBtn.setTooltip ("Limitador de salida (techo -0.5 dBFS)");

    setResizeLimits (825, 540, 1650, 1080);
    getConstrainer()->setFixedAspectRatio ((double) W / (double) H);
    setSize ((int) W, (int) H);

    selectModule ((int) p.apvts.state.getProperty ("activeModule", 0));
    refreshPresetText();
    startTimerHz (30);
}

PhaserAudioProcessorEditor::~PhaserAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void PhaserAudioProcessorEditor::selectModule (int index)
{
    activeModule = juce::jlimit (0, 3, index);
    proc.apvts.state.setProperty ("activeModule", activeModule, nullptr);
    for (int i = 0; i < 4; ++i) tabs[(size_t) i]->setActive (i == activeModule);
    const bool cy = (activeModule == 1 || activeModule == 2);
    leftPanel.setContent (moduleNames[activeModule], cy, enableIds[activeModule], leftSpecs (activeModule));
    rightPanel.setContent (juce::String (moduleNames[activeModule]) + " / DETAIL", ! cy, {}, rightSpecs (activeModule));
}

void PhaserAudioProcessorEditor::refreshPresetText()
{
    presetBtn.setButtonText (proc.presets.currentDisplayName());
}

void PhaserAudioProcessorEditor::timerCallback()
{
    inMeter.setLevels (proc.inPeakL.exchange (0.0f), proc.inPeakR.exchange (0.0f), 0.0f);
    outMeter.setLevels (proc.outPeakL.exchange (0.0f), proc.outPeakR.exchange (0.0f), proc.reductionDb.load());
    const auto name = proc.presets.currentDisplayName();
    if (name != presetBtn.getButtonText()) presetBtn.setButtonText (name);
}

void PhaserAudioProcessorEditor::showPresetMenu()
{
    proc.presets.rescan();
    const auto& list = proc.presets.entries();
    juce::PopupMenu menu;
    for (auto& cat : phaser::PresetManager::getCategories())
    {
        juce::PopupMenu sub; bool any = false;
        for (int i = 0; i < (int) list.size(); ++i)
            if (list[(size_t) i].category == cat) { sub.addItem (i + 1, proc.presets.displayName (i), true, i == proc.presets.findCurrentIndex()); any = true; }
        if (any) menu.addSubMenu (cat.toUpperCase(), sub);
    }
    juce::Component::SafePointer<PhaserAudioProcessorEditor> safe (this);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&presetBtn), [safe] (int r)
    {
        if (safe != nullptr && r > 0) { safe->proc.presets.loadIndex (r - 1); safe->refreshPresetText(); }
    });
}

void PhaserAudioProcessorEditor::showSettingsMenu()
{
    proc.presets.rescan();
    juce::PopupMenu menu, del;
    const auto& list = proc.presets.entries();
    bool anyUser = false;
    for (int i = 0; i < (int) list.size(); ++i)
        if (! list[(size_t) i].factory) { del.addItem (1000 + i, list[(size_t) i].name); anyUser = true; }
    menu.addItem (1, "Reset to PHASER INIT");
    menu.addSubMenu ("Delete user preset", del, anyUser);
    menu.addItem (2, "Open presets folder");
    menu.addSeparator();
    menu.addItem (3, "About PHASER V1 PRO");

    juce::Component::SafePointer<PhaserAudioProcessorEditor> safe (this);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&settingsBtn), [safe] (int r)
    {
        if (safe == nullptr || r == 0) return;
        auto& pm = safe->proc.presets;
        if (r == 1) { pm.resetToInit(); safe->refreshPresetText(); }
        else if (r == 2) { PresetManager::getUserFolder().createDirectory(); PresetManager::getUserFolder().revealToUser(); }
        else if (r == 3) juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon, "PHASER V1 PRO",
                                "ADVANCED COLOR PROCESSOR\nVST3 - 4 modules: Color / Spectrum / Motion / Glitch\nBuilt with JUCE.");
        else if (r >= 1000) pm.deleteUser (r - 1000);
    });
}

void PhaserAudioProcessorEditor::showSaveDialog()
{
    auto* w = new juce::AlertWindow ("Save preset", "Name and category for your preset:", juce::MessageBoxIconType::NoIcon);
    w->addTextEditor ("name", proc.apvts.state.getProperty ("presetName", "My Preset").toString(), "Name:");
    w->addComboBox ("cat", PresetManager::getCategories(), "Category:");
    w->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey));
    w->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    juce::Component::SafePointer<PhaserAudioProcessorEditor> safe (this);
    w->enterModalState (true, juce::ModalCallbackFunction::create ([safe, w] (int r)
    {
        if (safe == nullptr || r != 1) return;
        const auto name = w->getTextEditorContents ("name");
        const auto cat  = w->getComboBoxComponent ("cat")->getText();
        safe->proc.presets.saveUser (name, cat);
        safe->refreshPresetText();
    }), true);
}

void PhaserAudioProcessorEditor::showLoadDialog()
{
    auto chooser = std::make_shared<juce::FileChooser> ("Load PHASER preset", PresetManager::getUserFolder(), "*.phpreset");
    juce::Component::SafePointer<PhaserAudioProcessorEditor> safe (this);
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [safe, chooser] (const juce::FileChooser& fc)
    {
        auto f = fc.getResult();
        if (safe != nullptr && f.existsAsFile()) { safe->proc.presets.loadFromFile (f); safe->refreshPresetText(); }
    });
}

void PhaserAudioProcessorEditor::paint (juce::Graphics& g)
{
    const float s = (float) getWidth() / W;
    g.addTransform (juce::AffineTransform::scale (s));

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff010302), 0, 0, juce::Colour (0xff03140b), 0, H, false));
    g.fillRect (0.0f, 0.0f, W, H);
    g.setGradientFill (juce::ColourGradient (green.withAlpha (0.10f), W * 0.5f, H * 0.45f, juce::Colours::transparentBlack, W * 0.5f, H * 1.1f, true));
    g.fillRect (0.0f, 0.0f, W, H);

    // rejilla sutil
    g.setColour (green.withAlpha (0.025f));
    for (float x = 0; x < W; x += 40) g.drawVerticalLine ((int) x, 0.0f, H);
    for (float y = 0; y < H; y += 40) g.drawHorizontalLine ((int) y, 0.0f, W);

    // logo: estrella + PHASER
    juce::Path star; star.addStar ({ 62.0f, 52.0f }, 4, 9.0f, 34.0f);
    glowStroke (g, star, green, 2.0f, 1.0f);
    g.setColour (green.withAlpha (0.25f)); g.fillPath (star);
    g.setColour (juce::Colour (0xffe6ecea)); g.setFont (font (62.0f).withExtraKerningFactor (0.14f));
    g.drawText ("PHASER", juce::Rectangle<float> (104.0f, 6.0f, 300.0f, 70.0f), juce::Justification::centredLeft, false);
    g.setColour (green); g.setFont (font (11.5f).withExtraKerningFactor (0.3f));
    g.drawText ("ADVANCED COLOR PROCESSOR", juce::Rectangle<float> (106.0f, 66.0f, 300.0f, 18.0f), juce::Justification::centredLeft, false);
    g.setColour (cyan.withAlpha (0.8f)); g.setFont (font (11.0f));
    g.drawText ("V1 PRO", juce::Rectangle<float> (330.0f, 12.0f, 60.0f, 16.0f), juce::Justification::centredRight, false);

    // líneas decorativas
    juce::Path deco; deco.startNewSubPath (0, 112); deco.lineTo (60, 112); deco.lineTo (84, 126); deco.lineTo (W - 84, 126); deco.lineTo (W - 60, 112); deco.lineTo (W, 112);
    g.setColour (green.withAlpha (0.35f)); g.strokePath (deco, juce::PathStrokeType (1.0f));
    juce::Path deco2; deco2.startNewSubPath (0, 608); deco2.lineTo (190, 608); deco2.lineTo (205, 560);
    deco2.startNewSubPath (W, 608); deco2.lineTo (W - 190, 608); deco2.lineTo (W - 205, 560);
    g.setColour (green.withAlpha (0.3f)); g.strokePath (deco2, juce::PathStrokeType (1.0f));

    // marco de la barra inferior
    auto bar = chamfered (juce::Rectangle<float> (196.0f, 614.0f, 708.0f, 96.0f), 22.0f);
    g.setColour (juce::Colour (0xff04120b).withAlpha (0.9f)); g.fillPath (bar);
    glowStroke (g, bar, green.withAlpha (0.8f), 1.4f, 0.6f);
    g.setColour (green.withAlpha (0.25f));
    for (int i = 1; i < 5; ++i) g.drawVerticalLine (196 + 118 * i + (i > 2 ? 0 : 0), 630.0f, 696.0f);
}

void PhaserAudioProcessorEditor::resized()
{
    const float s = (float) getWidth() / W;
    auto R = [s] (float x, float y, float w, float h) { return juce::Rectangle<int> (juce::roundToInt (x * s), juce::roundToInt (y * s), juce::roundToInt (w * s), juce::roundToInt (h * s)); };

    prevBtn.setBounds (R (420, 24, 48, 40));
    presetBtn.setBounds (R (474, 24, 262, 40));
    nextBtn.setBounds (R (742, 24, 48, 40));
    loadBtn.setBounds (R (106, 86, 40, 32)); saveBtn.setBounds (R (152, 86, 40, 32)); settingsBtn.setBounds (R (198, 86, 40, 32));
    mixKnob.setBounds (R (806, 4, 96, 112));
    bypassBtn.setBounds (R (944, 26, 126, 40));

    leftPanel.setBounds (R (14, 128, 300, 420));
    rightPanel.setBounds (R (786, 128, 300, 420));
    visualizer.setBounds (R (330, 122, 440, 440));

    const float tw = 168, gap = 8, x0 = 550.0f - (4 * tw + 3 * gap) / 2.0f;
    for (int i = 0; i < 4; ++i) tabs[(size_t) i]->setBounds (R (x0 + (float) i * (tw + gap), 560, tw, 46));

    inKnob.setBounds (R (204, 618, 108, 90)); driveKnob.setBounds (R (322, 618, 108, 90));
    stereoKnob.setBounds (R (440, 618, 108, 90)); outKnob.setBounds (R (558, 618, 108, 90));
    limiterBtn.setBounds (R (704, 650, 180, 36));
    inMeter.setBounds (R (30, 618, 150, 90)); outMeter.setBounds (R (920, 618, 150, 90));
}
