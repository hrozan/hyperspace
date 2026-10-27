#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

/** MXR-style knurled black knob with a glowing value arc. */
class HyperLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    HyperLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float startAngle, float endAngle,
                           juce::Slider&) override;
};
