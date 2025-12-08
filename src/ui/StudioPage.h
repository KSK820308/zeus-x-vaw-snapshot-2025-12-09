#pragma once
#include <JuceHeader.h>
#include "ZXTheme.h"
#include "../audio/MappingSamplerProcessor.h"

class StudioPage : public juce::Component {
public:
    StudioPage() {
        setSize(800, 600);
        addAndMakeVisible(title);
        title.setText("Studio", juce::NotificationType::dontSendNotification);
        title.setColour(juce::Label::textColourId, juce::Colours::white);
        title.setJustificationType(juce::Justification::centredLeft);

        {
            std::array<juce::TextButton*, 6> btns { &autosampling, &mapping, &soundDesign, &styles, &phrases, &plugins };
            for (auto* b : btns) {
                addAndMakeVisible(*b);
                b->setClickingTogglesState(true);
                b->setColour(juce::TextButton::textColourOnId, juce::Colours::white);
                b->setColour(juce::TextButton::textColourOffId, juce::Colours::white);
                b->setColour(juce::TextButton::buttonOnColourId, ZXTheme::mixAccent());
                b->setColour(juce::TextButton::buttonColourId, juce::Colour::fromRGB(60,60,60));
            }
        }
        autosampling.setButtonText("Autosampling");
        mapping.setButtonText("Mapping");
        soundDesign.setButtonText("Sound Design");
        styles.setButtonText("Styles");
        phrases.setButtonText("Audio Phrases");
        plugins.setButtonText("Plugins");

        autosampling.onClick = [this]{ setTool(0); };
        mapping.onClick      = [this]{ setTool(1); };
        soundDesign.onClick  = [this]{ setTool(2); };
        styles.onClick       = [this]{ setTool(3); };
        phrases.onClick      = [this]{ setTool(4); };
        plugins.onClick      = [this]{ setTool(5); };

        setTool(0);

        addAndMakeVisible(contentHolder);
    }

    void resized() override {
        auto r = getLocalBounds().reduced(8);
        auto header = r.removeFromTop(40);
        auto fullHeader = header;
        auto titleArea = header.removeFromLeft(120);
        title.setBounds(titleArea);

        // Центриране на 6-те бутона по целия ред (съвпада с центъра на горните 3)
        auto group = fullHeader;
        int w = 84;
        int h = 24;
        int gap = 12;
        int totalW = 6 * w + 5 * gap;
        int startX = group.getX() + (group.getWidth() - totalW) / 2;
        int y = group.getY() + (group.getHeight() - h) / 2;
        autosampling.setBounds(startX, y, w, h);
        mapping.setBounds(startX + (w + gap), y, w, h);
        soundDesign.setBounds(startX + 2 * (w + gap), y, w, h);
        styles.setBounds(startX + 3 * (w + gap), y, w, h);
        phrases.setBounds(startX + 4 * (w + gap), y, w, h);
        plugins.setBounds(startX + 5 * (w + gap), y, w, h);

        contentHolder.setBounds(r.reduced(12));
        if (contentComp)
            contentComp->setBounds(contentHolder.getLocalBounds());
    }

private:
    juce::Label title;
    juce::TextButton autosampling, mapping, soundDesign, styles, phrases, plugins;
    std::unique_ptr<juce::Component> contentComp;
    juce::Component contentHolder;

    

