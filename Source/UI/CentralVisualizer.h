#pragma once
#include "UiUtil.h"

class PhaserAudioProcessor;

namespace phaser::ui
{
    // Esfera de energía reactiva. Lee del FIFO lock-free del procesador (nunca de los buffers de audio),
    // calcula un FFT de 1024 puntos en el hilo de mensajes (~30 fps) y dibuja con Graphics.
    class CentralVisualizer : public juce::Component, private juce::Timer
    {
    public:
        explicit CentralVisualizer (PhaserAudioProcessor& p);
        ~CentralVisualizer() override;
        void paint (juce::Graphics&) override;
    private:
        void timerCallback() override;
        void analyse();

        static constexpr int fftOrder = 10, fftSize = 1 << fftOrder, numBands = 32;
        struct Particle { float angle, radius, speed, size, hue; };
        struct Ring { float age; };

        PhaserAudioProcessor& proc;
        juce::dsp::FFT fft { fftOrder };
        juce::dsp::WindowingFunction<float> window { (size_t) fftSize, juce::dsp::WindowingFunction<float>::hann };
        std::array<float, fftSize> history {};
        std::array<float, fftSize * 2> fftData {};
        std::array<float, numBands> bands {};
        std::array<Particle, 64> particles {};
        std::vector<Ring> rings;
        juce::Random rng;
        float level = 0, prevLevel = 0, bass = 0, mid = 0, high = 0, energy = 0, rot = 0;
        double time = 0;
    };
}
