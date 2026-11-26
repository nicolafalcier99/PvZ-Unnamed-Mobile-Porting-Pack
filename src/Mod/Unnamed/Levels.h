// PVZU: Unnamed 1.3beta2 Day waves and level rules (evidence: the research notes (levels)).
// Pure functions: engine enums are passed as ints, rolls are passed in. Every constant is a
// "dayLevels.*" rule (pack data/rules-day-levels.json); a missing key gives the vanilla behaviour.
#pragma once

#include <functional>

namespace pvzu::unnamed {

// Wave points after the level multiplier (hook 22, 0x79ab09): Unnamed x4 for every adventure level,
// replacing vanilla's x1 / x3 (mini-boss) / x4 (bowling). Non-adventure modes keep vanillaAfter.
int WaveBudget(int pointsBeforeMultiplier, int vanillaPointsAfter, bool adventure);

// Hook 23 (0x7954e0): on level 1 the random pick written to mZombiesInWave[3][1] becomes a Flag Zombie.
bool ForceFlagAt(int level, int wave, int slot);

// Patch 0x40d6a3 (jge -> jmp): Unnamed's CanZombieSpawnOnLevel drops the `level < mStartingLevel` test.
bool AllowCheckUsesStartingLevel();

// Pole Vaulter vault grave (.text 0x525fa0 -> cave 0x651a97).
int VaultGraveColumn(int zombieX);            // (uint16)mX / 80 - 1
bool PlantBlocksVault(int seedType, int plantState); // Tall-nut-like plants bonk the vaulter (0x7b1910)

struct VaultInput {
	int variant = 0;       // ZombieState::variant
	bool hasPlant = false; // FindPlantTarget(VAULT) found a plant
	int plantSeed = -1;
	int plantState = 0;
	bool hypnotised = false;
	bool graveMade = false; // exe Zombie+0x12c
};
struct VaultDecision {
	bool bonk = false;  // vanilla Tall-nut bonk path
	bool grave = false; // make a grave at VaultGraveColumn(mX), mRow
	bool vase = false;  // Plan 9 R2: the vase pole's vault (0xc036e0, Mod/Unnamed/VasePole.h)
};
// Decides one frame inside the IN_VAULT window (anim time in (0.6, 0.7]).
VaultDecision DecideVault(const VaultInput& in);

// Hook 88 (0x79552b / 0xc0358d): grave-rising zombie type re-roll.
bool GraveRerollAccept(int roll);
// rand(n) returns 0..n-1. Returns the type to raise (engine ZombieType value).
int GraveRiseType(int vanillaType, const std::function<int(int)>& rand);

// Hook 595 (0x95b2e0): phase after rising from a grave (engine ZombiePhase value).
int GraveRisePhase(int zombieType, int vanillaPhase);
int JackRiseFuse(int roll1000);  // mPhaseCounter for a risen Jack-in-the-Box
int JackRiseFuseRange();         // Rand range for JackRiseFuse

// Zombie::ApplyBurn (.text 0x532b96 -> 0x6514b0): "too tough to ash" uses body + helm + shield.
int AshThresholdHp(int body, int helm, int shield, bool unnamed);
int AshThreshold();
bool TooToughToAsh(int body, int helm, int shield, bool unnamed);

} // namespace pvzu::unnamed
