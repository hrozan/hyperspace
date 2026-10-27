#include "ToggleSwitch.h"

#include "Theme.h"

ToggleSwitch::ToggleSwitch (juce::RangedAudioParameter& parameter, const juce::String& caption,
                            juce::Colour accent)
    : text (caption), colour (accent),
      attachment (parameter,
                  [this] (float v)
                  {
                      on = v > 0.5f;
                      repaint();

                      if (onStateChange != nullptr)
                          onStateChange (on);
                  },
                  nullptr)
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    setTooltip (parameter.getName (64));
    attachment.sendInitialUpdate();
}

void ToggleSwitch::mouseDown (const juce::MouseEvent&)
{
    // Paint the click straight away; the parameter callback corrects it if the
    // host refuses the change.
    attachment.setValueAsCompleteGesture (on ? 0.0f : 1.0f);
}

void ToggleSwitch::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();

    g.setGradientFill ({ juce::Colour (0xff131318), bounds.getX(), bounds.getY(),
                         juce::Colour (0xff0a0a0d), bounds.getX(), bounds.getBottom(), false });
    g.fillRoundedRectangle (bounds, 4.0f);

    g.setColour (juce::Colours::black);
    g.drawRoundedRectangle (bounds.reduced (0.5f), 4.0f, 1.0f);

    g.setColour (Theme::panelHilite);
    g.drawHorizontalLine (static_cast<int> (bounds.getY() + 1.0f), bounds.getX() + 2.0f, bounds.getRight() - 2.0f);

    const auto dot = juce::Rectangle<float> (6.0f, 6.0f)
                         .withCentre ({ bounds.getX() + 12.0f, bounds.getCentreY() });

    if (on)
    {
        g.setColour (colour.withAlpha (0.35f));
        g.fillEllipse (dot.expanded (4.0f));
        g.setColour (colour);
    }
    else
    {
        g.setColour (juce::Colour (0xff1e1e24));
    }

    g.fillEllipse (dot);

    g.setColour (on ? colour.brighter (0.5f) : juce::Colour (0xff6f6c66));
    g.setFont (Theme::label (9.0f, true, 0.2f));
    g.drawText (text, bounds.withTrimmedLeft (20.0f).toNearestInt(),
                juce::Justification::centredLeft, false);
}
