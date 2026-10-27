#include "Parameters.h"

namespace
{
juce::String hzString (float v, int)
{
    return v < 1000.0f ? juce::String (v, v < 10.0f ? 2 : 0) + " Hz"
                       : juce::String (v / 1000.0f, 2) + " kHz";
}

juce::String percentString (float v, int) { return juce::String (juce::roundToInt (v)) + " %"; }
juce::String dbString      (float v, int) { return juce::String (v, 1) + " dB"; }

/** The pre amp stages read as trims, so they carry their sign either way. */
juce::String signedDbString (float v, int)
{
    return (v > 0.0f ? "+" : "") + juce::String (v, 1) + " dB";
}
juce::String msString      (float v, int) { return juce::String (juce::roundToInt (v)) + " ms"; }

juce::String secondsString (float v, int)
{
    return juce::String (v, v < 10.0f ? 2 : 1) + " s";
}
}

juce::AudioProcessorValueTreeState::ParameterLayout Params::createLayout()
{
    using APF   = juce::AudioParameterFloat;
    using APC   = juce::AudioParameterChoice;
    using APB   = juce::AudioParameterBool;
    using Range = juce::NormalisableRange<float>;
    using Attr  = juce::AudioParameterFloatAttributes;

    constexpr int v = 1; // parameter version hint

    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // ---- reverb engine ----
    layout.add (std::make_unique<APC> (juce::ParameterID { ParamID::reverbType, v }, "Type",
                                       Params::typeChoices, 1));

    layout.add (std::make_unique<APF> (juce::ParameterID { ParamID::decay, v }, "Decay",
                                       Range { 0.1f, 20.0f, 0.0f, 0.35f }, 4.8f,
                                       Attr().withStringFromValueFunction (secondsString)));

    layout.add (std::make_unique<APF> (juce::ParameterID { ParamID::size, v }, "Size",
                                       Range { 0.0f, 100.0f, 0.1f }, 66.0f,
                                       Attr().withStringFromValueFunction (percentString)));

    layout.add (std::make_unique<APF> (juce::ParameterID { ParamID::preDelay, v }, "Pre-Delay",
                                       Range { 0.0f, 250.0f, 0.1f }, 75.0f,
                                       Attr().withStringFromValueFunction (msString)));

    // One parameter for the whole row: MS, then each division.
    layout.add (std::make_unique<APC> (juce::ParameterID { ParamID::preDelaySync, v }, "Pre-Delay Sync",
                                       Params::preSyncChoices, 0));

    layout.add (std::make_unique<APF> (juce::ParameterID { ParamID::diffusion, v }, "Diffusion",
                                       Range { 0.0f, 100.0f, 0.1f }, 74.0f,
                                       Attr().withStringFromValueFunction (percentString)));

    layout.add (std::make_unique<APF> (juce::ParameterID { ParamID::earlyLate, v }, "Early/Late",
                                       Range { 0.0f, 100.0f, 0.1f }, 62.0f,
                                       Attr().withStringFromValueFunction (percentString)));

    // ---- tone and damping ----
    layout.add (std::make_unique<APF> (juce::ParameterID { ParamID::lowCut, v }, "Low Cut",
                                       Range { 20.0f, 1000.0f, 0.0f, 0.35f }, 90.0f,
                                       Attr().withStringFromValueFunction (hzString)));

    layout.add (std::make_unique<APF> (juce::ParameterID { ParamID::highCut, v }, "High Cut",
                                       Range { 1000.0f, 20000.0f, 0.0f, 0.35f }, 7400.0f,
                                       Attr().withStringFromValueFunction (hzString)));

    layout.add (std::make_unique<APF> (juce::ParameterID { ParamID::damping, v }, "Damping",
                                       Range { 0.0f, 100.0f, 0.1f }, 60.0f,
                                       Attr().withStringFromValueFunction (percentString)));

    layout.add (std::make_unique<APF> (juce::ParameterID { ParamID::tone, v }, "Tone",
                                       Range { -12.0f, 12.0f, 0.1f }, -1.2f,
                                       Attr().withStringFromValueFunction (dbString)));

    // ---- modulation ----
    layout.add (std::make_unique<APB> (juce::ParameterID { ParamID::modOn, v }, "Modulation", true));

    layout.add (std::make_unique<APC> (juce::ParameterID { ParamID::modMode, v }, "Mod Mode",
                                       Params::modModeChoices, 0));

    layout.add (std::make_unique<APF> (juce::ParameterID { ParamID::modRate, v }, "Mod Rate",
                                       Range { 0.01f, 10.0f, 0.0f, 0.3f }, 0.62f,
                                       Attr().withStringFromValueFunction (hzString)));

    layout.add (std::make_unique<APC> (juce::ParameterID { ParamID::modSync, v }, "Mod Sync",
                                       Params::rateSyncChoices, 0));

    layout.add (std::make_unique<APF> (juce::ParameterID { ParamID::modDepth, v }, "Mod Depth",
                                       Range { 0.0f, 100.0f, 0.1f }, 58.0f,
                                       Attr().withStringFromValueFunction (percentString)));

    layout.add (std::make_unique<APF> (juce::ParameterID { ParamID::modSpread, v }, "Mod Spread",
                                       Range { 0.0f, 100.0f, 0.1f }, 80.0f,
                                       Attr().withStringFromValueFunction (percentString)));

    layout.add (std::make_unique<APC> (juce::ParameterID { ParamID::modShape, v }, "Mod Shape",
                                       Params::modShapeChoices, 0));

    layout.add (std::make_unique<APC> (juce::ParameterID { ParamID::modTarget, v }, "Mod Target",
                                       Params::modTargetChoices, 0));

    layout.add (std::make_unique<APF> (juce::ParameterID { ParamID::modMix, v }, "Mod Mix",
                                       Range { 0.0f, 100.0f, 0.1f }, 50.0f,
                                       Attr().withStringFromValueFunction (percentString)));

    // ---- pre amp, in front of the reverb ----
    layout.add (std::make_unique<APB> (juce::ParameterID { ParamID::preAmpOn, v }, "Pre Amp", true));

    layout.add (std::make_unique<APF> (juce::ParameterID { ParamID::preGain, v }, "Pre Gain",
                                       Range { -12.0f, 24.0f, 0.1f }, 6.0f,
                                       Attr().withStringFromValueFunction (signedDbString)));

    layout.add (std::make_unique<APF> (juce::ParameterID { ParamID::preTone, v }, "Pre Tone",
                                       Range { -12.0f, 12.0f, 0.1f }, 2.0f,
                                       Attr().withStringFromValueFunction (signedDbString)));

    layout.add (std::make_unique<APF> (juce::ParameterID { ParamID::preOutput, v }, "Pre Output",
                                       Range { -24.0f, 12.0f, 0.1f }, -2.0f,
                                       Attr().withStringFromValueFunction (signedDbString)));

    // ---- tail and output ----
    layout.add (std::make_unique<APB> (juce::ParameterID { ParamID::freeze, v }, "Freeze", false));
    layout.add (std::make_unique<APB> (juce::ParameterID { ParamID::duckOn, v }, "Ducking", true));

    layout.add (std::make_unique<APF> (juce::ParameterID { ParamID::duck, v }, "Duck",
                                       Range { -24.0f, 0.0f, 0.1f }, -4.5f,
                                       Attr().withStringFromValueFunction (dbString)));

    layout.add (std::make_unique<APF> (juce::ParameterID { ParamID::mix, v }, "Mix",
                                       Range { 0.0f, 100.0f, 0.1f }, 38.0f,
                                       Attr().withStringFromValueFunction (percentString)));

    layout.add (std::make_unique<APF> (juce::ParameterID { ParamID::width, v }, "Width",
                                       Range { 0.0f, 200.0f, 0.1f }, 120.0f,
                                       Attr().withStringFromValueFunction (percentString)));

    layout.add (std::make_unique<APF> (juce::ParameterID { ParamID::level, v }, "Level",
                                       Range { -24.0f, 12.0f, 0.1f }, -1.5f,
                                       Attr().withStringFromValueFunction (dbString)));

    layout.add (std::make_unique<APB> (juce::ParameterID { ParamID::bypass, v }, "Bypass", false));

    return layout;
}
