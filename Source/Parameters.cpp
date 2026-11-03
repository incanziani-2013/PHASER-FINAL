#include "Parameters.h"

namespace phaser
{
    using namespace juce;
    using APF = AudioParameterFloat;

    static AudioParameterFloatAttributes pctAttr()
    {
        return AudioParameterFloatAttributes()
            .withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v * 100.0f)) + " %"; })
            .withValueFromStringFunction ([] (const String& s) { return s.retainCharacters ("0123456789.-").getFloatValue() / 100.0f; });
    }
    static AudioParameterFloatAttributes dbAttr()
    {
        return AudioParameterFloatAttributes()
            .withStringFromValueFunction ([] (float v, int) { return String (v, 1) + " dB"; })
            .withValueFromStringFunction ([] (const String& s) { return s.retainCharacters ("0123456789.-").getFloatValue(); });
    }
    static AudioParameterFloatAttributes hzAttr()
    {
        return AudioParameterFloatAttributes()
            .withStringFromValueFunction ([] (float v, int) { return v >= 1000.0f ? String (v / 1000.0f, 2) + " kHz" : String (v, v < 10.0f ? 2 : 0) + " Hz"; })
            .withValueFromStringFunction ([] (const String& s)
            {
                auto t = s.trim().toLowerCase(); float v = t.retainCharacters ("0123456789.").getFloatValue();
                return t.contains ("k") ? v * 1000.0f : v;
            });
    }
    static AudioParameterFloatAttributes degAttr()
    {
        return AudioParameterFloatAttributes()
            .withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v)) + " deg"; })
            .withValueFromStringFunction ([] (const String& s) { return s.retainCharacters ("0123456789.-").getFloatValue(); });
    }
    static AudioParameterFloatAttributes qAttr()
    {
        return AudioParameterFloatAttributes()
            .withStringFromValueFunction ([] (float v, int) { return String (v, 2); })
            .withValueFromStringFunction ([] (const String& s) { return s.getFloatValue(); });
    }

    AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
    {
        std::vector<std::unique_ptr<RangedAudioParameter>> p;

        auto flt = [&] (const char* id, const char* name, NormalisableRange<float> r, float def, AudioParameterFloatAttributes a)
        { p.push_back (std::make_unique<APF> (ParameterID { id, 1 }, name, r, def, a)); };
        auto pct = [&] (const char* id, const char* name, float def, float lo = 0.0f, float hi = 1.0f)
        { flt (id, name, NormalisableRange<float> (lo, hi, 0.001f), def, pctAttr()); };
        auto boolean = [&] (const char* id, const char* name, bool def)
        { p.push_back (std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, name, def)); };
        auto choice = [&] (const char* id, const char* name, StringArray items, int def)
        { p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, name, items, def)); };

        // ---- MASTER ----
        flt (ids::inGain,  "Input Gain",  { -24.0f, 24.0f, 0.1f }, 0.0f, dbAttr());
        flt (ids::outGain, "Output Gain", { -24.0f, 24.0f, 0.1f }, 0.0f, dbAttr());
        pct (ids::mix,     "Master Mix",  1.0f);
        pct (ids::mDrive,  "Master Drive", 0.0f);
        boolean (ids::limiter, "Output Limiter", true);
        pct (ids::stereo,  "Stereo Control", 1.0f, 0.0f, 2.0f);
        boolean (ids::bypass, "Global Bypass", false);

        // ---- MODULE 01 COLOR ----
        boolean (ids::colorOn, "Color On", true);
        pct (ids::cDrive,  "Color Drive",      0.30f);
        pct (ids::cTone,   "Color Tone",       0.50f);
        pct (ids::cSat,    "Color Saturation", 0.40f);
        pct (ids::cBlend,  "Color Blend",      0.50f);
        pct (ids::cWarm,   "Color Warmth",     0.00f);
        pct (ids::cBright, "Color Brightness", 0.00f);
        pct (ids::cHarm,   "Color Harmonics",  0.00f);

        // ---- MODULE 02 SPECTRUM ----
        boolean (ids::spectrumOn, "Spectrum On", true);
        flt (ids::sLow,  "Spectrum Low",  { -18.0f, 18.0f, 0.1f }, 0.0f, dbAttr());
        flt (ids::sMid,  "Spectrum Mid",  { -18.0f, 18.0f, 0.1f }, 0.0f, dbAttr());
        flt (ids::sHigh, "Spectrum High", { -18.0f, 18.0f, 0.1f }, 0.0f, dbAttr());
        pct (ids::sRes,  "Spectrum Resonance", 0.30f);
        flt (ids::sFreq, "Spectrum Frequency", { 20.0f, 20000.0f, 1.0f, 0.3f }, 1000.0f, hzAttr());
        flt (ids::sQ,    "Spectrum Q",         { 0.2f, 10.0f, 0.01f, 0.5f }, 1.0f, qAttr());
        choice (ids::sFilter, "Spectrum Filter", { "Off", "Low-pass", "High-pass", "Band-pass", "Peak", "Low shelf", "High shelf" }, 0);

        // ---- MODULE 03 MOTION ----
        boolean (ids::motionOn, "Motion On", false);
        pct (ids::mWidth,    "Motion Width",      1.0f, 0.0f, 2.0f);
        flt (ids::mRate,     "Motion Rate",       { 0.05f, 10.0f, 0.01f, 0.4f }, 0.5f, hzAttr());
        pct (ids::mDepth,    "Motion Depth",      0.50f);
        flt (ids::mPhase,    "Motion Phase",      { 0.0f, 180.0f, 1.0f }, 90.0f, degAttr());
        pct (ids::mFeedback, "Motion Feedback",   0.20f, 0.0f, 0.85f);
        pct (ids::mMod,      "Motion Modulation", 0.50f);
        pct (ids::mStereo,   "Motion Stereo",     0.30f);
        choice (ids::mMode,  "Motion Mode", { "Chorus", "Flanger" }, 0);

        // ---- MODULE 04 GLITCH ----
        boolean (ids::glitchOn, "Glitch On", false);
        pct (ids::gCrush,   "Glitch Crush",   0.50f);
        pct (ids::gRedux,   "Glitch Redux",   0.30f);
        pct (ids::gStutter, "Glitch Stutter", 0.30f);
        pct (ids::gNoise,   "Glitch Noise",   0.10f);
        choice (ids::gRate, "Glitch Rate", { "1/1", "1/2", "1/4", "1/8", "1/16", "1/32" }, 2);
        pct (ids::gDigital, "Glitch Digital", 0.30f);
        pct (ids::gMix,     "Glitch Mix",     0.50f);
        boolean (ids::gCrushOn,   "Glitch Crush Enable",   true);
        boolean (ids::gReduxOn,   "Glitch Redux Enable",   true);
        boolean (ids::gStutterOn, "Glitch Stutter Enable", true);
        boolean (ids::gNoiseOn,   "Glitch Noise Enable",   true);
        boolean (ids::gDigitalOn, "Glitch Digital Enable", true);

        return { std::make_move_iterator (p.begin()), std::make_move_iterator (p.end()) };
    }
}
