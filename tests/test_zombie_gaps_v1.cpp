// PVZU: Plan 9 V1, zombie rules where the exe sided with the Chinese community (pure rules; evidence in
// the research notes (v1-zombie-contradictions) and pvz-unnamed-pack/SOURCES-zombie-gaps-v1.md).
#include "doctest/doctest.h"
#include "Mod/Unnamed/FogZombies.h"
#include "Mod/Unnamed/NightZombies.h"
#include "Mod/Unnamed/RoofZombies.h"
#include "Mod/Unnamed/Rules.h"
#include "Mod/Unnamed/ZombieGapsV1.h"

#include "nlohmann/json.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>

using namespace pvzu::unnamed;
namespace fs = std::filesystem;

namespace {

// The Unnamed values (same as pvz-unnamed-pack/data/rules-zombie-gaps-v1.json; the last test compares the two).
const nlohmann::json kV1Rules = {
	{"zombieGapsV1.newspaper.gaspInvincible", 1},
	{"zombieGapsV1.newspaper.burnIsHit", 1},
	{"zombieGapsV1.redEye.readingClearsFlags", 1},
	{"zombieGapsV1.redEye.garlicRipsPaper", 1},
	{"zombieGapsV1.knife.biteConditions", 1},
	{"zombieGapsV1.moonwalker.refusesFreeze", 1},
	{"zombieGapsV1.moonwalker.lethalMarginHp", 200},
	{"zombieGapsV1.moonwalker.biteRestartsSummon", 1},
	{"zombieGapsV1.moonwalker.burnIsHit", 1},
	{"zombieGapsV1.roadSign.vaultKeepsSpeed", 1},
	{"zombieGapsV1.staff.summonsWithoutThrow", 1},
	{"zombieGapsV1.garlicPogo.crushesPlants", 1},
	{"zombieGapsV1.garlicPogo.firstLandNoYucky", 1},
	{"zombieGapsV1.green.biteTypeMul", 7},
	{"zombieGapsV1.green.largeSunAt", 75},
	{"zombieGapsV1.pile.biteCadence", 1},
	{"zombieGapsV1.pile.targetOnlyWhileCarrying", 1},
	{"zombieGapsV1.zamboni.plainVelX", 0.25},
};

struct V1On {
	V1On() { SetRules(kV1Rules); }
	~V1On() { SetRules(nlohmann::json::object()); }
};

constexpr int kWhite = nightelite::RANDOM_SUMMONER;

} // namespace

TEST_CASE("ZombieGapsV1OffWithoutRules") {
	SetRules(nlohmann::json::object());
	CHECK_FALSE(GaspIgnoresBodyDamage(PHASE_NEWSPAPER_MADDENING));
	CHECK_FALSE(BurnBecomesHit(ZOMBIE_NEWSPAPER, 0, PHASE_NEWSPAPER_READING, true));
	CHECK_FALSE(BurnBecomesHit(ZOMBIE_DANCER, kWhite, PHASE_DANCER_DANCING_IN, true));
	CHECK_FALSE(ClearsHitFlags(ZOMBIE_NEWSPAPER, nightsub::FOOTBALL_HEAD, PHASE_NEWSPAPER_READING));
	CHECK_FALSE(GarlicRipsPaper(ZOMBIE_NEWSPAPER, nightsub::FOOTBALL_HEAD, PHASE_NEWSPAPER_READING));
	CHECK_FALSE(KnifeBiteConditionsOn());
	CHECK_FALSE(MoonwalkerRefusesFreeze(kWhite, PHASE_DANCER_DANCING_IN));
	CHECK_FALSE(MoonwalkerSavedBody(ZOMBIE_DANCER, kWhite, PHASE_DANCER_DANCING_IN, true, 5000, 500, 500).has_value());
	CHECK_FALSE(MoonwalkerBiteRestartsSummon(kWhite, PHASE_DANCER_DANCING_IN));
	CHECK_FALSE(VaultKeepsSpeed(0x29));
	CHECK_FALSE(StaffSummonsWithoutThrow(roofelite::STAFF, false));
	CHECK_FALSE(PogoCrushesPlants(ZOMBIE_POGO, fogelite::GARLIC, false));
	CHECK(PogoLandMakesYucky(fogelite::GARLIC, true));
	CHECK_FALSE(GreenFootballBite(ZOMBIE_FOOTBALL, nightsub::GREEN, 300, true, 3000, 3000, 300, 300, false).has_value());
	CHECK_FALSE(PileBitesThisTick(roofelite::LADDER_PILE, true, 1, false).has_value());
	CHECK_FALSE(PileTargetRuleOnlyWhileCarrying());
	CHECK_FALSE(PlainZamboniVelX().has_value());
}

