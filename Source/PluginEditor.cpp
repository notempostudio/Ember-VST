#include "PluginEditor.h"

EmberAudioProcessorEditor::EmberAudioProcessorEditor (EmberAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    // Stark black-and-white look
    laf.setColour (juce::Slider::rotarySliderFillColourId, juce::Colours::white);
    laf.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colours::dimgrey);
    laf.setColour (juce::Slider::thumbColourId, juce::Colours::white);
    laf.setColour (juce::Slider::textBoxTextColourId, juce::Colours::white);
    laf.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    laf.setColour (juce::Label::textColourId, juce::Colours::white);
    laf.setColour (juce::ComboBox::backgroundColourId, juce::Colours::black);
    laf.setColour (juce::ComboBox::textColourId, juce::Colours::white);
    laf.setColour (juce::ComboBox::outlineColourId, juce::Colours::white);
    laf.setColour (juce::ComboBox::arrowColourId, juce::Colours::white);
    laf.setColour (juce::PopupMenu::backgroundColourId, juce::Colours::black);
    laf.setColour (juce::PopupMenu::textColourId, juce::Colours::white);
    laf.setColour (juce::ToggleButton::textColourId, juce::Colours::white);
    laf.setColour (juce::ToggleButton::tickColourId, juce::Colours::white);
    laf.setColour (juce::ToggleButton::tickDisabledColourId, juce::Colours::dimgrey);
    setLookAndFeel (&laf);

    auto& s = proc.apvts;
    for (int b = 0; b < 3; ++b)
    {
        const auto n = juce::String (b + 1);
        bandDrive.push_back (std::make_unique<Knob> (s, "drive" + n, "Drive", *this));
        bandLevel.push_back (std::make_unique<Knob> (s, "level" + n, "Level", *this));

        if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (s.getParameter ("shape" + n)))
            shapeBox[b].addItemList (choice->choices, 1);
        addAndMakeVisible (shapeBox[b]);
        shapeAtt[b] = std::make_unique<ComboAtt> (s, "shape" + n, shapeBox[b]);

        muteButton[b].setButtonText ("Mute");
        addAndMakeVisible (muteButton[b]);
        muteAtt[b] = std::make_unique<ButtonAtt> (s, "mute" + n, muteButton[b]);
    }

    globals.push_back (std::make_unique<Knob> (s, "in",  "Input",    *this));
    globals.push_back (std::make_unique<Knob> (s, "xo1", "Low/Mid",  *this));
    globals.push_back (std::make_unique<Knob> (s, "xo2", "Mid/High", *this));
    globals.push_back (std::make_unique<Knob> (s, "mix", "Mix",      *this));
    globals.push_back (std::make_unique<Knob> (s, "out", "Output",   *this));

    setSize (760, 460);
}

EmberAudioProcessorEditor::~EmberAudioProcessorEditor() { setLookAndFeel (nullptr); }

void EmberAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (24.0f, juce::Font::bold));
    g.drawText ("EMBER", 16, 8, 300, 30, juce::Justification::centredLeft);

    const char* names[3] = { "LOW", "MID", "HIGH" };
    g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    for (int b = 0; b < 3; ++b)
    {
        g.drawRect (bandRect[b], 2);
        g.drawText (names[b], bandRect[b].withHeight (24), juce::Justification::centred);
    }
}

void EmberAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (16);
    area.removeFromTop (34);

    auto bands = area.removeFromTop (270);
    const int colW = bands.getWidth() / 3;
    for (int b = 0; b < 3; ++b)
    {
        bandRect[b] = bands.removeFromLeft (colW).reduced (6);
        auto r = bandRect[b].reduced (8);
        r.removeFromTop (24);
        shapeBox[b].setBounds (r.removeFromTop (28));
        r.removeFromTop (4);
        muteButton[b].setBounds (r.removeFromTop (24));
        r.removeFromTop (6);
        auto left = r.removeFromLeft (r.getWidth() / 2);
        bandDrive[(size_t) b]->setBounds (left);
        bandLevel[(size_t) b]->setBounds (r);
    }

    area.removeFromTop (12);
    const int gW = area.getWidth() / (int) globals.size();
    for (auto& k : globals)
        k->setBounds (area.removeFromLeft (gW).reduced (6));
}
