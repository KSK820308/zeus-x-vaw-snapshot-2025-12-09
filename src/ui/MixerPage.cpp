#include "MixerPage.h"
#include "ZXTheme.h"
#include "../audio/MappingSamplerProcessor.h"
MixerPage::MixerPage(const juce::StringArray& namesIn, bool isBusPage) {
    names = namesIn;
    isBus = isBusPage;
    for (int i = 0; i < names.size(); ++i) {
        juce::Colour acc = ZXTheme::mixAccent();
        if (names[i].startsWithIgnoreCase("RIGHT")) acc = ZXTheme::rightAccent();
        if (names[i].startsWithIgnoreCase("LOWER")) acc = ZXTheme::lowerAccent();
        bool master = isBus && names[i].equalsIgnoreCase("MASTER");
        bool showPanel = (isBus) || (!isBus && (i < 8 || names[i].startsWithIgnoreCase("RIGHT") || names[i].equalsIgnoreCase("LOWER") || names[i].equalsIgnoreCase("MULTIPAD")));
        int busIndex = -1;
        if (isBus && names[i].startsWithIgnoreCase("BUS ")) busIndex = names[i].substring(4).getIntValue();
        auto* s = strips.add(new MixerStrip(names[i], names[i], master ? ZXTheme::labelMaster() : acc, master, showPanel, isBus, busIndex));
        addAndMakeVisible(s);
        if (!isBus)
            s->onSelected = [this](const juce::String& id){ activeChannel = id; for (int k = 0; k < strips.size(); ++k) strips[k]->setSelected(strips[k]->getId() == activeChannel); InstrumentHost::instance().setActiveChannel(activeChannel); };
        auto* tag = tags.add(new ZXTagLabel());
        tag->setText(names[i]);
        tag->setAccent(master ? ZXTheme::labelMaster() : acc);
        addAndMakeVisible(tag);
    }
    if (!isBus) {
        voicePanel.reset(new SectionPanel("VOICE"));
        controlsPanel.reset(new SectionPanel("CONTROLS"));
        stylePanel.reset(new SectionPanel("STYLE"));
        addAndMakeVisible(voicePanel.get());
        addAndMakeVisible(controlsPanel.get());
        addAndMakeVisible(stylePanel.get());
    voiceZXPanel.reset(new InnerPanel("ZEUS-X VOICE", voicePanel.get(), [this](int pc, const juce::String& name, const juce::String& cat){
        if (voiceCenterBox) voiceCenterBox->setInstrument(cat, name);
        if (activeChannel.isNotEmpty()) {
            int msb = categoryToMSB(cat);
            int lsb = categoryToLSB(cat);
            MixerStrip::setMSB(activeChannel, msb);
            MixerStrip::setLSB(activeChannel, lsb);
            int ch = mapChannelToMidi(activeChannel);
            InstrumentHost::instance().sendBankSelect(msb, lsb, ch);
            MixerStrip::setPC(activeChannel, pc);
            InstrumentHost::instance().sendProgramChange(pc - 1, ch);
            for (int k = 0; k < strips.size(); ++k) if (strips[k]->getId() == activeChannel) strips[k]->refreshCodeDisplay();
            auto f = SoundLibrary::voiceFileFor(cat, pc);
            if (f.existsAsFile()) {
                auto proc = std::make_unique<MappingSamplerProcessor>();
                auto* mp = proc.get();
                InstrumentHost::instance().setChannelProcessor(activeChannel, std::move(proc));
                int msbDummy{}, lsbDummy{}, pcDummy{}; juce::String t;
                mp->importInstrument(f, msbDummy, lsbDummy, pcDummy, t);
                InstrumentHost::instance().setChannelLastVoiceFile(activeChannel, f);
            }
        }
    }));
    voiceZXPanel->setRightLimitProvider([this]{ auto* tlc = getTopLevelComponent(); juce::Rectangle<int> r; for (int i = 0; i < strips.size(); ++i) { if (strips[i]->getId().equalsIgnoreCase("CHORD 2")) { r = tlc->getLocalArea(this, strips[i]->getBounds()); break; } } if (r.isEmpty()) { r = tlc->getLocalArea(this, getLocalBounds()); } return r.getRight() - 6; });
        voiceCustomPanel.reset(new CustomVoicePanel(voicePanel.get(), [this](int pc, const juce::String& name, const juce::String& cat, int slotIndex){ if (voiceCenterBox) voiceCenterBox->setInstrument(cat, name); if (activeChannel.isNotEmpty()) { int msb = 1; int lsb = slotIndex; MixerStrip::setMSB(activeChannel, msb); MixerStrip::setLSB(activeChannel, lsb); int ch = mapChannelToMidi(activeChannel); InstrumentHost::instance().sendBankSelect(msb, lsb, ch); MixerStrip::setPC(activeChannel, pc); InstrumentHost::instance().sendProgramChange(pc - 1, ch); for (int k = 0; k < strips.size(); ++k) if (strips[k]->getId() == activeChannel) strips[k]->refreshCodeDisplay(); } }));
        styleZXPanel.reset(new InnerPanel("ZEUS-X STYLE"));
        styleCustomPanel.reset(new InnerPanel("CUSTOM STYLE"));
        addAndMakeVisible(voiceZXPanel.get());
        addAndMakeVisible(voiceCustomPanel.get());
        addAndMakeVisible(styleZXPanel.get());
        addAndMakeVisible(styleCustomPanel.get());
        voiceCenterBox.reset(new ZXCenterBox());
        addAndMakeVisible(voiceCenterBox.get());
        addAndMakeVisible(btnMidiMode);
        addAndMakeVisible(btnAudioMode);
    }
    btnMidiMode.setButtonText("MIDI");
    btnAudioMode.setButtonText("AUDIO");
    btnMidiMode.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    btnAudioMode.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    auto applyModeUI = [this]{
        auto active = juce::Colours::limegreen;
        btnMidiMode.setColour(juce::TextButton::textColourOffId, isAudioMode ? juce::Colours::white : active);
        btnAudioMode.setColour(juce::TextButton::textColourOffId, isAudioMode ? active : juce::Colours::white);
    };
    if (!isBus) {
        isAudioMode = false;
        applyModeUI();
        btnMidiMode.onClick = [this, applyModeUI]{ isAudioMode = false; applyModeUI(); populateVoiceCategories(); };
        btnAudioMode.onClick = [this, applyModeUI]{ isAudioMode = true;  applyModeUI(); populateVoiceCategories(); };
        populateVoiceCategories();
    }
    activeChannel = "RIGHT 1";
    for (int k = 0; k < strips.size(); ++k) strips[k]->setSelected(strips[k]->getId() == activeChannel);
    InstrumentHost::instance().setActiveChannel(activeChannel);
}

