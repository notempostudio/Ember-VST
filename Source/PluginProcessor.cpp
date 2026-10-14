#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    const juce::StringArray shapeNames { "Soft", "Hard", "Fold", "Crush" };

    juce::String pid (const char* base, int band) { return juce::String (base) + juce::String (band + 1); }

    inline float shapeSample (int type, float x) noexcept
    {
        switch (type)
        {
            case 0:  return std::tanh (x);                                  // soft clip
            case 1:  return juce::jlimit (-1.0f, 1.0f, x);                  // hard clip
            case 2:  return std::asin (std::sin (x)) * 0.6366198f;          // triangle wavefolder
            default: return std::round (std::tanh (x) * 6.0f) / 6.0f;       // coarse quantiser
        }
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout EmberAudioProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;
    const auto dB = AudioParameterFloatAttributes().withLabel ("dB");
    const auto hz = AudioParameterFloatAttributes().withLabel ("Hz");
    const auto pc = AudioParameterFloatAttributes().withLabel ("%");

    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "in", 1 },  "Input",
                 NormalisableRange<float> (-24.f, 24.f, 0.1f), 0.f, dB));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "out", 1 }, "Output",
                 NormalisableRange<float> (-24.f, 12.f, 0.1f), 0.f, dB));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "mix", 1 }, "Mix",
                 NormalisableRange<float> (0.f, 100.f, 0.1f), 100.f, pc));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "xo1", 1 }, "Low/Mid",
                 NormalisableRange<float> (40.f, 1000.f, 1.f, 0.4f), 200.f, hz));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "xo2", 1 }, "Mid/High",
                 NormalisableRange<float> (1000.f, 12000.f, 1.f, 0.4f), 2500.f, hz));

    for (int b = 0; b < numBands; ++b)
    {
        const auto n = String (b + 1);
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { pid ("drive", b), 1 }, "Drive " + n,
                     NormalisableRange<float> (0.f, 36.f, 0.1f), 12.f, dB));
        p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { pid ("shape", b), 1 }, "Shape " + n,
                     shapeNames, 0));
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { pid ("level", b), 1 }, "Level " + n,
                     NormalisableRange<float> (-24.f, 6.f, 0.1f), 0.f, dB));
        p.push_back (std::make_unique<AudioParameterBool> (ParameterID { pid ("mute", b), 1 }, "Mute " + n, false));
    }
    return { p.begin(), p.end() };
}

EmberAudioProcessor::EmberAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    pIn  = apvts.getRawParameterValue ("in");
    pOut = apvts.getRawParameterValue ("out");
    pMix = apvts.getRawParameterValue ("mix");
    pXo1 = apvts.getRawParameterValue ("xo1");
    pXo2 = apvts.getRawParameterValue ("xo2");
    for (int b = 0; b < numBands; ++b)
    {
        pDrive[b] = apvts.getRawParameterValue (pid ("drive", b));
        pShape[b] = apvts.getRawParameterValue (pid ("shape", b));
        pLevel[b] = apvts.getRawParameterValue (pid ("level", b));
        pMute[b]  = apvts.getRawParameterValue (pid ("mute", b));
    }
}

bool EmberAudioProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto out = l.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo())
           && out == l.getMainInputChannelSet();
}

void EmberAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const auto nCh = (juce::uint32) getTotalNumOutputChannels();

    // 2x oversampling around the nonlinear stage to tame aliasing.
    oversampler = std::make_unique<juce::dsp::Oversampling<float>> (
        nCh, 1, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true);
    oversampler->initProcessing ((size_t) samplesPerBlock);
    oversampler->reset();

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, nCh };
    mixer.prepare (spec);
    mixer.setMixingRule (juce::dsp::DryWetMixingRule::balanced);
    const auto latency = oversampler->getLatencyInSamples();
    mixer.setWetLatency (latency);
    setLatencySamples (juce::roundToInt (latency));

    juce::dsp::ProcessSpec osSpec { sampleRate * 2.0, (juce::uint32) samplesPerBlock * 2, nCh };
    for (auto* f : { &xo1, &xo2, &xo2Comp })
    {
        f->prepare (osSpec);
        f->reset();
    }

    inGain.reset (sampleRate * 2.0, 0.02);
    inGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (pIn->load()));
    outGain.reset (sampleRate, 0.02);
    outGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (pOut->load()));
    for (int b = 0; b < numBands; ++b)
    {
        drive[b].reset (sampleRate * 2.0, 0.02);
        level[b].reset (sampleRate * 2.0, 0.02);
        drive[b].setCurrentAndTargetValue (juce::Decibels::decibelsToGain (pDrive[b]->load()));
        level[b].setCurrentAndTargetValue (pMute[b]->load() > 0.5f ? 0.f
                                           : juce::Decibels::decibelsToGain (pLevel[b]->load()));
    }
}

void EmberAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); ++c)
        buffer.clear (c, 0, buffer.getNumSamples());

    // Update targets
    inGain.setTargetValue (juce::Decibels::decibelsToGain (pIn->load()));
    outGain.setTargetValue (juce::Decibels::decibelsToGain (pOut->load()));
    mixer.setWetMixProportion (pMix->load() / 100.0f);
    int shape[numBands];
    for (int b = 0; b < numBands; ++b)
    {
        drive[b].setTargetValue (juce::Decibels::decibelsToGain (pDrive[b]->load()));
        level[b].setTargetValue (pMute[b]->load() > 0.5f ? 0.f
                                 : juce::Decibels::decibelsToGain (pLevel[b]->load()));
        shape[b] = (int) pShape[b]->load();
    }
    const float f1 = pXo1->load();
    const float f2 = juce::jmax (pXo2->load(), f1 * 1.5f);
    xo1.setCutoffFrequency (f1);
    xo2.setCutoffFrequency (f2);
    xo2Comp.setCutoffFrequency (f2);   // keeps the low band phase-aligned with mid/high

    juce::dsp::AudioBlock<float> block (buffer);
    mixer.pushDrySamples (block);

    auto up = oversampler->processSamplesUp (block);
    const int n  = (int) up.getNumSamples();
    const int ch = (int) up.getNumChannels();

    for (int i = 0; i < n; ++i)
    {
        const float g = inGain.getNextValue();
        float d[numBands], lv[numBands];
        for (int b = 0; b < numBands; ++b) { d[b] = drive[b].getNextValue(); lv[b] = level[b].getNextValue(); }

        for (int c = 0; c < ch; ++c)
        {
            const float x = up.getSample (c, i) * g;
            float low, rest, mid, high, a, b2;
            xo1.processSample (c, x, low, rest);
            xo2.processSample (c, rest, mid, high);
            xo2Comp.processSample (c, low, a, b2);
            low = a + b2;                                   // all-pass copy of the low band

            const float y = shapeSample (shape[0], low  * d[0]) * lv[0]
                          + shapeSample (shape[1], mid  * d[1]) * lv[1]
                          + shapeSample (shape[2], high * d[2]) * lv[2];
            up.setSample (c, i, y);
        }
    }

    oversampler->processSamplesDown (block);
    mixer.mixWetSamples (block);
    outGain.applyGain (buffer, buffer.getNumSamples());
}

juce::AudioProcessorEditor* EmberAudioProcessor::createEditor() { return new EmberAudioProcessorEditor (*this); }

void EmberAudioProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void EmberAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new EmberAudioProcessor(); }
