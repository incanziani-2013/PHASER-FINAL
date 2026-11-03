#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace phaser;

PhaserAudioProcessor::PhaserAudioProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PHASER", createParameterLayout()),
      presets (apvts)
{
    raw.bind (apvts);
    presets.rescan();
}

bool PhaserAudioProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto out = l.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo()) return false;
    return out == l.getMainInputChannelSet();
}

void PhaserAudioProcessor::prepareToPlay (double sr, int block)
{
    sampleRateHz = sr;
    maxBlock = juce::jmax (64, block);
    dryBuf.setSize (2, maxBlock, false, true, false);
    scratch.setSize (2, maxBlock, false, true, false);
    color.prepare (sr); spectrum.prepare (sr); motion.prepare (sr); glitch.prepare (sr); master.prepare (sr);

    for (auto* s : { &sIn, &sMix, &sBypass, &amtColor, &amtSpectrum, &amtMotion, &amtGlitch }) s->reset (sr, 0.02);
    sIn.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (raw.inGain->load()));
    sMix.setCurrentAndTargetValue (raw.mix->load());
    sBypass.setCurrentAndTargetValue (raw.bypass->load() > 0.5f ? 1.0f : 0.0f);
    amtColor.setCurrentAndTargetValue (raw.colorOn->load() > 0.5f ? 1.0f : 0.0f);
    amtSpectrum.setCurrentAndTargetValue (raw.spectrumOn->load() > 0.5f ? 1.0f : 0.0f);
    amtMotion.setCurrentAndTargetValue (raw.motionOn->load() > 0.5f ? 1.0f : 0.0f);
    amtGlitch.setCurrentAndTargetValue (raw.glitchOn->load() > 0.5f ? 1.0f : 0.0f);
    vizFifo.reset();
}

template <typename Fn>
void PhaserAudioProcessor::runModule (juce::AudioBuffer<float>& b, bool on, juce::SmoothedValue<float>& amt, Fn&& fn)
{
    amt.setTargetValue (on ? 1.0f : 0.0f);
    const bool fading = amt.isSmoothing();
    if (! on && ! fading) return;

    const int n = b.getNumSamples(), nc = b.getNumChannels();
    if (fading) for (int c = 0; c < nc; ++c) scratch.copyFrom (c, 0, b, c, 0, n);
    fn();
    if (fading)
        for (int i = 0; i < n; ++i)
        {
            const float a = amt.getNextValue();
            for (int c = 0; c < nc; ++c) { auto* d = b.getWritePointer (c); const float s = scratch.getReadPointer (c)[i]; d[i] = s + (d[i] - s) * a; }
        }
}

void PhaserAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int total = buffer.getNumSamples();
    const int nc = juce::jmin (2, buffer.getNumChannels());
    for (int c = nc; c < buffer.getNumChannels(); ++c) buffer.clear (c, 0, total);

    bpm = 120.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto b = pos->getBpm()) bpm = *b;

    for (int pos = 0; pos < total; pos += maxBlock)
    {
        const int len = juce::jmin (maxBlock, total - pos);
        float* ptrs[2] = { buffer.getWritePointer (0) + pos, nc > 1 ? buffer.getWritePointer (1) + pos : nullptr };
        juce::AudioBuffer<float> sub (ptrs, nc, len);   // vista sin copia ni asignación
        processChunk (sub);
    }
}

