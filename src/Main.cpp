#include <JuceHeader.h>
#include "MainComponent.h"
#include "ui/ZXLookAndFeel.h"
#include "ui/PluginManager.h"
#include "audio/InstrumentHost.h"

class AppMenuModel : public juce::MenuBarModel {
public:
    enum { scanPluginsCmd = 1 };
    juce::StringArray getMenuBarNames() override { return { "Plugins" }; }
    juce::PopupMenu getMenuForIndex (int topLevelMenuIndex, const juce::String&) override {
        juce::PopupMenu m;
        if (topLevelMenuIndex == 0) m.addItem(scanPluginsCmd, "Scan Plugins");
        return m;
    }
    void menuItemSelected (int menuItemID, int topLevelMenuIndex) override {
        if (topLevelMenuIndex == 0 && menuItemID == scanPluginsCmd) PluginManager::instance().scan();
    }
};

class MainWindow : public juce::DocumentWindow {
public:
    MainWindow() : DocumentWindow("ZEUS-X VAW", juce::Colours::black, DocumentWindow::allButtons) {
        setUsingNativeTitleBar(true);
        setResizable(true, true);
        setContentOwned(new MainComponent(), true);
        centreWithSize(getWidth(), getHeight());
        setVisible(true);
    }
    void closeButtonPressed() override { juce::JUCEApplication::getInstance()->systemRequestedQuit(); }
};

class App : public juce::JUCEApplication {
public:
    const juce::String getApplicationName() override { return "ZEUS-X VAW"; }
    const juce::String getApplicationVersion() override { return "0.1.0"; }
    void initialise(const juce::String&) override {
        auto baseDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("ZEUS-X VAW Logs");
        baseDir.createDirectory();
        auto logFile = baseDir.getChildFile("app.log");
        juce::Logger::setCurrentLogger(new juce::FileLogger(logFile, "ZEUS-X VAW started", 0));
        PluginManager::instance().init();
        PluginManager::instance().loadCache();
        menuModel = std::make_unique<AppMenuModel>();
       #if JUCE_MAC
        juce::MenuBarModel::setMacMainMenu(menuModel.get());
       #endif
        mainWindow.reset(new MainWindow());
    }
    void shutdown() override {
       #if JUCE_MAC
        juce::MenuBarModel::setMacMainMenu(nullptr);
       #endif
        InstrumentHost::instance().shutdown();
        juce::Logger::setCurrentLogger(nullptr);
        mainWindow = nullptr;
        menuModel = nullptr;
    }
private:
    std::unique_ptr<MainWindow> mainWindow;
    std::unique_ptr<AppMenuModel> menuModel;
};

START_JUCE_APPLICATION(App)
