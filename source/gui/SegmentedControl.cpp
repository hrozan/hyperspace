#include "SegmentedControl.h"

#include "Theme.h"

SegmentedControl::SegmentedControl (const juce::StringArray& options)
    : items (options), accent (Theme::teal)
{
}

void SegmentedControl::setAccentColour (juce::Colour c)
{
    if (accent != c)
    {
        accent = c;
        repaint();
    }
}

void SegmentedControl::setItems (const juce::StringArray& options)
{
    items = options;
    selected = juce::jlimit (0, juce::jmax (0, items.size() - 1), selected);
    repaint();
}

void SegmentedControl::setSelectedIndex (int index)
{
    const auto clamped = juce::jlimit (0, juce::jmax (0, items.size() - 1), index);

    if (clamped != selected)
    {
        selected = clamped;
        repaint();
    }
}

int SegmentedControl::indexAt (juce::Point<float> p) const
{
    if (items.isEmpty() || getWidth() <= 0)
        return 0;

    const auto w = static_cast<float> (getWidth()) / static_cast<float> (items.size());
    return juce::jlimit (0, items.size() - 1, static_cast<int> (p.x / w));
}

void SegmentedControl::mouseDown (const juce::MouseEvent& e)
{
    const auto index = indexAt (e.position);

    // Show the click straight away. If the host refuses the change, the parameter
    // callback puts the selection back where it belongs.
    setSelectedIndex (index);

    if (onSelect != nullptr)
        onSelect (index);
}

void SegmentedControl::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const auto corner = 3.5f;

    g.setColour (juce::Colour (0xff0a0a0d));
    g.fillRoundedRectangle (bounds, corner);

    const auto segWidth = bounds.getWidth() / static_cast<float> (juce::jmax (1, items.size()));

    for (int i = 0; i < items.size(); ++i)
    {
        const auto cell = juce::Rectangle<float> (bounds.getX() + segWidth * static_cast<float> (i),
                                                  bounds.getY(), segWidth, bounds.getHeight());

        if (i == selected)
        {
            juce::Path p;
            p.addRoundedRectangle (cell.getX(), cell.getY(), cell.getWidth(), cell.getHeight(),
                                   corner, corner,
                                   i == 0, i == items.size() - 1,
                                   i == 0, i == items.size() - 1);

            g.setColour (accent.withAlpha (0.30f));
            g.fillRoundedRectangle (cell.expanded (2.0f), corner + 2.0f);

            g.setGradientFill ({ accent.brighter (0.45f), cell.getX(), cell.getY(),
                                 accent, cell.getX(), cell.getBottom(), false });
            g.fillPath (p);
        }
        else if (i > 0)
        {
            g.setColour (juce::Colours::black);
            g.drawVerticalLine (static_cast<int> (cell.getX()), cell.getY(), cell.getBottom());
        }

        g.setColour (i == selected ? juce::Colour (0xff02120f) : juce::Colour (0xff6f6c66));
        g.setFont (Theme::label (10.0f, true, 0.14f));
        g.drawText (items[i], cell.toNearestInt(), juce::Justification::centred, false);
    }

    g.setColour (juce::Colours::black);
    g.drawRoundedRectangle (bounds.reduced (0.5f), corner, 1.0f);

    g.setColour (Theme::panelHilite);
    g.drawHorizontalLine (static_cast<int> (bounds.getY() + 1.0f), bounds.getX() + 2.0f, bounds.getRight() - 2.0f);
}

//==============================================================================
ParameterSegment::ParameterSegment (juce::RangedAudioParameter& parameter, const juce::StringArray& options)
    : segment (options),
      attachment (parameter,
                  [this] (float newValue) { segment.setSelectedIndex (juce::roundToInt (newValue)); },
                  nullptr)
{
    addAndMakeVisible (segment);
    segment.onSelect = [this] (int index) { attachment.setValueAsCompleteGesture (static_cast<float> (index)); };
    attachment.sendInitialUpdate();
}
