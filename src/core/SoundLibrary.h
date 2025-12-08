#pragma once
#include <JuceHeader.h>
namespace SoundLibrary {
juce::StringArray voiceZXCategories();
juce::StringArray audioVoiceCategories();
juce::StringArray voiceZXCategories();
juce::File baseDir();
void ensureDirectories();
juce::File customVoiceBase();
juce::File customVoiceSlot(int index);
juce::String getCustomVoiceSlotName(int index);
void setCustomVoiceSlotName(int index, const juce::String& name);
void clearCustomVoiceSlot(int index);
juce::File customVoiceContentDir(int index);
void migrateCustomVoiceLegacy();
void migrateLegacyRoots();
juce::StringArray listCategoryItems(const juce::String& category);
juce::StringArray listCustomSlotItems(int index);
int nextFreePcForCategory(const juce::String& category);
juce::Array<juce::var> readVoiceManifest(const juce::String& category);
void writeVoiceManifest(const juce::String& category, const juce::Array<juce::var>& arr);
bool assignVoicePosition(const juce::String& category, int pc, const juce::String& title, const juce::File& sourceFile, const juce::File& imageFile, bool replace);
juce::File voiceFileFor(const juce::String& category, int pc);
bool deleteVoiceFile(const juce::File& file);
bool deleteVoicePosition(const juce::String& category, int pc);
bool renameVoiceItem(const juce::String& category, int pc, const juce::String& newTitle);
bool copyVoiceItem(const juce::String& category, int pc);
bool hasVoiceClipboard();
bool pasteVoiceItem(const juce::String& dstCategory, int dstPc, bool replace);
bool moveVoiceItem(const juce::String& srcCategory, int srcPc, const juce::String& dstCategory, int dstPc, bool replace);
bool renameCustomItem(int slotIndex, int pc, const juce::String& newTitle);
bool removeCustomItem(int slotIndex, int pc);
bool hasCustomItem(int slotIndex, int pc);
juce::String getCustomItemTitle(int slotIndex, int pc);
bool copyItemToClipboard(int slotIndex, int pc);
bool hasItemClipboard();
bool pasteItemFromClipboard(int slotIndex, int pc, bool replace);
void startMoveItem(int slotIndex, int pc);
bool hasMoveItem();
bool finishMoveItem(int dstSlotIndex, int dstPc, bool replace);
bool copySlotToClipboard(int slotIndex);
bool hasSlotClipboard();
bool pasteSlotFromClipboard(int dstSlotIndex, bool replace);
bool deleteSlot(int slotIndex);
bool moveSlot(int srcSlotIndex, int dstSlotIndex, bool replace);
}
