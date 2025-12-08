#include "Assets.h"
#include "SoundLibrary.h"
namespace Assets {
juce::File resourcesBase() {
   #if JUCE_MAC
    auto exe = juce::File::getSpecialLocation(juce::File::currentExecutableFile);
    auto res = exe.getParentDirectory().getParentDirectory().getChildFile("Resources");
    return res;
   #else
    return juce::File::getSpecialLocation(juce::File::currentExecutableFile).getParentDirectory();
   #endif
}
juce::File iconsDir() {
    auto docs = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
        .getChildFile("ZEUS-X VAW").getChildFile("Assets").getChildFile("icons");
    if (docs.isDirectory()) return docs;
    auto res = resourcesBase().getChildFile("assets").getChildFile("icons");
    return res;
}
static juce::String normalize(const juce::String& s) {
    auto t = s.trim();
    t = t.replaceCharacter(' ', '_');
    t = t.replaceCharacter('.', '_');
    t = t.replaceCharacter('-', '_');
    return t.toLowerCase();
}
juce::Image loadIconFor(const juce::String& name) {
    auto base = iconsDir();
    juce::String baseName = normalize(name);
    juce::StringArray exts { ".png", ".jpg", ".jpeg" };
    for (auto& ext : exts) {
        auto file = base.getChildFile(baseName + ext);
        if (file.existsAsFile()) {
            juce::Image img = juce::ImageFileFormat::loadFrom(file);
            if (!img.isNull()) return img;
        }
    }
    auto files = base.findChildFiles(juce::File::findFiles, false);
    for (auto& f : files) {
        auto stem = normalize(f.getFileNameWithoutExtension());
        if (stem == baseName) {
            juce::Image img = juce::ImageFileFormat::loadFrom(f);
            if (!img.isNull()) return img;
        }
    }
    return juce::Image();
}
juce::Image loadIconFor(const juce::String& name, const juce::String& category) {
    auto img = loadIconFor(name);
    if (!img.isNull()) return img;
    return loadIconFor(category);
}
void ensureAssetDirs() {
    auto docsBase = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("ZEUS-X VAW");
    docsBase.createDirectory();
    docsBase.getChildFile("Assets").getChildFile("icons").createDirectory();
}
}
