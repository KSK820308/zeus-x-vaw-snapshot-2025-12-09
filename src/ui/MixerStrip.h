#pragma once
#include <JuceHeader.h>
#include <map>
#include "ZXTheme.h"
#include "ZXLookAndFeel.h"
#include "BusSendPanel.h"
#include "PluginManager.h"
#include "PluginBrowserPanel.h"
#include "../audio/BusRegistry.h"
#include "../audio/InstrumentHost.h"
#include "../audio/MappingSamplerProcessor.h"
namespace SoundLibrary { bool deleteVoiceFile(const juce::File& file); }
class MixerStrip : public juce::Component {
public:
    MixerStrip(const juce::String& top, const juce::String& bottom, juce::Colour accent, bool isMaster=false, bool showPanel=false, bool isBusChannel=false, int busIdx=-1)
        : topText(top), bottomText(bottom), accentColour(accent), master(isMaster), showControls(showPanel), isBus(isBusChannel), busIndex(busIdx) {
        addAndMakeVisible(pan);
        addAndMakeVisible(fader);
        addAndMakeVisible(mute);
        addAndMakeVisible(solo);
        addAndMakeVisible(panLabel);
        mute.setButtonText("M");
        solo.setButtonText("S");
        mute.setToggleState(false, juce::NotificationType::dontSendNotification);
        solo.setToggleState(false, juce::NotificationType::dontSendNotification);
        mute.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
        mute.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        solo.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
        solo.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        setSize(90, 300);
        addMouseListener(this, true);
        pan.setRange(-64, 64, 1);
        fader.setRange(0.0, 127.0, 1.0);
        fader.setValue(100.0, juce::NotificationType::dontSendNotification);
        pan.setColour(juce::Slider::thumbColourId, accent);
        fader.setColour(juce::Slider::thumbColourId, accent);
        pan.setLookAndFeel(&laf);
        fader.setLookAndFeel(&laf);
        fader.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        pan.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        pan.setWantsKeyboardFocus(true);
        fader.setWantsKeyboardFocus(true);
        panLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        panLabel.setJustificationType(juce::Justification::centredLeft);
        if (isBus) { fader.setVisible(false); }
        if (!isBus)
            pan.onValueChange = [this]{ auto& st = registry()[topText]; st.panVal = (int) pan.getValue(); InstrumentHost::instance().setChannelPan(topText, (float) (pan.getValue()/64.0)); };
        if (auto it = registry().find(topText); it != registry().end()) {
            const State& st = it->second;
            pan.setValue(st.panVal, juce::NotificationType::dontSendNotification);
            fader.setValue(st.faderVal, juce::NotificationType::dontSendNotification);
            mute.setToggleState(st.mute, juce::NotificationType::dontSendNotification);
            solo.setToggleState(st.solo, juce::NotificationType::dontSendNotification);
        } else {
            State st; st.panVal = (int) pan.getValue(); st.faderVal = (int) fader.getValue(); st.mute = mute.getToggleState(); st.solo = solo.getToggleState();
            registry()[topText] = st;
        }
        if (isBus) { pan.setVisible(false); panLabel.setVisible(false); }
        if (showControls) {
            if (isBus) {
                addAndMakeVisible(pluginAddBtn);  pluginAddBtn.setButtonText("ADD PLUGIN");
                addAndMakeVisible(viewPluginBtn); viewPluginBtn.setButtonText("VIEW PLUGIN"); viewPluginBtn.setEnabled(false);
                addAndMakeVisible(pluginSaveBtn); pluginSaveBtn.setButtonText("SAVE PRESET");
                addAndMakeVisible(pluginLoadBtn); pluginLoadBtn.setButtonText("LOAD PRESET");
                addAndMakeVisible(pluginResetBtn); pluginResetBtn.setButtonText("RESET PLUGIN");
                addAndMakeVisible(pluginRemoveBtn);pluginRemoveBtn.setButtonText("REMOVE PLUGIN");

                if (busIndex > 0)
                    viewPluginBtn.setEnabled(InstrumentHost::instance().getBusInstance(busIndex) != nullptr);

                pluginAddBtn.onClick = [this]{
                    PluginManager::instance().init();
                    juce::DialogWindow::LaunchOptions o;
                    auto* browser = new PluginBrowserPanel();
                    browser->setOnChoose([this](const juce::PluginDescription& d){
                        busPluginWindow = nullptr;
                        juce::String err;
                        double sr = InstrumentHost::instance().getDeviceSampleRate(); int bs = InstrumentHost::instance().getDeviceBlockSize();
                        auto inst = PluginManager::instance().create(d, sr, bs, err);
                        if (inst) {
                            lastDesc = d;
                            juce::MemoryBlock st; inst->getStateInformation(st);
                            if (busIndex > 0) BusRegistry::instance().setBusPlugin(busIndex, d, st);
                            BusRegistry::instance().onChanged();
                            if (auto* proc = InstrumentHost::instance().getBusInstance(busIndex)) {
                                InstrumentHost::instance().openBusEditor(busIndex);
                                viewPluginBtn.setEnabled(true);
                            }
                        } else {
                            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Plugin Error", err.isNotEmpty() ? err : "Unable to create plugin instance.");
                        }
                    });
                    o.content.setOwned(browser);
                    o.dialogTitle = "Add Plugin";
                    o.escapeKeyTriggersCloseButton = true;
                    o.useNativeTitleBar = true;
                    o.resizable = true;
                    o.componentToCentreAround = this;
                    o.launchAsync();
                };
                viewPluginBtn.onClick = [this]{ auto* proc = InstrumentHost::instance().getBusInstance(busIndex); if (proc) { InstrumentHost::instance().toggleBusEditor(busIndex); } };
                pluginSaveBtn.onClick = [this]{ auto* proc = InstrumentHost::instance().getBusInstance(busIndex); if (proc) { auto dir = getPresetDir(); dir.createDirectory(); juce::File initial = dir.getChildFile(lastDesc ? lastDesc->name : "Preset").withFileExtension(".zxpreset"); presetChooser.reset(new juce::FileChooser("Save Preset", initial, "*.zxpreset")); presetChooser->launchAsync(juce::FileBrowserComponent::saveMode, [this, proc](const juce::FileChooser& fc){ juce::File f = fc.getResult(); if (f != juce::File()) { juce::MemoryBlock mb; proc->getStateInformation(mb); auto* o = new juce::DynamicObject(); o->setProperty("name", lastDesc ? lastDesc->name : juce::String()); o->setProperty("format", lastDesc ? lastDesc->pluginFormatName : juce::String()); o->setProperty("id", lastDesc ? lastDesc->fileOrIdentifier : juce::String()); o->setProperty("state", juce::Base64::toBase64(mb.getData(), mb.getSize())); juce::var root(o); f.replaceWithText(juce::JSON::toString(root)); } presetChooser = nullptr; }); } };
                pluginLoadBtn.onClick = [this]{ auto dir = getPresetDir(); dir.createDirectory(); presetChooser.reset(new juce::FileChooser("Load Preset", dir, "*.zxpreset")); presetChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [this](const juce::FileChooser& fc){ juce::File f = fc.getResult(); if (f != juce::File()) { auto v = juce::JSON::parse(f.loadFileAsString()); if (auto* o = v.getDynamicObject()) { juce::PluginDescription d; d.name = o->getProperty("name").toString(); d.pluginFormatName = o->getProperty("format").toString(); d.fileOrIdentifier = o->getProperty("id").toString(); juce::String stateStr = o->getProperty("state").toString(); juce::MemoryBlock mb; juce::MemoryOutputStream os(mb, false); juce::Base64::convertFromBase64(os, stateStr); if (busIndex > 0) BusRegistry::instance().setBusPlugin(busIndex, d, mb); lastDesc = d; BusRegistry::instance().onChanged(); viewPluginBtn.setEnabled(true); if (auto* proc = InstrumentHost::instance().getBusInstance(busIndex)) { InstrumentHost::instance().openBusEditor(busIndex); } } } presetChooser = nullptr; }); };
                pluginResetBtn.onClick = [this]{ if (lastDesc.has_value()) { busPluginWindow = nullptr; juce::String err; double sr = InstrumentHost::instance().getDeviceSampleRate(); int bs = InstrumentHost::instance().getDeviceBlockSize(); auto inst = PluginManager::instance().create(*lastDesc, sr, bs, err); if (inst) { juce::MemoryBlock st; inst->getStateInformation(st); if (busIndex > 0) BusRegistry::instance().setBusPlugin(busIndex, *lastDesc, st); BusRegistry::instance().onChanged(); viewPluginBtn.setEnabled(true); } } };
                pluginRemoveBtn.onClick = [this]{ busPluginWindow = nullptr; viewPluginBtn.setEnabled(false); if (busIndex > 0) BusRegistry::instance().removeBusPlugin(busIndex); BusRegistry::instance().onChanged(); };
            } else {
                addAndMakeVisible(busBtn); busBtn.setButtonText("BUS");
                addAndMakeVisible(monoBtn); monoBtn.setButtonText("MONO");
                addAndMakeVisible(velBtn); velBtn.setButtonText("VEL ON");
                if (topText.startsWithIgnoreCase("RIGHT")) {
                    addAndMakeVisible(tercaBtn); tercaBtn.setButtonText("TERCA");
                    addAndMakeVisible(scaleBtn); scaleBtn.setButtonText("SCALE");
                }
                actionAddVst = [this]{
                    PluginManager::instance().init();
                    juce::DialogWindow::LaunchOptions o;
                    auto* browser = new PluginBrowserPanel();
                    o.content.setOwned(browser);
                    o.dialogTitle = "Add Instrument";
                    o.escapeKeyTriggersCloseButton = true;
                    o.useNativeTitleBar = true;
                    o.resizable = true;
                    o.componentToCentreAround = this;
                    auto* dlg = o.launchAsync();
                    browser->setOnChoose([this, dlg](const juce::PluginDescription& d){
                        juce::MessageManager::callAsync([dlg]{ if (dlg) dlg->closeButtonPressed(); });
                        juce::String err; double sr = InstrumentHost::instance().getDeviceSampleRate(); int bs = InstrumentHost::instance().getDeviceBlockSize();
                        auto inst = PluginManager::instance().create(d, sr, bs, err);
                        if (inst) {
                            InstrumentHost::instance().setInstrument(std::move(inst));
                            InstrumentHost::instance().openEditor();
                        } else {
                            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Instrument Load", err.isNotEmpty() ? err : "Unable to create instrument.");
                        }
                    });
                };
                actionViewVst = []{ InstrumentHost::instance().toggleEditor(); };
                addAndMakeVisible(audioBtn); audioBtn.setButtonText("AUDIO");
                addAndMakeVisible(transBtn); transBtn.setButtonText("TRAN");
                addAndMakeVisible(octavBtn); octavBtn.setButtonText("OCT");
                addAndMakeVisible(msbBtn); msbBtn.setButtonText("MSB");
                addAndMakeVisible(lsbBtn); lsbBtn.setButtonText("LSB");
                addAndMakeVisible(pcBtn); pcBtn.setButtonText("PC");
                
                for (auto* b : { &busBtn, &monoBtn, &velBtn, &tercaBtn, &scaleBtn, &audioBtn, &transBtn, &octavBtn, &msbBtn, &lsbBtn, &pcBtn }) {
                    b->setClickingTogglesState(true);
                    b->setColour(juce::TextButton::textColourOnId, juce::Colours::white);
                    b->setColour(juce::TextButton::textColourOffId, juce::Colours::white);
                    b->setColour(juce::TextButton::buttonOnColourId, accentColour);
                    b->setColour(juce::TextButton::buttonColourId, juce::Colour::fromRGB(60,60,60));
                }
                for (auto* b : { &audioBtn, &transBtn, &octavBtn, &msbBtn, &lsbBtn, &pcBtn }) b->setClickingTogglesState(false);
                busBtn.onClick = [this]{ auto& st = registry()[topText]; st.bus = busBtn.getToggleState(); };
                monoBtn.onClick = [this]{ auto& st = registry()[topText]; st.mono = monoBtn.getToggleState(); };
                velBtn.onClick = [this]{ auto& st = registry()[topText]; st.vel = velBtn.getToggleState(); };
                tercaBtn.onClick = [this]{ auto& st = registry()[topText]; st.terca = tercaBtn.getToggleState(); };
                scaleBtn.onClick = [this]{ auto& st = registry()[topText]; st.scale = scaleBtn.getToggleState(); };
                transMin = -12; transMax = 12;
                octavMin = -5;  octavMax = 5;
                msbMin = 0; msbMax = 127;
                lsbMin = 0; lsbMax = 127;
                pcMin  = 1; pcMax  = 127;
                if (auto it2 = registry().find(topText); it2 != registry().end()) {
                    const State& st2 = it2->second;
                    busBtn.setToggleState(st2.bus, juce::NotificationType::dontSendNotification);
                    monoBtn.setToggleState(st2.mono, juce::NotificationType::dontSendNotification);
                    velBtn.setToggleState(st2.vel, juce::NotificationType::dontSendNotification);
                    tercaBtn.setToggleState(st2.terca, juce::NotificationType::dontSendNotification);
                    scaleBtn.setToggleState(st2.scale, juce::NotificationType::dontSendNotification);
                    msbBtn.setButtonText("MSB " + juce::String(st2.msb));
                    lsbBtn.setButtonText("LSB " + juce::String(st2.lsb));
                    pcBtn.setButtonText("PC "  + juce::String(st2.pc));
                    transBtn.setButtonText(st2.trans == 0 ? juce::String("TRAN") : juce::String("T ") + juce::String(st2.trans));
                    octavBtn.setButtonText(st2.octav == 0 ? juce::String("OCT") : juce::String("O ") + juce::String(st2.octav));
                } else {
                    auto& st2 = registry()[topText];
                    st2.bus = busBtn.getToggleState(); st2.mono = monoBtn.getToggleState(); st2.vel = velBtn.getToggleState(); st2.terca = tercaBtn.getToggleState(); st2.scale = scaleBtn.getToggleState();
                    st2.trans = 0; st2.octav = 0; st2.msb = 0; st2.lsb = 0; st2.pc = 0;
                    transBtn.setButtonText("TRAN");
                    octavBtn.setButtonText("OCT");
                    refreshCodeDisplay();
                }
                auto openNumericPopup = [this](juce::TextButton& anchor, const juce::String& title, int mn, int mx, int initial, std::function<void(int)> onCommit){
                    struct NumericIndicatorPanel : public juce::Component, private juce::Timer {
                        juce::Label value;
                        int minV, maxV; int v;
                        std::function<void(int)> commit;
                        bool blinkOn { true };
                        juce::CallOutBox* box { nullptr };
                        ~NumericIndicatorPanel() { if (commit) commit(juce::jlimit(minV, maxV, v)); if (box) box->dismiss(); }
                        NumericIndicatorPanel(int mn, int mx, int iv, std::function<void(int)> c) : minV(mn), maxV(mx), v(iv), commit(std::move(c)) {
                            setSize(160, 60);
                            setWantsKeyboardFocus(true);
                            value.setText(juce::String(v), juce::NotificationType::dontSendNotification);
                            value.setEditable(true, true, false);
                            value.setWantsKeyboardFocus(true);
                            value.setJustificationType(juce::Justification::centred);
                            value.setColour(juce::Label::textColourId, (minV == 0 && maxV == 127) ? MixerStrip::valueColourFor127(v) : juce::Colours::white);
                            value.onEditorShow = [this]{ if (auto* ed = value.getCurrentTextEditor()) { ed->setText(juce::String(v), juce::NotificationType::dontSendNotification); ed->setSelectAllWhenFocused(true); ed->selectAll(); ed->grabKeyboardFocus(); ed->setInputRestrictions(3, "0123456789"); ed->onReturnKey = [this]{ int n = juce::jlimit(minV, maxV, value.getText().getIntValue()); if (commit) commit(n); if (box) box->dismiss(); }; ed->onEscapeKey = [this]{ int n = juce::jlimit(minV, maxV, value.getText().getIntValue()); if (commit) commit(n); if (box) box->dismiss(); }; } };
                            value.onTextChange = [this]{
                                int n = value.getText().getIntValue();
                                v = juce::jlimit(minV, maxV, n);
                                value.setText(juce::String(v), juce::NotificationType::dontSendNotification);
                                value.setColour(juce::Label::textColourId, (minV == 0 && maxV == 127) ? MixerStrip::valueColourFor127(v) : juce::Colours::white);
                            };
                            addAndMakeVisible(value);
                            startTimer(380);
                            grabKeyboardFocus();
                            value.showEditor();
                        }
                        void resized() override { value.setBounds(getLocalBounds().reduced(12)); }
                        bool keyPressed(const juce::KeyPress& k) override {
                            if (k == juce::KeyPress::upKey)  { v = juce::jlimit(minV, maxV, v+1); value.setText(juce::String(v), juce::NotificationType::dontSendNotification); value.setColour(juce::Label::textColourId, (minV == 0 && maxV == 127) ? MixerStrip::valueColourFor127(v) : juce::Colours::white); return true; }
                            if (k == juce::KeyPress::downKey){ v = juce::jlimit(minV, maxV, v-1); value.setText(juce::String(v), juce::NotificationType::dontSendNotification); value.setColour(juce::Label::textColourId, (minV == 0 && maxV == 127) ? MixerStrip::valueColourFor127(v) : juce::Colours::white); return true; }
                            return false;
                        }
                        void focusLost(FocusChangeType) override {
                            int n = juce::jlimit(minV, maxV, value.getText().getIntValue());
                            if (commit) commit(n);
                            if (box) box->dismiss();
                        }
                        void timerCallback() override {
                            blinkOn = !blinkOn;
                            value.setAlpha(blinkOn ? 1.0f : 0.5f);
                        }
                    };
                    auto* p = new NumericIndicatorPanel(mn, mx, initial, onCommit);
                    std::unique_ptr<juce::Component> panel(p);
                    auto& box = juce::CallOutBox::launchAsynchronously(std::move(panel), anchor.getScreenBounds(), nullptr);
                    p->box = &box;
                };
                auto openInlineEditor = [this](juce::TextButton& anchor, int mn, int mx, int initial, std::function<void(int)> onCommit){
                    auto* ed = new juce::TextEditor();
                    addAndMakeVisible(ed);
                    ed->setBounds(anchor.getBounds());
                    ed->setText(juce::String(initial), juce::NotificationType::dontSendNotification);
                    ed->setJustification(juce::Justification::centred);
                    ed->setInputRestrictions(3, "0123456789");
                    ed->grabKeyboardFocus();
                    ed->setSelectAllWhenFocused(true);
                    ed->selectAll();
                    ed->onReturnKey = [this, ed, mn, mx, onCommit]{ int n = juce::jlimit(mn, mx, ed->getText().getIntValue()); if (onCommit) onCommit(n); if (auto* p = ed->getParentComponent()) p->removeChildComponent(ed); delete ed; };
                    ed->onEscapeKey = [this, ed, mn, mx, onCommit]{ int n = juce::jlimit(mn, mx, ed->getText().getIntValue()); if (onCommit) onCommit(n); if (auto* p = ed->getParentComponent()) p->removeChildComponent(ed); delete ed; };
                    ed->onFocusLost = [this, ed, mn, mx, onCommit]{ int n = juce::jlimit(mn, mx, ed->getText().getIntValue()); if (onCommit) onCommit(n); if (auto* p = ed->getParentComponent()) p->removeChildComponent(ed); delete ed; };
                };
                auto openInlineEditorSigned = [this](juce::TextButton& anchor, int mn, int mx, int initial, std::function<void(int)> onCommit){
                    auto* ed = new juce::TextEditor();
                    addAndMakeVisible(ed);
                    ed->setBounds(anchor.getBounds());
                    ed->setText(juce::String(initial), juce::NotificationType::dontSendNotification);
                    ed->setJustification(juce::Justification::centred);
                    ed->setInputRestrictions(3, "-0123456789");
                    ed->grabKeyboardFocus();
                    ed->setSelectAllWhenFocused(true);
                    ed->selectAll();
                    ed->onReturnKey = [this, ed, mn, mx, onCommit]{ int n = juce::jlimit(mn, mx, ed->getText().getIntValue()); if (onCommit) onCommit(n); if (auto* p = ed->getParentComponent()) p->removeChildComponent(ed); delete ed; };
                    ed->onEscapeKey = [this, ed, mn, mx, onCommit]{ int n = juce::jlimit(mn, mx, ed->getText().getIntValue()); if (onCommit) onCommit(n); if (auto* p = ed->getParentComponent()) p->removeChildComponent(ed); delete ed; };
                    ed->onFocusLost = [this, ed, mn, mx, onCommit]{ int n = juce::jlimit(mn, mx, ed->getText().getIntValue()); if (onCommit) onCommit(n); if (auto* p = ed->getParentComponent()) p->removeChildComponent(ed); delete ed; };
                };
                auto setTransText = [this]{ auto it = registry().find(topText); int v = (it != registry().end()) ? it->second.trans : 0; transBtn.setButtonText(v == 0 ? juce::String("TRAN") : juce::String("T ") + juce::String(v)); };
                auto setOctText  = [this]{ auto it = registry().find(topText); int v = (it != registry().end()) ? it->second.octav : 0; octavBtn.setButtonText(v == 0 ? juce::String("OCT") : juce::String("O ") + juce::String(v)); };
                transBtn.onClick = [this, openInlineEditorSigned, setTransText]{ auto& st = registry()[topText]; openInlineEditorSigned(transBtn, transMin, transMax, st.trans, [this, setTransText](int n){ auto& st2 = registry()[topText]; st2.trans = n; InstrumentHost::instance().setChannelTranspose(topText, n); InstrumentHost::instance().setMidiTranspose(mapChannelToMidi(topText), n); setTransText(); }); };
                octavBtn.onClick = [this, openInlineEditorSigned, setOctText]{ auto& st = registry()[topText]; openInlineEditorSigned(octavBtn, octavMin, octavMax, st.octav, [this, setOctText](int n){ auto& st2 = registry()[topText]; st2.octav = n; InstrumentHost::instance().setChannelOctave(topText, n); InstrumentHost::instance().setMidiOctave(mapChannelToMidi(topText), n); setOctText(); }); };
                msbBtn.onClick   = [this, openInlineEditor]{ auto& st = registry()[topText]; openInlineEditor(msbBtn, msbMin, msbMax, st.msb, [this](int n){ auto& st2 = registry()[topText]; st2.msb = n; InstrumentHost::instance().sendBankSelect(st2.msb, st2.lsb, mapChannelToMidi(topText)); refreshCodeDisplay(); if (onCodeChanged) onCodeChanged(topText, st2.msb, st2.lsb, st2.pc); }); };
                lsbBtn.onClick   = [this, openInlineEditor]{ auto& st = registry()[topText]; openInlineEditor(lsbBtn, lsbMin, lsbMax, st.lsb, [this](int n){ auto& st2 = registry()[topText]; st2.lsb = n; InstrumentHost::instance().sendBankSelect(st2.msb, st2.lsb, mapChannelToMidi(topText)); refreshCodeDisplay(); if (onCodeChanged) onCodeChanged(topText, st2.msb, st2.lsb, st2.pc); }); };
                pcBtn.onClick    = [this, openInlineEditor]{ auto& st = registry()[topText]; openInlineEditor(pcBtn, pcMin, pcMax, st.pc, [this](int n){ auto& st2 = registry()[topText]; st2.pc = n; InstrumentHost::instance().sendProgramChange(juce::jlimit(0,127,st2.pc - 1), mapChannelToMidi(topText)); refreshCodeDisplay(); if (onCodeChanged) onCodeChanged(topText, st2.msb, st2.lsb, st2.pc); }); };
                audioBtn.onClick = [this, openNumericPopup]{
                    struct AudioMenuPanel : public juce::Component {
                        juce::TextButton btnVstAdd, btnVstView;
                        std::function<void()> onVstAdd, onVstView;
                        juce::LookAndFeel* lf { nullptr };
                        juce::Colour accent { juce::Colours::darkgrey };
                        bool allowVst { false };
                        AudioMenuPanel(juce::LookAndFeel* l, juce::Colour a, bool allow) : lf(l), accent(a), allowVst(allow) {
                            setSize(220, 120);
                            if (allowVst) { addAndMakeVisible(btnVstAdd); btnVstAdd.setButtonText("VST INSTRUMENT"); addAndMakeVisible(btnVstView); btnVstView.setButtonText("VIEW VSTi"); }
                            if (lf) { if (allowVst) { btnVstAdd.setLookAndFeel(lf); btnVstView.setLookAndFeel(lf); } }
                            auto styleBtn = [&](juce::TextButton& b){ b.setColour(juce::TextButton::textColourOnId, juce::Colours::white); b.setColour(juce::TextButton::textColourOffId, juce::Colours::white); b.setColour(juce::TextButton::buttonOnColourId, accent); b.setColour(juce::TextButton::buttonColourId, juce::Colour::fromRGB(60,60,60)); };
                            if (allowVst) { styleBtn(btnVstAdd); styleBtn(btnVstView); }
                        }
                        void resized() override {
                            auto a = getLocalBounds().reduced(8);
                            int h = 26; int g = 6; int y = a.getY();
                            if (allowVst) { btnVstAdd.setBounds(a.getX(), y, a.getWidth(), h); y += h+g; btnVstView.setBounds(a.getX(), y, a.getWidth(), h); y += h+g; }
                        }
                    };
                    bool allowVst = topText.equalsIgnoreCase("RIGHT 1");
                    auto* p = new AudioMenuPanel(&laf, accentColour, allowVst);
                    std::unique_ptr<juce::Component> panel(p);
                    auto& box = juce::CallOutBox::launchAsynchronously(std::move(panel), audioBtn.getScreenBounds(), nullptr);
                    audioBox = &box;
                    p->onVstAdd = [this]{ if (actionAddVst) actionAddVst(); if (audioBox != nullptr) audioBox->dismiss(); };
                    p->onVstView = [this]{ if (actionViewVst) actionViewVst(); if (audioBox != nullptr) audioBox->dismiss(); };
                    p->btnVstAdd.onClick = [p]{ if (p->onVstAdd) p->onVstAdd(); };
                    p->btnVstView.onClick = [p]{ if (p->onVstView) p->onVstView(); };
                };
                
                busBtn.onClick = [this]{
                    if (busBox != nullptr) busBox->dismiss();
                    auto* p = new BusSendPanel();
                    p->setInitialSends(InstrumentHost::instance().getChannelSends(topText));
                    p->onSendsChanged = [this](const std::vector<std::pair<int,float>>& s){ InstrumentHost::instance().setChannelSends(topText, s); };
                    std::unique_ptr<juce::Component> panel (p);
                    auto& box = juce::CallOutBox::launchAsynchronously(std::move(panel), busBtn.getScreenBounds(), nullptr);
                    busBox = &box; // SafePointer
                    busActive = true;
                    busBtn.setToggleState(true, juce::NotificationType::dontSendNotification);
                    busBtn.setColour(juce::TextButton::buttonOnColourId, ZXTheme::soloActive());
                };
            }
        }
        fader.onValueChange = [this]{
            if (isBus) return;
            int v = (int) fader.getValue();
            auto& st = registry()[topText]; st.faderVal = v;
            InstrumentHost::instance().setChannelVolume(topText, (float) (fader.getValue()/127.0));
        };
        mute.setClickingTogglesState(true);
        solo.setClickingTogglesState(true);
        mute.onClick = [this]{ updateButtonStyles(); auto& st = registry()[topText]; st.mute = mute.getToggleState(); InstrumentHost::instance().setChannelMute(topText, mute.getToggleState()); };
        solo.onClick = [this]{ updateButtonStyles(); auto& st = registry()[topText]; st.solo = solo.getToggleState(); };
        mute.setLookAndFeel(&laf);
        solo.setLookAndFeel(&laf);
        updateButtonStyles();
    }
    void paint(juce::Graphics& g) override {
        auto b = getLocalBounds().toFloat();
        g.setColour(ZXTheme::panelBG());
        g.fillRoundedRectangle(b, 8.f);
        g.setColour(accentColour);
        g.drawRoundedRectangle(b, 8.f, 1.f);
        if (selected) {
            juce::Path glow;
            glow.addRoundedRectangle(b.expanded(2.0f), 10.0f);
            g.setColour(juce::Colours::white.withAlpha(0.90f));
            g.strokePath(glow, juce::PathStrokeType(3.5f));
            g.setColour(juce::Colours::white.withAlpha(0.35f));
            g.strokePath(glow, juce::PathStrokeType(8.0f));
        }
    }
    void resized() override {
        auto r = getLocalBounds().reduced(6);
        auto panArea = r.removeFromTop(82);
        int knobSize = 64;
        int knobX = panArea.getX() + (panArea.getWidth() - knobSize)/2;
        int knobY = panArea.getY() + (panArea.getHeight() - knobSize)/2 - 10;
        if (!isBus) pan.setBounds(knobX, knobY, knobSize, knobSize); else pan.setBounds(0,0,0,0);
        auto labelRow = panArea.removeFromBottom(18);
        int labelW = 36; // Pan
        int totalW = labelW;
        int cx = knobX + knobSize/2;
        int startX = cx - totalW/2;
        if (!isBus) panLabel.setBounds(startX, labelRow.getY(), labelW, labelRow.getHeight()); else panLabel.setBounds(0,0,0,0);
        auto btns = r.removeFromBottom(28);
        int buttonW = 32;
        int gap = 2;
        int faderW = buttonW * 2;
        auto content = r.reduced(0, 4);
        int desiredH = juce::roundToInt(content.getHeight() * 0.80f);
        int offsetY = content.getBottom() - desiredH - 4;
        auto band = juce::Rectangle<int>(content.getX(), offsetY, content.getWidth(), desiredH);
        auto upper = juce::Rectangle<int>(content.getX(), panArea.getBottom()+2, content.getWidth(), band.getY() - (panArea.getBottom()+2));
        int upperY = upper.getY(); int upperH = 24; int upperGap = 6;
        audioBtn.setBounds(upper.getX(), upperY, upper.getWidth(), upperH); upperY += upperH + upperGap;
        
        juce::Rectangle<int> rightBlock;
        if (!isBus) {
            auto faderArea = band.removeFromLeft(faderW);
            fader.setBounds(faderArea);
            fader.toFront(true);
            rightBlock = juce::Rectangle<int>(faderArea.getRight() + gap, band.getY(), band.getRight() - (faderArea.getRight() + gap), band.getHeight());
        } else {
            fader.setBounds(0,0,0,0);
            fader.setVisible(false);
            rightBlock = band;
        }
        if (showControls) {
            int y = rightBlock.getY();
            int bigH = 24; int gapY = 6;
            if (isBus) {
                pluginAddBtn.setBounds(rightBlock.getX(), y, rightBlock.getWidth(), bigH); y += bigH + gapY;
                viewPluginBtn.setBounds(rightBlock.getX(), y, rightBlock.getWidth(), bigH); y += bigH + gapY;
                pluginSaveBtn.setBounds(rightBlock.getX(), y, rightBlock.getWidth(), bigH); y += bigH + gapY;
                pluginLoadBtn.setBounds(rightBlock.getX(), y, rightBlock.getWidth(), bigH); y += bigH + gapY;
                pluginResetBtn.setBounds(rightBlock.getX(), y, rightBlock.getWidth(), bigH); y += bigH + gapY;
                pluginRemoveBtn.setBounds(rightBlock.getX(), y, rightBlock.getWidth(), bigH); y += bigH + gapY;
            } else {
                busBtn.setBounds(rightBlock.getX(), y, rightBlock.getWidth(), bigH); y += bigH + gapY;
                monoBtn.setBounds(rightBlock.getX(), y, rightBlock.getWidth(), bigH); y += bigH + gapY;
                velBtn.setBounds(rightBlock.getX(), y, rightBlock.getWidth(), bigH); y += bigH + gapY;
                if (topText.startsWithIgnoreCase("RIGHT")) {
                    tercaBtn.setBounds(rightBlock.getX(), y, rightBlock.getWidth(), bigH); y += bigH + gapY;
                    scaleBtn.setBounds(rightBlock.getX(), y, rightBlock.getWidth(), bigH); y += bigH + gapY;
                }
                if (topText.equalsIgnoreCase("RIGHT 1")) {
                    // removed explicit VST buttons from layout; logic preserved in AUDIO popup
                }
                int rowH = 24; 
                auto placeBtn = [&](juce::TextButton& b){ b.setBounds(rightBlock.getX(), y, rightBlock.getWidth(), rowH); y += rowH + 6; };
                placeBtn(transBtn);
                placeBtn(octavBtn);
                placeBtn(msbBtn);
                placeBtn(lsbBtn);
                placeBtn(pcBtn);
            }
        }
        mute.setBounds(btns.removeFromLeft(buttonW));
        solo.setBounds(btns.removeFromLeft(buttonW));
    }
    void visibilityChanged() override {
        if (isBus && busIndex > 0)
            viewPluginBtn.setEnabled(InstrumentHost::instance().getBusInstance(busIndex) != nullptr);
        if (!isBus && topText.equalsIgnoreCase("RIGHT 1"))
            instViewBtn.setEnabled(InstrumentHost::instance().getInstrument() != nullptr);
    }
    
    void mouseDown(const juce::MouseEvent& e) override {
        juce::Component::mouseDown(e);
        if (onSelected) onSelected(topText);
    }
    void setSelected(bool s) { selected = s; repaint(); }
    void refreshCodeDisplay() {
        auto it = registry().find(topText);
        int msbV = 0, lsbV = 0, pcV = 0;
        if (it != registry().end()) { msbV = it->second.msb; lsbV = it->second.lsb; pcV = it->second.pc; }
        msbBtn.setButtonText("MSB " + juce::String(msbV));
        lsbBtn.setButtonText("LSB " + juce::String(lsbV));
        pcBtn.setButtonText("PC "  + juce::String(pcV));
    }
    static int getMSB(const juce::String& id) { auto it = registry().find(id); if (it == registry().end()) return 0; return it->second.msb; }
    static int getLSB(const juce::String& id) { auto it = registry().find(id); if (it == registry().end()) return 0; return it->second.lsb; }
    static void setPC(const juce::String& id, int pc) { auto& st = registry()[id]; st.pc = juce::jlimit(0,127,pc); }
    static void setMSB(const juce::String& id, int msb) { auto& st = registry()[id]; st.msb = juce::jlimit(0,127,msb); }
    static void setLSB(const juce::String& id, int lsb) { auto& st = registry()[id]; st.lsb = juce::jlimit(0,127,lsb); }
    static int mapChannelToMidi(const juce::String& name) {
        if (name.startsWithIgnoreCase("RIGHT ")) { int n = name.substring(6).getIntValue(); return juce::jlimit(1, 5, n); }
        if (name.equalsIgnoreCase("LOWER")) return 6;
        if (name.equalsIgnoreCase("MULTIPAD")) return 7;
        if (name.equalsIgnoreCase("PERCUSSION")) return 9;
        if (name.equalsIgnoreCase("DRUMS")) return 10;
        if (name.equalsIgnoreCase("BASS")) return 11;
        if (name.equalsIgnoreCase("CHORD 1")) return 12;
        if (name.equalsIgnoreCase("CHORD 2")) return 13;
        if (name.equalsIgnoreCase("PAD")) return 14;
        if (name.equalsIgnoreCase("MELODY 1")) return 15;
        if (name.equalsIgnoreCase("MELODY 2")) return 16;
        return 1;
    }

