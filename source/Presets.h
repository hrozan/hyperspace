#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <vector>

namespace Presets
{
struct Preset
{
    juce::String name;
    std::vector<std::pair<juce::String, float>> values; // parameter id -> denormalised value
};

const std::vector<Preset>& factory();

void apply (juce::AudioProcessorValueTreeState&, int index);
}
