#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "SyncMath.h"

namespace ParamID
{
// ---- reverb engine ----
inline constexpr auto reverbType = "reverbType";
inline constexpr auto decay = "decay";
inline constexpr auto size = "size";
inline constexpr auto preDelay = "preDelay";
inline constexpr auto preDelaySync = "preDelaySync";
inline constexpr auto diffusion = "diffusion";
inline constexpr auto earlyLate = "earlyLate";

// ---- tone and damping ----
inline constexpr auto lowCut = "lowCut";
inline constexpr auto highCut = "highCut";
inline constexpr auto damping = "damping";
inline constexpr auto tone = "tone";

// ---- modulation ----
inline constexpr auto modOn = "modOn";
inline constexpr auto modMode = "modMode";
inline constexpr auto modRate = "modRate";
inline constexpr auto modSync = "modSync";
inline constexpr auto modDepth = "modDepth";
inline constexpr auto modSpread = "modSpread";
inline constexpr auto modShape = "modShape";
inline constexpr auto modTarget = "modTarget";
inline constexpr auto modMix = "modMix";

// ---- pre amp ----
inline constexpr auto preAmpOn = "preAmpOn";
inline constexpr auto preGain = "preGain";
inline constexpr auto preTone = "preTone";
inline constexpr auto preOutput = "preOutput";

// ---- tail and output ----
inline constexpr auto freeze = "freeze";
inline constexpr auto duckOn = "duckOn";
inline constexpr auto duck = "duck";
inline constexpr auto mix = "mix";
inline constexpr auto width = "width";
inline constexpr auto level = "level";
inline constexpr auto bypass = "bypass";
} // namespace ParamID

namespace Params
{
inline const juce::StringArray typeChoices { "Room", "Hall", "Plate", "Spring", "Ambient" };
inline const juce::StringArray preSyncChoices { "MS", "1/16", "1/8", "1/4" };
inline const juce::StringArray modModeChoices { "Chorus", "Vibrato", "Phaser" };
inline const juce::StringArray modShapeChoices { "Sine", "Triangle", "Random" };
inline const juce::StringArray modTargetChoices { "Delay", "Filter", "Both" };
inline const juce::StringArray rateSyncChoices { "FREE", "1/2", "1/4", "1/8" };

juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
} // namespace Params
