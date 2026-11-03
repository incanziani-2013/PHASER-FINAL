#pragma once
#include "Common.h"
#include "Parameters.h"
#include "DSP/ColorEngine.h"
#include "DSP/SpectrumEngine.h"
#include "DSP/MotionEngine.h"
#include "DSP/GlitchEngine.h"
#include "DSP/MasterEngine.h"
#include "Presets/PresetManager.h"

class PhaserAudioProcessor : public juce::AudioProcessor
{
public:
    PhaserAudioProcessor();
    ~PhaserAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "PHASER"; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.5; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "PHASER"; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // ---- compartido con la GUI (todo lock-free) ----
    juce::AudioProcessorValueTreeState apvts;
    phaser::PresetManager presets;

    std::atomic<float> inPeakL { 0 }, inPeakR { 0 }, outPeakL { 0 }, outPeakR { 0 };   // pico desde la última lectura
    std::atomic<float> reductionDb { 0 };
    std::atomic<bool>  visualizerActive { false };
    int readVisualizerSamples (float* dest, int maxNum);   // hilo de mensajes: devuelve las muestras más recientes

private:
    void processChunk (juce::AudioBuffer<float>& b);
    template <typename Fn> void runModule (juce::AudioBuffer<float>& b, bool on, juce::SmoothedValue<float>& amt, Fn&& fn);
    static void maxInto (std::atomic<float>& a, float v) noexcept { float c = a.load(); while (v > c && ! a.compare_exchange_weak (c, v)) {} }

    phaser::RawParams raw;
    phaser::ColorEngine color; phaser::SpectrumEngine spectrum; phaser::MotionEngine motion;
    phaser::GlitchEngine glitch; phaser::MasterEngine master;

    juce::AudioBuffer<float> dryBuf, scratch;
    juce::SmoothedValue<float> sIn, sMix, sBypass, amtColor, amtSpectrum, amtMotion, amtGlitch;
    int maxBlock = 512;
    double sampleRateHz = 44100.0, bpm = 120.0;

    juce::AbstractFifo vizFifo { 16384 };
    std::array<float, 16384> vizData {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PhaserAudioProcessor)
};