void MixerPage::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds().reduced(8);
    auto tagArea = bounds.removeFromTop(34);
    auto topArea = bounds.removeFromTop((int) std::round(bounds.getHeight() * 0.49));
    auto bottom = bounds;
    auto left   = voicePanel    ? voicePanel->getBounds()    : bottom.removeFromLeft(bottom.getWidth()/3);
    auto middle = controlsPanel ? controlsPanel->getBounds() : bottom.removeFromLeft(bottom.getWidth()/3);
    auto right  = stylePanel    ? stylePanel->getBounds()    : bottom;
    auto drawSection = [&](juce::Rectangle<int> r, const juce::String& title){
        auto rf = r.toFloat();
        g.setColour(juce::Colour::fromRGB(60,60,60));
        g.fillRoundedRectangle(rf, 8.f);
        g.setColour(ZXTheme::mixAccent());
        g.drawRoundedRectangle(rf, 8.f, 1.f);
        g.setColour(juce::Colours::white);
        g.setFont(juce::FontOptions(22.0f, juce::Font::bold));
        auto titleRow = r.removeFromTop(32);
        g.drawText(title, titleRow, juce::Justification::centred, false);
    };
    if (!isBus) { drawSection(left, "VOICE"); drawSection(middle, "CONTROLS"); drawSection(right, "STYLE"); }
}

