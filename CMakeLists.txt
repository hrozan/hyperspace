cmake_minimum_required(VERSION 3.22)

project(Hyperspace VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Emit compile_commands.json for clangd and other LSP clients.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Build architectures")
endif()

# Symlink the generated database to the project root so clangd finds it no matter
# what the build directory is called.
if(CMAKE_EXPORT_COMPILE_COMMANDS AND NOT CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    file(CREATE_LINK
        "${CMAKE_BINARY_DIR}/compile_commands.json"
        "${CMAKE_SOURCE_DIR}/compile_commands.json"
        SYMBOLIC)
endif()

# CI builds package the artefacts instead of installing them into the plugin folders.
option(HYPERSPACE_INSTALL_AFTER_BUILD
       "Copy the plugin into the user plugin folders after a successful build" ON)

# Neither DSP stage depends on JUCE, so the tests can be configured and run without
# fetching it. CI's per-commit test job turns the plugin off and builds only those.
option(HYPERSPACE_BUILD_PLUGIN "Build the JUCE plugin targets" ON)

enable_testing()

add_executable(HyperspaceDspTests
    tests/DspTests.cpp
    source/PreAmp.cpp
    source/ReverbEngine.cpp)

target_compile_features(HyperspaceDspTests PRIVATE cxx_std_20)

add_test(NAME dsp COMMAND HyperspaceDspTests)

if(NOT HYPERSPACE_BUILD_PLUGIN)
    return()
endif()

include(FetchContent)

FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.15
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(Hyperspace
    PRODUCT_NAME                "Hyperspace"
    COMPANY_NAME                "Rozan Audio"
    BUNDLE_ID                   "com.rozanaudio.hyperspace"
    PLUGIN_MANUFACTURER_CODE    Rzan
    PLUGIN_CODE                 Hypr
    FORMATS                     VST3 Standalone
    VST3_CATEGORIES             Fx Reverb
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     ${HYPERSPACE_INSTALL_AFTER_BUILD})

target_sources(Hyperspace PRIVATE
    source/Parameters.cpp
    source/PreAmp.cpp
    source/ReverbEngine.cpp
    source/Presets.cpp
    source/PresetManager.cpp
    source/PluginProcessor.cpp
    source/PluginEditor.cpp
    source/gui/DriveMeter.cpp
    source/gui/HyperLookAndFeel.cpp
    source/gui/IconButton.cpp
    source/gui/Knob.cpp
    source/gui/LevelMeter.cpp
    source/gui/ModDisplay.cpp
    source/gui/PanelSection.cpp
    source/gui/ResponseDisplay.cpp
    source/gui/SegmentedControl.cpp
    source/gui/TailDisplay.cpp
    source/gui/ToggleSwitch.cpp)

target_compile_definitions(Hyperspace PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(Hyperspace PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
  PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)
