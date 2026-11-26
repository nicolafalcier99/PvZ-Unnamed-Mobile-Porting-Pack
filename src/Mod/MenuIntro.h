// PVZU: the main-menu Adventure scene (zombie hand, flashing button, evil laugh; GameSelector waits 450 ticks).
// 2026-11-23: play it the first time, or after a year without playing; otherwise start the level at once.
#pragma once

#include <cstdint>
#include <optional>

namespace pvzu {

inline constexpr int64_t kLongAbsenceSeconds = 365LL * 24 * 60 * 60;

// theIntroSeenAt: when the scene last played (unix s); thePreviousPlay: when the game was last played before this
// run started; theShownThisSession: the scene already played in this run.
bool ShouldPlayAdventureIntro(std::optional<int64_t> theIntroSeenAt, std::optional<int64_t> thePreviousPlay, int64_t theNow,
	bool theShownThisSession);

} // namespace pvzu