    void setTool(int idx) {
        
        autosampling.setToggleState(idx==0, juce::NotificationType::dontSendNotification);
        mapping.setToggleState(idx==1, juce::NotificationType::dontSendNotification);
        soundDesign.setToggleState(idx==2, juce::NotificationType::dontSendNotification);
        styles.setToggleState(idx==3, juce::NotificationType::dontSendNotification);
        phrases.setToggleState(idx==4, juce::NotificationType::dontSendNotification);
        plugins.setToggleState(idx==5, juce::NotificationType::dontSendNotification);
        contentComp = nullptr;
        if (idx == 1) {
            auto proc = std::make_unique<MappingSamplerProcessor>();
            InstrumentHost::instance().setInstrumentProcessor(std::move(proc));
            auto* p = InstrumentHost::instance().getCurrentProcessor();
            juce::AudioProcessorEditor* ed = p ? p->createEditor() : nullptr;
            if (ed) {
                struct EditorHolder : public juce::Component {
                    juce::AudioProcessorEditor* child { nullptr };
                    EditorHolder(juce::AudioProcessorEditor* c) : child(c) { addAndMakeVisible(child); }
                    void resized() override { if (child) child->setBounds(getLocalBounds()); }
                };
                contentComp.reset(new EditorHolder(ed));
            } else {
                contentComp.reset(new juce::Label("", "Mapping page"));
            }
            
        } else if (idx == 2) {
            contentComp.reset(new juce::Label("", "Sound Design page"));
            #if 0
            struct SoundDesignPanel : public juce::Component {
                juce::TextButton load { "LOAD SOUND" }, save { "SAVE SOUND" };
                juce::Label eqTitle { "EQ", "EQ" }, revTitle { "REVERB", "REVERB" }, delTitle { "DELAY", "DELAY" }, adsrTitle{ "ADSR", "ADSR" }, octTitle{ "OCTAVE", "OCTAVE" };
                juce::Label lblHigh { "High", "High" }, lblMid { "Mid", "Mid" }, lblLow { "Low", "Low" };
                juce::Label lblAttack { "Attack", "Attack" }, lblRelease { "Release", "Release" };
                juce::Slider eqHigh, eqMid, eqLow, revWet, delMix, att, rel, oct;
                juce::ComboBox revType, delType;
                ZXLookAndFeel laf;
                bool loaded { false };
                juce::File lastZxi;
                SoundDesignPanel() {
                    for (auto* s : { &eqHigh,&eqMid,&eqLow,&revWet,&delMix,&att,&rel,&oct }) { addAndMakeVisible(*s); s->setLookAndFeel(&laf); s->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag); s->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 64, 20); s->setColour(juce::Slider::textBoxTextColourId, juce::Colours::white); s->setEnabled(false); }
                    for (auto* l : { &eqTitle,&revTitle,&delTitle,&adsrTitle,&octTitle,&lblHigh,&lblMid,&lblLow,&lblAttack,&lblRelease }) { addAndMakeVisible(*l); l->setColour(juce::Label::textColourId, juce::Colours::white); l->setJustificationType(juce::Justification::centred); }
                    addAndMakeVisible(load); addAndMakeVisible(save); load.setColour(juce::TextButton::buttonColourId, juce::Colour::fromRGB(60,60,60)); save.setColour(juce::TextButton::buttonColourId, juce::Colour::fromRGB(60,60,60));
                    revType.addItem("Room",1); revType.addItem("Hall",2); revType.addItem("Plate",3); addAndMakeVisible(revType);
                    delType.addItem("Stereo",1); delType.addItem("Ping-Pong",2); delType.addItem("Tape",3); addAndMakeVisible(delType);
                    eqHigh.setRange(-24,24,0.1); eqMid.setRange(-24,24,0.1); eqLow.setRange(-24,24,0.1);
                    revWet.setRange(0,1,0.01); delMix.setRange(0,1,0.01);
                    att.setRange(0,2,0.01); rel.setRange(0,3,0.01);
                    oct.setRange(-5,5,1);
                    auto applyEq = [this]{ if (!loaded) return; InstrumentHost::instance().setEQ((float)eqLow.getValue(),100.0f,(float)eqMid.getValue(),1000.0f,0.8f,(float)eqHigh.getValue(),8000.0f); if (auto* p = dynamic_cast<MappingSamplerProcessor*>(InstrumentHost::instance().getCurrentProcessor())) p->setFXEQ((float)eqLow.getValue(),(float)eqMid.getValue(),(float)eqHigh.getValue()); };
                    eqLow.onValueChange = applyEq; eqMid.onValueChange = applyEq; eqHigh.onValueChange = applyEq;
                    auto applyRev = [this]{ if (!loaded) return; int t = revType.getSelectedId(); float room=0.3f, damp=0.3f, width=1.0f; if (t==2){ room=0.6f; damp=0.4f; } else if (t==3){ room=0.4f; damp=0.2f; width=0.9f; } InstrumentHost::instance().setReverb(room,damp,width,(float)revWet.getValue(),1.0f-(float)revWet.getValue(),false); if (auto* p = dynamic_cast<MappingSamplerProcessor*>(InstrumentHost::instance().getCurrentProcessor())) p->setFXReverb(t,(float)revWet.getValue()); };
                    revWet.onValueChange = applyRev; revType.onChange = applyRev; revType.setSelectedId(1);
                    auto applyDel = [this]{ if (!loaded) return; int t = delType.getSelectedId(); float fb = t==2?0.4f:0.35f; float tMs = t==3?420.0f:350.0f; InstrumentHost::instance().setDelay(tMs,fb,(float)delMix.getValue()); if (auto* p = dynamic_cast<MappingSamplerProcessor*>(InstrumentHost::instance().getCurrentProcessor())) p->setFXDelay(t,(float)delMix.getValue()); };
                    delMix.onValueChange = applyDel; delType.onChange = applyDel; delType.setSelectedId(1);
                    auto applyADSR = [this]{ if (!loaded) return; if (auto* p = dynamic_cast<MappingSamplerProcessor*>(InstrumentHost::instance().getCurrentProcessor())) p->setADSR((float)att.getValue(),0.10f,0.80f,(float)rel.getValue()); };
                    att.onValueChange = applyADSR; rel.onValueChange = applyADSR;
                    auto applyOct = [this]{ if (!loaded) return; int o = (int) std::round(oct.getValue()); InstrumentHost::instance().setMidiOctave(1,o); if (auto* p = dynamic_cast<MappingSamplerProcessor*>(InstrumentHost::instance().getCurrentProcessor())) p->setOctave(o); };
                    oct.onValueChange = applyOct;
                    auto setEnabled = [this](bool en){ for (auto* s : { &eqHigh,&eqMid,&eqLow,&revWet,&delMix,&att,&rel,&oct }) s->setEnabled(en); revType.setEnabled(en); delType.setEnabled(en); };
                    setEnabled(false);
                    load.onClick = [this, setEnabled]{ juce::StringArray cats = SoundLibrary::voiceZXCategories(); juce::PopupMenu m; for (int i=0;i<cats.size();++i) m.addItem(i+1,cats[i]); m.showMenuAsync(juce::PopupMenu::Options(), [this,setEnabled,cats](int r){ if (r<=0) return; juce::String cat=cats[r-1]; CategoryPopup::open(this, cat, [this,setEnabled,cat](int pc, const juce::String&, const juce::String&){ auto f = SoundLibrary::voiceFileFor(cat, pc); if (f.existsAsFile()) { InstrumentHost::instance().setInstrumentProcessor(std::make_unique<MappingSamplerProcessor>()); if (auto* p = InstrumentHost::instance().getCurrentProcessor()) if (auto* mp = dynamic_cast<MappingSamplerProcessor*>(p)) { int msb{},lsb{},pcDummy{}; juce::String t; mp->importInstrument(f, msb, lsb, pcDummy, t); loaded = true; setEnabled(true); } } }); }); };
                    save.onClick = [this, setEnabled]{ juce::StringArray cats = SoundLibrary::voiceZXCategories(); juce::PopupMenu m; for (int i=0;i<cats.size();++i) m.addItem(i+1,cats[i]); m.showMenuAsync(juce::PopupMenu::Options(), [this,setEnabled,cats](int r){ if (r<=0) return; juce::String cat=cats[r-1]; CategoryPopup::open(this, cat, [this,setEnabled,cat](int pc, const juce::String&, const juce::String&){ struct NamePrompt : public juce::Component { juce::Label title{ "", "Name" }; juce::TextEditor ed; juce::TextButton ok{ "Save" }, cancel{ "Cancel" }; std::function<void(juce::String)> onOk; NamePrompt(){ addAndMakeVisible(title); addAndMakeVisible(ed); addAndMakeVisible(ok); addAndMakeVisible(cancel); title.setColour(juce::Label::textColourId, juce::Colours::white); ed.setText("MySound"); ok.onClick = [this]{ if (onOk) onOk(ed.getText()); if (auto* p = getParentComponent()) p->setVisible(false); }; cancel.onClick = [this]{ if (auto* p = getParentComponent()) p->setVisible(false); }; } void resized() override { auto r = getLocalBounds().reduced(6); title.setBounds(r.removeFromTop(24)); ed.setBounds(r.removeFromTop(26)); auto row = r.removeFromBottom(28); ok.setBounds(row.removeFromLeft(80)); cancel.setBounds(row.removeFromLeft(80)); } }; auto* prompt = new NamePrompt(); prompt->setSize(240, 120); std::unique_ptr<juce::Component> holder(prompt); auto& box = juce::CallOutBox::launchAsynchronously(std::move(holder), save.getScreenBounds(), nullptr); prompt->onOk = [this,cat,pc,&box](juce::String nm){ auto temp = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("zx_tmp_" + juce::String((int) juce::Time::getMillisecondCounter()) + ".zxi"); bool exported = false; if (auto* p = dynamic_cast<MappingSamplerProcessor*>(InstrumentHost::instance().getCurrentProcessor())) { exported = p->exportInstrument(temp, 0, 0, pc, nm); } if (exported) { bool exists = SoundLibrary::voiceFileFor(cat, pc).existsAsFile(); bool repl = true; if (exists) repl = juce::NativeMessageBox::showOkCancelBox(juce::MessageBoxIconType::WarningIcon, "Replace", "Slot occupied. Replace?", this, nullptr); if (repl) { SoundLibrary::assignVoicePosition(cat, pc, nm, temp, juce::File(), true); auto _ = SoundLibrary::listCategoryItems(cat); } temp.deleteFile(); } box.dismiss(); }; }); }); };

                    
                }
                void resized() override {
                    auto r = getLocalBounds().reduced(10);
                    auto top = r.removeFromTop(34);
                    load.setBounds(top.removeFromLeft(140)); save.setBounds(top.removeFromLeft(140));
                    int colW = r.getWidth()/2;
                    auto left = juce::Rectangle<int>(r.getX(), r.getY(), colW, r.getHeight()).reduced(10);
                    auto right= juce::Rectangle<int>(r.getX()+colW, r.getY(), r.getWidth()-colW, r.getHeight()).reduced(10);
                    eqTitle.setBounds(left.removeFromTop(24));
                    int h=76; int gap=10;
                    lblHigh.setBounds(left.removeFromTop(20)); eqHigh.setBounds(left.removeFromTop(h)); left.removeFromTop(gap);
                    lblMid.setBounds(left.removeFromTop(20)); eqMid.setBounds(left.removeFromTop(h)); left.removeFromTop(gap);
                    lblLow.setBounds(left.removeFromTop(20)); eqLow.setBounds(left.removeFromTop(h)); left.removeFromTop(gap);
                    adsrTitle.setBounds(left.removeFromTop(24));
                    lblAttack.setBounds(left.removeFromTop(20)); att.setBounds(left.removeFromTop(h)); left.removeFromTop(gap);
                    lblRelease.setBounds(left.removeFromTop(20)); rel.setBounds(left.removeFromTop(h)); left.removeFromTop(gap);
                    revTitle.setBounds(right.removeFromTop(24));
                    revWet.setBounds(right.removeFromTop(h)); revType.setBounds(right.removeFromTop(26)); right.removeFromTop(gap);
                    delTitle.setBounds(right.removeFromTop(24));
                    delMix.setBounds(right.removeFromTop(h)); delType.setBounds(right.removeFromTop(26)); right.removeFromTop(gap);
                    octTitle.setBounds(right.removeFromTop(24));
                    oct.setBounds(right.removeFromTop(h));
                }
            };
            #endif
        }
        contentHolder.removeAllChildren();
        if (!contentComp) {
            auto* l = new juce::Label(); l->setColour(juce::Label::textColourId, juce::Colours::white); l->setJustificationType(juce::Justification::centred);
            static const char* names[] = { "Autosampling", "Mapping", "Sound Design", "Styles", "Audio Phrases", "Plugins" };
            l->setText(juce::String(names[idx]) + " page", juce::NotificationType::dontSendNotification);
            contentComp.reset(l);
        }
        contentHolder.addAndMakeVisible(contentComp.get());
        contentComp->setBounds(contentHolder.getLocalBounds());
    }
};
