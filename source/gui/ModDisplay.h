#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class HyperspaceProcessor;

/** The modulation LFO drawn twice, separated by SPREAD, under a fixed playhead. */
class ModDisplay final : public juce::Component, private juce::Timer
{
public:
    explicit ModDisplay (HyperspaceProcessor&);

    void paint (juce::Graphics&) override;

private:
    void timerCallback() override { repaint(); }

    HyperspaceProcessor& proc;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModDisplay)
};
