#pragma once

#include <algorithm>
#include <array>
#include <cstddef>

/** Tempo-sync arithmetic, kept free of JUCE so it can be tested on its own. */
namespace SyncMath
{
/** Cycle length in beats for each entry of the modulation sync selector. */
inline constexpr std::array<double, 3> rateDivisionBeats {
    2.0, // 1/2
    1.0, // 1/4
    0.5  // 1/8
};

/** Cycle length in beats for each entry of the pre-delay sync selector. */
inline constexpr std::array<double, 3> preDelayDivisionBeats {
    0.25, // 1/16
    0.5,  // 1/8
    1.0   // 1/4
};

inline constexpr int numRateDivisions = static_cast<int> (rateDivisionBeats.size());
inline constexpr int numPreDelayDivisions = static_cast<int> (preDelayDivisionBeats.size());

/** Index 0 of either selector is the free-running entry (FREE / MS). */
inline constexpr bool isSynced (int syncMode) noexcept { return syncMode > 0; }

inline double safeBpm (double hostBpm) noexcept { return hostBpm > 0.0 ? hostBpm : 120.0; }

/**
    The rate the LFO should actually run at.

    One index covers the whole control, so a click is a single parameter change:
    hosts never see a half-applied state where sync is on but the division has
    not landed yet.
*/
inline float lfoRateHz (int syncMode, float freeRateHz, double hostBpm) noexcept
{
    if (!isSynced (syncMode))
        return freeRateHz;

    const auto index = std::clamp (syncMode - 1, 0, numRateDivisions - 1);
    const auto beats = rateDivisionBeats[static_cast<std::size_t> (index)];

    return static_cast<float> (safeBpm (hostBpm) / 60.0 / beats);
}

/** Pre-delay in milliseconds, either the knob value or the synced division. */
inline float preDelayMs (int syncMode, float freeMs, double hostBpm) noexcept
{
    if (!isSynced (syncMode))
        return freeMs;

    const auto index = std::clamp (syncMode - 1, 0, numPreDelayDivisions - 1);
    const auto beats = preDelayDivisionBeats[static_cast<std::size_t> (index)];

    return static_cast<float> (beats * 60000.0 / safeBpm (hostBpm));
}
} // namespace SyncMath
