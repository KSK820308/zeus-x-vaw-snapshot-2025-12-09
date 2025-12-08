#pragma once
#include <JuceHeader.h>
class MixerPage;
class TopBar;
class InstrumentHost;
class AudioEngine;

class MainComponent : public juce::Component {
public:
    MainComponent();
    void paint(juce::Graphics& g) override;
    void resized() override;
private:
    juce::TextButton page1 { "PAGE 1" };
    juce::TextButton page2 { "PAGE 2" };
    juce::TextButton fonts { "FONTS" };
    std::unique_ptr<MixerPage> currentPage;
    std::unique_ptr<juce::Component> studioPage;
    std::unique_ptr<juce::Component> settingsPage;
    int currentIndex { 1 };
    void buildPage(int index);
    std::unique_ptr<TopBar> top;
    std::unique_ptr<AudioEngine> testEngine;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
