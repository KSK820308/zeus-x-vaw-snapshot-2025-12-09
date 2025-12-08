#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "../src/core/SoundLibrary.h"
#include "../src/audio/MappingSamplerProcessor.h"

int main() {
    SoundLibrary::ensureDirectories();
    juce::File tmp = juce::File::getSpecialLocation(juce::File::tempDirectory)
        .getChildFile("zx_test_" + juce::String((int) juce::Time::getMillisecondCounter()) + ".zxi");
    tmp.replaceWithText("dummy");
    bool ok1 = SoundLibrary::assignVoicePosition("Piano", 99, "TestVoice", tmp, juce::File(), true);
    auto vf = SoundLibrary::voiceFileFor("Piano", 99);
    bool exists = vf.existsAsFile();
    bool manifestPass = ok1 && exists;

    juce::File zx = juce::File::getSpecialLocation(juce::File::tempDirectory)
        .getChildFile("zx_import_" + juce::String((int) juce::Time::getMillisecondCounter()) + ".zxi");
    {
        juce::FileOutputStream out(zx);
        out.write("ZXI1", 4);
        out.writeInt(1);
        juce::DynamicObject::Ptr root(new juce::DynamicObject());
        root->setProperty("name", "UnitTest");
        root->setProperty("msb", 0);
        root->setProperty("lsb", 0);
        root->setProperty("pc", 0);
        juce::DynamicObject::Ptr adsr(new juce::DynamicObject());
        adsr->setProperty("a", 0.01f);
        adsr->setProperty("d", 0.10f);
        adsr->setProperty("s", 0.80f);
        adsr->setProperty("r", 0.20f);
        root->setProperty("adsr", adsr.get());
        juce::DynamicObject::Ptr fx(new juce::DynamicObject());
        fx->setProperty("eqLow", 0.0f);
        fx->setProperty("eqMid", 0.0f);
        fx->setProperty("eqHigh", 0.0f);
        fx->setProperty("revPreset", 1);
        fx->setProperty("revWet", 0.0f);
        fx->setProperty("delPreset", 1);
        fx->setProperty("delMix", 0.0f);
        fx->setProperty("octave", 0);
        root->setProperty("fx", fx.get());
        juce::Array<juce::var> zones;
        root->setProperty("zones", zones);
        auto json = juce::JSON::toString(root.get());
        juce::MemoryBlock jmb(json.toRawUTF8(), (size_t) json.getNumBytesAsUTF8());
        out.writeInt64((int64) jmb.getSize());
        out.write(jmb.getData(), jmb.getSize());
        out.writeInt(0);
        out.flush();
    }
    int msb{}, lsb{}, pc{}; juce::String name;
    MappingSamplerProcessor mp;
    bool okImport = mp.importInstrument(zx, msb, lsb, pc, name);

    juce::Logger::writeToLog(juce::String("manifest:") + (manifestPass ? "OK" : "FAIL"));
    juce::Logger::writeToLog(juce::String("zxi:") + (okImport ? "OK" : "FAIL"));
    return (manifestPass && okImport) ? 0 : 1;
}
