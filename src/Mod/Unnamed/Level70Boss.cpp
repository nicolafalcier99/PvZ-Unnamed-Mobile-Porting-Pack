// PVZU: Unnamed level 70 (7-10) giant Newspaper boss. Exe addresses per key: pvz-unnamed-pack/SOURCES-level70-boss.md.
#include "Mod/Unnamed/Level70Boss.h"

#include "Mod/Unnamed/Rules.h"

#include <algorithm>
#include <cmath>

namespace pvzu::unnamed {

namespace {

constexpr const char* P = "level70Boss.";

std::string Key(const char* theName) { return std::string(P) + theName; }
int R(const char* theName, int vanilla) { return RuleInt(Key(theName), vanilla); }
float F(const char* theName, float vanilla) { return RuleFloat(Key(theName), vanilla); }
const nlohmann::json* A(const char* theName) { return RuleArray(Key(theName)); }

std::vector<int> List(const char* theName) {
	std::vector<int> aOut;
	if (const nlohmann::json* l = A(theName))
		for (const auto& v : *l)
			if (v.is_number_integer())
				aOut.push_back(v.get<int>());
	return aOut;
}

bool InList(const char* theName, int theValue) {
	const std::vector<int> l = List(theName);
	return std::find(l.begin(), l.end(), theValue) != l.end();
}

constexpr int kZombieBoss = 25;
constexpr int kSeedImitater = 48;
constexpr int kPhaseDying = 1, kPhaseBurned = 2, kPhaseMowered = 3;

} // namespace

// ---- level and trigger ----

int Boss70Level() { return R("level", 0); }
bool Boss70IsLevel(int boardLevel) { return Boss70Level() > 0 && boardLevel == Boss70Level(); }
bool Boss70Triggers(int boardLevel, int waveBeingSent) {
	return Boss70IsLevel(boardLevel) && waveBeingSent + 1 == R("triggerWave", 0);
}

// ---- the boss ----

Boss70Spawn Boss70SpawnRules() {
	Boss70Spawn s;
	s.row = R("row", 2);
	s.col = R("col", 8);
	s.scale = F("scale", 1.0f);
	s.hp = R("hp", 0);
	s.firstAttackCs = R("firstAttackCs", 0);
	return s;
}

Boss70Rect Boss70HitRect(Boss70Rect r) {
	const int wf = R("rectWidthFactor", 1), hf = R("rectHeightFactor", 1);
	const int aOldH = r.h;
	r.w *= wf;
	r.h *= hf;
	r.y -= r.h - aOldH; // 0x7a0878: h += h; y -= h_old
	return r;
}

float Boss70PinX() { return F("pinX", -1.0f); }

bool IsBossClass(int bodyMaxHealth, int zombieType) {
	const int aAbove = R("bossClassMaxHpAbove", 0);
	return (aAbove > 0 && bodyMaxHealth > aAbove) || (aAbove > 0 && zombieType == kZombieBoss);
}

bool IsBoss70(int zombieType, int variant, int boardLevel) {
	return zombieType == boss70::kNewspaper && variant == boss70::kVariant && Boss70IsLevel(boardLevel);
}

// ---- darkness and light ----

int Boss70Background(Boss70Light theLight) { return theLight == Boss70Light::DARK ? 1 : 0; }

bool Boss70PlantSleeps(Boss70Light theLight, bool nocturnal) {
	return theLight == Boss70Light::DARK ? !nocturnal : nocturnal; // sete al: IsNocturnal == edi (0 dark, 1 day)
}

int Boss70ReinforcementRollRange() { return R("reinforcementRoll", 0); }
bool Boss70ReinforcementAccepted(int roll) {
	return roll >= 0 && roll < Boss70ReinforcementRollRange() && !InList("reinforcementExclude", roll);
}
int PickBoss70Reinforcement(const std::function<int(int)>& rand) {
	const int n = Boss70ReinforcementRollRange();
	if (n <= 0)
		return 0;
	int r = 0;
	do
		r = rand(n);
	while (!Boss70ReinforcementAccepted(r));
	return r;
}
int Boss70ReinforcementRows() { return R("reinforcementRows", 5); }
int Boss70ReinforcementCol() { return R("reinforcementCol", 8); }

int Boss70StandIn(int theSeed, int theEngineSeeds) {
	if (theSeed >= 0 && theSeed < theEngineSeeds)
		return theSeed;
	const std::vector<int> aFrom = List("standInFrom"), aTo = List("standInTo");
	for (size_t i = 0; i < aFrom.size() && i < aTo.size(); ++i)
		if (aFrom[i] == theSeed && aTo[i] >= 0 && aTo[i] < theEngineSeeds)
			return aTo[i];
	return -1;
}

std::vector<int> Boss70Bank(Boss70Light theLight, int theNumPackets, int theEngineSeeds) {
	const std::vector<int> aSeeds = List(theLight == Boss70Light::DARK ? "darkBank" : "lightBank");
	const int n = std::min({theNumPackets, R("bankMax", 0), static_cast<int>(aSeeds.size())});
	std::vector<int> aOut;
	for (int i = 0; i < n; ++i)
		aOut.push_back(Boss70StandIn(aSeeds[i], theEngineSeeds));
	return aOut;
}

bool Boss70CountsAsElite(int variant, bool bossClass) { return !bossClass && variant > 0 && variant < boss70::kVariant; }
int Boss70LightRemovals(int theElites, int theNumPackets) {
	return std::max(0, std::min({theElites, theNumPackets, R("lightRemoveMax", 0)}));
}
int Boss70MadPacketSeed(int theEngineSeeds) { return Boss70StandIn(R("madPacketSeed", -1), theEngineSeeds); }

// ---- phases ----

Boss70Phase Boss70BeforeHit(int phase, int hp, int maxHp) {
	Boss70Phase p;
	if (Boss70Level() <= 0)
		return p;
	if (phase == boss70::kPhaseReading) {
		if (hp * R("shieldBelowThirds", 0) < maxHp) {
			p.giveShield = true;
			p.shieldHp = R("shieldHp", 1);
		}
	} else if (phase == boss70::kPhaseMad && hp < R("phase2Hp", 0)) {
		int aMax = maxHp;
		if (aMax > R("phase2Hp", 0)) {
			p.phase2MaxHp = R("phase2Hp", 0);
			p.phase2Sun = R("phase2Sun", -1);
			aMax = p.phase2MaxHp;
		}
		if (hp < R("phase3Hp", 0))
			p.phase3MaxHp = R("phase3Hp", 0);
	}
	return p;
}

int Boss70ProgressWidth(bool bossFound, bool bossAlive, int hp, int maxHp, int currentWave) {
	if (Boss70Level() <= 0)
		return -1;
	const int aWidth = R("meterWidth", 150);
	if (bossFound)
		return bossAlive && maxHp > 0 ? (maxHp - hp) * aWidth / maxHp : aWidth;
	return currentWave < R("triggerWave", 0) ? 0 : -1;
}

bool Boss70DrawsShieldGroup() { return Boss70Level() > 0; }
bool Boss70HandsOverPaper(int phase) { return phase != boss70::kPhaseMaddening && phase != boss70::kPhaseMad; }

// ---- attack loop ----

const char* Boss70AttackName(int theAttack) {
	static const char* const kNames[] = {"dark", "light", "laser", "sky", "rayGun", "frenzy", "rest"};
	return theAttack >= 0 && theAttack < 7 ? kNames[theAttack] : "?";
}

bool Boss70TickRuns(bool fading, bool buttered, bool iceTrapped, bool hypnotised, bool hasHead, bool dead, int phase) {
	if (fading || buttered || iceTrapped || hypnotised || !hasHead || dead)
		return false;
	return phase != kPhaseDying && phase != kPhaseBurned && phase != kPhaseMowered && phase != boss70::kPhaseMaddening;
}

int Boss70AttackRollRange(bool mad) { return mad ? R("attackRollMad", 0) : R("attackRollReading", 0); }
int Boss70TimerRollRange() { return R("timerRoll", 0); }
int Boss70NextTimer(int roll, bool mad) {
	const int t = roll + R("timerBase", 0);
	return mad ? t / 3 * 2 : t;
}
int Boss70RowRollRange() { return R("rowRoll", 5); }

int Boss70ColumnRollRange() { return static_cast<int>(List("columnTypes").size()); }
int Boss70ColumnType(int roll) {
	const std::vector<int> l = List("columnTypes");
	return roll >= 0 && roll < static_cast<int>(l.size()) ? l[roll] : 0; // ecx = 0 (Normal) for any other roll
}
float Boss70ColumnScale() { return F("columnScale", 1.0f); }
int Boss70ColumnHpFactor() { return R("columnHpFactor", 1); }
int Boss70ColumnFadeCs() { return R("columnFadeCs", -1); }
int Boss70ColumnDoorBucketHp() { return R("columnDoorBucketHp", 0); }

bool Boss70RayGunFires(int timer) {
	const int n = R("rayGunPeriod", 0);
	return n > 0 && InList("rayGunFireAt", ((timer % n) + n) % n);
}
float Boss70RayGunAngle(int timer) {
	// fldz; fiadd t; fmul 0.0314159; fsin; fmul 1.0471976
	return static_cast<float>(std::sin(static_cast<double>(timer) * F("rayGunAngleRate", 0.0f)) * F("rayGunAngleMax", 0.0f));
}
float Boss70RayGunSpeedX() { return F("rayGunSpeedX", -1.0f); }
int Boss70RayGunDx() { return R("rayGunDx", 0); }
int Boss70RayGunDy() { return R("rayGunDy", 0); }
int Boss70RayGunProjectile() { return R("rayGunProjectile", 3); }

double Boss70DrainFactor(int plants) {
	const double p = std::min(plants, R("drainPlantsCap", 50));
	return (p / 50.0 + 50.0 / p) * F("drainScale", 0.0f);
}
int Boss70DrainHp(int plantMaxHealth, double factor) {
	return static_cast<int>(std::nearbyint(static_cast<double>(plantMaxHealth) * factor)); // fistp, round to nearest
}
int Boss70DrainHeal(int total) { return std::min(total >> R("drainHealShift", 31), R("drainHealCap", 0)); }
int Boss70DrainFlashCs() { return R("drainFlashCs", 25); }

bool Boss70EatReaches(int bossRow, int bossX, int zombieRow, int zombieX, int zombieRectWidth) {
	if (std::abs(zombieRow - bossRow) > R("eatRows", -1))
		return false;
	const int aReach = R("eatReachPx", 0);
	// the exe's second test (x - x - width < reach) is always true for a positive width
	return zombieX + zombieRectWidth + aReach >= bossX && -zombieRectWidth < aReach;
}
int Boss70EatHeal(int zombieHpSum) { return std::min(zombieHpSum >> R("eatHealShift", 31), R("eatHealCap", 0)); }
int Boss70EatFreeze(int plants, int eaten) { return plants * eaten * R("eatFreezePerPlant", 0); }

std::vector<std::pair<int, int>> Boss70BungeeCells() {
	const std::vector<int> aRows = List("bungeeRows"), aCols = List("bungeeCols");
	std::vector<std::pair<int, int>> aOut;
	for (size_t i = 0; i < aRows.size() && i < aCols.size(); ++i)
		aOut.emplace_back(aRows[i], aCols[i]);
	return aOut;
}
int Boss70GraveFirstCol() { return R("graveFirstCol", 9); }
int Boss70GraveLastCol() { return R("graveLastCol", 8); }

bool Boss70SproutRandom(int seed, int imitaterOf) {
	const int aSprout = R("sproutSeed", -1);
	return aSprout >= 0 && (seed == aSprout || (seed == kSeedImitater && imitaterOf == aSprout));
}
int Boss70SproutRollRange() { return R("sproutRoll", 0); }
int Boss70SproutSeed() { return kSeedImitater; }

Boss70Butter Boss70ButterTick(int butteredCounter, int bodyMaxHealth, int zombieType, int phase) {
	Boss70Butter b;
	const int aLong = R("longButterCs", 0);
	if (aLong <= 0 || butteredCounter < aLong)
		return b;
	if (IsBossClass(bodyMaxHealth, zombieType)) {
		b.cut = true;
	} else if (zombieType == boss70::kNewspaper && phase == boss70::kPhaseMaddening) {
		b.cut = true;
		b.hypnotise = true;
	}
	return b;
}

int Boss70HeadAnimRate() { return R("headAnimRate", 15); }

std::string Boss70TextKey(Boss70Text theText) {
	switch (theText) {
	case Boss70Text::DARK_HUNT: return "PVZU_BOSS70_DARK_HUNT";
	case Boss70Text::PHASE2: return "PVZU_BOSS70_PHASE2";
	case Boss70Text::PHASE3: return "PVZU_BOSS70_PHASE3";
	case Boss70Text::LIGHT: return "PVZU_BOSS70_LIGHT";
	case Boss70Text::LASER: return "PVZU_BOSS70_LASER";
	case Boss70Text::RAY_GUN: return "PVZU_BOSS70_RAY_GUN";
	case Boss70Text::NONE: break;
	}
	return {};
}

const char* Boss70TextFallback(Boss70Text theText) {
	switch (theText) {
	case Boss70Text::DARK_HUNT: return "The Dark Hunt has begun!";
	case Boss70Text::PHASE2: return "DANGER! You woke the Second Master! DANGER!";
	case Boss70Text::PHASE3: return "The Second Master fights dirty! Young man, behave yourself!";
	case Boss70Text::LIGHT: return "White hole! A white tomorrow awaits us!";
	case Boss70Text::LASER: return "Vanish in the embrace of the dark flame!";
	case Boss70Text::RAY_GUN: return "Now fire the ray gun like crazy!";
	case Boss70Text::NONE: break;
	}
	return "";
}

int Boss70TextCs() { return R("textCs", 250); }

const std::vector<std::string>& Boss70RuleKeys() {
	static const std::vector<std::string> kKeys = [] {
		std::vector<std::string> k;
		for (const char* n :
			{"level", "triggerWave", "row", "col", "scale", "hp", "firstAttackCs", "rectWidthFactor", "rectHeightFactor", "pinX",
				"bossClassMaxHpAbove", "reinforcementRoll", "reinforcementExclude", "reinforcementRows", "reinforcementCol",
				"bankMax", "darkBank", "lightBank", "standInFrom", "standInTo", "lightRemoveMax", "shieldBelowThirds", "shieldHp",
				"phase2Hp", "phase2Sun", "phase3Hp", "meterWidth", "attackRollReading", "attackRollMad", "timerRoll", "timerBase",
				"rowRoll", "columnTypes", "columnScale", "columnHpFactor", "columnFadeCs", "columnDoorBucketHp", "rayGunPeriod",
				"rayGunFireAt", "rayGunAngleRate", "rayGunAngleMax", "rayGunSpeedX", "rayGunDx", "rayGunDy", "rayGunProjectile",
				"drainPlantsCap", "drainScale", "drainHealShift", "drainHealCap", "drainFlashCs", "eatRows", "eatReachPx",
				"eatHealShift", "eatHealCap", "eatFreezePerPlant", "madPacketSeed", "bungeeRows", "bungeeCols", "graveFirstCol",
				"graveLastCol", "sproutSeed", "sproutRoll", "longButterCs", "headAnimRate", "textCs"})
			k.push_back(Key(n));
		return k;
	}();
	return kKeys;
}

} // namespace pvzu::unnamed
