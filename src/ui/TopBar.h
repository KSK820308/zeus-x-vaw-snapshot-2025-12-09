#pragma once
#include <JuceHeader.h>

class TopBar : public juce::Component {
public:
    struct Handlers {
        std::function<void(int)> onTempoChanged;
        std::function<void(const juce::String&)> onMidiDeviceChanged;
        std::function<void(int)> onModeChanged; // 0=Live,1=Studio,2=Settings
    } handlers;

    TopBar() {
        addAndMakeVisible(lblTempo); lblTempo.setText("Tempo", juce::NotificationType::dontSendNotification);
        addAndMakeVisible(btnMinus); btnMinus.setButtonText("-");
        addAndMakeVisible(tempoBox); tempoBox.setText("120", juce::NotificationType::dontSendNotification);
        tempoBox.setEditable(true, true, false);
        addAndMakeVisible(btnPlus); btnPlus.setButtonText("+");
        addAndMakeVisible(lblMidi); lblMidi.setText("MIDI", juce::NotificationType::dontSendNotification);
        addAndMakeVisible(midiSource);
        addAndMakeVisible(modeLive); addAndMakeVisible(modeStudio); addAndMakeVisible(modeSettings);
        modeLive.setButtonText("Live"); modeStudio.setButtonText("Studio"); modeSettings.setButtonText("Settings");
        modeLive.setClickingTogglesState(true); modeStudio.setClickingTogglesState(true); modeSettings.setClickingTogglesState(true);
        modeLive.setToggleState(true, juce::NotificationType::dontSendNotification);

        btnMinus.onClick = [this]{ int t = tempoBox.getText().getIntValue(); t = juce::jlimit(20, 300, t - 1); tempoBox.setText(juce::String(t), juce::NotificationType::dontSendNotification); if (handlers.onTempoChanged) handlers.onTempoChanged(t); };
        btnPlus.onClick  = [this]{ int t = tempoBox.getText().getIntValue(); t = juce::jlimit(20, 300, t + 1); tempoBox.setText(juce::String(t), juce::NotificationType::dontSendNotification); if (handlers.onTempoChanged) handlers.onTempoChanged(t); };
        tempoBox.onTextChange = [this]{ int t = tempoBox.getText().getIntValue(); t = juce::jlimit(20, 300, t); if (handlers.onTempoChanged) handlers.onTempoChanged(t); };
        midiSource.onChange = [this]{ if (handlers.onMidiDeviceChanged) handlers.onMidiDeviceChanged(midiSource.getText()); };
        auto updateMode = [this](int m){
            modeLive.setToggleState(m==0, juce::NotificationType::dontSendNotification);
            modeStudio.setToggleState(m==1, juce::NotificationType::dontSendNotification);
            modeSettings.setToggleState(m==2, juce::NotificationType::dontSendNotification);
            if (handlers.onModeChanged) handlers.onModeChanged(m);
        };
        modeLive.onClick    = [updateMode]{ updateMode(0); };
        modeStudio.onClick  = [updateMode]{ updateMode(1); };
        modeSettings.onClick= [updateMode]{ updateMode(2); };
    }

    void setMidiDevices(const juce::StringArray& names) {
        midiSource.clear();
        for (int i = 0; i < names.size(); ++i) midiSource.addItem(names[i], i+1);
        if (!names.isEmpty()) {
            midiSource.setSelectedId(1, juce::NotificationType::dontSendNotification);
            if (handlers.onMidiDeviceChanged) handlers.onMidiDeviceChanged(names[0]);
        }
    }

    void resized() override {
        auto bounds = getLocalBounds().reduced(8);

        auto r = bounds; // леви контроли
        lblTempo.setBounds(r.removeFromLeft(60));
        btnMinus.setBounds(r.removeFromLeft(32));
        tempoBox.setBounds(r.removeFromLeft(54));
        btnPlus.setBounds(r.removeFromLeft(32));
        r.removeFromLeft(16);
        lblMidi.setBounds(r.removeFromLeft(40));
        midiSource.setBounds(r.removeFromLeft(160));

        // Центриране на трите основни бутона по хоризонтала, без промяна на размера
        const int wLive = 64, wStudio = 64, wSettings = 84;
        const int gap = 12;
        const int groupW = wLive + wStudio + wSettings + 2 * gap;
        int startX = bounds.getX() + (bounds.getWidth() - groupW) / 2;
        int y = bounds.getY();
        modeLive.setBounds(startX, y, wLive, 24);
        modeStudio.setBounds(startX + wLive + gap, y, wStudio, 24);
        modeSettings.setBounds(startX + wLive + gap + wStudio + gap, y, wSettings, 24);
    }

private:
    juce::Label lblTempo, lblMidi;
    juce::TextButton btnMinus, btnPlus;
    juce::Label tempoBox;
    juce::ComboBox midiSource;
    juce::TextButton modeLive, modeStudio, modeSettings;
};
