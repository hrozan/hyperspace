#include "ModDisplay.h"

#include "../Parameters.h"
#include "../PluginProcessor.h"
#include "Theme.h"

namespace
{
constexpr float windowCycles = 2.0f;
constexpr float playheadAt = 0.42f;
constexpr float swing = 0.30f;

/** Mirrors ReverbEngine::lfoValue; the engine's own state is not published. */
float lfoShape (int shape, float phase)
{
    const auto p = phase - std::floor (phase);

    switch (shape)
    {
    case 1: return 1.0f - std::abs (2.0f * p - 1.0f);

    case 2:
    {
        const auto hash = [] (float n) {
            auto h = static_cast<uint32_t> (static_cast<int32_t> (n) + 1013904223) * 2654435761u;
            h ^= h >> 15;
            h *= 2246822519u;
            h ^= h >> 13;
            return static_cast<float> (h & 0xffffffu) / static_cast<float> (0xffffffu);
        };

        const auto cycle = std::floor (phase);
        const auto a = hash (cycle), b = hash (cycle + 1.0f);
        return a + (b - a) * (p * p * (3.0f - 2.0f * p));
    }

    default: return 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * p);
    }
}

float valueOf (juce::AudioProcessorValueTreeState& state, const char* id, float fallback)
{
    if (auto* v = state.getRawParameterValue (id))
        return v->load();

    return fallback;
}

/**
    What each mode draws on top of its colour. Colour alone is easy to miss, and these
    are the differences between the three that matter: whether there is a dry path to
    beat against, and what the LFO is actually moving.
*/
struct ModeStyle
{
    const char* name;
    const char* note;
    bool dry;     // the unmodulated path, drawn as the line it is
    bool fill;    // the band between the tail and its resting pitch
    bool notches; // markers riding the sweep
};

ModeStyle styleFor (int mode)
{
    switch (mode)
    {
    case 1: return { "VIBRATO", "MODULATED TAIL ONLY", false, true, false };
    case 2: return { "PHASER", "DRY + SWEPT NOTCHES", true, false, true };
    default: return { "CHORUS", "DRY + DETUNED TAIL", true, false, false };
    }
}
} // namespace

ModDisplay::ModDisplay (HyperspaceProcessor& processor)
    : proc (processor)
{
    setInterceptsMouseClicks (false, false);
    startTimerHz (30);
}

