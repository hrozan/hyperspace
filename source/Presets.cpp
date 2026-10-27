#include "Presets.h"

#include "Parameters.h"

const std::vector<Presets::Preset>& Presets::factory()
{
    // Choice parameters are stored as their index:
    //   type     0 Room, 1 Hall, 2 Plate, 3 Spring, 4 Ambient
    //   modMode  0 Chorus, 1 Vibrato, 2 Phaser
    //   modShape 0 Sine, 1 Triangle, 2 Random
    //   modTarget 0 Delay, 1 Filter, 2 Both
    //   the pre amp runs in front of the reverb, so its output trim sets the level
    //   going into the tank as well as the dry side of Mix
    static const std::vector<Preset> presets {
        { "INFINITE CORRIDOR",
          { { ParamID::reverbType, 1.0f }, { ParamID::decay, 4.8f },        { ParamID::size, 66.0f },
            { ParamID::preDelay, 75.0f },  { ParamID::preDelaySync, 0.0f }, { ParamID::diffusion, 74.0f },
            { ParamID::earlyLate, 62.0f }, { ParamID::lowCut, 90.0f },      { ParamID::highCut, 7400.0f },
            { ParamID::damping, 60.0f },   { ParamID::tone, -1.2f },        { ParamID::modOn, 1.0f },
            { ParamID::modMode, 0.0f },    { ParamID::modRate, 0.62f },     { ParamID::modSync, 0.0f },
            { ParamID::modDepth, 58.0f },  { ParamID::modSpread, 80.0f },   { ParamID::modShape, 0.0f },
            { ParamID::modTarget, 0.0f },  { ParamID::modMix, 50.0f },      { ParamID::preAmpOn, 1.0f },
            { ParamID::preGain, 6.0f },    { ParamID::preTone, 2.0f },      { ParamID::preOutput, -2.0f },
            { ParamID::freeze, 0.0f },     { ParamID::duckOn, 1.0f },       { ParamID::duck, -4.5f },
            { ParamID::mix, 38.0f },       { ParamID::width, 120.0f },      { ParamID::level, -1.5f } } },

        { "LIGHTSPEED BLOOM",
          { { ParamID::reverbType, 1.0f }, { ParamID::decay, 12.0f },       { ParamID::size, 92.0f },
            { ParamID::preDelay, 120.0f }, { ParamID::preDelaySync, 0.0f }, { ParamID::diffusion, 88.0f },
            { ParamID::earlyLate, 88.0f }, { ParamID::lowCut, 220.0f },     { ParamID::highCut, 9000.0f },
            { ParamID::damping, 45.0f },   { ParamID::tone, 2.5f },         { ParamID::modOn, 1.0f },
            { ParamID::modMode, 0.0f },    { ParamID::modRate, 0.28f },     { ParamID::modSync, 0.0f },
            { ParamID::modDepth, 72.0f },  { ParamID::modSpread, 100.0f },  { ParamID::modShape, 0.0f },
            { ParamID::modTarget, 2.0f },  { ParamID::modMix, 60.0f },      { ParamID::preAmpOn, 1.0f },
            { ParamID::preGain, 10.0f },   { ParamID::preTone, 4.0f },      { ParamID::preOutput, -4.0f },
            { ParamID::freeze, 0.0f },     { ParamID::duckOn, 1.0f },       { ParamID::duck, -9.0f },
            { ParamID::mix, 52.0f },       { ParamID::width, 150.0f },      { ParamID::level, -3.5f } } },

        { "CARBON CHAMBER",
          { { ParamID::reverbType, 0.0f }, { ParamID::decay, 1.1f },        { ParamID::size, 34.0f },
            { ParamID::preDelay, 18.0f },  { ParamID::preDelaySync, 0.0f }, { ParamID::diffusion, 62.0f },
            { ParamID::earlyLate, 35.0f }, { ParamID::lowCut, 140.0f },     { ParamID::highCut, 5200.0f },
            { ParamID::damping, 72.0f },   { ParamID::tone, -3.0f },        { ParamID::modOn, 1.0f },
            { ParamID::modMode, 2.0f },    { ParamID::modRate, 1.8f },      { ParamID::modSync, 0.0f },
            { ParamID::modDepth, 80.0f },  { ParamID::modSpread, 55.0f },   { ParamID::modShape, 1.0f },
            { ParamID::modTarget, 0.0f },  { ParamID::modMix, 50.0f },      { ParamID::preAmpOn, 1.0f },
            { ParamID::preGain, 14.0f },   { ParamID::preTone, -3.0f },     { ParamID::preOutput, -5.0f },
            { ParamID::freeze, 0.0f },     { ParamID::duckOn, 1.0f },       { ParamID::duck, -6.0f },
            { ParamID::mix, 30.0f },       { ParamID::width, 90.0f },       { ParamID::level, -1.0f } } },

        { "ION VIBRATO",
          { { ParamID::reverbType, 2.0f }, { ParamID::decay, 3.2f },        { ParamID::size, 50.0f },
            { ParamID::preDelay, 40.0f },  { ParamID::preDelaySync, 0.0f }, { ParamID::diffusion, 90.0f },
            { ParamID::earlyLate, 75.0f }, { ParamID::lowCut, 160.0f },     { ParamID::highCut, 11000.0f },
            { ParamID::damping, 38.0f },   { ParamID::tone, 1.0f },         { ParamID::modOn, 1.0f },
            { ParamID::modMode, 1.0f },    { ParamID::modRate, 4.5f },      { ParamID::modSync, 0.0f },
            { ParamID::modDepth, 30.0f },  { ParamID::modSpread, 65.0f },   { ParamID::modShape, 0.0f },
            { ParamID::modTarget, 0.0f },  { ParamID::modMix, 100.0f },     { ParamID::preAmpOn, 1.0f },
            { ParamID::preGain, 4.0f },    { ParamID::preTone, 1.0f },      { ParamID::preOutput, -1.0f },
            { ParamID::freeze, 0.0f },     { ParamID::duckOn, 0.0f },       { ParamID::duck, -4.5f },
            { ParamID::mix, 42.0f },       { ParamID::width, 135.0f },      { ParamID::level, -2.0f } } },

        { "SPRING RELAY",
          { { ParamID::reverbType, 3.0f }, { ParamID::decay, 2.2f },        { ParamID::size, 28.0f },
            { ParamID::preDelay, 8.0f },   { ParamID::preDelaySync, 0.0f }, { ParamID::diffusion, 46.0f },
            { ParamID::earlyLate, 55.0f }, { ParamID::lowCut, 300.0f },     { ParamID::highCut, 4400.0f },
            { ParamID::damping, 64.0f },   { ParamID::tone, -2.0f },        { ParamID::modOn, 0.0f },
            { ParamID::modMode, 0.0f },    { ParamID::modRate, 1.0f },      { ParamID::modSync, 0.0f },
            { ParamID::modDepth, 40.0f },  { ParamID::modSpread, 50.0f },   { ParamID::modShape, 0.0f },
            { ParamID::modTarget, 0.0f },  { ParamID::modMix, 50.0f },      { ParamID::preAmpOn, 1.0f },
            { ParamID::preGain, 8.0f },    { ParamID::preTone, -2.0f },     { ParamID::preOutput, -3.0f },
            { ParamID::freeze, 0.0f },     { ParamID::duckOn, 1.0f },       { ParamID::duck, -3.0f },
            { ParamID::mix, 28.0f },       { ParamID::width, 100.0f },      { ParamID::level, 0.0f } } },

        { "FROZEN STARFIELD",
          { { ParamID::reverbType, 4.0f },  { ParamID::decay, 20.0f },       { ParamID::size, 78.0f },
            { ParamID::preDelay, 0.0f },    { ParamID::preDelaySync, 0.0f }, { ParamID::diffusion, 100.0f },
            { ParamID::earlyLate, 100.0f }, { ParamID::lowCut, 260.0f },     { ParamID::highCut, 12000.0f },
            { ParamID::damping, 30.0f },    { ParamID::tone, 3.5f },         { ParamID::modOn, 1.0f },
            { ParamID::modMode, 0.0f },     { ParamID::modRate, 0.12f },     { ParamID::modSync, 0.0f },
            { ParamID::modDepth, 100.0f },  { ParamID::modSpread, 100.0f },  { ParamID::modShape, 2.0f },
            { ParamID::modTarget, 1.0f },   { ParamID::modMix, 70.0f },      { ParamID::preAmpOn, 0.0f },
            { ParamID::preGain, 0.0f },     { ParamID::preTone, 5.0f },      { ParamID::preOutput, 0.0f },
            { ParamID::freeze, 1.0f },      { ParamID::duckOn, 0.0f },       { ParamID::duck, -4.5f },
            { ParamID::mix, 60.0f },        { ParamID::width, 160.0f },      { ParamID::level, -4.0f } } },
    };

    return presets;
}

void Presets::apply (juce::AudioProcessorValueTreeState& state, int index)
{
    const auto& all = factory();

    if (!juce::isPositiveAndBelow (index, static_cast<int> (all.size())))
        return;

    for (const auto& [id, value] : all[static_cast<size_t> (index)].values)
        if (auto* p = state.getParameter (id))
            p->setValueNotifyingHost (p->convertTo0to1 (value));
}
