#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "PluginProcessor.h"
#include "gui/HyperLookAndFeel.h"

class HyperspaceEditor final : public juce::AudioProcessorEditor
{
public:
    /** Logical panel size; the editor scales this to fit whatever the host gives it. */
    static constexpr int designWidth = 1040;
    static constexpr int designHeight = 650;

    explicit HyperspaceEditor (HyperspaceProcessor&);
    ~HyperspaceEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    class Content;

    HyperLookAndFeel lookAndFeel;

    std::unique_ptr<Content> content;
    juce::ComponentBoundsConstrainer constrainer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HyperspaceEditor)
};
