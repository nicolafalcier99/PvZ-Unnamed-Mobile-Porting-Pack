// PVZU: Unnamed Day zombies and elite forms. Section numbers refer to the research notes (zombies).
#include "Mod/Unnamed/DayZombies.h"

#include "Mod/Unnamed/Rules.h"

#include <cctype>
#include <cstdlib>
#include <cmath>

namespace pvzu::unnamed {

namespace {

// Every key is read through these two, so DayZombieRuleKeys() can list them.
int R(const char* key, int vanilla) { return RuleInt(key, vanilla); }
float RF(const char* key, float vanilla) { return RuleFloat(key, vanilla); }

bool IsNormalBody(ZombieType t) { return t == ZOMBIE_NORMAL || t == ZOMBIE_DUCKY_TUBE; }

} // namespace

int StateVariantFromExe(int exeCode) { return exeCode == -1 ? elite::NONE : exeCode; }

const char* EliteName(int variant) {
	switch (variant) {
	case elite::NONE: return "none";
	case elite::STICKY_NOTE: return "sticky-note";
	case elite::PHONOGRAPH: return "phonograph";
	case elite::DANCER_HEAD: return "dancer-head";
	case elite::PUMPKIN: return "pumpkin";
	case elite::UMBRELLA: return "umbrella-leaf";
	case elite::ROAD_SIGN: return "road-sign";
	case elite::GLOVE: return "glove";
	case elite::VASE_POLE: return "vase-pole";
	case elite::SILVER_COIN: return "silver-coin";
	default: return "unknown";
	}
}

// ---- spawn ----

int EliteRollRange(ZombieType type, bool skyDropped, int eliteOption) {
	if (eliteOption == eliteoption::NONE)
		return 0;
	const bool all = eliteOption == eliteoption::ALL;
	switch (type) {
	case ZOMBIE_NORMAL:
	case ZOMBIE_DUCKY_TUBE: return R(all ? "dayZombies.normal.rollRangeAllElite" : "dayZombies.normal.rollRange", 0); // 2.1, hooks 482/483
	case ZOMBIE_TRAFFIC_CONE: return R(all ? "dayZombies.cone.rollRangeAllElite" : "dayZombies.cone.rollRange", 0); // 4.1, hook 484
	case ZOMBIE_PAIL: return R(all ? "dayZombies.bucket.rollRangeAllElite" : "dayZombies.bucket.rollRange", 0);    // 5.1, hook 485
	case ZOMBIE_POLEVAULTER:                                                  // 6.2, hook 491
		if (all)
			return skyDropped ? R("dayZombies.pole.skyDropRollRangeAllElite", 0) : R("dayZombies.pole.rollRangeAllElite", 0);
		return skyDropped ? R("dayZombies.pole.skyDropRollRange", 0) : R("dayZombies.pole.rollRange", 0);
	default: return 0;
	}
}

bool NormalForcedCoin(ZombieType type, int eliteOption, bool iZombieLevel) {
	return iZombieLevel && eliteOption == eliteoption::ALL && (type == ZOMBIE_NORMAL || type == ZOMBIE_DUCKY_TUBE);
}

int RollEliteVariant(ZombieType type, int roll, int worldNumber, bool skyDropped, int eliteOption) {
	if (EliteRollRange(type, skyDropped, eliteOption) <= 0)
		return elite::NONE;
	switch (type) {
	case ZOMBIE_NORMAL:
	case ZOMBIE_DUCKY_TUBE:
		if (roll <= R("dayZombies.normal.stickyMaxRoll", -1)) return elite::STICKY_NOTE;
		if (roll <= R("dayZombies.normal.coinMaxRoll", -1)) return elite::SILVER_COIN;
		return elite::NONE;
	case ZOMBIE_TRAFFIC_CONE: // Rand(11): 0 -> 2, 1 -> 3 (exe: `cmp eax,1; jg skip; add eax,2`)
		return roll <= 1 ? roll + elite::PHONOGRAPH : elite::NONE;
	case ZOMBIE_PAIL:         // Rand(9): 0 -> 4, 1 -> 5
		return roll <= 1 ? roll + elite::PUMPKIN : elite::NONE;
	case ZOMBIE_POLEVAULTER: {
		const int r = skyDropped ? roll + R("dayZombies.pole.skyDropRollAdd", 0) : roll;
		if (r <= R("dayZombies.pole.gloveMaxRoll", -1)) return elite::GLOVE;
		if (r <= R("dayZombies.pole.roadSignMaxRoll", -1))
			return worldNumber == R("dayZombies.pole.roadSignWorld", -1) || eliteOption == eliteoption::ALL ? elite::ROAD_SIGN
			                                                                                              : elite::NONE;
		if (r <= R("dayZombies.pole.vaseMaxRoll", -1)) return elite::VASE_POLE;
		return elite::NONE;
	}
	default: return elite::NONE;
	}
}

float StartOffsetPx(ZombieType type) { // 1, hook 480
	if (type == ZOMBIE_JACK_IN_THE_BOX || type == ZOMBIE_JALAPENO_HEAD)
		return 0.0f;
	return static_cast<float>(R("dayZombies.startOffsetPx", 0));
}

int PoleStartBaseX(int vanillaBase) { return R("dayZombies.pole.startX", vanillaBase); } // hook 492

int ZombieStartBaseX(ZombieType type, int vanillaBase) { // hooks 495, 497, 512 (R8)
	switch (type) {
	case ZOMBIE_ZAMBONI: return R("dayZombies.zamboni.startX", vanillaBase);
	case ZOMBIE_CATAPULT: return R("dayZombies.catapult.startX", vanillaBase);
	case ZOMBIE_GARGANTUAR:
	case ZOMBIE_REDEYE_GARGANTUAR: return R("dayZombies.gargantuar.startX", vanillaBase);
	default: return vanillaBase;
	}
}

int ZombieStartBodyHp(ZombieType type, int variant, int vanilla) {
	if (IsNormalBody(type) && variant == elite::STICKY_NOTE) return R("dayZombies.sticky.bodyHp", vanilla);
	if (type == ZOMBIE_FLAG) return R("dayZombies.flag.bodyHp", vanilla);                 // hook 517
	if (type == ZOMBIE_PAIL && variant == elite::PUMPKIN) return R("dayZombies.pumpkin.bodyHp", vanilla);
	if (type == ZOMBIE_POLEVAULTER) return R("dayZombies.pole.bodyHp", vanilla);          // 0x522CB9
	return vanilla;
}

int EliteAttackRectExtraPx(int variant) { // hook 493
	return variant == elite::GLOVE ? R("dayZombies.glove.attackRectExtraPx", 0) : 0;
}

// ---- movement and eating ----

float EliteAnimRateMultiplier(ZombieType type, int variant) { // hook 662
	if (type == ZOMBIE_BOSS) return 1.0f;
	if (variant == elite::STICKY_NOTE) return RF("dayZombies.sticky.animRateMul", 1.0f);
	if (variant == elite::PUMPKIN) return RF("dayZombies.pumpkin.animRateMul", 1.0f);
	return 1.0f;
}

bool EliteEatsPlants(int variant) { // hook 675
	return variant != elite::STICKY_NOTE || R("dayZombies.sticky.eatsPlants", 1) != 0;
}

bool EliteShowsYuckyFace(int variant) { // hook 634: skipped for every variant >= 1
	return variant == elite::NONE || R("dayZombies.elite.noYuckyFace", 0) == 0;
}

bool BitesThisTick(ZombieType type, int zombieAge, int ticksBetweenEats) { // hook 672
	const int rem = zombieAge % ticksBetweenEats;
	if (type == ZOMBIE_FLAG) {
		const int lo = R("dayZombies.flag.biteRemainderMin", 0), hi = R("dayZombies.flag.biteRemainderMax", 0);
		if (hi > 0) return rem >= lo && rem <= hi;
	}
	return rem == 0;
}

// ---- Flag sun drain ----

bool FlagDrainsThisTick(ZombieType type, int ageBeforeIncrement) {
	const int period = R("dayZombies.flag.drainPeriodCs", 0);
	return type == ZOMBIE_FLAG && period > 0 && ageBeforeIncrement % period == 0;
}

bool FlagDrainAllowedIn(GameMode mode, bool isIZombieLevel, bool isAdventure, int boardLevel) { // fn 0x798310
	if (mode == GAMEMODE_CHALLENGE_LAST_STAND || mode == GAMEMODE_SCARY_POTTER_ENDLESS || isIZombieLevel)
		return false;
	return !(isAdventure && boardLevel == 55);
}

int FlagSunDrainRollRange() {
	const int lo = R("dayZombies.flag.drainMin", 0), hi = R("dayZombies.flag.drainMax", 0);
	return hi >= lo && hi > 0 ? hi - lo + 1 : 0;
}

int FlagSunDrain(int roll) { return roll + R("dayZombies.flag.drainMin", 0); }

// ---- auras ----

bool HasAura(int variant) { return variant == elite::PHONOGRAPH || variant == elite::PUMPKIN; }

bool AuraTick(int& counter) { // 0x797323: inc, then compare with 650
	const int period = R("dayZombies.aura.periodCs", 0);
	if (period <= 0) return false;
	if (++counter < period) return false;
	counter = 0;
	return true;
}

bool InAura(int sourceRow, int sourceX, int targetRow, int targetX) {
	const int rows = R("dayZombies.aura.rows", 0), range = R("dayZombies.aura.rangePx", 0);
	return std::abs(targetRow - sourceRow) <= rows && std::abs(sourceX - targetX) <= range;
}

bool AuraSkips(ZombieType type, int bodyMaxHealth, int variant) { // 0x7AA320
	return bodyMaxHealth > 120000 || type == ZOMBIE_BOSS || (type == ZOMBIE_NEWSPAPER && variant == 0x7F);
}

bool PumpkinGivesBucketTo(ZombieType type, HelmType helm) { // 0x7973D2: types <= 6 except Pole and Newspaper
	if (helm != HELMTYPE_NONE) return false;
	return type == ZOMBIE_NORMAL || type == ZOMBIE_FLAG || type == ZOMBIE_TRAFFIC_CONE || type == ZOMBIE_PAIL ||
		type == ZOMBIE_DOOR;
}

int PumpkinBucketHp() { return R("dayZombies.pumpkin.giveBucketHp", 1100); }

// ---- deaths ----

bool DeathEffectAllowed(ZombieHeight height, int fromWave) { // hook 691 guards
	return height != HEIGHT_GETTING_BUNGEE_DROPPED && fromWave >= 0;
}

bool SummonsDancersOnDeath(int variant) {
	return variant == elite::DANCER_HEAD && R("dayZombies.dancerHead.summonOnDeath", 0) != 0;
}

bool SkyDropsOnDeath(int variant) {
	return variant == elite::UMBRELLA && R("dayZombies.umbrella.skyDropOnDeath", 0) != 0;
}

int SkyDropTypeRollRange() { return R("dayZombies.skyDrop.typeRollRange", 0); }

bool SkyDropTypeAccepted(int roll) { return roll <= 7 || roll == 15 || roll >= 21; } // hook 85, 0x795C27

// ---- silver coin ----

bool CoinSquishes(ZombieType type, int variant, bool hasArm, bool hasHead) { // 0xC11210
	return type == ZOMBIE_NORMAL && variant == elite::SILVER_COIN && (!hasArm || !hasHead) &&
		R("dayZombies.coin.squishWhenCrippled", 0) != 0;
}

bool CoinCopiesOnBite(ZombieType type, int variant, int plantHealth) { // hook 686
	return type == ZOMBIE_NORMAL && variant == elite::SILVER_COIN &&
		plantHealth <= R("dayZombies.coin.copyAtPlantHpMax", -1);
}

bool CoinCopyNeedsFootballRoll(int playerLevel) {
	return playerLevel > R("dayZombies.coin.footballAfterPlayerLevel", 1 << 30);
}

int CoinCopyFootballRollRange() { return R("dayZombies.coin.footballRollRange", 0); }

bool CoinCopyIsFootball(int roll) { return roll == 0; }

bool PoleMakesGraveOnVault(int variant) { return variant != elite::GLOVE && variant != elite::VASE_POLE; }

// ---- glove pole leap (hook 361 -> 0x95adf0, 0x95af00; hook 557 -> 0x95ad80) ----

bool PlantDeathWakesGlove(PlantDeathCause cause) {
	if (R("dayZombies.glove.leapOnPlantDeath", 0) == 0) return false;
	return cause != PlantDeathCause::OTHER;
}

bool GloveLeapsFor(const GloveLeapCheck& c) {
	if (R("dayZombies.glove.leapOnPlantDeath", 0) == 0) return false;
	return !c.deadOrDying && c.type == ZOMBIE_POLEVAULTER && c.variant == elite::GLOVE &&  // 0x95ae50..0x95ae66
		c.phase != PHASE_POLEVAULTER_IN_VAULT && c.zombieRow == c.plantRow && c.zombieX > c.plantX; // ..0x95ae7c
}

GloveLeap GloveLeapAt(int plantX, ZombiePhase phase) {
	GloveLeap l;
	l.posX = static_cast<float>(plantX + R("dayZombies.glove.leapOffsetPx", 96));
	l.jumpMode = phase == PHASE_POLEVAULTER_PRE_VAULT ? 1 : 2;
	l.velX = l.jumpMode == 2 ? RF("dayZombies.glove.leapVelX", 0.5f) : 0.0f;
	l.bite = R("dayZombies.glove.leapBite", 800);
	return l;
}

bool GloveLandingKeepsPole(int variant, int jumpMode) {
	return R("dayZombies.glove.leapOnPlantDeath", 0) != 0 && variant == elite::GLOVE && jumpMode == 1;
}

// ---- art ----

EliteArt EliteArtFor(ZombieType type, int variant) { // study 9
	EliteArt a;
	auto head = [&](const char* file) {
		a.images.push_back({"anim_head1", file});
		a.hidePrefixes.push_back("anim_head2");
	};
	if (IsNormalBody(type)) {
		if (variant == elite::STICKY_NOTE) head("images/ZombieNoteSmall");
		else if (variant == elite::SILVER_COIN) head("reanim/Coin_silver_dollar");
	} else if (type == ZOMBIE_TRAFFIC_CONE) {
		if (variant == elite::PHONOGRAPH) head("images/Phonograph");
		else if (variant == elite::DANCER_HEAD) head("particles/ZombieDancerHead");
	} else if (type == ZOMBIE_PAIL) {
		if (variant == elite::PUMPKIN) head("reanim/Pumpkin_damage1");
		else if (variant == elite::UMBRELLA) {
			a.images.push_back({"anim_head1", "IMAGE_BLANK"});
			a.hidePrefixes = {"anim_hair", "anim_head2"};
			a.umbrellaLeaf = true;
		}
	} else if (type == ZOMBIE_POLEVAULTER) {
		if (variant == elite::GLOVE) head("images/Zen_GardenGlove");
		else if (variant == elite::VASE_POLE) {
			a.images.push_back({"Zombie_polevaulter_pole2", "reanim/Zombie_gargantuar_telephonepole"});
			a.images.push_back({"Zombie_polevaulter_pole", "particles/Zombie_seaweed"});
		} else if (variant == elite::ROAD_SIGN) {
			a.images.push_back({"Zombie_polevaulter_pole2", "reanim/Zombie_gargantuar_duckxing"});
			a.images.push_back({"Zombie_polevaulter_pole", "particles/Zombie_seaweed"});
		}
	}
	return a;
}

EliteArt ResolveEliteArt(ZombieType type, int variant, const std::function<bool(const std::string&)>& available) {
	EliteArt a = EliteArtFor(type, variant);
	for (const auto& img : a.images)
		if (img.file != "IMAGE_BLANK" && !available(img.file))
			return {};
	return a;
}

// ---- dev flag ----

std::optional<ForcedVariant> ParseForcedVariant(std::string_view text) {
	const auto colon = text.find(':');
	if (colon == std::string_view::npos || colon == 0 || colon + 1 >= text.size())
		return std::nullopt;
	std::string name(text.substr(0, colon));
	for (char& c : name) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
	if (name.starts_with("ZOMBIE_")) name.erase(0, 7);
	const std::string num(text.substr(colon + 1));
	char* end = nullptr;
	const long v = std::strtol(num.c_str(), &end, 0);
	if (end == num.c_str() || *end != '\0' || v < 0)
		return std::nullopt;
	return ForcedVariant{name, static_cast<int>(v)};
}

const std::vector<std::string>& DayZombieRuleKeys() {
	static const std::vector<std::string> keys = {
		"dayZombies.startOffsetPx",
		"dayZombies.normal.rollRange", "dayZombies.normal.stickyMaxRoll", "dayZombies.normal.coinMaxRoll",
		"dayZombies.sticky.bodyHp", "dayZombies.sticky.animRateMul", "dayZombies.sticky.eatsPlants",
		"dayZombies.elite.noYuckyFace",
		"dayZombies.coin.squishWhenCrippled", "dayZombies.coin.copyAtPlantHpMax",
		"dayZombies.coin.footballAfterPlayerLevel", "dayZombies.coin.footballRollRange",
		"dayZombies.flag.bodyHp", "dayZombies.flag.drainPeriodCs", "dayZombies.flag.drainMin",
		"dayZombies.flag.drainMax", "dayZombies.flag.biteRemainderMin", "dayZombies.flag.biteRemainderMax",
		"dayZombies.cone.rollRange", "dayZombies.bucket.rollRange",
		"dayZombies.pumpkin.bodyHp", "dayZombies.pumpkin.animRateMul", "dayZombies.pumpkin.giveBucketHp",
		"dayZombies.aura.periodCs", "dayZombies.aura.rangePx", "dayZombies.aura.rows",
		"dayZombies.dancerHead.summonOnDeath", "dayZombies.umbrella.skyDropOnDeath", "dayZombies.skyDrop.typeRollRange",
		"dayZombies.pole.bodyHp", "dayZombies.pole.startX", "dayZombies.pole.rollRange",
		"dayZombies.zamboni.startX", "dayZombies.catapult.startX", "dayZombies.gargantuar.startX",
		"dayZombies.pole.skyDropRollRange", "dayZombies.pole.skyDropRollAdd", "dayZombies.pole.gloveMaxRoll",
		"dayZombies.pole.roadSignMaxRoll", "dayZombies.pole.vaseMaxRoll", "dayZombies.pole.roadSignWorld",
		"dayZombies.glove.attackRectExtraPx",
		"dayZombies.glove.leapOnPlantDeath", "dayZombies.glove.leapOffsetPx", "dayZombies.glove.leapBite",
		"dayZombies.glove.leapVelX",
		"dayZombies.normal.rollRangeAllElite", "dayZombies.cone.rollRangeAllElite", "dayZombies.bucket.rollRangeAllElite",
		"dayZombies.pole.rollRangeAllElite", "dayZombies.pole.skyDropRollRangeAllElite",
	};
	return keys;
}

} // namespace pvzu::unnamed
