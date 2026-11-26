// PVZU: Plan 9 V1 zombie rules. Evidence: the research notes (v1-zombie-contradictions).
#include "Mod/Unnamed/ZombieGapsV1.h"

#include "Mod/Unnamed/FogZombies.h"
#include "Mod/Unnamed/NightZombies.h"
#include "Mod/Unnamed/RoofZombies.h"
#include "Mod/Unnamed/Rules.h"

#include <cmath>

namespace pvzu::unnamed {

namespace {
int R(const char* key, int off) { return RuleInt(key, off); }
bool On(const char* key) { return R(key, 0) != 0; }
constexpr int kRoadSign = 0x29; // Pole Vaulter (DayZombies elite::ROAD_SIGN)
} // namespace

bool GaspIgnoresBodyDamage(ZombiePhase phase) { // 0x651d22: cmp [ebp+0x28],0x1e; je past the body write
	return phase == PHASE_NEWSPAPER_MADDENING && On("zombieGapsV1.newspaper.gaspInvincible");
}

bool BurnBecomesHit(ZombieType type, int variant, ZombiePhase phase, bool hasArm) {
	if (type == ZOMBIE_NEWSPAPER) // 0x6514d5: cmp [esi+0x24],5; je 0x6514f1 -> TakeDamage(1800, 0x12)
		return On("zombieGapsV1.newspaper.burnIsHit");
	return type == ZOMBIE_DANCER && variant == nightelite::RANDOM_SUMMONER && phase == PHASE_DANCER_DANCING_IN && hasArm &&
		On("zombieGapsV1.moonwalker.burnIsHit"); // 0xc05490
}

bool ClearsHitFlags(ZombieType type, int subVariant, ZombiePhase phase) { // 0xc05900
	return type == ZOMBIE_NEWSPAPER && subVariant == nightsub::FOOTBALL_HEAD && phase == PHASE_NEWSPAPER_READING &&
		On("zombieGapsV1.redEye.readingClearsFlags");
}

bool GarlicRipsPaper(ZombieType type, int subVariant, ZombiePhase phase) { // 0x7954af -> 0xc05ad0
	return type == ZOMBIE_NEWSPAPER && subVariant == nightsub::FOOTBALL_HEAD && phase == PHASE_NEWSPAPER_READING &&
		On("zombieGapsV1.redEye.garlicRipsPaper");
}

bool KnifeBiteConditionsOn() { return On("zombieGapsV1.knife.biteConditions"); }

bool KnifeSkipsBite(ZombiePhase phase, bool hasObject, bool iceFreezeActive, bool plantFrozen) { // 0x651210, 0xc02730
	return phase == PHASE_NEWSPAPER_MAD && hasObject && !iceFreezeActive && !plantFrozen;
}

bool MoonwalkerRefusesFreeze(int variant, ZombiePhase phase) { // 0xc053c0 (phase and variant only, no type test)
	return variant == nightelite::RANDOM_SUMMONER && phase == PHASE_DANCER_DANCING_IN &&
		On("zombieGapsV1.moonwalker.refusesFreeze");
}

std::optional<int> MoonwalkerSavedBody(ZombieType type, int variant, ZombiePhase phase, bool hasArm, int damage, int body,
	int bodyMax) { // 0xc05400
	const int margin = R("zombieGapsV1.moonwalker.lethalMarginHp", 0);
	if (margin <= 0 || type != ZOMBIE_DANCER || phase != PHASE_DANCER_DANCING_IN ||
		variant != nightelite::RANDOM_SUMMONER || !hasArm || damage + margin < body)
		return std::nullopt;
	return bodyMax >> 1;
}

bool MoonwalkerBiteRestartsSummon(int variant, ZombiePhase phase) { // 0xc05510
	return variant == nightelite::RANDOM_SUMMONER && phase == PHASE_DANCER_DANCING_IN &&
		On("zombieGapsV1.moonwalker.biteRestartsSummon");
}

bool VaultKeepsSpeed(int variant) { return variant == kRoadSign && On("zombieGapsV1.roadSign.vaultKeepsSpeed"); }

bool StaffSummonsWithoutThrow(int variant, bool heldImp) {
	return variant == roofelite::STAFF && !heldImp && On("zombieGapsV1.staff.summonsWithoutThrow");
}

bool PogoCrushesPlants(ZombieType type, int variant, bool flatTires) { // 0x95b010 -> 0x52ad71
	return type == ZOMBIE_POGO && variant == fogelite::GARLIC && !flatTires && On("zombieGapsV1.garlicPogo.crushesPlants");
}

bool PogoLandMakesYucky(int variant, bool firstHit) { // 0x95991c (2nd+ hit) vs 0x9598ca (first, 1800, no yucky)
	if (variant != fogelite::GARLIC) return false;
	return !(firstHit && On("zombieGapsV1.garlicPogo.firstLandNoYucky"));
}

std::optional<GreenBite> GreenFootballBite(ZombieType type, int subVariant, int plantHealth, bool hasHelm, int helm,
	int helmMax, int body, int bodyMax, bool iZombieLevel) { // 0xc12f40 .. 0xc12699
	const int mul = R("zombieGapsV1.green.biteTypeMul", 0); // fimul [ebp+0x24]: the zombie type, 7
	if (mul <= 0 || type != ZOMBIE_FOOTBALL || subVariant != nightsub::GREEN)
		return std::nullopt;
	const double denom = static_cast<double>(hasHelm ? helm : 0) + body;
	if (denom <= 0) return std::nullopt;
	const double x = (static_cast<double>(helmMax) + bodyMax) / denom * mul;
	GreenBite b;
	b.plantHealth = static_cast<int>(std::nearbyint(plantHealth - x)); // fisub / fchs / fistp, round to nearest even
	b.largeSun = iZombieLevel && std::nearbyint(x) >= R("zombieGapsV1.green.largeSunAt", 75);
	return b;
}

std::optional<bool> PileBitesThisTick(int variant, bool holdsLadder, int zombieAge, bool chilled) { // 0x79c300
	if (variant != roofelite::LADDER_PILE || !On("zombieGapsV1.pile.biteCadence"))
		return std::nullopt;
	if (!holdsLadder) return true; // 0x79c35b
	const int rem = zombieAge % (chilled ? 8 : 4);
	return rem > 0 && rem < 6;
}

bool PileTargetRuleOnlyWhileCarrying() { return On("zombieGapsV1.pile.targetOnlyWhileCarrying"); }

std::optional<float> PlainZamboniVelX() {
	const float v = RuleFloat("zombieGapsV1.zamboni.plainVelX", -1.0f);
	return v >= 0.0f ? std::optional<float>(v) : std::nullopt;
}

const std::vector<std::string>& ZombieGapsV1RuleKeys() {
	static const std::vector<std::string> keys = {
		"zombieGapsV1.newspaper.gaspInvincible", "zombieGapsV1.newspaper.burnIsHit",
		"zombieGapsV1.redEye.readingClearsFlags", "zombieGapsV1.redEye.garlicRipsPaper",
		"zombieGapsV1.knife.biteConditions",
		"zombieGapsV1.moonwalker.refusesFreeze", "zombieGapsV1.moonwalker.lethalMarginHp",
		"zombieGapsV1.moonwalker.biteRestartsSummon", "zombieGapsV1.moonwalker.burnIsHit",
		"zombieGapsV1.roadSign.vaultKeepsSpeed", "zombieGapsV1.staff.summonsWithoutThrow",
		"zombieGapsV1.garlicPogo.crushesPlants", "zombieGapsV1.garlicPogo.firstLandNoYucky",
		"zombieGapsV1.green.biteTypeMul", "zombieGapsV1.green.largeSunAt",
		"zombieGapsV1.pile.biteCadence", "zombieGapsV1.pile.targetOnlyWhileCarrying",
		"zombieGapsV1.zamboni.plainVelX",
	};
	return keys;
}

} // namespace pvzu::unnamed