TEST_CASE("NewspaperGaspAndBurn") {
	V1On on;
	CHECK(GaspIgnoresBodyDamage(PHASE_NEWSPAPER_MADDENING));      // no type test in the cave
	CHECK_FALSE(GaspIgnoresBodyDamage(PHASE_NEWSPAPER_READING));
	CHECK_FALSE(GaspIgnoresBodyDamage(PHASE_NEWSPAPER_MAD));
	for (ZombiePhase p : {PHASE_NEWSPAPER_READING, PHASE_NEWSPAPER_MADDENING, PHASE_NEWSPAPER_MAD})
		CHECK(BurnBecomesHit(ZOMBIE_NEWSPAPER, 0, p, true));     // every Newspaper, any form, any phase
	CHECK(BurnBecomesHit(ZOMBIE_NEWSPAPER, nightelite::KNIFE, PHASE_NEWSPAPER_MAD, false));
	CHECK_FALSE(BurnBecomesHit(ZOMBIE_NORMAL, 0, PHASE_ZOMBIE_NORMAL, true));
	// the white summoner only while dancing in with its arm
	CHECK(BurnBecomesHit(ZOMBIE_DANCER, kWhite, PHASE_DANCER_DANCING_IN, true));
	CHECK_FALSE(BurnBecomesHit(ZOMBIE_DANCER, kWhite, PHASE_DANCER_DANCING_IN, false));
	CHECK_FALSE(BurnBecomesHit(ZOMBIE_DANCER, kWhite, PHASE_DANCER_SNAPPING_FINGERS, true));
	CHECK_FALSE(BurnBecomesHit(ZOMBIE_DANCER, 0, PHASE_DANCER_DANCING_IN, true));
}

TEST_CASE("RedEyeNewspaperReading") {
	V1On on;
	CHECK(ClearsHitFlags(ZOMBIE_NEWSPAPER, nightsub::FOOTBALL_HEAD, PHASE_NEWSPAPER_READING));
	CHECK_FALSE(ClearsHitFlags(ZOMBIE_NEWSPAPER, nightsub::FOOTBALL_HEAD, PHASE_NEWSPAPER_MAD));
	CHECK_FALSE(ClearsHitFlags(ZOMBIE_NEWSPAPER, 0, PHASE_NEWSPAPER_READING));
	CHECK_FALSE(ClearsHitFlags(ZOMBIE_NORMAL, nightsub::FOOTBALL_HEAD, PHASE_NEWSPAPER_READING));
	CHECK(GarlicRipsPaper(ZOMBIE_NEWSPAPER, nightsub::FOOTBALL_HEAD, PHASE_NEWSPAPER_READING));
	CHECK_FALSE(GarlicRipsPaper(ZOMBIE_NEWSPAPER, nightsub::FOOTBALL_HEAD, PHASE_NEWSPAPER_MADDENING));
	CHECK_FALSE(GarlicRipsPaper(ZOMBIE_NEWSPAPER, 0, PHASE_NEWSPAPER_READING));
}

