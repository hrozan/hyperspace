#include "LevelMeter.h"

#include <juce_audio_basics/juce_audio_basics.h>

#include "Theme.h"

LevelMeter::LevelMeter (std::function<float (int)> source)
    : getPeak (std::move (source))
{ startTimerHz (30); }

void LevelMeter::timerCallback()
{
    bool changed = false;

    for (int c = 0; c < 2; ++c)
    {
        const auto peak = getPeak != nullptr ? getPeak (c) : 0.0f;
        const auto previous = displayed[static_cast<size_t> (c)];

        // snap up, fall away slowly
        const auto next = peak > previous ? peak : previous * 0.80f;

        if (std::abs (next - previous) > 1.0e-4f)
        {
            displayed[static_cast<size_t> (c)] = next;
            changed = true;
        }
    }

    if (changed)
        repaint();
}

void LevelMeter::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const auto columnWidth = 9.0f;
    const auto gap = 4.0f;

    for (int c = 0; c < 2; ++c)
    {
        const auto column = juce::Rectangle<float> (bounds.getX() + static_cast<float> (c) * (columnWidth + gap),
                                                    bounds.getY(), columnWidth, bounds.getHeight());

        g.setColour (juce::Colour (0xff0a0a0d));
        g.fillRoundedRectangle (column, 2.0f);
        g.setColour (juce::Colours::black);
        g.drawRoundedRectangle (column.reduced (0.5f), 2.0f, 1.0f);

        const auto db = juce::Decibels::gainToDecibels (displayed[static_cast<size_t> (c)], -60.0f);
        const auto lit = juce::roundToInt (
            juce::jmap (juce::jlimit (-48.0f, 6.0f, db), -48.0f, 6.0f, 0.0f, static_cast<float> (segments)));

        const auto inner = column.reduced (2.0f);
        const auto cell = inner.getHeight() / static_cast<float> (segments);

        for (int s = 0; s < segments; ++s)
        {
            const auto seg = juce::Rectangle<float> (
                inner.getX(), inner.getBottom() - static_cast<float> (s + 1) * cell, inner.getWidth(), cell - 2.0f);

            if (s < lit)
            {
                const auto colour = s > segments - 3 ? Theme::saber : Theme::teal;
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
}
