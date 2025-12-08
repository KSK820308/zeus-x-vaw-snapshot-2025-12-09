#include "MappingSamplerProcessor.h"
#include "InstrumentHost.h"
#include "../core/SoundLibrary.h"
#include "../ui/CategoryPopup.h"

class LinearKeyboard : public juce::Component {
public:
    void setZoom(float z) { zoomScale = juce::jlimit(0.5f, 3.0f, z); repaint(); }
    float getZoom() const { return zoomScale; }
    void setSelectedNote(int n) { selectedNote = juce::jlimit(0, 127, n); repaint(); }
    int getSelectedNote() const { return selectedNote; }
    juce::Rectangle<float> getRectangleForKey(int note) const {
        auto a = getLocalBounds().toFloat(); float w = a.getWidth() / 128.0f; return { a.getX() + w * note, a.getBottom() - keyHeight, w, keyHeight };
    }
    void paint(juce::Graphics& g) override {
        auto a = getLocalBounds().toFloat(); g.setColour(juce::Colours::white); g.fillRect(a);
        float w = a.getWidth() / 128.0f; for (int n = 0; n < 128; ++n) {
            auto r = getRectangleForKey(n);
            bool isBlack = (n % 12 == 1 || n % 12 == 3 || n % 12 == 6 || n % 12 == 8 || n % 12 == 10);
            g.setColour(isBlack ? juce::Colours::black : juce::Colours::white);
            g.fillRect(r);
            g.setColour(juce::Colours::black); g.drawRect(r);
            g.setColour(juce::Colour::fromRGB(40,40,40));
            g.setFont(juce::FontOptions(10.0f));
            auto name = juce::MidiMessage::getMidiNoteName(n, true, true, 4);
            g.drawText(name, r.reduced(2).removeFromBottom(12), juce::Justification::centred);
            if (n == selectedNote) { g.setColour(juce::Colours::yellow.withAlpha(0.35f)); g.fillRect(r); g.setColour(juce::Colours::yellow); g.drawRect(r, 2.0f); }
        }
    }
    void mouseDown(const juce::MouseEvent& e) override {
        auto a = getLocalBounds().toFloat(); float w = a.getWidth() / 128.0f; int n = juce::jlimit(0,127,(int) ((e.position.x - a.getX()) / w)); setSelectedNote(n); if (onNote) onNote(n, true);
    }
    void mouseUp(const juce::MouseEvent& e) override { if (selectedNote >= 0) if (onNote) onNote(selectedNote, false); }
    std::function<void(int,bool)> onNote;
private:
    float zoomScale { 1.0f };
    int selectedNote { -1 };
    float keyHeight { 110.0f };
};

class UpDownKnob : public juce::Component {
public:
    UpDownKnob(const juce::String& name, double minV, double maxV, double stepV, double initial, bool integer, std::function<juce::String(double)> fmt = {})
        : label(name), minVal(minV), maxVal(maxV), step(stepV), isInt(integer), formatter(fmt) {
        addAndMakeVisible(label);
        addAndMakeVisible(value);
        addAndMakeVisible(up);
        addAndMakeVisible(down);
        label.setColour(juce::Label::textColourId, juce::Colours::white);
        label.setText(name, juce::NotificationType::dontSendNotification);
        value.setColour(juce::Label::textColourId, juce::Colours::white);
        value.setColour(juce::Label::backgroundColourId, juce::Colour::fromRGB(35,38,42));
        value.setJustificationType(juce::Justification::centredRight);
        value.setEditable(true, true, false);
        up.setButtonText("▲");
        down.setButtonText("▼");
        up.setRepeatSpeed(300, 60, 20);
        down.setRepeatSpeed(300, 60, 20);
        up.setColour(juce::TextButton::buttonColourId, juce::Colour::fromRGB(62,70,80));
        up.setColour(juce::TextButton::buttonOnColourId, juce::Colour::fromRGB(80,120,200));
        up.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
        down.setColour(juce::TextButton::buttonColourId, juce::Colour::fromRGB(62,70,80));
        down.setColour(juce::TextButton::buttonOnColourId, juce::Colour::fromRGB(80,120,200));
        down.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
        setValue(initial);
        up.onClick = [this]{ setValue(current + step); if (onChange) onChange(current); };
        down.onClick = [this]{ setValue(current - step); if (onChange) onChange(current); };
        value.setTooltip(name);
        value.onTextChange = [this]{ auto t = value.getText().trim(); double v = current; if (isInt) v = (double) t.getIntValue(); else v = t.getDoubleValue(); setValue(v); if (onChange) onChange(current); };
    }
    void paint(juce::Graphics& g) override {
        auto r = getLocalBounds().toFloat();
        g.setColour(juce::Colour::fromRGB(45,50,58));
        g.fillRoundedRectangle(r, 6.0f);
        g.setColour(juce::Colour::fromRGB(70,78,88));
        g.drawRoundedRectangle(r, 6.0f, 1.0f);
        g.setColour(juce::Colours::white);
        g.setFont(juce::FontOptions(12.0f));
    }
    void setValue(double v) {
        current = juce::jlimit(minVal, maxVal, v);
        juce::String text;
        if (formatter) text = formatter(current);
        else if (isInt) text = juce::String((int) std::round(current));
        else text = juce::String(current, 3);
        value.setText(text, juce::NotificationType::dontSendNotification);
    }
    double getValue() const { return current; }
    void resized() override {
        auto r = getLocalBounds().reduced(4);
        auto top = r.removeFromTop(20);
        label.setBounds(top);
        auto btnArea = r.removeFromRight(24);
        up.setBounds(btnArea.removeFromTop(btnArea.getHeight()/2).reduced(2));
        down.setBounds(btnArea.reduced(2));
        value.setBounds(r.reduced(2));
    }
    std::function<void(double)> onChange;
private:
    juce::Label label;
    juce::Label value;
    juce::TextButton up, down;
    double minVal, maxVal, step;
    double current { 0.0 };
    bool isInt { false };
    std::function<juce::String(double)> formatter;
};

