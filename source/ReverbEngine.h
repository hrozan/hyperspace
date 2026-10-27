#pragma once

#include <array>
#include <cmath>
#include <atomic>
#include <vector>

/**
    Modulated reverb: pre-delay, early reflections and an eight-line feedback delay
    network, with a selectable modulation stage.

    Deliberately free of JUCE types so it can be compiled and tested on its own.
    process() works in place on raw channel pointers.
*/
class ReverbEngine
{
public:
    static constexpr int maxChannels = 2;
    static constexpr int numLines = 8;     // feedback delay network size
    static constexpr int numDiffusers = 4; // input diffusers per channel
    static constexpr int numTaps = 8;      // early reflection taps per channel
    static constexpr int numAllpass = 6;   // phaser stages per channel

    enum class Type { room = 0, hall, plate, spring, ambient };
    enum class ModMode { chorus = 0, vibrato, phaser };
    enum class ModShape { sine = 0, triangle, random };
    enum class ModTarget { delayLines = 0, filter, both };

    struct Settings
    {
        Type type = Type::hall;
        float decaySeconds = 4.8f;
        float size = 0.66f; // 0..1
        float preDelayMs = 75.0f;
        float diffusion = 0.74f; // 0..1
        float earlyLate = 0.62f; // 0 = early only, 1 = tail only

        float lowCutHz = 90.0f;
        float highCutHz = 7400.0f;
        float damping = 0.6f; // 0..1, high-frequency loss per pass
        float toneDb = -1.2f; // tilt, negative is darker

        bool modOn = true;
        ModMode modMode = ModMode::chorus;
        ModShape modShape = ModShape::sine;
        ModTarget modTarget = ModTarget::delayLines;
        float modRateHz = 0.62f;
        float modDepth = 0.58f; // 0..1
        float modSpread = 0.8f; // 0..1, L/R LFO offset in half cycles
        float modMix = 0.5f;    // 0..1, modulated against unmodulated

        bool freeze = false;
        bool duckOn = true;
        float duckDb = -4.5f; // full-scale ducking depth

        float mix = 0.38f;      // 0..1 dry/wet
        float width = 1.2f;     // 0..2
        float levelGain = 1.0f; // linear output gain
    };

    /** Damping as the tank's loop cutoff: wide open at 0, down to 1 kHz at 1. */
    static float dampingCutoffHz (float damping) noexcept
    { return 20000.0f * std::pow (0.05f, damping < 0.0f ? 0.0f : (damping > 1.0f ? 1.0f : damping)); }

    void prepare (double sampleRate, int numChannels);
    void reset();

    void setSettings (const Settings& s) noexcept { target = s; }

    /** In-place. `channels` must hold at least `numChannels` valid pointers. */
    void process (float* const* channels, int numChannels, int numSamples) noexcept;

    /** Display taps, safe to read from the message thread. */
    float getModPhase() const noexcept { return uiPhase.load (std::memory_order_relaxed); }
    float getTailLevel() const noexcept { return uiTail.load (std::memory_order_relaxed); }

private:
    // Coefficients are refreshed every `controlInterval` samples; values that would
    // zipper at that rate are ramped across the block instead.
    static constexpr int controlInterval = 32;

    /** Fractional delay line with linear interpolation. */
    struct Line
    {
        std::vector<float> buffer;
        int write = 0;

        void prepare (int lengthSamples);
        void clear() noexcept;

        inline void push (float x) noexcept
        {
            buffer[static_cast<size_t> (write)] = x;

            if (++write >= static_cast<int> (buffer.size()))
                write = 0;
        }

