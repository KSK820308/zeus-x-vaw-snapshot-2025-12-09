#pragma once
#include <JuceHeader.h>
#include "PluginManager.h"

class PluginBrowserPanel : public juce::Component, public juce::ListBoxModel, public juce::ChangeListener {
public:
    PluginBrowserPanel() {
        setSize(520, 400);
        list.setModel(this);
        list.setRowHeight(48);
        addAndMakeVisible(list);
        refresh();
        PluginManager::instance().init();
        PluginManager::instance().addChangeListener(this);

        hint.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.8f));
        hint.setJustificationType(juce::Justification::centred);
        hint.setText("Няма намерени плъгини. Използвай Scan Plugins от менюто.", juce::NotificationType::dontSendNotification);
        addAndMakeVisible(hint);
        btnScan.setButtonText("Scan Plugins");
        btnScan.onClick = []{ PluginManager::instance().scan(); };
        addAndMakeVisible(btnScan);
    }

    ~PluginBrowserPanel() override { PluginManager::instance().removeChangeListener(this); }

    void setOnChoose(std::function<void(const juce::PluginDescription&)> cb) { onChoose = std::move(cb); }

    void resized() override {
        auto r = getLocalBounds().reduced(8);
        list.setBounds(r);
        bool empty = items.isEmpty();
        hint.setVisible(empty);
        btnScan.setVisible(empty);
        if (empty) {
            auto mid = r.removeFromTop(60);
            hint.setBounds(r.getX(), r.getCentreY() - 40, r.getWidth(), 24);
            btnScan.setBounds(r.getCentreX() - 60, r.getCentreY() - 8, 120, 28);
        }
    }

    int getNumRows() override { return items.size(); }
    void paintListBoxItem(int row, juce::Graphics& g, int width, int height, bool selected) override {
        g.fillAll(juce::Colours::black);
        if (selected) g.fillAll(juce::Colours::darkgrey.withAlpha(0.3f));
        auto& d = items.getReference(row);
        auto name = d.name + "  [" + d.pluginFormatName + "]";
        auto sub = d.manufacturerName + "  •  " + d.fileOrIdentifier;
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(18.0f));
        g.drawText(name, 6, 3, width-12, height/2, juce::Justification::centredLeft);
        g.setColour(juce::Colours::lightgrey.withAlpha(0.65f));
        g.setFont(juce::Font(13.0f));
        g.drawText(sub, 6, height/2 - 1, width-12, height/2, juce::Justification::centredLeft);
    }
    void listBoxItemClicked(int row, const juce::MouseEvent&) override {
        if (row >= 0 && row < items.size() && onChoose) onChoose(items[row]);
    }
    void listBoxItemDoubleClicked(int row, const juce::MouseEvent& e) override { listBoxItemClicked(row, e); }

    void refresh() {
        items = PluginManager::instance().getDescriptions();
        list.updateContent();
        resized();
    }

    void changeListenerCallback(juce::ChangeBroadcaster*) override { refresh(); }
private:
    juce::ListBox list { "plugins", this };
    juce::Array<juce::PluginDescription> items;
    std::function<void(const juce::PluginDescription&)> onChoose;
    juce::Label hint;
    juce::TextButton btnScan;
};