void MixerPage::resized() {
    auto bounds = getLocalBounds().reduced(8);
    auto tagArea = bounds.removeFromTop(34);
    auto topArea = bounds.removeFromTop((int) std::round(bounds.getHeight() * 0.49));
    int n = strips.size();
    int spacing = 6;
    int w = juce::jmax(72, (topArea.getWidth() - spacing * juce::jmax(0, n-1)) / juce::jmax(1, n));
    int x = topArea.getX();
    juce::Font f(juce::FontOptions(15.0f, juce::Font::bold));
    for (int i = 0; i < strips.size(); ++i) {
        auto stripBounds = juce::Rectangle<int>(x, topArea.getY(), w, topArea.getHeight());
        strips[i]->setBounds(stripBounds);
        int labelW = w;
        int labelH = 30;
        int labelX = x;
        tags[i]->setBounds(labelX, tagArea.getY(), labelW, labelH);
        x += w + spacing;
    }
    auto bottom = bounds;
    int gap = spacing;
    int vgap = 8;
    int channelsPerSection = juce::jmax(1, n / 3);
    int totalChannelsW = n * w + spacing * juce::jmax(0, n - 1);
    int baseX = topArea.getX();
    int gridRight = baseX + totalChannelsW;
    int sectionWExact = channelsPerSection * w + spacing * juce::jmax(0, channelsPerSection - 1);
    int bottomY = bottom.getY() + vgap;
    int bottomH = juce::jmax(0, bottom.getHeight() - vgap);
    auto left = juce::Rectangle<int>(baseX, bottomY, sectionWExact, bottomH);
    auto middleX = baseX + sectionWExact + gap;
    auto middle = juce::Rectangle<int>(middleX, bottomY, sectionWExact, bottomH);
    auto rightX = middleX + sectionWExact + gap;
    auto right = juce::Rectangle<int>(rightX, bottomY, juce::jmax(0, gridRight - rightX), bottomH);
    if (voicePanel) voicePanel->setBounds(left);
    if (controlsPanel) controlsPanel->setBounds(middle);
    if (stylePanel) stylePanel->setBounds(right);
    if (voicePanel) voicePanel->toFront(true);
    if (controlsPanel) controlsPanel->toFront(true);
    if (stylePanel) stylePanel->toFront(true);
    if (!isBus) {
        if (voiceZXPanel) voiceZXPanel->setBounds(left.getX(), left.getY(), w, left.getHeight());
        if (voiceCustomPanel) voiceCustomPanel->setBounds(left.getRight() - w, left.getY(), w, left.getHeight());
        if (styleZXPanel) styleZXPanel->setBounds(right.getX(), right.getY(), w, right.getHeight());
        if (styleCustomPanel) styleCustomPanel->setBounds(right.getRight() - w, right.getY(), w, right.getHeight());
        if (voiceZXPanel) voiceZXPanel->toFront(true);
        if (voiceCustomPanel) voiceCustomPanel->toFront(true);
        if (styleZXPanel) styleZXPanel->toFront(true);
        if (styleCustomPanel) styleCustomPanel->toFront(true);
    }
    if (voicePanel && voiceZXPanel && voiceCustomPanel && voiceCenterBox) {
        auto vp = voicePanel->getBounds();
        auto leftCol = voiceZXPanel->getBounds();
        auto rightCol = voiceCustomPanel->getBounds();
        int headerH = 28;
        int padding = 6;
        int gapRow = 6;
        int rows = 8;
        int areaH = leftCol.getHeight() - headerH - 2 * padding;
        int rowH = (areaH - gapRow * (rows - 1)) / rows;
        auto rowTop = [&](int idx){ return leftCol.getY() + headerH + padding + idx * (rowH + gapRow); };
        int topY = rowTop(1);
        int bottomY = rowTop(6) + rowH;
        int leftX = leftCol.getRight() + gapRow;
        int rightX = rightCol.getX() - gapRow;
        voiceCenterBox->setBounds(leftX, topY, juce::jmax(0, rightX - leftX), juce::jmax(0, bottomY - topY));
        voiceCenterBox->toFront(false);
    }
    if (voicePanel) {
        auto vp = voicePanel->getBounds();
        int headerH = 32; int wbtn = 64; int hbtn = 22; int gap = 16;
        auto header = juce::Rectangle<int>(vp.getX(), vp.getY(), vp.getWidth(), headerH);
        juce::Font f(juce::FontOptions(22.0f, juce::Font::bold));
        int textW = (int) std::round(f.getStringWidthFloat("VOICE"));
        int centreX = header.getCentreX();
        int leftX = centreX - (textW/2) - gap - wbtn;
        int rightX = centreX + (textW/2) + gap;
        btnMidiMode.setBounds(leftX, header.getY() + (headerH - hbtn)/2, wbtn, hbtn);
        btnAudioMode.setBounds(rightX, header.getY() + (headerH - hbtn)/2, wbtn, hbtn);
        btnMidiMode.toFront(true);
        btnAudioMode.toFront(true);
    }
    for (int i = 0; i < strips.size(); ++i) {
        if (!isBus)
            strips[i]->onSelected = [this](const juce::String& id){ activeChannel = id; for (int k = 0; k < strips.size(); ++k) strips[k]->setSelected(strips[k]->getId() == activeChannel); InstrumentHost::instance().setActiveChannel(activeChannel); };
        if (!isBus)
            strips[i]->onCodeChanged = [this](const juce::String& id, int msb, int lsb, int pc){ auto cat = categoryByMSBLSB(msb, lsb); juce::StringArray items = (msb == 1 ? SoundLibrary::listCustomSlotItems(lsb) : SoundLibrary::listCategoryItems(cat)); juce::String name; int idx = pc - 1; if (idx >= 0 && idx < items.size()) name = items[idx]; else name = juce::String("Instrument ") + juce::String(pc); if (voiceCenterBox) voiceCenterBox->setInstrument(cat, name); };
    }
}

void MixerPage::populateVoiceCategories() {
    if (!voiceZXPanel) return;
    voiceZXPanel->clearButtons();
    if (!isAudioMode) {
        juce::String leftCol[]  = { "Piano", "Organ", "Strings", "Woodwind", "Acc Gtr", "Bass", "Pad", "DrumKit" };
        juce::String rightCol[] = { "El Piano", "Accordion", "Brass", "Choir", "El Gtr", "Synth", "SFX", "Percussion" };
        juce::StringArray names;
        for (int row = 0; row < 8; ++row) { names.add(leftCol[row]); names.add(rightCol[row]); }
        voiceZXPanel->setSingleColumn(false);
        for (int i = 0; i < names.size(); ++i) voiceZXPanel->addButton(names[i]);
    } else {
        juce::String audioCats[] = { "AUDIO DRUMS", "AUDIO PERCUSSION", "AUDIO BASS", "AUDIO ACC GTR", "AUDIO EL GTR", "AUDIO DIST GTR" };
        voiceZXPanel->setSingleColumn(true);
        for (auto& c : audioCats) voiceZXPanel->addButton(c);
    }
    voiceZXPanel->resized();
}
