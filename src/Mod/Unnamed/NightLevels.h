// PVZU: Unnamed 1.3beta2 Night (Adventure 2-1..2-10) level rules: start graves, allow-list extras and the two extra
// grave rises (gold Football death, Dancer "Death Carnival"). Evidence: the research notes (w2-night) sections 2.3, 2.4,
// 3 and 4. Pure functions: engine enums are passed as ints. Every constant is a "nightLevels.*" rule (pack
// data/rules-night-levels.json); a missing key gives the vanilla behaviour. The final-wave grave type re-roll is
// Plan 2's hook 88 code (Mod/Unnamed/Levels.h GraveRiseType): the exe has one site for every level.
#pragma once

#include <array>

namespace pvzu::unnamed {

// Gravestone count per grid column 0..8 (8 = rightmost).
using GraveLayout = std::array<int, 9>;

// Start graves of an Adventure night level (exe Board::InitLevel 0x40A945.., AddGraveStones 0x408FC0).
// false (out zeroed) = no rule for this level: the engine keeps its own layout.
bool NightStartGraves(int level, GraveLayout& out);
int GraveTotal(const GraveLayout& layout);

// gZombieAllowedLevels (exe 0x6A35B0) rows that differ from PvZ-Portable's table: true = also allowed.
bool NightAllowExtra(int zombieType, int level);

// Gold Football (variant 0x102) DieNoLoot (0xC03520): every gravestone raises a Football (0xC0358D).
// fromWave < 0 = cut-scene / UI zombie, never a game death.
bool GoldFootballRaisesGraves(int zombieType, int variant, int fromWave);
int GoldFootballGraveType();

// Dancer "Death Carnival" (0x7966BA/0x7966C0): each backup a Dancer summons adds 1 to its counter (exe +0x12C);
// at everyBackups it resets to 0 and every gravestone raises a zombie (SpawnZombiesFromGraves with the dancer's
// hypnosis flag).
struct CarnivalStep {
	int counter = 0;
	bool raise = false;
};
CarnivalStep CarnivalOnBackupSummoned(int summonerType, int summonerVariant, int counter);
bool CarnivalPassesHypnosis();

} // namespace pvzu::unnamed
