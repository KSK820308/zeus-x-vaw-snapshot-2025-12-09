#pragma once
#include <JuceHeader.h>
#include "ZXLookAndFeel.h"

class BusSendPanel : public juce::Component {
public:
    BusSendPanel() {
        setSize(300, 220);
        const int rows = 4;
        for (int i = 0; i < rows; ++i) {
            auto* cb = new juce::ComboBox();
            for (int j = 1; j <= 8; ++j) cb->addItem("Bus " + juce::String(j), j);
            cb->setSelectedId(i + 1);
            cb->onChange = [this, i]{ if (combos[i] != nullptr) combos[i]->setText(combos[i]->getText(), juce::NotificationType::dontSendNotification); };
            combos.add(cb); addAndMakeVisible(cb);

            auto* sendLbl = new juce::Label();
            sendLbl->setText("SEND", juce::NotificationType::dontSendNotification);
            sendLbl->setColour(juce::Label::textColourId, juce::Colours::white);
            sendLbl->setJustificationType(juce::Justification::centred);
            sendLabels.add(sendLbl); addAndMakeVisible(sendLbl);

            auto* s = new juce::Slider(juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox);
            s->setRange(0, 127, 1); s->setValue(0); s->setLookAndFeel(&laf); s->setWantsKeyboardFocus(true);
            sliders.add(s); addAndMakeVisible(s);

            auto* v = new juce::Label();
            v->setText("0", juce::NotificationType::dontSendNotification);
            v->setColour(juce::Label::textColourId, juce::Colours::white);
            v->setJustificationType(juce::Justification::centred);
            v->setEditable(true, true, false);
            values.add(v); addAndMakeVisible(v);

            s->onValueChange = [this, i]{ values[i]->setText(juce::String((int)sliders[i]->getValue()), juce::NotificationType::dontSendNotification); notify(); };
            v->onTextChange = [this, i]{ int n = values[i]->getText().getIntValue(); n = juce::jlimit(0,127,n); sliders[i]->setValue(n); notify(); };
        }
        resized();
    }

    void paint(juce::Graphics& g) override {
        auto b = getLocalBounds().toFloat();
        g.setColour(juce::Colour::fromRGB(24,24,24));
        g.fillRoundedRectangle(b, 8.0f);
        g.setColour(juce::Colour(0xFFd4af37));
        g.drawRoundedRectangle(b, 8.0f, 1.0f);
    }

    void resized() override {
        auto r = getLocalBounds().reduced(10);
        int gap = 8;
        int count = combos.size();
        int available = r.getHeight() - gap * (count - 1);
        int rowH = juce::jmax(26, available / count);
        for (int i = 0; i < count; ++i) {
            auto row = r.removeFromTop(rowH);
            if (i != count - 1) r.removeFromTop(gap);

            auto cbW = 90; auto valW = 48; auto knobW = rowH; // квадрат за кноба
            auto cbx = row.removeFromLeft(cbW);
            if (combos[i] != nullptr) combos[i]->setBounds(cbx);

            auto valBox = row.removeFromRight(valW);
            if (values[i] != nullptr) values[i]->setBounds(valBox);
            auto knobBox = row.removeFromRight(knobW);
            if (sliders[i] != nullptr) sliders[i]->setBounds(knobBox);

            auto sendW = 46;
            auto sendBox = row.removeFromRight(sendW);
            if (sendLabels[i] != nullptr) sendLabels[i]->setBounds(sendBox);
        }
    }

private:
    ZXLookAndFeel laf;
    juce::OwnedArray<juce::ComboBox> combos;
    juce::OwnedArray<juce::Slider> sliders;
    juce::OwnedArray<juce::Label> values;
    juce::OwnedArray<juce::Label> sendLabels;
    void notify() {
        if (onSendsChanged) {
            std::vector<std::pair<int,float>> v;
            for (int i = 0; i < combos.size(); ++i) {
                int busIdx = combos[i]->getSelectedId();
                float amt = (float) (sliders[i]->getValue() / 127.0);
                v.emplace_back(busIdx, amt);
            }
            onSendsChanged(v);
        }
    }
public:
    std::function<void(const std::vector<std::pair<int,float>>&)> onSendsChanged;
    void setInitialSends(const std::vector<std::pair<int,float>>& v) {
        int rows = combos.size();
        for (int i = 0; i < rows; ++i) {
            int busIdx = (i < (int)v.size() ? v[i].first : (i + 1));
            float amt = (i < (int)v.size() ? v[i].second : 0.0f);
            if (combos[i] != nullptr) combos[i]->setSelectedId(juce::jlimit(1, 8, busIdx));
            int v127 = juce::roundToInt(juce::jlimit(0.0f, 1.0f, amt) * 127.0f);
            if (sliders[i] != nullptr) sliders[i]->setValue(v127, juce::NotificationType::dontSendNotification);
            if (values[i] != nullptr) values[i]->setText(juce::String(v127), juce::NotificationType::dontSendNotification);
        }
    }
};
