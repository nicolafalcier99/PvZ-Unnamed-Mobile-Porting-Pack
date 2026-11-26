// PVZU: Unnamed 1.3beta2 Fog (Adventure 4-1..4-10) level rules. Evidence: the research notes (w4-fog) sections 4 and 5
// (U9, U12 resolved in pvz-unnamed-pack/SOURCES-fog-levels.md). Pure functions; every constant is a "fogLevels.*" rule
// (pack data/rules-fog-levels.json); a missing key gives the vanilla behaviour.
// The level tables themselves (waves, flags, allowed zombies, start sun, award) are vanilla in the exe (0x6A34E8,
// 0x6A35B0, InitLevel start sun 0xC05CE8): world 4 differs only through global rules built elsewhere (x4 budgets,
// zombie values, elites, +133 px start, final-wave pool spawn table in poolLevels.*).
#pragma once

namespace pvzu::unnamed {

// FogStartColumn: no rule, the engine's own LeftFogColumn stands.
inline constexpr int kNoFogRule = -1;

// Hook 145 (0x7A8310): the option byte 0x800298 puts fog over the whole lawn (only the dead 1.2 dialog copy 0xb7393b
// writes it, so 0 in 1.3beta2; the research notes (r8-options) 2).
// theOption: the panel's full-screen fog row (R8, the research notes (r8-options) 2), OR-ed with the rule.
bool FullFogOn(bool theOption = false);

// Board::LeftFogColumn (exe 0x41C1C0): first fogged column, 0..8. Full fog -> 0 for every stage (the hook is at the
// function's entry); else the pack's per-level row for an Adventure level; else kNoFogRule.
int FogStartColumn(int level, bool adventure, bool theFullFogOption = false);

} // namespace pvzu::unnamed
