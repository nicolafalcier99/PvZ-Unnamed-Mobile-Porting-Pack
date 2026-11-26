// PVZU: skip the "Click to start" title screen on phones (2026-11-22: "we can also make an automation to skip that").
#pragma once

namespace pvzu {

// Phones skip; desktop keeps the vanilla title unless -pvzu-skip-title; demo playback never skips.
bool ShouldSkipTitleScreen(bool mobile, bool demoPlayback, bool skipFlag);

} // namespace pvzu
