#pragma once
#include <JuceHeader.h>
namespace ZXTheme {
static inline juce::Colour panelBG() { return juce::Colour::fromRGB(18,18,18); }
static inline juce::Colour slotBG() { return juce::Colour::fromFloatRGBA(0.f,0.f,0.f,0.55f); }
static inline juce::Colour labelText() { return juce::Colours::white; }
static inline juce::Colour rightAccent() { return juce::Colour(0xFF3B82F6); }
static inline juce::Colour lowerAccent() { return juce::Colour(0xFFF59E0B); }
static inline juce::Colour mixAccent() { return juce::Colour(0xFFC4A000); }
static inline juce::Colour labelMaster() { return juce::Colour(0xFFEF4444); }
static inline juce::Colour muteActive() { return juce::Colour(0xFFE23B3B); }
static inline juce::Colour soloActive() { return juce::Colour(0xFF00D47D); }
}