class MappingSamplerEditor : public juce::AudioProcessorEditor, public juce::FileDragAndDropTarget {
public:
    MappingSamplerEditor(MappingSamplerProcessor& p) : juce::AudioProcessorEditor(&p), proc(p) {
        setSize(960, 540);
        
        
        addAndMakeVisible(knA);
        addAndMakeVisible(knD);
        addAndMakeVisible(knS);
        addAndMakeVisible(knR);
        addAndMakeVisible(viewport);
        addAndMakeVisible(status);
        viewport.setScrollBarsShown(false, true);
        viewport.setScrollBarThickness(18);
        viewport.setViewedComponent(&canvas, false);
        canvas.addAndMakeVisible(dropHint);
        canvas.addAndMakeVisible(grid);
        canvas.addAndMakeVisible(zones);
        canvas.addAndMakeVisible(kb);
        zones.proc = &proc;
        dropHint.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
        dropHint.setJustificationType(juce::Justification::centred);
        dropHint.setText("Drag & Drop WAV/AIFF files here", juce::NotificationType::dontSendNotification);
        
        knA.onChange = [this](double v){ proc.setADSR((float)v, (float)knD.getValue(), (float)knS.getValue(), (float)knR.getValue()); };
        knD.onChange = [this](double v){ proc.setADSR((float)knA.getValue(), (float)v, (float)knS.getValue(), (float)knR.getValue()); };
        knS.onChange = [this](double v){ proc.setADSR((float)knA.getValue(), (float)knD.getValue(), (float)v, (float)knR.getValue()); };
        knR.onChange = [this](double v){ proc.setADSR((float)knA.getValue(), (float)knD.getValue(), (float)knS.getValue(), (float)v); };
        kb.onNote = [this](int note, bool down){ if (down) { applyCapturedNote(note); send(juce::MidiMessage::noteOn(1, note, 1.0f)); } else { send(juce::MidiMessage::noteOff(1, note)); } };
        addAndMakeVisible(msbKn);
        addAndMakeVisible(lsbKn);
        addAndMakeVisible(pcKn);
        addAndMakeVisible(instName);
        addAndMakeVisible(exportBtn);
        instName.setText("Instrument", juce::NotificationType::dontSendNotification);
        instName.setColour(juce::TextEditor::backgroundColourId, juce::Colour::fromRGB(35,38,42));
        instName.setColour(juce::TextEditor::textColourId, juce::Colours::white);
        exportBtn.setButtonText("Export .zxi");
        exportBtn.setColour(juce::TextButton::buttonColourId, juce::Colour::fromRGB(62,70,80));
        exportBtn.onClick = [this]{
            juce::PopupMenu m;
            auto cats = SoundLibrary::voiceZXCategories();
            for (int i = 0; i < cats.size(); ++i) m.addItem(i+1, cats[i]);
            m.showMenuAsync(juce::PopupMenu::Options(), [this, cats](int r){
                if (r <= 0) return; int idx = r - 1; juce::String cat = cats[idx];
                juce::String nm = instName.getText().trim(); if (nm.isEmpty()) nm = "Instrument";
                auto* tlc = getTopLevelComponent();
                juce::Rectangle<int> panelRect = tlc ? tlc->getLocalArea(this, getLocalBounds()) : juce::Rectangle<int>();
                CategoryPopup::open(this, cat, [this, cat, nm](int pc, const juce::String&, const juce::String&){
                    auto temp = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("zx_tmp_" + juce::String((int) juce::Time::getMillisecondCounter()) + ".zxi");
                    bool exported = proc.exportInstrument(temp, 0, 0, pc, nm);
                    if (exported) {
                        bool occupied = SoundLibrary::voiceFileFor(cat, pc).existsAsFile();
                        bool doReplace = true;
                        if (occupied) doReplace = juce::NativeMessageBox::showOkCancelBox(juce::MessageBoxIconType::QuestionIcon, "Replace", "Target slot occupied. Replace?", this, nullptr);
                        SoundLibrary::assignVoicePosition(cat, pc, nm, temp, juce::File(), doReplace);
                        temp.deleteFile();
                    }
                }, -1, -1, panelRect);
            });
        };
        addAndMakeVisible(zoomIn);
        addAndMakeVisible(zoomOut);
        zoomIn.setButtonText("+");
        zoomOut.setButtonText("-");
        zoomIn.setColour(juce::TextButton::buttonColourId, juce::Colour::fromRGB(55,60,68));
        zoomOut.setColour(juce::TextButton::buttonColourId, juce::Colour::fromRGB(55,60,68));
        zoomIn.onClick = [this]{ kb.setZoom(kb.getZoom() + 0.15f); resized(); };
        zoomOut.onClick = [this]{ kb.setZoom(kb.getZoom() - 0.15f); resized(); };
        // Range controls under keyboard (as children of canvas)
        canvas.addAndMakeVisible(keyRangeLbl); keyRangeLbl.setColour(juce::Label::textColourId, juce::Colours::white);
        keyRangeLbl.setText("Key Range:", juce::NotificationType::dontSendNotification);
        keyRangeLbl.setFont(juce::FontOptions(16.0f));
        canvas.addAndMakeVisible(velRangeLbl); velRangeLbl.setColour(juce::Label::textColourId, juce::Colours::white);
        velRangeLbl.setText("Vel Range:", juce::NotificationType::dontSendNotification);
        velRangeLbl.setFont(juce::FontOptions(16.0f));
        canvas.addAndMakeVisible(keyLowBtn); canvas.addAndMakeVisible(keyHighBtn);
        keyLowBtn.setClickingTogglesState(true); keyHighBtn.setClickingTogglesState(true);
        keyLowBtn.setColour(juce::TextButton::buttonColourId, juce::Colour::fromRGB(60,60,60));
        keyHighBtn.setColour(juce::TextButton::buttonColourId, juce::Colour::fromRGB(60,60,60));
        keyLowBtn.onClick = [this]{ capturingLow = keyLowBtn.getToggleState(); if (capturingLow) { keyHighBtn.setToggleState(false, juce::NotificationType::dontSendNotification); capturingHigh = false; } };
        keyHighBtn.onClick = [this]{ capturingHigh = keyHighBtn.getToggleState(); if (capturingHigh) { keyLowBtn.setToggleState(false, juce::NotificationType::dontSendNotification); capturingLow = false; } };
        canvas.addAndMakeVisible(velLowVal); canvas.addAndMakeVisible(velHighVal);
        velLowVal.setColour(juce::Label::textColourId, juce::Colours::white);
        velHighVal.setColour(juce::Label::textColourId, juce::Colours::white);
        velLowVal.setColour(juce::Label::backgroundColourId, juce::Colour::fromRGB(35,38,42));
        velHighVal.setColour(juce::Label::backgroundColourId, juce::Colour::fromRGB(35,38,42));
        velLowVal.setFont(juce::FontOptions(16.0f));
        velHighVal.setFont(juce::FontOptions(16.0f));
        velLowVal.setEditable(true, true, false); velHighVal.setEditable(true, true, false);
        velLowVal.onTextChange = [this]{ int v = juce::jlimit(0,127, velLowVal.getText().getIntValue()); applyVelLow(v); };
        velHighVal.onTextChange = [this]{ int v = juce::jlimit(0,127, velHighVal.getText().getIntValue()); applyVelHigh(v); };

        InstrumentHost::instance().setMidiInListener([this](const juce::MidiMessage& m){ if (m.isNoteOn()) applyCapturedNote(m.getNoteNumber()); });
        zones.editor = this;
        updateRangeDisplays();
    }
    void resized() override {
        auto area = getLocalBounds();
        auto top = area.removeFromTop(72);
        int kw = 140;
        
        knA.setBounds(top.removeFromLeft(kw).reduced(6));
        knD.setBounds(top.removeFromLeft(kw).reduced(6));
        knS.setBounds(top.removeFromLeft(kw).reduced(6));
        knR.setBounds(top.removeFromLeft(kw).reduced(6));
        msbKn.setBounds(top.removeFromLeft(kw).reduced(6));
        lsbKn.setBounds(top.removeFromLeft(kw).reduced(6));
        pcKn.setBounds(top.removeFromLeft(kw).reduced(6));
        instName.setBounds(top.removeFromLeft(kw*2).reduced(6));
        exportBtn.setBounds(top.removeFromLeft(120).reduced(6));
        // assignBtn removed
        status.setBounds(top.removeFromLeft(kw * 5));
        status.proc = &proc; status.zonesView = &zones; status.attach(); status.refresh();
        int kbH = 90;
        int sbH = 18;
        int ctrlH = 44;
        auto rightPanel = area.removeFromRight(64);
        viewport.setBounds(area);
        int vpW = viewport.getWidth();
        int vpH = viewport.getHeight();
        int contentW = (int) std::round((double) vpW * (double) kb.getZoom());
        canvas.setSize(contentW, vpH);
        auto kbArea = juce::Rectangle<int>(0, vpH - kbH - sbH - ctrlH, contentW, kbH);
        kb.setBounds(kbArea);
        auto gridArea = juce::Rectangle<int>(0, 0, contentW, vpH - kbH - sbH - ctrlH);
        grid.setBounds(gridArea);
        zones.setBounds(gridArea);
        zones.kb = &kb;
        dropHint.setBounds(gridArea);
        grid.repaint();
        zoomIn.setBounds(rightPanel.removeFromTop(28).reduced(6));
        zoomOut.setBounds(rightPanel.removeFromTop(28).reduced(6));
        auto ctrl = juce::Rectangle<int>(0, vpH - sbH - ctrlH, contentW, ctrlH).reduced(6);
        auto r1 = ctrl.removeFromLeft(280);
        keyRangeLbl.setBounds(r1.removeFromLeft(110));
        keyLowBtn.setBounds(r1.removeFromLeft(85));
        keyHighBtn.setBounds(r1.removeFromLeft(85));
        auto r2 = ctrl.removeFromLeft(280);
        velRangeLbl.setBounds(r2.removeFromLeft(110));
        velLowVal.setBounds(r2.removeFromLeft(85));
        velHighVal.setBounds(r2.removeFromLeft(85));
    }
    bool isInterestedInFileDrag(const juce::StringArray& files) override {
        for (auto& f : files) { juce::File ff(f); auto ext = ff.getFileExtension().toLowerCase(); if (ext == ".wav" || ext == ".aif" || ext == ".aiff") return true; }
        return false;
    }
    void filesDropped(const juce::StringArray& files, int, int) override {
        int startKey = kb.getSelectedNote() >= 0 ? kb.getSelectedNote() : 0;
        struct Item { juce::File file; int root; };
        juce::Array<Item> items;
        for (auto& f : files) { juce::File ff(f); auto ext = ff.getFileExtension().toLowerCase(); if (ext == ".wav" || ext == ".aif" || ext == ".aiff") {
            juce::String nm = ff.getFileName().toLowerCase();
            juce::StringArray notes { "c","c#","db","d","d#","eb","e","f","f#","gb","g","g#","ab","a","a#","bb","b" };
            int inferred = -1;
            for (int octave = 0; octave <= 9 && inferred < 0; ++octave) {
                for (int i = 0; i < notes.size(); ++i) {
                    juce::String token = notes[i] + juce::String(octave);
                    if (nm.contains(token)) {
                        int base = 0; juce::String n = notes[i];
                        if (n == "c") base = 0; else if (n == "c#" || n=="db") base = 1; else if (n=="d") base = 2; else if (n=="d#" || n=="eb") base = 3; else if (n=="e") base = 4; else if (n=="f") base = 5; else if (n=="f#" || n=="gb") base = 6; else if (n=="g") base = 7; else if (n=="g#" || n=="ab") base = 8; else if (n=="a") base = 9; else if (n=="a#" || n=="bb") base = 10; else if (n=="b") base = 11;
                        inferred = juce::jlimit(0,127, base + octave * 12 + 12);
                        break;
                    }
                }
            }
            if (inferred < 0) {
                int numCandidate = -1;
                juce::Array<int> nums; int current = -1; bool inDigits = false;
                for (int ci = 0; ci < nm.length(); ++ci) {
                    juce::juce_wchar ch = nm[ci];
                    if (ch >= '0' && ch <= '9') {
                        if (!inDigits) { current = 0; inDigits = true; }
                        current = current * 10 + (int)(ch - '0');
                    } else {
                        if (inDigits) { nums.add(current); inDigits = false; current = -1; }
                    }
                }
                if (inDigits) nums.add(current);
                for (int ni = nums.size() - 1; ni >= 0; --ni) {
                    int v = nums[ni]; if (v >= 0 && v <= 127) { numCandidate = v; break; }
                }
                if (numCandidate >= 0) inferred = numCandidate;
            }
            items.add({ ff, inferred });
        } }
        if (items.size() == 1) {
            int rootCandidate = items[0].root >= 0 ? items[0].root : (kb.getSelectedNote() >= 0 ? kb.getSelectedNote() : 60);
            proc.loadSample(items[0].file, rootCandidate, 0, 127);
        } else if (items.size() > 1) {
            struct Cmp { int compareElements(const Item& a, const Item& b) const { return (a.root < b.root) ? -1 : ((a.root > b.root) ? 1 : 0); } } cmp;
            items.sort(cmp);
            int k = startKey;
            for (int i = 0; i < items.size(); ++i) {
                Item it = items[i];
                if (it.root >= 0) proc.addSample(it.file, it.root);
                else { proc.addSample(it.file, k); k = juce::jmin(127, k + 1); }
            }
        }
        zones.repaint();
    }
    bool keyPressed(const juce::KeyPress& k) override {
        if (k == juce::KeyPress('a', juce::ModifierKeys::commandModifier, 0)) { zones.selectAll(); return true; }
        if (k.getKeyCode() == juce::KeyPress::deleteKey || k.getKeyCode() == juce::KeyPress::backspaceKey) { zones.deleteSelection(); return true; }
        return false;
    }
private:
    void send(const juce::MidiMessage& m) {
        InstrumentHost::instance().init();
        InstrumentHost::instance().sendRawMidi(m);
    }
    void updateRangeDisplays() {
        int idx = zones.selected >= 0 ? zones.selected : (zones.selection.isEmpty() ? -1 : zones.selection[0]);
        if (idx < 0 || idx >= (int) proc.getZones().size()) {
            keyLowBtn.setButtonText("-"); keyHighBtn.setButtonText("-");
            velLowVal.setText("0", juce::NotificationType::dontSendNotification);
            velHighVal.setText("127", juce::NotificationType::dontSendNotification);
            return;
        }
        const auto& z = proc.getZones()[(size_t) idx];
        keyLowBtn.setButtonText(juce::MidiMessage::getMidiNoteName(z.lowKey, true, true, 4));
        keyHighBtn.setButtonText(juce::MidiMessage::getMidiNoteName(z.highKey, true, true, 4));
        velLowVal.setText(juce::String(z.velLow), juce::NotificationType::dontSendNotification);
        velHighVal.setText(juce::String(z.velHigh), juce::NotificationType::dontSendNotification);
    }
    void applyCapturedNote(int note) {
        int idx = zones.selected >= 0 ? zones.selected : (zones.selection.isEmpty() ? -1 : zones.selection[0]);
        if (idx < 0) return;
        auto& z = const_cast<MappingSamplerProcessor::Zone&>(proc.getZones()[(size_t) idx]);
        int low = z.lowKey; int high = z.highKey;
        if (capturingLow) {
            low = juce::jlimit(0,127,note);
            if (low > high) high = low;
            proc.setZoneRange(idx, low, high);
            capturingLow = false; keyLowBtn.setToggleState(false, juce::NotificationType::dontSendNotification);
        } else if (capturingHigh) {
            high = juce::jlimit(0,127,note);
            if (high < low) low = high;
            proc.setZoneRange(idx, low, high);
            capturingHigh = false; keyHighBtn.setToggleState(false, juce::NotificationType::dontSendNotification);
        }
        zones.repaint(); updateRangeDisplays();
    }
    void applyVelLow(int v) {
        int idx = zones.selected >= 0 ? zones.selected : (zones.selection.isEmpty() ? -1 : zones.selection[0]);
        if (idx < 0) return;
        auto& z = const_cast<MappingSamplerProcessor::Zone&>(proc.getZones()[(size_t) idx]);
        int vl = juce::jlimit(0,127,v); int vh = z.velHigh; if (vl > vh) vh = vl;
        proc.setZoneVelocity(idx, vl, vh);
        zones.repaint(); updateRangeDisplays();
    }
    void applyVelHigh(int v) {
        int idx = zones.selected >= 0 ? zones.selected : (zones.selection.isEmpty() ? -1 : zones.selection[0]);
        if (idx < 0) return;
        auto& z = const_cast<MappingSamplerProcessor::Zone&>(proc.getZones()[(size_t) idx]);
        int vl = z.velLow; int vh = juce::jlimit(0,127,v); if (vh < vl) vl = vh;
        proc.setZoneVelocity(idx, vl, vh);
        zones.repaint(); updateRangeDisplays();
    }
    MappingSamplerProcessor& proc;
    juce::Label keyRangeLbl, velRangeLbl;
    juce::TextButton keyLowBtn, keyHighBtn;
    juce::Label velLowVal, velHighVal;
    bool capturingLow { false }, capturingHigh { false };
    UpDownKnob knRoot { "Root", 0, 127, 1, 60, true, [](double v){ return juce::MidiMessage::getMidiNoteName((int) std::round(v), true, true, 4); } };
    UpDownKnob knLow  { "Low",  0, 127, 1, 36, true };
    UpDownKnob knHigh { "High", 0, 127, 1, 96, true };
    UpDownKnob knA    { "Attack", 0.001, 2.0, 0.01, 0.01, false };
    UpDownKnob knD    { "Decay",  0.001, 2.0, 0.01, 0.10, false };
    UpDownKnob knS    { "Sustain",0.0, 1.0, 0.01, 0.80, false };
    UpDownKnob knR    { "Release",0.001, 3.0, 0.02, 0.20, false };
    UpDownKnob msbKn  { "MSB", 0, 127, 1, 0, true };
    UpDownKnob lsbKn  { "LSB", 0, 127, 1, 0, true };
    UpDownKnob pcKn   { "PC",  0, 127, 1, 0, true };
    juce::TextEditor instName;
    juce::TextButton exportBtn;
    juce::TextButton assignBtn;
    juce::File lastExportFile;
    LinearKeyboard kb;
    juce::Viewport viewport;
    juce::Component canvas;
    std::unique_ptr<juce::FileChooser> chooser;
    juce::Label dropHint;
    juce::TextButton zoomIn, zoomOut;
    struct ZonesView;
    struct VisualGrid : public juce::Component {
        LinearKeyboard* kb { nullptr };
        void paint(juce::Graphics& g) override {
            auto a = getLocalBounds().toFloat();
            g.setColour(juce::Colour::fromRGB(28,28,28)); g.fillRect(a);
            g.setColour(juce::Colour::fromRGB(52,52,52)); g.drawRect(a);
            float keyW = a.getWidth() / 128.0f;
            int lastX = std::numeric_limits<int>::min();
            for (int n = 0; n <= 128; ++n) {
                int x = (int) std::lround(a.getX() + keyW * (double) n);
                if (x == lastX) continue; lastX = x;
                g.setColour((n % 12 == 0) ? juce::Colour::fromRGB(64,64,64) : juce::Colour::fromRGB(46,46,46));
                g.drawVerticalLine(x, a.getY(), a.getBottom());
            }
            int rows = 16;
            float rowH = a.getHeight() / (float) rows;
            int lastY = std::numeric_limits<int>::min();
            for (int r = 1; r < rows; ++r) {
                int y = (int) std::lround(a.getY() + rowH * (double) r);
                if (y == lastY) continue; lastY = y;
                g.setColour((r % 4 == 0) ? juce::Colour::fromRGB(60,60,60) : juce::Colour::fromRGB(46,46,46));
                g.drawHorizontalLine(y, a.getX(), a.getRight());
            }
        }
    } grid;
    struct StatusBar : public juce::Component {
        MappingSamplerProcessor* proc { nullptr };
        ZonesView* zonesView { nullptr };
        UpDownKnob rRoot { "Root", 0, 127, 1, 60, true, [](double v){ return juce::MidiMessage::getMidiNoteName((int) std::round(v), true, true, 4); } };
        UpDownKnob kLow  { "Low",  0, 127, 1, 36, true };
        UpDownKnob kHigh { "High", 0, 127, 1, 96, true };
        UpDownKnob vLow  { "Vel Low", 1, 127, 1, 1, true };
        UpDownKnob vHigh { "Vel High", 1, 127, 1, 127, true };
        void attach() {
            this->addAndMakeVisible(rRoot); this->addAndMakeVisible(kLow); this->addAndMakeVisible(kHigh); this->addAndMakeVisible(vLow); this->addAndMakeVisible(vHigh);
            rRoot.onChange = [this](double v){ int idx = zonesView->selected; if (proc && idx >= 0) proc->setZoneRoot(idx, (int) v); };
            kLow.onChange = [this](double v){ if (!proc || !zonesView) return; if (!zonesView->selection.isEmpty()) { for (int i = 0; i < zonesView->selection.size(); ++i) { int idx = zonesView->selection[i]; proc->setZoneRange(idx, (int) v, (int) kHigh.getValue()); } } else { int idx = zonesView->selected; if (idx >= 0) proc->setZoneRange(idx, (int) v, (int) kHigh.getValue()); } };
            kHigh.onChange = [this](double v){ if (!proc || !zonesView) return; if (!zonesView->selection.isEmpty()) { for (int i = 0; i < zonesView->selection.size(); ++i) { int idx = zonesView->selection[i]; proc->setZoneRange(idx, (int) kLow.getValue(), (int) v); } } else { int idx = zonesView->selected; if (idx >= 0) proc->setZoneRange(idx, (int) kLow.getValue(), (int) v); } };
            vLow.onChange = [this](double v){ if (!proc || !zonesView) return; if (!zonesView->selection.isEmpty()) { for (int i = 0; i < zonesView->selection.size(); ++i) { int idx = zonesView->selection[i]; proc->setZoneVelocity(idx, (int) v, (int) vHigh.getValue()); } } else { int idx = zonesView->selected; if (idx >= 0) proc->setZoneVelocity(idx, (int) v, (int) vHigh.getValue()); } };
            vHigh.onChange = [this](double v){ if (!proc || !zonesView) return; if (!zonesView->selection.isEmpty()) { for (int i = 0; i < zonesView->selection.size(); ++i) { int idx = zonesView->selection[i]; proc->setZoneVelocity(idx, (int) vLow.getValue(), (int) v); } } else { int idx = zonesView->selected; if (idx >= 0) proc->setZoneVelocity(idx, (int) vLow.getValue(), (int) v); } };
        }
        void refresh() {
            if (!proc || zonesView->selected < 0) return; auto& z = proc->getZones()[(size_t) zonesView->selected];
            rRoot.setValue(z.rootKey); kLow.setValue(z.lowKey); kHigh.setValue(z.highKey); vLow.setValue(z.velLow); vHigh.setValue(z.velHigh);
        }
        void resized() override {
            auto r = getLocalBounds(); int w = 140; rRoot.setBounds(r.removeFromLeft(w).reduced(4)); kLow.setBounds(r.removeFromLeft(w).reduced(4)); kHigh.setBounds(r.removeFromLeft(w).reduced(4)); vLow.setBounds(r.removeFromLeft(w).reduced(4)); vHigh.setBounds(r.removeFromLeft(w).reduced(4));
        }
    } status;
    struct ZonesView : public juce::Component {
        MappingSamplerProcessor* proc { nullptr };
        LinearKeyboard* kb { nullptr };
        MappingSamplerEditor* editor { nullptr };
        float yFromVel(int v, float h) { return juce::jmap((float) v, 127.0f, 1.0f, 0.0f, h); }
        int velFromY(float y, float h) { int v = (int) juce::jmap(y, 0.0f, h, 127.0f, 1.0f); return juce::jlimit(1,127,v); }
        enum Edge { none, inside, left, right, top, bottom };
        int selected { -1 };
        juce::Array<int> selection;
        Edge dragEdge { none };
        int startNote { 0 }, startVel { 0 };
        juce::Point<int> startPos;
        bool previewActive { false };
        int previewDxCells { 0 };
        int previewTopVel { -1 };
        int previewBottomVel { -1 };
        juce::Array<int> startLows;
        
