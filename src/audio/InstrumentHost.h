#pragma once
#include <JuceHeader.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <array>
#include "GainProcessor.h"
#include "BusRegistry.h"
#include "PanVolumeProcessor.h"
#include "../ui/PluginManager.h"

// Simple MIDI transpose processor per channel
class MidiTransposeProcessor : public juce::AudioProcessor {
public:
    MidiTransposeProcessor() : juce::AudioProcessor(BusesProperties().withInput("in", juce::AudioChannelSet::stereo(), true)
                                                                          .withOutput("out", juce::AudioChannelSet::stereo(), true)) {}
    const juce::String getName() const override { return "MidiTransposeProcessor"; }
    void prepareToPlay(double, int) override {}
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override {
        return layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo()
            && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
    }
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override {
        juce::ignoreUnused(buffer);
        if (!enabled) { midi.clear(); return; }
        juce::MidiBuffer out;
        for (const auto metadata : midi) {
            auto msg = metadata.getMessage();
            int ch = juce::jlimit(1, 16, msg.getChannel());
            int transpose = semis[ch - 1] + octs[ch - 1] * 12;
            if (msg.isNoteOn()) {
                int note = juce::jlimit(0, 127, msg.getNoteNumber() + transpose);
                auto newMsg = juce::MidiMessage::noteOn(msg.getChannel(), note, msg.getVelocity());
                out.addEvent(newMsg, metadata.samplePosition);
            } else if (msg.isNoteOff()) {
                int note = juce::jlimit(0, 127, msg.getNoteNumber() + transpose);
                auto newMsg = juce::MidiMessage::noteOff(msg.getChannel(), note);
                out.addEvent(newMsg, metadata.samplePosition);
            } else {
                out.addEvent(msg, metadata.samplePosition);
            }
        }
        midi.swapWith(out);
    }
    bool hasEditor() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    double getTailLengthSeconds() const override { return 0.0; }
    const juce::String getParameterName(int) override { return {}; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void setStateInformation(const void*, int) override {}
    void getStateInformation(juce::MemoryBlock&) override {}
    void setTranspose(int s) { for (int i = 0; i < 16; ++i) semis[i] = s; }
    void setOctave(int o) { for (int i = 0; i < 16; ++i) octs[i] = o; }
    void setTransposeForChannel(int channel, int s) { channel = juce::jlimit(1,16,channel); semis[channel - 1] = juce::jlimit(-12, 12, s); }
    void setOctaveForChannel(int channel, int o) { channel = juce::jlimit(1,16,channel); octs[channel - 1] = juce::jlimit(-5, 5, o); }
    void setEnabled(bool en) { enabled = en; }
private:
    int semis[16] {};
    int octs[16] {};
    bool enabled { true };
};

class InstrumentHost {
public:
    static InstrumentHost& instance() { static InstrumentHost h; return h; }
    

    void init() {
        if (initialised) return;
        deviceManager.initialiseWithDefaultDevices(0, 2);
        deviceManager.addAudioCallback(&player);
        initialised = true;
    }
    juce::String getActiveChannel() const { return activeChannelId; }
    double getDeviceSampleRate() const {
        if (auto* dev = deviceManager.getCurrentAudioDevice()) return dev->getCurrentSampleRate();
        return 44100.0;
    }
    int getDeviceBlockSize() const {
        if (auto* dev = deviceManager.getCurrentAudioDevice()) return dev->getCurrentBufferSizeSamples();
        return 512;
    }

    juce::StringArray getMidiDevices() const {
        auto devs = juce::MidiInput::getAvailableDevices();
        juce::StringArray names; for (auto& d : devs) names.add(d.name); return names;
    }
    void ensureDefaultMidiInput() {
        auto devs = juce::MidiInput::getAvailableDevices();
        if (currentMidi.identifier.isEmpty() && devs.size() > 0) setMidiInput(devs[0].name);
    }

    void setMidiInput(const juce::String& name) {
        init();
        auto devs = juce::MidiInput::getAvailableDevices();
        // disable current
        if (currentMidi.identifier.isNotEmpty()) {
            deviceManager.removeMidiInputDeviceCallback(currentMidi.identifier, &player);
            if (tap) deviceManager.removeMidiInputDeviceCallback(currentMidi.identifier, tap.get());
            deviceManager.setMidiInputDeviceEnabled(currentMidi.identifier, false);
            currentMidi = {};
        }
        for (auto& d : devs) if (d.name == name) {
            currentMidi = d;
            deviceManager.setMidiInputDeviceEnabled(d.identifier, true);
            deviceManager.addMidiInputDeviceCallback(d.identifier, &player);
            ensureTap(); deviceManager.addMidiInputDeviceCallback(d.identifier, tap.get());
            break;
        }
    }

