#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

// Ember: a 3-band distortion with per-band drive, shape and level.
class EmberAudioProcessor : public juce::AudioProcessor
{
public:
    EmberAudioProcessor();
    ~EmberAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Ember"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    juce::AudioProcessorValueTreeState apvts;

private:
    static constexpr int numBands = 3;

    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;
    juce::dsp::DryWetMixer<float> mixer;

    juce::dsp::LinkwitzRileyFilter<float> xo1, xo2, xo2Comp;

    juce::SmoothedValue<float> inGain, outGain;
    juce::SmoothedValue<float> drive[numBands], level[numBands];

    std::atomic<float>* pIn = nullptr;
    std::atomic<float>* pOut = nullptr;
    std::atomic<float>* pMix = nullptr;
    std::atomic<float>* pXo1 = nullptr;
    std::atomic<float>* pXo2 = nullptr;
    std::atomic<float>* pDrive[numBands] {};
    std::atomic<float>* pShape[numBands] {};
    std::atomic<float>* pLevel[numBands] {};
    std::atomic<float>* pMute[numBands] {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EmberAudioProcessor)
};