        // existing startLows declaration moved above
        
        void select(int idx) { selected = idx; repaint(); if (kb && idx >= 0) { auto& z = proc->getZones()[(size_t) idx]; int c = (z.lowKey + z.highKey) / 2; kb->setSelectedNote(c); } if (editor) editor->updateRangeDisplays(); }
        void selectAll() { selection.clear(); auto& zs = proc->getZones(); for (int i = 0; i < (int)zs.size(); ++i) selection.add(i); if (!selection.isEmpty()) selected = selection[0]; repaint(); }
        void deleteSelection() { if (!proc) return; selection.sort(); for (int i = selection.size()-1; i >= 0; --i) proc->removeZone(selection[i]); selection.clear(); selected = -1; repaint(); }
        void paint(juce::Graphics& g) override {
            if (!proc) return; auto a = getLocalBounds().toFloat(); float cell = a.getWidth() / 128.0f;
            auto& zs = proc->getZones();
            for (int i = 0; i < (int)zs.size(); ++i) {
                auto& z = zs[(size_t)i];
                float x = a.getX() + cell * (float) z.lowKey;
                float w = cell * (float) (z.highKey - z.lowKey + 1);
                float y1 = yFromVel(z.velHigh, a.getHeight()); float y2 = yFromVel(z.velLow, a.getHeight());
                juce::Rectangle<float> rr(x, y1, juce::jmax(1.0f, w), juce::jmax(4.0f, y2 - y1));
                bool isSel = selection.contains(i);
                auto col = (isSel ? juce::Colours::yellow.withAlpha(0.75f) : juce::Colour::fromRGB(80,120,200).withAlpha(0.75f));
                g.setColour(col); g.fillRect(rr);
                g.setColour(juce::Colour::fromRGB(160,190,240)); g.drawRect(rr, 1.0f);
                g.setColour(juce::Colours::white); g.setFont(juce::FontOptions(12.0f));
                auto root = juce::MidiMessage::getMidiNoteName(z.rootKey, true, true, 4);
                g.drawText(z.file.getFileName() + "  Root:" + root, rr.reduced(4), juce::Justification::centred);
            }
            if (previewActive && !selection.isEmpty()) {
                g.setColour(juce::Colour::fromRGB(90,200,240).withAlpha(0.35f));
                for (int k = 0; k < selection.size(); ++k) {
                    auto& z = zs[(size_t) selection[k]];
                    int newLow = juce::jlimit(0,127, z.lowKey + previewDxCells);
                    int newHigh = juce::jlimit(0,127, newLow + (z.highKey - z.lowKey));
                    int vh = (previewTopVel >= 0 ? previewTopVel : z.velHigh);
                    int vl = (previewBottomVel >= 0 ? previewBottomVel : z.velLow);
                    float x = a.getX() + cell * (float) newLow;
                    float w = cell * (float) (newHigh - newLow + 1);
                    float y1 = yFromVel(vh, a.getHeight()); float y2 = yFromVel(vl, a.getHeight());
                    juce::Rectangle<float> ghost(x, y1, juce::jmax(1.0f, w), juce::jmax(4.0f, y2 - y1));
                    g.fillRect(ghost);
                    g.setColour(juce::Colour::fromRGB(90,200,240)); g.drawRect(ghost, 1.0f);
                }
            }
        }
        void mouseMove(const juce::MouseEvent& e) override {
            if (!proc) { setMouseCursor(juce::MouseCursor::NormalCursor); return; }
            auto a = getLocalBounds().toFloat(); float cell = a.getWidth() / 128.0f; auto& zs = proc->getZones();
            juce::MouseCursor cur = juce::MouseCursor::NormalCursor;
            for (int i = 0; i < (int)zs.size(); ++i) {
                auto& z = zs[(size_t)i]; float x = a.getX() + cell * (float) z.lowKey; float w = cell * (float) (z.highKey - z.lowKey + 1);
                float y1 = yFromVel(z.velHigh, a.getHeight()); float y2 = yFromVel(z.velLow, a.getHeight()); juce::Rectangle<float> rr(x, y1, juce::jmax(1.0f, w), juce::jmax(4.0f, y2 - y1));
                const int edgeX = 4, edgeY = 3;
                if (std::abs(e.x - rr.getX()) < edgeX || std::abs(e.x - rr.getRight()) < edgeX) { cur = juce::MouseCursor::LeftRightResizeCursor; break; }
                if (std::abs(e.y - rr.getY()) < edgeY || std::abs(e.y - rr.getBottom()) < edgeY) { cur = juce::MouseCursor::UpDownResizeCursor; break; }
            }
            setMouseCursor(cur);
        }
        void mouseDown(const juce::MouseEvent& e) override {
            if (!proc) return; auto a = getLocalBounds().toFloat(); float cell = a.getWidth() / 128.0f; auto& zs = proc->getZones(); dragEdge = none; selected = -1;
            for (int i = 0; i < (int)zs.size(); ++i) {
                auto& z = zs[(size_t)i]; float x = a.getX() + cell * (float) z.lowKey; float w = cell * (float) (z.highKey - z.lowKey + 1);
                float y1 = yFromVel(z.velHigh, a.getHeight()); float y2 = yFromVel(z.velLow, a.getHeight()); juce::Rectangle<float> rr(x, y1, juce::jmax(1.0f, w), juce::jmax(4.0f, y2 - y1));
                if (rr.contains(e.position)) {
                    if (e.mods.isCommandDown()) { if (selection.contains(i)) selection.removeFirstMatchingValue(i); else selection.add(i); selected = i; }
                    else { if (!selection.isEmpty() && selection.contains(i)) { selected = i; } else { selection.clear(); selection.add(i); select(i); } }
                    const int edgeX = 4, edgeY = 3;
                    float mx = e.x; float my = e.y; dragEdge = inside; if (std::abs(mx - rr.getX()) < edgeX) dragEdge = left; else if (std::abs(mx - rr.getRight()) < edgeX) dragEdge = right; if (std::abs(my - rr.getY()) < edgeY) dragEdge = top; else if (std::abs(my - rr.getBottom()) < edgeY) dragEdge = bottom; startPos = { e.x, e.y }; startNote = z.lowKey; startVel = z.velLow; startLows.clear(); for (int k = 0; k < selection.size(); ++k) { auto& zz = zs[(size_t) selection[k]]; startLows.add(zz.lowKey); } previewActive = true; previewDxCells = 0; previewTopVel = -1; previewBottomVel = -1; break; }
            }
            repaint();
        }
        void clampNoOverlapRange(int index, int low, int high) {
            auto& zs = proc->getZones(); low = juce::jlimit(0,127,low); high = juce::jlimit(low,127,high);
            for (int i = 0; i < (int)zs.size(); ++i) if (i != index) {
                auto& o = zs[(size_t)i]; bool velOverlap = !(o.velHigh < zs[(size_t)index].velLow || o.velLow > zs[(size_t)index].velHigh);
                if (velOverlap) {
                    if (low < o.highKey && high > o.lowKey) {
                        if (low < o.lowKey) high = juce::jmin(high, o.lowKey);
                        else low = juce::jmax(low, o.highKey);
                    }
                }
            }
            proc->setZoneRange(index, low, high);
        }
        void mouseDrag(const juce::MouseEvent& e) override {
            if (!proc || selected < 0) return; auto a = getLocalBounds().toFloat(); float cell = a.getWidth() / 128.0f; auto& z = const_cast<MappingSamplerProcessor::Zone&>(proc->getZones()[(size_t) selected]);
            int dxCells = (int) std::floor((e.x - (float) startPos.x) / cell);
            previewDxCells = dxCells;
            int v = velFromY((float) e.y, a.getHeight());
            if (dragEdge == left) { int newLow = juce::jmin(z.highKey, z.lowKey + dxCells); clampNoOverlapRange(selected, newLow, z.highKey); }
            else if (dragEdge == right) { int newHigh = juce::jmax(z.lowKey, z.highKey + dxCells); clampNoOverlapRange(selected, z.lowKey, newHigh); }
            else if (dragEdge == top) {
                int newVh = juce::jmax(z.velLow, v);
                for (int k = 0; k < selection.size(); ++k) {
                    int idx = selection[k]; auto& zz = const_cast<MappingSamplerProcessor::Zone&>(proc->getZones()[(size_t) idx]); int vh = juce::jmax(zz.velLow, newVh); proc->setZoneVelocity(idx, zz.velLow, vh);
                }
            }
            else if (dragEdge == bottom) {
                int newVl = juce::jmin(z.velHigh, v);
                for (int k = 0; k < selection.size(); ++k) {
                    int idx = selection[k]; auto& zz = const_cast<MappingSamplerProcessor::Zone&>(proc->getZones()[(size_t) idx]); int vl = juce::jmin(zz.velHigh, newVl); proc->setZoneVelocity(idx, vl, zz.velHigh);
                }
            }
            else {
                for (int k = 0; k < selection.size(); ++k) {
                    int idx = selection[k]; auto& zz = const_cast<MappingSamplerProcessor::Zone&>(proc->getZones()[(size_t) idx]); int width = zz.highKey - zz.lowKey; int baseLow = startLows[k]; int newLow = juce::jlimit(0,127,baseLow + dxCells); int newHigh = juce::jlimit(0,127,newLow + width); clampNoOverlapRange(idx, newLow, newHigh);
                }
            }
            repaint();
        }
        void mouseUp(const juce::MouseEvent&) override { dragEdge = none; previewActive = false; previewTopVel = previewBottomVel = -1; previewDxCells = 0; repaint(); if (proc) proc->rebuild(); if (editor) editor->updateRangeDisplays(); }
        void mouseDoubleClick(const juce::MouseEvent& e) override { if (!proc) return; if (selection.isEmpty() && selected >= 0) selection.add(selected); deleteSelection(); }
    } zones;
    
};