void PhaserAudioProcessor::processChunk (juce::AudioBuffer<float>& b)
{
    const int n = b.getNumSamples(), nc = b.getNumChannels();
    float* const* ch = b.getArrayOfWritePointers();

    // Saneado de entrada (un NaN en un filtro IIR lo dejaría mudo para siempre) + medidor de entrada + copia seca
    float pl = 0, pr = 0;
    for (int c = 0; c < nc; ++c)
    {
        float pk = 0;
        for (int i = 0; i < n; ++i) { float v = ch[c][i]; if (! std::isfinite (v)) { v = 0; ch[c][i] = 0; } pk = juce::jmax (pk, std::abs (v)); }
        (c == 0 ? pl : pr) = pk;
        dryBuf.copyFrom (c, 0, b, c, 0, n);
    }
    maxInto (inPeakL, pl); maxInto (inPeakR, nc > 1 ? pr : pl);

    // Ganancia de entrada suavizada
    sIn.setTargetValue (juce::Decibels::decibelsToGain (raw.inGain->load()));
    for (int i = 0; i < n; ++i) { const float g = sIn.getNextValue(); for (int c = 0; c < nc; ++c) ch[c][i] *= g; }

    // Módulos en serie: COLOR -> SPECTRUM -> MOTION -> GLITCH
    ColorParams cp { raw.cDrive->load(), raw.cTone->load(), raw.cSat->load(), raw.cBlend->load(), raw.cWarm->load(), raw.cBright->load(), raw.cHarm->load() };
    SpectrumParams sp; sp.low = raw.sLow->load(); sp.mid = raw.sMid->load(); sp.high = raw.sHigh->load();
    sp.res = raw.sRes->load(); sp.freq = raw.sFreq->load(); sp.q = raw.sQ->load(); sp.filter = (int) raw.sFilter->load();
    MotionParams mp; mp.width = raw.mWidth->load(); mp.rate = raw.mRate->load(); mp.depth = raw.mDepth->load();
    mp.phaseDeg = raw.mPhase->load(); mp.feedback = raw.mFeedback->load(); mp.mod = raw.mMod->load();
    mp.stereo = raw.mStereo->load(); mp.mode = (int) raw.mMode->load();
    GlitchParams gp; gp.crush = raw.gCrush->load(); gp.redux = raw.gRedux->load(); gp.stutter = raw.gStutter->load();
    gp.noise = raw.gNoise->load(); gp.rate = (int) raw.gRate->load(); gp.digital = raw.gDigital->load(); gp.mix = raw.gMix->load();
    gp.crushOn = raw.gCrushOn->load() > 0.5f; gp.reduxOn = raw.gReduxOn->load() > 0.5f; gp.stutterOn = raw.gStutterOn->load() > 0.5f;
    gp.noiseOn = raw.gNoiseOn->load() > 0.5f; gp.digitalOn = raw.gDigitalOn->load() > 0.5f; gp.bpm = bpm;
    MasterParams mps; mps.drive = raw.mDrive->load(); mps.stereo = raw.stereo->load(); mps.outGainDb = raw.outGain->load();
    mps.limiter = raw.limiter->load() > 0.5f;

    runModule (b, raw.colorOn->load() > 0.5f,    amtColor,    [&] { color.process (ch, nc, n, cp); });
    runModule (b, raw.spectrumOn->load() > 0.5f, amtSpectrum, [&] { spectrum.process (ch, nc, n, sp); });
    runModule (b, raw.motionOn->load() > 0.5f,   amtMotion,   [&] { motion.process (ch, nc, n, mp); });
    runModule (b, raw.glitchOn->load() > 0.5f,   amtGlitch,   [&] { glitch.process (ch, nc, n, gp); });
    master.process (ch, nc, n, mps);
    reductionDb.store (master.getReductionDb());

    // Mix global + bypass con fundido (la latencia es 0, el seco está alineado)
    sMix.setTargetValue (raw.mix->load());
    sBypass.setTargetValue (raw.bypass->load() > 0.5f ? 1.0f : 0.0f);
    for (int i = 0; i < n; ++i)
    {
        const float mix = sMix.getNextValue(), byp = sBypass.getNextValue();
        for (int c = 0; c < nc; ++c)
        {
            const float dry = dryBuf.getReadPointer (c)[i], wet = ch[c][i];
            const float m = dry + (wet - dry) * mix;
            ch[c][i] = m + (dry - m) * byp;
        }
    }

    // Medidor de salida + FIFO del visualizador (solo si hay editor abierto)
    float ol = 0, orr = 0;
    for (int c = 0; c < nc; ++c)
    {
        float pk = 0; for (int i = 0; i < n; ++i) pk = juce::jmax (pk, std::abs (ch[c][i]));
        (c == 0 ? ol : orr) = pk;
    }
    maxInto (outPeakL, ol); maxInto (outPeakR, nc > 1 ? orr : ol);

    if (visualizerActive.load (std::memory_order_relaxed))
    {
        int s1, n1, s2, n2;
        vizFifo.prepareToWrite (n, s1, n1, s2, n2);
        auto mono = [&] (int i) { return nc > 1 ? 0.5f * (ch[0][i] + ch[1][i]) : ch[0][i]; };
        for (int i = 0; i < n1; ++i) vizData[(size_t) (s1 + i)] = mono (i);
        for (int i = 0; i < n2; ++i) vizData[(size_t) (s2 + i)] = mono (n1 + i);
        vizFifo.finishedWrite (n1 + n2);
    }
}

int PhaserAudioProcessor::readVisualizerSamples (float* dest, int maxNum)
{
    int avail = vizFifo.getNumReady();
    int s1, n1, s2, n2;
    if (avail > maxNum)   // descarta lo viejo
    {
        const int skip = avail - maxNum;
        vizFifo.prepareToRead (skip, s1, n1, s2, n2);
        vizFifo.finishedRead (n1 + n2);
        avail = vizFifo.getNumReady();
    }
    const int want = juce::jmin (avail, maxNum);
    vizFifo.prepareToRead (want, s1, n1, s2, n2);
    for (int i = 0; i < n1; ++i) dest[i] = vizData[(size_t) (s1 + i)];
    for (int i = 0; i < n2; ++i) dest[n1 + i] = vizData[(size_t) (s2 + i)];
    vizFifo.finishedRead (n1 + n2);
    return n1 + n2;
}

void PhaserAudioProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, dest);
}

void PhaserAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* PhaserAudioProcessor::createEditor() { return new PhaserAudioProcessorEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new PhaserAudioProcessor(); }
