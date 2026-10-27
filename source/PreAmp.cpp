#include "PreAmp.h"

#include <algorithm>
#include <cmath>

namespace
{
constexpr float twoPi = 6.28318530717958647692f;

inline float onePoleCoef (float cutoffHz, double sampleRate) noexcept
{
    const auto c = 1.0f - std::exp (-twoPi * cutoffHz / static_cast<float> (sampleRate));
    return std::clamp (c, 0.0f, 1.0f);
}

inline float dbToGain (float db) noexcept { return std::pow (10.0f, db * 0.05f); }
} // namespace

void PreAmp::prepare (double newSampleRate, int numChannels)
{
    sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;
    activeChannels = std::clamp (numChannels, 1, maxChannels);

    const auto blocksPerSecond = static_cast<float> (sampleRate) / static_cast<float> (controlInterval);
    const auto coef = std::clamp (1.0f - std::exp (-1.0f / (0.02f * blocksPerSecond)), 0.0f, 1.0f);

    for (auto* s : { &smGain, &smTone, &smOutput, &smEnable })
        s->coef = coef;

    reset();

    smGain.snap (target.gainDb);
    smTone.snap (target.toneDb);
    smOutput.snap (target.outputDb);
    smEnable.snap (target.enabled ? 1.0f : 0.0f);
}

void PreAmp::reset()
{
    for (auto& t : tilt)
        t.reset();

    driveEnvelope = 0.0f;
    uiDrive.store (0.0f, std::memory_order_relaxed);
}

void PreAmp::process (float* const* data, int numChannels, int numSamples) noexcept
{
    const auto nch = std::clamp (numChannels, 0, maxChannels);

    if (nch == 0 || numSamples <= 0)
        return;

    const auto tiltCoef = onePoleCoef (700.0f, sampleRate);
    const auto releaseCoef = onePoleCoef (1.0f / (twoPi * 0.2f), sampleRate);

    for (int start = 0; start < numSamples; start += controlInterval)
    {
        const auto n = std::min (controlInterval, numSamples - start);

        // Blending the whole stage in and out rather than switching it keeps the
        // bypass silent, and lets gain, tilt and trim ride the same fade.
        const auto enable = smEnable.step (target.enabled ? 1.0f : 0.0f);
        const auto gain = dbToGain (smGain.step (target.gainDb) * enable);
        const auto toneDb = smTone.step (target.toneDb) * enable;
        const auto output = dbToGain (smOutput.step (target.outputDb) * enable);

        const auto lowGain = dbToGain (-toneDb);
        const auto highGain = dbToGain (toneDb);

        for (int k = 0; k < n; ++k)
        {
            const auto i = start + k;

            for (int c = 0; c < nch; ++c)
            {
                const auto driven = data[c][i] * gain;

                // tanh is the console curve: unity slope around zero, so clean
                // settings stay clean, and progressively rounder as it is pushed.
                const auto saturated = enable > 0.0f ? driven + enable * (std::tanh (driven) - driven) : driven;

                const auto low = tilt[static_cast<size_t> (c)].lowpass (saturated, tiltCoef);
                data[c][i] = (low * lowGain + (saturated - low) * highGain) * output;

                if (c == 0)
                {
                    // Peak of what the saturation sees, falling slowly: this is what
                    // the DRIVE meter reports, not the output level.
                    const auto rectified = std::fabs (driven);
                    driveEnvelope = rectified > driveEnvelope
                                        ? rectified
                                        : driveEnvelope + releaseCoef * (rectified - driveEnvelope);
                }
            }
        }
    }

    uiDrive.store (std::clamp (driveEnvelope, 0.0f, 4.0f), std::memory_order_relaxed);
}