MappingSamplerProcessor::MappingSamplerProcessor() : juce::AudioProcessor(BusesProperties().withInput("in", juce::AudioChannelSet::stereo(), true).withOutput("out", juce::AudioChannelSet::stereo(), true)) {
    synth.clearVoices();
    for (int i = 0; i < 128; ++i) synth.addVoice(new juce::SamplerVoice());
    formatManager.registerBasicFormats();
}
const juce::String MappingSamplerProcessor::getName() const { return "MappingSampler"; }
void MappingSamplerProcessor::prepareToPlay(double sr, int bs) { synth.setCurrentPlaybackSampleRate(sr); }
void MappingSamplerProcessor::releaseResources() {}
bool MappingSamplerProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const { return layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo() && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo(); }
void MappingSamplerProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) {
    buffer.clear();
    juce::MidiBuffer filtered;
    for (auto it = midi.findNextSamplePosition(0); it != midi.end(); ++it) {
        const auto metadata = *it; auto msg = metadata.getMessage();
        if (msg.isNoteOn()) {
            int note = msg.getNoteNumber(); int vel = (int) msg.getVelocity(); int channel = 1;
            for (int i = 0; i < (int)zones.size(); ++i) {
                auto& z = zones[(size_t)i];
                if (note >= z.lowKey && note <= z.highKey && vel >= z.velLow && vel <= z.velHigh) { channel = (i % 16) + 1; break; }
            }
            currentChannel = channel;
            msg.setChannel(channel);
            // Маркирай канала като активен за този тон
            if (channel >= 1 && channel <= 16) {
                noteActiveMask[(size_t) note] |= (uint16_t)(1u << (channel - 1));
            }
            filtered.addEvent(msg, metadata.samplePosition);
        } else if (msg.isNoteOff()) {
            int note = msg.getNoteNumber();
            uint16_t mask = noteActiveMask[(size_t) note];
            if (sustainOn) {
                pendingOffMask[(size_t) note] |= mask;
                noteActiveMask[(size_t) note] &= (uint16_t) ~mask;
            } else {
                for (int ch = 1; ch <= 16; ++ch) {
                    if (mask & (uint16_t)(1u << (ch - 1))) {
                        auto off = juce::MidiMessage::noteOff(ch, note);
                        filtered.addEvent(off, metadata.samplePosition);
                    }
                }
                noteActiveMask[(size_t) note] &= (uint16_t) ~mask;
            }
        } else if (msg.isController()) {
            int num = msg.getControllerNumber(); int val = msg.getControllerValue();
            if (num == 64) {
                sustainOn = (val >= 64);
                if (!sustainOn) {
                    for (int n = 0; n < 128; ++n) {
                        uint16_t pmask = pendingOffMask[(size_t) n];
                        if (pmask != 0) {
                            for (int ch = 1; ch <= 16; ++ch) {
                                if (pmask & (uint16_t)(1u << (ch - 1))) {
                                    auto off = juce::MidiMessage::noteOff(ch, n);
                                    filtered.addEvent(off, metadata.samplePosition);
                                }
                            }
                            pendingOffMask[(size_t) n] = 0;
                        }
                    }
                }
            } else if (num == 7 || num == 11) {
                masterGain = juce::jlimit(0.0f, 1.0f, (float) val / 127.0f);
            }
            filtered.addEvent(msg, metadata.samplePosition);
        } else if (msg.isPitchWheel()) {
            filtered.addEvent(msg, metadata.samplePosition);
        } else {
            filtered.addEvent(msg, metadata.samplePosition);
        }
    }
    synth.renderNextBlock(buffer, filtered, 0, buffer.getNumSamples());
    int ns = buffer.getNumSamples(); int nc = buffer.getNumChannels();
    for (int ch = 0; ch < nc; ++ch) {
        float* d = buffer.getWritePointer(ch);
        for (int i = 0; i < ns; ++i) {
            float s = d[i] * masterGain;
            if (s > 0.98f) s = 0.98f; else if (s < -0.98f) s = -0.98f;
            d[i] = s;
        }
    }
}
bool MappingSamplerProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* MappingSamplerProcessor::createEditor() { return new MappingSamplerEditor(*this); }
bool MappingSamplerProcessor::acceptsMidi() const { return true; }
bool MappingSamplerProcessor::producesMidi() const { return false; }
double MappingSamplerProcessor::getTailLengthSeconds() const { return 0.0; }
int MappingSamplerProcessor::getNumPrograms() { return 1; }
int MappingSamplerProcessor::getCurrentProgram() { return 0; }
void MappingSamplerProcessor::setCurrentProgram(int) {}
const juce::String MappingSamplerProcessor::getProgramName(int) { return {}; }
void MappingSamplerProcessor::changeProgramName(int, const juce::String&) {}
void MappingSamplerProcessor::getStateInformation(juce::MemoryBlock&) {}
void MappingSamplerProcessor::setStateInformation(const void*, int) {}
void MappingSamplerProcessor::loadSample(const juce::File& file, int rootKey, int lowKey, int highKey) {
    auto reader = std::unique_ptr<juce::AudioFormatReader>(formatManager.createReaderFor(file));
    if (!reader) return;
    readers.clear();
    readers.push_back(std::move(reader));
    zones.clear();
    auto parseRoot = [&](const juce::String& name){
        juce::String nm = name.toLowerCase();
        juce::StringArray notes { "c","c#","db","d","d#","eb","e","f","f#","gb","g","g#","ab","a","a#","bb","b" };
        for (int octave = 0; octave <= 9; ++octave) {
            for (int i = 0; i < notes.size(); ++i) {
                juce::String token = notes[i] + juce::String(octave);
                int pos = nm.indexOf(token);
                if (pos >= 0) {
                    int base = 0; juce::String n = notes[i];
                    if (n == "c") base = 0; else if (n == "c#" || n=="db") base = 1; else if (n=="d") base = 2; else if (n=="d#" || n=="eb") base = 3; else if (n=="e") base = 4; else if (n=="f") base = 5; else if (n=="f#" || n=="gb") base = 6; else if (n=="g") base = 7; else if (n=="g#" || n=="ab") base = 8; else if (n=="a") base = 9; else if (n=="a#" || n=="bb") base = 10; else if (n=="b") base = 11;
                    return juce::jlimit(0,127, base + octave * 12 + 12);
                }
            }
        }
        int numCandidate = -1; juce::Array<int> nums; int current = -1; bool inDigits = false;
        for (int ci = 0; ci < nm.length(); ++ci) {
            juce::juce_wchar ch = nm[ci];
            if (ch >= '0' && ch <= '9') { if (!inDigits) { current = 0; inDigits = true; } current = current * 10 + (int)(ch - '0'); }
            else { if (inDigits) { nums.add(current); inDigits = false; current = -1; } }
        }
        if (inDigits) nums.add(current);
        for (int ni = nums.size() - 1; ni >= 0; --ni) { int v = nums[ni]; if (v >= 0 && v <= 127) { numCandidate = v; break; } }
        if (numCandidate >= 0) return numCandidate;
        return -1;
    };
    int rk = rootKey;
    int inferred = parseRoot(file.getFileName());
    if (inferred >= 0) rk = inferred;
    Zone z; z.file = file; z.rootKey = juce::jlimit(0,127,rk); z.lowKey = juce::jlimit(0,127,lowKey); z.highKey = juce::jlimit(0,127,highKey); zones.push_back(z);
    rebuild();
}

