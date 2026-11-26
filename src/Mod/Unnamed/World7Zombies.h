// PVZU: Unnamed 1.3beta2 world-7 zombie rules and the ZomBotany zombies in Unnamed (Plan 7 Task 2), as pure rules
// (rolls are passed in). Evidence: the research notes (w6-w7-and-new-content) sections 2.4, 4, 4.1, 8
// and pvz-unnamed-pack/SOURCES-world7-zombies.md (exe addresses, including the re-reads done for this task). Every constant
// comes from data/rules-world7-zombies.json (keys "world7Zombies.*") with the vanilla value as fallback, so a pack
// without a key plays that rule like vanilla.
//
// "World 7" in the exe is 0x7aa4c0: game mode Adventure and the PROFILE's level (PlayerInfo+0x24) 61..70, never the
// board's level. A few checks use "profile level > 60" instead (true in 71-73 and in any mode). Both helpers below take
// the board level only to make that explicit: they ignore it.
//
// Already ported elsewhere and reused, not repeated here: Gargantuar / Giga staff 0x106 (RoofZombies.h, its 1-in-20 is
// the vanilla aPoleHit % 20 == 1, study 8.9), the silver-coin Normal 0x105 (DayZombies.h; its world-7 Football copy is
// forced gold here), the Pole Vaulter road sign 0x29 (DayZombies.h; its world gate comes from here), the Newspaper
// Almanac paper (NightZombies.h, profile level > 60).
#pragma once

#include "ConstEnums.h"
#include "Mod/Unnamed/Options.h"

#include <string>
#include <vector>

