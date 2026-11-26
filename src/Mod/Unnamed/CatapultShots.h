// PVZU: Plan 9 R3, Unnamed's Catapult projectiles 0x12 (vase) and 0x20 (bug spray) and the vase Catapult's volley, as
// pure rules (rolls passed in). Evidence: the research notes (r3-catapult-shots) and pvz-unnamed-pack/SOURCES-catapult-
// shots.md. Keys "catapultShots.*" (data/rules-catapult-shots.json); a missing key plays the rule like vanilla. Which
// catapult throws which shot (the world-7 forms 0x70 / 0x19 and their per-shot rolls) stays in World7Zombies.h.
#pragma once

#include <string>
#include <utility>
#include <vector>

namespace pvzu::unnamed {

inline constexpr int kShotVase = 0x12, kShotBugSpray = 0x20;
bool IsCatapultShotType(int theExeType);

// ---- the vase Catapult's volley (hooks 547 / 548, 0x910135..0x910234) ----
// At the fire event a not-hypnotised vase Catapult (variant 0x70) draws Rand(range) < volleyBelow; on a hit it throws
// one shot at the first plant of each column 0..8 of its row instead of its one shot.
int CatapultVolleyRollRange(int theVariant, bool theMindControlled);   // 0 = no roll
bool CatapultVolleys(int theRoll);
int VolleyShotRollRange();                 // 3 (hook 537 with [0x701240] = 1)
int VolleyShotType(int theRoll);           // (roll & 1) ? 0x12 : 0x20
struct VolleyCandidate {
	int col = 0;
	int seed = 0;      // engine seed (a mod plant's carrier)
	int modSeed = 0;   // 0 = an ordinary plant
};
std::vector<int> VolleyTargets(const std::vector<VolleyCandidate>& thePlantsInRow);   // indices, board order
int VolleyBallsAfter(int theBalls);        // balls < 1 -> 1

// ---- ball tracks (hook 549, 0x91027a): after the loop-end ball, the count of the four basketball tracks to hide;
// -1 = vanilla (one track at == 4 / 3 / 2 / 1) ----
int CatapultHiddenBallTracks(int theBalls);

// ---- the vase shot 0x12 hitting a plant (hook 393 0x7a1400, 0x7aa380, 0x7aa450) ----
bool VaseShotMakesVase(int theExeType, int theDamageRangeFlags);
int VaseShotDamage();                      // 300 (0x7b08ce), on the already dead plant
bool VaseShotKillsPlant(int theSeed, int theState, bool theSquished, bool theDead, int theBungeeState);
int VaseZombieRollRange();                 // Rand(33)
bool VaseZombieAllowed(int theZombieType); // re-roll when false

// ---- the bug-spray shot 0x20 hitting a plant (hook 394, 0x9102e7) ----
int SprayDamage();                         // 100 (0x7b0910)
int SprayKillRollRange();                  // Rand(20); 0 = no roll
bool SprayKills(int theRoll);              // roll == 0: the plant takes HP + 1
int SprayLaunchRate(int theLaunchRate, int theBaseLaunchRate);   // the slow-down; unchanged without the keys
int SprayFoley();                          // 0x47
int SprayParticle();                       // 0x58
std::pair<int, int> SprayParticleOffset(); // (34, 28) from the plant's mX / mY
int SprayParticleRenderOrder();            // 400000

const std::vector<std::string>& CatapultShotRuleKeys();

} // namespace pvzu::unnamed
