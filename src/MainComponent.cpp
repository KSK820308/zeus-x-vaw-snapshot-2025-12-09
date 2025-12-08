#include "MainComponent.h"
#include "ui/MixerPage.h"
#include "ui/ZXTheme.h"
#include "ui/TopBar.h"
#include "ui/StudioPage.h"
#include "audio/InstrumentHost.h"
#include "audio/AudioEngine.h"
#include "core/SoundLibrary.h"
#include "core/Assets.h"

MainComponent::MainComponent() {
    top.reset(new TopBar()); addAndMakeVisible(top.get());
    InstrumentHost::instance().init();
    InstrumentHost::instance().setOverlayParent(this);
    BusRegistry::instance().onChanged = []{ InstrumentHost::instance().onBusChanged(); };
    top->handlers.onTempoChanged = [](int){ };
    top->handlers.onMidiDeviceChanged = [](const juce::String& n){ InstrumentHost::instance().setMidiInput(n); };
    top->handlers.onModeChanged = [this](int m){
        if (currentPage) { this->removeChildComponent(currentPage.get()); currentPage = nullptr; }
        if (this->studioPage) { this->removeChildComponent(this->studioPage.get()); this->studioPage = nullptr; }
        if (this->settingsPage) { this->removeChildComponent(this->settingsPage.get()); this->settingsPage = nullptr; }
        if (m == 0) {
            page1.setVisible(true); page2.setVisible(true);
            buildPage(currentIndex);
        } else if (m == 1) {
            page1.setVisible(false); page2.setVisible(false);
            this->studioPage.reset(new StudioPage());
            this->addAndMakeVisible(this->studioPage.get());
            resized();
        } else {
            page1.setVisible(false); page2.setVisible(false);
            auto& dm = InstrumentHost::instance().getDeviceManager();
            auto* adc = new juce::AudioDeviceSelectorComponent(dm, 0, 0, 0, 2, true, false, true, false);
            this->settingsPage.reset(adc);
            this->addAndMakeVisible(this->settingsPage.get());
            resized();
        }
    };
    top->setMidiDevices(InstrumentHost::instance().getMidiDevices());
    {
        auto devs = InstrumentHost::instance().getMidiDevices();
        if (!devs.isEmpty()) { InstrumentHost::instance().setMidiInput(devs[0]); }
    }
    SoundLibrary::ensureDirectories();
    Assets::ensureAssetDirs();
    this->juce::Component::addAndMakeVisible(page1);
    this->juce::Component::addAndMakeVisible(page2);
    page1.onClick = [this]{ buildPage(1); };
    page2.onClick = [this]{ buildPage(2); };
    buildPage(1);
    setSize(1440, 810);
}

void MainComponent::paint(juce::Graphics& g) {
    g.fillAll(ZXTheme::panelBG());
}

void MainComponent::resized() {
    auto r = getLocalBounds().reduced(8);
    auto header = r.removeFromTop(40);
    if (top) top->setBounds(header);
    auto nav = r.removeFromTop(24);
    page1.setBounds(nav.removeFromLeft(84));
    page2.setBounds(nav.removeFromLeft(84));
    if (currentPage) currentPage->setBounds(r);
    if (studioPage) studioPage->setBounds(r);
    if (settingsPage) settingsPage->setBounds(r);
    InstrumentHost::instance().updateOverlayBounds();
}

void MainComponent::buildPage(int index) {
    currentIndex = index;
    if (currentPage) removeChildComponent(currentPage.get());
    juce::StringArray names;
    if (index == 1) {
        names.addArray({"PERCUSSION","DRUMS","BASS","CHORD 1","CHORD 2","PAD","MELODY 1","MELODY 2","RIGHT 1","RIGHT 2","RIGHT 3","RIGHT 4","RIGHT 5","LOWER","MULTIPAD"});
    } else {
        names.addArray({"BUS 1","BUS 2","BUS 3","BUS 4","BUS 5","BUS 6","BUS 7","BUS 8","MASTER"});
    }
    currentPage.reset(new MixerPage(names, index==2));
    addAndMakeVisible(currentPage.get());
    resized();
}
