// PVZU: Unnamed world-7 zombie rules and the ZomBotany zombies in Unnamed. Section numbers refer to
// the research notes (w6-w7-and-new-content); exe addresses are listed per key in pvz-unnamed-pack/SOURCES-world7-zombies.md.
#include "Mod/Unnamed/World7Zombies.h"

#include "Mod/Unnamed/Rules.h"

#include <algorithm>
#include <cmath>

namespace pvzu::unnamed {

namespace {

int R(const char* key, int vanilla) { return RuleInt(key, vanilla); }
float RF(const char* key, float vanilla) { return RuleFloat(key, vanilla); }
bool On(const char* key) { return R(key, 0) != 0; }

constexpr int kExeBasketball = 9;   // PROJECTILE_BASKETBALL
constexpr int kPoleHead = 0x19;     // RoofZombies roofelite::POLE_HEAD

} // namespace

// ---- gates ----

bool IsWorld7(bool isAdventure, int playerLevel, int /*boardLevelIgnored*/) { // 0x7aa4c0
	const int first = R("world7Zombies.world7.firstLevel", -1);
	if (!isAdventure || first < 0)
		return false;
	return playerLevel >= first && playerLevel <= R("world7Zombies.world7.lastLevel", -1);
}

bool PlayerLevelAbove60(int playerLevel, int /*boardLevelIgnored*/) {
	const int above = R("world7Zombies.above60.level", -1);
	return above >= 0 && playerLevel > above;
}

int RoadSignWorldNumber(bool world7) { return world7 && R("world7Zombies.world7.firstLevel", -1) >= 0 ? 7 : 0; }

const char* World7FormName(ZombieType type, int variant, int subVariant) {
	if (type == ZOMBIE_GARGANTUAR && variant == w7::IMP_GARGANTUAR) return "imp-gargantuar";
	if (type == ZOMBIE_CATAPULT && variant == w7::VASE_CATAPULT) return "vase";
	if (type == ZOMBIE_FOOTBALL && subVariant == w7::BLACK_FOOTBALL) return variant != 0 ? "black+gold" : "black";
	if (type == ZOMBIE_TALLNUT_HEAD && variant == w7::TALLNUT_ODD) return "odd-0x101";
	return nullptr;
}

// ---- Flag (hook 91) ----

bool FlagFillsRows() { return On("world7Zombies.flag.fillRows"); }

std::vector<int> FlagExtraRows(int flagRow, bool sixRowLawn) {
	std::vector<int> rows;
	if (!FlagFillsRows())
		return rows;
	const int top = sixRowLawn ? R("world7Zombies.flag.topRowSixRows", 4) : R("world7Zombies.flag.topRow", 4);
	for (int r = top; r >= 0; --r) // 0x7a1d68..0x7a1dce: ESI = top .. 0, skips the flag's own row (0x7a1d69)
		if (r != flagRow)
			rows.push_back(r);
	return rows;
}

int FlagBucketHp() { return R("world7Zombies.flag.bucketHp", 0); }

// ---- Imp (hook 571) ----

int ImpGargRollRange(int setting) { return setting == eliteoption::NONE ? 0 : R("world7Zombies.imp.gargRoll", 0); }
bool ImpBecomesGarg(int roll, int setting) {
	return ImpGargRollRange(setting) > 0 && (roll == R("world7Zombies.imp.gargHit", -1) || setting == eliteoption::ALL);
}
int ImpGargHpBonus() { return R("world7Zombies.imp.gargHpBonus", 0); }

// ---- Football black (hook 489) ----

bool FootballBecomesBlack(int extraRoll, bool green, bool gold, int playerLevel) { // 0x9dd406..0x9dd419
	if (green || gold || extraRoll < 0)
		return false;
	return PlayerLevelAbove60(playerLevel) && extraRoll == R("world7Zombies.football.blackExtraHit", -1);
}

int BlackFootballBodyHp(int vanilla) { return R("world7Zombies.football.blackBodyHp", vanilla); }   // 0x9dd460
int BlackFootballHelmHp(int vanilla) { return R("world7Zombies.football.blackHelmHp", vanilla); }
int GoldBlackFootballBodyHp(int vanilla) { return R("world7Zombies.football.goldBlackBodyHp", vanilla); } // 0x9dd433
int GoldBlackFootballHelmHp(int vanilla) { return R("world7Zombies.football.goldBlackHelmHp", vanilla); }

std::vector<W7ArtImage> BlackFootballArt() { // 0xc11df0: the green parts' tracks, IMAGE_B1. .. IMAGE_RL
	std::vector<W7ArtImage> a;
	for (const char* part : {"helmet", "leftarm_hand", "leftleg_lower", "rightleg_lower", "rightarm_hand", "upperbody",
	                         "upperbody2", "upperbody3"})
		a.push_back({std::string("zombie_football_") + part, std::string("images/Black_football_") + part});
	return a;
}

std::string BlackHelmImage(int damageIndex) { // hooks 700 / 701 -> 0xc126c0 / 0xc12700 (0x19 branch)
	if (damageIndex == 1) return "images/Black_football_helmet2";
	if (damageIndex == 2) return "images/Black_football_helmet3";
	return "";
}

// ---- Catapult (hooks 498, 537) ----

int CatapultWorld7Form(int roll, int playerLevel, int setting) { // 0x910000
	if (setting == eliteoption::NONE)
		return 0;
	const bool all = setting == eliteoption::ALL;
	const int from = R(all ? "world7Zombies.catapult.vaseRollFromAllElite" : "world7Zombies.catapult.vaseRollFrom", -1);
	if (from < 0 || roll < from ||
		roll >= R(all ? "world7Zombies.catapult.vaseRollBelowAllElite" : "world7Zombies.catapult.vaseRollBelow", -1))
		return 0;
	return PlayerLevelAbove60(playerLevel) ? w7::VASE_CATAPULT : 0;
}

int VaseCatapultBalls(int vanilla) { return R("world7Zombies.catapult.vaseBalls", vanilla); }   // +0x114 = 0xa

std::vector<W7ArtImage> VaseCatapultArt() { // 0x910050..0x910091: tracks 0x66ed08.., IMAGE_BUG_SPRAY (0x6a744c)
	return {{"Zombie_catapult_basketball", "images/bug_spray"}, {"Zombie_catapult_basketball2", "images/bug_spray"},
	        {"Zombie_catapult_basketball3", "images/bug_spray"}, {"Zombie_catapult_basketball4", "images/bug_spray"}};
}

int CatapultShotRollRange(int variant, bool world7OrBossLevel) { // 0x7a1974..0x7a19c0
	if (variant == w7::VASE_CATAPULT || (variant == kPoleHead && world7OrBossLevel))
		return R("world7Zombies.catapult.shotRoll", 0);
	return 0;
}

CatapultShot RollCatapultShot(int variant, int roll, int balls) {
	CatapultShot s{kExeBasketball, balls};
	if (variant == w7::VASE_CATAPULT && roll < R("world7Zombies.catapult.vaseShotBelow", 0)) {
		s.exeProjectile = 0x20;
	} else if (variant == kPoleHead && roll < R("world7Zombies.catapult.poleShotBelow", 0)) {
		s.exeProjectile = 0x12;
		s.balls = std::min(balls, R("world7Zombies.catapult.poleShotBallCap", balls));   // `cmp +0x114, 5; jl`
	}
	return s;
}

// ---- Jack (hook 707) ----

bool JackPopsWhenHit(bool world7, ZombieType type, int variant, int bodyHp) { // `cmp [esi+0xc8], 0x172; ja`
	const int maxHp = R("world7Zombies.jack.popMaxBodyHp", -1);
	return maxHp >= 0 && world7 && type == ZOMBIE_JACK_IN_THE_BOX && variant != 0 && bodyHp <= maxHp;
}

bool CoinFootballIsGold() { return On("world7Zombies.coin.footballGold"); }

// ---- Tree of Wisdom (hook 268) ----

int TreeBuffedPlantHp(int hp, bool isAdventure, int treePurchase, int finishedAdventures, int playerLevel, int treeHeight) {
	const int minH = R("world7Zombies.tree.minHeight", -1);
	if (minH < 0 || !isAdventure || treePurchase != 1)
		return hp;
	if (finishedAdventures < 1 && !PlayerLevelAbove60(playerLevel)) // `cmp [+0x2c], 1; jge` / `cmp [+0x24], 0x3d; jl`
		return hp;
	if (treeHeight <= minH)
		return hp;
	const int extra = std::min(treeHeight - minH, R("world7Zombies.tree.maxExtraFeet", 0));
	// fild / fdiv 100.0f / fld1 faddp / fimul / fistp, round to nearest even. double (= the exe's x87 at MSVC's
	// default 53-bit precision): the same result on every CPU (80-bit x87 long double gave 31 for 30 HP at 105 ft)
	const double f = 1.0 + static_cast<double>(extra) / 100.0;
	return static_cast<int>(std::nearbyint(f * static_cast<double>(hp)));
}

// ---- ZomBotany ----

bool IsZomBotanyType(ZombieType t) { return t >= ZOMBIE_PEA_HEAD && t <= ZOMBIE_TALLNUT_HEAD; }

ZomBotanyArmour ZomBotanyStats(ZombieType type, int vanillaBodyHp, HelmType vanillaHelm, int vanillaHelmHp) {
	ZomBotanyArmour a{vanillaBodyHp, vanillaHelm, vanillaHelmHp, nullptr, 0};
	auto helm = [&](const char* key, HelmType t, const char* prefix) {
		if (const int hp = R(key, -1); hp >= 0) {
			a.helm = t;
			a.helmHp = hp;
			a.helmPrefix = prefix;
		}
	};
	switch (type) {
	case ZOMBIE_PEA_HEAD: helm("world7Zombies.zombotany.peaHeadConeHp", HELMTYPE_TRAFFIC_CONE, "anim_cone"); break;   // 507
	case ZOMBIE_WALLNUT_HEAD: a.helmHp = R("world7Zombies.zombotany.wallnutHelmHp", vanillaHelmHp); break;           // 0x523825
	case ZOMBIE_TALLNUT_HEAD: a.helmHp = R("world7Zombies.zombotany.tallnutHelmHp", vanillaHelmHp); break;           // 0x523957
	case ZOMBIE_JALAPENO_HEAD:                                                                                       // 0x523a81, 509
		a.bodyHp = R("world7Zombies.zombotany.jalapenoBodyHp", vanillaBodyHp);
		helm("world7Zombies.zombotany.jalapenoConeHp", HELMTYPE_TRAFFIC_CONE, "anim_cone");
		a.doorHp = std::max(0, R("world7Zombies.zombotany.jalapenoDoorHp", 0));
		break;
	case ZOMBIE_GATLING_HEAD: helm("world7Zombies.zombotany.gatlingBucketHp", HELMTYPE_PAIL, "anim_bucket"); break;    // 510
	case ZOMBIE_SQUASH_HEAD:                                                                                         // 511
		helm("world7Zombies.zombotany.squashBucketHp", HELMTYPE_PAIL, "anim_bucket");
		a.doorHp = std::max(0, R("world7Zombies.zombotany.squashDoorHp", 0));
		break;
	default: break;
	}
	return a;
}

int TallnutOddRollRange() { return R("world7Zombies.zombotany.tallnutOddRoll", 0); }   // 0xc033a0: Rand(0xc)
int RollTallnutOdd(int roll) { return TallnutOddRollRange() > 0 && roll == 0 ? w7::TALLNUT_ODD : 0; }

// ---- Peashooter Zombie fire-rate ramp (Plan 9 R5, 0x7958fe) ----
int PeaHeadRampRollRange() { return std::max(0, R("peaHeadRamp.roll", 0)); }   // 0x795909: mov eax, 0x4b
int PeaHeadRampAfterShot(int raises, int roll) {
	// 0x795913: cmp roll, [edi+0x12c]; jl skip; inc [edi+0x12c] (roll is below the range, so n stops at the range)
	return PeaHeadRampRollRange() > 0 && roll >= raises ? raises + 1 : raises;
}
int PeaHeadShotInterval(int raises, int vanilla) {
	const int range = PeaHeadRampRollRange();
	if (range <= 0)
		return vanilla;
	return R("peaHeadRamp.baseInterval", vanilla) - std::clamp(raises, 0, range);   // 0x795925: 150 - n
}
const std::vector<std::string>& PeaHeadRampRuleKeys() {
	static const std::vector<std::string> keys = {"peaHeadRamp.roll", "peaHeadRamp.baseInterval"};
	return keys;
}

int GatlingSpreadRollRange() { return R("world7Zombies.gatling.spreadRoll", 0); }
bool GatlingSpreads(int roll) { return GatlingSpreadRollRange() > 0 && roll < R("world7Zombies.gatling.spreadBelow", 0); }
float GatlingSpreadVx() { return RF("world7Zombies.gatling.spreadVx", 0.0f); }   // [0x69adf0]
float GatlingSpreadVy() { return RF("world7Zombies.gatling.spreadVy", 0.0f); }   // [0x69af44]
bool HypnotisedHeadsFirePlayerPeas() { return On("world7Zombies.heads.hypnotisedFirePlayerPeas"); }

bool JalapenoBurnsHypnotised() { return On("world7Zombies.jalapeno.burnsHypnotised"); }
int JalapenoIceTimer() { return R("world7Zombies.jalapeno.iceTimer", 0); }

bool SquashJumpsOnReach() { return On("world7Zombies.squash.jumpOnReach"); }
int SquashLandingZombieDamage() { return R("world7Zombies.squash.landingZombieDamage", 0); }

// ---- Wall-nut Zombie self-copy (Plan 9 R6; exe 0x7971ea..0x797297, 0x797d32) ----

bool WallnutCopyOn() { return On("wallnutZombie.copy.on"); }

int WallnutCopyQuietTicks() { return WallnutCopyOn() ? R("wallnutZombie.copy.quietTicks", 0) : 0; }

bool WallnutCopyEligible(const WallnutCopyView& z) {
	if (!WallnutCopyOn() || z.type != ZOMBIE_WALLNUT_HEAD || z.helm != HELMTYPE_WALLNUT) // 0x7971eb, 0x7971f5
		return false;
	return z.fromWave >= 0 && z.x <= R("wallnutZombie.copy.maxX", 800) && !z.dead && z.hasHead && !z.hypnotised;
}

WallnutCopyStep WallnutCopyTick(int theLastHelmHp, int theQuietTicks, int theHelmHp) {
	WallnutCopyStep s;
	s.lastHelmHp = theHelmHp;              // 0x79720e: stored every eligible tick
	if (theLastHelmHp > theHelmHp)         // `jle` not taken: the helm went down
		return s;                          // 0x79721a: counter 0
	s.quietTicks = theQuietTicks + 1;      // 0x797229
	const int quiet = WallnutCopyQuietTicks();
	if (quiet > 0 && s.quietTicks >= quiet) { // 0x79722f `cmp 0xe10; jl`
		s.quietTicks = 0;                  // 0x79723f
		s.copy = true;
	}
	return s;
}

int WallnutCopyDamage(int theCopyHelmHp, int theOwnHelmHp) { return theCopyHelmHp - theOwnHelmHp; } // 0x797284

unsigned int WallnutCopyDamageFlags() { return 1U; }

int WallnutCopyFromWave() { return 0; }

const std::vector<std::string>& WallnutZombieRuleKeys() {
	static const std::vector<std::string> keys = {
		"wallnutZombie.copy.on", "wallnutZombie.copy.quietTicks", "wallnutZombie.copy.maxX",
	};
	return keys;
}

// ---- Almanac ----

int AlmanacZombieCount(int vanilla) { return R("world7Zombies.almanac.zombieCount", vanilla); }

const char* ZomBotanyNameKey(ZombieType type) {
	if (AlmanacZombieCount(0) <= 0)
		return nullptr;
	switch (type) { // exe table 0xb232b9: 豌豆 坚果 辣椒 机枪 倭瓜 高坚果
	case ZOMBIE_PEA_HEAD: return "PEASHOOTER_ZOMBIE";
	case ZOMBIE_WALLNUT_HEAD: return "WALLNUT_ZOMBIE";
	case ZOMBIE_JALAPENO_HEAD: return "JALAPENO_ZOMBIE";
	case ZOMBIE_GATLING_HEAD: return "GATLING_PEA_ZOMBIE";
	case ZOMBIE_SQUASH_HEAD: return "SQUASH_ZOMBIE";
	case ZOMBIE_TALLNUT_HEAD: return "TALLNUT_ZOMBIE";
	default: return nullptr;
	}
}

ZombieType AlmanacZombieAt(int index, int count) {
	if (index < 0 || index >= count || index >= NUM_ZOMBIE_TYPES)
		return ZOMBIE_INVALID;
	if (count <= ZOMBIE_BOSS + 1)
		return static_cast<ZombieType>(index);
	if (index < ZOMBIE_BOSS)
		return static_cast<ZombieType>(index);
	if (index == count - 1)
		return ZOMBIE_BOSS;
	return static_cast<ZombieType>(index + 1);   // 25.. -> 26..
}

AlmanacSlot AlmanacZombieSlot(int index, int count) {
	if (count <= ZOMBIE_BOSS + 1) { // vanilla GetZombiePosition
		const ZombieType t = AlmanacZombieAt(index, count);
		if (t == ZOMBIE_BOSS)
			return {192, 486, 1.0f};
		return {t % 5 * 85 + 22, t / 5 * 80 + 86, 1.0f};
	}
	// 6 columns x 6 rows of 66 px windows, 72 px apart (x 20..446), 76 px apart (y 84..530)
	return {20 + index % 6 * 72, 84 + index / 6 * 76, 66.0f / 76.0f};
}

const std::vector<std::string>& World7ZombieRuleKeys() {
	static const std::vector<std::string> keys = {
		"world7Zombies.world7.firstLevel", "world7Zombies.world7.lastLevel", "world7Zombies.above60.level",
		"world7Zombies.football.goldBlackBodyHp", "world7Zombies.football.goldBlackHelmHp",
		"world7Zombies.catapult.vaseRollFromAllElite", "world7Zombies.catapult.vaseRollBelowAllElite",
		"world7Zombies.flag.fillRows", "world7Zombies.flag.topRow", "world7Zombies.flag.topRowSixRows",
		"world7Zombies.flag.bucketHp", "world7Zombies.imp.gargRoll", "world7Zombies.imp.gargHit",
		"world7Zombies.imp.gargHpBonus", "world7Zombies.football.blackExtraHit", "world7Zombies.football.blackBodyHp",
		"world7Zombies.football.blackHelmHp", "world7Zombies.catapult.vaseRollFrom", "world7Zombies.catapult.vaseRollBelow",
		"world7Zombies.catapult.vaseBalls", "world7Zombies.catapult.shotRoll", "world7Zombies.catapult.vaseShotBelow",
		"world7Zombies.catapult.poleShotBelow", "world7Zombies.catapult.poleShotBallCap", "world7Zombies.jack.popMaxBodyHp",
		"world7Zombies.coin.footballGold", "world7Zombies.tree.minHeight", "world7Zombies.tree.maxExtraFeet",
		"world7Zombies.zombotany.peaHeadConeHp", "world7Zombies.zombotany.wallnutHelmHp",
		"world7Zombies.zombotany.tallnutHelmHp", "world7Zombies.zombotany.tallnutOddRoll",
		"world7Zombies.zombotany.jalapenoBodyHp", "world7Zombies.zombotany.jalapenoConeHp",
		"world7Zombies.zombotany.jalapenoDoorHp", "world7Zombies.zombotany.gatlingBucketHp",
		"world7Zombies.zombotany.squashBucketHp", "world7Zombies.zombotany.squashDoorHp", "world7Zombies.gatling.spreadRoll",
		"world7Zombies.gatling.spreadBelow", "world7Zombies.gatling.spreadVx", "world7Zombies.gatling.spreadVy",
		"world7Zombies.heads.hypnotisedFirePlayerPeas", "world7Zombies.jalapeno.burnsHypnotised",
		"world7Zombies.jalapeno.iceTimer", "world7Zombies.squash.jumpOnReach", "world7Zombies.squash.landingZombieDamage",
		"world7Zombies.almanac.zombieCount",
	};
	return keys;
}

// ---- Tall-nut Zombie (Plan 9 R7) ----

TallnutRegen TallnutZombieRegenTick(int helmType, int helmHp, int counter, int recordedHp) {
	TallnutRegen r{helmHp, counter, recordedHp, 0};
	const int period = R("tallnutZombie.regen.periodCs", 0);
	if (period <= 0 || helmType != HELMTYPE_TALLNUT) // 0xc01316: cmp [esi+0xc4], 9; the counter stops too
		return r;
	if (++r.counter < period) // 0x7972b3: inc [esi+0x114]; cmp 0x64; jl
		return r;
	r.counter = 0;
	const int hp = R("tallnutZombie.regen.hp", 0);
	r.helmHp = helmHp + hp; // add [esi+0xd0], 0xa: no cap
	r.recordedHp = helmHp + hp;
	r.healed = hp;
	if (recordedHp > helmHp) // `cmp [esi+0x12c], eax; jg`: hurt since the last regeneration
		return r;
	const int extra = R("tallnutZombie.regen.undamagedExtraHp", 0);
	r.helmHp += extra;
	r.recordedHp += extra;
	r.healed += extra;
	return r;
}

bool TallnutOddSummonsNow(int variant, bool scenePlaying, int zombieAge) { // 0xc033f0
	const int period = R("tallnutZombie.odd.summonPeriodCs", 0);
	if (period <= 0 || variant != w7::TALLNUT_ODD || !scenePlaying || zombieAge < 0)
		return false;
	return zombieAge % period == R("tallnutZombie.odd.summonAtCs", -1); // `div ecx` (unsigned), `cmp edx, 0x258`
}

int TallnutOddSummonRollRange() { return R("tallnutZombie.odd.summonRoll", 0); }   // Rand(8)

ZombieType TallnutOddSummonType(int roll) { // `cmp eax, 4; jbe` -> 0x17, else 7
	if (TallnutOddSummonRollRange() <= 0 || roll < 0)
		return ZOMBIE_INVALID;
	return roll < R("tallnutZombie.odd.gargantuarBelow", 0) ? ZOMBIE_GARGANTUAR : ZOMBIE_FOOTBALL;
}

int TallnutOddRiseCounterCs() { return R("tallnutZombie.odd.riseCounterCs", 0); }   // +0x68 = 0x32

// Ruling: the exe passes the Board pointer (EBX of Board::UpdateGameObjects) as fromWave, a positive number that never
// matches a wave; 1000 does the same here (above any wave count, so never counted for the wave-health trigger).
int TallnutOddSummonFromWave() { return 1000; }

int TallnutOddAlpha() { return R("tallnutZombie.odd.alpha", -1); }   // 0xc0337a: mov edi, 0xb0

const std::vector<std::string>& TallnutZombieRuleKeys() {
	static const std::vector<std::string> keys = {
		"tallnutZombie.regen.periodCs", "tallnutZombie.regen.hp", "tallnutZombie.regen.undamagedExtraHp",
		"tallnutZombie.odd.summonPeriodCs", "tallnutZombie.odd.summonAtCs", "tallnutZombie.odd.summonRoll",
		"tallnutZombie.odd.gargantuarBelow", "tallnutZombie.odd.riseCounterCs", "tallnutZombie.odd.alpha",
	};
	return keys;
}

} // namespace pvzu::unnamed