        inline float read (float delaySamples) const noexcept
        {
            const auto n = static_cast<int> (buffer.size());
            auto d = delaySamples < 1.0f ? 1.0f : delaySamples;
            d = d > static_cast<float> (n - 2) ? static_cast<float> (n - 2) : d;

            // Split the delay before wrapping: a float big enough to hold the buffer
            // length cannot also hold the fraction, and rounding there used to walk
            // the index off the end of the buffer.
            const auto whole = static_cast<int> (d);
            const auto frac = d - static_cast<float> (whole);

            auto i0 = write - whole;
            if (i0 < 0)
                i0 += n;

            auto i1 = i0 - 1;
            if (i1 < 0)
                i1 += n;

            const auto a = buffer[static_cast<size_t> (i0)];
            return a + frac * (buffer[static_cast<size_t> (i1)] - a);
        }
    };

    /** Schroeder all-pass, used for input diffusion and spring dispersion. */
    struct Diffuser
    {
        Line line;
        float delaySamples = 64.0f;

        inline float process (float x, float g) noexcept
        {
            const auto delayed = line.read (delaySamples);
            const auto v = x + g * delayed;
            line.push (v);
            return delayed - g * v;
        }
    };

    /** One-pole, used for damping, cuts and the tone tilt. */
    struct OnePole
    {
        float z = 0.0f;

        inline float lowpass (float x, float coef) noexcept
        {
            z += coef * (x - z);
            return z;
        }
        inline float highpass (float x, float coef) noexcept
        {
            z += coef * (x - z);
            return x - z;
        }
        void reset() noexcept { z = 0.0f; }
    };

    /** First-order all-pass, the phaser mode's sweep stage. */
    struct Allpass1
    {
        float a1 = 0.0f, z = 0.0f;

        inline float process (float x) noexcept
        {
            const auto y = a1 * x + z;
            z = x - a1 * y;
            return y;
        }

        void reset() noexcept { z = 0.0f; }
    };

    /** Linear ramp across one control block, so modulated delays do not step. */
    struct Ramp
    {
        float value = 0.0f, increment = 0.0f;

        void setTarget (float t, int steps) noexcept
        { increment = steps > 0 ? (t - value) / static_cast<float> (steps) : 0.0f; }

        inline float next() noexcept
        {
            value += increment;
            return value;
        }
        void snap (float v) noexcept
        {
            value = v;
            increment = 0.0f;
        }
    };

    struct Smoothed
    {
        float value = 0.0f, coef = 0.2f;

        void snap (float v) noexcept { value = v; }
        inline float step (float t) noexcept
        {
            value += coef * (t - value);
            return value;
        }
    };

    struct ChannelState
    {
        Line preDelay;
        Line early;
        std::array<Diffuser, numDiffusers> diffusers {};
        OnePole lowCut, highCut, tone;
        Line modDelay;
        Ramp modDelaySamples;
        std::array<Allpass1, numAllpass> phaser {};
        std::array<Ramp, numAllpass> phaserCoef {};
    };

    static float lfoValue (ModShape, float phase) noexcept;

    double sampleRate = 44100.0;
    int activeChannels = 2;

    std::array<ChannelState, maxChannels> channels {};

    // ---- feedback delay network ----
    std::array<Line, numLines> lines {};
    std::array<float, numLines> baseSamples {}; // unscaled line lengths
    std::array<float, numLines> lineGain {};
    std::array<OnePole, numLines> lineDamp {};
    std::array<Ramp, numLines> lineDelay {}; // scaled length plus modulation

    std::array<std::array<float, numTaps>, maxChannels> tapSamples {};
    std::array<std::array<float, numTaps>, maxChannels> tapGain {};

    Settings target {};

    Smoothed smSize, smDecay, smPreDelay, smDiffusion, smEarlyLate, smLowCut, smHighCut, smDamping, smTone, smModDepth,
        smModMix, smDuck, smMix, smWidth, smLevel, smModEnable;

    float phase = 0.0f; // shared LFO phase, 0..1
    float duckEnvelope = 0.0f;

    std::atomic<float> uiPhase { 0.0f };
    std::atomic<float> uiTail { 0.0f };
};
