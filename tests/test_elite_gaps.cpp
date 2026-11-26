// PVZU: Plan 9 R4, Football gaps and elite-option-0 branches (pure rules; evidence in
// the research notes (r4-football) and pvz-unnamed-pack/SOURCES-elite-gaps.md).
#include "doctest/doctest.h"
#include "Mod/Unnamed/EliteGaps.h"
#include "Mod/Unnamed/NightZombies.h"
#include "Mod/Unnamed/Options.h"
#include "Mod/Unnamed/Rules.h"
#include "Mod/Unnamed/World7Zombies.h"

#include "nlohmann/json.hpp"

#include <cstdlib>
#include <deque>
#include <filesystem>
#include <fstream>
#include <random>

using namespace pvzu::unnamed;
namespace fs = std::filesystem;
namespace ne = pvzu::unnamed::nightelite;
namespace ns = pvzu::unnamed::nightsub;
namespace eo = pvzu::unnamed::eliteoption;

namespace {

// The Unnamed values (same as pvz-unnamed-pack/data/rules-elite-gaps.json; the last test compares the two).
const nlohmann::json kGapRules = {
	{"eliteGaps.black.walkMul", 0.85},
	{"eliteGaps.black.eatMul", 1.2},
	{"eliteGaps.black.zeroBecomes", 1},
	{"eliteGaps.black.smallHitBelow", 5},
	{"eliteGaps.black.smallHitRoll", 5},
	{"eliteGaps.black.refusesFreeze", 1},
	{"eliteGaps.football.goldDrawsBlackRoll", 1},
	{"eliteGaps.football.graveSkipsGold", 1},
	{"eliteGaps.green.walkEvent", 1},
	{"eliteGaps.green.gatlingAbovePlayerLevel", 60},
	{"eliteGaps.gatling.headX", 83.0},
	{"eliteGaps.gatling.headY", 0.0},
	{"eliteGaps.gatling.headRot", 0.2},
	{"eliteGaps.gatling.headSx", -1.0},
	{"eliteGaps.gatling.headSy", 1.0},
	{"eliteGaps.gatling.startCounter", 150},
	{"eliteGaps.gatling.extraShots", {1, 20, 39, 64, 83, 102, 121, 146}},
	{"eliteGaps.gatling.spreads", 0},
	{"eliteGaps.gatling.skipPhase", 50},
	{"eliteGaps.newspaper.above70Level", 70},
	{"eliteGaps.newspaper.paperFromCounter", 1200},
	{"eliteGaps.newspaper.paperFullCounter", 30000},
	{"eliteGaps.newspaper.paperFullHp", 3000},
	{"eliteGaps.newspaper.paperDivisor", 10},
	{"eliteGaps.newspaper.rollsInIZombie", 1},
	{"eliteGaps.helm.iZombieRoll", 3},
	{"eliteGaps.helm.hp", 2800},
	{"eliteGaps.helm.boardLevelAbove", 70},
	{"eliteGaps.helm.waveAbove", 25},
	{"eliteGaps.iZombieNormal.coinRoll", 7},
	{"eliteGaps.iZombieNormal.coinRollHelm", 30},
	{"eliteGaps.iZombieNormal.coinMaxRollPlusOne", 3},
};

// The Night / World-7 keys these rules lean on (pack values).
const nlohmann::json kNeighbourRules = {
	{"nightZombies.football.greenRollRange", 4},
	{"nightZombies.football.goldRollRange", 13},
	{"nightZombies.football.extraRollRange", 3},
	{"nightZombies.football.greenHelmDropAgeCs", 800},
	{"nightZombies.football.greenHelmDropMaxPlayerLevel", 70},
	{"nightZombies.football.greenChewBodyBonus", 1000},
	{"nightZombies.newspaper.footballHeadMaxRoll", 1},
	{"nightZombies.newspaper.almanacRoll", 2},
	{"nightZombies.newspaper.knifeRoll", 3},
	{"world7Zombies.above60.level", 60},
	{"world7Zombies.football.blackExtraHit", 0},
};

struct GapRulesOn {
	explicit GapRulesOn(bool withGaps = true) {
		nlohmann::json all = kNeighbourRules;
		if (withGaps)
			for (auto it = kGapRules.begin(); it != kGapRules.end(); ++it) all[it.key()] = it.value();
		SetRules(all);
	}
	~GapRulesOn() { SetRules(nlohmann::json::object()); }
};

// a scripted Rand: returns the queued values in order and records every n asked for
struct Script {
	std::deque<int> values;
	std::vector<int> asked;
	int operator()(int n) {
		asked.push_back(n);
		REQUIRE(!values.empty());
		const int v = values.front();
		values.pop_front();
		return v;
	}
};

struct Roller {
	std::mt19937 rng{20261123};
	int operator()(int n) { return std::uniform_int_distribution<int>(0, n - 1)(rng); }
};

NightForm F(int v, int s = 0) { return NightForm{v, s}; }

} // namespace

