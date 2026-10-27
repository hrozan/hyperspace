#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

/** Small bevelled button carrying one of the preset icons. */
class IconButton final : public juce::Button
{
public:
    enum class Icon { plus, save, trash };

    IconButton (Icon, const juce::String& tooltip, juce::Colour accent);

    void paintButton (juce::Graphics&, bool highlighted, bool down) override;

private:
    Icon icon;
    juce::Colour accentColour;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (IconButton)
};
