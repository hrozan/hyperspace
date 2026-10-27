#include "IconButton.h"

#include "Theme.h"

IconButton::IconButton (Icon i, const juce::String& tooltip, juce::Colour accent)
    : juce::Button (tooltip), icon (i), accentColour (accent)
{
    setTooltip (tooltip);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void IconButton::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    const auto bounds = getLocalBounds().toFloat();

    g.setGradientFill ({ juce::Colour (down ? 0xff0a0a0d : 0xff131318), bounds.getX(), bounds.getY(),
                         juce::Colour (down ? 0xff060608 : 0xff0a0a0d), bounds.getX(), bounds.getBottom(), false });
    g.fillRoundedRectangle (bounds, 4.0f);

    g.setColour (juce::Colours::black);
    g.drawRoundedRectangle (bounds.reduced (0.5f), 4.0f, 1.0f);

    g.setColour (Theme::panelHilite);
    g.drawHorizontalLine (static_cast<int> (bounds.getY() + 1.0f), bounds.getX() + 2.0f, bounds.getRight() - 2.0f);

    auto colour = accentColour;

    if (! isEnabled())
        colour = colour.withAlpha (0.25f);
    else if (highlighted)
        colour = colour.brighter (0.4f);

    g.setColour (colour);

    // icons are drawn on a 16x16 grid, then scaled into the button
    const auto area = bounds.reduced (bounds.getWidth() * 0.26f);
    const auto scale = area.getWidth() / 16.0f;
    const auto transform = juce::AffineTransform::scale (scale).translated (area.getX(), area.getY());

    juce::Path p;

    switch (icon)
    {
        case Icon::plus:
            p.startNewSubPath (8.0f, 2.6f);  p.lineTo (8.0f, 13.4f);
            p.startNewSubPath (2.6f, 8.0f);  p.lineTo (13.4f, 8.0f);
            break;

        case Icon::save:
            p.startNewSubPath (2.6f, 2.6f);
            p.lineTo (11.6f, 2.6f);
            p.lineTo (13.4f, 4.4f);
            p.lineTo (13.4f, 13.4f);
            p.lineTo (2.6f, 13.4f);
            p.closeSubPath();
            p.addRectangle (5.3f, 2.6f, 5.4f, 3.6f);
            p.addRectangle (4.9f, 9.0f, 6.2f, 4.4f);
            break;

        case Icon::trash:
            p.startNewSubPath (3.0f, 4.4f);  p.lineTo (13.0f, 4.4f);
            p.startNewSubPath (6.2f, 4.4f);  p.lineTo (6.2f, 2.9f);
            p.lineTo (9.8f, 2.9f);           p.lineTo (9.8f, 4.4f);
            p.startNewSubPath (4.3f, 4.4f);  p.lineTo (5.1f, 13.2f);
            p.lineTo (10.9f, 13.2f);         p.lineTo (11.7f, 4.4f);
            p.startNewSubPath (7.0f, 6.8f);  p.lineTo (7.0f, 10.9f);
            p.startNewSubPath (9.0f, 6.8f);  p.lineTo (9.0f, 10.9f);
            break;
    }

    p.applyTransform (transform);
    g.strokePath (p, juce::PathStrokeType (1.45f * scale, juce::PathStrokeType::curved,
                                           juce::PathStrokeType::rounded));
}
