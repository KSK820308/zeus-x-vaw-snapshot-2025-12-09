#pragma once
#include <JuceHeader.h>
#include "../ui/PluginManager.h"

class BusRegistry {
public:
    static BusRegistry& instance() { static BusRegistry r; return r; }

    void setBusPlugin(int index, const juce::PluginDescription& desc, const juce::MemoryBlock& state) {
        if (index < 1 || index > 8) return;
        buses[index] = { desc, state, buses[index].inputGain }; onChanged();
    }
    void removeBusPlugin(int index) { buses.erase(index); onChanged(); }
    bool hasBus(int index) const { return buses.find(index) != buses.end(); }

    std::unique_ptr<juce::AudioPluginInstance> createBusInstance(int index, juce::String& errorOut) const {
        auto it = buses.find(index);
        if (it == buses.end()) return nullptr;
        auto inst = PluginManager::instance().create(it->second.desc, 44100.0, 512, errorOut);
        if (inst && it->second.state.getSize() > 0) inst->setStateInformation(it->second.state.getData(), (int) it->second.state.getSize());
        return inst;
    }

    void setBusPluginInput(int index, float gain01) { if (index >= 1 && index <= 8) { buses[index].inputGain = juce::jlimit(0.0f, 1.0f, gain01); onChanged(); } }
    float getBusPluginInput(int index) const { auto it = buses.find(index); return it==buses.end() ? 1.0f : it->second.inputGain; }

    juce::PluginDescription getBusDescription(int index) const {
        auto it = buses.find(index);
        return it == buses.end() ? juce::PluginDescription{} : it->second.desc;
    }

    const juce::MemoryBlock& getBusState(int index) const {
        static juce::MemoryBlock empty;
        auto it = buses.find(index);
        return it == buses.end() ? empty : it->second.state;
    }

    std::function<void()> onChanged;

private:
    struct BusEntry { juce::PluginDescription desc; juce::MemoryBlock state; float inputGain { 1.0f }; };
    std::map<int, BusEntry> buses;
};
