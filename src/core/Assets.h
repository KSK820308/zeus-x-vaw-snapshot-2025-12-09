#pragma once
#include <JuceHeader.h>
namespace Assets {
juce::File resourcesBase();
juce::File iconsDir();
juce::Image loadIconFor(const juce::String& name);
juce::Image loadIconFor(const juce::String& name, const juce::String& category);
void ensureAssetDirs();
}
