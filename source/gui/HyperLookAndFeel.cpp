#include "HyperLookAndFeel.h"

#include "Theme.h"

HyperLookAndFeel::HyperLookAndFeel()
{
    setColour (juce::Slider::rotarySliderFillColourId, Theme::teal);
    setColour (juce::Slider::rotarySliderOutlineColourId, Theme::knobTrack);
    setColour (juce::Label::textColourId, Theme::textDim);
    setColour (juce::TooltipWindow::backgroundColourId, Theme::displayBg);
    setColour (juce::TooltipWindow::textColourId, Theme::text);
}

void HyperLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                                         float startAngle, float endAngle, juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat();
    const auto size = juce::jmin (bounds.getWidth(), bounds.getHeight());
    const auto centre = bounds.getCentre();
    const auto radius = size * 0.5f;
    const auto angle = startAngle + sliderPos * (endAngle - startAngle);

    const auto accent = slider.findColour (juce::Slider::rotarySliderFillColourId);

    // ---- value ring ----
    // The ring is inset by half its own thickness so the stroke stays inside the
    // component; a wider glow pass would otherwise be clipped flat on the outside.
    const auto thickness = juce::jmax (2.0f, size * 0.052f);
    const auto glow = thickness * 1.9f;
    const auto ringR = radius - glow * 0.5f - 1.0f;

    juce::Path track;
    track.addCentredArc (centre.x, centre.y, ringR, ringR, 0.0f, startAngle, endAngle, true);
    g.setColour (Theme::knobTrack);
    g.strokePath (track, juce::PathStrokeType (thickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    if (sliderPos > 0.004f)
    {
        juce::Path fill;
        fill.addCentredArc (centre.x, centre.y, ringR, ringR, 0.0f, startAngle, angle, true);

        // butt caps on the glow, so the arc start does not bloom into a blob
        g.setColour (accent.withAlpha (0.18f));
        g.strokePath (fill, juce::PathStrokeType (glow, juce::PathStrokeType::curved, juce::PathStrokeType::butt));
        g.setColour (accent);
        g.strokePath (fill,
                      juce::PathStrokeType (thickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // ---- knurled cap ----
    const auto capR = radius * 0.66f;
    const auto cap = juce::Rectangle<float> (capR * 2.0f, capR * 2.0f).withCentre (centre);

    g.setColour (juce::Colour (0xff0b0b0e));
    g.fillEllipse (cap);

    const int teeth = 26;
    for (int i = 0; i < teeth; ++i)
    {
        const auto a = juce::MathConstants<float>::twoPi * static_cast<float> (i) / static_cast<float> (teeth);
        const auto inner = capR * 0.88f;
        juce::Path tooth;
        tooth.startNewSubPath (centre.x + std::sin (a) * inner, centre.y - std::cos (a) * inner);
        tooth.lineTo (centre.x + std::sin (a) * capR, centre.y - std::cos (a) * capR);
        g.setColour (juce::Colour (0xff26262d).withAlpha (i % 2 == 0 ? 0.95f : 0.35f));
        g.strokePath (tooth, juce::PathStrokeType (juce::jmax (1.0f, size * 0.022f)));
    }

    // ---- glossy face ----
    const auto face = cap.reduced (capR * 0.14f);
    juce::ColourGradient body { juce::Colour (0xff4a4a52),
                                face.getX() + face.getWidth() * 0.34f,
                                face.getY() + face.getHeight() * 0.22f,
                                juce::Colour (0xff070709),
                                face.getCentreX(),
                                face.getBottom(),
                                true };
    body.addColour (0.36, juce::Colour (0xff18181d));
    g.setGradientFill (body);
    g.fillEllipse (face);

    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.drawEllipse (face, 1.0f);

    // specular highlight
    const auto spec =
        juce::Rectangle<float> (face.getWidth() * 0.46f, face.getHeight() * 0.30f)
            .withCentre ({ face.getCentreX() - face.getWidth() * 0.13f, face.getY() + face.getHeight() * 0.20f });
    g.setGradientFill ({ juce::Colours::white.withAlpha (0.34f), spec.getCentreX(), spec.getY(),
                         juce::Colours::transparentWhite, spec.getCentreX(), spec.getBottom(), false });
    g.fillEllipse (spec);

    // ---- pointer ----
    const auto pointerLength = capR * 0.80f;
    const auto pointerWidth = juce::jmax (1.6f, size * 0.038f);

    juce::Path pointer;
    pointer.addRoundedRectangle (-pointerWidth * 0.5f, -pointerLength, pointerWidth, pointerLength * 0.74f,
                                 pointerWidth * 0.5f);
    pointer.applyTransform (juce::AffineTransform::rotation (angle).translated (centre));

    g.setColour (juce::Colours::white.withAlpha (0.35f));
    g.fillPath (pointer);
    g.setColour (juce::Colour (0xfff4f1ec));
    g.fillPath (pointer);
}
