#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

/** Lit-dot switch from the mockup: ON, FREEZE and DUCK all use it. */
class ToggleSwitch final : public juce::Component, public juce::SettableTooltipClient
{
public:
    ToggleSwitch (juce::RangedAudioParameter&, const juce::String& caption, juce::Colour accent);

    bool isOn() const noexcept { return on; }

    /** Called after the parameter changes, so a panel can fade with its switch. */
    std::function<void (bool)> onStateChange;

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    juce::String text;
    juce::Colour colour;
    bool on = false;

    juce::ParameterAttachment attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ToggleSwitch)
};
