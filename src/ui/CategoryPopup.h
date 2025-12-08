#pragma once
#include <JuceHeader.h>
#include "../core/SoundLibrary.h"
#include "ZXTheme.h"
#include "../core/Assets.h"
class ItemButton : public juce::Button {
public:
    ItemButton(const juce::String& cat) : juce::Button(juce::String()), category(cat) {}
    void setText(const juce::String& t) { text = t; icon = Assets::loadIconFor(t, category); repaint(); }
    void setOnActivate(std::function<void()> fn) { onActivate = std::move(fn); }
    void setOnContext(std::function<void(const juce::MouseEvent&)> fn) { onContext = std::move(fn); }
    void setOnInlineRename(std::function<void(const juce::String&)> fn) { onInlineRename = std::move(fn); }
    void beginInlineRename() { beginInlineEdit(); }
    void paintButton(juce::Graphics& g, bool over, bool down) override {
        auto b = getLocalBounds().toFloat();
        auto bg = juce::Colour::fromRGB(45,45,45);
        if (down) bg = bg.brighter(0.15f); else if (over) bg = bg.brighter(0.08f);
        g.setColour(bg);
        g.fillRoundedRectangle(b, 6.f);
        g.setColour(selected ? juce::Colours::limegreen : ZXTheme::mixAccent());
        g.drawRoundedRectangle(b, 6.f, 1.5f);
        int margin = 6;
        int side = juce::jmax(32, getHeight() - margin*2);
        auto iconRect = juce::Rectangle<int>(getX()+margin, getY()+margin, side, side).translated(-getX(), -getY());
        if (!icon.isNull()) {
            g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
            juce::Path rounded;
            rounded.addRoundedRectangle(iconRect.toFloat(), 6.0f);
            {
                juce::Graphics::ScopedSaveState save(g);
                g.reduceClipRegion(rounded);
                int iw = icon.getWidth();
                int ih = icon.getHeight();
                int crop = juce::jmin(iw, ih);
                int sx = (iw - crop) / 2;
                int sy = (ih - crop) / 2;
                g.drawImage(icon, iconRect.getX(), iconRect.getY(), iconRect.getWidth(), iconRect.getHeight(), sx, sy, crop, crop, false);
            }
            g.setColour(juce::Colours::grey);
            g.strokePath(rounded, juce::PathStrokeType(1.0f));
        } else {
            juce::Path rounded;
            rounded.addRoundedRectangle(iconRect.toFloat(), 6.0f);
            g.setColour(juce::Colour::fromRGB(25,25,25));
            g.fillPath(rounded);
            g.setColour(juce::Colours::grey);
            g.strokePath(rounded, juce::PathStrokeType(1.0f));
            g.setColour(juce::Colours::white);
            g.setFont(juce::FontOptions(20.0f, juce::Font::bold));
            auto letter = category.isNotEmpty() ? category.substring(0,1).toUpperCase() : juce::String("?");
            g.drawText(letter, iconRect, juce::Justification::centred, false);
        }
        int textX = iconRect.getRight() + 10;
        auto textArea = juce::Rectangle<int>(textX, margin, getWidth() - textX - margin, getHeight() - margin*2);
        g.setColour(juce::Colours::white);
        g.setFont(juce::FontOptions(16.0f, juce::Font::plain));
        g.drawText(text, textArea, juce::Justification::centredLeft, true);
        if (editor != nullptr) editor->setBounds(textArea);
    }
    void setSelected(bool s) { selected = s; repaint(); }
    void clicked() override { if (editor == nullptr) { if (onActivate) onActivate(); } }
    void mouseDown(const juce::MouseEvent& e) override {
        if (e.mods.isRightButtonDown()) { if (onContext) onContext(e); return; }
        if (e.mods.isShiftDown() && e.mods.isLeftButtonDown()) { if (onInlineRename) beginInlineEdit(); return; }
        juce::Button::mouseDown(e);
    }
private:
    void beginInlineEdit() {
        if (editor != nullptr) return;
        editor.reset(new juce::TextEditor());
        addAndMakeVisible(editor.get());
        editor->setText(text, juce::NotificationType::dontSendNotification);
        editor->setSelectAllWhenFocused(true);
        editor->grabKeyboardFocus();
        editor->onReturnKey = [this]{ commitInlineEdit(); };
        editor->onEscapeKey = [this]{ cancelInlineEdit(); };
        editor->onFocusLost = [this]{ commitInlineEdit(); };
        repaint();
    }
    void commitInlineEdit() {
        if (editor == nullptr) return;
        auto newText = editor->getText().trim();
        if (newText.isNotEmpty()) {
            text = newText;
            if (onInlineRename) onInlineRename(newText);
        }
        removeChildComponent(editor.get());
        editor.reset();
        repaint();
    }
    void cancelInlineEdit() {
        if (editor == nullptr) return;
        removeChildComponent(editor.get());
        editor.reset();
        repaint();
    }
    juce::String text;
    std::function<void()> onActivate;
    std::function<void(const juce::MouseEvent&)> onContext;
    std::function<void(const juce::String&)> onInlineRename;
    juce::Image icon;
    juce::String category;
    std::unique_ptr<juce::TextEditor> editor;
    bool selected { false };
};
class CategoryPopup : public juce::Component {
public:
    CategoryPopup(const juce::String& cat) : category(cat) {
        addAndMakeVisible(title);
        title.setText(cat, juce::NotificationType::dontSendNotification);
        title.setJustificationType(juce::Justification::centred);
        title.setColour(juce::Label::textColourId, juce::Colours::white);
        title.setFont(juce::FontOptions(18.0f, juce::Font::bold));
        items = SoundLibrary::listCategoryItems(category);
        if (items.isEmpty()) { for (int i = 1; i <= 50; ++i) items.add("Instrument " + juce::String(i)); }
        for (int i = 0; i < 10; ++i) {
            auto* b = itemButtons.add(new ItemButton(cat));
            b->setOnActivate([this,i]{
                if (onChoose) {
                    lastClickedButton = i;
                    int idx = page * 10 + i;
                    juce::String name = idx < items.size() ? items[idx] : juce::String();
                    onChoose(name, category);
                }
            });
            b->setOnContext([this,i,b](const juce::MouseEvent&){
                int pc = page * 10 + i + 1;
                juce::PopupMenu m;
                m.addItem(1, "Delete");
                m.addItem(2, "Rename");
                m.addItem(3, "Copy");
                m.addItem(4, "Paste", SoundLibrary::hasVoiceClipboard());
                m.addItem(5, "Move");
                m.showMenuAsync(juce::PopupMenu::Options(), [this, pc, b](int r){
                    if (r == 1) {
                        auto f = SoundLibrary::voiceFileFor(category, pc);
                        if (f.existsAsFile()) { SoundLibrary::deleteVoiceFile(f); InstrumentHost::instance().removeInstrumentByFile(f); }
                        else { SoundLibrary::deleteVoicePosition(category, pc); }
                        setPage(page);
                    }
                    else if (r == 2) { b->beginInlineRename(); }
                    else if (r == 3) { SoundLibrary::copyVoiceItem(category, pc); }
                    else if (r == 4) { bool occupied = (pc <= items.size() && items[pc-1].isNotEmpty()); bool doReplace = true; if (occupied) doReplace = juce::NativeMessageBox::showOkCancelBox(juce::MessageBoxIconType::QuestionIcon, "Replace", "Target occupied. Replace?", this, nullptr); SoundLibrary::pasteVoiceItem(category, pc, doReplace); setPage(page); }
                    else if (r == 5) { int currentPc = pc; struct MoveDialog : public juce::Component { juce::TextEditor cat, pos; juce::TextButton ok{ "OK" }; std::function<void(juce::String,int)> onDone; MoveDialog(const juce::String& c, int initialPc){ addAndMakeVisible(cat); addAndMakeVisible(pos); addAndMakeVisible(ok); setSize(280, 110); cat.setText(c); pos.setText(juce::String(initialPc)); ok.onClick = [this]{ if (onDone) onDone(cat.getText(), pos.getText().getIntValue()); }; } void resized() override { auto r=getLocalBounds().reduced(8); cat.setBounds(r.removeFromTop(30)); pos.setBounds(r.removeFromTop(30)); ok.setBounds(r.removeFromTop(28)); } }; struct InlineMoveWindow : public juce::DialogWindow { InlineMoveWindow() : juce::DialogWindow("Move Voice", juce::Colours::black, true) {} void closeButtonPressed() override { exitModalState(0); auto sp = juce::Component::SafePointer<InlineMoveWindow>(this); juce::MessageManager::callAsync([sp]{ if (sp.getComponent() != nullptr) delete sp.getComponent(); }); } }; auto* dlg = new MoveDialog(category, currentPc); auto* win = new InlineMoveWindow(); win->setContentOwned(dlg, true); win->centreAroundComponent(this, 300, 140); win->setResizable(false, false); win->setUsingNativeTitleBar(false); win->setTitleBarButtonsRequired(0, 0); win->enterModalState(true, nullptr, true); dlg->onDone = [this, win, currentPc](juce::String dstCat, int dstPc){ bool occupied = false; auto items2 = SoundLibrary::listCategoryItems(dstCat); if (dstPc>=1 && dstPc<=items2.size()) occupied = items2[dstPc-1].isNotEmpty(); bool doReplace = true; if (occupied) doReplace = juce::NativeMessageBox::showOkCancelBox(juce::MessageBoxIconType::QuestionIcon, "Replace", "Target occupied. Replace?", this, nullptr); SoundLibrary::moveVoiceItem(category, currentPc, dstCat, dstPc, doReplace); setPage(page); win->exitModalState(0); auto sp = juce::Component::SafePointer<juce::DialogWindow>(win); juce::MessageManager::callAsync([sp]{ if (sp.getComponent() != nullptr) delete sp.getComponent(); }); }; }
                });
            });
            b->setOnInlineRename([this,i](const juce::String& newTitle){ int pc = page * 10 + i + 1; SoundLibrary::renameVoiceItem(category, pc, newTitle); setPage(page); });
            addAndMakeVisible(b);
        }
        for (int i = 0; i < 10; ++i) {
            auto* p = pageButtons.add(new juce::TextButton(juce::String(i+1)));
            p->setColour(juce::TextButton::textColourOffId, juce::Colours::white);
            p->onClick = [this,i]{ setPage(i); };
            addAndMakeVisible(p);
        }
        setPage(0);
    }
    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour::fromRGB(30,30,30));
        g.setColour(ZXTheme::mixAccent());
        g.drawRoundedRectangle(getLocalBounds().toFloat(), 8.f, 1.f);
    }
    void resized() override {
        auto r = getLocalBounds().reduced(12);
        auto header = r.removeFromTop(30);
        title.setBounds(header);
        auto grid = r.removeFromTop(r.getHeight() - 40);
        int gap = 8;
        int colW = (grid.getWidth() - gap) / 2;
        int rowH = (grid.getHeight() - gap * 4) / 5;
        int idx = 0;
        for (int row = 0; row < 5; ++row) {
            auto left = juce::Rectangle<int>(grid.getX(), grid.getY() + row * (rowH + gap), colW, rowH);
            auto right = juce::Rectangle<int>(left.getRight() + gap, left.getY(), colW, rowH);
            if (idx < itemButtons.size()) itemButtons[idx++]->setBounds(left);
            if (idx < itemButtons.size()) itemButtons[idx++]->setBounds(right);
        }
        auto pages = r;
        int pw = 34;
        int ph = 26;
        int startX = pages.getX();
        int yCentered = pages.getY() + (pages.getHeight() - ph) / 2;
        for (int i = 0; i < pageButtons.size(); ++i) pageButtons[i]->setBounds(startX + i * (pw + 6), yCentered, pw, ph);
    }
    void mouseDown(const juce::MouseEvent& e) override { dragStart = e.getPosition(); dragBounds = getBounds(); juce::Component::mouseDown(e); }
    void mouseDrag(const juce::MouseEvent& e) override { if (!dragEnabled) { juce::Component::mouseDrag(e); return; } int nx = dragBounds.getX() + (e.getPosition().x - dragStart.x); int ny = dragBounds.getY() + (e.getPosition().y - dragStart.y); int minX = dragLimit.getX(); int maxX = dragLimit.getRight() - getWidth(); int minY = dragLimit.getY(); int maxY = dragLimit.getBottom() - getHeight(); nx = juce::jlimit(minX, juce::jmax(minX, maxX), nx); ny = juce::jlimit(minY, juce::jmax(minY, maxY), ny); setBounds(nx, ny, getWidth(), getHeight()); }
    void enableDrag(const juce::Rectangle<int>& limit) { dragEnabled = true; dragLimit = limit; }
    static void open(juce::Component* parent, const juce::String& category, std::function<void(int, const juce::String&, const juce::String&)> chooseCb = {}, int selectedPc = -1, int rightLimitX = -1, juce::Rectangle<int> panelArea = {}) {
        auto* tlc = parent->getTopLevelComponent();
        if (!tlc) tlc = parent;
        struct Overlay : public juce::Component {
            CategoryPopup* content { nullptr };
            std::function<void()> onClose;
            juce::Rectangle<int> voiceArea;
            bool hitTest(int x, int y) override {
                juce::Point<int> p(x, y);
                if (content && content->getBounds().contains(p)) return true;
                if (!voiceArea.isEmpty() && voiceArea.contains(p)) return false;
                return true;
            }
            void mouseDown(const juce::MouseEvent& e) override {
                if (content && content->getBounds().contains(e.getPosition())) return;
                if (!voiceArea.isEmpty() && voiceArea.contains(e.getPosition())) return;
                if (content) {
                    auto sp = juce::Component::SafePointer<Overlay>(this);
                    auto cb = onClose;
                    juce::MessageManager::callAsync([sp, cb]{
                        if (cb) cb();
                        if (sp.getComponent() != nullptr) delete sp.getComponent();
                    });
                }
            }
        };
        static Overlay* gOverlay = nullptr;
        if (gOverlay == nullptr || gOverlay->getParentComponent() != tlc) {
            if (gOverlay != nullptr) { auto sp = juce::Component::SafePointer<Overlay>(gOverlay); juce::MessageManager::callAsync([sp]{ if (sp.getComponent() != nullptr) delete sp.getComponent(); }); }
            gOverlay = new Overlay();
            tlc->addAndMakeVisible(gOverlay);
            gOverlay->setBounds(tlc->getLocalBounds());
        }
        gOverlay->voiceArea = panelArea;
        auto* content = new CategoryPopup(category);
        if (gOverlay->content != nullptr) gOverlay->removeChildComponent(gOverlay->content);
        gOverlay->content = content;
        gOverlay->onClose = [tlc]{
            // remove existing overlay and clear static pointer
            auto* overlayPtr = gOverlay;
            gOverlay = nullptr;
            if (overlayPtr != nullptr) {
                auto sp = juce::Component::SafePointer<juce::Component>(overlayPtr);
                juce::MessageManager::callAsync([sp]{ if (sp.getComponent() != nullptr) delete sp.getComponent(); });
            }
        };
        gOverlay->addAndMakeVisible(content);
        auto vpLocal = parent->getParentComponent() ? parent->getParentComponent()->getLocalBounds() : parent->getLocalBounds();
        auto vpArea = tlc->getLocalArea(parent->getParentComponent() ? parent->getParentComponent() : parent, vpLocal);
        auto anchor = tlc->getLocalArea(parent, parent->getLocalBounds());
        int h = juce::jmin(480, vpArea.getHeight() - 12); h = juce::jmax(240, h);
        int wMax = vpArea.getWidth() - 12; int w = juce::jmin(720, wMax); w = juce::jmax(240, w);
        bool anchorLeft = anchor.getCentreX() < vpArea.getCentreX();
        int x = anchorLeft ? anchor.getRight() + 6 : (vpArea.getX() + 6);
        int xRightLimit = (rightLimitX > 0 ? rightLimitX : (anchorLeft ? (vpArea.getRight() - 6) : (anchor.getX() - 6)));
        int availableW = juce::jmax(0, xRightLimit - x);
        if (availableW <= 0) {
            x = vpArea.getX() + 6;
            availableW = vpArea.getWidth() - 12;
        }
        w = juce::jmin(w, availableW);
        int y = vpArea.getBottom() - h - 6;
        content->setBounds(x, y, w, h);
        content->enableDrag(vpArea.reduced(6));
        content->onChoose = [chooseCb, content](const juce::String& name, const juce::String& cat){ int pc = content->page * 10 + 1; juce::StringArray items = content->items; int idx = items.indexOf(name); if (idx >= 0) pc = content->page * 10 + (idx % 10) + 1; if (chooseCb) chooseCb(pc, name, cat); };
        // highlight selected
        if (selectedPc >= 1) {
            content->setPage((selectedPc - 1) / 10);
            int localIndex = (selectedPc - 1) % 10;
            if (localIndex >= 0 && localIndex < content->itemButtons.size()) content->itemButtons[localIndex]->setSelected(true);
        }
    }