void MappingSamplerProcessor::addSample(const juce::File& file, int rootKey) {
    auto reader = std::unique_ptr<juce::AudioFormatReader>(formatManager.createReaderFor(file));
    if (!reader) return;
    readers.push_back(std::move(reader));
    auto nextFree = [&](int start){
        int k = juce::jlimit(0,127,start);
        while (k <= 127) {
            bool occupied = false;
            for (auto& o : zones) if (k >= o.lowKey && k <= o.highKey) { occupied = true; break; }
            if (!occupied) return k;
            ++k;
        }
        return juce::jlimit(0,127,start);
    };
    int useKey = nextFree(rootKey);
    Zone z; z.file = file; z.rootKey = useKey; z.lowKey = useKey; z.highKey = useKey; zones.push_back(z);
    rebuild();
}
void MappingSamplerProcessor::setADSR(float a, float d, float s, float r) {
    adsrParams = { a, d, s, r };
    for (int i = 0; i < synth.getNumSounds(); ++i) {
        if (auto* snd = dynamic_cast<juce::SamplerSound*>(synth.getSound(i).get())) snd->setEnvelopeParameters(adsrParams);
    }
}

void MappingSamplerProcessor::setZoneRange(int index, int lowKey, int highKey) {
    if (index < 0 || index >= (int)zones.size()) return;
    zones[index].lowKey = juce::jlimit(0,127,lowKey);
    zones[index].highKey = juce::jlimit(0,127,highKey);
}
void MappingSamplerProcessor::setZoneRoot(int index, int rootKey) {
    if (index < 0 || index >= (int)zones.size()) return;
    zones[index].rootKey = juce::jlimit(0,127,rootKey);
    rebuild();
}
void MappingSamplerProcessor::removeZone(int index) {
    if (index < 0 || index >= (int)zones.size()) return;
    zones.erase(zones.begin() + index);
    rebuild();
}
void MappingSamplerProcessor::setZoneVelocity(int index, int velLow, int velHigh) {
    if (index < 0 || index >= (int)zones.size()) return;
    zones[index].velLow = juce::jlimit(0,127,velLow);
    zones[index].velHigh = juce::jlimit(0,127,velHigh);
}
void MappingSamplerProcessor::setZoneGain(int index, float gainLinear) { if (index < 0 || index >= (int)zones.size()) return; zones[index].gain = juce::jlimit(0.0f, 4.0f, gainLinear); }
void MappingSamplerProcessor::setZonePan(int index, float panMinus1To1) { if (index < 0 || index >= (int)zones.size()) return; zones[index].pan = juce::jlimit(-1.0f, 1.0f, panMinus1To1); }
void MappingSamplerProcessor::setZoneTune(int index, float centsMinus100To100) { if (index < 0 || index >= (int)zones.size()) return; zones[index].tuneCents = juce::jlimit(-100.0f, 100.0f, centsMinus100To100); }
namespace {
struct ZoneSound : public juce::SamplerSound {
    int channel;
    ZoneSound(const juce::String& name, juce::AudioFormatReader& source, const juce::BigInteger& midiNotes, int midiNoteForNormalPitch, double attack, double release, double maxSampleLengthSeconds, int ch)
        : juce::SamplerSound(name, source, midiNotes, midiNoteForNormalPitch, attack, release, maxSampleLengthSeconds), channel(ch) {}
    bool appliesToChannel(int midiChannel) override { juce::ignoreUnused(midiChannel); return true; }
};
}
void MappingSamplerProcessor::rebuild() {
    synth.clearSounds();
    for (size_t i = 0; i < zones.size(); ++i) {
        auto& z = zones[i];
        juce::AudioFormatReader* r = nullptr;
        if (i < readers.size()) r = readers[i].get();
        if (!r) continue;
        juce::BigInteger notes; for (int n = z.lowKey; n <= z.highKey; ++n) notes.setBit(n);
        auto* sound = new ZoneSound(z.file.getFileName(), *r, notes, z.rootKey, adsrParams.attack, adsrParams.release, 60.0, (int)((i % 16) + 1));
        sound->setEnvelopeParameters(adsrParams);
        synth.addSound(sound);
    }
}