TEST_CASE("EliteGapsVanillaWithoutRules") {
	SetRules(nlohmann::json::object());
	CHECK(BlackArmourDamage(20, false) == 20);
	CHECK(BlackArmourDamage(0, true) == 0);
	CHECK(BlackArmourSmallHitRollRange(1) == 0);
	CHECK_FALSE(BlackArmourRefusesFreeze(ZOMBIE_FOOTBALL, w7::BLACK_FOOTBALL));
	CHECK_FALSE(GreenWalkEventOn());
	CHECK(GreenFootballWalkEvent(ZOMBIE_FOOTBALL, 0, ns::GREEN, true, 5000, 65) == GreenWalkEvent{});
	CHECK_FALSE(GreenRefreshesAnimSpeed(ZOMBIE_FOOTBALL, 0, ns::GREEN));
	CHECK_FALSE(GatlingFootballExtraShot(1));
	CHECK(NewspaperAbove70(F(0), 4, 73, false, eo::DEFAULT) == F(0));
	CHECK(NewspaperAbove70PaperHp(73, 20000, 150) == 150);
	CHECK_FALSE(NewspaperRollsInIZombie());
	CHECK(FootballHelmRollRange(true) == 0);
	CHECK_FALSE(GetsFootballHelm(false, 0, 73, 30));
	CHECK(IZombieNormalCoinRollRange(false) == 0);
	CHECK_FALSE(GatlingUpdateSkipped(PHASE_DANCER_RISING));
}

TEST_CASE("BlackArmourDamageIsFloatRounded") {
	GapRulesOn on;
	// 0x818069: fild; fmul 0.85f / 1.2f; fistp (nearest). 0.85f = 0.85000002, 1.2f = 1.20000005
	CHECK(BlackArmourDamage(20, false) == 17);
	CHECK(BlackArmourDamage(20, true) == 24);
	CHECK(BlackArmourDamage(1800, false) == 1530);
	CHECK(BlackArmourDamage(1800, true) == 2160);
	CHECK(BlackArmourDamage(40, false) == 34);
	CHECK(BlackArmourDamage(10, false) == 9);    // 8.5000002 -> 9 (the float is just above the tie)
	CHECK(BlackArmourDamage(5, true) == 6);
	CHECK(BlackArmourDamage(1, false) == 1);
	CHECK(BlackArmourDamage(0, false) == 1);     // `test ebp,ebp; jne; inc ebp`
	CHECK(BlackArmourDamage(0, true) == 1);
	// 0xc00710: below 5, Rand(5), damage <= roll -> 0
	CHECK(BlackArmourSmallHitRollRange(4) == 5);
	CHECK(BlackArmourSmallHitRollRange(5) == 0);
	CHECK(BlackArmourSmallHit(4, 4) == 0);
	CHECK(BlackArmourSmallHit(4, 3) == 4);
	CHECK(BlackArmourSmallHit(1, 0) == 1);
	CHECK(BlackArmourSmallHit(1, 1) == 0);
	CHECK(BlackArmourRefusesFreeze(ZOMBIE_FOOTBALL, w7::BLACK_FOOTBALL));
	CHECK_FALSE(BlackArmourRefusesFreeze(ZOMBIE_FOOTBALL, ns::GREEN));
	CHECK_FALSE(BlackArmourRefusesFreeze(ZOMBIE_DOOR, w7::BLACK_FOOTBALL));
}

