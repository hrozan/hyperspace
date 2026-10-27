#include "DriveMeter.h"

#include <juce_audio_basics/juce_audio_basics.h>

#include "Theme.h"

DriveMeter::DriveMeter (std::function<float()> source)
    : getDrive (std::move (source))
{
    setInterceptsMouseClicks (false, false);
    startTimerHz (30);
}

void DriveMeter::timerCallback()
{
    const auto drive = getDrive != nullptr ? getDrive() : 0.0f;

    // snap up, fall away slowly, as the output meters do
    const auto next = drive > displayed ? drive : displayed * 0.82f;

    if (std::abs (next - displayed) > 1.0e-4f)
    {
        displayed = next;
        repaint();
    }
}

void DriveMeter::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();

    g.setColour (juce::Colour (0xff0a0a0d));
    g.fillRoundedRectangle (bounds, 3.0f);
    g.setColour (juce::Colours::black);
    g.drawRoundedRectangle (bounds.reduced (0.5f), 3.0f, 1.0f);

    const auto db = juce::Decibels::gainToDecibels (displayed, -60.0f);
    const auto lit = juce::roundToInt (juce::jmap (juce::jlimit (-24.0f, 6.0f, db),
                                                   -24.0f, 6.0f, 0.0f, static_cast<float> (segments)));

    const auto inner = bounds.reduced (2.0f);
    const auto cell = inner.getWidth() / static_cast<float> (segments);

    for (int s = 0; s < segments; ++s)
    {
        const auto seg = juce::Rectangle<float> (inner.getX() + static_cast<float> (s) * cell,
                                                 inner.getY(), cell - 2.0f, inner.getHeight());

        if (s < lit)
        {
            // the last stretch is where the saturation is doing audible work
            const auto colour = s > segments - 4 ? Theme::saber : Theme::teal;
            g.setColour (colour.withAlpha (0.35f));
            g.fillRoundedRectangle (seg.expanded (1.5f), 1.0f);
            g.setColour (colour);
        }
        else
        {
            g.setColour (juce::Colour (0xff1e1e24));
        }

        g.fillRoundedRectangle (seg, 1.0f);
    }
}
