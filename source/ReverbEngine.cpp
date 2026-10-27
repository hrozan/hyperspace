#include "ReverbEngine.h"

#include <algorithm>
#include <cmath>

namespace
{
constexpr float pi = 3.14159265358979323846f;
constexpr float twoPi = 2.0f * pi;

// Line lengths in milliseconds at size 1.0 in a hall; mutually prime enough that
// the network does not fall into a short common period.
constexpr std::array<float, 8> lineMs { 23.3f, 29.7f, 37.1f, 43.9f, 51.7f, 59.3f, 67.9f, 73.1f };

// Input diffusers, short and fixed: they smear transients before the tank sees them.
constexpr std::array<float, 4> diffuserMs { 4.77f, 6.91f, 8.86f, 11.35f };

// Early reflection pattern, scaled by size and type. Alternating signs widen it.
constexpr std::array<float, 8> tapMs { 7.0f, 13.3f, 21.7f, 29.1f, 38.3f, 47.9f, 58.7f, 71.3f };
constexpr std::array<float, 8> tapWeights { 0.84f, -0.72f, 0.66f, -0.58f, 0.5f, -0.42f, 0.36f, -0.3f };

constexpr float maxSizeScale = 1.5f;
constexpr float modHeadroomMs = 12.0f;

/** Each reverb type is a scale on the tank and a bias on diffusion and damping. */
struct TypeTraits
{
    float scale;
    float diffusionBias;
    float dampingBias;
};

TypeTraits traitsFor (ReverbEngine::Type type) noexcept
{
    switch (type)
    {
    case ReverbEngine::Type::room: return { 0.45f, 0.00f, 0.05f };
    case ReverbEngine::Type::plate: return { 0.62f, 0.22f, -0.10f };
    case ReverbEngine::Type::spring: return { 0.30f, 0.30f, 0.18f };
    case ReverbEngine::Type::ambient: return { 0.22f, -0.15f, 0.00f };
    case ReverbEngine::Type::hall:
    default: return { 1.00f, 0.08f, 0.00f };
    }
}

inline float wrap01 (float x) noexcept
{
    x -= std::floor (x);
    return x;
}

inline float onePoleCoef (float cutoffHz, double sampleRate) noexcept
{
    const auto c = 1.0f - std::exp (-twoPi * cutoffHz / static_cast<float> (sampleRate));
    return std::clamp (c, 0.0f, 1.0f);
}

inline float dbToGain (float db) noexcept { return std::pow (10.0f, db * 0.05f); }

/** Deterministic value noise, so the random shape is the same on every run. */
inline float hashToUnit (float n) noexcept
{
    auto h = static_cast<uint32_t> (static_cast<int32_t> (n) + 1013904223) * 2654435761u;
    h ^= h >> 15;
    h *= 2246822519u;
    h ^= h >> 13;
    return static_cast<float> (h & 0xffffffu) / static_cast<float> (0xffffffu);
}
} // namespace

//==============================================================================
void ReverbEngine::Line::prepare (int lengthSamples)
{
    buffer.assign (static_cast<size_t> (std::max (8, lengthSamples)), 0.0f);
    write = 0;
}

void ReverbEngine::Line::clear() noexcept
{
    std::fill (buffer.begin(), buffer.end(), 0.0f);
    write = 0;
}

//==============================================================================
float ReverbEngine::lfoValue (ModShape shape, float ph) noexcept
{
    const auto p = wrap01 (ph);

    switch (shape)
    {
    case ModShape::triangle: return 1.0f - std::fabs (2.0f * p - 1.0f);

    case ModShape::random:
    {
        // smoothed value noise: one new target per cycle, eased between
        const auto cycle = std::floor (ph);
        const auto a = hashToUnit (cycle);
        const auto b = hashToUnit (cycle + 1.0f);
        const auto s = p * p * (3.0f - 2.0f * p);
        return a + (b - a) * s;
    }

    case ModShape::sine:
    default: return 0.5f - 0.5f * std::cos (twoPi * p);
    }
}