TEST_CASE("KnifeBiteOnlyWhenMadWithKnife") {
	V1On on;
	CHECK(KnifeBiteConditionsOn());
	CHECK(KnifeSkipsBite(PHASE_NEWSPAPER_MAD, true, false, false));
	CHECK_FALSE(KnifeSkipsBite(PHASE_NEWSPAPER_READING, true, false, false)); // still reading: a normal bite
	CHECK_FALSE(KnifeSkipsBite(PHASE_NEWSPAPER_MAD, false, false, false));
	CHECK_FALSE(KnifeSkipsBite(PHASE_NEWSPAPER_MAD, true, true, false));     // ice-head Jack freeze ([0x800100])
	CHECK_FALSE(KnifeSkipsBite(PHASE_NEWSPAPER_MAD, true, false, true));     // Zomboni-frozen plant (+0x144 == 0)
}

TEST_CASE("WhiteSummonerDancingIn") {
	V1On on;
	CHECK(MoonwalkerRefusesFreeze(kWhite, PHASE_DANCER_DANCING_IN));
	CHECK_FALSE(MoonwalkerRefusesFreeze(kWhite, PHASE_DANCER_SNAPPING_FINGERS));
	CHECK_FALSE(MoonwalkerRefusesFreeze(nightelite::MOONWALKER, PHASE_DANCER_DANCING_IN));
	// 0xc05432: lea ecx,[damage+200]; cmp ecx, body; jl -> not lethal
	CHECK(MoonwalkerSavedBody(ZOMBIE_DANCER, kWhite, PHASE_DANCER_DANCING_IN, true, 300, 500, 500) == 250);
	CHECK(MoonwalkerSavedBody(ZOMBIE_DANCER, kWhite, PHASE_DANCER_DANCING_IN, true, 1800, 100, 501) == 250); // max >> 1
	CHECK_FALSE(MoonwalkerSavedBody(ZOMBIE_DANCER, kWhite, PHASE_DANCER_DANCING_IN, true, 299, 500, 500).has_value());
	CHECK_FALSE(MoonwalkerSavedBody(ZOMBIE_DANCER, kWhite, PHASE_DANCER_DANCING_IN, false, 1800, 500, 500).has_value());
	CHECK_FALSE(MoonwalkerSavedBody(ZOMBIE_DANCER, kWhite, PHASE_DANCER_SNAPPING_FINGERS, true, 1800, 500, 500).has_value());
	CHECK_FALSE(MoonwalkerSavedBody(ZOMBIE_DANCER, 0, PHASE_DANCER_DANCING_IN, true, 1800, 500, 500).has_value());
	CHECK_FALSE(MoonwalkerSavedBody(ZOMBIE_BACKUP_DANCER, kWhite, PHASE_DANCER_DANCING_IN, true, 1800, 500, 500).has_value());
	CHECK(MoonwalkerBiteRestartsSummon(kWhite, PHASE_DANCER_DANCING_IN));
	CHECK_FALSE(MoonwalkerBiteRestartsSummon(kWhite, PHASE_DANCER_DANCING_LEFT));
	CHECK_FALSE(MoonwalkerBiteRestartsSummon(nightelite::TROPHY, PHASE_DANCER_DANCING_IN));
}

TEST_CASE("RoadSignStaffPogoZamboni") {
	V1On on;
	CHECK(VaultKeepsSpeed(0x29));
	CHECK_FALSE(VaultKeepsSpeed(0x32));
	CHECK_FALSE(VaultKeepsSpeed(0));
	CHECK(StaffSummonsWithoutThrow(roofelite::STAFF, false));
	CHECK_FALSE(StaffSummonsWithoutThrow(roofelite::STAFF, true)); // its first throw: the Imp it carried
	CHECK_FALSE(StaffSummonsWithoutThrow(0, false));
	CHECK(PogoCrushesPlants(ZOMBIE_POGO, fogelite::GARLIC, false));
	CHECK_FALSE(PogoCrushesPlants(ZOMBIE_POGO, fogelite::GARLIC, true));   // mFlatTires (+0x78)
	CHECK_FALSE(PogoCrushesPlants(ZOMBIE_POGO, fogelite::CHOCOLATE, false));
	CHECK_FALSE(PogoLandMakesYucky(fogelite::GARLIC, true));
	CHECK(PogoLandMakesYucky(fogelite::GARLIC, false));
	CHECK_FALSE(PogoLandMakesYucky(fogelite::CHOCOLATE, false));
	CHECK(PlainZamboniVelX() == 0.25f);
}

