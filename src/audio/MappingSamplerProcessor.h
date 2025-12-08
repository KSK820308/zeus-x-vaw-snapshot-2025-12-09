#pragma once
#include <JuceHeader.h>

class MappingSamplerProcessor : public juce::AudioProcessor {
public:
    MappingSamplerProcessor();
    const juce::String getName() const override;
    void prepareToPlay(double, int) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    bool hasEditor() const override;
    juce::AudioProcessorEditor* createEditor() override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    double getTailLengthSeconds() const override;
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int) override;
    const juce::String getProgramName(int) override;
    void changeProgramName(int, const juce::String&) override;
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;
    void loadSample(const juce::File& file, int rootKey, int lowKey, int highKey);
    void addSample(const juce::File& file, int rootKey);
    void setADSR(float a, float d, float s, float r);
    struct Zone { juce::File file; int rootKey{}; int lowKey{}; int highKey{}; int velLow{0}; int velHigh{127}; float gain{1.0f}; float pan{0.0f}; float tuneCents{0.0f}; };
    const std::vector<Zone>& getZones() const { return zones; }
    void setZoneRange(int index, int lowKey, int highKey);
    void setZoneRoot(int index, int rootKey);
    void removeZone(int index);
    void setZoneVelocity(int index, int velLow, int velHigh);
    void setZoneGain(int index, float gainLinear);
    void setZonePan(int index, float panMinus1To1);
    void setZoneTune(int index, float centsMinus100To100);
    void rebuild();
    bool exportInstrument(const juce::File& destFile, int msb, int lsb, int pc, const juce::String& name);
    bool importInstrument(const juce::File& srcFile, int& msb, int& lsb, int& pc, juce::String& name);
    bool exportInstrumentWithExistingSamples(const juce::File& srcFile, const juce::File& destFile, int msb, int lsb, int pc, const juce::String& name);
private:
    juce::Synthesiser synth;
    juce::ADSR::Parameters adsrParams { 0.01f, 0.1f, 0.8f, 0.2f };
    juce::AudioFormatManager formatManager;
    std::vector<std::unique_ptr<juce::AudioFormatReader>> readers;
    std::vector<Zone> zones;
    std::vector<int> zoneSampleIndices;
    int currentChannel { 1 };
    bool sustainOn { false };
    std::array<uint16_t, 128> noteActiveMask { };
    std::array<uint16_t, 128> pendingOffMask { };
    float masterGain { 1.0f };
    // FX state for export/import
    float fxEqLowDb { 0.0f }, fxEqMidDb { 0.0f }, fxEqHighDb { 0.0f };
    int fxRevPreset { 1 }; float fxRevWet { 0.0f };
    int fxDelPreset { 1 }; float fxDelMix { 0.0f };
    int fxOctave { 0 };
public:
    void setFXEQ(float lowDb, float midDb, float highDb) { fxEqLowDb = lowDb; fxEqMidDb = midDb; fxEqHighDb = highDb; }
    void setFXReverb(int preset, float wet) { fxRevPreset = preset; fxRevWet = wet; }
    void setFXDelay(int preset, float mix) { fxDelPreset = preset; fxDelMix = mix; }
    void setOctave(int o) { fxOctave = juce::jlimit(-5,5,o); }
};
