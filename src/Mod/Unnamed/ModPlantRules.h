// PVZU: Unnamed 1.3beta2 mod-only plant rules (seeds 0x50..0x7f, the extended plant table at exe 0x860000).
// Evidence: the research notes (mod-plants-behaviour) (one section per plant, addresses are the exe's,
// hook numbers are rows of the research notes (hook-triage)) and pvz-unnamed-pack/SOURCES-mod-plants.md (one bullet per pack key).
// Pure functions: the engine rolls with its own RNG (Sexy::Rand(range)) and passes the roll in; board facts are
// passed as plain numbers. Every constant is a pack rule (pvz-unnamed-pack/data/rules-mod-plants.json, keys
// "modPlants.<plant>.<name>"). Mod plants do not exist in vanilla, so while the "modPlants" feature is off (owned by
// the mod-plant infrastructure: seed table, chooser, hooks ModPlantUpdate/Fire/DoSpecial) or a key is missing, every
// function returns its "no effect" value. No engine wiring here; functions are named by plant so each engine hook
// can call the matching one.
#pragma once

#include <cstdint>
#include <vector>

namespace pvzu::unnamed {

// Feature switch: GameData::Get().Feature("modPlants") (off in vanilla mode).
bool ModPlantsOn();

// Mod seed numbers (exe seed type = vanilla base + 0x50).
enum ModSeed : int {
	kModSeedShadowPeashooter = 0x50,
	kModSeedPrimalSunflower = 0x51,
	kModSeedShrapnelCherry = 0x52,
	kModSeedBowlingWallnut = 0x53,
	kModSeedMysticIceShooter = 0x55,
	kModSeedFireDragonGrass = 0x56,
	kModSeedPrimalRepeater = 0x57,
	kModSeedToxicShadowShroom = 0x58,
	kModSeedGoldenSunShroom = 0x59,
	kModSeedMagicDevourer = 0x5b,
	kModSeedHallucinationShroom = 0x5c,
	kModSeedPrismShroom = 0x5d,
	kModSeedCharmedTallnut = 0x67,
	kModSeedMoonflower = 0x69,
	kModSeedRewindGrass = 0x6b,
	kModSeedDreamStarfruit = 0x6d,
	kModSeedHolyFirePumpkin = 0x6e,
	kModSeedDuskPult = 0x70,
	kModSeedButterPult = 0x72,
	kModSeedStrongCoffeeBean = 0x73,
	kModSeedLeafShield = 0x75,
	kModSeedBossFlower = 0x76,
	kModSeedSniper = 0x78,
	kModSeedElementShroom = 0x7a,
	kModSeedSpiritTail = 0x7b,
	kModSeedLoadedCannon = 0x7f,
};
bool IsModSeed(int seedType); // 0x50..0x7f (exe compares seed >= 0x50 everywhere, e.g. GetCost hook 364 0x7a3eab)

// --- Group A: shadow state, shadow family, poison (the research notes (mod-plants-behaviour), find-A) ------------------

// Shadow state (exe Plant+0xcc). 0x7b0660 / 0xc00dc0 / 0xc06130 list; update 0x7b06b0 from hook 311.
bool IsShadowPlant(int seedType);
bool ShadowUpdateRunsInMode(int gameMode);                     // 0x7b06c5: not modes 0x2b / 0x32
// Points neighbour Q gives plant P (0x7b06ef..0x7b0738, 0xc12bf0). dRow/dCol = Q - P.
int ShadowNeighbourPoints(int selfSeed, int otherSeed, bool otherShadow, bool otherSquished, bool otherIsSelf,
	int dRow, int dCol);
int ShadowThreshold(int seedType);                             // 0xc11460: 3 / 2 / 1; 0 = never (off / not shadow)
bool ShadowStateFromPoints(int seedType, int points);          // 0x7b075b setge
struct ShadowTint {
	bool apply = false;
	int r = 0, g = 0, b = 0, a = 0;
};
ShadowTint ShadowPlantTint(int seedType, bool shadow);         // 0x7a3ce2 (Dream Starfruit flicker is group B)


// Weakened (exe word Zombie+0x1a): see ZombieWeaken* (Dream Starfruit grey star, Dusk-pult, Fire-dragon).

// Shadow Peashooter 0x50 (UpdateShooting hook 327 -> 0x7b0000; DoRowAreaDamage hook 274 -> 0x7a915c).
struct ShadowPeashooterShot {
	bool handled = false;   // false: not this plant or feature off -> the engine's own Fire
	bool beam = false;      // true: DoRowAreaDamage (own row, attack rect) instead of a pea
	int projectileType = -1;
	int originDx = 0, originDy = 0, potDy = 0; // pea start = (x + dx, y + dy [+ potDy on a Flower Pot])
	int beamDamage = 0, beamDamageFlags = 0;   // per zombie in the rect
	int beamPuffs = 0, beamPuffStepPx = 0;     // FUMECLOUD particles (visual)
};
ShadowPeashooterShot ShadowPeashooterFire(int seedType, bool shadow, int row);

// Primal Sunflower 0x51 / Moonflower 0x69 production (0x7b0200, 0xc131f0, 0x7b0305).
struct ModSunDrop {
	int coinType = 0;   // 0 = no coin
	int motion = 0;
	int usableSeed = -1; // coin type 16: exe Coin+0x68
};
struct ModSunDrops {
	ModSunDrop first, second;
};
bool ModSunPlantProduces(int seedType);                         // 0x7a5414: 0x51 / 0x69
int ModSunPlantTimerRollRange();                                // Rand(151) at each drop; 0 = no roll
int ModSunPlantNextTimer(int launchRate, int roll);             // 0x7b02af: rate - 150 + roll
int PrimalSunflowerPacketRollRange(bool shadow);                // 0xc13207 Rand(25); 0 = no roll
ModSunDrops PrimalSunflowerProduce(bool shadow, int roll);
ModSunDrops MoonflowerProduce(bool shadow);

// Toxic Shadow-shroom 0x58: poison (exe word Zombie+0x140 kind, Zombie+0x148 timer).
struct ToxicPoisonSet {
	bool apply = false;
	int kind = 0;  // 1 normal, 2 ignores armour
	int timer = 0;
};
ToxicPoisonSet ToxicShadowShroomEaten(int seedType, bool plantShadow, int currentKind); // 0x7a65b0 (EatPlant hook 688)
struct ToxicPoisonStep {
	int damage = 0;
	bool bodyOnly = false; // TakeBodyDamage (skips helmet/shield) instead of TakeDamage
	int flashCs = 0;       // mJustGotShotCounter, 0 = leave
	int kind = 0;
	int timer = 0;
};
ToxicPoisonStep ToxicPoisonTick(int kind, int timer, bool bossClass); // 0x7a6740
bool ModPlantDodgesZombieShots(int seedType);                         // hook 374 0xc02c30: 0x58 / 0x59 / 0x6d

// Repeater poison (projectile 0x23, exe zombie extension record +8; Primal Repeater is group C; the +500 cs per hit is
// PrimalPoisonAddCs).
struct RepeaterPoisonStep {
	int damage = 0;
	int counter = 0;
};
RepeaterPoisonStep RepeaterPoisonTick(int counter);                   // 0xc10cd0: 1 per cs

// Dusk-pult 0x70 (Fire 0x91040b; DoImpact hook 399 0x910457; shadow 0x7a50b0, hook 279 0x7a5150).
int DuskPultProjectileType(int seedType, int vanillaType);            // 0x21
bool DuskPultLooksInAdjacentRows(int seedType, bool shadow);          // hook 279: row+1 then row-1
struct DuskPultVolley {
	bool above = false, below = false;
	float aboveDy = 0.0f, aboveDz = 0.0f; // added to the row-1 projectile's mPosY / mPosZ
};
DuskPultVolley DuskPultExtraRows(int seedType, bool shadow);
bool DuskShardSplashReaches(int projectileType, int projRow, int projRectX, int projRectW, int zombieRow,
	int zombieRectX, int zombieRectW, bool zombieHypnotised);
struct DuskSplash {
	int damage = 0;
	int damageFlags = 0;
	float pushPx = 0.0f; // added to the zombie's mPosX
	int weakenCs = 0;    // added to the zombie's weakened counter
};
DuskSplash DuskShardSplash(int projectileType);

// ---- Element-shroom 0x7a (Gloom base) ----------------------------------------------------------------
// Plant fields (exe): +0xcc bits (last three elements, newest bit 0; 0 = fire, 1 = ice), +0xcd chosen element,
// +0xce elements gained.
enum ElementShroomStateId { kElementNone = 0, kElementFire3 = 1, kElementIce3 = 2, kElementFire2Ice1 = 3,
	kElementIce2Fire1 = 4 };
int ElementShroomState(int bits, int count);                 // 0x7b149e
struct ElementGain { int bits = 0; int count = 0; };
ElementGain ElementShroomGainElement(int bits, int mode, int count); // 0x7b14ed
struct ElementTick { int resetCountdownTo = -1; bool gain = false; }; // -1 = leave mStateCountdown
ElementTick ElementShroomTick(int stateCountdown, int count); // 0x7b1450 (UpdateAbilities)
int ElementShroomToggleMode(int mode);                       // 0x7a8ef1 (plant clicked)
int ElementShroomCountdownDigit(int stateCountdown);         // 0x7b1536, 0 = no digit
int ElementShroomShootingCounter();                          // hook 281 0x7a8fc0: +0x90 on fire (0 = vanilla)
bool ElementShroomPulseAt(int shootingCounter);              // 0x7b177d: damage pulse 126/98/70
bool ElementShroomPuffAt(int shootingCounter);               // 0x7b170d: puff particle 136/108/80
int ElementShroomRowReach(int state);                        // hooks 273/359: 2 for state 3, else 1 (Gloom)
struct ElementRect { bool own = false; int x = 0, y = 0, w = 0, h = 0; };
ElementRect ElementShroomAttackRect(int state, int plantX, int plantY); // hook 371 0x7a91ae
int ElementShroomIceRollRange(int state);                    // 0x7a90eb: Rand(18) in state 2, else 0 (no roll)
struct ElementHit {
	int damage = 0;        // TakeDamage amount
	int damageFlags = 0;   // DamageFlags
	bool thaw = false;     // remove ice trap and chill (state 1)
	int stripRollRange = 0;// Rand(n) for the helm/shield strip, 0 = none (state 1, non-boss)
	bool freeze = false;   // HitIceTrap (state 2, roll 0)
	int iceTrapCs = 0;     // mIceTrapCounter after the freeze
	int pushPx = 0;        // x += pushPx (state 2, roll 1)
};
// One zombie hit by a pulse (0x7a9070). iceRoll = Rand(ElementShroomIceRollRange(state)) or 0.
ElementHit ElementShroomHit(int state, int vanillaDamage, int vanillaFlags, int iceRoll, int zombieType, int zombieX,
	bool zombieBossClass);
enum ElementStrip { kElementStripNone = 0, kElementStripHelm = 1, kElementStripShield = 2 };
int ElementShroomFireStrip(int roll, bool hasHelm, bool hasShield); // 0xc05600
struct ElementHeal { int hp = 0; int maxHp = 0; };
ElementHeal ElementShroomPulseHeal(int state, int hp, int maxHp); // 0x7b179f (state 4)
bool ElementShroomIgnoresPlantern();                         // cave 0x651717

// ---- Prism-shroom 0x5d (Scaredy base) ----------------------------------------------------------------
bool PrismShroomClickable(bool asleep, int shootingCounter); // hook 54 0x7a6127
struct PrismClick {
	bool fires = false;
	int sun = 0;              // Board sun after the click
	int shootingCounter = 0;  // plant +0x90
	bool buzzer = false;      // not enough sun
	int sunFlashCs = 0;       // Board+0x5578 mOutOfMoneyCounter
};
// canTakeSun = Board::CanTakeSunMoney(cost); freeShot = conveyor-belt level (0x41be50) or the FREE cheat.
PrismClick PrismShroomClick(bool asleep, int shootingCounter, int sun, bool canTakeSun, bool freeShot); // 0x7a6014
int PrismShroomShotCost();                                   // 50
int PrismShroomProjectileType();                             // 0x7a5279: 0x16, -1 = off
int PrismShroomShotDamage(int sunAtShot);                    // 0x7978e6: Projectile+0x80

// ---- Dream Starfruit 0x6d (Starfruit base) + star kinds (Projectile+0x80 = 1..5) ---------------------
int DreamStarfruitColourRollRange(int appCounter);           // 0x959de0: 5 when (counter & 0xc) == 0, else 0
int DreamStarfruitStarKind(int colourIndex);                 // 0x959e6d: +0x80 = colour + 1
int DreamStarfruitReturnTimer(bool shadow);                  // 0xc00e9a: star +0x64 = 30 in shadow state
bool DreamStarfruitAlwaysTargets(bool shadow);               // hook 284 0xc00e40
bool StarKindTurnsHoming(int projectileType, int motion, int kind, int clickBackoff); // hook 403 0xc00ec0
uint32_t StarKindColour(int kind);                           // 0x959cc0 (0xRRGGBB), 0 = none
int StarKindDamageBonus(int kind);                           // 0x959cf0
enum StarHitFate { kStarHitVanilla = 0, kStarHitDies = 1, kStarHitSurvives = 2 };
int StarKindHitFate(int projectileType, int kind, int hitsSoFar); // 0x959f30 (type 7 only)
struct StarZombie {
	int type = 0;          // Zombie+0x24
	int variant = 0;       // +0x134
	int subVariant = 0;    // +0x14c (1 = football-head Newspaper when type 5)
	int bodyHp = 0;        // +0xc8
	float scale = 1.0f;    // +0x11c
	bool canBeFrozen = false; // Zombie::CanBeFrozen
	bool iZombie = false;  // LawnApp::IsIZombieLevel
};
int StarKindImpactRollRange(int kind, const StarZombie& z);  // 0 = no roll is drawn
struct StarImpact {
	bool freeze = false, chill = false, dropHelm = false, hypnotise = false, blowAway = false;
	int weakenAddCs = 0;
};
StarImpact StarKindImpact(int kind, const StarZombie& z, int roll); // 0x959fcd..0x95a0d5
int ZombieWeakenRollRange(int weakenCs, int zombieType, int subVariant); // 0xc05a70: Rand(2) for football Newspaper
int ZombieWeakenedDamage(int damage, int weakenCs, int zombieType, int subVariant, int roll); // 0x95a12b
int ZombieWeakenTick(int weakenCs);                          // hook 622 0x95a140

// ---- Spirit-tail 0x7b (Cattail base) ------------------------------------------------------------------
int SpiritTailRollRange();                                   // 0xc106ba: 36, 0 = off
int SpiritTailProjectileType(int roll);                      // -1 = roll again (0xc106c4..0xc106e9)
int SpiritTailExtraRollRange(int projectileType);            // 0xc10600: cabbage 2, star 6, else 0
int SpiritTailProjectileField80(int projectileType, int roll); // +0x80 after the shot is made
int SpiritTailMotion();                                      // 0x467393: 9 homing, -1 = off

// ---- Mystic-Ice Shooter 0x55 (Snow Pea base) ----------------------------------------------------------
struct MysticIceShot {
	bool fires = false;
	int x = 0, y = 0, projectileType = 0, motion = 0, damageRangeFlags = 0;
	float velX = 0.0f, velY = 0.0f;
};
int MysticIceVelRollRange();                                 // 2000
MysticIceShot MysticIceTickShot(int plantX, int plantY, int roll); // 0xc04761..0xc047b4
bool MysticIceConvertsProjectile(int projectileType, int hitTorchwoodCol, bool dead); // 0xc047d4
int MysticIceConvertedType();                                // 1
struct MysticIceLife { int launchRate = 0; bool dies = false; int deathParticle = -1; };
MysticIceLife MysticIceLifeTick(int launchRate);             // 0xc047e7
int MysticIceUpgradeSeed(int seedType);                      // 0xc04843: Peashooter -> Snow Pea on death, -1 = none
bool ModPlantZomboniFreezeImmuneB(int seedType);             // 0x7b1812/0xc04e70 (0x7a, 0x55 of this group)

// --- Sniper 0x78 (UpdateAbilities 0x7a5400 -> 0x7b1300; pick 0x7b1200; HP sum 0x794ba1) ------------------------
struct SniperCandidate {
	int x = 0;              // Zombie+0x8 mX
	int zombieType = 0;     // +0x24
	int variant = 0;        // Unnamed elite form (exe +0x134); 0x7f on a Newspaper = the 7-10 boss
	int bodyHp = 0, bodyMaxHp = 0;      // +0xc8 / +0xcc
	int helmType = 0, helmHp = 0;       // +0xc4 / +0xd0
	int shieldType = 0, shieldHp = 0;   // +0xd8 / +0xdc
	int flyingHp = 0, flyingMaxHp = 0;  // +0xe4 / +0xe8
	bool hasHead = true;                // +0xba
	bool effectedByDamage = true;       // Zombie::EffectedByDamage(1) (ground damage)
};
int SniperHpSum(const SniperCandidate& z);                                       // 0x794ba1
bool SniperCanPick(const SniperCandidate& z, int sniperRightX);                  // 0x7b1232..0x7b1269
int SniperPickTarget(const std::vector<SniperCandidate>& zombies, int sniperRightX); // index or -1; ties: later
bool SniperKeepsTarget(const SniperCandidate& z, int sniperRightX);              // 0x7b1367..0x7b138c
int SniperCountdownAfterTick(int countdownAfterDecrement);                       // 0x7b1314: < 1 -> 500
struct SniperTick {
	bool warn = false;  // sound 0x52 at 120
	bool fire = false;  // TakeDamage(damage, 0) + sound 0x37 at 5
	int damage = 0;
	int foley = 0;      // the FoleyType to play this tick: 0x52 with the warning, 0x37 with the shot (0x7b13a0 / 0x7b13c2); 0 = none
};
SniperTick SniperTimer(int countdown); // countdown value this tick, only acted on while aiming (0x7b1392..0x7b13c7)

// --- Primal Repeater 0x57 (Fire hook 352 -> 0xc10ba0; impact 0xc10bf0 / 0xc10c71; damage 0x7b0890) -----------
int PrimalRepeaterRollRange();                              // 0 = no roll
int PrimalRepeaterShotType(int roll, int vanillaType);      // 0 -> 0x22, 1 -> 0x23, else vanillaType
bool PrimalRepeaterSecondPea();                             // hook 290 0x7a887f: fires at launch counter 25 too
int PrimalKnockbackPx(int projectileType);                  // 0xc10bfb: 0x22 -> +30 to Zombie mPosX, else 0
int PrimalPoisonAddCs(int projectileType);                  // 0xc10c8b: 0x23 -> side-table +8 += 500, else 0

// --- Shrapnel Cherry 0x52 (DoSpecial 0x794000; impact 0x79ef88; Update hook 400 0x79f067) ----------------------
struct ShrapnelShard {
	float velX = 0.0f, velY = 0.0f; // Projectile+0x3c / +0x40, motion 7
	int kind = 0;                   // Projectile+0x84: 2 = core (re-plants a Cherry), 1 = flying shard
};
std::vector<ShrapnelShard> ShrapnelCherryShards();          // 7 stars (empty when off)
int ShrapnelShardHitDamage(int kind);                       // 0x79efa8: 250 on top of the star hit, 0 = none
bool ShrapnelCoreReplants(int kind, int ageCs);             // 0x79f067: kind 2 at age 1400
int ShrapnelCoreReplantColumn(int starX);                   // 0x79f082: x / 80
bool ShrapnelCherryExplodesWhenSquished(int seedType);      // hook 303 0x79436a (vanilla: Cherry Bomb only)

// --- Loaded Cannon 0x7f (0xc04b53 -> 0x461069; CanPlantAt 0xc11840; MouseDown 0xc049c0 / 0xc11870) -----------
bool LoadedCannonArmsByItself();                            // false: ARMING never ends without a load
enum class CannonLoad { None, Kernel, Doom };
CannonLoad LoadedCannonLoad(int cannonState, int plantedSeed); // state 0x23 + seed 0x22 / 0xf
int LoadedCannonPacketPenaltyCs(CannonLoad load);           // 0xc04a71: -6000 on mRefreshCounter
int LoadedCannonCobBurnCap(int ammoSeed);                   // 0xc1179f: ammo 0x22 -> 3000, else 0 (vanilla 1800)
int LoadedCannonDoomBurnCap();                              // 0xc11894: 3600
bool LoadedCannonDoomClearsGraves();                        // hook 345 0xc118f0: no crater, graves removed
// ApplyBurn with an optional cap (0xc11770): damage to take, or -1 = burn to death. cap 0 = vanilla 1800.
int BurnCapDamage(int armourAndBodySum, int cap);

// --- Butter-pult 0x72 (Fire 0x7a529b; FindTargetAndFire 0x7a53c0; GetDamageRangeFlags 0x7a6233) ----------------
int ButterPultProjectile(int vanillaType);                  // -> 12 (BUTTER)
int ButterPultButterStreak();                               // projectile +0x88 for RoofPlants ButterCs: 0
int ButterPultDamageRangeFlags(int vanillaFlags);           // -> 13 (like Melon)

// --- Bowling Wall-nut 0x53 (UpdateAbilities 0x7a4730; GetPlantAttackRect 0x7a47b7; eat 0x959423) -------------
bool BowlingWallnutRolls(bool isWallnutBowlingLevel);       // true in Unnamed mode
bool BowlingWallnutEdible();                                // false in Unnamed mode
bool BowlingWallnutRandomAnimRate();                        // false: no RandF 12..18 like Wall-nut
// Plant::UpdateBowling on any lawn (geometry, no pack key). The exe's UpdateBowling (0x462ce0) tests the row with
// GridToPixelY(0, row) (0x462d77 `xor ecx,ecx`) and bounces at rows 0 / 4 (0x462dab, 0x4630d9), so on the roof the nut
// never reaches its hit test and on a 6-row lawn it rolls into row 6. The port follows the lawn instead:
int BowlingRoofSlopeY(int x);                               // roof: ground height above the flat part at x
int BowlingLastRow(bool sixRows);                           // 4, or 5 on a 6-row lawn
// Board::GetPlantsOnLawn: a rolling nut holds its planting square (hook 41 flying slot) only while its middle is in it.
bool BowlingNutInItsSquare(int plantCol, int startRow, int row, int centreCol);

// --- Mod plants: shared rules read for fork D (the research notes (mod-plants-behaviour)) -------------------------
// Seed the vanilla PlantInitialize switch runs for a mod seed (0x7a3e1a: seed - 0x50); other seeds unchanged.
int ModPlantInitSeed(int seed);
// Board::GetPlantsOnLawn slot for a mod seed (hook 41, 0x7a5040): 0 = not a mod rule (vanilla), 1 = under,
// 2 = pumpkin, 3 = flying (Coffee Bean slot).
int ModPlantLawnSlot(int seed);
// Coffee-type planting (hook 47 + patch 0x40e137): see CoffeeLikePlantingReason (Strong Coffee Bean).
// 0x7aa320: zombies the mod plants never touch (max HP > 120000, Zomboss, Newspaper boss form).
bool ModPlantBossClass(int zombieType, int bodyMaxHp, bool newspaperBossForm);

// --- Rewind Grass 0x6b (0x7b0a00) ------------------------------------------------------------------------------
int RewindGrassBlowDelayCs();     // 0x7b0a47: 50 (0 = no rewind)
float RewindGrassBlowAnimRate();  // 0x7b0a5a: 20.0
// 0x7b0ad3..0x7b0b12: is this zombie healed (and maybe moved)?
bool RewindGrassAffects(bool bossClass, int phase, int zombieX, bool hasHead);
// 0x7b0b1d..0x7b0b2f: is it also sent back? (not Bungee, not hypnotised, not blowing away)
bool RewindGrassSendsBack(int zombieType, bool mindControlled, bool blowingAway);
float RewindGrassBackX();         // 0xc054f1: 800.0
bool RewindGrassResetsPhaseCounter(int phase); // 0xc054e0: Dancer dancing in (40)
struct ZombieHpSet {
	int body, bodyMax, helm, helmMax, shield, shieldMax, flying, flyingMax;
};
void RewindGrassHeal(ZombieHpSet& hp); // 0x7946a7: each HP raised to its max
// Squish, eat, bite, projectile hit, Gargantuar smash, Catapult target, lob collision all skip it.
bool RewindGrassUntouchable(int seed);

// --- Charmed Tall-nut 0x67 -------------------------------------------------------------------------------------
struct CharmedTallnutBite {
	int counter = 0;   // new Plant+0x114
	bool summon = false;
};
// 0x9500a8: one zombie bite (fromBite = third argument != 0). damage after CharmedTallnutBiteDamage.
CharmedTallnutBite CharmedTallnutOnBite(int counter, int damage, int hpBefore, bool fromBite);
int CharmedTallnutSummonRollRange();       // 0x9500f0: Rand(3); 0 = no summon
int CharmedTallnutSummonType(int roll);    // 0x950103: {Newspaper, Football, Gargantuar}; -1 = none
int CharmedTallnutSummonBodyHp(int bodyHp); // 0x950121: x1.35 (body only)
int CharmedTallnutRiseCounterCs();          // 0x950147: 50
int CharmedTallnutBiteDamage(int damage, bool shadow); // 0xc01040: x1.5 in shadow state
int CharmedTallnutShadowThreshold();        // 0xc1147e: 2 (0 = no shadow rule)
struct CharmedTallnutGarg {
	bool summon = false;
	int gargBodyHp = 0;   // body and max
	int plantHpAfter = 0;
};
// 0xc00f80: on entering the shadow state (0 -> 1)
CharmedTallnutGarg CharmedTallnutEnterShadow(int hp, int maxHp);
struct NutRegen {
	int hp, counter, lastHp;
};
// cave 0x651dd8 (via 0xc02230): shadow-state regen tick; returns the new values (no change when not shadow)
NutRegen CharmedTallnutRegenTick(bool shadow, int gameMode, int hp, int maxHp, int lastHp, int counter);
int CharmedTallnutBiteBackRollRange();      // 0xc021e9: Rand(200); 0 = no bite-back
struct CharmedTallnutBiteBack {
	int damageToEater = 0;
	bool charmEater = false;
};
// 0xc021ad: an eating tick on a shadow-state Charmed Tall-nut
CharmedTallnutBiteBack CharmedTallnutOnEaten(bool shadow, int eaterPhase, bool eaterHasObject, int eaterVariant, int roll);
// seeds that count as a Tall-nut for Pogo, squash, CanTargetPlant, cracked frames, chew sound, replanting
bool ModSeedCountsAsTallnut(int seed);

// --- Hallucination-shroom 0x5c (0x7a65eb in EatPlant, after the plant's HP reaches 0) -------------------------
bool HallucinationTurnsEaterToImp(int seed, int plantHpAfterBite, bool eaterBossClass);
struct HallucinationImp {
	int zombieType = 0, bodyHp = 0, bodyMaxHp = 0;
	int rectX = 0, rectW = 0, attackRectX = 0, attackRectW = 0;
	int reanimType = 0, particle = 0;
};
HallucinationImp HallucinationImpMorph(int bodyHp); // helm, shield and flying all become 0

// --- Magic Devourer 0x5b (0x7b07c0) ----------------------------------------------------------------------------
// extra countdown tick in state 9 (eating); returns the new mStateCountdown
int MagicDevourerCountdownTick(int state, int countdown);
bool MagicDevourerSwallowsNow(int state, int countdown, int col, int row, bool bowlingLevel);
int MagicDevourerCardCoinType(); // 0x7b083b: 16 (COIN_USABLE_SEED_PACKET); 0 = none
int MagicDevourerCardCoinMotion(); // 0x7b0839: 2 (from plant)

// --- Holy-fire Pumpkin 0x6e ------------------------------------------------------------------------------------
// GetPumpkinAt, stacking, shovel, render order, draw: 0x6e acts as SEED_PUMPKINSHELL (not for the Pumpkin blasts)
bool HolyFirePumpkinActsAsPumpkin(int seed);
// 0x95bfd0 / 0x7b1800: a 0x6e plant in the square keeps the whole square from the Zomboni freeze
bool HolyFirePumpkinGuardsSquare(int seedInSquare);

// ---- Fork E: Fire-dragon Grass 0x56, Golden Sun-shroom 0x59, Strong Coffee 0x73, Leaf Shield 0x75, Boss Flower 0x76

// --- Fire-dragon Grass (0x56; helper 0x7b0400, 0xc026c0, 0xc055b0, 0xc060b0, 0xc060fb, 0xc061c0) ---
// Range test used both to start the attack (state READY) and for the hit pass (0x7b050b..0x7b053b, 0xc026c0, 0xc061c0).
// effectedByFlags = Zombie::EffectedByDamage(FireDragonDamageFlags()).
bool FireDragonInRange(int zombieType, bool hypnotised, bool effectedByFlags, int zombieRow, int zombieX, int plantRow,
	int plantX, int plantCol);
int FireDragonDamageFlags();      // 9 = GROUND|DOG (0x7b051d), 0 = off
int FireDragonAttackCs();         // 70: mStateCountdown when the attack starts, state -> 10 (0x7b05a8); 0 = off
int FireDragonHitAtCountdown();   // 12: the flame lands when state 10 and countdown == 12 (0x7b0434); -1 = off
int FireDragonFlameBudget();      // 1800 total damage per flame (0xc060fb); 0 = off
int FireDragonHitDamage(int zombieBodyMaxHp); // 100 + round(max/15), cap 600 (0xc055b0)
struct FireDragonHit {
	int damage = 0;     // passed to Zombie::TakeDamage(damage, 0)
	int budgetLeft = 0; // budget for the next zombie of the same flame
	int poisonAdd = 0;  // add to the zombie's poison/shadow-mark counter (exe Zombie+0x1a) in shadow state
};
FireDragonHit FireDragonSpend(int damage, int budgetLeft, bool shadowState); // 0xc060b0..0xc060e9
bool FireDragonClearsChillAndFreeze(); // 0x7b054c: RemoveIceTrap, mChilledCounter = 0 on every zombie it hits
bool FireDragonZomboniFreezeImmune();  // 0x7b1817: not frozen by the Zomboni ice

// --- Golden Sun-shroom (0x59; helper 0x7b0ce0, hook 472 0x7a6505, hook 374 0xc02c5e) ---
struct GoldenSunShroomStep {
	int state = -1;          // new mState, -1 = unchanged
	int disappearCs = -1;    // set mDisappearCountdown, -1 = unchanged
	int timerCs = -1;        // set the burst timer (exe Plant+0xbc), -1 = unchanged
	bool dropSun = false;    // Board::AddCoin(x, y, coinType, coinMotion)
	int coinType = 0;
	int coinMotion = 0;
};
// One tick. timerCs = the burst timer (exe +0xbc, counted down by Plant::Update like the flash countdown).
GoldenSunShroomStep GoldenSunShroomUpdate(int gameMode, bool onBoard, bool notOnGround, int state, int timerCs);
int GoldenSunShroomPacketStartCounter(int seedType, int imitaterType); // 3500 for 0x59 (0x7a6500), -1 = vanilla path
// Zombie shots fly over it: ModPlantDodgesZombieShots (the one hook 374 list, 0xc02c5e is its 0x59 entry).

// --- Strong Coffee Bean (0x73; helper 0x7b0b90; Plant::Die hook 362 0xc05db2; CanPlantAt hook 47 + patch 0x40e137) ---
bool StrongCoffeeTriggers(int doSpecialCountdown, bool onBoard); // countdown == 1 (0x7b0b9b)
bool StrongCoffeeWakes(int coffeeRow, int coffeeCol, int plantRow, int plantCol, bool plantNotOnGround); // 3x3
int StrongCoffeeWakeCs(); // 100 -> mWakeUpCounter (0x7b0c05); 0 = off
// Coffee-like placement for 0x23 / 0x5b / 0x73: returns a PlantingReason (0 OK, 1 NOT_HERE, 13 NEEDS_SLEEPING).
int CoffeeLikePlantingReason(int seedType, bool hasFlying, bool hasNormal, bool normalAsleep, int normalWakeUpCounter,
	bool normalGrabbedByBungee);
struct CoffeeHeal {
	int hp = 0;
	int maxHp = 0;
};
// The plant left at a dying Coffee / Strong Coffee's square (state 2 = it did its job) (0xc11ce0, 0xc05db2..0xc05dea).
CoffeeHeal CoffeeBeanDeathHeal(int dyingSeed, int dyingState, int hp, int maxHp);

// --- Leaf Shield (0x75; helper 0x7b0da0, damage lookup 0x7b0890) ---
bool LeafShieldReflects(int projDamageRangeFlags, int projRow, int projX, int leafRow, int leafX, int leafWidth);
int LeafShieldHpCost(int projectileDamage); // (damage >> 3) + 2 (0x7b0e27)
struct LeafShieldBounce {
	int motionType = 0;
	bool negateVelX = false;
	int damageRangeFlags = 0; // new mDamageRangeFlags (1 = hits zombies)
	int flashCs = 0;          // leaf mEatenFlashCountdown
};
LeafShieldBounce LeafShieldBounceOf(int motionType); // 0x7b0e18..0x7b0e8f
// The mod's projectile damage lookup (0x7b0890; also hooks 0x7a0f34, 0x7a12b3, 0x7a14b3).
// tableDamage = ProjectileDefinition damage (types 0..13); field80 = exe Projectile+0x80 (type 22).
int ModProjectileDamage(int projectileType, int tableDamage, int field80, bool iZombieLevel);

// --- Boss Flower (0x76; hooks 291 0xc00080, 292 0xc000b0, 310 0xc00050) ---
int BossFlowerRollRange();             // 128 (0xc000d6), 0 = off
int BossFlowerSeedFromRoll(int roll);  // table 0xc000fd: the seed, or -1 = roll again
int BossFlowerCoinType();              // 16 COIN_USABLE_SEED_PACKET
int BossFlowerCoinMotion();            // 2 COIN_MOTION_FROM_PLANT

} // namespace pvzu::unnamed
