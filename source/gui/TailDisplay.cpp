#include "TailDisplay.h"

#include "../Parameters.h"
#include "../PluginProcessor.h"
#include "Theme.h"

namespace
{
float valueOf (juce::AudioProcessorValueTreeState& state, const char* id, float fallback)
{
    if (auto* v = state.getRawParameterValue (id))
        return v->load();

    return fallback;
}

/** The early reflection pattern the engine actually uses, as fractions of the spread. */
constexpr std::array<float, 7> tapPosition { 0.0f, 0.17f, 0.31f, 0.46f, 0.58f, 0.72f, 0.86f };
constexpr std::array<float, 7> tapHeight { 0.92f, 0.74f, 0.83f, 0.6f, 0.68f, 0.5f, 0.57f };
} // namespace

TailDisplay::TailDisplay (HyperspaceProcessor& processor)
    : proc (processor)
{
    setInterceptsMouseClicks (false, false);
    startTimerHz (15);
}

void TailDisplay::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff101016), bounds.getCentre(), juce::Colour (0xff06060a),
                                             bounds.getBottomLeft(), true));
    g.fillRoundedRectangle (bounds, 4.0f);

    const auto r = bounds.reduced (2.0f);
    const auto mid = r.getY() + r.getHeight() * 0.52f;

    auto& state = proc.getValueTreeState();

    const auto decay = valueOf (state, ParamID::decay, 4.8f);
    const auto size = valueOf (state, ParamID::size, 66.0f) * 0.01f;
    const auto earlyLate = valueOf (state, ParamID::earlyLate, 62.0f) * 0.01f;
    const auto modDepth = valueOf (state, ParamID::modDepth, 58.0f) * 0.01f;
    const auto modOn = valueOf (state, ParamID::modOn, 1.0f) > 0.5f;
    const auto frozen = valueOf (state, ParamID::freeze, 0.0f) > 0.5f;
    const auto preMs = proc.getEffectivePreDelayMs();

    // The window holds the pre-delay plus the whole decay, so the picture rescales
    // with the controls rather than showing a fixed slice. Time runs on a square-root
    // axis: seconds of tail would otherwise squeeze the pre-delay and the early
    // reflections, which are tens of milliseconds, into the first few pixels.
    const auto windowSeconds = juce::jmax (0.3f, preMs * 0.001f + decay);
    const auto toX = [&] (float seconds) {
        return r.getX() + r.getWidth() * std::sqrt (juce::jmax (0.0f, seconds) / windowSeconds);
    };
    const auto toSeconds = [&] (float x) {
        const auto t = (x - r.getX()) / r.getWidth();
        return t * t * windowSeconds;
    };

    const auto preX = toX (preMs * 0.001f);
    const auto earlySpread = 0.02f + 0.09f * size;
    const auto earlyEnd = toX (preMs * 0.001f + earlySpread);

    g.setColour (juce::Colours::white.withAlpha (0.07f));
    g.drawHorizontalLine (static_cast<int> (mid), r.getX(), r.getRight());

    g.setColour (juce::Colours::white.withAlpha (0.025f));
    g.fillRect (juce::Rectangle<float> (r.getX(), r.getY(), preX - r.getX(), r.getHeight()));

    static constexpr float dashes[] = { 2.0f, 3.0f };
    g.setColour (Theme::teal.withAlpha (0.35f));
    g.drawDashedLine ({ preX, r.getY(), preX, r.getBottom() }, dashes, 2);

    const auto headroom = (mid - r.getY()) * 0.86f;
    const auto earlyGain = juce::jmin (1.0f, 1.3f * (1.0f - earlyLate) + 0.25f);

    // ---- early reflections ----
    g.setColour (Theme::tealBright.withAlpha (0.9f));

    for (size_t t = 0; t < tapPosition.size(); ++t)
    {
        const auto x = preX + (earlyEnd - preX) * tapPosition[t];
        const auto a = tapHeight[t] * headroom * earlyGain;
        g.drawLine (x, mid - a, x, mid + a * 0.8f, 1.6f);
    }

    // ---- late tail ----
    const auto lateGain = 0.35f + 0.65f * earlyLate;
    const auto rippleDepth = modOn ? 0.26f * modDepth : 0.0f;

    const auto envelopeAt = [&] (float seconds) {
        const auto decayed = frozen ? 1.0f : std::exp (-3.0f * seconds / juce::jmax (0.05f, decay));
        const auto ripple = 1.0f + rippleDepth * std::sin (seconds * 11.0f);
        return decayed * ripple * headroom * lateGain;
    };

    juce::Path upper, lower;
    const auto tailStart = toSeconds (earlyEnd);

    upper.startNewSubPath (earlyEnd, mid);
    lower.startNewSubPath (earlyEnd, mid);

    for (float x = earlyEnd; x <= r.getRight(); x += 2.0f)
    {
        const auto a = envelopeAt (toSeconds (x) - tailStart);
        upper.lineTo (x, mid - a);
        lower.lineTo (x, mid + a * 0.8f);
    }

    g.setColour (Theme::teal.withAlpha (0.16f));

    for (auto edge : { &upper, &lower })
    {
        juce::Path filled = *edge;
        filled.lineTo (r.getRight(), mid);
        filled.closeSubPath();
        g.fillPath (filled);
    }

    g.setColour (Theme::teal.withAlpha (0.85f));
    g.strokePath (upper, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved));
    g.strokePath (lower, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved));

    // texture inside the envelope, so the tail reads as noise rather than a shape
    uint32_t rng = 7;
    const auto nextRandom = [&rng] {
        rng = rng * 1103515245u + 12345u;
        return static_cast<float> ((rng >> 8) & 0xffffu) / 65535.0f;
    };

    g.setColour (Theme::tealBright.withAlpha (0.45f));

    for (float x = earlyEnd; x <= r.getRight(); x += 2.6f)
    {
        const auto a = envelopeAt (toSeconds (x) - tailStart) * (0.35f + 0.65f * nextRandom());
        g.drawLine (x, mid - a, x, mid + a * 0.8f, 0.9f);
    }

    g.setColour (juce::Colour (0xff4e4c48));
    g.setFont (Theme::label (8.0f, true, 0.2f));
    g.drawText (frozen ? juce::String ("FROZEN")
                       : juce::String (juce::CharPointer_UTF8 ("PRE-DELAY \xc2\xb7 EARLY \xc2\xb7 TAIL")),
                r.reduced (7.0f, 5.0f), juce::Justification::topRight, false);

    g.setColour (juce::Colours::black);
    g.drawRoundedRectangle (bounds.reduced (0.5f), 4.0f, 1.0f);
}