    void setInstrument(std::unique_ptr<juce::AudioPluginInstance> inst) {
        init();
        ensureDefaultMidiInput();
        instrumentWindow = nullptr;
        if (instrumentEditorComponent) {
            if (overlayLayer) overlayLayer->removeChildComponent(instrumentEditorComponent.get());
            instrumentEditorComponent = nullptr;
        }
        if (overlayLayer) overlayLayer->setVisible(false);
        if (!graph) graph.reset(new juce::AudioProcessorGraph());
        player.setProcessor(graph.get());
        graph->clear();
        audioOut = graph->addNode(std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(juce::AudioProcessorGraph::AudioGraphIOProcessor::audioOutputNode));
        midiIn  = graph->addNode(std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(juce::AudioProcessorGraph::AudioGraphIOProcessor::midiInputNode));
        instNode = graph->addNode(std::move(inst));
        instrumentProc = instNode ? instNode->getProcessor() : nullptr;
        if (auto* api = dynamic_cast<juce::AudioPluginInstance*>(instrumentProc))
            instrumentDesc = api->getPluginDescription();
        else
            instrumentDesc = juce::PluginDescription{};
        panNode = graph->addNode(std::make_unique<PanVolumeProcessor>());
        if (instNode && panNode && audioOut) {
            graph->addConnection({ { instNode->nodeID, 0 }, { panNode->nodeID, 0 } });
            graph->addConnection({ { instNode->nodeID, 1 }, { panNode->nodeID, 1 } });
            graph->addConnection({ { panNode->nodeID, 0 }, { audioOut->nodeID, 0 } });
            graph->addConnection({ { panNode->nodeID, 1 }, { audioOut->nodeID, 1 } });
            if (midiIn) {
                midiXGlobal = graph->addNode(std::make_unique<MidiTransposeProcessor>());
                graph->addConnection({ { midiIn->nodeID, juce::AudioProcessorGraph::midiChannelIndex }, { midiXGlobal->nodeID, juce::AudioProcessorGraph::midiChannelIndex } });
                graph->addConnection({ { midiXGlobal->nodeID, juce::AudioProcessorGraph::midiChannelIndex }, { instNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex } });
                channelMode = false;
                if (auto* mp = dynamic_cast<MidiTransposeProcessor*>(midiXGlobal ? midiXGlobal->getProcessor() : nullptr)) mp->setEnabled(true);
            }
        }
        updateSends();
        reprepare();
    }
    void setInstrumentProcessor(std::unique_ptr<juce::AudioProcessor> proc) {
        init();
        ensureDefaultMidiInput();
        instrumentWindow = nullptr;
        if (instrumentEditorComponent) {
            if (overlayLayer) overlayLayer->removeChildComponent(instrumentEditorComponent.get());
            instrumentEditorComponent = nullptr;
        }
        if (overlayLayer) overlayLayer->setVisible(false);
        if (!graph) graph.reset(new juce::AudioProcessorGraph());
        player.setProcessor(graph.get());
        graph->clear();
        audioOut = graph->addNode(std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(juce::AudioProcessorGraph::AudioGraphIOProcessor::audioOutputNode));
        midiIn  = graph->addNode(std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(juce::AudioProcessorGraph::AudioGraphIOProcessor::midiInputNode));
        instNode = graph->addNode(std::move(proc));
        instrumentProc = instNode ? instNode->getProcessor() : nullptr;
        panNode = graph->addNode(std::make_unique<PanVolumeProcessor>());
        if (instNode && panNode && audioOut) {
            graph->addConnection({ { instNode->nodeID, 0 }, { panNode->nodeID, 0 } });
            graph->addConnection({ { instNode->nodeID, 1 }, { panNode->nodeID, 1 } });
            graph->addConnection({ { panNode->nodeID, 0 }, { audioOut->nodeID, 0 } });
            graph->addConnection({ { panNode->nodeID, 1 }, { audioOut->nodeID, 1 } });
            if (midiIn) {
                midiXGlobal = graph->addNode(std::make_unique<MidiTransposeProcessor>());
                graph->addConnection({ { midiIn->nodeID, juce::AudioProcessorGraph::midiChannelIndex }, { midiXGlobal->nodeID, juce::AudioProcessorGraph::midiChannelIndex } });
                graph->addConnection({ { midiXGlobal->nodeID, juce::AudioProcessorGraph::midiChannelIndex }, { instNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex } });
                channelMode = false;
                if (auto* mp = dynamic_cast<MidiTransposeProcessor*>(midiXGlobal ? midiXGlobal->getProcessor() : nullptr)) mp->setEnabled(true);
            }
        }
        updateSends();
        reprepare();
    }

    juce::AudioPluginInstance* getInstrument() const { return dynamic_cast<juce::AudioPluginInstance*>(instrumentProc); }
    juce::AudioProcessor* getCurrentProcessor() const { return instrumentProc; }

    void openEditor() {
        auto* api = dynamic_cast<juce::AudioPluginInstance*>(instrumentProc);
        if (api == nullptr) return;
        if (auto* ed = api->createEditor()) {
            instrumentWindow = nullptr;
            instrumentEditorComponent.reset(ed);
            ensureOverlayLayer();
            if (overlayParent && overlayLayer) {
                overlayParent->addAndMakeVisible(overlayLayer.get());
                overlayLayer->addAndMakeVisible(instrumentEditorComponent.get());
                overlayLayer->setVisible(true);
                updateOverlayBounds();
                overlayLayer->toFront(true);
                instrumentEditorComponent->toFront(true);
            }
        }
    }

    void focusEditor() { if (instrumentEditorComponent) instrumentEditorComponent->toFront(true); else if (instrumentWindow) instrumentWindow->toFront(true); }
    bool isInstrumentEditorVisible() const { return instrumentEditorComponent && instrumentEditorComponent->isVisible(); }
    void hideEditor() { if (instrumentEditorComponent) instrumentEditorComponent->setVisible(false); }
    void showEditor() {
        ensureOverlayLayer();
        if (!overlayParent || !instrumentEditorComponent) { openEditor(); return; }
        if (overlayLayer) {
            overlayParent->addAndMakeVisible(overlayLayer.get());
            overlayLayer->setVisible(true);
            overlayLayer->addAndMakeVisible(instrumentEditorComponent.get());
            instrumentEditorComponent->setVisible(true);
            updateOverlayBounds();
            overlayLayer->toFront(true);
            instrumentEditorComponent->toFront(true);
        }
    }
    void toggleEditor() { if (isInstrumentEditorVisible()) hideEditor(); else showEditor(); }
    void removeInstrument() { instrumentEditorComponent = nullptr; instrumentWindow = nullptr; instrumentProc = nullptr; instNode = nullptr; panNode = nullptr; graph = nullptr; }

