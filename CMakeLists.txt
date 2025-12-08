cmake_minimum_required(VERSION 3.22)
project(ZeusXVawApp VERSION 0.1.0)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Copy compile_commands.json to source dir for clangd
if (EXISTS "${CMAKE_BINARY_DIR}/compile_commands.json")
    file(COPY "${CMAKE_BINARY_DIR}/compile_commands.json" DESTINATION "${CMAKE_SOURCE_DIR}")
endif()
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(JUCE_ENABLE_MODULE_SOURCE_GROUPS ON)
set(JUCE_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(JUCE_BUILD_EXTRAS OFF CACHE BOOL "" FORCE)

set(VST3_SDK_PATH "${CMAKE_SOURCE_DIR}/Vendor/VST3_SDK")
set(JUCE_VST3_SDK_DIR "${VST3_SDK_PATH}" CACHE PATH "Path to VST3 SDK" FORCE)

add_subdirectory(Vendor/JUCE)

juce_add_gui_app(ZeusXVawApp
    COMPANY_NAME "ZEUS"
    BUNDLE_ID com.zeus.zeusxvaw.app
    PRODUCT_NAME "ZEUS-X VAW"
)

juce_generate_juce_header(ZeusXVawApp)

target_sources(ZeusXVawApp PRIVATE
    src/Main.cpp
    src/MainComponent.h
    src/MainComponent.cpp
    src/ui/ZXTheme.h
    src/ui/MixerStrip.h
    src/ui/MixerStrip.cpp
    src/ui/MixerPage.h
    src/ui/MixerPage.cpp
    src/ui/CategoryPopup.h
    src/ui/StudioPage.h
    src/ui/ZXLookAndFeel.h
    src/ui/ZXTagLabel.h
    src/ui/BusSendPanel.h
    src/ui/PluginManager.h
    src/ui/PluginBrowserPanel.h
    src/ui/TopBar.h
    src/audio/AudioEngine.h
    src/audio/InstrumentHost.h
    src/audio/MappingSamplerProcessor.h
    src/audio/MappingSamplerProcessor.cpp
    src/core/SoundLibrary.h
    src/core/SoundLibrary.cpp
    src/core/Assets.h
    src/core/Assets.cpp
)

target_link_libraries(ZeusXVawApp PRIVATE
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_dsp
    "-framework AudioUnit"
    "-framework CoreAudioKit"
)

target_compile_definitions(ZeusXVawApp PRIVATE
    JUCE_PLUGINHOST_AU=1
    JUCE_PLUGINHOST_VST3=1
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
)
set(ASSETS_DIR "${CMAKE_SOURCE_DIR}/assets")
add_custom_command(TARGET ZeusXVawApp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:ZeusXVawApp>/../Resources/assets/icons"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${ASSETS_DIR}/icons" "$<TARGET_FILE_DIR:ZeusXVawApp>/../Resources/assets/icons"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_BINARY_DIR}/compile_commands.json" "${CMAKE_SOURCE_DIR}/compile_commands.json"
)

add_executable(ZXTests
    tests/SoundLibraryTest.cpp
)


target_link_libraries(ZXTests PRIVATE
    juce::juce_core
    juce::juce_data_structures
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_dsp
    "-framework AudioUnit"
    "-framework CoreAudioKit"
)
target_include_directories(ZXTests PRIVATE
    ${CMAKE_BINARY_DIR}/ZeusXVawApp_artefacts/JuceLibraryCode
)
target_sources(ZXTests PRIVATE
    src/core/SoundLibrary.cpp
    src/core/Assets.cpp
    src/audio/MappingSamplerProcessor.cpp
)
target_compile_definitions(ZXTests PRIVATE
    JUCE_PLUGINHOST_AU=1
    JUCE_PLUGINHOST_VST3=1
)
