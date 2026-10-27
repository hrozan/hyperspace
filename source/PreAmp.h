#pragma once

#include <array>
#include <atomic>

/**
    Console-style input stage in front of the reverb: gain into a soft saturation,
    a tilt, then an output trim.

    Like ReverbEngine it depends on no JUCE types, so it can be compiled and tested
    on its own. process() works in place on raw channel pointers.
*/
class PreAmp
{
public:
    static constexpr int maxChannels = 2;

    struct Settings
    {
        bool  enabled   = true;
        float gainDb    = 6.0f;    // drive into the saturation
        float toneDb    = 2.0f;    // tilt around 700 Hz, negative is darker
        float outputDb  = -2.0f;   // trim after it
    };

    void prepare (double sampleRate, int numChannels);
    void reset();

    void setSettings (const Settings& s) noexcept { target = s; }

    void process (float* const* channels, int numChannels, int numSamples) noexcept;

    /** How hard the stage is being driven, 0..1, for the DRIVE meter. */
    float getDrive() const noexcept { return uiDrive.load (std::memory_order_relaxed); }

private:
    struct OnePole
    {
        float z = 0.0f;

        inline float lowpass (float x, float coef) noexcept { z += coef * (x - z); return z; }
        void reset() noexcept { z = 0.0f; }
    };

    struct Smoothed
    {
        float value = 0.0f, coef = 0.2f;

        void snap (float v) noexcept { value = v; }
        inline float step (float t) noexcept { value += coef * (t - value); return value; }
    };

    static constexpr int controlInterval = 32;

    double sampleRate = 44100.0;
    int    activeChannels = 2;

    std::array<OnePole, maxChannels> tilt {};

    Settings target {};
    Smoothed smGain, smTone, smOutput, smEnable;

    float driveEnvelope = 0.0f;
    std::atomic<float> uiDrive { 0.0f };
};