TEST_CASE("FootballDefaultRollEveryBranch") {
	GapRulesOn on;
	{ // green, gold: Rand(4)=0, Rand(13)=0, nothing more
		Script s{{0, 0}};
		CHECK(RollFootballDefault(std::ref(s), true, false) == FootballDefaultRolls{F(ne::GOLD_FOOTBALL, ns::GREEN), -1, false});
		CHECK(s.asked == std::vector<int>{4, 13});
	}
	{ // green, not gold: Rand(3) drawn, never black
		Script s{{0, 5, 0}};
		CHECK(RollFootballDefault(std::ref(s), true, false) == FootballDefaultRolls{F(0, ns::GREEN), 0, false});
		CHECK(s.asked == std::vector<int>{4, 13, 3});
	}
	{ // plain, not gold, profile > 60: eligible
		Script s{{1, 7, 0}};
		CHECK(RollFootballDefault(std::ref(s), true, false) == FootballDefaultRolls{F(0), 0, true});
		CHECK(s.asked == std::vector<int>{4, 13, 3});
	}
	{ // plain, not gold, profile <= 60: drawn, not eligible
		Script s{{2, 7, 0}};
		CHECK(RollFootballDefault(std::ref(s), false, false) == FootballDefaultRolls{F(0), 0, false});
	}
	{ // gold at profile > 60: the third roll follows (bl = 3), black possible (0x9dd423)
		Script s{{3, 0, 0}};
		CHECK(RollFootballDefault(std::ref(s), true, false) == FootballDefaultRolls{F(ne::GOLD_FOOTBALL), 0, true});
		CHECK(s.asked == std::vector<int>{4, 13, 3});
	}
	{ // gold at profile <= 60: no third roll
		Script s{{3, 0}};
		CHECK(RollFootballDefault(std::ref(s), false, false) == FootballDefaultRolls{F(ne::GOLD_FOOTBALL), -1, false});
		CHECK(s.asked == std::vector<int>{4, 13});
	}
	{ // grave rise (flag 1), plain: no gold roll; profile > 60 -> Rand(3), eligible
		Script s{{1, 0}};
		CHECK(RollFootballDefault(std::ref(s), true, true) == FootballDefaultRolls{F(0), 0, true});
		CHECK(s.asked == std::vector<int>{4, 3});
	}
	{ // grave rise, plain, profile <= 60: nothing after the green roll
		Script s{{1}};
		CHECK(RollFootballDefault(std::ref(s), false, true) == FootballDefaultRolls{F(0), -1, false});
		CHECK(s.asked == std::vector<int>{4});
	}
	{ // grave rise, green: Rand(13) drawn, gold refused (0xc12bc2), Rand(3) drawn
		Script s{{0, 0, 2}};
		CHECK(RollFootballDefault(std::ref(s), true, true) == FootballDefaultRolls{F(0, ns::GREEN), 2, false});
		CHECK(s.asked == std::vector<int>{4, 13, 3});
	}
}

TEST_CASE("FootballDefaultRollWithoutTheGapKeysIsTheOldChain") {
	GapRulesOn on(false);
	Script s{{3, 0}};   // gold never drew the third roll, graves rolled gold
	CHECK(RollFootballDefault(std::ref(s), true, false) == FootballDefaultRolls{F(ne::GOLD_FOOTBALL), -1, false});
	Script g{{1, 0}};
	CHECK(RollFootballDefault(std::ref(g), true, true) == FootballDefaultRolls{F(ne::GOLD_FOOTBALL), -1, false});
}

TEST_CASE("FootballDefaultRatesAtProfile65") {
	GapRulesOn on;
	Roller rand;
	constexpr int kN = 20000;
	int black = 0, goldBlack = 0, gold = 0, green = 0;
	for (int i = 0; i < kN; ++i) {
		const FootballDefaultRolls r = RollFootballDefault(std::ref(rand), true, false);
		const bool b = r.blackEligible && FootballBecomesBlack(r.extraRoll, false, false, 65);
		black += b;
		goldBlack += b && r.form.variant == ne::GOLD_FOOTBALL;
		gold += !b && r.form.variant == ne::GOLD_FOOTBALL && r.form.subVariant == 0;
		green += r.form.subVariant == ns::GREEN;
	}
	CHECK(black / double(kN) == doctest::Approx(0.25).epsilon(0.06));
	CHECK(goldBlack / double(kN) == doctest::Approx(1.0 / 52).epsilon(0.2));
	CHECK(gold / double(kN) == doctest::Approx(1.0 / 26).epsilon(0.15));
	CHECK(green / double(kN) == doctest::Approx(0.25).epsilon(0.06));
}

