// PVZU: Unnamed 1.3beta2 Night-plant rules (evidence: the research notes (w2-night) section 1,
// cross-world chill rule: the research notes (w4-fog) X2).
// Pure functions: the engine rolls with its own RNG (Sexy::Rand(range)) and passes the roll in.
// Every constant is a pack rule (pvz-unnamed-pack/data/rules-night-plants.json, keys "nightPlants.*") whose fallback is
// the vanilla behaviour, and every function also returns vanilla while the "nightPlants" feature is off (vanilla
// mode), so an engine site can call it without its own guard. Sites that roll still check NightPlantsOn() first so
// vanilla never draws an extra Rand (deterministic replays).
#pragma once

namespace pvzu::unnamed {

// Feature switch: GameData::Get().Feature("nightPlants") (off in vanilla mode).
bool NightPlantsOn();

// --- Puff-shroom / Sea-shroom "Frost Armour" (study 1.1, hook 688) ----------------------------
// Eaten to 0 HP while awake: Rand(100) < seed + 15 freezes the eater (HitIceTrap, then chill 750 cs).
int PuffFreezeRollRange();
bool PuffFreezesEater(int seedType, bool asleep, int roll);
int PuffFreezeChillCs();

// --- every HitIceTrap: Ice-shroom, Puff/Sea freeze (study 1.7, hook 718) -----------------------
int IceTrapDamage(); // vanilla 20

// --- every ApplyChill: +10 body damage while the zombie has its arm (w4-fog X2, hook 692) ---------
int ChillArmBodyDamage(bool hasArm); // 0 = none (vanilla)
unsigned ChillArmDamageFlags();

// --- Sun-shroom (study 1.2, hooks 267, 293) -----------------------------------------------------
int SunShroomGrowCs(); // in play; vanilla 12000
int SunShroomBonusRollRange();
bool SunShroomBonusSun(bool small, int roll); // grown only: extra small sun (15)

// --- Fume-shroom (study 1.3, .text 0x4680DE) ----------------------------------------------------
int FumeAttackWidth(); // vanilla 340

// --- Grave Buster "Necro Conversion" (study 1.4, hook 152) --------------------------------------
// After its grave's loot coin dropped: a large sun (50), then Rand(3) == 1 adds a sun (25).
bool GraveBusterRefunds();
int GraveBusterRefundCoin(); // engine CoinType
int GraveBusterBonusRollRange();
bool GraveBusterBonusSun(int roll);
int GraveBusterBonusCoin(); // engine CoinType

// --- Hypno-shroom "Void Myth" (study 1.5, hook 636) ---------------------------------------------
bool HypnoGivesBucket(int zombieType); // Normal, Cone, Bucket, Door, Peashooter / Gatling zombie
int HypnoBucketHp();

// --- Scaredy-shroom (study 1.6, cave 0x65198A, hook 290) ----------------------------------------
int ScaredyHp(); // vanilla 300
bool ScaredySecondShot(int launchCounter); // the Repeater's second shot at launch counter 25

// --- Doom-shroom (study 1.8) ---------------------------------------------------------------------
int DoomRadius();   // vanilla 250
int DoomRowRange(); // vanilla 3
int DoomCraterCs(); // vanilla 18000

} // namespace pvzu::unnamed