    void setSends(const std::vector<std::pair<int, float>>& s) { sends = s; updateSends(); }
    void onBusChanged() { updateSends(); }
    std::vector<std::pair<int,float>> getSends() const { return sends; }

    void shutdown() {
        instrumentEditorComponent = nullptr;
        instrumentWindow = nullptr;
        player.setProcessor(nullptr);
        if (currentMidi.identifier.isNotEmpty()) {
            deviceManager.removeMidiInputDeviceCallback(currentMidi.identifier, &player);
            if (tap) deviceManager.removeMidiInputDeviceCallback(currentMidi.identifier, tap.get());
            deviceManager.setMidiInputDeviceEnabled(currentMidi.identifier, false);
            currentMidi = {};
        }
        graph = nullptr;
        instrumentGui = nullptr;
        deviceManager.removeAudioCallback(&player);
        midiOut = nullptr;
    }

private:
    bool initialised { false };
    juce::AudioDeviceManager deviceManager;
    juce::AudioProcessorPlayer player;
    std::unique_ptr<juce::MidiOutput> midiOut;
    juce::MidiDeviceInfo currentMidi;
    std::unique_ptr<juce::AudioPluginInstance> instrumentGui;
    juce::PluginDescription instrumentDesc;
    std::unique_ptr<juce::AudioProcessorGraph> graph;
    std::vector<std::pair<int,float>> sends;
    std::unique_ptr<juce::DocumentWindow> instrumentWindow;
    juce::Component* overlayParent { nullptr };
    std::unique_ptr<juce::Component> instrumentEditorComponent;
    std::array<std::unique_ptr<juce::Component>, 8> busEditorComponents;
    juce::AudioProcessorGraph::Node::Ptr panNode;
    juce::AudioProcessorGraph::Node::Ptr instNode;
    juce::AudioProcessorGraph::Node::Ptr midiXGlobal;
    juce::AudioProcessorGraph::Node::Ptr audioOut;
    juce::AudioProcessorGraph::Node::Ptr midiIn;
    juce::AudioProcessorGraph::Node::Ptr eqNode;
    juce::AudioProcessorGraph::Node::Ptr revNode;
    juce::AudioProcessorGraph::Node::Ptr delNode;
    std::array<juce::AudioProcessorGraph::Node::Ptr, 8> busNodes{};
    std::vector<juce::AudioProcessorGraph::Node::Ptr> sendGainNodes;
    std::vector<int> sendGainBusIdx;
    juce::AudioProcessor* instrumentProc { nullptr };
    float channelVolume { 1.0f };
    float panCalibration { 0.28f };
    bool channelMode { false };
    juce::String activeChannelId;
    void setVolume(float v) { if (panNode) if (auto* pv = dynamic_cast<PanVolumeProcessor*>(panNode->getProcessor())) pv->setVolume(v); }
    void setPan(float p) { if (panNode) if (auto* pv = dynamic_cast<PanVolumeProcessor*>(panNode->getProcessor())) pv->setPan(p); }
    void setBalance(float b) { if (panNode) if (auto* pv = dynamic_cast<PanVolumeProcessor*>(panNode->getProcessor())) pv->setBalance(b); }
    void setSwap(bool s) { if (panNode) if (auto* pv = dynamic_cast<PanVolumeProcessor*>(panNode->getProcessor())) pv->setSwap(s); }
public:
    void setEQ(float lowDb, float lowHz, float midDb, float midHz, float midQ, float highDb, float highHz) {}
    void setReverb(float room, float damp, float width, float wet, float dry, bool freeze) {}
    void setDelay(float timeMs, float feedback, float mix) {}
    void setPanCalibration(float o) {
        panCalibration = juce::jlimit(-0.5f, 0.5f, o);
        if (auto* pv = dynamic_cast<PanVolumeProcessor*>(panNode ? panNode->getProcessor() : nullptr)) pv->setPanBias(panCalibration);
        for (auto& kv : channels) {
            auto& ch = kv.second;
            if (auto* pv2 = dynamic_cast<PanVolumeProcessor*>(ch.panNode ? ch.panNode->getProcessor() : nullptr)) pv2->setPanBias(panCalibration);
        }
    }
    void setEffectsEnabled(bool en) { effectsEnabled = en; connectInstrumentChain(); }
    void setMidiOutput(const juce::String& name) {
        init();
        midiOut = nullptr;
        auto devs = juce::MidiOutput::getAvailableDevices();
        for (auto& d : devs) if (d.name == name) { midiOut = juce::MidiOutput::openDevice(d.identifier); break; }
    }
    void setOverlayParent(juce::Component* parent) { overlayParent = parent; }
    void updateOverlayBounds() {
        if (!overlayParent) return;
        ensureOverlayLayer();
        auto b = overlayParent->getLocalBounds();
        if (overlayLayer) overlayLayer->setBounds(b);
        if (instrumentEditorComponent && instrumentEditorComponent->isVisible()) {
            int w = juce::jmin(instrumentEditorComponent->getWidth(), b.getWidth() * 9 / 10);
            int h = juce::jmin(instrumentEditorComponent->getHeight(), b.getHeight() * 9 / 10);
            int x = b.getCentreX() - w/2;
            int y = b.getCentreY() - h/2;
            instrumentEditorComponent->setBounds(x, y, w, h);
        }
        for (int i = 0; i < 8; ++i) {
            auto& comp = busEditorComponents[i];
            if (comp && comp->isVisible()) {
                int w = juce::jmin(comp->getWidth(), b.getWidth() * 9 / 10);
                int h = juce::jmin(comp->getHeight(), b.getHeight() * 9 / 10);
                int x = b.getCentreX() - w/2;
                int y = b.getCentreY() - h/2;
                comp->setBounds(x, y, w, h);
            }
        }
    }
    void openBusEditor(int index) {
        if (index < 1 || index > 8) return;
        auto* api = getBusInstance(index);
        if (!api) return;
        if (!overlayParent) return;
        ensureOverlayLayer();
        if (busEditorComponents[index - 1]) { overlayLayer->setVisible(true); overlayLayer->toFront(true); busEditorComponents[index - 1]->toFront(true); updateOverlayBounds(); return; }
        if (auto* ed = api->createEditor()) {
            busEditorComponents[index - 1].reset(ed);
            overlayParent->addAndMakeVisible(overlayLayer.get());
            overlayLayer->addAndMakeVisible(busEditorComponents[index - 1].get());
            overlayLayer->setVisible(true);
            updateOverlayBounds();
            overlayLayer->toFront(true);
            busEditorComponents[index - 1]->toFront(true);
        }
    }
    void focusBusEditor(int index) { if (index >= 1 && index <= 8) { auto& c = busEditorComponents[index - 1]; if (c) c->toFront(true); } }
    bool isBusEditorVisible(int index) const { if (index < 1 || index > 8) return false; auto& c = busEditorComponents[index - 1]; return c && c->isVisible(); }
    void hideBusEditor(int index) { if (index < 1 || index > 8) return; auto& c = busEditorComponents[index - 1]; if (c) c->setVisible(false); }
    void showBusEditor(int index) {
        if (index < 1 || index > 8) return;
        ensureOverlayLayer();
        auto& c = busEditorComponents[index - 1];
        if (!overlayParent) return;
        if (c) {
            overlayParent->addAndMakeVisible(overlayLayer.get());
            overlayLayer->setVisible(true);
            overlayLayer->addAndMakeVisible(c.get());
            c->setVisible(true);
            updateOverlayBounds();
            overlayLayer->toFront(true);
            c->toFront(true);
        } else {
            openBusEditor(index);
        }
    }
    void toggleBusEditor(int index) { if (isBusEditorVisible(index)) hideBusEditor(index); else showBusEditor(index); }
    void setChannelVolume(float vol01) { channelVolume = juce::jlimit(0.0f, 1.0f, vol01); setVolume(channelVolume); for (size_t i = 0; i < sends.size() && i < sendGainNodes.size(); ++i) { auto node = sendGainNodes[i]; float gain = sends[i].second; if (auto* gp = dynamic_cast<GainProcessor*>(node ? node->getProcessor() : nullptr)) gp->setGain(juce::jlimit(0.0f, 1.0f, gain * channelVolume)); } }
    void setChannelPan(float panMinus1To1) { setPan(panMinus1To1); }
    void setChannelBalance(float balanceMinus1To1) { setBalance(balanceMinus1To1); }
    void setChannelSwap(bool swapLR) { setSwap(swapLR); }
    void setMute(bool m) { if (m) setVolume(0.0f); else setVolume(channelVolume); }
    void sendProgramChange(int program, int midiChannel = 1) {
        init();
        program = juce::jlimit(0, 127, program);
        auto msg = juce::MidiMessage::programChange(midiChannel, program);
        player.getMidiMessageCollector().addMessageToQueue(msg);
        if (midiOut) midiOut->sendMessageNow(msg);
    }
    void sendRawMidi(const juce::MidiMessage& m) {
        init();
        player.getMidiMessageCollector().addMessageToQueue(m);
        if (midiOut) midiOut->sendMessageNow(m);
    }
    void sendBankSelect(int msb, int lsb, int midiChannel = 1) {
        init();
        msb = juce::jlimit(0, 127, msb);
        lsb = juce::jlimit(0, 127, lsb);
        auto cc0  = juce::MidiMessage::controllerEvent(midiChannel, 0, msb);
        auto cc32 = juce::MidiMessage::controllerEvent(midiChannel, 32, lsb);
        player.getMidiMessageCollector().addMessageToQueue(cc0);
        player.getMidiMessageCollector().addMessageToQueue(cc32);
        if (midiOut) { midiOut->sendMessageNow(cc0); midiOut->sendMessageNow(cc32); }
    }