TEST_CASE("GreenFootballWalkEvent") {
	GapRulesOn on;
	CHECK(GreenWalkEventOn());
	const auto ev = [](int variant, int sub, bool helm, int age, int level) {
		return GreenFootballWalkEvent(ZOMBIE_FOOTBALL, variant, sub, helm, age, level);
	};
	CHECK(ev(0, ns::GREEN, true, 799, 41) == GreenWalkEvent{});
	CHECK(ev(0, ns::GREEN, true, 800, 41) == GreenWalkEvent{true, true, false, 1000});    // world 2: helmet off
	CHECK(ev(0, ns::GREEN, true, 800, 60) == GreenWalkEvent{true, true, false, 1000});
	CHECK(ev(0, ns::GREEN, true, 800, 61) == GreenWalkEvent{true, true, true, 1000});     // world 7: both
	CHECK(ev(0, ns::GREEN, true, 900, 70) == GreenWalkEvent{true, true, true, 1000});
	CHECK(ev(0, ns::GREEN, true, 900, 71) == GreenWalkEvent{true, false, true, 1000});    // 71-73 keep the helmet
	CHECK(ev(ne::GOLD_FOOTBALL, ns::GREEN, true, 900, 65) == GreenWalkEvent{true, true, true, 1000});
	CHECK(ev(0, ns::GREEN, false, 900, 65) == GreenWalkEvent{});                          // helmet gone: never again
	CHECK(ev(gaps::GATLING_FOOTBALL, ns::GREEN, true, 900, 73) == GreenWalkEvent{});      // already has its head
	CHECK(ev(0, w7::BLACK_FOOTBALL, true, 900, 65) == GreenWalkEvent{});
	CHECK(ev(0, 0, true, 900, 65) == GreenWalkEvent{});
	CHECK(GreenFootballWalkEvent(ZOMBIE_NEWSPAPER, 0, ns::GREEN, true, 900, 65) == GreenWalkEvent{});
	CHECK(GreenRefreshesAnimSpeed(ZOMBIE_FOOTBALL, 0, ns::GREEN));
	CHECK(GreenRefreshesAnimSpeed(ZOMBIE_FOOTBALL, ne::GOLD_FOOTBALL, ns::GREEN));
	CHECK_FALSE(GreenRefreshesAnimSpeed(ZOMBIE_FOOTBALL, gaps::GATLING_FOOTBALL, ns::GREEN));
	CHECK_FALSE(GreenRefreshesAnimSpeed(ZOMBIE_FOOTBALL, 0, 0));
}

TEST_CASE("GatlingFootballHead") {
	GapRulesOn on;
	CHECK(IsGatlingFootball(ZOMBIE_FOOTBALL, gaps::GATLING_FOOTBALL));
	CHECK_FALSE(IsGatlingFootball(ZOMBIE_GATLING_HEAD, gaps::GATLING_FOOTBALL));
	CHECK_FALSE(IsGatlingFootball(ZOMBIE_FOOTBALL, 0));
	CHECK(GatlingFootballHeadOffset() == HeadOffset{83.0f, 0.0f, 0.2f, -1.0f, 1.0f});
	CHECK(GatlingFootballStartCounter() == 150);
	int shots = 0;
	for (int c = 150; c >= 0; --c) shots += GatlingFootballExtraShot(c) || c == 18 || c == 35 || c == 51 || c == 68;
	CHECK(shots == 12);   // 12 peas per 150 cs, the ZomBotany head fires 4
	for (int c : {1, 20, 39, 64, 83, 102, 121, 146}) CHECK_MESSAGE(GatlingFootballExtraShot(c), c);
	for (int c : {0, 18, 100, 150}) CHECK_MESSAGE(!GatlingFootballExtraShot(c), c);
	CHECK_FALSE(GatlingFootballSpreads());
	CHECK(GatlingUpdateSkipped(PHASE_DANCER_RISING));
	CHECK_FALSE(GatlingUpdateSkipped(PHASE_ZOMBIE_NORMAL));
}

