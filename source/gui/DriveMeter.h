#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

/** Horizontal bar under the pre amp, reading how hard the stage is being driven. */
class DriveMeter final : public juce::Component, private juce::Timer
{
public:
    static constexpr int segments = 18;

    /** `source` returns the current drive level, linear, where 1.0 is full scale. */
    explicit DriveMeter (std::function<float()> source);

    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;

    std::function<float()> getDrive;
    float displayed = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DriveMeter)
};
