#pragma once
#include "../Parameters.h"
#include <string>
#include <utility>
#include <vector>

namespace phaser
{
    // Cada preset parte de los valores por defecto (= PHASER INIT) y solo lista lo que cambia.
    // Los valores son "reales" (dB, Hz, 0..1, índice de opción, 0/1 para botones).
    struct FactoryPreset
    {
        std::string name, category;
        std::vector<std::pair<std::string, float>> values;
    };

    inline const std::vector<FactoryPreset>& getFactoryPresets()
    {
        using namespace ids;
        static const std::vector<FactoryPreset> presets =
        {
            { "Chromatic Void", "Atmosphere", {
                { cDrive, .5f }, { cTone, .7f }, { cSat, .6f }, { cBlend, 1.f }, { cWarm, .4f }, { cBright, .5f }, { cHarm, .5f },
                { sLow, 3.f }, { sHigh, 4.f }, { sRes, .4f },
                { motionOn, 1.f }, { mMode, 0.f }, { mRate, .3f }, { mDepth, .6f }, { mMod, .5f }, { mWidth, 1.4f } } },

            { "Neon Distortion", "Distortion", {
                { cDrive, .85f }, { cSat, .9f }, { cTone, .65f }, { cBlend, .8f }, { cHarm, .7f }, { cBright, .6f },
                { sHigh, 3.f }, { sFilter, 4.f }, { sFreq, 3000.f }, { sRes, .6f }, { mDrive, .3f } } },

            { "Dark Atmosphere", "Atmosphere", {
                { cDrive, .3f }, { cTone, .25f }, { cWarm, .7f }, { cBright, .1f }, { cSat, .4f },
                { sLow, 3.f }, { sHigh, -8.f }, { sFilter, 1.f }, { sFreq, 4500.f }, { sRes, .3f },
                { motionOn, 1.f }, { mMode, 0.f }, { mRate, .15f }, { mDepth, .8f }, { mFeedback, .3f }, { mMod, .6f }, { mWidth, 1.6f } } },

            { "Digital Collapse", "Glitch", {
                { cDrive, .5f },
                { glitchOn, 1.f }, { gCrush, .7f }, { gRedux, .5f }, { gStutter, .5f }, { gNoise, .2f }, { gRate, 3.f }, { gDigital, .6f }, { gMix, .8f } } },

            { "Crystal Motion", "Motion", {
                { cDrive, .15f }, { cBright, .7f },
                { sHigh, 5.f }, { sFilter, 6.f }, { sFreq, 8000.f }, { sRes, .3f },
                { motionOn, 1.f }, { mMode, 1.f }, { mRate, .8f }, { mDepth, .5f }, { mFeedback, .5f }, { mMod, .7f }, { mStereo, .4f }, { mPhase, 90.f }, { mWidth, 1.5f } } },

            { "Underground Texture", "Texture", {
                { cDrive, .6f }, { cSat, .7f }, { cWarm, .6f }, { cTone, .35f },
                { sLow, 4.f }, { sHigh, -4.f },
                { glitchOn, 1.f }, { gNoise, .35f }, { gCrush, .3f }, { gRedux, .2f }, { gMix, .4f }, { gStutterOn, 0.f }, { gDigitalOn, 0.f } } },

            { "Hyper Saturation", "Distortion", {
                { cDrive, 1.f }, { cSat, 1.f }, { cHarm, .9f }, { cBlend, .9f }, { cTone, .55f }, { cWarm, .3f },
                { mDrive, .5f }, { limiter, 1.f }, { outGain, -3.f } } },

            { "Broken Signal", "Glitch", {
                { glitchOn, 1.f }, { gCrush, .55f }, { gRedux, .75f }, { gStutter, .7f }, { gNoise, .3f }, { gDigital, .8f }, { gMix, .9f }, { gRate, 4.f },
                { sFilter, 3.f }, { sFreq, 2500.f }, { sRes, .7f } } },

            { "Space Manipulation", "Motion", {
                { cDrive, .1f }, { sLow, -2.f },
                { motionOn, 1.f }, { mMode, 0.f }, { mRate, .25f }, { mDepth, 1.f }, { mPhase, 180.f }, { mFeedback, .4f }, { mMod, .8f }, { mStereo, .6f }, { mWidth, 1.8f } } },

            { "Future Lead", "Lead", {
                { cDrive, .55f }, { cSat, .5f }, { cBright, .7f }, { cHarm, .6f }, { cTone, .65f },
                { sMid, 2.5f }, { sFreq, 2500.f }, { sQ, 1.2f }, { sHigh, 3.f },
                { motionOn, 1.f }, { mMode, 0.f }, { mRate, 2.5f }, { mDepth, .2f }, { mMod, .3f }, { mWidth, 1.3f } } },

            { "Ambient Dreams", "Atmosphere", {
                { cDrive, .15f }, { cSat, .25f }, { cWarm, .5f }, { cTone, .45f },
                { sLow, 1.f }, { sHigh, 2.f },
                { motionOn, 1.f }, { mMode, 0.f }, { mRate, .12f }, { mDepth, .9f }, { mPhase, 120.f }, { mFeedback, .45f }, { mMod, .7f }, { mStereo, .5f }, { mWidth, 1.7f } } },

            { "Dirty 808", "Bass", {
                { cDrive, .75f }, { cSat, .85f }, { cWarm, .8f }, { cTone, .3f }, { cBright, .1f }, { cHarm, .4f },
                { sLow, 4.f }, { sMid, -2.f }, { sFreq, 400.f }, { sQ, .8f },
                { stereo, .6f }, { outGain, -2.f } } },

            { "Vocal Mutation", "Vocal", {
                { cDrive, .4f }, { cSat, .5f }, { cHarm, .6f }, { cBright, .5f },
                { sFilter, 3.f }, { sFreq, 1800.f }, { sRes, .5f },
                { motionOn, 1.f }, { mMode, 0.f }, { mRate, .6f }, { mDepth, .3f }, { mMod, .4f },
                { glitchOn, 1.f }, { gCrush, .4f }, { gRedux, .3f }, { gStutterOn, 0.f }, { gNoise, .1f }, { gDigital, .3f }, { gMix, .35f } } },

            { "Glitch Machine", "Glitch", {
                { cDrive, .2f },
                { glitchOn, 1.f }, { gStutter, 1.f }, { gRate, 3.f }, { gCrush, .4f }, { gRedux, .4f }, { gNoiseOn, 0.f }, { gDigital, .4f }, { gMix, 1.f } } },

            { "Metallic Dreams", "Texture", {
                { cDrive, .4f }, { cSat, .5f }, { cBright, .6f },
                { sFilter, 4.f }, { sFreq, 2200.f }, { sRes, .8f },
                { motionOn, 1.f }, { mMode, 1.f }, { mRate, .5f }, { mDepth, .7f }, { mFeedback, .8f }, { mMod, 1.f }, { mStereo, .7f }, { mWidth, 1.4f } } },

            { "Deep Spectrum", "Atmosphere", {
                { cDrive, .2f }, { cWarm, .5f },
                { sLow, 6.f }, { sMid, -3.f }, { sHigh, 5.f }, { sFreq, 800.f }, { sQ, .7f }, { sFilter, 4.f }, { sRes, .3f } } },

            { "Cybernetic", "Lead", {
                { cDrive, .5f }, { cBright, .6f },
                { motionOn, 1.f }, { mMode, 1.f }, { mRate, 1.2f }, { mDepth, .4f }, { mFeedback, .6f }, { mMod, .5f },
                { glitchOn, 1.f }, { gCrush, .5f }, { gRedux, .3f }, { gDigital, .5f }, { gStutter, .3f }, { gRate, 4.f }, { gMix, .55f }, { gNoiseOn, 0.f } } },

            { "Frozen Signal", "Atmosphere", {
                { sHigh, 3.f },
                { motionOn, 1.f }, { mMode, 0.f }, { mRate, .1f }, { mDepth, .9f }, { mFeedback, .6f }, { mMod, .8f }, { mWidth, 1.8f },
                { glitchOn, 1.f }, { gStutter, .9f }, { gRate, 0.f }, { gCrushOn, 0.f }, { gReduxOn, 0.f }, { gNoiseOn, 0.f }, { gDigitalOn, 0.f }, { gMix, .6f } } },

            { "Industrial Waves", "Distortion", {
                { cDrive, .8f }, { cSat, .8f }, { cHarm, .8f }, { cTone, .4f },
                { motionOn, 1.f }, { mMode, 1.f }, { mRate, .35f }, { mDepth, .8f }, { mFeedback, .7f }, { mMod, .8f }, { mStereo, .3f },
                { glitchOn, 1.f }, { gCrush, .3f }, { gDigital, .5f }, { gNoise, .25f }, { gMix, .3f }, { gStutterOn, 0.f }, { gReduxOn, 0.f } } },

            { "PHASER INIT", "Init", {} }
        };
        return presets;
    }
}
