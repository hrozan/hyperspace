#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

/** Bevelled panel with the teal title bar used by every section. */
class PanelSection final : public juce::Component
{
public:
    /** `headerHeight` leaves room for whatever the owner puts in the title bar. */
    explicit PanelSection (const juce::String& title, int headerHeight = 20, bool showLed = true);

    void paint (juce::Graphics&) override;

    /** Fades the panel's own title while the section is switched off. */
    void setActive (bool);

    juce::Rectangle<int> getContentBounds() const;
    juce::Rectangle<int> getHeaderBounds() const;

private:
    juce::String titleText;
    int header;
    bool led;
    bool active = true;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PanelSection)
};
