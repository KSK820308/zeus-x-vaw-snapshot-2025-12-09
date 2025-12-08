#pragma once
#include <JuceHeader.h>

class AudioEngine : private juce::MidiInputCallback, private juce::Timer {
public:
    AudioEngine() {
        deviceManager.initialiseWithDefaultDevices(0, 2);
        sourcePlayer.setSource(&synthSource);
        deviceManager.addAudioCallback(&sourcePlayer);
        synth.clearVoices();
        // Simple sine-based sound
        struct SimpleSound : public juce::SynthesiserSound { bool appliesToNote(int) override { return true; } bool appliesToChannel(int) override { return true; } };
        struct SimpleVoice : public juce::SynthesiserVoice {
            juce::dsp::Oscillator<float> osc { [](float x){ return std::sin(x); } };
            float level = 0.2f; double sr = 44100.0;
            bool canPlaySound(juce::SynthesiserSound* s) override { return dynamic_cast<SimpleSound*>(s) != nullptr; }
            void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound*, int) override {
                auto freq = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
                osc.setFrequency(freq);
                level = juce::jlimit(0.0f, 1.0f, velocity);
            }
            void stopNote(float, bool allowTailOff) override { clearCurrentNote(); }
            void pitchWheelMoved(int) override {}
            void controllerMoved(int, int) override {}
            void renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override {
                juce::dsp::AudioBlock<float> block(outputBuffer);
                auto chBlock = block.getSubBlock((size_t) startSample, (size_t) numSamples);
                for (size_t ch = 0; ch < chBlock.getNumChannels(); ++ch) {
                    auto* data = chBlock.getChannelPointer(ch);
                    for (int i = 0; i < numSamples; ++i) data[i] += osc.processSample(0.0f) * level * 0.25f;
                }
            }
            void prepare(double sampleRate, int) { sr = sampleRate; osc.prepare({ sampleRate, 512, 1 }); }
        };
        for (int i = 0; i < 8; ++i) synth.addVoice(new SimpleVoice());
        synth.addSound(new SimpleSound());
        startTimerHz(60);
    }
    ~AudioEngine() {
        deviceManager.removeAudioCallback(&sourcePlayer);
        sourcePlayer.setSource(nullptr);
        closeMidiInput();
    }

    void setTempo(int t) { tempo = juce::jlimit(20, 300, t); }
    int getTempo() const { return tempo; }

    juce::StringArray getMidiDevices() const {
        auto devs = juce::MidiInput::getAvailableDevices();
        juce::StringArray names;
        for (auto& d : devs) names.add(d.name);
        return names;
    }
    void setMidiInput(const juce::String& name) {
        closeMidiInput();
        auto devs = juce::MidiInput::getAvailableDevices();
        for (auto& d : devs) if (d.name == name) { currentMidi = d; deviceManager.setMidiInputDeviceEnabled(d.identifier, true); deviceManager.addMidiInputDeviceCallback(d.identifier, this); break; }
    }

private:
    void handleIncomingMidiMessage(juce::MidiInput*, const juce::MidiMessage& m) override { midiCollector.addMessageToQueue(m); }
    void timerCallback() override {}
    void closeMidiInput() { if (currentMidi.identifier.isNotEmpty()) { deviceManager.removeMidiInputDeviceCallback(currentMidi.identifier, this); deviceManager.setMidiInputDeviceEnabled(currentMidi.identifier, false); } currentMidi = {}; }

    juce::AudioDeviceManager deviceManager;
    juce::AudioSourcePlayer sourcePlayer;
    juce::Synthesiser synth;
    struct SynthSource : public juce::AudioSource {
        juce::Synthesiser& s;
        juce::MidiMessageCollector& coll;
        SynthSource(juce::Synthesiser& sy, juce::MidiMessageCollector& c) : s(sy), coll(c) {}
        void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override { s.setCurrentPlaybackSampleRate(sampleRate); }
        void releaseResources() override {}
        void getNextAudioBlock(const juce::AudioSourceChannelInfo& info) override {
            juce::MidiBuffer mb; coll.removeNextBlockOfMessages(mb, info.numSamples);
            s.renderNextBlock(*info.buffer, mb, 0, info.numSamples);
        }
    } synthSource { synth, midiCollector };
    juce::MidiMessageCollector midiCollector;
    juce::MidiDeviceInfo currentMidi;
    int tempo { 120 };
};
