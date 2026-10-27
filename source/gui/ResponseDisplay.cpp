#include "ResponseDisplay.h"

#include "../Parameters.h"
#include "../ReverbEngine.h"
#include "Theme.h"

namespace
{
constexpr float minHz = 20.0f, maxHz = 20000.0f;
constexpr float topDb = 10.0f, bottomDb = -38.0f;

float valueOf (juce::AudioProcessorValueTreeState& state, const char* id, float fallback)
{
    if (auto* v = state.getRawParameterValue (id))
        return v->load();

    return fallback;
}
}

ResponseDisplay::ResponseDisplay (juce::AudioProcessorValueTreeState& s) : state (s)
{
    setInterceptsMouseClicks (false, false);
    startTimerHz (15);
}

void ResponseDisplay::timerCallback()
{
    const auto newLow  = valueOf (state, ParamID::lowCut, lowCut);
    const auto newHigh = valueOf (state, ParamID::highCut, highCut);
    const auto newDamp = valueOf (state, ParamID::damping, damping * 100.0f) * 0.01f;
    const auto newTone = valueOf (state, ParamID::tone, tone);

    const auto changed = [] (float a, float b) { return std::abs (a - b) > 1.0e-4f; };

    if (changed (newLow, lowCut) || changed (newHigh, highCut)
        || changed (newDamp, damping) || changed (newTone, tone))
    {
        lowCut = newLow;
        highCut = newHigh;
        damping = newDamp;
        tone = newTone;
        repaint();
    }
}

float ResponseDisplay::magnitudeDb (float f) const
{
    const auto onePoleHp = [f] (float fc) { const auto r = f / fc; return r / std::sqrt (1.0f + r * r); };
    const auto onePoleLp = [f] (float fc) { const auto r = f / fc; return 1.0f / std::sqrt (1.0f + r * r); };

    // The damping filter sits inside the tank, so it shapes the tail on top of the cuts.
    const auto dampCutoff = ReverbEngine::dampingCutoffHz (damping);

    auto gain = onePoleHp (lowCut) * onePoleLp (highCut) * onePoleLp (dampCutoff);

    // Tone is a tilt around 700 Hz: the two halves move in opposite directions.
    const auto split = (f * f) / (f * f + 700.0f * 700.0f);
    const auto tilt = juce::Decibels::decibelsToGain (-tone) * (1.0f - split)
                        + juce::Decibels::decibelsToGain (tone) * split;

    return juce::Decibels::gainToDecibels (gain * tilt, -60.0f);
}

void ResponseDisplay::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff101016), bounds.getCentre(),
                                             juce::Colour (0xff06060a), bounds.getBottomLeft(), true));
    g.fillRoundedRectangle (bounds, 4.0f);

    const auto r = bounds.reduced (2.0f);

    g.setColour (juce::Colours::white.withAlpha (0.04f));
    for (int i = 1; i < 5; ++i)
        g.drawVerticalLine (static_cast<int> (r.getX() + r.getWidth() * static_cast<float> (i) / 5.0f),
                            r.getY(), r.getBottom());

    const auto unityY = r.getY() + r.getHeight() * (topDb / (topDb - bottomDb));

    static constexpr float dashes[] = { 3.0f, 4.0f };
    g.setColour (Theme::teal.withAlpha (0.22f));
    g.drawDashedLine ({ r.getX(), unityY, r.getRight(), unityY }, dashes, 2);

    juce::Path curve;
    const auto width = static_cast<int> (r.getWidth());

    for (int px = 0; px <= width; ++px)
    {
        const auto t = static_cast<float> (px) / juce::jmax (1.0f, r.getWidth());
        const auto hz = minHz * std::pow (maxHz / minHz, t);
        const auto db = juce::jlimit (bottomDb, topDb, magnitudeDb (hz));

        const auto x = r.getX() + static_cast<float> (px);
        const auto y = r.getY() + r.getHeight() * (topDb - db) / (topDb - bottomDb);

        if (px == 0)
            curve.startNewSubPath (x, y);
        else
            curve.lineTo (x, y);
    }

    auto filled = curve;
    filled.lineTo (r.getRight(), r.getBottom());
    filled.lineTo (r.getX(), r.getBottom());
    filled.closeSubPath();

    g.setColour (Theme::teal.withAlpha (0.08f));
    g.fillPath (filled);

    g.setColour (Theme::teal.withAlpha (0.18f));
    g.strokePath (curve, juce::PathStrokeType (7.0f, juce::PathStrokeType::curved));
    g.setColour (Theme::teal);
    g.strokePath (curve, juce::PathStrokeType (1.9f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
    g.setColour (Theme::tealBright.withAlpha (0.8f));
    g.strokePath (curve, juce::PathStrokeType (0.8f, juce::PathStrokeType::curved));

    g.setColour (juce::Colour (0xff4e4c48));
    g.setFont (Theme::label (8.0f, true, 0.2f));
    g.drawText ("RESPONSE", r.reduced (7.0f, 5.0f), juce::Justification::topRight, false);

    g.setColour (juce::Colours::black);
    g.drawRoundedRectangle (bounds.reduced (0.5f), 4.0f, 1.0f);
}
