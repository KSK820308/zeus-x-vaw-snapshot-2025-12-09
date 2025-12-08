#pragma once
#include <JuceHeader.h>

class PluginManager : public juce::ChangeBroadcaster {
public:
    static PluginManager& instance() { static PluginManager pm; return pm; }

    void init() {
        if (!initialised) {
            formatManager = std::make_unique<juce::AudioPluginFormatManager>();
           #if JUCE_MAC
            formatManager->addFormat (std::make_unique<juce::AudioUnitPluginFormat>());
           #endif
            formatManager->addFormat (std::make_unique<juce::VST3PluginFormat>());
            initialised = true;
        }
    }

    void scan() {
        init();
        juce::KnownPluginList local;
        for (int i = 0; i < formatManager->getNumFormats(); ++i) {
            auto* fmt = formatManager->getFormat(i);
            juce::FileSearchPath paths;
           #if JUCE_MAC
            if (fmt->getName().containsIgnoreCase("VST3")) {
                paths.add(juce::File("/Library/Audio/Plug-Ins/VST3"));
                auto userHome = juce::File::getSpecialLocation(juce::File::userHomeDirectory);
                paths.add(userHome.getChildFile("Library/Audio/Plug-Ins/VST3"));
            }
           #endif
            juce::PluginDirectoryScanner scanner (local, *fmt, paths, true, juce::File());
            juce::String pluginBeingScanned;
            while (scanner.scanNextFile(true, pluginBeingScanned)) {}
        }
        descs = local.getTypes();
        saveCache();
        sendChangeMessage();
    }

    juce::Array<juce::PluginDescription> getDescriptions() const { return descs; }

    std::unique_ptr<juce::AudioPluginInstance> create(const juce::PluginDescription& desc,
                                                       double sampleRate,
                                                       int blockSize,
                                                       juce::String& errorOut) {
        init();
        for (int i = 0; i < formatManager->getNumFormats(); ++i) {
            auto* fmt = formatManager->getFormat(i);
            if (fmt->getName() == desc.pluginFormatName) {
                if (auto inst = std::unique_ptr<juce::AudioPluginInstance>(fmt->createInstanceFromDescription(desc, sampleRate, blockSize))) return std::move(inst);
            }
        }
        if (auto inst = std::unique_ptr<juce::AudioPluginInstance>(formatManager->createPluginInstance(desc, sampleRate, blockSize, errorOut))) return std::move(inst);
        for (int i = 0; i < formatManager->getNumFormats(); ++i) {
            auto* fmt = formatManager->getFormat(i);
            if (fmt->getName() == desc.pluginFormatName) {
                juce::OwnedArray<juce::PluginDescription> found;
                fmt->findAllTypesForFile(found, desc.fileOrIdentifier);
                for (auto* pd : found) {
                    if (pd->name == desc.name || pd->fileOrIdentifier == desc.fileOrIdentifier) {
                        if (auto inst2 = std::unique_ptr<juce::AudioPluginInstance>(fmt->createInstanceFromDescription(*pd, sampleRate, blockSize))) return std::move(inst2);
                    }
                }
            }
        }
        return nullptr;
    }

    void loadCache() {
        init();
        descs.clear();
        auto f = getCacheFile();
        if (f.existsAsFile()) {
            auto parsed = juce::JSON::parse(f.loadFileAsString());
            if (auto* arr = parsed.getArray()) {
                for (auto& v : *arr) {
                    if (auto* o = v.getDynamicObject()) {
                        juce::PluginDescription d;
                        d.name = o->getProperty("name").toString();
                        d.descriptiveName = o->getProperty("descriptiveName").toString();
                        d.pluginFormatName = o->getProperty("pluginFormatName").toString();
                        d.fileOrIdentifier = o->getProperty("fileOrIdentifier").toString();
                        d.category = o->getProperty("category").toString();
                        d.manufacturerName = o->getProperty("manufacturer").toString();
                        d.isInstrument = (bool) o->getProperty("isInstrument");
                        d.numInputChannels = (int) o->getProperty("numInputChannels");
                        d.numOutputChannels = (int) o->getProperty("numOutputChannels");
                        d.version = o->getProperty("version").toString();
                        d.uniqueId = (int) o->getProperty("uniqueId");
                        d.deprecatedUid = (int) o->getProperty("deprecatedUid");
                        descs.add(d);
                    }
                }
            }
            sendChangeMessage();
        }
    }

    void saveCache() {
        auto f = getCacheFile();
        juce::Array<juce::var> arr;
        for (auto& d : descs) {
            auto* o = new juce::DynamicObject();
            o->setProperty("name", d.name);
            o->setProperty("descriptiveName", d.descriptiveName);
            o->setProperty("pluginFormatName", d.pluginFormatName);
            o->setProperty("fileOrIdentifier", d.fileOrIdentifier);
            o->setProperty("category", d.category);
            o->setProperty("manufacturer", d.manufacturerName);
            o->setProperty("isInstrument", d.isInstrument);
            o->setProperty("numInputChannels", d.numInputChannels);
            o->setProperty("numOutputChannels", d.numOutputChannels);
            o->setProperty("version", d.version);
            o->setProperty("uniqueId", d.uniqueId);
            o->setProperty("deprecatedUid", d.deprecatedUid);
            arr.add(juce::var(o));
        }
        juce::var root(arr);
        f.replaceWithText(juce::JSON::toString(root));
    }

private:
    juce::File getCacheFile() const {
        auto appData = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
        auto dir = appData.getChildFile("ZEUS-X VAW");
        dir.createDirectory();
        return dir.getChildFile("plugins.json");
    }

private:
    bool initialised { false };
    std::unique_ptr<juce::AudioPluginFormatManager> formatManager;
    juce::Array<juce::PluginDescription> descs;
};
