// PVZU: Unnamed 1.3beta2 Pool-plant rules (evidence: the research notes (w3-pool) P1-P8).
// Pure functions: the engine rolls with its own RNG (Sexy::Rand(range)) and passes the roll in.
// Every constant is a pack rule (pvz-unnamed-pack/data/rules-pool-plants.json, keys "poolPlants.*") whose fallback is the
// vanilla behaviour, and every function also returns vanilla while the "poolPlants" feature is off (vanilla mode), so
// an engine site can call it without its own guard. Sites that roll still check PoolPlantsOn() first so vanilla never
// draws an extra Rand (deterministic replays).
// The rules are not tied to world 3: the exe's hooks never look at the level, so in Unnamed mode a Tall-nut regrows
// and a Torchwood steps tiers on every lawn (Tall-nut regen skips the Zen Garden, as the exe does).
// Lily Pad 500 cs recharge and the cost changes (Threepeater 350, Tangle Kelp 50, Spikeweed 125, Tall-nut 150/2400)
// are table values in pvz-unnamed-pack/data/plants.json, not code.
#pragma once

#include "Mod/Unnamed/State.h"

#include <vector>

namespace pvzu::unnamed {

// Feature switch: GameData::Get().Feature("poolPlants") (off in vanilla mode).
bool PoolPlantsOn();

// --- Squash (P2: .text 0x4608D6 / 0x4608E0, hook 298) -----------------------------------------------
int SquashReach(bool zombieEating); // vanilla 70 / 110
int SquashSurviveRollRange();
bool SquashSurvives(int roll);      // a new Squash where it landed (land only; in the pool it dies as vanilla)

// --- Threepeater (P3: hook 352) ----------------------------------------------------------------------
enum class PeaKind { Pea, Snow, Fire };
int ThreepeaterPeaRollRange();      // 0 = no roll (vanilla)
PeaKind ThreepeaterPeaKind(int roll); // rolled per pea

// --- Tangle Kelp (P4: hooks 294, 48/49, 360) ----------------------------------------------------------
int TangleKelpDrags();                     // vanilla 1
bool TangleKelpReadyAgain(int dragsDone);  // after a finished drag: ready for the next one instead of dying
enum class TanglePlacement { Vanilla, NotHere, Ok };
// Board::CanPlantAt after the crater/ice/dirt tests: Unnamed sends Tangle Kelp past the "only in water" test and the
// roof pot test straight to the shared tail, which refuses an Imitater under it or a plant on it.
TanglePlacement TangleKelpPlacement(int seedType, bool underPlantIsImitater, bool hasNormalPlant);
// V2 (C24) Board::CanPlantAt: 0x40e233 NOPs drop Spikeweed from the "pool / roof / under-plant -> needs ground" test
// (Spikerock keeps it); hook 49 (0x95a800) on a roof: Spikeweed goes on without a pot and is refused on one.
bool SpikeweedNeedsGround(int seedType); // vanilla: Spikeweed and Spikerock
enum class RoofPot { Vanilla, Ok, Refused };
RoofPot SpikeweedRoofPot(int seedType, bool hasFlowerPot);
// Plant::FindTargetZombie: vanilla skips every zombie not in the pool.
bool TangleKelpSkipsTarget(int zombieType, bool inPool, int bodyMaxHealth);

// --- Jalapeno (P5: hook 346 column, hook 347 barrage, .text cave 0x651E6C Ice-shroom) ------------------
bool JalapenoBurnsColumn();
int JalapenoColumnLanes(int background);  // 6 on pool/fog lawns, else 5
int JalapenoColumnFireDx();               // fire reanim x = plant mX + this
int JalapenoIceCutDx();                   // every row: mIceMinX = max(mIceMinX, mX + this)
// Zombie rect [x, x+w] covers column col: round(x/80) - 1 <= col <= round((x+w)/80) - 1 (x87 round-half-even).
bool JalapenoColumnHits(int rectX, int rectWidth, int col);
int JalapenoIceRollRange();
bool JalapenoIceEligible(int hp, int maxHp); // exploded on its own fuse untouched (300/300)
bool JalapenoLeavesIce(int roll);
int JalapenoIceSeed();                       // SEED_ICESHROOM
struct BarrageShot {
	int dx = 0, dy = 0;    // spawn offset from the Jalapeno's mX, mY
	float vx = 0, vy = 0;  // per-tick velocity, the helper's vx adjustment already added (float32 as the exe)
};
std::vector<BarrageShot> JalapenoBarrage(); // empty = no barrage (vanilla)
int JalapenoBarrageConversions();           // ConvertToFireball calls per shot (2 -> tier 2, blue)
int JalapenoBarrageMotion();                // ProjectileMotion (7 = MOTION_STAR)
int JalapenoBarrageRangeFlags();
int JalapenoBarrageRenderOrder();

// --- Spikeweed / Spikerock (P6: hook 276) --------------------------------------------------------------
bool SpikeweedStunEligible(int seedType, bool zombieHasHead, int butteredCounter, int zombiePhase);
int SpikeweedStunRollRange();
bool SpikeweedStuns(int roll);
int SpikeweedStunCs(); // mButteredCounter after ApplyButter

// --- Torchwood fire tiers (P7: hooks 295, 411, 412; in-place 0x460661) -----------------------------------
enum class TorchAction { None, ToFireball, ToPea };
TorchAction TorchwoodAction(int projectileType, int fireTier);
// Projectile::ConvertToFireball(theGridX) entry. Returns whether the conversion runs (vanilla: hitGridX != gridX)
// and, with the tiers on, steps s.fireTier: 0 -> 1, 1 -> 2, -1 -> 3, 3 -> 4, 2/4 -> 5; a pea a Torchwood just thawed
// (hitGridX != -1, not yet converted) starts at -1 and converts even at that same Torchwood.
bool FireballConvert(ProjectileState& s, int hitTorchwoodGridX, int gridX);
// Tier the Snow Pea's 34% fire shot starts with before its ConvertToFireball: -1 with the tiers on (-> 3), else the
// Day value set directly (3).
int SnowPeaFireShotStartTier();
// Tint of the new fire-pea reanim per tier (false = vanilla look). Tier 3 uses the Day key.
bool FireTierLook(int tier, int& r, int& g, int& b);

// --- Tall-nut (P8: hook 807, 0x796747, cave 0x651A7C) ----------------------------------------------------
// Once per blink tick. s.counter = ticks of unchanged HP, s.extra = last HP. Returns the new HP.
int TallnutRegenTick(PlantState& s, int hp, int maxHp, bool zenGarden);
int TallnutThornsDamage(int plantSeedType); // to the zombie per chew; 0 = none
unsigned TallnutThornsFlags();
bool TallnutSmashed(int plantSeedType, int zombieType); // true: HP -= TallnutSmashDamage() instead of Squish()
int TallnutSmashDamage();

} // namespace pvzu::unnamed