    bool loadSf2IntoAUSampler(const juce::File& sf2File, int bankMsb, int bankLsb, int preset);

    // Multi-channel management
    struct ChannelSlot {
        juce::String id;
        juce::AudioProcessorGraph::Node::Ptr instNode;
        juce::AudioProcessorGraph::Node::Ptr panNode;
        juce::AudioProcessorGraph::Node::Ptr midiXNode;
        juce::AudioPluginInstance* instrumentProc { nullptr };
        std::unique_ptr<juce::Component> editorComponent;
        float volume { 1.0f };
        float pan { 0.0f };
        int transpose { 0 };
        int octave { 0 };
        std::vector<std::pair<int,float>> sends;
        std::vector<juce::AudioProcessorGraph::Node::Ptr> sendGainNodes;
        std::vector<int> sendGainBusIdx;
        juce::File lastVoiceFile;
    };
    void setChannelInstrument(const juce::String& id, std::unique_ptr<juce::AudioPluginInstance> inst) {
        init();
        ensureGraph();
        if (instNode) { graph->removeNode(instNode.get()); instNode = nullptr; }
        if (panNode)  { graph->removeNode(panNode.get());  panNode = nullptr; }
        auto& ch = channels[id]; ch.id = id;
        
        // CRITICAL: Delete editor before plugin to avoid crash
        if (ch.editorComponent) {
            if (overlayLayer) overlayLayer->removeChildComponent(ch.editorComponent.get());
            ch.editorComponent.reset();
        }

        // remove old nodes
        if (ch.panNode) graph->removeNode(ch.panNode.get());
        if (ch.midiXNode) graph->removeNode(ch.midiXNode.get());
        if (ch.instNode) graph->removeNode(ch.instNode.get());
        // create nodes
        ch.instNode = graph->addNode(std::move(inst));
        ch.panNode  = graph->addNode(std::make_unique<PanVolumeProcessor>());
        ch.midiXNode= graph->addNode(std::make_unique<MidiTransposeProcessor>());
        // connect: midiIn -> midiX -> inst -> pan -> audioOut
        if (midiIn) {
            graph->addConnection({ { midiIn->nodeID, juce::AudioProcessorGraph::midiChannelIndex }, { ch.midiXNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex } });
            graph->addConnection({ { ch.midiXNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex }, { ch.instNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex } });
        }
        graph->addConnection({ { ch.instNode->nodeID, 0 }, { ch.panNode->nodeID, 0 } });
        graph->addConnection({ { ch.instNode->nodeID, 1 }, { ch.panNode->nodeID, 1 } });
        if (audioOut) {
            graph->addConnection({ { ch.panNode->nodeID, 0 }, { audioOut->nodeID, 0 } });
            graph->addConnection({ { ch.panNode->nodeID, 1 }, { audioOut->nodeID, 1 } });
        }
        ch.instrumentProc = ch.instNode ? dynamic_cast<juce::AudioPluginInstance*>(ch.instNode->getProcessor()) : nullptr;
        if (auto* pv = dynamic_cast<PanVolumeProcessor*>(ch.panNode ? ch.panNode->getProcessor() : nullptr)) pv->setPanBias(panCalibration);
        channelMode = true;
        if (auto* mp = dynamic_cast<MidiTransposeProcessor*>(ch.midiXNode ? ch.midiXNode->getProcessor() : nullptr)) mp->setEnabled(ch.id == activeChannelId);
        if (auto* mpG = dynamic_cast<MidiTransposeProcessor*>(midiXGlobal ? midiXGlobal->getProcessor() : nullptr)) mpG->setEnabled(false);
        applyChannelParams(id);
        reprepare();
    }
    void setChannelProcessor(const juce::String& id, std::unique_ptr<juce::AudioProcessor> proc) {
        init();
        ensureGraph();
        if (instNode) { graph->removeNode(instNode.get()); instNode = nullptr; }
        if (panNode)  { graph->removeNode(panNode.get());  panNode = nullptr; }
        auto& ch = channels[id]; ch.id = id;

        if (ch.editorComponent) {
            if (overlayLayer) overlayLayer->removeChildComponent(ch.editorComponent.get());
            ch.editorComponent.reset();
        }
        if (ch.panNode) graph->removeNode(ch.panNode.get());
        if (ch.midiXNode) graph->removeNode(ch.midiXNode.get());
        if (ch.instNode) graph->removeNode(ch.instNode.get());

        ch.instNode = graph->addNode(std::move(proc));
        ch.panNode  = graph->addNode(std::make_unique<PanVolumeProcessor>());
        ch.midiXNode= graph->addNode(std::make_unique<MidiTransposeProcessor>());

        if (midiIn) {
            graph->addConnection({ { midiIn->nodeID, juce::AudioProcessorGraph::midiChannelIndex }, { ch.midiXNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex } });
            graph->addConnection({ { ch.midiXNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex }, { ch.instNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex } });
        }
        graph->addConnection({ { ch.instNode->nodeID, 0 }, { ch.panNode->nodeID, 0 } });
        graph->addConnection({ { ch.instNode->nodeID, 1 }, { ch.panNode->nodeID, 1 } });
        if (audioOut) {
            graph->addConnection({ { ch.panNode->nodeID, 0 }, { audioOut->nodeID, 0 } });
            graph->addConnection({ { ch.panNode->nodeID, 1 }, { audioOut->nodeID, 1 } });
        }
        if (auto* pv = dynamic_cast<PanVolumeProcessor*>(ch.panNode ? ch.panNode->getProcessor() : nullptr)) pv->setPanBias(panCalibration);
        channelMode = true;
        if (auto* mp = dynamic_cast<MidiTransposeProcessor*>(ch.midiXNode ? ch.midiXNode->getProcessor() : nullptr)) mp->setEnabled(ch.id == activeChannelId);
        if (auto* mpG = dynamic_cast<MidiTransposeProcessor*>(midiXGlobal ? midiXGlobal->getProcessor() : nullptr)) mpG->setEnabled(false);
        applyChannelParams(id);
        reprepare();
    }
    void setChannelLastVoiceFile(const juce::String& id, const juce::File& f) {
        auto it = channels.find(id); if (it == channels.end()) return; it->second.lastVoiceFile = f;
    }
    juce::File getChannelLastVoiceFile(const juce::String& id) const {
        auto it = channels.find(id); if (it == channels.end()) return {}; return it->second.lastVoiceFile;
    }
    void toggleChannelEditor(const juce::String& id) {
        auto it = channels.find(id);
        if (it == channels.end()) return;
        auto& ch = it->second;
        auto* api = dynamic_cast<juce::AudioPluginInstance*>(ch.instrumentProc);
        if (!api) return;
        if (!overlayParent) return;
        ensureOverlayLayer();
        if (!ch.editorComponent) {
            if (auto* ed = api->createEditor()) ch.editorComponent.reset(ed);
        }
        if (ch.editorComponent) {
            bool visible = ch.editorComponent->isVisible();
            overlayParent->addAndMakeVisible(overlayLayer.get());
            overlayLayer->addAndMakeVisible(ch.editorComponent.get());
            overlayLayer->setVisible(true);
            ch.editorComponent->setVisible(!visible);
            updateOverlayBounds();
            overlayLayer->toFront(true);
            ch.editorComponent->toFront(true);
        }
    }
    void setChannelVolume(const juce::String& id, float vol01) {
        auto it = channels.find(id); if (it == channels.end()) return; auto& ch = it->second; ch.volume = juce::jlimit(0.0f,1.0f,vol01);
        if (auto* pv = dynamic_cast<PanVolumeProcessor*>(ch.panNode ? ch.panNode->getProcessor() : nullptr)) pv->setVolume(ch.volume);
        for (size_t i = 0; i < ch.sends.size() && i < ch.sendGainNodes.size(); ++i) {
            auto node = ch.sendGainNodes[i]; float gain = ch.sends[i].second;
            if (auto* gp = dynamic_cast<GainProcessor*>(node ? node->getProcessor() : nullptr)) gp->setGain(juce::jlimit(0.0f, 1.0f, gain * ch.volume));
        }
    }
    void setChannelPan(const juce::String& id, float panMinus1To1) {
        auto it = channels.find(id); if (it == channels.end()) return; auto& ch = it->second; ch.pan = juce::jlimit(-1.0f,1.0f,panMinus1To1);
        if (auto* pv = dynamic_cast<PanVolumeProcessor*>(ch.panNode ? ch.panNode->getProcessor() : nullptr)) pv->setPan(ch.pan);
    }
    void setChannelMute(const juce::String& id, bool m) {
        auto it = channels.find(id); if (it == channels.end()) return; auto& ch = it->second;
        if (auto* pv = dynamic_cast<PanVolumeProcessor*>(ch.panNode ? ch.panNode->getProcessor() : nullptr)) pv->setVolume(m ? 0.0f : ch.volume);
    }
    void setChannelTranspose(const juce::String& id, int semis) {
        auto it = channels.find(id); if (it == channels.end()) return; auto& ch = it->second; ch.transpose = juce::jlimit(-12, 12, semis);
        if (auto* mp = dynamic_cast<MidiTransposeProcessor*>(ch.midiXNode ? ch.midiXNode->getProcessor() : nullptr)) mp->setTranspose(ch.transpose);
    }
    void setChannelOctave(const juce::String& id, int oct) {
        auto it = channels.find(id); if (it == channels.end()) return; auto& ch = it->second; ch.octave = juce::jlimit(-5, 5, oct);
        if (auto* mp = dynamic_cast<MidiTransposeProcessor*>(ch.midiXNode ? ch.midiXNode->getProcessor() : nullptr)) mp->setOctave(ch.octave);
    }
    void setMidiTranspose(int midiChannel, int semis) {
        if (auto* mp = dynamic_cast<MidiTransposeProcessor*>(midiXGlobal ? midiXGlobal->getProcessor() : nullptr)) mp->setTransposeForChannel(midiChannel, semis);
    }
    void setMidiOctave(int midiChannel, int oct) {
        if (auto* mp = dynamic_cast<MidiTransposeProcessor*>(midiXGlobal ? midiXGlobal->getProcessor() : nullptr)) mp->setOctaveForChannel(midiChannel, oct);
    }
    void setChannelSends(const juce::String& id, const std::vector<std::pair<int,float>>& s) {
        auto it = channels.find(id); if (it == channels.end()) return; auto& ch = it->second; ch.sends = s; updateChannelSends(id);
    }
    std::vector<std::pair<int,float>> getChannelSends(const juce::String& id) const {
        auto it = channels.find(id); if (it == channels.end()) return {}; return it->second.sends;
    }
    void setActiveChannel(const juce::String& id) {
        activeChannelId = id;
        for (auto& kv : channels) {
            auto& ch = kv.second;
            if (auto* mp = dynamic_cast<MidiTransposeProcessor*>(ch.midiXNode ? ch.midiXNode->getProcessor() : nullptr)) mp->setEnabled(ch.id == activeChannelId);
        }
        if (auto* mpG = dynamic_cast<MidiTransposeProcessor*>(midiXGlobal ? midiXGlobal->getProcessor() : nullptr)) mpG->setEnabled(!channelMode);
    }
    void removeChannelInstrument(const juce::String& id) {
        auto it = channels.find(id); if (it == channels.end()) return; auto& ch = it->second;
        if (ch.editorComponent) { if (overlayLayer) overlayLayer->removeChildComponent(ch.editorComponent.get()); ch.editorComponent.reset(); }
        if (ch.instNode) { graph->removeNode(ch.instNode.get()); ch.instNode = nullptr; }
        if (ch.panNode)  { graph->removeNode(ch.panNode.get());  ch.panNode = nullptr; }
        if (ch.midiXNode){ if (auto* mp = dynamic_cast<MidiTransposeProcessor*>(ch.midiXNode->getProcessor())) mp->setEnabled(false); }
        ch.instrumentProc = nullptr;
        ch.lastVoiceFile = juce::File();
        ch.sends.clear(); updateChannelSends(id);
        reprepare();
    }
    void removeInstrumentByFile(const juce::File& f) {
        for (auto& kv : channels) {
            auto& ch = kv.second;
            if (ch.lastVoiceFile.existsAsFile() && ch.lastVoiceFile.getFullPathName() == f.getFullPathName()) {
                removeChannelInstrument(ch.id);
            }
        }
    }
private:
    bool effectsEnabled { false };
    void ensureGraph() {
        if (!graph) graph.reset(new juce::AudioProcessorGraph());
        player.setProcessor(graph.get());
        if (!audioOut) audioOut = graph->addNode(std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(juce::AudioProcessorGraph::AudioGraphIOProcessor::audioOutputNode));
        if (!midiIn)  midiIn  = graph->addNode(std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(juce::AudioProcessorGraph::AudioGraphIOProcessor::midiInputNode));
    }
    void connectInstrumentChain() {
        if (!graph || !instNode || !audioOut) return;
        graph->disconnectNode(instNode->nodeID);
        if (panNode) graph->disconnectNode(panNode->nodeID);
        graph->addConnection({ { instNode->nodeID, 0 }, { audioOut->nodeID, 0 } });
        graph->addConnection({ { instNode->nodeID, 1 }, { audioOut->nodeID, 1 } });
        if (midiIn) {
            if (!midiXGlobal) midiXGlobal = graph->addNode(std::make_unique<MidiTransposeProcessor>());
            graph->addConnection({ { midiIn->nodeID, juce::AudioProcessorGraph::midiChannelIndex }, { midiXGlobal->nodeID, juce::AudioProcessorGraph::midiChannelIndex } });
            graph->addConnection({ { midiXGlobal->nodeID, juce::AudioProcessorGraph::midiChannelIndex }, { instNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex } });
        }
    }
    void applyChannelParams(const juce::String& id) {
        auto it = channels.find(id); if (it == channels.end()) return; auto& ch = it->second;
        setChannelVolume(id, ch.volume);
        setChannelPan(id, ch.pan);
        setChannelBalance(0.0f);
        setChannelSwap(false);
        setChannelTranspose(id, ch.transpose);
        setChannelOctave(id, ch.octave);
    }
    void updateSends() {
        init();
        if (!graph) return;
        if (audioOut == nullptr)
            audioOut = graph->addNode(std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(juce::AudioProcessorGraph::AudioGraphIOProcessor::audioOutputNode));
        if (instNode && panNode && audioOut) {
            if (midiIn && !midiXGlobal) {
                midiXGlobal = graph->addNode(std::make_unique<MidiTransposeProcessor>());
                graph->addConnection({ { midiIn->nodeID, juce::AudioProcessorGraph::midiChannelIndex }, { midiXGlobal->nodeID, juce::AudioProcessorGraph::midiChannelIndex } });
                graph->addConnection({ { midiXGlobal->nodeID, juce::AudioProcessorGraph::midiChannelIndex }, { instNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex } });
            }
        }
        // Maintain bus nodes without clearing the graph and apply latest states
        for (int busIdx = 1; busIdx <= 8; ++busIdx) {
            auto& node = busNodes[busIdx - 1];
            if (BusRegistry::instance().hasBus(busIdx)) {
                if (node == nullptr) {
                    juce::String err2;
                    double sr = getDeviceSampleRate(); int bs = getDeviceBlockSize();
                    auto desc = BusRegistry::instance().getBusDescription(busIdx);
                    auto busInst = PluginManager::instance().create(desc, sr, bs, err2);
                    if (busInst) {
                        auto state = BusRegistry::instance().getBusState(busIdx);
                        if (state.getSize() > 0) busInst->setStateInformation(state.getData(), (int) state.getSize());
                        node = graph->addNode(std::move(busInst));
                        graph->addConnection({ { node->nodeID, 0 }, { audioOut->nodeID, 0 } });
                        graph->addConnection({ { node->nodeID, 1 }, { audioOut->nodeID, 1 } });
                    }
                } else {
                    auto* proc = node->getProcessor();
                    auto* api = dynamic_cast<juce::AudioPluginInstance*>(proc);
                    auto desc = BusRegistry::instance().getBusDescription(busIdx);
                    if (api != nullptr) {
                        auto current = api->getPluginDescription();
                        bool different = (current.pluginFormatName != desc.pluginFormatName)
                                         || (current.fileOrIdentifier != desc.fileOrIdentifier)
                                         || (current.name != desc.name);
                        if (different) {
                            juce::String err3;
                            double sr = getDeviceSampleRate(); int bs = getDeviceBlockSize();
                            auto busInst2 = PluginManager::instance().create(desc, sr, bs, err3);
                            if (busInst2) {
                                auto state = BusRegistry::instance().getBusState(busIdx);
                                if (state.getSize() > 0)
                                    busInst2->setStateInformation(state.getData(), (int) state.getSize());
                                auto newNode = graph->addNode(std::move(busInst2));
                                graph->addConnection({ { newNode->nodeID, 0 }, { audioOut->nodeID, 0 } });
                                graph->addConnection({ { newNode->nodeID, 1 }, { audioOut->nodeID, 1 } });
                                graph->removeNode(node.get());
                                node = newNode;
                            }
                        } else {
                            auto state = BusRegistry::instance().getBusState(busIdx);
                            if (state.getSize() > 0)
                                api->setStateInformation(state.getData(), (int) state.getSize());
                        }
                    }
                }
            } else {
                if (node != nullptr) { graph->removeNode(node.get()); node = nullptr; }
                busEditorComponents[busIdx - 1] = nullptr;
            }
        }
        bool configSame = (sendGainBusIdx.size() == sends.size());
        if (configSame) {
            for (size_t i = 0; i < sends.size(); ++i) {
                if (sendGainBusIdx[i] != sends[i].first) { configSame = false; break; }
            }
        }
        if (!instNode) {
            for (auto& g : sendGainNodes) if (g) graph->removeNode(g.get());
            sendGainNodes.clear();
            sendGainBusIdx.clear();
        } else if (!configSame) {
            for (auto& g : sendGainNodes) if (g) graph->removeNode(g.get());
            sendGainNodes.clear();
            sendGainBusIdx.clear();
            for (auto& s : sends) {
                int busIdx = s.first; float gain = s.second;
                if (busIdx < 1 || busIdx > 8) continue;
                auto busNode = busNodes[busIdx - 1];
                if (busNode == nullptr) continue;
                auto gainNode = graph->addNode(std::make_unique<GainProcessor>());
                sendGainNodes.push_back(gainNode);
                sendGainBusIdx.push_back(busIdx);
                if (auto* gp = dynamic_cast<GainProcessor*>(gainNode->getProcessor())) gp->setGain(juce::jlimit(0.0f, 1.0f, gain * channelVolume));
                graph->addConnection({ { instNode->nodeID, 0 }, { gainNode->nodeID, 0 } });
                graph->addConnection({ { instNode->nodeID, 1 }, { gainNode->nodeID, 1 } });
                graph->addConnection({ { gainNode->nodeID, 0 }, { busNode->nodeID, 0 } });
                graph->addConnection({ { gainNode->nodeID, 1 }, { busNode->nodeID, 1 } });
            }
        } else {
            for (size_t i = 0; i < sends.size(); ++i) {
                auto node = sendGainNodes[i];
                float gain = sends[i].second;
                if (auto* gp = dynamic_cast<GainProcessor*>(node ? node->getProcessor() : nullptr)) gp->setGain(juce::jlimit(0.0f, 1.0f, gain * channelVolume));
            }
        }
        instrumentProc = instNode ? instNode->getProcessor() : nullptr;
    }
    void updateChannelSends(const juce::String& id) {
        init(); ensureGraph();
        auto it = channels.find(id); if (it == channels.end()) return; auto& ch = it->second;
        if (!ch.instNode) return;
        updateSends();
        // Build lookup of existing send nodes by bus index
        std::map<int, juce::AudioProcessorGraph::Node::Ptr> existing;
        for (size_t i = 0; i < ch.sendGainBusIdx.size() && i < ch.sendGainNodes.size(); ++i) {
            existing[ch.sendGainBusIdx[i]] = ch.sendGainNodes[i];
        }
        // Track which buses are desired after update
        std::set<int> desired;
        for (auto& s : ch.sends) desired.insert(s.first);
        // Remove nodes for buses no longer desired
        for (auto& kv : existing) {
            if (desired.find(kv.first) == desired.end()) {
                if (kv.second) graph->removeNode(kv.second.get());
            }
        }
        // Rebuild arrays to match desired order
        ch.sendGainNodes.clear(); ch.sendGainBusIdx.clear();
        for (auto& s : ch.sends) {
            int busIdx = s.first; float gain = s.second;
            if (busIdx < 1 || busIdx > 8) continue;
            auto busNode = busNodes[busIdx - 1]; if (busNode == nullptr) continue;
            auto itExisting = existing.find(busIdx);
            juce::AudioProcessorGraph::Node::Ptr gainNode;
            if (itExisting != existing.end() && itExisting->second != nullptr) {
                gainNode = itExisting->second;
            } else {
                gainNode = graph->addNode(std::make_unique<GainProcessor>());
                graph->addConnection({ { ch.instNode->nodeID, 0 }, { gainNode->nodeID, 0 } });
                graph->addConnection({ { ch.instNode->nodeID, 1 }, { gainNode->nodeID, 1 } });
                graph->addConnection({ { gainNode->nodeID, 0 }, { busNode->nodeID, 0 } });
                graph->addConnection({ { gainNode->nodeID, 1 }, { busNode->nodeID, 1 } });
            }
            if (auto* gp = dynamic_cast<GainProcessor*>(gainNode ? gainNode->getProcessor() : nullptr)) gp->setGain(juce::jlimit(0.0f, 1.0f, gain * ch.volume));
            ch.sendGainNodes.push_back(gainNode);
            ch.sendGainBusIdx.push_back(busIdx);
        }
        reprepare();
    }
    
