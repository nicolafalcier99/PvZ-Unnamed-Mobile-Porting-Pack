// PVZU: Unnamed Fog zombies and elite forms. Section numbers refer to the research notes (w4-fog); exe addresses are
// listed per key in pvz-unnamed-pack/SOURCES-fog-zombies.md.
#include "Mod/Unnamed/FogZombies.h"

#include "Mod/Unnamed/Rules.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>

namespace pvzu::unnamed {

namespace {

int R(const char* key, int vanilla) { return RuleInt(key, vanilla); }
float RF(const char* key, float vanilla) { return RuleFloat(key, vanilla); }
bool On(const char* key) { return R(key, 0) != 0; }

constexpr int kTallnutPlus80 = 103;   // the mod's own Tall-nut+80 seed code (0x67)

} // namespace

bool IsFogType(ZombieType t) {
	return t == ZOMBIE_JACK_IN_THE_BOX || t == ZOMBIE_BALLOON || t == ZOMBIE_DIGGER || t == ZOMBIE_POGO;
}

const char* FogEliteName(ZombieType type, int variant) {
	switch (type) {
	case ZOMBIE_JACK_IN_THE_BOX:
		return variant == fogelite::ICE_JACK ? "ice-head" : variant == fogelite::POTATO_JACK ? "potato-head" : "none";
	case ZOMBIE_BALLOON: return variant == fogelite::CLOUD ? "cloud" : "none";
	case ZOMBIE_DIGGER: return variant == fogelite::SCREWDRIVER ? "screwdriver" : "none";
	case ZOMBIE_POGO: return variant == fogelite::CHOCOLATE ? "chocolate" : variant == fogelite::GARLIC ? "garlic" : "none";
	default: return "none";
	}
}

// ---- spawn rolls ----

int JackRollRange(int eliteOption) { // 3.1, hook 500 0x796a47
	if (eliteOption == eliteoption::NONE) return 0;
	return R(eliteOption == eliteoption::ALL ? "fogZombies.jack.eliteRollAllElite" : "fogZombies.jack.eliteRoll", 0);
}

int RollJack(int roll) { // `r < 2 -> 0xe + r`
	return roll >= 0 && roll < R("fogZombies.jack.eliteCount", 0) ? fogelite::ICE_JACK + roll : 0;
}

bool JackRollAllowed(bool scaryPotterLevel) { return !scaryPotterLevel; }

int BalloonRollRange(int eliteOption) { // 3.2, hook 503 0x7a6291
	return eliteOption == eliteoption::NONE ? 0 : R("fogZombies.balloon.cloudRoll", 0);
}

int RollBalloon(int roll, int eliteOption) {
	return roll == R("fogZombies.balloon.cloudHit", -1) || eliteOption == eliteoption::ALL ? fogelite::CLOUD : 0;
}

int DiggerRollRange(int eliteOption) { // 3.3, hook 490 0x797461
	return eliteOption == eliteoption::NONE ? 0 : R("fogZombies.digger.eliteRoll", 0);
}

int RollDigger(int roll, int eliteOption) {
	if (eliteOption == eliteoption::ALL) return fogelite::SCREWDRIVER;
	return roll >= 0 && roll < R("fogZombies.digger.eliteBelow", 0) ? fogelite::SCREWDRIVER : 0;
}

int DiggerSurfaceRollRange() { return R("fogZombies.digger.surfaceRoll", 0); }

int DiggerSurfaceX(int roll) { return (roll + 1) * R("fogZombies.digger.surfaceStepPx", 0); }

bool BalloonDiggerRollAllowed(bool iZombieLevel) { return !iZombieLevel; }

int PogoRollRange(int eliteOption) { // 3.4, hook 501 0x796c05
	if (eliteOption == eliteoption::NONE) return 0;
	if (eliteOption == eliteoption::ALL) return R("fogZombies.pogo.eliteRollAllElite", 0);
	return R("fogZombies.pogo.eliteRoll", 0);
}

int RollPogo(int roll) {
	if (PogoRollRange(eliteoption::DEFAULT) <= 0) return 0;
	return roll == 0 ? fogelite::CHOCOLATE : roll == 1 ? fogelite::GARLIC : 0;
}

int PogoGarlicVisibleRollRange() { return R("fogZombies.pogo.garlicInvisibleRoll", 0); }

bool PogoGarlicVisible(int roll) { return roll != 0; }   // `mVisible = (Rand(4) != 0)`

// ---- hit points ----

int FogBodyHp(ZombieType type, int variant, int vanilla) {
	if (type == ZOMBIE_JACK_IN_THE_BOX) return R("fogZombies.jack.bodyHp", vanilla);   // patch 0x522fc1
	if (type == ZOMBIE_BALLOON && variant == fogelite::CLOUD) return R("fogZombies.balloon.cloudBodyHp", vanilla);
	if (type == ZOMBIE_POGO && variant == fogelite::CHOCOLATE) return vanilla + R("fogZombies.pogo.chocolateHpDelta", 0);
	return vanilla;
}

int FogHelmHp(ZombieType type, int vanilla) {
	return type == ZOMBIE_DIGGER ? R("fogZombies.digger.helmHp", vanilla) : vanilla;   // patch 0x522be9
}

int FogFlyingHp(ZombieType type, int vanilla) {
	return type == ZOMBIE_BALLOON ? R("fogZombies.balloon.flyingHp", vanilla) : vanilla;   // patch 0x5234b9
}

// ---- Jack-in-the-Box explosion ----

JackBlast JackExplosion(int variant, bool mindControlled, bool iZombieLevel) { // hooks 560-562
	if (variant == fogelite::ICE_JACK)
		return mindControlled ? JackBlast::ICE_ZOMBIES : JackBlast::FREEZE_PLANTS;
	if (variant == fogelite::POTATO_JACK) {
		if (mindControlled) return JackBlast::PACKETS_READY;
		return iZombieLevel ? JackBlast::VANILLA : JackBlast::PACKETS_COOL;   // 0x7a6571 IsIZombieLevel
	}
	return JackBlast::VANILLA;
}

int IceFreezeCounter(int plantCount) { return R("fogZombies.jack.iceFreezeCsPerPlant", 0) * plantCount; }

int PotatoPacketCount() { return R("fogZombies.jack.potatoPackets", 0); }

bool IceFreezeStep(IceFreeze& f) { // hook 315 -> 0x796045
	if (f.active == 0)
		return false;
	if (--f.counter > 0)
		return true;
	f.active = 0;
	return false;
}

// ---- Balloon ----

std::optional<float> BalloonVelX(ZombieType type, int variant, bool walkingPhase) { // hook 521 0x797c79
	if (type != ZOMBIE_BALLOON || variant == fogelite::CLOUD || walkingPhase)
		return std::nullopt;
	const float v = RF("fogZombies.balloon.flyVx", -1.0f);
	return v > 0.0f ? std::optional<float>(v) : std::nullopt;
}

bool MowerSkips(ZombieType type, int flyingHealth) { // .text 0x458832 -> cave 0x651615
	return type == ZOMBIE_BALLOON && flyingHealth != 0 && On("fogZombies.balloon.ignoreMowers");
}

bool FlyersEat() { return On("fogZombies.balloon.eatWhileFlying"); }   // patch 0x52f63e

bool FlyingBalloonSkipsPlant(int seed) { // hook 676 0x959420
	if (!FlyersEat()) return false;
	switch (seed) {
	case SEED_WALLNUT: case SEED_POTATOMINE: case SEED_TALLNUT: case SEED_PUMPKINSHELL: case SEED_EXPLODE_O_NUT:
	case SEED_GIANT_WALLNUT: case SEED_GARLIC: case kTallnutPlus80:
		return true;
	default: return false;
	}
}

bool CloudKeepsFlying(int variant, bool flyingPhase) { // hook 551 0x7a6340
	return variant == fogelite::CLOUD && flyingPhase && On("fogZombies.balloon.cloudNeverPops");
}

// ---- Digger ----

std::optional<bool> TunnelingDiggerTargets(int seed) { // hook 654 0x795787
	if (!On("fogZombies.digger.eatsUnderground")) return std::nullopt;
	switch (seed) {
	case SEED_POTATOMINE: return std::nullopt;   // the vanilla test (unarmed only)
	case SEED_LILYPAD: case SEED_FLOWERPOT: case SEED_WALLNUT: case SEED_TALLNUT: case SEED_PUMPKINSHELL:
	case SEED_CHOMPER: case SEED_TORCHWOOD: case SEED_CACTUS: case SEED_SPIKEWEED: case SEED_SPIKEROCK:
		return true;
	default: return false;
	}
}

float DiggerSurfaceBelowX(int surfaceX, float vanilla) { // hook 585 0x7974d7
	const float base = RF("fogZombies.digger.surfaceBasePx", -1.0f);
	return base < 0.0f ? vanilla : static_cast<float>(surfaceX) + base;
}

bool DiggerRisesWithoutAxe(int variant) { // hook 586
	return variant == fogelite::SCREWDRIVER && DiggerRollRange() > 0;
}

// ---- Pogo ----

PogoNext PogoBounceEnd(int variant, std::optional<int> targetDx, bool inHighBounce1) { // patch 0x5256eb, hook 533
	const float aBounceTime = 80.0f; // POGO_BOUNCE_TIME
	const bool noWindup = On("fogZombies.pogo.noWindup");
	if (noWindup && variant == fogelite::CHOCOLATE) // 0x796cef / 0x959a06
		return {PogoMove::FORWARD, (R("fogZombies.pogo.chocolateBounceDx", 0) + 60) / aBounceTime};
	if (!targetDx)
		return {PogoMove::BOUNCE, 0.0f};
	if (noWindup)
		return {PogoMove::FORWARD, (std::abs(*targetDx) + 60) / aBounceTime};
	if (inHighBounce1)
		return {PogoMove::FORWARD, (*targetDx + 60) / aBounceTime};
	return {PogoMove::HIGH_BOUNCE, 0.0f};
}

bool PogoTargetsZombies() { return On("fogZombies.pogo.noWindup"); }   // FindZombieTarget 0x52e840 in the same block

bool PogoTurnsYucky(int variant) { return variant == fogelite::GARLIC && R("fogZombies.pogo.yuckyCounter", 0) > 0; }

int PogoYuckyCounter() { return R("fogZombies.pogo.yuckyCounter", 0); }

bool PogoBonksOn(int seed) { // hook 532 0x959b60
	if (seed == SEED_TALLNUT) return true;
	return (seed == SEED_GIANT_WALLNUT || seed == kTallnutPlus80) && On("fogZombies.pogo.bonkGiantNuts");
}

bool PogoLandsOnZombies() { return On("fogZombies.pogo.landOnZombies"); }   // hook 531 0x959840

int PogoLandRectGrowPx() { return R("fogZombies.pogo.landRectGrowPx", 0); }

int PogoLandDamage(int variant, bool firstZombie) {
	const int d = firstZombie ? R("fogZombies.pogo.landFirstDamage", 0) : R("fogZombies.pogo.landOtherDamage", 0);
	if (variant != fogelite::CHOCOLATE) return d;
	return static_cast<int>(d * RF("fogZombies.pogo.chocolateDamageMul", 1.0f)); // 3000 / 333 (truncated)
}

// ---- art ----

FogArt FogArtFor(ZombieType type, int variant) {
	FogArt a;
	if (type == ZOMBIE_JACK_IN_THE_BOX && (variant == fogelite::ICE_JACK || variant == fogelite::POTATO_JACK)) {
		a.images.push_back({"anim_head1", "IMAGE_BLANK"}); // helper 0x794800 (EBP = REANIM_ICESHROOM 28 / POTATOMINE 23)
		a.hidePrefixes = {"anim_hair", "anim_head2"};
		a.headReanim = variant == fogelite::ICE_JACK ? ReanimationType::REANIM_ICESHROOM : ReanimationType::REANIM_POTATOMINE;
		a.headAnim = "anim_head_idle"; // 0x79488f: push 0x668c34 (0x800120 is 0 on this path)
	} else if (type == ZOMBIE_BALLOON && variant == fogelite::CLOUD) {
		a.images.push_back({"Zombie_balloon_top", "particles/Zombie_boss_iceball_particles"});
	} else if (type == ZOMBIE_DIGGER && variant == fogelite::SCREWDRIVER) {
		a.images.push_back({"Zombie_digger_pickaxe", "particles/Zomboss_particles"});
	} else if (type == ZOMBIE_POGO && variant == fogelite::CHOCOLATE) {
		a.images.push_back({"anim_head1", "images/Chocolate"});
		a.hidePrefixes = {"anim_head2"};
	} else if (type == ZOMBIE_POGO && variant == fogelite::GARLIC) {
		a.images.push_back({"anim_head1", "reanim/Garlic_body3"});
		a.hidePrefixes = {"anim_head2"};
	}
	return a;
}

FogArt ResolveFogArt(ZombieType type, int variant, const std::function<bool(const std::string&)>& available) {
	FogArt a = FogArtFor(type, variant);
	for (const auto& img : a.images)
		if (img.file != "IMAGE_BLANK" && !available(img.file))
			return {};
	return a;
}

std::string FogHeadAnimTrack(const std::string& wanted, const std::vector<std::string>& trackNames) {
	auto lower = [](std::string s) {
		std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		return s;
	};
	const std::string w = lower(wanted);
	for (const auto& t : trackNames)
		if (lower(t) == w) // FindTrackIndex compares with stricmp (0x61cccc)
			return wanted;
	return trackNames.empty() ? wanted : trackNames.front(); // a miss returns index 0 (0x472f64 `xor eax, eax`)
}

const std::vector<std::string>& FogZombieRuleKeys() {
	static const std::vector<std::string> keys = {
		"fogZombies.jack.bodyHp", "fogZombies.jack.eliteRoll", "fogZombies.jack.eliteCount",
		"fogZombies.jack.iceFreezeCsPerPlant", "fogZombies.jack.potatoPackets",
		"fogZombies.balloon.flyingHp", "fogZombies.balloon.flyVx", "fogZombies.balloon.ignoreMowers",
		"fogZombies.balloon.eatWhileFlying", "fogZombies.balloon.cloudRoll", "fogZombies.balloon.cloudHit",
		"fogZombies.balloon.cloudBodyHp", "fogZombies.balloon.cloudNeverPops",
		"fogZombies.digger.helmHp", "fogZombies.digger.eatsUnderground", "fogZombies.digger.eliteRoll",
		"fogZombies.digger.eliteBelow", "fogZombies.digger.surfaceRoll", "fogZombies.digger.surfaceStepPx",
		"fogZombies.digger.surfaceBasePx",
		"fogZombies.jack.eliteRollAllElite", "fogZombies.pogo.eliteRoll", "fogZombies.pogo.eliteRollAllElite", "fogZombies.pogo.chocolateHpDelta",
		"fogZombies.pogo.garlicInvisibleRoll", "fogZombies.pogo.noWindup", "fogZombies.pogo.chocolateBounceDx",
		"fogZombies.pogo.yuckyCounter", "fogZombies.pogo.landOnZombies", "fogZombies.pogo.landFirstDamage",
		"fogZombies.pogo.landOtherDamage", "fogZombies.pogo.chocolateDamageMul", "fogZombies.pogo.landRectGrowPx",
		"fogZombies.pogo.bonkGiantNuts",
	};
	return keys;
}

} // namespace pvzu::unnamed
