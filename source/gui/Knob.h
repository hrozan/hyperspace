#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

/** Rotary knob with the caption and formatted readout underneath, as in the mockup. */
class Knob final : public juce::Component
{
public:
    Knob (juce::RangedAudioParameter& parameter, const juce::String& caption, juce::Colour accent, bool hero = false);

    void resized() override;
    void paint (juce::Graphics&) override;

    /** Shows this instead of the parameter's own value; empty restores it. */
    void setOverrideText (const juce::String&);

    /** Greys the knob out and stops it responding, for a parameter the host overrides. */
    void setDimmed (bool);

    /** Recolours the value ring, for a control that follows the modulation mode. */
    void setAccentColour (juce::Colour);

private:
    void refreshReadout();

    juce::RangedAudioParameter& param;
    juce::String captionText;
    juce::String readout;
    juce::String overrideText;
    bool isHero;

    juce::Slider slider;
    juce::SliderParameterAttachment attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Knob)
};
