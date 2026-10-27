#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

/** Frequency response of the cuts, the tail damping and the tone tilt. */
class ResponseDisplay final : public juce::Component, private juce::Timer
{
public:
    explicit ResponseDisplay (juce::AudioProcessorValueTreeState&);

    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;
    float magnitudeDb (float frequencyHz) const;

    juce::AudioProcessorValueTreeState& state;
    float lowCut = 90.0f, highCut = 7400.0f, damping = 0.6f, tone = -1.2f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ResponseDisplay)
};