    void reprepare() {
        if (!graph) return;
        if (auto* dev = deviceManager.getCurrentAudioDevice()) {
            double sr = dev->getCurrentSampleRate();
            int bs = dev->getCurrentBufferSizeSamples();
            graph->prepareToPlay(sr, bs);
        }
    }
public:
    juce::AudioDeviceManager& getDeviceManager() { return deviceManager; }
    juce::AudioPluginInstance* getBusInstance(int index) const {
        if (index < 1 || index > 8) return nullptr;
        auto node = busNodes[index - 1];
        if (node == nullptr) return nullptr;
        return dynamic_cast<juce::AudioPluginInstance*>(node->getProcessor());
    }
private:
    struct OverlayLayer : public juce::Component {
        InstrumentHost* host;
        OverlayLayer(InstrumentHost* h) : host(h) { setInterceptsMouseClicks(true, true); }
        void mouseDown(const juce::MouseEvent& e) override {
            bool inside = false;
            for (auto* c : getChildren()) {
                if (c->isVisible() && c->getBounds().contains(e.getPosition())) { inside = true; break; }
            }
            if (!inside && host) host->hideAllEditors();
        }
    };
    // MIDI tapper (top-level in InstrumentHost)
    struct MidiTap : public juce::MidiInputCallback {
        InstrumentHost* host { nullptr };
        explicit MidiTap(InstrumentHost* h) : host(h) {}
        void handleIncomingMidiMessage(juce::MidiInput*, const juce::MidiMessage& m) override {
            if (host && host->onMidiIn) host->onMidiIn(m);
        }
    };
    std::unique_ptr<MidiTap> tap;
    void ensureTap() { if (!tap) tap = std::make_unique<MidiTap>(this); }
public:
    std::function<void(const juce::MidiMessage&)> onMidiIn;
    void setMidiInListener(std::function<void(const juce::MidiMessage&)> cb) {
        onMidiIn = std::move(cb);
        ensureTap();
        if (currentMidi.identifier.isNotEmpty()) {
            deviceManager.addMidiInputDeviceCallback(currentMidi.identifier, tap.get());
        }
    }
    std::unique_ptr<OverlayLayer> overlayLayer;
    void ensureOverlayLayer() {
        if (!overlayLayer) overlayLayer.reset(new OverlayLayer(this));
    }
    void hideAllEditors() {
        if (overlayLayer) overlayLayer->setVisible(false);
        if (instrumentEditorComponent) instrumentEditorComponent->setVisible(false);
        for (auto& c : busEditorComponents) if (c) c->setVisible(false);
    }
    std::map<juce::String, ChannelSlot> channels;
};
