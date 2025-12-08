#pragma once
#include <JuceHeader.h>

class EQProcessor : public juce::AudioProcessor {
public:
    EQProcessor() : juce::AudioProcessor(BusesProperties().withInput("in", juce::AudioChannelSet::stereo(), true).withOutput("out", juce::AudioChannelSet::stereo(), true)) {}
    const juce::String getName() const override { return "EQ"; }
    void prepareToPlay(double sr, int) override {
        sampleRate = sr;
        juce::dsp::ProcessSpec spec { sampleRate, 512, 2 };
        low.reset(); mid.reset(); high.reset();
        updateFilters();
    }
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override {
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> ctx(block);
        low.process(ctx);
        mid.process(ctx);
        high.process(ctx);
    }
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override { return layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo() && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo(); }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    void getStateInformation(juce::MemoryBlock& destData) override {
        juce::MemoryOutputStream os(destData, false);
        os.writeFloat(lowGainDb); os.writeFloat(midGainDb); os.writeFloat(highGainDb);
        os.writeFloat(lowFreq); os.writeFloat(midFreq); os.writeFloat(highFreq);
        os.writeFloat(midQ);
    }
    void setStateInformation(const void* data, int sizeInBytes) override {
        juce::MemoryInputStream is(data, (size_t) sizeInBytes, false);
        lowGainDb = is.readFloat(); midGainDb = is.readFloat(); highGainDb = is.readFloat();
        lowFreq = is.readFloat(); midFreq = is.readFloat(); highFreq = is.readFloat();
        midQ = is.readFloat(); updateFilters();
    }
    void setLow(float gainDb, float freq) { lowGainDb = gainDb; lowFreq = freq; updateFilters(); }
    void setMid(float gainDb, float freq, float q) { midGainDb = gainDb; midFreq = freq; midQ = q; updateFilters(); }
    void setHigh(float gainDb, float freq) { highGainDb = gainDb; highFreq = freq; updateFilters(); }
private:
    double sampleRate { 44100.0 };
    float lowGainDb { 0.0f }, midGainDb { 0.0f }, highGainDb { 0.0f };
    float lowFreq { 100.0f }, midFreq { 1000.0f }, highFreq { 8000.0f }, midQ { 0.7f };
    juce::dsp::IIR::Filter<float> low;
    juce::dsp::IIR::Filter<float> mid;
    juce::dsp::IIR::Filter<float> high;
    void updateFilters() {
        auto gs = juce::Decibels::decibelsToGain(lowGainDb);
        low.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowShelf(sampleRate, lowFreq, 0.707f, gs);
        auto gm = juce::Decibels::decibelsToGain(midGainDb);
        mid.coefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, midFreq, midQ, gm);
        auto gh = juce::Decibels::decibelsToGain(highGainDb);
        high.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighShelf(sampleRate, highFreq, 0.707f, gh);
    }
};

class ReverbProcessor : public juce::AudioProcessor {
public:
    ReverbProcessor() : juce::AudioProcessor(BusesProperties().withInput("in", juce::AudioChannelSet::stereo(), true).withOutput("out", juce::AudioChannelSet::stereo(), true)) {}
    const juce::String getName() const override { return "Reverb"; }
    void prepareToPlay(double, int) override {}
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override {
        juce::Reverb::Parameters p; p.roomSize = roomSize; p.damping = damping; p.width = width; p.wetLevel = wet; p.dryLevel = dry; p.freezeMode = freeze ? 1.0f : 0.0f; rv.setParameters(p);
        int c = buffer.getNumChannels(); int n = buffer.getNumSamples();
        if (c >= 2) rv.processStereo(buffer.getWritePointer(0), buffer.getWritePointer(1), n);
        else if (c == 1) rv.processMono(buffer.getWritePointer(0), n);
    }
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override { return layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo() && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo(); }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    void getStateInformation(juce::MemoryBlock& destData) override { juce::MemoryOutputStream os(destData, false); os.writeFloat(roomSize); os.writeFloat(damping); os.writeFloat(width); os.writeFloat(wet); os.writeFloat(dry); os.writeBool(freeze); }
    void setStateInformation(const void* data, int sizeInBytes) override { juce::MemoryInputStream is(data, (size_t) sizeInBytes, false); roomSize = is.readFloat(); damping = is.readFloat(); width = is.readFloat(); wet = is.readFloat(); dry = is.readFloat(); freeze = is.readBool(); }
    void setParams(float rs, float dp, float wd, float wt, float dr, bool fr) { roomSize = rs; damping = dp; width = wd; wet = wt; dry = dr; freeze = fr; }
private:
    juce::Reverb rv;
    float roomSize{0.3f}, damping{0.3f}, width{1.0f}, wet{0.15f}, dry{0.85f}; bool freeze{false};
};

class DelayProcessor : public juce::AudioProcessor {
public:
    DelayProcessor() : juce::AudioProcessor(BusesProperties().withInput("in", juce::AudioChannelSet::stereo(), true).withOutput("out", juce::AudioChannelSet::stereo(), true)) {}
    const juce::String getName() const override { return "Delay"; }
    void prepareToPlay(double sr, int) override { sampleRate = sr; int maxDelay = (int) (sr * 2); bufferL.setSize(1, maxDelay); bufferR.setSize(1, maxDelay); bufferL.clear(); bufferR.clear(); writePos = 0; }
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override {
        int n = buffer.getNumSamples();
        auto* inL = buffer.getWritePointer(0);
        auto* inR = buffer.getWritePointer(juce::jmin(1, buffer.getNumChannels()-1));
        auto* dl = bufferL.getWritePointer(0);
        auto* dr = bufferR.getWritePointer(0);
        int delaySamples = (int) juce::jlimit(1, (int) bufferL.getNumSamples()-1, (int) std::round(timeMs * sampleRate / 1000.0));
        for (int i = 0; i < n; ++i) {
            int rp = (writePos + bufferL.getNumSamples() - delaySamples) % bufferL.getNumSamples();
            float dlL = dl[rp]; float dlR = dr[rp];
            float outL = inL[i] * (1.0f - mix) + dlL * mix;
            float outR = inR[i] * (1.0f - mix) + dlR * mix;
            dl[writePos] = inL[i] + dlL * feedback;
            dr[writePos] = inR[i] + dlR * feedback;
            inL[i] = outL; if (buffer.getNumChannels() >= 2) inR[i] = outR; else inL[i] = outL;
            writePos = (writePos + 1) % bufferL.getNumSamples();
        }
    }
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override { return layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo() && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo(); }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    void getStateInformation(juce::MemoryBlock& destData) override { juce::MemoryOutputStream os(destData, false); os.writeFloat(timeMs); os.writeFloat(feedback); os.writeFloat(mix); }
    void setStateInformation(const void* data, int sizeInBytes) override { juce::MemoryInputStream is(data, (size_t) sizeInBytes, false); timeMs = is.readFloat(); feedback = is.readFloat(); mix = is.readFloat(); }
    void setParams(float tMs, float fb, float mx) { timeMs = tMs; feedback = fb; mix = mx; }
    
private:
    double sampleRate { 44100.0 };
    juce::AudioBuffer<float> bufferL, bufferR; int writePos { 0 };
    float timeMs { 350.0f }, feedback { 0.35f }, mix { 0.25f };
};
