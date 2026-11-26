// PVZU: Unnamed 1.3beta2 Roof (Adventure 5-1..5-9) level rules. Evidence: the research notes (w5-roof) sections 0, 3 and
// 5 (and the asm reads in pvz-unnamed-pack/SOURCES-roof-levels.md). Pure functions; every constant is a rule (pack
// data/rules-roof-levels.json, plus the shared dayLevels.* / dayZombies.* keys); a missing key gives the vanilla
// behaviour. The level tables themselves (waves, flags, allowed zombies, start sun 50, pre-placed pots, award) are
// vanilla in the exe (0x6A34E8, 0x6A35B0, InitLevel 0xC05CE8): world 5 differs through global rules built elsewhere
// (x4 budgets in Levels.h, zombie values, elites, +133 px start) and the sky drop below. 5-10 is Zomboss.*.
#pragma once

#include <functional>

namespace pvzu::unnamed {

// Cells a sky drop picks from (Board::SetupBungeeDrop: columns 4..8 x rows 0..4).
inline constexpr int kSkyDropCells = 25;

struct SkyDrop {
	int count = 0;  // zombies bungees bring down
	int points = 0; // the engine's (unused) point budget, kept for the table
};

// Final-wave sky drop of an Adventure roof level (exe Board::SpawnZombiesFromSky 0x412C30, vanilla counts).
// false = no rule for this level: the engine keeps its own counts. count is capped at kSkyDropCells.
bool RoofFinalWaveSkyDrop(int level, SkyDrop& out);

// Type of one sky-dropped zombie (ZombieType value). The exe calls the vanilla picker first in every case
// (SpawnZombiesFromSky 0x412CA3, PickGraveRisingZombieType: weighted Rand), then hook 85 (0x795C27, one site for every
// sky drop, the roof's final wave and the Buckethead umbrella's) throws that result away and draws
// Rand(dayZombies.skyDrop.typeRollRange) again until the roll is one of 13 zombie types (0..7, 15, 21..24).
// No range rule (or a roll source that never gives an accepted type): the vanilla pick. rand(n) returns 0..n-1.
int SkyDropType(const std::function<int()>& vanillaPick, const std::function<int(int)>& rand);

} // namespace pvzu::unnamed