namespace pvzu::unnamed {

namespace w7 {
constexpr int IMP_GARGANTUAR = 0x2b;  // Zombie+0x134 of an Imp that became a Gargantuar on landing (hook 571)
constexpr int VASE_CATAPULT = 0x70;   // Catapult with 10 bug-spray "vases" instead of basketballs (hook 498)
constexpr int TALLNUT_ODD = 0x101;    // Tall-nut Zombie 1-in-12 form (hook 508); no behaviour found for it
constexpr int BLACK_FOOTBALL = 0x19;  // Football Zombie+0x14C (ZombieState::subVariant): black armour (green is 0x1A)
}

// ---- the two gates (exe 0x7aa4c0 and `cmp [PlayerInfo+0x24], 0x3c`) ----
bool IsWorld7(bool isAdventure, int playerLevel, int boardLevelIgnored = 0);
bool PlayerLevelAbove60(int playerLevel, int boardLevelIgnored = 0);
// Day's RollEliteVariant(worldNumber): the road sign 0x29 needs worldNumber == 7 (dayZombies.pole.roadSignWorld).
int RoadSignWorldNumber(bool world7);

const char* World7FormName(ZombieType type, int variant, int subVariant);   // "imp-gargantuar", "vase", ... or nullptr

// ---- Flag (hook 91, 0x7a1d2b): a Flag Zombie of a wave in world 7 brings one Flag per other row, all bucketed ----
bool FlagFillsRows();
// rows top..0 without flagRow; top = 4, or 5 on the pool / fog lawns (bg 2 / 3)
std::vector<int> FlagExtraRows(int flagRow, bool sixRowLawn);
int FlagBucketHp();   // helm type 2 (bucket) 1500 / 1500, the flag itself too

// ---- Imp (hook 571, 0x7a1f36): world 7, on landing Rand(3) == 1 -> type Gargantuar, variant 0x2b, HP +930 ----
// setting = the player's elite option (the jump at 0x7a1f53 is patched code): "none" -> no roll, "all" -> `jo`, the
// Rand(3) is drawn and always hits (0x7b2000).
int ImpGargRollRange(int setting = eliteoption::DEFAULT);
bool ImpBecomesGarg(int roll, int setting = eliteoption::DEFAULT);
int ImpGargHpBonus();

// ---- Football black armour (hook 489: 0x9dd321..0x9dd419, 0xc11df0, 0x9dd460) ----
// The Night chain draws Rand(4) green, Rand(13) gold, then Rand(3) unless gold; a non-green, non-gold Football whose
// Rand(3) is 0 turns black when the profile level is > 60 (any mode). P = 3/4 * 12/13 * 1/3 = 3/13.
bool FootballBecomesBlack(int extraRoll, bool green, bool gold, int playerLevel);
int BlackFootballBodyHp(int vanilla);   // 800
int BlackFootballHelmHp(int vanilla);   // 4500
// A gold Football painted black (0x9dd423: variant 0x102 -> 900 / 2200; only the elite option "all" makes one here,
// NightZombies RollFootballAllElite).
int GoldBlackFootballBodyHp(int vanilla);   // 900
int GoldBlackFootballHelmHp(int vanilla);   // 2200
struct W7ArtImage {
	std::string track;
	std::string file;   // pack file name without extension
	bool operator==(const W7ArtImage&) const = default;
};
std::vector<W7ArtImage> BlackFootballArt();      // 8 parts, images/Black_football_* (mod resources.xml 62-75)
std::string BlackHelmImage(int damageIndex);     // 1, 2 -> Black_football_helmet2 / 3 (hooks 700 / 701)

// ---- Catapult (hooks 498 / 0x910000, 537 / 0x7a1940) ----
// Spawn roll Rand(100): 50..59 with profile level > 60 -> 0x70 with 10 balls (the pole head 0..49 is RoofZombies).
// setting = the player's elite option: "all" patches the bounds to 83 (0x796e16) .. 100 (0x910002).
int CatapultWorld7Form(int roll, int playerLevel, int setting = eliteoption::DEFAULT);
int VaseCatapultBalls(int vanilla);
std::vector<W7ArtImage> VaseCatapultArt();       // the 4 Zombie_catapult_basketball tracks -> IMAGE_BUG_SPRAY
// Each shot: a vase catapult draws Rand(100) < 35 -> exe projectile 0x20; a pole-head catapult in world 7 (or the 7-10
// boss level) draws Rand(100) < 50 -> exe projectile 0x12 and its balls drop to 5 at most. 0x12 / 0x20 are mod
// projectile types on the basketball carrier; their hits and the vase Catapult's volley: Mod/Unnamed/CatapultShots.h.
int CatapultShotRollRange(int variant, bool world7OrBossLevel);
struct CatapultShot {
	int exeProjectile = 9;   // 9 = vanilla basketball
	int balls = 0;           // balls left after the cap
	bool operator==(const CatapultShot&) const = default;
};
CatapultShot RollCatapultShot(int variant, int roll, int balls);

// ---- Jack-in-the-Box (hook 707, 0x7a0a96): world 7, an elite Jack hit while body HP <= 370 pops at once and the hit
// does no damage (TakeDamage returns at 0x531877) ----
bool JackPopsWhenHit(bool world7, ZombieType type, int variant, int bodyHp);

// ---- silver coin copy (hook 686 0xc11260: [0x9dd48e] = 2 around the call): the world-7 Football copy is gold ----
bool CoinFootballIsGold();

// ---- Tree of Wisdom plant HP (hook 268, 0x7a1c60) ----
// Adventure, Tree bought (purchase byte == 1), (adventure finished or profile level > 60), tree > 100 ft:
// HP x (1 + min(height - 100, 200) / 100), rounded to nearest (fistp); max HP = HP.
int TreeBuffedPlantHp(int hp, bool isAdventure, int treePurchase, int finishedAdventures, int playerLevel, int treeHeight);

// ---- ZomBotany zombies in Unnamed (hooks 507-511, patches 0x523825, 0x523957, 0x523a81; every mode) ----
struct ZomBotanyArmour {
	int bodyHp = 0;
	HelmType helm = HELMTYPE_NONE;
	int helmHp = 0;
	const char* helmPrefix = nullptr;   // "anim_cone" / "anim_bucket" shown on the body reanim, nullptr = none
	int doorHp = 0;                     // > 0: screen-door shield (SHIELDTYPE_DOOR) with that HP
	bool operator==(const ZomBotanyArmour&) const = default;
};
bool IsZomBotanyType(ZombieType type);   // 26..31
ZomBotanyArmour ZomBotanyStats(ZombieType type, int vanillaBodyHp, HelmType vanillaHelm, int vanillaHelmHp);
int TallnutOddRollRange();               // Rand(12) (0 with "no elites")
int RollTallnutOdd(int roll);            // == 0 -> 0x101

// ---- Peashooter Zombie fire-rate ramp (Plan 9 R5; hook 572's tail 0x7958fe, the research notes (r5-peashooter-zombie))
// After every pea of a type-26 zombie (any mode, hypnotised too): roll Rand(75); roll >= n -> n + 1 (n = raises so far,
// exe Zombie+0x12C, the port's ZombieState::stored); next mPhaseCounter = 150 - n. n stops at 75: twice the fire rate.
// Keys "peaHeadRamp.*" (data/rules-peahead-ramp.json, feature peaHeadRamp); missing keys = vanilla 150 every shot.
int PeaHeadRampRollRange();                          // 75; 0 = no ramp
int PeaHeadRampAfterShot(int raises, int roll);      // the new raise count
int PeaHeadShotInterval(int raises, int vanilla);    // vanilla (150) - raises, raises clamped to 0..range
const std::vector<std::string>& PeaHeadRampRuleKeys();

// ---- Gatling Pea Zombie (hook 575: 0x795ce6 / 0x795d88): each pea, Rand(100) < 50 adds two peas at +-32 degrees ----
int GatlingSpreadRollRange();
bool GatlingSpreads(int roll);
float GatlingSpreadVx();   // 2.82117 px / tick (leftward for a normal zombie)
float GatlingSpreadVy();   // 1.78649 px / tick, one pea up, one down
// Hypnotised Peashooter / Gatling heads fire the player's pea (type 0, damage flags 1) to the right (hooks 572, 576).
bool HypnotisedHeadsFirePlayerPeas();

// ---- Jalapeno Zombie (hook 573, 0x958130) ----
// Not hypnotised: also burns the hypnotised zombies of its row and the Zomboss ice ball in its row (then the vanilla
// plant burn). Hypnotised: Plant::BurnRow on its row and the row's ice melts (Board+0x624 + row*4 = 20).
bool JalapenoBurnsHypnotised();
int JalapenoIceTimer();

// ---- Squash Zombie (hooks 577-584) ----
// It jumps as soon as a plant is in reach (vanilla: once it starts eating), lands on that plant's x, and the landing
// also hits the other side's zombies under it for 1800.
bool SquashJumpsOnReach();
int SquashLandingZombieDamage();

// ---- Wall-nut Zombie self-copy (Plan 9 R6; Almanac 神谕心法; the research notes (r6-wallnut-zombie)) ----
// Zombie::Update entry (hook 618 -> 0xc10cd0 -> 0x7971ea), every mode: a Wall-nut Zombie whose helm HP has not gone
// down for 3600 ticks adds a Wall-nut Zombie (fromWave 0) at its own row and x, then hits it with TakeDamage(copy
// helm - own helm, flags 1) so the helms match. Rules "wallnutZombie.copy.*" (data/rules-wallnut-zombie.json; missing
// = off). The exe keeps the quiet counter in Zombie+0x114 (mSummonCounter) and the last helm HP in +0x12c (mBossMode).
struct WallnutCopyView {
	ZombieType type = ZOMBIE_NORMAL;
	HelmType helm = HELMTYPE_NONE;
	int fromWave = 0;
	int x = 0;              // GameObject mX (int)
	bool dead = false;
	bool hasHead = true;
	bool hypnotised = false;
};
struct WallnutCopyStep {
	int lastHelmHp = 0;   // exe +0x12c after the tick
	int quietTicks = 0;   // exe +0x114 after the tick
	bool copy = false;    // add the copy now
	bool operator==(const WallnutCopyStep&) const = default;
};
bool WallnutCopyOn();
// 0x7971eb (type 0x1b), 0x7971f5 (helm type 8), 0x797d32..0x797d6a (fromWave >= 0, mX <= 800, alive, head, not hypnotised)
bool WallnutCopyEligible(const WallnutCopyView& theZombie);
// 0x797208..0x79723f, only on eligible ticks (a skipped tick changes neither field)
WallnutCopyStep WallnutCopyTick(int theLastHelmHp, int theQuietTicks, int theHelmHp);
int WallnutCopyQuietTicks();   // 3600 (0 = never)
int WallnutCopyDamage(int theCopyHelmHp, int theOwnHelmHp);   // pushed as is; <= 0 changes nothing in TakeDamage
unsigned int WallnutCopyDamageFlags();   // 1 = bit DAMAGE_BYPASSES_SHIELD (EAX = 1 at 0x79728b)
int WallnutCopyFromWave();               // 0: IZombiePlaceZombie's AddZombieInRow(type, row, 0)
const std::vector<std::string>& WallnutZombieRuleKeys();

// ---- Almanac (0x40324f / 0x403bd0 `cmp 0x21`; hooks 9 / 10: types 26..31 use their own title / description keys) ----
int AlmanacZombieCount(int vanilla);   // 33
// The English keys this port uses for 26..31 (the exe's keys are the Chinese words 豌豆..高坚果); nullptr = vanilla name.
const char* ZomBotanyNameKey(ZombieType type);
// Index -> type: 0..24, then 26..32, the Zomboss last. Vanilla order (type = index) when count <= 26.
ZombieType AlmanacZombieAt(int index, int count);
struct AlmanacSlot {
	int x = 0, y = 0;
	float scale = 1.0f;   // window size 76 * scale
	bool operator==(const AlmanacSlot&) const = default;
};
// A 6 x 6 grid of smaller windows for 33 zombies (this port's layout: the mod's own was not read).
AlmanacSlot AlmanacZombieSlot(int index, int count);

// Every rules key the world-7 zombies read (for the pack consistency test).
const std::vector<std::string>& World7ZombieRuleKeys();

// ---- Tall-nut Zombie (Plan 9 R7; the research notes (r7-tallnut-zombie), pvz-unnamed-pack/SOURCES-tallnut-zombie.md) ----
// Keys "tallnutZombie.*" (data/rules-tallnut-zombie.json, feature "tallnutZombie"); a missing key = vanilla.
// Spirit Regeneration (hook 618 -> 0x79729c..0x797302, once per Zombie::Update, every mode): while the tall-nut helm
// (type 9) is on, every 100 updates the helm heals 10, and 10 more when its HP is not below the HP recorded after the
// previous regeneration (not hurt in that second). No cap: the helm can pass its max.
struct TallnutRegen {
	int helmHp = 0;       // after this update
	int counter = 0;      // exe Zombie+0x114 (mSummonCounter)
	int recordedHp = 0;   // exe Zombie+0x12c (ZombieState::stored): the helm HP after the last regeneration
	int healed = 0;       // HP added by this update (0 = none)
	bool operator==(const TallnutRegen&) const = default;
};
TallnutRegen TallnutZombieRegenTick(int helmType, int helmHp, int counter, int recordedHp);
// The 1-in-12 form 0x101 (0xc033f0): in SCENE_PLAYING, at zombie age (before ++) % 1200 == 600 it summons a zombie
// that rises from the ground at its own column, same row and same hypnotised state.
bool TallnutOddSummonsNow(int variant, bool scenePlaying, int zombieAge);
int TallnutOddSummonRollRange();             // Rand(8)
ZombieType TallnutOddSummonType(int roll);   // 0..4 Gargantuar, 5..7 Football
int TallnutOddRiseCounterCs();               // phase counter 50 before one rise update
int TallnutOddSummonFromWave();              // this port's non-wave fromWave (the exe passes the Board pointer)
// The form's colour (0xc03360): the mind-control colour at this alpha; -1 = vanilla colours
int TallnutOddAlpha();
const std::vector<std::string>& TallnutZombieRuleKeys();

} // namespace pvzu::unnamed
