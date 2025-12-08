#pragma once
#include <JuceHeader.h>
#include "MixerStrip.h"
#include "ZXTagLabel.h"
#include "../core/SoundLibrary.h"
#include "CategoryPopup.h"
class MixerPage : public juce::Component {
public:
    MixerPage(const juce::StringArray& names, bool isBusPage);
    void paint(juce::Graphics& g) override;
    void resized() override;
private:
    juce::OwnedArray<MixerStrip> strips;
    juce::OwnedArray<ZXTagLabel> tags;
    juce::StringArray names;
    bool isBus { false };
    class SectionPanel : public juce::Component {
    public:
        SectionPanel(const juce::String& t) : title(t) {
            addAndMakeVisible(label);
            label.setText(t, juce::NotificationType::dontSendNotification);
            label.setJustificationType(juce::Justification::centred);
            label.setColour(juce::Label::textColourId, juce::Colours::white);
            label.setFont(juce::FontOptions(18.0f, juce::Font::bold));
        }
        void paint(juce::Graphics& g) override {
            auto b = getLocalBounds().toFloat();
            g.setColour(juce::Colour::fromRGB(60,60,60));
            g.fillRoundedRectangle(b, 8.f);
            g.setColour(ZXTheme::mixAccent());
            g.drawRoundedRectangle(b, 8.f, 1.f);
        }
        void resized() override {
            auto r = getLocalBounds();
            auto top = r.removeFromTop(32);
            label.setBounds(top.withTrimmedLeft(8).withTrimmedRight(8));
        }
    private:
        juce::String title;
        juce::Label label;
    };
    class InnerPanel : public juce::Component {
    public:
        InnerPanel(const juce::String& t, juce::Component* anchor = nullptr, std::function<void(int, const juce::String&, const juce::String&)> chooseFn = {}) : title(t), anchorParent(anchor), onChoose(chooseFn) {
            addAndMakeVisible(label);
            label.setText(t, juce::NotificationType::dontSendNotification);
            label.setJustificationType(juce::Justification::centred);
            label.setColour(juce::Label::textColourId, juce::Colours::white);
            label.setFont(juce::FontOptions(16.0f, juce::Font::bold));
        }
        void clearButtons() {
            buttons.clear(true);
            repaint();
        }
        void setSingleColumn(bool b) { singleColumn = b; }
        void addButton(const juce::String& text) {
            auto* b = buttons.add(new juce::TextButton(text));
            b->onClick = [this, text]{
                int limit = rightLimitProvider ? rightLimitProvider() : -1;
                auto* tlc = getTopLevelComponent();
                juce::Component* areaComp = anchorParent ? anchorParent : this;
                juce::Rectangle<int> panelRect = tlc && areaComp ? tlc->getLocalArea(areaComp, areaComp->getLocalBounds()) : juce::Rectangle<int>();
                CategoryPopup::open(this, text, onChoose, -1, limit, panelRect);
            };
            addAndMakeVisible(b);
        }
        void paint(juce::Graphics& g) override {
            auto b = getLocalBounds().toFloat();
            g.setColour(juce::Colour::fromRGB(50,50,50));
            g.fillRoundedRectangle(b, 8.f);
            g.setColour(ZXTheme::mixAccent());
            g.drawRoundedRectangle(b, 8.f, 1.f);
        }
        void resized() override {
            auto r = getLocalBounds();
            auto top = r.removeFromTop(28);
            label.setBounds(top.withTrimmedLeft(6).withTrimmedRight(6));
            if (buttons.size() > 0) {
                int padding = 6;
                int gap = 6;
                auto area = r.reduced(padding);
                int sizingRows = 8;
                int baseH = (area.getHeight() - gap * (sizingRows - 1)) / sizingRows;
                int rowH = juce::jmax(18, baseH);
                int startY = area.getY();
                if (singleColumn) {
                    int idx = 0;
                    int colW = area.getWidth();
                    for (int row = 0; idx < buttons.size(); ++row) {
                        auto full = juce::Rectangle<int>(area.getX(), startY + row * (rowH + gap), colW, rowH);
                        buttons[idx++]->setBounds(full);
                    }
                } else {
                    int colW = (area.getWidth() - gap) / 2;
                    int rows = sizingRows;
                    int idx = 0;
                    for (int row = 0; row < rows && idx < buttons.size(); ++row) {
                        auto left = juce::Rectangle<int>(area.getX(), startY + row * (rowH + gap), colW, rowH);
                        auto right = juce::Rectangle<int>(left.getRight() + gap, left.getY(), colW, rowH);
                        if (idx < buttons.size()) buttons[idx++]->setBounds(left);
                        if (idx < buttons.size()) buttons[idx++]->setBounds(right);
                    }
                }
            }
        }
    public:
        juce::String title;
        juce::Label label;
        juce::OwnedArray<juce::TextButton> buttons;
        juce::Component* anchorParent { nullptr };
        std::function<void(int, const juce::String&, const juce::String&)> onChoose;
        bool singleColumn { false };
        std::function<int()> rightLimitProvider;
        void setRightLimitProvider(std::function<int()> fn) { rightLimitProvider = std::move(fn); }
    };
    class CustomVoicePanel : public juce::Component {
    public:
        class SlotButton : public juce::TextButton {
        public:
            SlotButton(int idx) : index(idx) {
                setColour(juce::TextButton::textColourOnId, juce::Colours::white);
                setColour(juce::TextButton::textColourOffId, juce::Colours::white);
            }
            juce::String fallbackTitle() const {
                return juce::String("Slot ") + juce::String(index + 1);
            }
            void mouseDown(const juce::MouseEvent& e) override {
                if (e.mods.isRightButtonDown()) {
                    juce::PopupMenu m;
                    m.addItem(10, "Copy Slot");
                    m.addItem(11, "Paste Slot", SoundLibrary::hasSlotClipboard());
                    m.addItem(12, "Move Slot");
                    m.addSeparator();
                    m.addItem(1, "Rename Slot");
                    m.addItem(2, "Delete Slot");
                    m.showMenuAsync(juce::PopupMenu::Options(), [this](int r){
                        if (r == 10) { SoundLibrary::copySlotToClipboard(index); }
                        else if (r == 11) { bool occupied = !SoundLibrary::listCustomSlotItems(index).isEmpty(); bool doReplace = true; if (occupied) doReplace = juce::NativeMessageBox::showOkCancelBox(juce::MessageBoxIconType::QuestionIcon, "Replace Slot", "Target slot not empty. Replace?", this, nullptr); SoundLibrary::pasteSlotFromClipboard(index, doReplace); auto name = SoundLibrary::getCustomVoiceSlotName(index); setButtonText(name.isNotEmpty() ? name : fallbackTitle()); }
                        else if (r == 12) { struct IndexDialog : public juce::Component { juce::TextEditor idx; juce::TextButton ok{ "OK" }; int srcIndex{}; std::function<void(int)> onDone; IndexDialog(int s){ srcIndex=s; addAndMakeVisible(idx); addAndMakeVisible(ok); setSize(220, 70); idx.setText("", juce::NotificationType::dontSendNotification); idx.setJustification(juce::Justification::centredLeft); idx.setInputRestrictions(2, "0123456789"); ok.onClick = [this]{ if (onDone) onDone(idx.getText().getIntValue()); }; } void resized() override { auto r=getLocalBounds().reduced(8); idx.setBounds(r.removeFromTop(30)); ok.setBounds(r.removeFromTop(28)); } }; struct InlineIdxWindow : public juce::DialogWindow { InlineIdxWindow() : juce::DialogWindow("Move Slot (1-16)", juce::Colours::black, true) {} void closeButtonPressed() override { exitModalState(0); auto sp = juce::Component::SafePointer<InlineIdxWindow>(this); juce::MessageManager::callAsync([sp]{ if (sp.getComponent() != nullptr) delete sp.getComponent(); }); } }; auto* dlg = new IndexDialog(index); auto* win = new InlineIdxWindow(); win->setContentOwned(dlg, true); win->centreAroundComponent(this, 240, 100); win->setResizable(false, false); win->setUsingNativeTitleBar(false); win->setTitleBarButtonsRequired(0, 0); win->enterModalState(true, nullptr, true); dlg->onDone = [this, win, dlg](int dst){ int human = juce::jlimit(1,16,dst); int target = human - 1; bool occupied = !SoundLibrary::listCustomSlotItems(target).isEmpty(); bool doReplace = true; if (occupied) doReplace = juce::NativeMessageBox::showOkCancelBox(juce::MessageBoxIconType::QuestionIcon, "Replace Slot", "Target slot not empty. Replace?", this, nullptr); SoundLibrary::moveSlot(dlg->srcIndex, target, doReplace); win->exitModalState(0); auto sp = juce::Component::SafePointer<juce::DialogWindow>(win); juce::MessageManager::callAsync([sp]{ if (sp.getComponent() != nullptr) delete sp.getComponent(); }); }; }
                        else if (r == 1) {
                            struct RenameDialog : public juce::Component {
                                juce::TextEditor name;
                                juce::TextButton ok{ "OK" }, clear{ "Clear" };
                                int idx { -1 };
                                SlotButton* target { nullptr };
                                juce::CallOutBox* box { nullptr };
                                juce::DialogWindow* win { nullptr };
                                RenameDialog(const juce::String& current) {
                                    addAndMakeVisible(name);
                                    name.setText(current);
                                    addAndMakeVisible(ok);
                                    addAndMakeVisible(clear);
                                    setSize(260, 90);
                                    name.setSelectAllWhenFocused(true);
                                    name.setJustification(juce::Justification::centredLeft);
                                    name.setFocusContainer(true);
                                    name.setWantsKeyboardFocus(true);
                                    name.grabKeyboardFocus();
                                    name.setColour(juce::TextEditor::textColourId, juce::Colours::white);
                                    name.setColour(juce::TextEditor::backgroundColourId, juce::Colour::fromRGB(20,20,20));
                                    name.setColour(juce::TextEditor::outlineColourId, juce::Colours::grey);
                                }
                                void attach(SlotButton* t, int i, juce::DialogWindow* w) {
                                    target = t; idx = i; win = w; box = nullptr;
                                    auto doCommit = [this]{
                                        auto s = name.getText().trim();
                                        SoundLibrary::setCustomVoiceSlotName(idx, s);
                                        if (target) target->setButtonText(s.isNotEmpty() ? s : target->fallbackTitle());
                                        if (win) { win->exitModalState(0); auto sp = juce::Component::SafePointer<juce::DialogWindow>(win); juce::MessageManager::callAsync([sp]{ if (sp.getComponent() != nullptr) delete sp.getComponent(); }); }
                                    };
                                    ok.onClick = doCommit;
                                    name.onReturnKey = doCommit;
                                    clear.onClick = [this]{
                                        SoundLibrary::setCustomVoiceSlotName(idx, "");
                                        SoundLibrary::clearCustomVoiceSlot(idx);
                                        if (target) target->setButtonText(target->fallbackTitle());
                                        if (win) { win->exitModalState(0); auto sp = juce::Component::SafePointer<juce::DialogWindow>(win); juce::MessageManager::callAsync([sp]{ if (sp.getComponent() != nullptr) delete sp.getComponent(); }); }
                                    };
                                }
                                void resized() override {
                                    auto r = getLocalBounds().reduced(12);
                                    name.setBounds(r.removeFromTop(34));
                                    auto row = r.removeFromTop(34);
                                    ok.setBounds(row.removeFromLeft(80));
                                    clear.setBounds(row.removeFromLeft(80));
                                }
                            };
                            struct InlineNameWindow : public juce::DialogWindow {
                                InlineNameWindow() : juce::DialogWindow("Rename", juce::Colours::black, true) {}
                                void closeButtonPressed() override { exitModalState(0); auto sp = juce::Component::SafePointer<InlineNameWindow>(this); juce::MessageManager::callAsync([sp]{ if (sp.getComponent() != nullptr) delete sp.getComponent(); }); }
                            };
                            auto* dlg = new RenameDialog(getButtonText());
                            auto* win = new InlineNameWindow();
                            win->setContentOwned(dlg, true);
                            win->centreAroundComponent(this, 280, 100);
                            win->setResizable(false, false);
                            win->setUsingNativeTitleBar(false);
                            win->setTitleBarButtonsRequired(0, 0);
                            win->enterModalState(true, nullptr, true);
                            dlg->attach(this, index, win);
                        } else if (r == 2) {
                            bool confirm = juce::NativeMessageBox::showOkCancelBox(juce::MessageBoxIconType::QuestionIcon, "Delete Slot", "Delete all contents?", this, nullptr);
                            if (confirm) { SoundLibrary::deleteSlot(index); setButtonText(fallbackTitle()); }
                        }
                    });
                } else {
                    juce::TextButton::mouseDown(e);
                }
            }
            int index;
        };
        CustomVoicePanel(juce::Component* anchor = nullptr, std::function<void(int, const juce::String&, const juce::String&, int)> chooseFn = {}) : anchorParent(anchor), onChoose(chooseFn) {
            addAndMakeVisible(label);
            label.setText("CUSTOM VOICE", juce::NotificationType::dontSendNotification);
            label.setJustificationType(juce::Justification::centred);
            label.setColour(juce::Label::textColourId, juce::Colours::white);
            label.setFont(juce::FontOptions(16.0f, juce::Font::bold));
            
        }
        void paint(juce::Graphics& g) override {
            auto b = getLocalBounds().toFloat();
            g.setColour(juce::Colour::fromRGB(50,50,50));
            g.fillRoundedRectangle(b, 8.f);
            g.setColour(ZXTheme::mixAccent());
            g.drawRoundedRectangle(b, 8.f, 1.f);
        }
        void resized() override {
            auto r = getLocalBounds();
            auto top = r.removeFromTop(28);
            label.setBounds(top.withTrimmedLeft(6).withTrimmedRight(6));
            int padding = 6;
            int gap = 6;
            auto area = r.reduced(padding);
            int colW = (area.getWidth() - gap) / 2;
            int rows = 8;
            int rowH = (area.getHeight() - gap * (rows - 1)) / rows;
            int startY = area.getY();
            int idx = 0;
            for (int row = 0; row < rows; ++row) {
                auto left = juce::Rectangle<int>(area.getX(), startY + row * (rowH + gap), colW, rowH);
                auto right = juce::Rectangle<int>(left.getRight() + gap, left.getY(), colW, rowH);
                if (idx < buttons.size()) buttons[idx++]->setBounds(left);
                if (idx < buttons.size()) buttons[idx++]->setBounds(right);
            }
        }
    private:
        juce::Label label;
        juce::OwnedArray<SlotButton> buttons;
        juce::Component* anchorParent { nullptr };
        std::function<void(int, const juce::String&, const juce::String&, int)> onChoose;
    };
    class ZXCenterBox : public juce::Component {
    public:
        void setInstrument(const juce::String& cat, const juce::String& name) {
            category = cat;
            title = name.trim().toUpperCase();
            image = Assets::loadIconFor(name, cat);
            repaint();
        }
        void paint(juce::Graphics& g) override {
            auto b = getLocalBounds().toFloat();
            g.setColour(juce::Colour::fromRGB(40,40,40));
            g.fillRoundedRectangle(b, 8.f);
            g.setColour(ZXTheme::mixAccent());
            g.drawRoundedRectangle(b, 8.f, 1.f);
            auto r = getLocalBounds().reduced(12);
            auto header = r.removeFromTop(30);
            g.setColour(juce::Colours::white);
            g.setFont(juce::FontOptions(18.0f, juce::Font::bold));
            g.drawText(title, header, juce::Justification::centred, false);
            if (!image.isNull()) {
                g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
                auto dest = r.toFloat();
                int iw = image.getWidth();
                int ih = image.getHeight();
                float scale = dest.getHeight() / (float) ih;
                float drawW = iw * scale;
                float dx = dest.getX() + (dest.getWidth() - drawW) / 2.0f;
                g.drawImage(image, dx, dest.getY(), drawW, dest.getHeight(), 0, 0, iw, ih, false);
            }
        }
    private:
        juce::String category;
        juce::String title;
        juce::Image image;
    };
    std::unique_ptr<SectionPanel> voicePanel;
    std::unique_ptr<SectionPanel> controlsPanel;
    std::unique_ptr<SectionPanel> stylePanel;
    std::unique_ptr<InnerPanel> voiceZXPanel;
    std::unique_ptr<CustomVoicePanel> voiceCustomPanel;
    std::unique_ptr<InnerPanel> styleZXPanel;
    std::unique_ptr<InnerPanel> styleCustomPanel;
    std::unique_ptr<ZXCenterBox> voiceCenterBox;
    juce::String activeChannel;
    juce::TextButton btnMidiMode, btnAudioMode;
    bool isAudioMode { false };
    void populateVoiceCategories();
    juce::String categoryByMSBLSB(int msb, int lsb) {
        if (msb == 1) {
            auto name = SoundLibrary::getCustomVoiceSlotName(lsb);
            if (name.isNotEmpty()) return name.trim().toUpperCase();
            return juce::String("AUDIO SLOT ") + juce::String(lsb + 1);
        }
        return lsbToCategory(lsb);
    }
    int mapChannelToMidi(const juce::String& name) {
        if (name.startsWithIgnoreCase("RIGHT ")) {
            int n = name.substring(6).getIntValue(); return juce::jlimit(1, 5, n);
        }
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
    int categoryToMSB(const juce::String&) {
        return 0; // Main voices bank; user voices will use MSB=1 when wired
    }
    int categoryToLSB(const juce::String& cat) {
        if (cat.equalsIgnoreCase("Piano")) return 0;
        if (cat.equalsIgnoreCase("El Piano")) return 1;
        if (cat.equalsIgnoreCase("Organ")) return 2;
        if (cat.equalsIgnoreCase("Strings")) return 3;
        if (cat.equalsIgnoreCase("Woodwind")) return 4;
        if (cat.equalsIgnoreCase("Acc Gtr")) return 5;
        if (cat.equalsIgnoreCase("Bass")) return 6;
        if (cat.equalsIgnoreCase("Pad")) return 7;
        if (cat.equalsIgnoreCase("DrumKit")) return 8;
        if (cat.equalsIgnoreCase("Accordion")) return 9;
        if (cat.equalsIgnoreCase("Brass")) return 10;
        if (cat.equalsIgnoreCase("Choir")) return 11;
        if (cat.equalsIgnoreCase("El Gtr")) return 12;
        if (cat.equalsIgnoreCase("Synth")) return 13;
        if (cat.equalsIgnoreCase("SFX")) return 14;
        if (cat.equalsIgnoreCase("Percussion")) return 15;
        if (cat.equalsIgnoreCase("AUDIO DRUMS")) return 20;
        if (cat.equalsIgnoreCase("AUDIO PERCUSSION")) return 21;
        if (cat.equalsIgnoreCase("AUDIO BASS")) return 22;
        if (cat.equalsIgnoreCase("AUDIO ACC GTR")) return 23;
        if (cat.equalsIgnoreCase("AUDIO EL GTR")) return 24;
        if (cat.equalsIgnoreCase("AUDIO DIST GTR")) return 25;
        return 0;
    }
    juce::String lsbToCategory(int lsb) {
        switch (lsb) {
            case 0:  return "Piano";
            case 1:  return "El Piano";
            case 2:  return "Organ";
            case 3:  return "Strings";
            case 4:  return "Woodwind";
            case 5:  return "Acc Gtr";
            case 6:  return "Bass";
            case 7:  return "Pad";
            case 8:  return "DrumKit";
            case 9:  return "Accordion";
            case 10: return "Brass";
            case 11: return "Choir";
            case 12: return "El Gtr";
            case 13: return "Synth";
            case 14: return "SFX";
            case 15: return "Percussion";
            case 20: return "AUDIO DRUMS";
            case 21: return "AUDIO PERCUSSION";
            case 22: return "AUDIO BASS";
            case 23: return "AUDIO ACC GTR";
            case 24: return "AUDIO EL GTR";
            case 25: return "AUDIO DIST GTR";
            default: return "Piano";
        }
    }
};
