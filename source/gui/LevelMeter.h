#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

/** Pair of segmented output meters, matching the L/R columns in the mockup. */
class LevelMeter final : public juce::Component, private juce::Timer
{
public:
    static constexpr int segments = 14;

    /** `source` returns the current peak (linear) for a channel index. */
    explicit LevelMeter (std::function<float (int)> source);

    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;

    std::function<float (int)> getPeak;
    std::array<float, 2> displayed { 0.0f, 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LevelMeter)
};