private:
    juce::String topText;
    juce::String bottomText;
    juce::Colour accentColour;
    bool master;
    ZXLookAndFeel laf;
    juce::Slider pan { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow };
    juce::Slider fader { juce::Slider::LinearVertical, juce::Slider::TextBoxBelow };
    juce::TextButton mute, solo;
    juce::Label panLabel { "Pan", "Pan" };
    bool showControls { false };
    bool isBus { false };
    int busIndex { -1 };
    juce::TextButton busBtn, monoBtn, velBtn, tercaBtn, scaleBtn;
    juce::TextButton pluginAddBtn, viewPluginBtn, pluginSaveBtn, pluginLoadBtn, pluginResetBtn, pluginRemoveBtn;
    juce::TextButton instAddBtn, instViewBtn;
    juce::TextButton audioBtn;
    bool busActive { false };
    juce::Component::SafePointer<juce::CallOutBox> busBox;
    juce::Component::SafePointer<juce::CallOutBox> audioBox;
    juce::TextButton transBtn, octavBtn, msbBtn, lsbBtn, pcBtn;
    int transMin{}, transMax{}, octavMin{}, octavMax{}, msbMin{}, msbMax{}, lsbMin{}, lsbMax{}, pcMin{}, pcMax{};
    std::optional<juce::PluginDescription> lastDesc;
    std::function<void()> actionAddVst;
    std::function<void()> actionViewVst;
