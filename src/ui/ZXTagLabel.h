#pragma once
#include <JuceHeader.h>
class ZXTagLabel : public juce::Component {
public:
    void setText(const juce::String& t) { text = t; repaint(); }
    void setAccent(juce::Colour c) { accent = c; repaint(); }
    void paint(juce::Graphics& g) override {
        auto r = getLocalBounds().toFloat();
        g.setColour(juce::Colour::fromRGB(20,20,20));
        g.fillRoundedRectangle(r, 6.0f);
        g.setColour(accent);
        g.drawRoundedRectangle(r, 6.0f, 1.5f);
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(juce::FontOptions(15.0f, juce::Font::bold)));
        g.drawText(text, getLocalBounds().reduced(8,2), juce::Justification::centred);
    }
private:
    juce::String text;
    juce::Colour accent { juce::Colours::white };
};