void ReverbEngine::prepare (double newSampleRate, int numChannels)
{
    sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;
    activeChannels = std::clamp (numChannels, 1, maxChannels);

    const auto msToSamples = [this] (float ms) { return static_cast<float> (ms * 0.001 * sampleRate); };

    for (size_t i = 0; i < static_cast<size_t> (numLines); ++i)
    {
        baseSamples[i] = msToSamples (lineMs[i]);
        lines[i].prepare (static_cast<int> (baseSamples[i] * maxSizeScale + msToSamples (modHeadroomMs)) + 8);
    }

    for (int c = 0; c < maxChannels; ++c)
    {
        auto& ch = channels[static_cast<size_t> (c)];

        ch.preDelay.prepare (static_cast<int> (msToSamples (260.0f)) + 8);
        ch.early.prepare (static_cast<int> (msToSamples (tapMs.back() * maxSizeScale)) + 8);

        for (size_t d = 0; d < static_cast<size_t> (numDiffusers); ++d)
        {
            // Nudge the right channel's diffusers so the two sides decorrelate.
            ch.diffusers[d].delaySamples = msToSamples (diffuserMs[d] * (c == 1 ? 1.17f : 1.0f));
            ch.diffusers[d].line.prepare (static_cast<int> (ch.diffusers[d].delaySamples) + 8);
        }

        ch.modDelay.prepare (static_cast<int> (msToSamples (60.0f)) + 8);

        for (size_t t = 0; t < static_cast<size_t> (numTaps); ++t)
        {
            tapSamples[static_cast<size_t> (c)][t] = msToSamples (tapMs[t] * (c == 1 ? 1.13f : 1.0f));

            // The right channel reorders the pattern rather than inverting it, so a
            // mono sum of the two does not cancel the early reflections.
            tapGain[static_cast<size_t> (c)][t] = c == 1 ? tapWeights[(t + 3) % numTaps] : tapWeights[t];
        }
    }

    // One-pole smoothers running at control rate; ~25 ms to settle.
    const auto blocksPerSecond = static_cast<float> (sampleRate) / static_cast<float> (controlInterval);
    const auto coef = std::clamp (1.0f - std::exp (-1.0f / (0.025f * blocksPerSecond)), 0.0f, 1.0f);

    for (auto* s : { &smSize, &smDecay, &smPreDelay, &smDiffusion, &smEarlyLate, &smLowCut, &smHighCut, &smDamping,
                     &smTone, &smModDepth, &smModMix, &smDuck, &smMix, &smWidth, &smLevel, &smModEnable })
        s->coef = coef;

    // Enabling or bypassing a stage inside the tank has to crossfade, or it clicks.
    smModEnable.coef = std::clamp (1.0f - std::exp (-1.0f / (0.015f * blocksPerSecond)), 0.0f, 1.0f);

    reset();

    smSize.snap (target.size);
    smDecay.snap (target.decaySeconds);
    smPreDelay.snap (target.preDelayMs);
    smDiffusion.snap (target.diffusion);
    smEarlyLate.snap (target.earlyLate);
    smLowCut.snap (target.lowCutHz);
    smHighCut.snap (target.highCutHz);
    smDamping.snap (target.damping);
    smTone.snap (target.toneDb);
    smModDepth.snap (target.modDepth);
    smModMix.snap (target.modMix);
    smDuck.snap (target.duckDb);
    smMix.snap (target.mix);
    smWidth.snap (target.width);
    smLevel.snap (target.levelGain);
    smModEnable.snap (target.modOn ? 1.0f : 0.0f);

    const auto scale = traitsFor (target.type).scale * (0.5f + target.size);

    for (size_t i = 0; i < static_cast<size_t> (numLines); ++i)
        lineDelay[i].snap (baseSamples[i] * scale);
}

