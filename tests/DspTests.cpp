// Checks the DSP on its own: no JUCE, no host, no audio device. Most cases feed an
// impulse (or a sine) and then silence, and assert that what comes back is finite,
// bounded, and has actually decayed by the end.

#include "../source/PreAmp.h"
#include "../source/ReverbEngine.h"
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace
{
int failures = 0;

struct Trace { float peak = 0.0f; float endRms = 0.0f; bool finite = true; };

/** Impulse (or sine) in, then silence; reports the peak and how much is left at the end. */
Trace run (ReverbEngine::Settings s, double fs, float seconds, bool sine = false)
{
    ReverbEngine engine;
    engine.setSettings (s);
    engine.prepare (fs, 2);

    const int total = static_cast<int> (fs * seconds);
    const int block = 128;
    std::vector<float> l (block), r (block);
    float* ptrs[2] { l.data(), r.data() };

    Trace out;
    double energy = 0.0; long count = 0;

    for (int i = 0; i < total; i += block)
    {
        for (int k = 0; k < block; ++k)
        {
            const int n = i + k;
            const auto x = sine ? 0.4f * std::sin (2.0f * 3.14159265f * 220.0f * static_cast<float> (n) / static_cast<float> (fs))
                                : (n == 0 ? 1.0f : 0.0f);
            l[k] = x; r[k] = x;
        }

        engine.process (ptrs, 2, block);

        for (int k = 0; k < block; ++k)
            for (float v : { l[k], r[k] })
            {
                if (! std::isfinite (v)) out.finite = false;
                out.peak = std::max (out.peak, std::fabs (v));

                if (i + k > total - static_cast<int> (fs * 0.2)) { energy += v * v; ++count; }
            }
    }

    out.endRms = count > 0 ? static_cast<float> (std::sqrt (energy / static_cast<double> (count))) : 0.0f;
    return out;
}

void check (const char* name, Trace t, float maxPeak, float maxEndRms)
{
    const bool ok = t.finite && t.peak < maxPeak && t.endRms < maxEndRms;
    if (! ok) ++failures;
    std::printf ("%-28s peak %7.4f  residue %11.8f  %s\n", name, t.peak, t.endRms, ok ? "ok" : "FAIL");
}
}

