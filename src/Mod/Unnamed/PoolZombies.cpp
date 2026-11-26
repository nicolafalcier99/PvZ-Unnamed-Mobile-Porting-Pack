// PVZU: Unnamed Pool zombies and elite forms. Section numbers refer to the research notes (w3-pool); exe addresses are
// listed per key in pvz-unnamed-pack/SOURCES-pool-zombies.md.
#include "Mod/Unnamed/PoolZombies.h"

#include "Mod/Unnamed/Rules.h"

#include <cmath>
#include <cstdlib>

namespace pvzu::unnamed {

namespace {

int R(const char* key, int vanilla) { return RuleInt(key, vanilla); }
float RF(const char* key, float vanilla) { return RuleFloat(key, vanilla); }
bool On(const char* key) { return R(key, 0) != 0; }

} // namespace

bool IsPoolType(ZombieType t) { return t == ZOMBIE_SNORKEL || t == ZOMBIE_DOLPHIN_RIDER || t == ZOMBIE_ZAMBONI; }

const char* PoolEliteName(ZombieType type, int variant, int extra) {
	switch (type) {
	case ZOMBIE_SNORKEL: return variant == poolelite::LOTUS ? "lotus" : "none";
	case ZOMBIE_DOLPHIN_RIDER: return variant == poolelite::FOOTBALL_HEAD ? "football-head" : "none";
	case ZOMBIE_ZAMBONI: return extra == zamboni::ORANGE ? "orange" : extra == zamboni::RED ? "red" : "none";
	default: return "none";
	}
}

// ---- spawn rolls ----

int PoolRollRange(ZombieType type, int eliteOption) {
	if (eliteOption == eliteoption::NONE)
		return 0;
	switch (type) {
	case ZOMBIE_SNORKEL: return R("poolZombies.snorkel.eliteRoll", 0);        // Z1, hook 499
	case ZOMBIE_DOLPHIN_RIDER: return R("poolZombies.dolphin.eliteRoll", 0);  // Z2, hook 494
	case ZOMBIE_ZAMBONI: // Z3c, hook 496; option 2 draws nothing (0x95a414)
		return eliteOption == eliteoption::ALL ? 0 : R("poolZombies.zamboni.eliteRoll", 0);
	default: return 0;
	}
}

int RollSnorkel(int roll, int eliteOption) {
	return roll == R("poolZombies.snorkel.eliteHit", -1) || eliteOption == eliteoption::ALL ? poolelite::LOTUS : 0;
}
int RollDolphin(int roll, int eliteOption) {
	return roll == R("poolZombies.dolphin.eliteHit", -1) || eliteOption == eliteoption::ALL ? poolelite::FOOTBALL_HEAD : 0;
}
bool ZamboniAlwaysOrange(int eliteOption) {
	return eliteOption == eliteoption::ALL && R("poolZombies.zamboni.eliteRoll", 0) > 0;
}
int RollZamboni(int roll) { return roll == R("poolZombies.zamboni.eliteHit", -1) ? zamboni::ORANGE : zamboni::NORMAL; }

bool ZamboniRollAllowed(int gameMode, bool isAdventure, int boardLevel) { // 0x95A400
	return gameMode != 0x89 && !(isAdventure && boardLevel == 65);
}

int PoolBodyHp(ZombieType type, int vanilla) {
	return type == ZOMBIE_DOLPHIN_RIDER ? R("poolZombies.dolphin.hp", vanilla) : vanilla; // 0x522D5E
}

// ---- movement and eating ----

float DolphinJumpVelX(int variant, float vanilla) { // hook 559, 0x796A10
	return variant == poolelite::FOOTBALL_HEAD ? RF("poolZombies.dolphin.eliteJumpVx", vanilla) : vanilla;
}

bool PoolEatsPlants(ZombieType type, int variant) { // hook 675 chain 0x795F8B
	return !(type == ZOMBIE_SNORKEL && variant == poolelite::LOTUS && On("poolZombies.snorkel.lotusIgnoresPlants"));
}

// ---- Zomboni freeze (Z3a, hook 620) ----

bool ZamboniFreezesPlants(int state, bool mindControlled, int zombieHeight) {
	return On("poolZombies.zamboni.freezePlants") && !mindControlled && zombieHeight != HEIGHT_GETTING_BUNGEE_DROPPED &&
		state != zamboni::ORANGE;
}

bool InZamboniFreezeBox(int zRow, int zX, int pRow, int pX) { // 0x794320
	const int rows = R("poolZombies.zamboni.freezeRows", 0), half = R("poolZombies.zamboni.freezeHalfWidthPx", 0);
	return std::abs(pRow - zRow) <= rows && pX >= zX - half && pX <= zX + half;
}

bool ZamboniFreezeImmune(int seed, bool squareHasModPumpkin) { // 0x794320 list
	if (squareHasModPumpkin)
		return true;
	switch (seed) {
	case 0x7A: case 0x56: case 0x55:                    // the mod's own plant codes (vanilla seed + 80)
	case SEED_SNOWPEA: case SEED_ICESHROOM: case SEED_WINTERMELON: case SEED_TORCHWOOD: case SEED_JALAPENO:
	case SEED_SPIKEWEED: case SEED_TANGLEKELP: case SEED_SPLITPEA: case SEED_SPIKEROCK:
		return true;
	default: return false;
	}
}

int ZamboniFreezeTintRed() { return R("poolZombies.zamboni.freezeTintRed", 255); }
int ZamboniFreezeTintGreen() { return R("poolZombies.zamboni.freezeTintGreen", 255); }

// ---- Zomboni hidden elite (Z3c) ----

float ZamboniVelX(int state, int bodyHp, float vanilla) { // hook 598, 0x95A600
	if (state == zamboni::ORANGE) return RF("poolZombies.zamboni.eliteVx", vanilla);
	if (state == zamboni::RED || bodyHp == R("poolZombies.zamboni.redVxAtBodyHp", -1)) return RF("poolZombies.zamboni.redVx", vanilla);
	return vanilla;
}

ZamboniHit ZamboniEliteHit(int state, int damage, int bodyHp) { // 0xC019ED -> 0x95A476
	if (state != zamboni::ORANGE)
		return {damage, state};
	const int flat = R("poolZombies.zamboni.eliteArmorFlat", 0);
	const double mul = RF("poolZombies.zamboni.eliteArmorMul", 1.0f);
	const int d = damage <= flat ? 0 : static_cast<int>(std::lround((damage - flat) * mul)); // FPU round-to-nearest
	if (d + R("poolZombies.zamboni.eliteBreakHp", 0) >= bodyHp) // 0x95A499: the hit is dropped, the armour breaks
		return {0, zamboni::RED};
	return {d, zamboni::ORANGE};
}

int ZamboniStateAfterTick(int state, int bodyHp) { // hook 613, 0x95A440
	return state == zamboni::ORANGE && bodyHp <= R("poolZombies.zamboni.eliteRedHp", -1) ? zamboni::RED : state;
}

std::optional<Rgb> ZamboniTint(int state) { // hook 647, 0x95A300
	if (state == zamboni::ORANGE && R("poolZombies.zamboni.tintEliteR", -1) >= 0)
		return Rgb{R("poolZombies.zamboni.tintEliteR", 255), R("poolZombies.zamboni.tintEliteG", 255), R("poolZombies.zamboni.tintEliteB", 255)};
	if (state == zamboni::RED && R("poolZombies.zamboni.tintRedR", -1) >= 0)
		return Rgb{R("poolZombies.zamboni.tintRedR", 255), R("poolZombies.zamboni.tintRedG", 255), R("poolZombies.zamboni.tintRedB", 255)};
	return std::nullopt;
}

int ZamboniIceMaxX(int vanilla) { return R("poolZombies.zamboni.iceMaxX", vanilla); }

// ---- Ice-shroom freezes the Zomboni (Z3b) ----

bool PoolFreezeGateOn() { return On("poolZombies.zamboni.iceShroomFreezes"); }

bool PoolFreezeGate(int variant, bool mindControlled, bool hasHead, int bodyHp) { // hook 712, 0x7CD050
	return variant != 0x100 && !mindControlled && hasHead && (bodyHp & 0xFF) != 0;
}

bool BossRefusesFreeze(ZombieType type, int bodyHp) { // hook 713, 0xC10AE0
	return type == ZOMBIE_BOSS && bodyHp == 1;
}

// ---- art ----

PoolArt PoolArtFor(ZombieType type, int variant) {
	PoolArt a;
	if (type == ZOMBIE_SNORKEL && variant == poolelite::LOTUS) { // helper 0x794800 with ebp = 6 (REANIM_LILYPAD)
		a.images.push_back({"anim_head1", "IMAGE_BLANK"});
		a.hidePrefixes = {"anim_hair", "anim_head2"};
		a.lilyPadHead = true;
	} else if (type == ZOMBIE_DOLPHIN_RIDER && variant == poolelite::FOOTBALL_HEAD) { // 0x79699B
		a.images.push_back({"anim_head1", "particles/ZombieFootballHead"});
		a.showTracks.push_back("anim_head2");
	}
	return a;
}

PoolArt ResolvePoolArt(ZombieType type, int variant, const std::function<bool(const std::string&)>& available) {
	PoolArt a = PoolArtFor(type, variant);
	for (const auto& img : a.images)
		if (img.file != "IMAGE_BLANK" && !available(img.file))
			return {};
	return a;
}

const std::vector<std::string>& PoolZombieRuleKeys() {
	static const std::vector<std::string> keys = {
		"poolZombies.snorkel.eliteRoll", "poolZombies.snorkel.eliteHit", "poolZombies.snorkel.lotusIgnoresPlants",
		"poolZombies.dolphin.hp", "poolZombies.dolphin.eliteRoll", "poolZombies.dolphin.eliteHit",
		"poolZombies.dolphin.eliteJumpVx",
		"poolZombies.zamboni.freezePlants", "poolZombies.zamboni.freezeRows", "poolZombies.zamboni.freezeHalfWidthPx",
		"poolZombies.zamboni.freezeTintRed", "poolZombies.zamboni.freezeTintGreen", "poolZombies.zamboni.iceShroomFreezes",
		"poolZombies.zamboni.iceMaxX", "poolZombies.zamboni.eliteRoll", "poolZombies.zamboni.eliteHit",
		"poolZombies.zamboni.eliteVx", "poolZombies.zamboni.redVx", "poolZombies.zamboni.redVxAtBodyHp",
		"poolZombies.zamboni.eliteArmorFlat", "poolZombies.zamboni.eliteArmorMul", "poolZombies.zamboni.eliteBreakHp",
		"poolZombies.zamboni.eliteRedHp", "poolZombies.zamboni.tintEliteR", "poolZombies.zamboni.tintEliteG",
		"poolZombies.zamboni.tintEliteB", "poolZombies.zamboni.tintRedR", "poolZombies.zamboni.tintRedG",
		"poolZombies.zamboni.tintRedB",
	};
	return keys;
}

} // namespace pvzu::unnamed