void ReverbEngine::reset()
{
    for (auto& line : lines)
        line.clear();

    for (auto& damp : lineDamp)
        damp.reset();

    for (auto& ch : channels)
    {
        ch.preDelay.clear();
        ch.early.clear();
        ch.modDelay.clear();

        for (auto& d : ch.diffusers)
            d.line.clear();

        for (auto& a : ch.phaser)
            a.reset();

        ch.lowCut.reset();
        ch.highCut.reset();
        ch.tone.reset();
    }

    phase = 0.0f;
    duckEnvelope = 0.0f;
}

//==============================================================================
namespace
{
/** In-place 8 point Hadamard, normalised: the tank mixes without gaining energy. */
inline void hadamard8 (std::array<float, 8>& v) noexcept
{
    for (int span = 1; span < 8; span <<= 1)
    {
        for (int i = 0; i < 8; i += span << 1)
        {
            for (int j = 0; j < span; ++j)
            {
                const auto a = v[static_cast<size_t> (i + j)];
                const auto b = v[static_cast<size_t> (i + j + span)];
                v[static_cast<size_t> (i + j)] = a + b;
                v[static_cast<size_t> (i + j + span)] = a - b;
            }
        }
    }

    constexpr auto norm = 0.35355339f; // 1 / sqrt(8)

    for (auto& x : v)
        x *= norm;
}

inline float allpassCoef (float frequencyHz, double sampleRate) noexcept
{
    const auto nyquist = static_cast<float> (sampleRate) * 0.5f;
    const auto fc = std::clamp (frequencyHz, 20.0f, nyquist * 0.9f);
    const auto t = std::tan (pi * fc / static_cast<float> (sampleRate));

    return (t - 1.0f) / (t + 1.0f);
}
} // namespace