int main()
{
    using S = ReverbEngine::Settings;
    using Type = ReverbEngine::Type;
    using Mode = ReverbEngine::ModMode;
    using Shape = ReverbEngine::ModShape;
    using Target = ReverbEngine::ModTarget;

    for (double fs : { 44100.0, 48000.0, 88200.0, 96000.0 })
    {
        char label[64];
        std::snprintf (label, sizeof (label), "default @ %.0f Hz", fs);
        check (label, run (S {}, fs, 10.0f), 1.5f, 1.0e-5f);
    }

    for (auto type : { Type::room, Type::hall, Type::plate, Type::spring, Type::ambient })
    {
        S s; s.type = type;
        static const char* names[] { "room", "hall", "plate", "spring", "ambient" };
        check (names[static_cast<int> (type)], run (s, 48000.0, 10.0f), 1.5f, 1.0e-5f);
    }

    for (auto mode : { Mode::chorus, Mode::vibrato, Mode::phaser })
    {
        S s; s.modMode = mode; s.modDepth = 1.0f; s.modRateHz = 6.0f;
        static const char* names[] { "chorus full depth", "vibrato full depth", "phaser full depth" };
        check (names[static_cast<int> (mode)], run (s, 48000.0, 10.0f), 1.5f, 1.0e-5f);
    }

    for (auto shape : { Shape::sine, Shape::triangle, Shape::random })
    {
        S s; s.modShape = shape; s.modDepth = 1.0f;
        static const char* names[] { "shape sine", "shape triangle", "shape random" };
        check (names[static_cast<int> (shape)], run (s, 48000.0, 10.0f), 1.5f, 1.0e-5f);
    }

    for (auto target : { Target::delayLines, Target::filter, Target::both })
    {
        S s; s.modTarget = target; s.modDepth = 1.0f;
        static const char* names[] { "target delay", "target filter", "target both" };
        check (names[static_cast<int> (target)], run (s, 48000.0, 10.0f), 1.5f, 1.0e-5f);
    }

    { S s; s.decaySeconds = 0.1f; s.size = 0.0f; check ("min decay, min size", run (s, 48000.0, 5.0f), 1.5f, 1.0e-6f); }
    { S s; s.decaySeconds = 20.0f; s.size = 1.0f; check ("20 s, max size", run (s, 48000.0, 30.0f), 1.5f, 1.0e-4f); }
    { S s; s.modOn = false; check ("modulation off", run (s, 48000.0, 10.0f), 1.5f, 1.0e-5f); }
    { S s; s.diffusion = 1.0f; s.preDelayMs = 250.0f; check ("max diffusion, max pre", run (s, 48000.0, 10.0f), 1.5f, 1.0e-5f); }
    { S s; s.mix = 1.0f; s.width = 2.0f; s.levelGain = 4.0f; check ("wet, wide, +12 dB", run (s, 48000.0, 10.0f), 4.0f, 1.0e-4f); }
    { S s; s.lowCutHz = 1000.0f; s.highCutHz = 1000.0f; s.toneDb = 12.0f; check ("cuts closed, tone max", run (s, 48000.0, 10.0f), 1.5f, 1.0e-5f); }
    { S s; check ("sustained sine", run (s, 48000.0, 6.0f, true), 2.0f, 1.0f); }

    // Freeze has to hold what is already in the tank rather than fade.
    {
        S s;
        ReverbEngine engine;
        engine.setSettings (s);
        engine.prepare (48000.0, 2);

        const int block = 128;
        std::vector<float> l (block), r (block);
        float* p[2] { l.data(), r.data() };
        float before = 0.0f, after = 0.0f;

        for (int i = 0; i < 48000 * 14; i += block)
        {
            for (int k = 0; k < block; ++k)
            {
                const auto n = i + k;
                const auto x = n < 48000 ? 0.4f * std::sin (2.0f * 3.14159265f * 220.0f * static_cast<float> (n) / 48000.0f)
                                         : 0.0f;
                l[k] = x; r[k] = x;
            }

            if (i == 48000) { s.freeze = true; engine.setSettings (s); }

            engine.process (p, 2, block);

            for (int k = 0; k < block; ++k)
            {
                if (i + k > 48000 * 2 && i + k < 48000 * 3) before = std::max (before, std::fabs (l[k]));
                if (i + k > 48000 * 12) after = std::max (after, std::fabs (l[k]));
            }
        }

        const bool ok = after > before * 0.25f && after < before * 4.0f && std::isfinite (after);
        if (! ok) ++failures;
        std::printf ("%-28s held %7.4f -> %7.4f after 10 s  %s\n", "freeze", before, after, ok ? "ok" : "FAIL");
    }

    // ---- pre amp ----
    {
        const int block = 128;
        std::vector<float> l (block), r (block);
        float* p[2] { l.data(), r.data() };

        const auto runPreAmp = [&] (PreAmp::Settings ps, float amplitude)
        {
            PreAmp amp;
            amp.setSettings (ps);
            amp.prepare (48000.0, 2);

            float peak = 0.0f;
            bool finite = true;

            for (int i = 0; i < 48000; i += block)
            {
                for (int k = 0; k < block; ++k)
                {
                    const auto n = i + k;
                    l[k] = r[k] = amplitude * std::sin (2.0f * 3.14159265f * 220.0f
                                                            * static_cast<float> (n) / 48000.0f);
                }

                amp.process (p, 2, block);

                for (int k = 0; k < block; ++k)
                    for (float v : { l[k], r[k] })
                    {
                        if (! std::isfinite (v)) finite = false;
                        peak = std::max (peak, std::fabs (v));
                    }
            }

            return std::make_pair (peak, finite);
        };

        const auto report = [] (const char* name, std::pair<float, bool> result, float lo, float hi)
        {
            const auto ok = result.second && result.first >= lo && result.first <= hi;
            if (! ok) ++failures;
            std::printf ("%-28s peak %7.4f  %s\n", name, result.first, ok ? "ok" : "FAIL");
        };

        // Switched off it has to be a straight wire, whatever the knobs say.
        PreAmp::Settings off; off.enabled = false; off.gainDb = 24.0f; off.outputDb = 12.0f;
        report ("pre amp bypassed", runPreAmp (off, 0.5f), 0.4999f, 0.5001f);

        PreAmp::Settings unity; unity.gainDb = 0.0f; unity.toneDb = 0.0f; unity.outputDb = 0.0f;
        report ("pre amp unity", runPreAmp (unity, 0.5f), 0.4f, 0.5f);

        // Saturation has to hold the level down however hard it is driven.
        PreAmp::Settings hot; hot.gainDb = 24.0f; hot.toneDb = 0.0f; hot.outputDb = 0.0f;
        report ("pre amp driven hard", runPreAmp (hot, 1.0f), 0.9f, 1.05f);

        PreAmp::Settings trimmed; trimmed.gainDb = 24.0f; trimmed.outputDb = -24.0f;
        report ("pre amp trimmed back", runPreAmp (trimmed, 1.0f), 0.0f, 0.12f);

        PreAmp::Settings bright; bright.gainDb = 12.0f; bright.toneDb = 12.0f;
        report ("pre amp tone up", runPreAmp (bright, 0.5f), 0.0f, 4.0f);

        PreAmp::Settings dark; dark.gainDb = 12.0f; dark.toneDb = -12.0f;
        report ("pre amp tone down", runPreAmp (dark, 0.5f), 0.0f, 4.0f);

        // The drive reading has to rise with signal and fall back to nothing.
        {
            PreAmp amp;
            PreAmp::Settings ps; ps.gainDb = 12.0f;
            amp.setSettings (ps);
            amp.prepare (48000.0, 2);

            for (int i = 0; i < 48000; i += block)
            {
                for (int k = 0; k < block; ++k)
                    l[k] = r[k] = 0.5f * std::sin (2.0f * 3.14159265f * 220.0f
                                                       * static_cast<float> (i + k) / 48000.0f);
                amp.process (p, 2, block);
            }

            const auto loud = amp.getDrive();

            for (int i = 0; i < 48000 * 2; i += block)
            {
                std::fill (l.begin(), l.end(), 0.0f);
                std::fill (r.begin(), r.end(), 0.0f);
                amp.process (p, 2, block);
            }

            const auto quiet = amp.getDrive();
            const auto ok = loud > 1.0f && quiet < 0.01f;
            if (! ok) ++failures;
            std::printf ("%-28s driven %7.4f -> silent %7.4f  %s\n", "pre amp drive meter",
                         loud, quiet, ok ? "ok" : "FAIL");
        }
    }

    std::printf ("\n%s\n", failures == 0 ? "all checks passed" : "SOME CHECKS FAILED");
    return failures;
}
