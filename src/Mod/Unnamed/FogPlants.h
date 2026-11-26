// PVZU: Unnamed 1.3beta2 Fog-plant rules (evidence: the research notes (w4-fog) section 2).
// Pure functions: the engine rolls with its own RNG (Sexy::Rand(range)) and passes the roll in.
// Every constant is a pack rule (pvz-unnamed-pack/data/rules-fog-plants.json, keys "fogPlants.*") whose fallback is the
// vanilla behaviour, and every function also returns vanilla while the "fogPlants" feature is off (vanilla mode), so
// an engine site can call it without its own guard. Sites that roll still check FogPlantsOn() first so vanilla never
// draws an extra Rand (deterministic replays).
// The exe's hooks never look at the level, so in Unnamed mode these rules hold on every lawn, not only in world 4.
// Table values (costs, recharges, Starfruit launch rate, Spike / Star damage 1) are in pvz-unnamed-pack/data/plants.json and
// projectiles.json (Plan 1 tables), not here. Sea-shroom's 39 % freeze is the Night rule PuffFreezesEater (shared hook
// 688, Mod/Unnamed/NightPlants.h); chill +10 is the cross-world Night rule ChillArmBodyDamage.
#pragma once

#include "Mod/Unnamed/State.h"

namespace pvzu::unnamed {

// Feature switch: GameData::Get().Feature("fogPlants") (off in vanilla mode).
bool FogPlantsOn();

// --- Plantern (2.2: patch 0x41a702, cave 0x651670, hook 306) -----------------------------------------
int PlanternFogRadius(); // Board::UpdateFog ClearFogAroundPlant size; vanilla 4
// Plant::UpdateShooter when mLaunchCounter ran out: the vanilla `mLaunchRate - roll` (roll = Rand(15)), or with a live
// Plantern in the shooter's 3x3: rate / 2 when rate >= 100 (Gloom-shroom: full rate; Gatling / Cattail: rate / 2 + 25).
int PlanternLaunchCounter(int seedType, int launchRate, int roll, bool planternNear);
int PlanternHealTickCs(); // 0 = no heal (vanilla)
// One heal step on a plant in a Plantern's 3x3: below half HP max HP loses 1, then HP below max gains 1.
void PlanternHealStep(int& hp, int& maxHp);

// --- Cactus (2.3: hooks 290, 282, 806, cave 0x6519c0) ---------------------------------------------------
bool CactusSecondVolley(int launchCounter); // a second FindTargetAndFire at launch counter 25
bool CactusShotAnim(float& animRate, int& shootingCounter); // false = vanilla anim (35, 23 high / 35 low)
// Squished by Zomboni / Catapult; a Cactus already squished (waiting to disappear) is not converted again.
bool CactusBecomesSpikeweed(int plantSeedType, int zombieType, bool alreadySquished = false);
// Projectile::DoImpact end (hook 806, caller == the projectile): a spike lives through up to 64 hits.
bool SpikeSurvivesHit(ProjectileState& s, int projectileType);

// --- Blover (2.4: cave 0x651bc7, hook 803, patch 0x466603) ------------------------------------------------
bool BloverStorm(); // replaces BlowAwayFliers
struct BloverHit {
	bool touched = false;     // false: nothing happens to this zombie (hypnotised)
	bool blowAway = false;    // before the damage (Imps)
	int damage = 0;           // TakeDamage(damage, DAMAGE_FREEZE)
	bool blowIfArmless = false;
	int balloonChillCs = 0;   // flying balloon: mChilledCounter = this (balloons are never blown away)
};
BloverHit BloverOnZombie(int zombieType, bool mindControlled, bool flyingBalloon);

// --- Split Pea (2.5: hooks 352, 314, 325, 326, 278, 354) ----------------------------------------------------
enum class SplitShot { Pea, Snow, Fire };
SplitShot SplitPeaShotKind(bool behindPlant); // projectile mX < plant mX: back head
int SplitPeaSweepRollRange();
bool SplitPeaStartsSweep(int roll);           // front head fired while idle
// V2: Plant::PlantInitialize zeroes +0xEC (memset 0x45dcb4), so a planted Split Pea sweeps at once (1.3beta1 "[quote not included]"): 0 with fogPlants.splitPea.sweepOnPlant, else -1 (idle).
int SplitPeaInitialSweepTimer();
// PlantState sweep fields (sweepTimer -1 = idle). One tick of the oscillator (hook 314).
void SplitPeaSweepTick(PlantState& s);
bool SplitPeaSweeping(const PlantState& s);
int SplitPeaSweepLaunchCounter();             // mLaunchCounter held here while sweeping (normal fire suppressed)
bool SplitPeaSweepFiresNow(const PlantState& s); // every 6 cs
bool SplitPeaSweepEnds(PlantState& s);        // timer >= 500 -> idle
void SplitPeaSweepVelocity(int angle, float& vx, float& vy);
int SplitPeaSweepMotion();                    // ProjectileMotion (7 = MOTION_STAR)
int SplitPeaSweepRangeFlags();

// --- Starfruit (2.6: hooks 286, 287, 400; Plan 9 R1 the research notes (r1-starfruit)) --------------------------
float StarfruitStarSpeedMult(); // the 5 stars' vx, vy; vanilla 1
int StarfruitStarCurve();       // hook 286 [+0x88] = 276 on the 5 stars: kStarCurveBoomerang; vanilla kStarCurveNone
int StarfruitExtraRollRange();  // 0 = no roll
bool StarfruitExtraStars(int roll);
int StarfruitExtraStarCount();      // hook 287: stars added on a hit roll (4); vanilla 0
int StarfruitExtraStarCurve(int i); // the i-th extra star's tag (exe +0x88 = i)
// Projectile::Update after age++ (0x95b1ab / 0x95b1d9), for a type-7 motion-7 star without a kind: boomerang negates
// vx, vy at the two return ages; spiral tag n sets v = speed * age * (cos, sin)(turn * age + (n - 1) pi/2). True = changed.
bool StarCurveStep(int curve, int age, float& vx, float& vy);
// Projectile::Die of a star called from CheckForCollision (cave 0x6511e3): the exe's limit on its counter, which starts at
// 1 (255: lives through 254 Die calls); 0 = vanilla (dies). Kind stars 2..5 use StarKindHitFate (ModPlantRules.h).
int StarPierceLimit();
bool StarSurvivesDie(ProjectileState& s, int limit); // counts the hit (pierceHits) when it lives

// --- Pumpkin (2.7: cave 0x6513b6, cave 0x6519fd, hook 800) ------------------------------------------------
bool PumpkinExplodesWhenBitten(int plantSeedType, int hpBeforeBite);
bool PumpkinExplodesWhenSmashed(int plantSeedType, int zombieType);
int PumpkinExplodeSeed(); // SEED_CHERRYBOMB

// --- Magnet-shroom (2.8: patches 0x4620a2, 0x462124) ---------------------------------------------------------
bool MagnetAllRows();
bool MagnetUnlimitedRadius();

// --- Cattail (2.9: hook 352) ----------------------------------------------------------------------------------
int CattailCobRollRange(); // 0 = no roll
bool CattailFiresCob(int roll);
int CattailCobProjectile(); // PROJECTILE_COBBIG

} // namespace pvzu::unnamed