public:
    std::function<void(const juce::String&)> onSelected;
    std::function<void(const juce::String&, int, int, int)> onCodeChanged;
    const juce::String& getId() const { return topText; }
private:
    struct PluginEditorWindow : public juce::DocumentWindow {
        std::function<void()> onClosed;
        PluginEditorWindow(const juce::String& name) : juce::DocumentWindow(name, juce::Colours::black, juce::DocumentWindow::allButtons) {
            setUsingNativeTitleBar(true);
            setResizable(false, false);
        }
        void closeButtonPressed() override { if (onClosed) onClosed(); }
    };
    std::unique_ptr<PluginEditorWindow> busPluginWindow;
    std::unique_ptr<juce::FileChooser> presetChooser;
    
    juce::File getPresetDir() const {
        auto appData = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
        auto dir = appData.getChildFile("ZEUS-X VAW").getChildFile("Presets");
        return dir;
    }
    
    static juce::Colour valueColourFor127(int v) {
        v = juce::jlimit(0, 127, v);
        if (v >= 110) return juce::Colour(0xFFE23B3B);
        if (v >= 85)  return juce::Colour(0xFFE5C51B);
        return juce::Colour(0xFF1FA545);
    }
    void updateButtonStyles() {
        auto inactive = juce::Colour::fromRGB(60,60,60);
        mute.setColour(juce::TextButton::buttonOnColourId, ZXTheme::muteActive());
        mute.setColour(juce::TextButton::buttonColourId, inactive);
        solo.setColour(juce::TextButton::buttonOnColourId, ZXTheme::soloActive());
        solo.setColour(juce::TextButton::buttonColourId, inactive);
        repaint();
    }
    struct State { int panVal{}; int faderVal{}; bool mute{}; bool solo{}; bool bus{}; bool mono{}; bool vel{}; bool terca{}; bool scale{}; int trans{}; int octav{}; int msb{}; int lsb{}; int pc{}; };
    static std::map<juce::String, State>& registry() { static std::map<juce::String, State> r; return r; }
    bool selected { false };
    
};