void ModDisplay::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff101016), bounds.getCentre(), juce::Colour (0xff06060a),
                                             bounds.getBottomLeft(), true));
    g.fillRoundedRectangle (bounds, 4.0f);

    const auto r = bounds.reduced (2.0f);

    g.setColour (juce::Colours::white.withAlpha (0.04f));
    for (int i = 1; i < 4; ++i)
        g.drawVerticalLine (static_cast<int> (r.getX() + r.getWidth() * static_cast<float> (i) / 4.0f), r.getY(),
                            r.getBottom());

    auto& state = proc.getValueTreeState();

    const auto mode = juce::roundToInt (valueOf (state, ParamID::modMode, 0.0f));
    const auto shape = juce::roundToInt (valueOf (state, ParamID::modShape, 0.0f));
    const auto spread = valueOf (state, ParamID::modSpread, 80.0f) * 0.005f; // cycles
    const auto depth = valueOf (state, ParamID::modDepth, 58.0f) * 0.01f;
    const auto on = valueOf (state, ParamID::modOn, 1.0f) > 0.5f;

    const auto style = styleFor (mode);
    const auto palette = Theme::modePalette (mode);
    const auto alpha = on ? 1.0f : 0.45f;

    const auto phase = proc.getModPhase();
    const auto amplitude = r.getHeight() * 0.5f * swing * (0.3f + 0.7f * depth);

    const auto traceFor = [&] (float offset) {
        juce::Path path;
        const auto width = static_cast<int> (r.getWidth());

        for (int px = 0; px <= width; ++px)
        {
            const auto t = static_cast<float> (px) / juce::jmax (1.0f, r.getWidth());
            const auto v = lfoShape (shape, phase + offset + (t - playheadAt) * windowCycles);

            const auto x = r.getX() + static_cast<float> (px);
            const auto y = r.getCentreY() - (v * 2.0f - 1.0f) * amplitude;

            if (px == 0)
                path.startNewSubPath (x, y);
            else
                path.lineTo (x, y);
        }

        return path;
    };

    // Vibrato has no dry path at all, so the band between the tail and its resting
    // pitch is the whole signal; the other two beat the modulated path against a dry
    // one, and that dry path is drawn as the line it is.
    if (style.fill)
    {
        auto area = traceFor (0.0f);
        area.lineTo (r.getRight(), r.getCentreY());
        area.lineTo (r.getX(), r.getCentreY());
        area.closeSubPath();

        g.setColour (palette.colour.withAlpha (0.14f * alpha));
        g.fillPath (area);
    }

    if (style.dry)
    {
        g.setColour (juce::Colour (0xffd8d4cd).withAlpha (0.45f * alpha));
        g.drawHorizontalLine (static_cast<int> (r.getCentreY()), r.getX(), r.getRight());

        const auto tag = juce::Rectangle<float> (r.getX() + 5.0f, r.getCentreY() - 15.0f, 27.0f, 12.0f);
        g.setColour (juce::Colour (0xff06060a).withAlpha (0.74f));
        g.fillRoundedRectangle (tag, 2.0f);
        g.setColour (juce::Colour (0xffd8d4cd).withAlpha (0.6f * alpha));
        g.setFont (Theme::label (8.0f, true, 0.18f));
        g.drawText ("DRY", tag.toNearestInt(), juce::Justification::centred, false);
    }
    else
    {
        static constexpr float dashes[] = { 3.0f, 4.0f };
        g.setColour (palette.colour.withAlpha (0.25f * alpha));
        g.drawDashedLine ({ r.getX(), r.getCentreY(), r.getRight(), r.getCentreY() }, dashes, 2);
    }

    // The phaser sweeps notches rather than delay time, so markers ride the trace to
    // show where in the spectrum they currently sit.
    if (style.notches)
    {
        g.setColour (palette.light.withAlpha (0.5f * alpha));

        for (float x = r.getX() + 10.0f; x <= r.getRight(); x += 24.0f)
        {
            const auto t = (x - r.getX()) / juce::jmax (1.0f, r.getWidth());
            const auto v = lfoShape (shape, phase + (t - playheadAt) * windowCycles);
            const auto y = r.getCentreY() - (v * 2.0f - 1.0f) * amplitude;
            g.drawLine (x, y - 7.0f, x, y + 7.0f, 1.3f);
        }
    }

    for (const auto& pair : { std::pair<juce::Colour, float> { palette.colour, 0.0f },
                              std::pair<juce::Colour, float> { palette.light, spread } })
    {
        const auto path = traceFor (pair.second);

        g.setColour (pair.first.withAlpha (0.16f * alpha));
        g.strokePath (path, juce::PathStrokeType (8.0f, juce::PathStrokeType::curved));
        g.setColour (pair.first.withAlpha (alpha));
        g.strokePath (path, juce::PathStrokeType (2.1f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    const auto px = r.getX() + r.getWidth() * playheadAt;
    g.setColour (palette.light.withAlpha (0.5f * alpha));
    g.drawVerticalLine (static_cast<int> (px), r.getY(), r.getBottom());

    const auto dotY = r.getCentreY() - (lfoShape (shape, phase) * 2.0f - 1.0f) * amplitude;
    g.setColour (juce::Colours::white.withAlpha (alpha));
    g.fillEllipse (juce::Rectangle<float> (6.5f, 6.5f).withCentre ({ px, dotY }));

    // The badge says it in words too, over its own backing so it stays legible
    // wherever the trace happens to be.
    const auto badge = juce::Rectangle<float> (108.0f, 26.0f).withRightX (r.getRight() - 4.0f).withY (r.getY() + 3.0f);
    g.setColour (juce::Colour (0xff06060a).withAlpha (0.74f));
    g.fillRoundedRectangle (badge, 3.0f);

    auto text = badge.reduced (6.0f, 4.0f);
    g.setColour (palette.colour.withAlpha (alpha));
    g.setFont (Theme::label (10.0f, true, 0.22f));
    g.drawText (style.name, text.removeFromTop (10.0f).toNearestInt(), juce::Justification::centredRight, false);

    g.setColour (juce::Colour (0xff55524e));
    g.setFont (Theme::label (8.0f, true, 0.12f));
    g.drawText (style.note, text.toNearestInt(), juce::Justification::centredRight, false);

    g.setColour (juce::Colours::black);
    g.drawRoundedRectangle (bounds.reduced (0.5f), 4.0f, 1.0f);
}
