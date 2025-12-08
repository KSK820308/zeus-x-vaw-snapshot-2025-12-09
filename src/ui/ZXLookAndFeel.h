#pragma once
#include <JuceHeader.h>
class ZXLookAndFeel : public juce::LookAndFeel_V4 {
public:
    void drawRotarySlider(juce::Graphics& g,int x,int y,int w,int h,float sliderPos,const float rotaryStartAngle,const float rotaryEndAngle,juce::Slider& s) override {
        int size = std::min(w, h);
        float cx = (float) x + (w - size) * 0.5f;
        float cy = (float) y + (h - size) * 0.5f;
        auto r = juce::Rectangle<float>(cx, cy, (float) size, (float) size).reduced(4.0f);
        float rotationOffset = -juce::MathConstants<float>::pi / 2.0f;
        float startA = rotaryStartAngle + rotationOffset;
        float endA   = rotaryEndAngle   + rotationOffset;
        auto ang = startA + sliderPos * (endA - startA);
        auto centre = r.getCentre();
        float radius = std::min(r.getWidth(), r.getHeight()) * 0.5f - 4.0f;
        auto outerGrad = juce::ColourGradient(juce::Colour::fromRGB(32,32,32), centre, juce::Colour::fromRGB(16,16,16), centre.translated(0.0f, radius), true);
        g.setGradientFill(outerGrad);
        g.fillEllipse(r);
        g.setColour(juce::Colour::fromRGB(20,20,20));
        g.drawEllipse(r,1.0f);
        auto innerR = r.reduced(r.getWidth()*0.18f);
        auto innerGrad = juce::ColourGradient(juce::Colour::fromRGB(70,70,70), innerR.getCentre(), juce::Colour::fromRGB(40,40,40), innerR.getBottomLeft(), true);
        g.setGradientFill(innerGrad);
        g.fillEllipse(innerR);
        g.setColour(juce::Colour::fromRGB(120,120,120));
        g.drawEllipse(innerR,1.0f);
        // subtle top highlight wedge
        {
            juce::Path wedge;
            float topA1 = -juce::MathConstants<float>::pi / 2.0f - 0.12f;
            float topA2 = -juce::MathConstants<float>::pi / 2.0f + 0.12f;
            wedge.addPieSegment(innerR.expanded(2.0f), topA1, topA2, 0.60f);
            g.setColour(juce::Colour::fromFloatRGBA(1.f,1.f,1.f,0.06f));
            g.fillPath(wedge);
        }
        g.setColour(juce::Colours::white.withAlpha(0.25f));
        int ticks = 24;
        for (int i = 0; i <= ticks; ++i) {
            float t = (float) i / (float) ticks;
            float a = startA + t * (endA - startA);
            float startR = radius + 2.0f;                 // just outside outer ring
            float len    = (i % 2 == 0) ? 6.0f : 3.0f;     // short ticks
            auto p1 = juce::Point<float>(centre.x + startR*std::cos(a), centre.y + startR*std::sin(a));
            auto p2 = juce::Point<float>(centre.x + (startR + len)*std::cos(a), centre.y + (startR + len)*std::sin(a));
            g.drawLine(p1.x, p1.y, p2.x, p2.y, (i % 2 == 0) ? 2.0f : 1.2f);
        }
        g.setColour(juce::Colour(0xFF00D47D));
        auto tip = juce::Point<float>(centre.x + (radius-6.0f)*std::cos(ang), centre.y + (radius-6.0f)*std::sin(ang));
        auto tail = juce::Point<float>(centre.x + (radius-16.0f)*std::cos(ang), centre.y + (radius-16.0f)*std::sin(ang));
        g.drawLine(tail.x, tail.y, tip.x, tip.y, 3.0f);
    }
    void drawButtonBackground(juce::Graphics& g, juce::Button& b, const juce::Colour& background, bool highlighted, bool down) override {
        auto r = b.getLocalBounds().toFloat();
        auto c = background;
        if (down) c = c.darker(0.15f);
        if (highlighted) c = c.brighter(0.05f);
        g.setColour(c);
        g.fillRoundedRectangle(r, 6.0f);
        g.setColour(juce::Colours::black.withAlpha(0.6f));
        g.drawRoundedRectangle(r, 6.0f, 1.0f);
    }
    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          const juce::Slider::SliderStyle style, juce::Slider& s) override {
        juce::ignoreUnused(minSliderPos, maxSliderPos, style);
        auto area = juce::Rectangle<float>(x, y, (float) width, (float) height);
        auto track = area.reduced(6.0f, 0.0f);
        g.setColour(juce::Colour::fromRGB(28,28,28));
        g.fillRoundedRectangle(track, 8.0f);
        auto groove = track.reduced(track.getWidth()*0.38f, 0.0f);
        g.setColour(juce::Colour::fromRGB(22,22,22));
        g.fillRoundedRectangle(groove, 6.0f);
        int ticks = 16;
        g.setColour(juce::Colours::white.withAlpha(0.45f));
        for (int i = 0; i < ticks; ++i) {
            float t = (float) i / (float) (ticks - 1);
            float yy = track.getY() + t * track.getHeight();
            float len = (i % 2 == 0 ? 10.0f : 6.0f);
            float leftEnd = groove.getX() - 2.0f;
            float leftStart = leftEnd - len;
            float rightStart = groove.getRight() + 2.0f;
            float rightEnd = rightStart + len;
            g.drawLine(leftStart, yy, leftEnd, yy, 1.0f);
            g.drawLine(rightStart, yy, rightEnd, yy, 1.0f);
        }
        float knobH = 28.0f;
        float knobW = juce::jlimit(36.0f, track.getWidth() - 6.0f, 64.0f);
        float clampedPos = juce::jlimit(track.getY(), track.getBottom()-knobH, sliderPos - knobH*0.5f);
        auto knob = juce::Rectangle<float>(track.getX() + (track.getWidth()-knobW)*0.5f, clampedPos, knobW, knobH);
        juce::DropShadow ds(juce::Colour(0x50000000), 6, juce::Point<int>());
        ds.drawForRectangle(g, knob.getSmallestIntegerContainer());
        g.setColour(juce::Colour::fromRGB(36,36,36));
        g.fillRoundedRectangle(knob, 8.0f);
        g.setColour(juce::Colour::fromRGB(10,10,10));
        g.drawRoundedRectangle(knob, 8.0f, 1.0f);
        int v127 = juce::roundToInt(s.getValue());
        juce::Colour valueColour = juce::Colour(0xFF1FA545);
        if (v127 >= 110) valueColour = juce::Colour(0xFFE23B3B).brighter(0.35f);
        else if (v127 >= 85) valueColour = juce::Colour(0xFFE5C51B).brighter(0.25f);
        g.setColour(valueColour);
        g.setFont(juce::Font(juce::FontOptions(14.0f, juce::Font::bold)));
        auto valueText = juce::String(v127);
        g.drawText(valueText, knob.getSmallestIntegerContainer().reduced(4), juce::Justification::centred);
    }
};
