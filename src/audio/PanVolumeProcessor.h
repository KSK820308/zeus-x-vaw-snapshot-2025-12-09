#pragma once
#include <JuceHeader.h>

class PanVolumeProcessor : public juce::AudioProcessor {
public:
    PanVolumeProcessor()
        : juce::AudioProcessor(juce::AudioProcessor::BusesProperties()
            .withInput("Input", juce::AudioChannelSet::stereo(), true)
            .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
          parameters(*this, nullptr, "PanVol", {
        std::make_unique<juce::AudioParameterFloat>("vol", "Volume", 0.0f, 1.0f, 1.0f),
        std::make_unique<juce::AudioParameterFloat>("pan", "Pan", -1.0f, 1.0f, 0.0f),
        std::make_unique<juce::AudioParameterFloat>("balance", "Balance", -1.0f, 1.0f, 0.0f),
        std::make_unique<juce::AudioParameterBool>("swap", "Swap", false)
    }) {}
    const juce::String getName() const override { return "PanVolume"; }
    void prepareToPlay(double sampleRate, int) override {
        volSmooth.reset(sampleRate, 0.05);
        panSmooth.reset(sampleRate, 0.05);
        balSmooth.reset(sampleRate, 0.05);
        volSmooth.setCurrentAndTargetValue(parameters.getRawParameterValue("vol")->load());
        panSmooth.setCurrentAndTargetValue(parameters.getRawParameterValue("pan")->load());
        balSmooth.setCurrentAndTargetValue(parameters.getRawParameterValue("balance")->load());
    }
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override {
        volSmooth.setTargetValue(parameters.getRawParameterValue("vol")->load());
        panSmooth.setTargetValue(juce::jlimit(-1.0f, 1.0f, parameters.getRawParameterValue("pan")->load()));
        balSmooth.setTargetValue(juce::jlimit(-1.0f, 1.0f, parameters.getRawParameterValue("balance")->load()));
        bool s = parameters.getRawParameterValue("swap")->load() > 0.5f;
        int n = buffer.getNumSamples();
        int c = buffer.getNumChannels();
        for (int i = 0; i < n; ++i) {
            float v = volSmooth.getNextValue();
            float p = juce::jlimit(-1.0f, 1.0f, panSmooth.getNextValue() + panBias);
            float b = balSmooth.getNextValue();
            float gl = v * std::sqrt(0.5f * (1.0f - p));
            float gr = v * std::sqrt(0.5f * (1.0f + p));
            gl *= juce::jlimit(0.0f, 2.0f, 1.0f - b);
            gr *= juce::jlimit(0.0f, 2.0f, 1.0f + b);
            float left = s ? gr : gl;
            float right = s ? gl : gr;
            if (c > 0) buffer.setSample(0, i, buffer.getSample(0, i) * left);
            if (c > 1) buffer.setSample(1, i, buffer.getSample(1, i) * right);
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
    void getStateInformation (juce::MemoryBlock& destData) override {
        juce::MemoryOutputStream os(destData, false);
        os.writeFloat(*parameters.getRawParameterValue("vol"));
        os.writeFloat(*parameters.getRawParameterValue("pan"));
        os.writeFloat(*parameters.getRawParameterValue("balance"));
        os.writeBool(parameters.getRawParameterValue("swap")->load() > 0.5f);
    }
    void setStateInformation (const void* data, int sizeInBytes) override {
        juce::MemoryInputStream is(data, (size_t) sizeInBytes, false);
        float v = is.readFloat();
        float p = is.readFloat();
        float bal = is.readFloat();
        bool sw = is.readBool();
        parameters.getParameter("vol")->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, v));
        parameters.getParameter("pan")->setValueNotifyingHost(juce::jlimit(-1.0f, 1.0f, p));
        parameters.getParameter("balance")->setValueNotifyingHost(juce::jlimit(-1.0f, 1.0f, bal));
        parameters.getParameter("swap")->setValueNotifyingHost(sw ? 1.0f : 0.0f);
    }
    void setVolume(float v) { parameters.getParameter("vol")->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, v)); }
    void setPan(float p) { parameters.getParameter("pan")->setValueNotifyingHost(juce::jlimit(-1.0f, 1.0f, p)); }
    void setBalance(float b) { parameters.getParameter("balance")->setValueNotifyingHost(juce::jlimit(-1.0f, 1.0f, b)); }
    void setPanBias(float o) { panBias = juce::jlimit(-0.5f, 0.5f, o); }
    void setSwap(bool s) { parameters.getParameter("swap")->setValueNotifyingHost(s ? 1.0f : 0.0f); }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
private:
    juce::AudioProcessorValueTreeState parameters;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> volSmooth;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> panSmooth;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> balSmooth;
    float panBias { 0.0f };
};