TEST_CASE("NewspaperAboveProfile70") {
	GapRulesOn on;
	// at or below 70 nothing changes
	CHECK(NewspaperAbove70(F(0), 4, 70, true, eo::DEFAULT) == F(0));
	CHECK(NewspaperAbove70(F(ne::KNIFE), 3, 70, true, eo::DEFAULT) == F(ne::KNIFE));
	// above 70: the football head always, with the knife / Almanac paper of rolls 3 / 2
	CHECK(NewspaperAbove70(F(0), 4, 71, true, eo::DEFAULT) == F(0, ns::FOOTBALL_HEAD));
	CHECK(NewspaperAbove70(F(0, ns::FOOTBALL_HEAD), 0, 71, true, eo::DEFAULT) == F(0, ns::FOOTBALL_HEAD));
	CHECK(NewspaperAbove70(F(ne::KNIFE), 3, 73, true, eo::DEFAULT) == F(ne::KNIFE, ns::FOOTBALL_HEAD));
	CHECK(NewspaperAbove70(F(ne::ALMANAC_PAPER), 2, 73, true, eo::DEFAULT) == F(ne::ALMANAC_PAPER, ns::FOOTBALL_HEAD));
	CHECK(NewspaperAbove70(F(0), 2, 73, false, eo::DEFAULT) == F(0, ns::FOOTBALL_HEAD));   // Almanac still gated
	// paper HP from the board's effect counter
	CHECK(NewspaperAbove70PaperHp(73, 1200, 150) == 150);
	CHECK(NewspaperAbove70PaperHp(73, 1201, 150) == 120);
	CHECK(NewspaperAbove70PaperHp(73, 20000, 150) == 2000);
	CHECK(NewspaperAbove70PaperHp(73, 29999, 150) == 2999);
	CHECK(NewspaperAbove70PaperHp(73, 30000, 150) == 3000);
	CHECK(NewspaperAbove70PaperHp(73, 900000, 150) == 3000);
	CHECK(NewspaperAbove70PaperHp(70, 20000, 150) == 150);
	CHECK(NewspaperRollsInIZombie());
}

TEST_CASE("FootballHelmOnNormalNewspaperDancer") {
	GapRulesOn on;
	// which types run 0xc109bf: Normal only in I, Zombie (whatever the option), Newspaper / Dancer unless option 1
	CHECK(TakesFootballHelmCheck(ZOMBIE_NORMAL, true, eo::DEFAULT));
	CHECK(TakesFootballHelmCheck(ZOMBIE_NORMAL, true, eo::NONE));
	CHECK_FALSE(TakesFootballHelmCheck(ZOMBIE_NORMAL, false, eo::DEFAULT));
	CHECK(TakesFootballHelmCheck(ZOMBIE_NEWSPAPER, false, eo::DEFAULT));
	CHECK(TakesFootballHelmCheck(ZOMBIE_DANCER, true, eo::ALL));
	CHECK_FALSE(TakesFootballHelmCheck(ZOMBIE_NEWSPAPER, true, eo::NONE));
	CHECK_FALSE(TakesFootballHelmCheck(ZOMBIE_DANCER, false, eo::NONE));
	CHECK_FALSE(TakesFootballHelmCheck(ZOMBIE_TRAFFIC_CONE, true, eo::DEFAULT));
	CHECK(FootballHelmRollRange(true) == 3);
	CHECK(FootballHelmRollRange(false) == 0);
	CHECK(GetsFootballHelm(true, 0, 1, 1));
	CHECK_FALSE(GetsFootballHelm(true, 1, 73, 30));
	CHECK(GetsFootballHelm(false, -1, 71, 26));     // the board's level and wave, no roll
	CHECK_FALSE(GetsFootballHelm(false, -1, 71, 25));
	CHECK_FALSE(GetsFootballHelm(false, -1, 70, 30));
	CHECK(FootballHelmHp(0) == 2800);
	// I, Zombie Normal: Rand(7) + 1 / Rand(30) + 1, 1..3 -> coin
	CHECK(IZombieNormalCoinRollRange(false) == 7);
	CHECK(IZombieNormalCoinRollRange(true) == 30);
	CHECK(IZombieNormalIsCoin(0));
	CHECK(IZombieNormalIsCoin(2));
	CHECK_FALSE(IZombieNormalIsCoin(3));
	CHECK_FALSE(IZombieNormalIsCoin(29));
}

TEST_CASE("EliteGapsPackRulesMatchTheseValues") {
	GapRulesOn on;
	for (const auto& k : EliteGapsRuleKeys()) CHECK_MESSAGE(kGapRules.contains(k), k);
	CHECK(EliteGapsRuleKeys().size() == kGapRules.size());
	const char* home = std::getenv("HOME");
	fs::path dir = std::getenv("PVZU_PACK_DIR") ? fs::path(std::getenv("PVZU_PACK_DIR"))
	                                             : fs::path(home ? home : "") / "pvz-unnamed-pack";
	fs::path file = dir / "data/rules-elite-gaps.json";
	if (!fs::exists(file)) { MESSAGE("pack not here, skipped: " << file.string()); return; }
	std::ifstream in(file);
	nlohmann::json pack = nlohmann::json::parse(in, nullptr, false);
	REQUIRE(pack.is_object());
	CHECK(pack == kGapRules);
	std::ifstream mj(dir / "mod.json");
	nlohmann::json mod = nlohmann::json::parse(mj, nullptr, false);
	REQUIRE(mod.is_object());
	CHECK(mod["features"].value("eliteGaps", false));
}
