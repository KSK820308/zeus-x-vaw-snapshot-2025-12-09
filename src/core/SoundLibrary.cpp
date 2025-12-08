#include "SoundLibrary.h"
namespace SoundLibrary {
static int msbForCategory(const juce::String&) { return 0; }
static int lsbForCategory(const juce::String& cat) {
    if (cat.equalsIgnoreCase("Piano")) return 0;
    if (cat.equalsIgnoreCase("El Piano")) return 1;
    if (cat.equalsIgnoreCase("Organ")) return 2;
    if (cat.equalsIgnoreCase("Strings")) return 3;
    if (cat.equalsIgnoreCase("Woodwind")) return 4;
    if (cat.equalsIgnoreCase("Acc Gtr")) return 5;
    if (cat.equalsIgnoreCase("Bass")) return 6;
    if (cat.equalsIgnoreCase("Pad")) return 7;
    if (cat.equalsIgnoreCase("DrumKit")) return 8;
    if (cat.equalsIgnoreCase("Accordion")) return 9;
    if (cat.equalsIgnoreCase("Brass")) return 10;
    if (cat.equalsIgnoreCase("Choir")) return 11;
    if (cat.equalsIgnoreCase("El Gtr")) return 12;
    if (cat.equalsIgnoreCase("Synth")) return 13;
    if (cat.equalsIgnoreCase("SFX")) return 14;
    if (cat.equalsIgnoreCase("Percussion")) return 15;
    if (cat.equalsIgnoreCase("AUDIO DRUMS")) return 20;
    if (cat.equalsIgnoreCase("AUDIO PERCUSSION")) return 21;
    if (cat.equalsIgnoreCase("AUDIO BASS")) return 22;
    if (cat.equalsIgnoreCase("AUDIO ACC GTR")) return 23;
    if (cat.equalsIgnoreCase("AUDIO EL GTR")) return 24;
    if (cat.equalsIgnoreCase("AUDIO DIST GTR")) return 25;
    return 0;
}
static bool retagZXI(const juce::File& file, int msb, int lsb, int pc, const juce::String& name) {
    juce::FileInputStream in(file); if (!in.openedOk()) return false;
    char magic[4]; if (in.read(magic, 4) != 4) return false; if (juce::String(magic, 4) != "ZXI1") return false;
    int version = in.readInt(); juce::ignoreUnused(version);
    int64 jsonLen = in.readInt64(); if (jsonLen <= 0) return false;
    juce::MemoryBlock jb; jb.setSize((size_t) jsonLen); in.read(jb.getData(), (size_t) jsonLen);
    auto v = juce::JSON::parse(juce::String::fromUTF8((const char*) jb.getData(), (int) jb.getSize()));
    auto* root = v.getDynamicObject(); if (!root) return false;
    root->setProperty("name", name);
    root->setProperty("msb", msb);
    root->setProperty("lsb", lsb);
    root->setProperty("pc", pc);
    int sampleCount = in.readInt();
    struct SampleInfo { juce::String name; int64 size; juce::MemoryBlock data; };
    juce::Array<SampleInfo> samples; samples.ensureStorageAllocated(sampleCount);
    for (int i = 0; i < sampleCount; ++i) {
        int nameLen = in.readInt(); juce::MemoryBlock nb; nb.setSize((size_t) nameLen); in.read(nb.getData(), (size_t) nameLen);
        juce::String nm = juce::String::fromUTF8((const char*) nb.getData(), nameLen);
        int64 sz = in.readInt64(); SampleInfo si; si.name = nm; si.size = sz; si.data.setSize((size_t) sz);
        if (sz > 0) in.read(si.data.getData(), (size_t) sz);
        samples.add(si);
    }
    juce::FileOutputStream out(file); if (!out.openedOk()) return false;
    out.write("ZXI1", 4); out.writeInt(1);
    auto json = juce::JSON::toString(v); juce::MemoryBlock jmb(json.toRawUTF8(), (size_t) json.getNumBytesAsUTF8());
    out.writeInt64((int64) jmb.getSize()); out.write(jmb.getData(), jmb.getSize());
    out.writeInt(samples.size());
    for (int i = 0; i < samples.size(); ++i) {
        auto& si = samples.getReference(i);
        auto nm8 = si.name.toRawUTF8(); int nlen = (int) strlen(nm8);
        out.writeInt(nlen); out.write(nm8, (size_t) nlen);
        out.writeInt64(si.size); if (si.size > 0) out.write(si.data.getData(), (size_t) si.size);
    }
    out.flush();
    return true;
}
static juce::var g_itemClipboard;
static int g_itemClipboardSlot { -1 };
static bool g_moveActive { false };
static juce::var g_slotClipboard;
static juce::var findItem(int slotIndex, int pc);
static void upsertItem(int slotIndex, const juce::var& item);
static juce::File slotMapFile(int index);
static juce::Array<juce::var> readMap(int index);
static void writeMap(int index, const juce::Array<juce::var>& arr);
static void copyResourceIfPresent(int srcSlot, int dstSlot, const juce::String& key, juce::DynamicObject* obj) {
    if (obj == nullptr) return;
    auto v = obj->getProperty(juce::Identifier(key));
    if (v.isVoid()) return;
    auto name = v.toString();
    if (name.isEmpty()) return;
    auto srcDir = customVoiceSlot(srcSlot);
    auto dstDir = customVoiceSlot(dstSlot);
    auto srcFile = srcDir.getChildFile(name);
    auto dstFile = dstDir.getChildFile(name);
    if (srcFile.existsAsFile()) {
        dstDir.createDirectory();
        if (dstFile.exists()) dstFile.deleteFile();
        srcFile.copyFileTo(dstFile);
        obj->setProperty(key, dstFile.getFileName());
    }
}

static void ensureSlotMapFromDir(int index) {
    auto f = slotMapFile(index);
    if (f.existsAsFile()) return;
    auto dir = customVoiceSlot(index);
    if (!dir.isDirectory()) return;
    auto files = dir.findChildFiles(juce::File::findFiles, false);
    juce::Array<juce::var> arr;
    int pc = 1;
    for (auto& file : files) {
        auto nm = file.getFileName();
        if (nm.startsWithChar('.')) continue;
        juce::DynamicObject* d = new juce::DynamicObject();
        d->setProperty("pc", pc);
        d->setProperty("title", file.getFileNameWithoutExtension());
        d->setProperty("file", nm);
        arr.add(juce::var(d));
        pc++;
    }
    if (arr.size() > 0) writeMap(index, arr);
}
juce::StringArray voiceZXCategories() {
    return {
        "Piano","Organ","Strings","Woodwind","Acc Gtr","Bass","Pad","DrumKit",
        "El Piano","Accordion","Brass","Choir","El Gtr","Synth","Percussion"
    };
}
juce::StringArray audioVoiceCategories() {
    return { "AUDIO DRUMS", "AUDIO PERCUSSION", "AUDIO BASS", "AUDIO ACC GTR", "AUDIO EL GTR", "AUDIO DIST GTR" };
}
juce::File baseDir() {
    auto docs = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
    return docs.getChildFile("ZEUS-X VAW").getChildFile("ZEUS-X SOUNDS");
}
juce::File customVoiceBase() {
    auto docs = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
    return docs.getChildFile("ZEUS-X VAW").getChildFile("CUSTOM VOICE");
}
static juce::File findSlotDirByIndex(const juce::File& base, int index) {
    auto dirs = base.findChildFiles(juce::File::findDirectories, false);
    for (auto& d : dirs) {
        auto marker = d.getChildFile(".slotindex");
        if (marker.existsAsFile()) {
            auto txt = marker.loadFileAsString().trim();
            if (txt == juce::String(index)) return d;
        }
    }
    auto fallback = base.getChildFile(juce::String("Slot ") + juce::String(index + 1).paddedLeft('0', 2));
    return fallback;
}
juce::File customVoiceSlot(int index) {
    auto base = customVoiceBase();
    return findSlotDirByIndex(base, index);
}
void ensureDirectories() {
    migrateLegacyRoots();
    auto base = baseDir();
    base.createDirectory();
    auto cats = voiceZXCategories();
    for (auto& c : cats) base.getChildFile(c).createDirectory();
    auto audioCats = audioVoiceCategories();
    for (auto& c : audioCats) base.getChildFile(c).createDirectory();
    auto zxStyle = base.getParentDirectory().getChildFile("ZEUS-X STYLE");
    zxStyle.createDirectory();
    auto customStyle = base.getParentDirectory().getChildFile("CUSTOM STYLE");
    customStyle.createDirectory();
    
}
juce::String getCustomVoiceSlotName(int index) {
    auto dir = customVoiceSlot(index);
    auto f = dir.getChildFile(".name");
    if (f.existsAsFile()) return f.loadFileAsString().trim();
    return {};
}
void setCustomVoiceSlotName(int index, const juce::String& name) {
    auto dir = customVoiceSlot(index);
    dir.createDirectory();
    dir.getChildFile(".slotindex").replaceWithText(juce::String(index));
    auto newName = name.trim();
    dir.getChildFile(".name").replaceWithText(newName);
}
void clearCustomVoiceSlot(int index) {
    auto dir = customVoiceSlot(index);
    if (!dir.isDirectory()) return;
    auto children = dir.findChildFiles(juce::File::findFilesAndDirectories, false);
    for (auto& f : children) {
        auto n = f.getFileName();
        if (n != ".name" && n != ".slotindex") f.deleteRecursively();
    }
}
juce::File customVoiceContentDir(int index) {
    return customVoiceSlot(index);
}

static int findFirstEmptyCustomSlot() {
    for (int i = 0; i < 16; ++i) {
        auto name = SoundLibrary::getCustomVoiceSlotName(i);
        auto dir = SoundLibrary::customVoiceSlot(i);
        auto contents = dir.findChildFiles(juce::File::findFilesAndDirectories, false);
        if (name.isEmpty() && contents.isEmpty()) return i;
    }
    return -1;
}

void migrateCustomVoiceLegacy() {
    auto custom = customVoiceBase();
    auto cats = voiceZXCategories();
    for (auto& c : cats) {
        auto legacy = custom.getChildFile(c);
        if (legacy.isDirectory()) {
            auto items = legacy.findChildFiles(juce::File::findFilesAndDirectories, true);
            if (!items.isEmpty()) {
                int slot = findFirstEmptyCustomSlot();
                if (slot >= 0) {
                    setCustomVoiceSlotName(slot, c);
                    auto target = customVoiceSlot(slot).getChildFile(c);
                    target.createDirectory();
                    legacy.moveFileTo(target);
                }
            }
            legacy.deleteRecursively();
        }
    }
}

void migrateLegacyRoots() {
    auto docs = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
    auto legacyDocs = docs.getChildFile("ZEUS-X-WAV");
    auto newDocs = docs.getChildFile("ZEUS-X VAW");
    if (legacyDocs.isDirectory()) {
        if (!newDocs.exists()) legacyDocs.moveFileTo(newDocs);
        else { legacyDocs.copyDirectoryTo(newDocs); legacyDocs.deleteRecursively(); }
    }
    auto appData = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
    auto legacyApp = appData.getChildFile("ZEUS-X-WAV");
    auto newApp = appData.getChildFile("ZEUS-X VAW");
    if (legacyApp.isDirectory()) {
        if (!newApp.exists()) legacyApp.moveFileTo(newApp);
        else { legacyApp.copyDirectoryTo(newApp); legacyApp.deleteRecursively(); }
    }
}

juce::StringArray listCategoryItems(const juce::String& category) {
    juce::StringArray out;
    auto base = baseDir().getChildFile(category);
    auto manifest = base.getChildFile(".voice.json");
    if (manifest.existsAsFile()) {
        auto v = juce::JSON::parse(manifest.loadFileAsString());
        if (v.isArray()) {
            auto* arr = v.getArray();
            struct Cmp { int compareElements(const std::pair<int, juce::String>& a, const std::pair<int, juce::String>& b) noexcept { return a.first - b.first; } };
            juce::Array<std::pair<int, juce::String>> items;
            for (int i = 0; i < arr->size(); ++i) {
                auto o = (*arr)[i];
                int pc = (int) o.getProperty("pc", juce::var());
                juce::String title = o.getProperty("title", juce::var()).toString();
                if (title.isEmpty()) title = juce::String("Instrument ") + juce::String(pc);
                items.add({ pc, title });
            }
            Cmp cmp; items.sort(cmp);
            for (int i = 0; i < items.size(); ++i) out.add(items[i].second);
            return out;
        }
    }
    if (base.isDirectory()) {
        auto files = base.findChildFiles(juce::File::findFiles, false);
        for (auto& f : files) {
            auto nm = f.getFileName();
            if (nm.startsWithChar('.')) continue;
            if (f.getFileExtension().toLowerCase() != ".zxi") continue;
            out.add(f.getFileNameWithoutExtension());
        }
    }
    return out;
}

int nextFreePcForCategory(const juce::String& category) {
    auto arr = SoundLibrary::readVoiceManifest(category);
    bool used[101]{}; // 1..100
    for (int i = 0; i < arr.size(); ++i) {
        int pc = (int) arr[i].getProperty("pc", juce::var());
        if (pc >= 1 && pc <= 100) used[pc] = true;
    }
    for (int pc = 1; pc <= 100; ++pc) if (!used[pc]) return pc;
    return -1;
}

juce::StringArray listCustomSlotItems(int index) {
    ensureSlotMapFromDir(index);
    juce::StringArray out;
    auto arr = readMap(index);
    struct Cmp { int compareElements(const std::pair<int, juce::String>& a, const std::pair<int, juce::String>& b) noexcept { return a.first - b.first; } };
    juce::Array<std::pair<int, juce::String>> items;
    for (int i = 0; i < arr.size(); ++i) {
        auto o = arr[i];
        int pc = (int) o.getProperty("pc", juce::var());
        juce::String title = o.getProperty("title", juce::var()).toString();
        items.add({ pc, title });
    }
    Cmp cmp; items.sort(cmp);
    for (int i = 0; i < items.size(); ++i) out.add(items[i].second);
    return out;
}

static juce::File slotMapFile(int index) { return customVoiceSlot(index).getChildFile(".map.json"); }

static juce::Array<juce::var> readMap(int index) {
    juce::Array<juce::var> out;
    auto f = slotMapFile(index);
    if (f.existsAsFile()) {
        auto v = juce::JSON::parse(f.loadFileAsString());
        if (v.isArray()) {
            auto* arr = v.getArray();
            for (int i = 0; i < arr->size(); ++i) out.add((*arr)[i]);
        }
    }
    return out;
}

static void writeMap(int index, const juce::Array<juce::var>& arr) {
    auto f = slotMapFile(index);
    juce::var v(arr);
    f.replaceWithText(juce::JSON::toString(v));
}

bool renameCustomItem(int slotIndex, int pc, const juce::String& newTitle) {
    auto arr = readMap(slotIndex);
    int idx = -1;
    for (int i = 0; i < arr.size(); ++i) {
        auto o = arr[i];
        if ((int) o.getProperty("pc", juce::var()) == pc) { idx = i; break; }
    }
    if (idx < 0) return false;
    if (auto* d = arr[idx].getDynamicObject()) {
        d->setProperty("title", newTitle.trim());
        writeMap(slotIndex, arr);
        return true;
    }
    return false;
}

bool removeCustomItem(int slotIndex, int pc) {
    auto dir = customVoiceSlot(slotIndex);
    auto arr = readMap(slotIndex);
    int idx = -1;
    for (int i = 0; i < arr.size(); ++i) if ((int) arr[i].getProperty("pc", juce::var()) == pc) { idx = i; break; }
    if (idx < 0) return false;
    auto* d = arr[idx].getDynamicObject();
    if (d) {
        auto fn = d->getProperty("file").toString();
        auto img = d->getProperty("image").toString();
        if (fn.isNotEmpty()) { auto f = dir.getChildFile(fn); if (f.existsAsFile()) f.deleteFile(); }
        if (img.isNotEmpty()) { auto imf = dir.getChildFile(img); if (imf.existsAsFile()) imf.deleteFile(); }
    }
    arr.remove(idx);
    writeMap(slotIndex, arr);
    return true;
}

bool hasCustomItem(int slotIndex, int pc) {
    auto v = findItem(slotIndex, pc);
    return v.isObject();
}

juce::String getCustomItemTitle(int slotIndex, int pc) {
    auto v = findItem(slotIndex, pc);
    if (auto* d = v.getDynamicObject()) return d->getProperty("title").toString();
    return {};
}

static juce::var findItem(int slotIndex, int pc) {
    auto arr = readMap(slotIndex);
    for (int i = 0; i < arr.size(); ++i) {
        auto o = arr[i];
        if ((int) o.getProperty("pc", juce::var()) == pc) return o;
    }
    return juce::var();
}

static void upsertItem(int slotIndex, const juce::var& item) {
    auto arr = readMap(slotIndex);
    int pc = (int) item.getProperty("pc", juce::var());
    int idx = -1;
    for (int i = 0; i < arr.size(); ++i) if ((int) arr[i].getProperty("pc", juce::var()) == pc) { idx = i; break; }
    if (idx >= 0) arr.set(idx, item); else arr.add(item);
    writeMap(slotIndex, arr);
}

bool copyItemToClipboard(int slotIndex, int pc) {
    auto v = findItem(slotIndex, pc);
    if (!v.isObject()) return false;
    g_itemClipboard = v;
    g_itemClipboardSlot = slotIndex;
    return true;
}

bool hasItemClipboard() { return !g_itemClipboard.isVoid() && g_itemClipboard.isObject(); }

bool pasteItemFromClipboard(int slotIndex, int pc, bool replace) {
    if (!hasItemClipboard()) return false;
    if (hasCustomItem(slotIndex, pc) && !replace) return false;
    auto dir = customVoiceSlot(slotIndex);
    dir.createDirectory();
    auto v = g_itemClipboard;
    if (auto* d = v.getDynamicObject()) {
        if (hasCustomItem(slotIndex, pc) && replace) removeCustomItem(slotIndex, pc);
        d->setProperty("pc", pc);
        copyResourceIfPresent(g_itemClipboardSlot, slotIndex, "file", d);
        copyResourceIfPresent(g_itemClipboardSlot, slotIndex, "image", d);
        upsertItem(slotIndex, juce::var(d));
        return true;
    }
    return false;
}

void startMoveItem(int slotIndex, int pc) { g_moveActive = copyItemToClipboard(slotIndex, pc); }

bool hasMoveItem() { return g_moveActive && hasItemClipboard(); }

bool finishMoveItem(int dstSlotIndex, int dstPc, bool replace) {
    if (!hasMoveItem()) return false;
    int srcPc = (int) g_itemClipboard.getProperty("pc", juce::var());
    bool ok = pasteItemFromClipboard(dstSlotIndex, dstPc, replace);
    if (ok) removeCustomItem(g_itemClipboardSlot, srcPc);
    g_moveActive = false; g_itemClipboard = juce::var(); g_itemClipboardSlot = -1;
    return ok;
}

bool copySlotToClipboard(int slotIndex) {
    ensureSlotMapFromDir(slotIndex);
    auto arr = readMap(slotIndex);
    g_slotClipboard = juce::var(arr);
    return arr.size() > 0;
}

bool hasSlotClipboard() { return g_slotClipboard.isArray() && g_slotClipboard.getArray()->size() > 0; }

bool pasteSlotFromClipboard(int dstSlotIndex, bool replace) {
    if (!hasSlotClipboard()) return false;
    auto* arr = g_slotClipboard.getArray();
    if (!arr) return false;
    for (int i = 0; i < arr->size(); ++i) {
        auto v = (*arr)[i];
        int pc = (int) v.getProperty("pc", juce::var());
        g_itemClipboard = v;
        g_itemClipboardSlot = -1;
        pasteItemFromClipboard(dstSlotIndex, pc, replace);
    }
    g_itemClipboard = juce::var(); g_itemClipboardSlot = -1;
    return true;
}

bool deleteSlot(int slotIndex) {
    clearCustomVoiceSlot(slotIndex);
    writeMap(slotIndex, {});
    return true;
}

bool moveSlot(int srcSlotIndex, int dstSlotIndex, bool replace) {
    if (!copySlotToClipboard(srcSlotIndex)) return false;
    if (replace) deleteSlot(dstSlotIndex);
    bool ok = pasteSlotFromClipboard(dstSlotIndex, true);
    if (ok) deleteSlot(srcSlotIndex);
    return ok;
}

// Re-open helpers inside namespace
static juce::File voiceManifestFile(const juce::String& category);
juce::Array<juce::var> readVoiceManifest(const juce::String& category);
void writeVoiceManifest(const juce::String& category, const juce::Array<juce::var>& arr);
bool assignVoicePosition(const juce::String& category, int pc, const juce::String& title, const juce::File& sourceFile, const juce::File& imageFile, bool replace);
}
namespace SoundLibrary {
static juce::File voiceManifestFile(const juce::String& category) {
    return baseDir().getChildFile(category).getChildFile(".voice.json");
}

juce::Array<juce::var> readVoiceManifest(const juce::String& category) {
    juce::Array<juce::var> out;
    auto f = voiceManifestFile(category);
    if (f.existsAsFile()) {
        auto v = juce::JSON::parse(f.loadFileAsString());
        if (v.isArray()) {
            auto* arr = v.getArray();
            for (int i = 0; i < arr->size(); ++i) out.add((*arr)[i]);
        }
    }
    return out;
}

void writeVoiceManifest(const juce::String& category, const juce::Array<juce::var>& arr) {
    auto f = voiceManifestFile(category);
    juce::var v(arr);
    f.getParentDirectory().createDirectory();
    f.replaceWithText(juce::JSON::toString(v));
}

bool assignVoicePosition(const juce::String& category, int pc, const juce::String& title, const juce::File& sourceFile, const juce::File& imageFile, bool replace) {
    auto catDir = baseDir().getChildFile(category);
    catDir.createDirectory();
    auto arr = readVoiceManifest(category);
    bool occupied = false; int occupiedIndex = -1;
    for (int i = 0; i < arr.size(); ++i) {
        auto o = arr[i];
        if ((int) o.getProperty("pc", juce::var()) == pc) { occupied = true; occupiedIndex = i; break; }
    }
    if (occupied && !replace) return false;
    juce::String baseName = title.trim().isNotEmpty() ? title.trim() : sourceFile.getFileNameWithoutExtension();
    juce::String slug = baseName.replaceCharacters("/\\:\"' ", "_");
    juce::String nn = juce::String(pc).paddedLeft('0', 2);
    juce::File targetFile = catDir.getChildFile(nn + "_" + slug + ".zxi");
    // remove previous file if replacing
    if (occupied && occupiedIndex >= 0) {
        auto old = arr[occupiedIndex].getDynamicObject();
        if (old != nullptr) {
            auto ofn = old->getProperty(juce::Identifier("file")).toString();
            if (ofn.isNotEmpty()) {
                auto of = catDir.getChildFile(ofn);
                if (of.existsAsFile()) of.deleteFile();
            }
        }
    }
    if (sourceFile.existsAsFile()) {
        if (targetFile.exists()) targetFile.deleteFile();
        sourceFile.copyFileTo(targetFile);
        retagZXI(targetFile, msbForCategory(category), lsbForCategory(category), pc, baseName);
    }
    juce::DynamicObject* d = new juce::DynamicObject();
    d->setProperty("pc", pc);
    d->setProperty("title", baseName);
    d->setProperty("file", targetFile.getFileName());
    if (imageFile.existsAsFile()) d->setProperty("image", imageFile.getFileName());
    if (occupied && occupiedIndex >= 0) arr.set(occupiedIndex, juce::var(d)); else arr.add(juce::var(d));
    writeVoiceManifest(category, arr);
    return true;
}
juce::File voiceFileFor(const juce::String& category, int pc) {
    auto arr = readVoiceManifest(category);
    for (auto& o : arr) {
        if ((int) o.getProperty("pc", juce::var()) == pc) {
            auto fn = o.getProperty("file", juce::var()).toString();
            if (fn.isNotEmpty()) return baseDir().getChildFile(category).getChildFile(fn);
        }
    }
    return {};
}
bool deleteVoiceFile(const juce::File& file) {
    if (!file.existsAsFile()) return false;
    auto categoryDir = file.getParentDirectory();
    auto category = categoryDir.getFileName();
    auto arr = readVoiceManifest(category);
    int idx = -1;
    for (int i = 0; i < arr.size(); ++i) {
        auto o = arr[i];
        auto fn = o.getProperty("file", juce::var()).toString();
        if (fn == file.getFileName()) { idx = i; break; }
    }
    if (idx < 0) return false;
    // delete file and optional image
    auto* d = arr[idx].getDynamicObject();
    if (d != nullptr) {
        auto img = d->getProperty("image").toString();
        if (img.isNotEmpty()) {
            auto imf = categoryDir.getChildFile(img);
            if (imf.existsAsFile()) imf.deleteFile();
        }
    }
    file.deleteFile();
    // remove entry from manifest
    arr.remove(idx);
    writeVoiceManifest(category, arr);
    return true;
}
bool deleteVoicePosition(const juce::String& category, int pc) {
    auto arr = readVoiceManifest(category);
    int idx = -1;
    for (int i = 0; i < arr.size(); ++i) if ((int) arr[i].getProperty("pc", juce::var()) == pc) { idx = i; break; }
    if (idx < 0) return false;
    auto* d = arr[idx].getDynamicObject();
    if (d) {
        auto img = d->getProperty("image").toString();
        if (img.isNotEmpty()) {
            auto imf = baseDir().getChildFile(category).getChildFile(img);
            if (imf.existsAsFile()) imf.deleteFile();
        }
        auto fn = d->getProperty("file").toString();
        if (fn.isNotEmpty()) {
            auto vf = baseDir().getChildFile(category).getChildFile(fn);
            if (vf.existsAsFile()) vf.deleteFile();
        }
    }
    arr.remove(idx);
    writeVoiceManifest(category, arr);
    return true;
}
static juce::var g_voiceClipboard;
bool renameVoiceItem(const juce::String& category, int pc, const juce::String& newTitle) {
    auto arr = readVoiceManifest(category);
    int idx = -1;
    for (int i = 0; i < arr.size(); ++i) if ((int) arr[i].getProperty("pc", juce::var()) == pc) { idx = i; break; }
    if (idx < 0) return false;
    auto* d = arr[idx].getDynamicObject(); if (!d) return false;
    juce::String oldFile = d->getProperty("file").toString();
    juce::String baseName = newTitle.trim();
    juce::String slug = baseName.replaceCharacters("/\\:\"' ", "_");
    juce::String nn = juce::String(pc).paddedLeft('0', 2);
    juce::File catDir = baseDir().getChildFile(category);
    juce::File oldF = catDir.getChildFile(oldFile);
    juce::File newF = catDir.getChildFile(nn + "_" + slug + ".zxi");
    if (oldF.existsAsFile()) {
        if (newF.existsAsFile()) newF.deleteFile();
        oldF.moveFileTo(newF);
        // keep msb/lsb/pc the same
        retagZXI(newF, msbForCategory(category), lsbForCategory(category), pc, baseName);
    }
    d->setProperty("title", baseName);
    d->setProperty("file", newF.getFileName());
    writeVoiceManifest(category, arr);
    return true;
}
bool copyVoiceItem(const juce::String& category, int pc) {
    auto arr = readVoiceManifest(category);
    int idx = -1;
    for (int i = 0; i < arr.size(); ++i) if ((int) arr[i].getProperty("pc", juce::var()) == pc) { idx = i; break; }
    if (idx < 0) return false;
    if (auto* d = arr[idx].getDynamicObject()) {
        auto* dd = new juce::DynamicObject();
        for (auto& k : d->getProperties()) dd->setProperty(k.name, k.value);
        dd->setProperty("category", category);
        g_voiceClipboard = juce::var(dd);
    } else g_voiceClipboard = arr[idx];
    return true;
}
bool hasVoiceClipboard() { return !g_voiceClipboard.isVoid() && g_voiceClipboard.isObject(); }
bool pasteVoiceItem(const juce::String& dstCategory, int dstPc, bool replace) {
    if (!hasVoiceClipboard()) return false;
    auto* d = g_voiceClipboard.getDynamicObject(); if (!d) return false;
    auto srcTitle = d->getProperty("title").toString();
    auto srcFileName = d->getProperty("file").toString();
    auto srcCat = d->getProperty("category").toString(); if (srcCat.isEmpty()) srcCat = dstCategory;
    juce::File srcFile = baseDir().getChildFile(srcCat).getChildFile(srcFileName);
    bool ok = assignVoicePosition(dstCategory, dstPc, srcTitle, srcFile, juce::File(), replace);
    if (ok) {
        bool sameCategory = (srcCat.equalsIgnoreCase(dstCategory));
        bool hasCanonicalPrefix = (srcFileName.length() >= 3 && juce::CharacterFunctions::isDigit(srcFileName[0]) && juce::CharacterFunctions::isDigit(srcFileName[1]) && srcFileName[2] == '_');
        if (sameCategory && !hasCanonicalPrefix && srcFile.existsAsFile()) {
            srcFile.deleteFile();
        }
    }
    return ok;
}
bool moveVoiceItem(const juce::String& srcCategory, int srcPc, const juce::String& dstCategory, int dstPc, bool replace) {
    auto srcFile = voiceFileFor(srcCategory, srcPc);
    if (!srcFile.existsAsFile()) return false;
    auto arr = readVoiceManifest(srcCategory);
    int idx = -1;
    for (int i = 0; i < arr.size(); ++i) if ((int) arr[i].getProperty("pc", juce::var()) == srcPc) { idx = i; break; }
    juce::String title = (idx >= 0) ? arr[idx].getProperty("title", juce::var()).toString() : srcFile.getFileNameWithoutExtension();
    bool ok = assignVoicePosition(dstCategory, dstPc, title, srcFile, juce::File(), replace);
    if (ok) deleteVoiceFile(srcFile);
    return ok;
}
}
