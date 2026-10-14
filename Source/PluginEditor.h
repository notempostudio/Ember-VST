#pragma once
#include "PluginProcessor.h"

class EmberAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit EmberAudioProcessorEditor (EmberAudioProcessor&);
    ~EmberAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAtt  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;

    struct Knob
    {
        Knob (juce::AudioProcessorValueTreeState& s, const juce::String& paramId,
              const juce::String& name, juce::Component& parent)
        {
            slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
            slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 74, 16);
            label.setText (name, juce::dontSendNotification);
            label.setJustificationType (juce::Justification::centred);
            parent.addAndMakeVisible (slider);
            parent.addAndMakeVisible (label);
            attachment = std::make_unique<SliderAtt> (s, paramId, slider);
        }
        void setBounds (juce::Rectangle<int> r)
        {
            label.setBounds (r.removeFromTop (16));
            slider.setBounds (r);
        }
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<SliderAtt> attachment;
    };

    EmberAudioProcessor& proc;
    juce::LookAndFeel_V4 laf;

    std::vector<std::unique_ptr<Knob>> bandDrive, bandLevel, globals;
    juce::ComboBox shapeBox[3];
    juce::ToggleButton muteButton[3];
    std::unique_ptr<ComboAtt> shapeAtt[3];
    std::unique_ptr<ButtonAtt> muteAtt[3];
    juce::Rectangle<int> bandRect[3];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EmberAudioProcessorEditor)
};
