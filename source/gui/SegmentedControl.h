#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

/** Row of mutually exclusive buttons. Selection is driven by the owner. */
class SegmentedControl final : public juce::Component
{
public:
    explicit SegmentedControl (const juce::StringArray& options);

    void setSelectedIndex (int index);
    int getSelectedIndex() const noexcept { return selected; }

    void setItems (const juce::StringArray& options);

    /** Recolours the selected cell, for a row that names the modulation mode. */
    void setAccentColour (juce::Colour);

    std::function<void (int)> onSelect;

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    int indexAt (juce::Point<float>) const;

    juce::StringArray items;
    int selected = 0;
    juce::Colour accent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SegmentedControl)
};

/** A SegmentedControl bound to one choice or bool parameter. */
class ParameterSegment final : public juce::Component
{
public:
    ParameterSegment (juce::RangedAudioParameter&, const juce::StringArray& options);

    void setAccentColour (juce::Colour c) { segment.setAccentColour (c); }

    void resized() override { segment.setBounds (getLocalBounds()); }

private:
    SegmentedControl segment;
    juce::ParameterAttachment attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParameterSegment)
};