bool MappingSamplerProcessor::exportInstrument(const juce::File& destFile, int msb, int lsb, int pc, const juce::String& name) {
    juce::File parent = destFile.getParentDirectory(); if (!parent.exists()) parent.createDirectory();
    juce::FileOutputStream out(destFile);
    if (!out.openedOk()) return false;
    out.write("ZXI1", 4);
    out.writeInt(1);
    juce::DynamicObject::Ptr root(new juce::DynamicObject());
    root->setProperty("name", name);
    root->setProperty("msb", msb);
    root->setProperty("lsb", lsb);
    root->setProperty("pc", pc);
    juce::DynamicObject::Ptr adsr(new juce::DynamicObject());
    adsr->setProperty("a", adsrParams.attack);
    adsr->setProperty("d", adsrParams.decay);
    adsr->setProperty("s", adsrParams.sustain);
    adsr->setProperty("r", adsrParams.release);
    root->setProperty("adsr", adsr.get());
    juce::Array<juce::var> jzones;
    juce::HashMap<juce::String,int> sampleIndex;
    juce::Array<juce::File> sampleFiles;
    for (int i = 0; i < (int)zones.size(); ++i) {
        auto& z = zones[(size_t)i];
        int idx = -1; juce::String key = z.file.getFullPathName();
        if (sampleIndex.contains(key)) { idx = sampleIndex[key]; }
        else { idx = sampleFiles.size(); sampleFiles.add(z.file); sampleIndex.set(key, idx); }
        juce::DynamicObject::Ptr jo(new juce::DynamicObject());
        jo->setProperty("file", z.file.getFileName());
        jo->setProperty("sample", idx);
        jo->setProperty("root", z.rootKey);
        jo->setProperty("low", z.lowKey);
        jo->setProperty("high", z.highKey);
        jo->setProperty("vlow", z.velLow);
        jo->setProperty("vhigh", z.velHigh);
        jo->setProperty("gain", z.gain);
        jo->setProperty("pan", z.pan);
        jo->setProperty("tune", z.tuneCents);
        jzones.add(jo.get());
    }
    root->setProperty("zones", jzones);
    auto json = juce::JSON::toString(root.get());
    juce::MemoryBlock jmb(json.toRawUTF8(), (size_t) json.getNumBytesAsUTF8());
    out.writeInt64((int64) jmb.getSize());
    out.write(jmb.getData(), jmb.getSize());
    out.writeInt((int) sampleFiles.size());
    for (int i = 0; i < sampleFiles.size(); ++i) {
        juce::File f = sampleFiles[i]; juce::String nm = f.getFileName(); auto nm8 = nm.toRawUTF8(); int nlen = (int) strlen(nm8);
        out.writeInt(nlen); out.write(nm8, (size_t) nlen);
        juce::FileInputStream fin(f);
        if (!fin.openedOk()) { out.writeInt64(0); continue; }
        int64 sz = fin.getTotalLength(); out.writeInt64(sz);
        const size_t bufSz = 65536; juce::HeapBlock<char> buf(bufSz);
        while (!fin.isExhausted()) { auto n = fin.read(buf.getData(), (int) bufSz); if (n > 0) out.write(buf.getData(), (size_t) n); else break; }
    }
    out.flush();
    return true;
}