private:
    void setPage(int p) {
        page = juce::jlimit(0, 9, p);
        items = SoundLibrary::listCategoryItems(category);
        for (int i = 0; i < itemButtons.size(); ++i) { int idx = page * 10 + i; juce::String name = idx < items.size() ? items[idx] : juce::String(); itemButtons[i]->setText(name); itemButtons[i]->setSelected(false); }
    }
    juce::String category;
    juce::Label title;
    juce::OwnedArray<ItemButton> itemButtons;
    juce::OwnedArray<juce::TextButton> pageButtons;
    juce::StringArray items;
    int page { 0 };
public:
    std::function<void(const juce::String&, const juce::String&)> onChoose;
private:
    int lastClickedButton { -1 };
    bool dragEnabled { false };
    juce::Rectangle<int> dragLimit;
    juce::Point<int> dragStart;
    juce::Rectangle<int> dragBounds;
};

class CustomCategoryPopup : public juce::Component {
public:
    CustomCategoryPopup(int slot, const juce::String& display) : slotIndex(slot), displayName(display) {
        addAndMakeVisible(title);
        title.setText(display, juce::NotificationType::dontSendNotification);
        title.setJustificationType(juce::Justification::centred);
        title.setColour(juce::Label::textColourId, juce::Colours::white);
        title.setFont(juce::FontOptions(18.0f, juce::Font::bold));
        items = SoundLibrary::listCustomSlotItems(slotIndex);
        for (int i = 0; i < 10; ++i) {
            auto* b = itemButtons.add(new ItemButton(display));
            b->setOnActivate([this,i]{ if (onChoose) { int idx = page * 10 + i; juce::String name = idx < items.size() ? items[idx] : juce::String(); onChoose(name, displayName); } });
            b->setOnContext([this,i,b](const juce::MouseEvent&){ int pc = page * 10 + i + 1; juce::PopupMenu m; m.addItem(1, "Copy"); m.addItem(2, "Paste", SoundLibrary::hasItemClipboard()); m.addItem(3, "Rename"); m.addItem(4, "Delete"); m.addSeparator(); m.addItem(5, SoundLibrary::hasMoveItem() ? "Move Here" : "Move"); m.showMenuAsync(juce::PopupMenu::Options(), [this, pc, b](int r){ if (r == 1) { SoundLibrary::copyItemToClipboard(slotIndex, pc); } else if (r == 2) { bool occupied = SoundLibrary::hasCustomItem(slotIndex, pc); bool doReplace = true; if (occupied) doReplace = juce::NativeMessageBox::showOkCancelBox(juce::MessageBoxIconType::QuestionIcon, "Replace", "Target occupied. Replace?", this, nullptr); SoundLibrary::pasteItemFromClipboard(slotIndex, pc, doReplace); setPage(page); } else if (r == 3) { b->beginInlineRename(); } else if (r == 4) { SoundLibrary::removeCustomItem(slotIndex, pc); setPage(page); } else if (r == 5) { if (SoundLibrary::hasMoveItem()) { bool occupied = SoundLibrary::hasCustomItem(slotIndex, pc); bool doReplace = true; if (occupied) doReplace = juce::NativeMessageBox::showOkCancelBox(juce::MessageBoxIconType::QuestionIcon, "Replace", "Target occupied. Replace?", this, nullptr); SoundLibrary::finishMoveItem(slotIndex, pc, doReplace); setPage(page); } else { SoundLibrary::startMoveItem(slotIndex, pc); } } }); });
            b->setOnInlineRename([this,i](const juce::String& newTitle){ int pc = page * 10 + i + 1; SoundLibrary::renameCustomItem(slotIndex, pc, newTitle); setPage(page); });
            addAndMakeVisible(b);
        }
        for (int i = 0; i < 10; ++i) {
            auto* p = pageButtons.add(new juce::TextButton(juce::String(i+1)));
            p->setColour(juce::TextButton::textColourOffId, juce::Colours::white);
            p->onClick = [this,i]{ setPage(i); };
            addAndMakeVisible(p);
        }
        setPage(0);
    }
    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour::fromRGB(30,30,30));
        g.setColour(ZXTheme::mixAccent());
        g.drawRoundedRectangle(getLocalBounds().toFloat(), 8.f, 1.f);
    }
    void resized() override {
        auto r = getLocalBounds().reduced(12);
        auto header = r.removeFromTop(30);
        title.setBounds(header);
        auto grid = r.removeFromTop(r.getHeight() - 40);
        int gap = 8; int colW = (grid.getWidth() - gap) / 2; int rowH = (grid.getHeight() - gap * 4) / 5; int idx = 0;
        for (int row = 0; row < 5; ++row) {
            auto left = juce::Rectangle<int>(grid.getX(), grid.getY() + row * (rowH + gap), colW, rowH);
            auto right = juce::Rectangle<int>(left.getRight() + gap, left.getY(), colW, rowH);
            if (idx < itemButtons.size()) itemButtons[idx++]->setBounds(left);
            if (idx < itemButtons.size()) itemButtons[idx++]->setBounds(right);
        }
        auto pages = r; int pw = 34; int ph = 26; int startX = pages.getX(); int yCentered = pages.getY() + (pages.getHeight() - ph) / 2;
        for (int i = 0; i < pageButtons.size(); ++i) pageButtons[i]->setBounds(startX + i * (pw + 6), yCentered, pw, ph);
    }
    void mouseDown(const juce::MouseEvent& e) override { dragStart = e.getPosition(); dragBounds = getBounds(); juce::Component::mouseDown(e); }
    void mouseDrag(const juce::MouseEvent& e) override { if (!dragEnabled) { juce::Component::mouseDrag(e); return; } int nx = dragBounds.getX() + (e.getPosition().x - dragStart.x); int ny = dragBounds.getY() + (e.getPosition().y - dragStart.y); int minX = dragLimit.getX(); int maxX = dragLimit.getRight() - getWidth(); int minY = dragLimit.getY(); int maxY = dragLimit.getBottom() - getHeight(); nx = juce::jlimit(minX, juce::jmax(minX, maxX), nx); ny = juce::jlimit(minY, juce::jmax(minY, maxY), ny); setBounds(nx, ny, getWidth(), getHeight()); }
    void enableDrag(const juce::Rectangle<int>& limit) { dragEnabled = true; dragLimit = limit; }
    static void open(juce::Component* parent, int slotIndex, const juce::String& displayName, std::function<void(int, const juce::String&, const juce::String&, int)> chooseCb = {}) {
        auto* tlc = parent->getTopLevelComponent(); if (!tlc) tlc = parent;
        struct Overlay : public juce::Component { CustomCategoryPopup* content { nullptr }; std::function<void()> onClose; void mouseDown(const juce::MouseEvent& e) override { if (content && !content->getBounds().contains(e.getPosition())) { auto sp = juce::Component::SafePointer<Overlay>(this); auto cb = onClose; juce::MessageManager::callAsync([sp, cb]{ if (cb) cb(); }); } } };
        auto* overlay = new Overlay(); tlc->addAndMakeVisible(overlay); overlay->setBounds(tlc->getLocalBounds());
        auto* content = new CustomCategoryPopup(slotIndex, displayName); overlay->content = content; overlay->onClose = [overlay]{ auto sp = juce::Component::SafePointer<juce::Component>(overlay); juce::MessageManager::callAsync([sp]{ if (sp.getComponent() != nullptr) delete sp.getComponent(); }); }; overlay->addAndMakeVisible(content);
        int w = juce::jmin(720, parent->getWidth() - 12); int h = juce::jmin(480, parent->getHeight() - 12); w = juce::jmax(320, w); h = juce::jmax(240, h);
        auto areaInTlc = tlc->getLocalArea(parent, parent->getLocalBounds()); int x = areaInTlc.getCentreX() - w / 2; int y = areaInTlc.getBottom() - h - 12; content->setBounds(x, y, w, h);
        auto vpLocal2 = parent->getParentComponent() ? parent->getParentComponent()->getLocalBounds() : parent->getLocalBounds();
        auto vpArea2 = tlc->getLocalArea(parent->getParentComponent() ? parent->getParentComponent() : parent, vpLocal2);
        content->enableDrag(vpArea2.reduced(6));
        content->onChoose = [overlay, chooseCb, content, slotIndex](const juce::String& name, const juce::String& cat){ int pc = content->page * 10 + 1; juce::StringArray items = content->items; int idx = items.indexOf(name); if (idx >= 0) pc = content->page * 10 + (idx % 10) + 1; if (chooseCb) chooseCb(pc, name, cat, slotIndex); auto sp = juce::Component::SafePointer<juce::Component>(overlay); juce::MessageManager::callAsync([sp]{ if (sp.getComponent() != nullptr) delete sp.getComponent(); }); };
    }
private:
    void setPage(int p) {
        page = juce::jlimit(0, 9, p);
        items = SoundLibrary::listCustomSlotItems(slotIndex);
        for (int i = 0; i < itemButtons.size(); ++i) { int idx = page * 10 + i; juce::String name = idx < items.size() ? items[idx] : juce::String(); itemButtons[i]->setText(name); }
    }
    int slotIndex { -1 };
    juce::String displayName;
    juce::Label title;
    juce::OwnedArray<ItemButton> itemButtons;
    juce::OwnedArray<juce::TextButton> pageButtons;
    juce::StringArray items;
    int page { 0 };
    bool dragEnabled { false };
    juce::Rectangle<int> dragLimit;
    juce::Point<int> dragStart;
    juce::Rectangle<int> dragBounds;
public:
    std::function<void(const juce::String&, const juce::String&)> onChoose;
};
