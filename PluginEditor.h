#pragma once
#include "PluginProcessor.h"

class GoldKnobEditor : public juce::AudioProcessorEditor
{
public:
    explicit GoldKnobEditor (GoldKnobAudioProcessor& p)
        : AudioProcessorEditor (&p),
          attachment (p.apvts, "gold", knob)
    {
        const auto gold = juce::Colour (0xffd4a437);
        knob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        knob.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 90, 22);
        knob.setTextValueSuffix (" dB");
        knob.setNumDecimalPlacesToDisplay (2);
        knob.setColour (juce::Slider::rotarySliderFillColourId, gold);
        knob.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff3a3a3a));
        knob.setColour (juce::Slider::thumbColourId, gold);
        knob.setColour (juce::Slider::textBoxTextColourId, gold);
        knob.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        addAndMakeVisible (knob);
        setSize (260, 300);
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colour (0xff141414));
        g.setColour (juce::Colour (0xffd4a437));
        g.setFont (juce::FontOptions (28.0f, juce::Font::bold));
        g.drawText ("GOLD", getLocalBounds().removeFromTop (60), juce::Justification::centred);
    }

    void resized() override
    {
        knob.setBounds (getLocalBounds().withTrimmedTop (60).reduced (30));
    }

private:
    juce::Slider knob;
    juce::AudioProcessorValueTreeState::SliderAttachment attachment;
};