bool MappingSamplerProcessor::importInstrument(const juce::File& srcFile, int& msb, int& lsb, int& pc, juce::String& name) {
    msb = lsb = pc = 0; name = {};
    juce::FileInputStream in(srcFile);
    if (!in.openedOk()) return false;
    char magic[4]; if (in.read(magic, 4) != 4) return false; if (juce::String(magic, 4) != "ZXI1") return false;
    int version = in.readInt(); juce::ignoreUnused(version);
    int64 jsonLen = in.readInt64(); if (jsonLen <= 0) return false;
    juce::MemoryBlock jb; jb.setSize((size_t) jsonLen);
    in.read(jb.getData(), (size_t) jsonLen);
    auto v = juce::JSON::parse(juce::String::fromUTF8((const char*) jb.getData(), (int) jb.getSize()));
    if (auto* root = v.getDynamicObject()) {
        name = root->getProperty("name").toString();
        msb = (int) root->getProperty("msb");
        lsb = (int) root->getProperty("lsb");
        pc  = (int) root->getProperty("pc");
        if (auto* a = root->getProperty("adsr").getDynamicObject()) {
            adsrParams.attack  = (float) a->getProperty("a");
            adsrParams.decay   = (float) a->getProperty("d");
            adsrParams.sustain = (float) a->getProperty("s");
            adsrParams.release = (float) a->getProperty("r");
        }
        zones.clear(); readers.clear();
        auto arr = root->getProperty("zones");
        if (arr.isArray()) {
            auto* za = arr.getArray();
            zoneSampleIndices.clear(); zoneSampleIndices.reserve(za->size());
            for (int i = 0; i < za->size(); ++i) {
                auto zv = za->getReference(i);
                if (auto* zjo = zv.getDynamicObject()) {
                    Zone z; z.file = juce::File(zjo->getProperty("file").toString());
                    z.rootKey = (int) zjo->getProperty("root");
                    z.lowKey  = (int) zjo->getProperty("low");
                    z.highKey = (int) zjo->getProperty("high");
                    z.velLow  = (int) zjo->getProperty("vlow");
                    z.velHigh = (int) zjo->getProperty("vhigh");
                    z.gain    = (float) zjo->getProperty("gain");
                    z.pan     = (float) zjo->getProperty("pan");
                    z.tuneCents = (float) zjo->getProperty("tune");
                    zones.push_back(z);
                    int sidx = (int) zjo->getProperty("sample"); zoneSampleIndices.push_back(juce::jmax(0, sidx));
                }
            }
        }
    }
    int sampleCount = in.readInt();
    juce::Logger::writeToLog("ImportInstrument: src=" + srcFile.getFullPathName() + " samples=" + juce::String(sampleCount));
    auto cacheDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("ZEUS-X VAW").getChildFile("Cache").getChildFile(srcFile.getFileNameWithoutExtension());
    cacheDir.createDirectory();
    juce::Logger::writeToLog("ImportInstrument: cacheDir=" + cacheDir.getFullPathName());
    std::vector<juce::File> localFiles; localFiles.reserve((size_t) sampleCount);
    for (int i = 0; i < sampleCount; ++i) {
        int nameLen = in.readInt(); juce::MemoryBlock nb; nb.setSize((size_t) nameLen); in.read(nb.getData(), (size_t) nameLen);
        juce::String nm = juce::String::fromUTF8((const char*) nb.getData(), nameLen);
        int64 sz = in.readInt64();
        auto lf = cacheDir.getChildFile(nm);
        juce::FileOutputStream os(lf);
        if (!os.openedOk()) { localFiles.push_back(juce::File()); juce::Logger::writeToLog("ImportInstrument: write fail for " + lf.getFullPathName()); continue; }
        const size_t bufSz = 65536; juce::HeapBlock<char> buf(bufSz);
        int64 rem = sz;
        while (rem > 0) { auto chunk = (int) juce::jmin<int64>(bufSz, rem); auto n = in.read(buf.getData(), chunk); if (n > 0) { os.write(buf.getData(), (size_t) n); rem -= n; } else break; }
        os.flush(); localFiles.push_back(lf);
    }
    readers.clear();
    for (size_t i = 0; i < zones.size(); ++i) {
        int sidx = (i < zoneSampleIndices.size() ? zoneSampleIndices[i] : 0);
        juce::File lf = (sidx >= 0 && sidx < (int) localFiles.size()) ? localFiles[(size_t) sidx] : juce::File();
        auto r = std::unique_ptr<juce::AudioFormatReader>(formatManager.createReaderFor(lf));
        if (r) readers.push_back(std::move(r)); else readers.push_back(nullptr);
        if (!readers.back()) juce::Logger::writeToLog("ImportInstrument: reader null for " + lf.getFullPathName());
    }
    rebuild();
    juce::Logger::writeToLog("ImportInstrument: zones=" + juce::String((int)zones.size()) + " sounds=" + juce::String(synth.getNumSounds()));
    return true;
}
