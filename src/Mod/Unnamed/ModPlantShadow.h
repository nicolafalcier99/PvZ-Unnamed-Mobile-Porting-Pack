// PVZU: Plan 8 part 3c pure helpers for Unnamed's shadow state, Moonflower sun, the zombie-side statuses (Toxic and
// Repeater poison, "weakened") and the mod plants' placement rules. The single-plant rules (thresholds, points per
// neighbour, poison steps, Charmed Tall-nut numbers) are in ModPlantRules.h; these add what the engine wiring
// (Mod/Engine/ModPlantsShadow.*, Mod/Engine/ModZombieStatus.*) needs on top. Evidence:
// the research notes (mod-plants-behaviour) 1.2, 1.3, 0x67, 0x69, 0x7f; pack keys in data/rules-mod-plants-shadow.json
// (pvz-unnamed-pack/SOURCES-mod-plants.md, "Plan 8 part 3c"). Every helper has no effect while "modPlants" is off or its
// key is missing.
#pragma once

#include <cstdint>
#include <vector>

namespace pvzu::unnamed {

// One plant on the lawn as the shadow update sees it (0x7b06d7: Board::IteratePlants).
struct ShadowLawnPlant {
	int seed = -1;          // the plant's mod seed, or its vanilla seed
	int row = 0, col = 0;
	bool shadow = false;    // its own shadow state (exe +0xcc) as it is now
	bool squished = false;  // exe +0x142
	bool self = false;      // the plant being updated
};
// Points the plant gets from the whole lawn (0x7b06d7..0x7b074b, per neighbour ShadowNeighbourPoints).
int ShadowPointsOnLawn(int selfSeed, int selfRow, int selfCol, const std::vector<ShadowLawnPlant>& lawn);
struct ShadowChange {
	bool shadow = false;  // the new state
	bool entered = false; // 0 -> 1 (0x7b0766 -> 0xc00f80: Charmed Tall-nut's Gargantuar)
	bool left = false;    // 1 -> 0
};
// One tick of hook 311's shadow update (0x7b06b0). In Zen Garden / Tree of Wisdom nothing runs: the state stays.
ShadowChange ShadowStateStep(int selfSeed, bool wasShadow, int gameMode, int points);
// Dream Starfruit shows the shadow tint only when Rand(6) == 0 in its draw (0xc00e0d); 0 = always shown.
int ShadowTintFlickerRange(int seed);

// Moonflower sun (helper 0x7b0200, shared with Primal Sunflower): modes where it never produces (0x7b0213..).
bool ModSunRunsInMode(int gameMode);
// PlantInitialize hook 265 (0x7a4770): a mod sun plant's first drop is Rand(launchRate + 1); 0 = vanilla start.
int ModSunFirstTimerRollRange(int seed, int launchRate);

// Zombie statuses.
uint32_t RepeaterPoisonTint();                 // 0xc10d5a: RGB while the Repeater poison runs (0xRRGGBB), 0 = none
// The statuses' place in Zombie::DrawReanim's colour chain (hook 0x52d2bf, after the burned and low-HP Zomboss
// branches, before mind control and chill): 0xc10d30 Repeater poison green, else 0x95a365 weakened grey (pack
// dreamStarfruit.weakenTintGrey, exe 0x30), both only for zombies with the side record (0xc10e10 / 0x52ae40: mFromWave
// not -2 / -3). rgb is the colour AND the additive colour (0x52d2d9 stores both, 0x52d382 turns the additive on).
// (0x95a160's RGB(69,69,69) block is not reached: nothing jumps to it.)
struct ZombieTint {
	bool on = false;
	uint32_t rgb = 0; // 0xRRGGBB
};
ZombieTint ZombieStatusTint(bool hasSideRecord, int repeaterPoisonCs, int weakenCs);
int ZombieWeakenAdd(int weakenCs, int addCs);  // exe `add word [zombie+0x1a]`: a 16-bit word, it wraps

// Placement.
// Wall-nut first aid compares the exact seeds (0x957620 / 0x957650 list 3, 0x31, 0x32, 0x67): a Charmed Tall-nut and
// a vanilla Tall-nut (the same engine base) never replace each other.
bool ModNutFirstAidAllowed(int cursorSeed, int plantSeed);
// Loaded Cannon (0x7f) is an engine Cob Cannon that covers its column and the next one (hook 40); port: both squares
// must be free (the exe plants it on one free square and lets it overlap).
bool LoadedCannonNeedsTwoSquares();
bool LoadedCannonFits(int col, int gridCols, bool rightSquareFree);

} // namespace pvzu::unnamed
