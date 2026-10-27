#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class HyperspaceProcessor;

/**
    The shape of the tail the current settings produce: the pre-delay gap, the early
    reflection taps and the decay envelope with the modulation riding on it.
*/
class TailDisplay final : public juce::Component, private juce::Timer
{
public:
    explicit TailDisplay (HyperspaceProcessor&);

    void paint (juce::Graphics&) override;

private:
    void timerCallback() override { repaint(); }

    HyperspaceProcessor& proc;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TailDisplay)
};
