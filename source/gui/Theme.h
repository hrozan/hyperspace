#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

/** Palette and type shared by every panel, taken from design/hyperspace-layout.png. */
namespace Theme
{
inline const juce::Colour background   { 0xff09090b };
inline const juce::Colour panelTop     { 0xff15151a };
inline const juce::Colour panelBottom  { 0xff0d0d11 };
inline const juce::Colour panelEdge    { 0xff000000 };
inline const juce::Colour panelHilite  { 0x12ffffff };
inline const juce::Colour displayBg    { 0xff07070a };

inline const juce::Colour teal         { 0xff15c6b2 };
inline const juce::Colour tealBright   { 0xff63ead9 };
inline const juce::Colour saber        { 0xffff2233 };

/** One hue per modulation mode, all at the teal's brightness so the chassis holds. */
inline const juce::Colour chorus       { 0xff3d9bff };
inline const juce::Colour chorusLight  { 0xff8fc4ff };
inline const juce::Colour vibrato      { 0xffa97bff };
inline const juce::Colour vibratoLight { 0xffcdb0ff };
inline const juce::Colour phaser       { 0xffff8a3d };
inline const juce::Colour phaserLight  { 0xffffb77a };

struct ModePalette
{
    juce::Colour colour, light;
};

/** Index order matches the modMode parameter: Chorus, Vibrato, Phaser. */
inline ModePalette modePalette (int modeIndex)
{
    switch (modeIndex)
    {
        case 1:  return { vibrato, vibratoLight };
        case 2:  return { phaser,  phaserLight };
        default: return { chorus,  chorusLight };
    }
}

inline const juce::Colour text         { 0xffeceae6 };
inline const juce::Colour textDim      { 0xff79766f };
inline const juce::Colour knobTrack    { 0xff25252c };

/** How far a switched-off panel fades; the settings stay readable underneath. */
inline constexpr float disabledAlpha = 0.4f;

inline juce::Font displayFont (float height, float tracking = 0.0f)
{
    juce::Font f { juce::FontOptions ("Futura", height, juce::Font::plain) };
    return tracking > 0.0f ? f.withExtraKerningFactor (tracking) : f;
}

inline juce::Font label (float height, bool bold = true, float tracking = 0.18f)
{
    juce::Font f { juce::FontOptions ("Avenir Next Condensed", height,
                                      bold ? juce::Font::bold : juce::Font::plain) };
    return f.withExtraKerningFactor (tracking);
}
}
