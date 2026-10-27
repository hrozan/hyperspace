#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <functional>
#include <vector>

/**
    Factory presets plus user presets stored as XML under the user's application
    data directory. Factory entries cannot be overwritten or deleted.
*/
class PresetManager
{
public:
    struct Entry
    {
        juce::String name;
        bool isFactory = true;
        juce::File file; // user presets only
    };

    explicit PresetManager (juce::AudioProcessorValueTreeState&);

    static juce::File directory();
    static juce::String fileExtension() { return ".hyperpreset"; }

    void refresh();

    const std::vector<Entry>& getEntries() const noexcept { return entries; }
    int getCurrentIndex() const noexcept { return current; }
    juce::String getCurrentName() const;
    bool currentIsFactory() const;

    void load (int index);
    void step (int delta);

    /** Writes the current parameter state as a new user preset and selects it. */
    bool saveAs (const juce::String& name);
    bool overwriteCurrent();
    bool deleteCurrent();

    std::function<void()> onChange;

private:
    void notify() const
    {
        if (onChange != nullptr)
            onChange();
    }

    juce::AudioProcessorValueTreeState& state;
    std::vector<Entry> entries;
    int current = 0;
};
