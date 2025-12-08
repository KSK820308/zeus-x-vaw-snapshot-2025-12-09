#pragma once
#include <JuceHeader.h>

class GainProcessor : public juce::AudioProcessor {
public:
    GainProcessor()
        : juce::AudioProcessor(juce::AudioProcessor::BusesProperties()
            .withInput("Input", juce::AudioChannelSet::stereo(), true)
            .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
          parameters(*this, nullptr, "GainParams", { std::make_unique<juce::AudioParameterFloat>("gain", "Gain", 0.0f, 1.0f, 1.0f) }) {}
    const juce::String getName() const override { return "Gain"; }
    void prepareToPlay(double sampleRate, int samplesPerBlock) override { juce::ignoreUnused(samplesPerBlock); gainSmooth.reset(sampleRate, 0.05); gainSmooth.setCurrentAndTargetValue(*parameters.getRawParameterValue("gain")); }
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override {
        gainSmooth.setTargetValue(*parameters.getRawParameterValue("gain"));
        int n = buffer.getNumSamples();
        int c = buffer.getNumChannels();
        for (int i = 0; i < n; ++i) {
            float g = gainSmooth.getNextValue();
            for (int ch = 0; ch < c; ++ch) {
                buffer.setSample(ch, i, buffer.getSample(ch, i) * g);
            }
        }
    }
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override {
        return layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo()
            && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
    }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock& destData) override { juce::MemoryOutputStream(destData, false).writeFloat(*parameters.getRawParameterValue("gain")); }
    void setStateInformation (const void* data, int sizeInBytes) override { juce::MemoryInputStream is(data, (size_t) sizeInBytes, false); float g = is.readFloat(); parameters.getParameter("gain")->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, g)); }
    void setGain(float g) { parameters.getParameter("gain")->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, g)); }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
private:
    juce::AudioProcessorValueTreeState parameters;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> gainSmooth;
};