TEST_CASE("GreenFootballBiteGrowsAsItIsHurt") {
	V1On on;
	auto bite = [](int plant, bool hasHelm, int helm, int body, bool iz = false) {
		return GreenFootballBite(ZOMBIE_FOOTBALL, nightsub::GREEN, plant, hasHelm, helm, 3000, body, 300, iz);
	};
	CHECK(bite(300, true, 3000, 300) == GreenBite{293, false});      // x = 3300 / 3300 * 7 = 7
	CHECK(bite(300, false, 0, 300) == GreenBite{223, false});        // 3300 / 300 * 7 = 77
	CHECK(bite(300, false, 0, 300, true) == GreenBite{223, true});   // round(77) >= 75 in I, Zombie: large sun
	CHECK(bite(300, true, 1000, 200) == GreenBite{281, false});      // 3300 / 1200 * 7 = 19.25: 280.75 rounds to 281
	CHECK(bite(10, false, 0, 22) == GreenBite{-1040, false});         // 3300 / 22 * 7 = 1050
	CHECK(bite(300, false, 0, 1000) == GreenBite{277, false});        // 23.1 -> 276.9 -> 277
	CHECK(bite(301, false, 0, 1320) == GreenBite{284, false});        // 17.5 -> 283.5 -> 284 (to even)
	CHECK(bite(302, false, 0, 1320) == GreenBite{284, false});        // 284.5 -> 284 (to even)
	CHECK_FALSE(GreenFootballBite(ZOMBIE_FOOTBALL, 0, 300, true, 3000, 3000, 300, 300, false).has_value());
	CHECK_FALSE(GreenFootballBite(ZOMBIE_NEWSPAPER, nightsub::GREEN, 300, true, 3000, 3000, 300, 300, false).has_value());
}

TEST_CASE("LadderPileBites") {
	V1On on;
	// with a ladder: 3 of 4 ticks (rem 1, 2, 3), chilled 5 of 8 (rem 1..5)
	int bites = 0;
	for (int age = 0; age < 400; ++age) bites += *PileBitesThisTick(roofelite::LADDER_PILE, true, age, false);
	CHECK(bites == 300);
	bites = 0;
	for (int age = 0; age < 400; ++age) bites += *PileBitesThisTick(roofelite::LADDER_PILE, true, age, true);
	CHECK(bites == 250);
	CHECK_FALSE(*PileBitesThisTick(roofelite::LADDER_PILE, true, 4, false));
	for (int age = 0; age < 16; ++age) CHECK(*PileBitesThisTick(roofelite::LADDER_PILE, false, age, age % 2));
	CHECK_FALSE(PileBitesThisTick(roofelite::GHOST_LADDER, true, 1, false).has_value());
	CHECK(PileTargetRuleOnlyWhileCarrying());
}

TEST_CASE("ZombieGapsV1PackRulesMatch") {
	for (const auto& k : ZombieGapsV1RuleKeys()) CHECK_MESSAGE(kV1Rules.contains(k), k);
	CHECK(ZombieGapsV1RuleKeys().size() == kV1Rules.size());
	const char* home = std::getenv("HOME");
	fs::path file = std::getenv("PVZU_PACK_DIR") ? fs::path(std::getenv("PVZU_PACK_DIR"))
	                                              : fs::path(home ? home : "") / "pvz-unnamed-pack";
	file /= "data/rules-zombie-gaps-v1.json";
	if (!fs::exists(file)) { MESSAGE("pack file not here, skipped: " << file.string()); return; }
	std::ifstream in(file);
	nlohmann::json pack = nlohmann::json::parse(in, nullptr, false);
	REQUIRE(pack.is_object());
	CHECK(pack == kV1Rules);
}
