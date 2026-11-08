#include "PluginProcessor.h"
#include "PluginEditor.h"

GoldKnobAudioProcessor::GoldKnobAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout GoldKnobAudioProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    // "Gold" amount expressed as the gain applied to very quiet signal (dB).
    // Peaks near full scale stay put; everything below them is lifted.
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "gold", 1 }, "Gold",
        juce::NormalisableRange<float> (0.0f, 12.0f, 0.01f, 0.5f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));
    return layout;
}

// Low-level gain of the curve y = log(1 + k|x|) / log(1 + k) is k / log(1 + k).
// Solve for k so that this equals the requested dB boost.
float GoldKnobAudioProcessor::goldDbToK (float db)
{
    const double target = std::pow (10.0, (double) db / 20.0);
    if (target <= 1.0000001) return 1.0e-4f;

    double lo = 1.0e-4, hi = 1.0e5;
    for (int i = 0; i < 60; ++i)
    {
        const double mid = std::sqrt (lo * hi);
        const double g = mid / std::log1p (mid);
        (g < target ? lo : hi) = mid;
    }
    return (float) std::sqrt (lo * hi);
}

bool GoldKnobAudioProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto& out = l.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo())
           && out == l.getMainInputChannelSet();
}

void GoldKnobAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    oversampling = std::make_unique<juce::dsp::Oversampling<float>> (
        (size_t) getTotalNumInputChannels(), 2,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true);
    oversampling->initProcessing ((size_t) samplesPerBlock);
    setLatencySamples ((int) std::round (oversampling->getLatencyInSamples()));

    smoothK.reset (sampleRate * 4.0, 0.03);
    smoothK.setCurrentAndTargetValue (goldDbToK (*apvts.getRawParameterValue ("gold")));
}

void GoldKnobAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    if (oversampling == nullptr) return;

    smoothK.setTargetValue (goldDbToK (apvts.getRawParameterValue ("gold")->load()));

    juce::dsp::AudioBlock<float> block (buffer);
    auto up = oversampling->processSamplesUp (block);

    const auto numCh = up.getNumChannels();
    const auto numSamples = up.getNumSamples();

    for (size_t i = 0; i < numSamples; ++i)
    {
        const float k = smoothK.getNextValue();
        const float norm = 1.0f / std::log1p (k);

        for (size_t ch = 0; ch < numCh; ++ch)
        {
            float* d = up.getChannelPointer (ch);
            const float x = d[i];
            d[i] = std::copysign (std::log1p (k * std::abs (x)) * norm, x);
        }
    }

    oversampling->processSamplesDown (block);
}

juce::AudioProcessorEditor* GoldKnobAudioProcessor::createEditor()
{
    return new GoldKnobEditor (*this);
}

void GoldKnobAudioProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void GoldKnobAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GoldKnobAudioProcessor();
}