void ReverbEngine::process (float* const* data, int numChannels, int numSamples) noexcept
{
    const auto nch = std::clamp (numChannels, 0, maxChannels);

    if (nch == 0 || numSamples <= 0)
        return;

    const auto fs = static_cast<float> (sampleRate);
    const auto traits = traitsFor (target.type);
    const auto phaseInc = std::max (0.0f, target.modRateHz) / fs;
    const auto frozen = target.freeze;

    const auto attackCoef = onePoleCoef (1.0f / (twoPi * 0.005f), sampleRate);
    const auto releaseCoef = onePoleCoef (1.0f / (twoPi * 0.250f), sampleRate);

    float tailPeak = 0.0f;

    for (int start = 0; start < numSamples; start += controlInterval)
    {
        const auto n = std::min (controlInterval, numSamples - start);

        // ---- control rate ----
        const auto size = smSize.step (target.size);
        const auto decay = std::max (0.05f, smDecay.step (target.decaySeconds));
        const auto preMs = smPreDelay.step (target.preDelayMs);
        const auto diffusion = std::clamp (smDiffusion.step (target.diffusion) + traits.diffusionBias, 0.0f, 1.0f);
        const auto earlyLate = smEarlyLate.step (target.earlyLate);
        const auto lowCutHz = smLowCut.step (target.lowCutHz);
        const auto highCutHz = smHighCut.step (target.highCutHz);
        const auto damping = std::clamp (smDamping.step (target.damping) + traits.dampingBias, 0.0f, 1.0f);
        const auto toneDb = smTone.step (target.toneDb);
        const auto depth = smModDepth.step (target.modDepth);
        const auto modMix = smModMix.step (target.modMix);
        const auto duckDb = smDuck.step (target.duckDb);
        const auto mix = smMix.step (target.mix);
        const auto width = smWidth.step (target.width);
        const auto level = smLevel.step (target.levelGain);
        const auto modOn = smModEnable.step (target.modOn ? 1.0f : 0.0f);

        // The LFO at the end of this block; everything it drives ramps towards it.
        const auto endPhase = phase + phaseInc * static_cast<float> (n);

        const auto modsDelay = target.modTarget != ModTarget::filter;
        const auto modsFilter = target.modTarget != ModTarget::delayLines;

        // ---- tank geometry ----
        const auto scale = traits.scale * (0.5f + size);
        const auto lineSwing = modsDelay ? depth * modOn * 0.0022f * fs : 0.0f;

        std::array<float, numLines> lineLength {};

        for (size_t i = 0; i < static_cast<size_t> (numLines); ++i)
        {
            lineLength[i] = baseSamples[i] * scale;

            const auto lfo = lfoValue (target.modShape, endPhase + static_cast<float> (i) * 0.113f) * 2.0f - 1.0f;
            lineDelay[i].setTarget (lineLength[i] + lfo * lineSwing, n);

            // Decay gain per pass: -60 dB over the decay time for this line's length.
            const auto seconds = lineLength[i] / fs;
            lineGain[i] = frozen ? 0.9999f : std::clamp (std::pow (10.0f, -3.0f * seconds / decay), 0.0f, 0.9995f);
        }

        auto dampCutoff = dampingCutoffHz (damping);

        if (modsFilter)
            dampCutoff *= std::exp2 ((lfoValue (target.modShape, endPhase) * 2.0f - 1.0f) * depth * modOn * 1.2f);

        const auto dampCoef = frozen ? 1.0f : onePoleCoef (std::clamp (dampCutoff, 200.0f, 20000.0f), sampleRate);

        const auto hpCoef = onePoleCoef (lowCutHz, sampleRate);
        const auto lpCoef = onePoleCoef (highCutHz, sampleRate);
        const auto tiltCoef = onePoleCoef (700.0f, sampleRate);
        const auto lowGain = dbToGain (-toneDb);
        const auto highGain = dbToGain (toneDb);

        const auto preDelaySamples = std::max (1.0f, preMs * 0.001f * fs);
        const auto diffuserGain = 0.35f + diffusion * 0.3f;
        const auto inputGate = frozen ? 0.0f : 1.0f;

        const auto duckGain = target.duckOn ? dbToGain (duckDb * std::min (1.0f, duckEnvelope * 5.0f)) : 1.0f;

        // ---- the modulation stage, per channel ----
        const auto vibrato = target.modMode == ModMode::vibrato;
        const auto phasing = target.modMode == ModMode::phaser;

        for (int c = 0; c < nch; ++c)
        {
            auto& ch = channels[static_cast<size_t> (c)];
            const auto chPhase = endPhase + (c == 1 ? target.modSpread * 0.5f : 0.0f);
            const auto lfo = lfoValue (target.modShape, chPhase) * 2.0f - 1.0f;

            if (phasing)
            {
                const auto centreHz = 620.0f * std::exp2 (lfo * depth * 2.0f);

                for (size_t s = 0; s < static_cast<size_t> (numAllpass); ++s)
                {
                    const auto spread = std::exp2 (0.28f * (static_cast<float> (s) - 2.5f));
                    ch.phaserCoef[s].setTarget (allpassCoef (centreHz * spread, sampleRate), n);
                }
            }
            else
            {
                const auto base = 0.012f * fs;
                const auto swing = (vibrato ? 0.004f : 0.006f) * fs * depth;
                ch.modDelaySamples.setTarget (base + swing * lfo, n);
            }
        }

        // ---- audio rate ----
        for (int k = 0; k < n; ++k)
        {
            const auto i = start + k;

            const auto dryL = data[0][i];
            const auto dryR = nch > 1 ? data[1][i] : dryL;

            const auto rectified = std::max (std::fabs (dryL), std::fabs (dryR));
            duckEnvelope += (rectified > duckEnvelope ? attackCoef : releaseCoef) * (rectified - duckEnvelope);

            // ---- input: pre-delay, cuts, early reflections, diffusion ----
            std::array<float, maxChannels> tankIn { 0.0f, 0.0f };
            std::array<float, maxChannels> early { 0.0f, 0.0f };

            for (int c = 0; c < nch; ++c)
            {
                auto& ch = channels[static_cast<size_t> (c)];
                const auto& taps = tapSamples[static_cast<size_t> (c)];
                const auto& gains = tapGain[static_cast<size_t> (c)];

                ch.preDelay.push (c == 0 ? dryL : dryR);
                auto v = ch.preDelay.read (preDelaySamples);

                v = ch.lowCut.highpass (v, hpCoef);
                v = ch.highCut.lowpass (v, lpCoef);

                ch.early.push (v);

                float er = 0.0f;

                for (size_t t = 0; t < static_cast<size_t> (numTaps); ++t)
                    er += ch.early.read (taps[t] * scale) * gains[t];

                early[static_cast<size_t> (c)] = er * 0.5f * inputGate;

                for (auto& d : ch.diffusers)
                    v = d.process (v, diffuserGain);

                tankIn[static_cast<size_t> (c)] = v * 0.5f;
            }

            if (nch == 1)
            {
                tankIn[1] = tankIn[0];
                early[1] = early[0];
            }

            // ---- feedback delay network ----
            std::array<float, numLines> v {};

            for (size_t l = 0; l < static_cast<size_t> (numLines); ++l)
            {
                auto s = lines[l].read (lineDelay[l].next());
                s = lineDamp[l].lowpass (s, dampCoef);
                v[l] = s * lineGain[l];
            }

            const auto tailL = (v[0] + v[2] - v[4] + v[6]) * 0.5f;
            const auto tailR = (v[1] - v[3] + v[5] + v[7]) * 0.5f;

            hadamard8 (v);

            for (size_t l = 0; l < static_cast<size_t> (numLines); ++l)
            {
                const auto src = static_cast<size_t> ((l & 1) != 0 ? 1 : 0);
                lines[l].push (v[l] + tankIn[src] * inputGate);
            }

            // ---- wet: early/late balance, modulation stage, tone ----
            std::array<float, maxChannels> wet { 0.0f, 0.0f };

            for (int c = 0; c < nch; ++c)
            {
                auto& ch = channels[static_cast<size_t> (c)];
                const auto tail = c == 0 ? tailL : tailR;

                auto w = early[static_cast<size_t> (c)] * (1.0f - earlyLate) + tail * earlyLate;

                // Always run the stage, blend by the enable gain: switching it in or
                // out then crossfades instead of clicking.
                ch.modDelay.push (w);

                float modulated = w;

                if (phasing)
                {
                    for (size_t s = 0; s < static_cast<size_t> (numAllpass); ++s)
                    {
                        ch.phaser[s].a1 = ch.phaserCoef[s].next();
                        modulated = ch.phaser[s].process (modulated);
                    }
                }
                else
                {
                    modulated = ch.modDelay.read (ch.modDelaySamples.next());
                }

                // Vibrato is the modulated path alone; chorus and phaser need the
                // unmodulated path alongside it for the comb to appear.
                const auto blend = (vibrato ? 1.0f : modMix) * modOn;
                w += blend * (modulated - w);

                const auto low = ch.tone.lowpass (w, tiltCoef);
                wet[static_cast<size_t> (c)] = low * lowGain + (w - low) * highGain;
            }

            if (nch > 1)
            {
                const auto mid = (wet[0] + wet[1]) * 0.5f;
                const auto side = (wet[0] - wet[1]) * 0.5f * width;
                wet[0] = mid + side;
                wet[1] = mid - side;
            }

            for (int c = 0; c < nch; ++c)
            {
                const auto dry = c == 0 ? dryL : dryR;
                const auto w = wet[static_cast<size_t> (c)] * duckGain;

                tailPeak = std::max (tailPeak, std::fabs (w));
                data[c][i] = (dry * (1.0f - mix) + w * mix) * level;
            }

            phase = wrap01 (phase + phaseInc);
        }
    }

    uiPhase.store (phase, std::memory_order_relaxed);
    uiTail.store (tailPeak, std::memory_order_relaxed);
}
