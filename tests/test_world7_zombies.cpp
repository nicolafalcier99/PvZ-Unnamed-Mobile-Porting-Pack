// PVZU: Plan 7 Task 2, world-7 zombie rules and the ZomBotany zombies in Unnamed (pure rules; evidence in
// the research notes (w6-w7-and-new-content) sections 2.4, 4, 4.1, 8 and pvz-unnamed-pack/SOURCES-world7-zombies.md).
#include "doctest/doctest.h"
#include "Mod/Unnamed/DayZombies.h"
#include "Mod/Unnamed/Options.h"
#include "Mod/Unnamed/Rules.h"
#include "Mod/Unnamed/World7Zombies.h"

#include "nlohmann/json.hpp"

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <random>
#include <set>

using namespace pvzu::unnamed;
namespace fs = std::filesystem;
namespace eo = pvzu::unnamed::eliteoption;

namespace {

// The Unnamed values (same as pvz-unnamed-pack/data/rules-world7-zombies.json; the last test compares the two).
const nlohmann::json kW7Rules = {
	{"world7Zombies.world7.firstLevel", 61},
	{"world7Zombies.world7.lastLevel", 70},
	{"world7Zombies.above60.level", 60},
	{"world7Zombies.flag.fillRows", 1},
	{"world7Zombies.flag.topRow", 4},
	{"world7Zombies.flag.topRowSixRows", 5},
	{"world7Zombies.flag.bucketHp", 1500},
	{"world7Zombies.imp.gargRoll", 3},
	{"world7Zombies.imp.gargHit", 1},
	{"world7Zombies.imp.gargHpBonus", 930},
	{"world7Zombies.football.blackExtraHit", 0},
	{"world7Zombies.football.blackBodyHp", 800},
	{"world7Zombies.football.blackHelmHp", 4500},
	{"world7Zombies.football.goldBlackBodyHp", 900},
	{"world7Zombies.football.goldBlackHelmHp", 2200},
	{"world7Zombies.catapult.vaseRollFrom", 50},
	{"world7Zombies.catapult.vaseRollBelow", 60},
	{"world7Zombies.catapult.vaseRollFromAllElite", 83},
	{"world7Zombies.catapult.vaseRollBelowAllElite", 100},
	{"world7Zombies.catapult.vaseBalls", 10},
	{"world7Zombies.catapult.shotRoll", 100},
	{"world7Zombies.catapult.vaseShotBelow", 35},
	{"world7Zombies.catapult.poleShotBelow", 50},
	{"world7Zombies.catapult.poleShotBallCap", 5},
	{"world7Zombies.jack.popMaxBodyHp", 370},
	{"world7Zombies.coin.footballGold", 1},
	{"world7Zombies.tree.minHeight", 100},
	{"world7Zombies.tree.maxExtraFeet", 200},
	{"world7Zombies.zombotany.peaHeadConeHp", 370},
	{"world7Zombies.zombotany.wallnutHelmHp", 1600},
	{"world7Zombies.zombotany.tallnutHelmHp", 2400},
	{"world7Zombies.zombotany.tallnutOddRoll", 12},
	{"world7Zombies.zombotany.jalapenoBodyHp", 870},
	{"world7Zombies.zombotany.jalapenoConeHp", 370},
	{"world7Zombies.zombotany.jalapenoDoorHp", 1100},
	{"world7Zombies.zombotany.gatlingBucketHp", 1100},
	{"world7Zombies.zombotany.squashBucketHp", 1100},
	{"world7Zombies.zombotany.squashDoorHp", 1100},
	{"world7Zombies.gatling.spreadRoll", 100},
	{"world7Zombies.gatling.spreadBelow", 50},
	{"world7Zombies.gatling.spreadVx", 2.82117343},
	{"world7Zombies.gatling.spreadVy", 1.78649282},
	{"world7Zombies.heads.hypnotisedFirePlayerPeas", 1},
	{"world7Zombies.jalapeno.burnsHypnotised", 1},
	{"world7Zombies.jalapeno.iceTimer", 20},
	{"world7Zombies.squash.jumpOnReach", 1},
	{"world7Zombies.squash.landingZombieDamage", 1800},
	{"world7Zombies.almanac.zombieCount", 33},
};

struct W7RulesOn {
	explicit W7RulesOn(nlohmann::json extra = nlohmann::json::object()) {
		nlohmann::json all = kW7Rules;
		for (auto it = extra.begin(); it != extra.end(); ++it)
			all[it.key()] = it.value();
		SetRules(all);
	}
	~W7RulesOn() { SetRules(nlohmann::json::object()); }
};

// Rand(n) of the exe, for rate checks
struct Roller {
	std::mt19937 rng{20261123};
	int operator()(int n) { return std::uniform_int_distribution<int>(0, n - 1)(rng); }
};

constexpr int kRolls = 2000;

} // namespace

TEST_CASE("World7VanillaWithoutRules") {
	SetRules(nlohmann::json::object());
	CHECK_FALSE(IsWorld7(true, 65));
	CHECK_FALSE(PlayerLevelAbove60(65));
	CHECK(RoadSignWorldNumber(true) == 0);
	CHECK_FALSE(FlagFillsRows());
	CHECK(ImpGargRollRange() == 0);
	CHECK_FALSE(FootballBecomesBlack(0, false, false, 65));
	CHECK(CatapultWorld7Form(55, 65) == 0);
	CHECK(CatapultShotRollRange(w7::VASE_CATAPULT, true) == 0);
	CHECK_FALSE(JackPopsWhenHit(true, ZOMBIE_JACK_IN_THE_BOX, 0xe, 100));
	CHECK_FALSE(CoinFootballIsGold());
	CHECK(TreeBuffedPlantHp(300, true, 1, 1, 65, 250) == 300);
	CHECK(ZomBotanyStats(ZOMBIE_WALLNUT_HEAD, 270, HELMTYPE_WALLNUT, 1100) == ZomBotanyArmour{270, HELMTYPE_WALLNUT, 1100, nullptr, 0});
	CHECK(ZomBotanyStats(ZOMBIE_PEA_HEAD, 270, HELMTYPE_NONE, 0) == ZomBotanyArmour{270, HELMTYPE_NONE, 0, nullptr, 0});
	CHECK(TallnutOddRollRange() == 0);
	CHECK(GatlingSpreadRollRange() == 0);
	CHECK_FALSE(HypnotisedHeadsFirePlayerPeas());
	CHECK_FALSE(JalapenoBurnsHypnotised());
	CHECK_FALSE(SquashJumpsOnReach());
	CHECK(AlmanacZombieCount(26) == 26);
	CHECK(ZomBotanyNameKey(ZOMBIE_PEA_HEAD) == nullptr);
	CHECK(AlmanacZombieAt(25, 26) == ZOMBIE_BOSS);
	CHECK(AlmanacZombieSlot(3, 26) == AlmanacSlot{3 * 85 + 22, 86, 1.0f});
}

TEST_CASE("World7GateReadsTheProfileNotTheBoard") {
	W7RulesOn on;
	// exe 0x7aa4c0: mGameMode == 0, PlayerInfo != 0, 61 <= PlayerInfo+0x24 <= 70
	for (int lvl = 61; lvl <= 70; ++lvl) CHECK_MESSAGE(IsWorld7(true, lvl), lvl);
	for (int lvl : {0, 1, 41, 50, 60, 71, 72, 73}) CHECK_MESSAGE(!IsWorld7(true, lvl), lvl);
	CHECK_FALSE(IsWorld7(false, 65));   // a minigame with a world-7 profile
	// the board level never counts: a replayed 5-1 (board 41) with a world-7 profile is world 7, a board at 65 with a
	// profile at 41 is not
	CHECK(IsWorld7(true, 65, 41));
	CHECK_FALSE(IsWorld7(true, 41, 65));
	// "profile level > 60": also 71-73 and any mode
	CHECK(PlayerLevelAbove60(61));
	CHECK(PlayerLevelAbove60(73));
	CHECK_FALSE(PlayerLevelAbove60(60));
	CHECK(PlayerLevelAbove60(61, 1));
	CHECK_FALSE(PlayerLevelAbove60(60, 70));
	CHECK(RoadSignWorldNumber(true) == 7);
	CHECK(RoadSignWorldNumber(false) == 0);
}

TEST_CASE("World7PoleRoadSignRate") {
	W7RulesOn on({{"dayZombies.pole.rollRange", 22}, {"dayZombies.pole.gloveMaxRoll", 2},
	              {"dayZombies.pole.roadSignMaxRoll", 9}, {"dayZombies.pole.vaseMaxRoll", 12},
	              {"dayZombies.pole.roadSignWorld", 7}});
	Roller rand;
	int inW7 = 0, outside = 0;
	for (int i = 0; i < kRolls; ++i) {
		const int r = rand(EliteRollRange(ZOMBIE_POLEVAULTER));
		inW7 += RollEliteVariant(ZOMBIE_POLEVAULTER, r, RoadSignWorldNumber(IsWorld7(true, 65, 41))) == elite::ROAD_SIGN;
		outside += RollEliteVariant(ZOMBIE_POLEVAULTER, r, RoadSignWorldNumber(IsWorld7(true, 41, 65))) == elite::ROAD_SIGN;
	}
	CHECK(outside == 0);
	CHECK(inW7 / double(kRolls) == doctest::Approx(7.0 / 22).epsilon(0.12));   // rolls 3..9 of Rand(22)
}

TEST_CASE("World7FlagFillsEveryRow") {
	W7RulesOn on;
	CHECK(FlagFillsRows());
	CHECK(FlagExtraRows(2, false) == std::vector<int>{4, 3, 1, 0});
	CHECK(FlagExtraRows(0, false) == std::vector<int>{4, 3, 2, 1});
	CHECK(FlagExtraRows(5, true) == std::vector<int>{4, 3, 2, 1, 0});
	CHECK(FlagExtraRows(3, true) == std::vector<int>{5, 4, 2, 1, 0});
	CHECK(FlagBucketHp() == 1500);
}

TEST_CASE("World7ImpBecomesGargantuarOneInThree") {
	W7RulesOn on;
	CHECK(ImpGargRollRange() == 3);
	CHECK(ImpBecomesGarg(1));
	CHECK_FALSE(ImpBecomesGarg(0));
	CHECK_FALSE(ImpBecomesGarg(2));
	CHECK(ImpGargHpBonus() == 930);   // 270 + 930 = 1200 (the mod Almanac says 1100)
	Roller rand;
	int hits = 0;
	for (int i = 0; i < kRolls; ++i) hits += ImpBecomesGarg(rand(ImpGargRollRange()));
	CHECK(hits / double(kRolls) == doctest::Approx(1.0 / 3).epsilon(0.1));
}

TEST_CASE("World7BlackFootball") {
	W7RulesOn on;
	CHECK(FootballBecomesBlack(0, false, false, 61));
	CHECK_FALSE(FootballBecomesBlack(0, false, false, 60));   // world 2 draws the roll and ignores it
	CHECK_FALSE(FootballBecomesBlack(1, false, false, 65));
	CHECK_FALSE(FootballBecomesBlack(0, true, false, 65));    // green keeps its armour
	CHECK_FALSE(FootballBecomesBlack(0, false, true, 65));    // gold never draws the roll
	CHECK(BlackFootballBodyHp(270) == 800);
	CHECK(BlackFootballHelmHp(2800) == 4500);
	// the Night chain: Rand(4) green, Rand(13) gold, Rand(3) unless gold -> black 3/13 at profile level > 60
	Roller rand;
	int black = 0, blackAt60 = 0;
	for (int i = 0; i < kRolls; ++i) {
		const bool green = rand(4) == 0;
		const bool gold = rand(13) == 0;
		const int extra = gold ? -1 : rand(3);
		black += FootballBecomesBlack(extra, green, gold, 65);
		blackAt60 += FootballBecomesBlack(extra, green, gold, 60);
	}
	CHECK(blackAt60 == 0);
	CHECK(black / double(kRolls) == doctest::Approx(3.0 / 13).epsilon(0.12));
	const auto art = BlackFootballArt();
	REQUIRE(art.size() == 8);
	CHECK(art[0] == W7ArtImage{"zombie_football_helmet", "images/Black_football_helmet"});
	CHECK(art[7] == W7ArtImage{"zombie_football_upperbody3", "images/Black_football_upperbody3"});
	CHECK(BlackHelmImage(1) == "images/Black_football_helmet2");
	CHECK(BlackHelmImage(2) == "images/Black_football_helmet3");
	CHECK(BlackHelmImage(0).empty());
}

TEST_CASE("World7CatapultVase") {
	W7RulesOn on;
	CHECK(CatapultWorld7Form(50, 61) == w7::VASE_CATAPULT);
	CHECK(CatapultWorld7Form(59, 61) == w7::VASE_CATAPULT);
	CHECK(CatapultWorld7Form(49, 61) == 0);   // the pole head (RoofZombies)
	CHECK(CatapultWorld7Form(60, 61) == 0);
	CHECK(CatapultWorld7Form(55, 60) == 0);
	CHECK(VaseCatapultBalls(20) == 10);
	CHECK(VaseCatapultArt().size() == 4);
	CHECK(VaseCatapultArt()[3] == W7ArtImage{"Zombie_catapult_basketball4", "images/bug_spray"});
	Roller rand;
	int vases = 0;
	for (int i = 0; i < kRolls; ++i) vases += CatapultWorld7Form(rand(100), 65) == w7::VASE_CATAPULT;
	CHECK(vases / double(kRolls) == doctest::Approx(0.10).epsilon(0.2));
	// shots
	CHECK(CatapultShotRollRange(w7::VASE_CATAPULT, false) == 100);   // the vase catapult: any level
	CHECK(CatapultShotRollRange(0x19, true) == 100);                 // pole head: world 7 / the boss level only
	CHECK(CatapultShotRollRange(0x19, false) == 0);
	CHECK(CatapultShotRollRange(0, true) == 0);
	CHECK(RollCatapultShot(w7::VASE_CATAPULT, 34, 10) == CatapultShot{0x20, 10});
	CHECK(RollCatapultShot(w7::VASE_CATAPULT, 35, 10) == CatapultShot{9, 10});
	CHECK(RollCatapultShot(0x19, 49, 20) == CatapultShot{0x12, 5});
	CHECK(RollCatapultShot(0x19, 49, 3) == CatapultShot{0x12, 3});
	CHECK(RollCatapultShot(0x19, 50, 20) == CatapultShot{9, 20});
	int vaseShots = 0, poleShots = 0;
	for (int i = 0; i < kRolls; ++i) {
		vaseShots += RollCatapultShot(w7::VASE_CATAPULT, rand(100), 10).exeProjectile == 0x20;
		poleShots += RollCatapultShot(0x19, rand(100), 10).exeProjectile == 0x12;
	}
	CHECK(vaseShots / double(kRolls) == doctest::Approx(0.35).epsilon(0.1));
	CHECK(poleShots / double(kRolls) == doctest::Approx(0.50).epsilon(0.1));
}

// The elite option "all" as a setting (0x7b2000; pvz-unnamed-pack SOURCES-shop-ui.md): the Imp's jump at 0x7a1f53 becomes
// `jo` (Rand(3) drawn, always a Gargantuar); the Catapult vase range moves to 83..99 (0x796e16 = 83, 0x910002 = 100).
TEST_CASE("World7AllElitesOver2000Rolls") {
	W7RulesOn on;
	std::mt19937 gen(777);
	auto roll = [&](int n) { return std::uniform_int_distribution<int>(0, n - 1)(gen); };
	const int N = 2000;
	CHECK(ImpGargRollRange(eo::ALL) == 3);
	CHECK(ImpGargRollRange(eo::NONE) == 0);
	int garg = 0;
	for (int i = 0; i < N; ++i) garg += ImpBecomesGarg(roll(ImpGargRollRange(eo::ALL)), eo::ALL) ? 1 : 0;
	CHECK(garg == N);
	int vase = 0;
	for (int i = 0; i < N; ++i) vase += CatapultWorld7Form(roll(100), 61, eo::ALL) == w7::VASE_CATAPULT ? 1 : 0;
	CHECK(std::abs(vase - N * 17.0 / 100) <= 4 * std::sqrt(N * 0.17 * 0.83));
	CHECK(CatapultWorld7Form(82, 61, eo::ALL) == 0);
	CHECK(CatapultWorld7Form(83, 61, eo::ALL) == w7::VASE_CATAPULT);
	CHECK(CatapultWorld7Form(99, 61, eo::ALL) == w7::VASE_CATAPULT);
	CHECK(CatapultWorld7Form(99, 60, eo::ALL) == 0);
	CHECK(CatapultWorld7Form(55, 61, eo::NONE) == 0);
	CHECK(GoldBlackFootballBodyHp(540) == 900);
	CHECK(GoldBlackFootballHelmHp(1260) == 2200);
	// option 0 through the new parameters = the calls without them
	CHECK(ImpGargRollRange(eo::DEFAULT) == ImpGargRollRange());
	for (int r = 0; r < 100; ++r) {
		CHECK(ImpBecomesGarg(r % 3, eo::DEFAULT) == ImpBecomesGarg(r % 3));
		for (int level : {60, 61, 71}) CHECK(CatapultWorld7Form(r, level, eo::DEFAULT) == CatapultWorld7Form(r, level));
	}
}

TEST_CASE("World7JackPopsBelow371") {
	W7RulesOn on;
	CHECK(JackPopsWhenHit(true, ZOMBIE_JACK_IN_THE_BOX, 0xe, 370));
	CHECK(JackPopsWhenHit(true, ZOMBIE_JACK_IN_THE_BOX, 0xf, 1));
	CHECK_FALSE(JackPopsWhenHit(true, ZOMBIE_JACK_IN_THE_BOX, 0xe, 371));
	CHECK_FALSE(JackPopsWhenHit(true, ZOMBIE_JACK_IN_THE_BOX, 0, 100));   // `+0x134 != -1`: elite Jacks only
	CHECK_FALSE(JackPopsWhenHit(false, ZOMBIE_JACK_IN_THE_BOX, 0xe, 100));
	CHECK_FALSE(JackPopsWhenHit(true, ZOMBIE_NORMAL, 0xe, 100));
	CHECK(CoinFootballIsGold());
}

TEST_CASE("World7TreeOfWisdomPlantHp") {
	W7RulesOn on;
	CHECK(TreeBuffedPlantHp(300, true, 1, 0, 61, 150) == 450);    // x1.5
	CHECK(TreeBuffedPlantHp(300, true, 1, 0, 61, 300) == 900);    // x3 cap (200 ft over 100)
	CHECK(TreeBuffedPlantHp(300, true, 1, 0, 61, 900) == 900);
	CHECK(TreeBuffedPlantHp(4000, true, 1, 1, 5, 101) == 4040);   // adventure finished once
	CHECK(TreeBuffedPlantHp(300, true, 1, 0, 61, 100) == 300);    // must be > 100 ft
	CHECK(TreeBuffedPlantHp(300, true, 1, 0, 60, 300) == 300);    // first run below 61
	CHECK(TreeBuffedPlantHp(300, false, 1, 1, 65, 300) == 300);   // not adventure
	CHECK(TreeBuffedPlantHp(300, true, 0, 1, 65, 300) == 300);    // no tree
	CHECK(TreeBuffedPlantHp(300, true, 2, 1, 65, 300) == 300);    // `cmp byte, 1`
	CHECK(TreeBuffedPlantHp(25, true, 1, 1, 65, 133) == 33);      // 25 * 1.33 = 33.25 -> 33 (fistp, nearest)
	CHECK(TreeBuffedPlantHp(30, true, 1, 1, 65, 105) == 32);      // 31.5 -> 32 (round half to even)
}

TEST_CASE("ZomBotanyStatsInUnnamed") {
	W7RulesOn on;
	using A = ZomBotanyArmour;
	CHECK(ZomBotanyStats(ZOMBIE_PEA_HEAD, 270, HELMTYPE_NONE, 0) == A{270, HELMTYPE_TRAFFIC_CONE, 370, "anim_cone", 0});
	CHECK(ZomBotanyStats(ZOMBIE_WALLNUT_HEAD, 270, HELMTYPE_WALLNUT, 1100) == A{270, HELMTYPE_WALLNUT, 1600, nullptr, 0});
	CHECK(ZomBotanyStats(ZOMBIE_TALLNUT_HEAD, 270, HELMTYPE_TALLNUT, 2200) == A{270, HELMTYPE_TALLNUT, 2400, nullptr, 0});
	CHECK(ZomBotanyStats(ZOMBIE_JALAPENO_HEAD, 500, HELMTYPE_NONE, 0) == A{870, HELMTYPE_TRAFFIC_CONE, 370, "anim_cone", 1100});
	CHECK(ZomBotanyStats(ZOMBIE_GATLING_HEAD, 270, HELMTYPE_NONE, 0) == A{270, HELMTYPE_PAIL, 1100, "anim_bucket", 0});
	CHECK(ZomBotanyStats(ZOMBIE_SQUASH_HEAD, 270, HELMTYPE_NONE, 0) == A{270, HELMTYPE_PAIL, 1100, "anim_bucket", 1100});
	// totals against the mod Almanac: 640, 1870, 2340, 2470 (Gatling "high", Tall-nut "extremely high")
	auto total = [](const A& a) { return a.bodyHp + a.helmHp + a.doorHp; };
	CHECK(total(ZomBotanyStats(ZOMBIE_PEA_HEAD, 270, HELMTYPE_NONE, 0)) == 640);
	CHECK(total(ZomBotanyStats(ZOMBIE_WALLNUT_HEAD, 270, HELMTYPE_WALLNUT, 1100)) == 1870);
	CHECK(total(ZomBotanyStats(ZOMBIE_JALAPENO_HEAD, 500, HELMTYPE_NONE, 0)) == 2340);
	CHECK(total(ZomBotanyStats(ZOMBIE_SQUASH_HEAD, 270, HELMTYPE_NONE, 0)) == 2470);
	// not ZomBotany: unchanged
	CHECK(ZomBotanyStats(ZOMBIE_NORMAL, 270, HELMTYPE_NONE, 0) == A{270, HELMTYPE_NONE, 0, nullptr, 0});
	CHECK(ZomBotanyStats(ZOMBIE_REDEYE_GARGANTUAR, 6000, HELMTYPE_NONE, 0) == A{6000, HELMTYPE_NONE, 0, nullptr, 0});
	for (int t = ZOMBIE_PEA_HEAD; t <= ZOMBIE_TALLNUT_HEAD; ++t) CHECK(IsZomBotanyType(static_cast<ZombieType>(t)));
	CHECK_FALSE(IsZomBotanyType(ZOMBIE_REDEYE_GARGANTUAR));
	// the Tall-nut head's 1-in-12 (hook 508, after `mov [eax+0xd0], 0x960`; the study table put it on the Jalapeno row)
	CHECK(TallnutOddRollRange() == 12);
	CHECK(RollTallnutOdd(0) == w7::TALLNUT_ODD);
	CHECK(RollTallnutOdd(1) == 0);
	Roller rand;
	int odd = 0;
	for (int i = 0; i < kRolls; ++i) odd += RollTallnutOdd(rand(TallnutOddRollRange())) == w7::TALLNUT_ODD;
	CHECK(odd / double(kRolls) == doctest::Approx(1.0 / 12).epsilon(0.25));
}

TEST_CASE("ZomBotanyBehaviour") {
	W7RulesOn on;
	CHECK(GatlingSpreadRollRange() == 100);
	CHECK(GatlingSpreads(49));
	CHECK_FALSE(GatlingSpreads(50));
	CHECK(GatlingSpreadVx() == doctest::Approx(2.82117f));
	CHECK(GatlingSpreadVy() == doctest::Approx(1.78649f));
	Roller rand;
	int spreads = 0;
	for (int i = 0; i < kRolls; ++i) spreads += GatlingSpreads(rand(GatlingSpreadRollRange()));
	CHECK(spreads / double(kRolls) == doctest::Approx(0.5).epsilon(0.08));
	CHECK(HypnotisedHeadsFirePlayerPeas());
	CHECK(JalapenoBurnsHypnotised());
	CHECK(JalapenoIceTimer() == 20);
	CHECK(SquashJumpsOnReach());
	CHECK(SquashLandingZombieDamage() == 1800);
}

TEST_CASE("World7FormNames") {
	CHECK(std::string(World7FormName(ZOMBIE_GARGANTUAR, w7::IMP_GARGANTUAR, 0)) == "imp-gargantuar");
	CHECK(std::string(World7FormName(ZOMBIE_CATAPULT, w7::VASE_CATAPULT, 0)) == "vase");
	CHECK(std::string(World7FormName(ZOMBIE_FOOTBALL, 0, w7::BLACK_FOOTBALL)) == "black");
	CHECK(std::string(World7FormName(ZOMBIE_TALLNUT_HEAD, w7::TALLNUT_ODD, 0)) == "odd-0x101");
	CHECK(World7FormName(ZOMBIE_GARGANTUAR, 0x106, 0) == nullptr);   // the staff: RoofEliteName
	CHECK(World7FormName(ZOMBIE_FOOTBALL, 0, 0x1a) == nullptr);       // green: NightEliteName
}

TEST_CASE("World7AlmanacThirtyThreeZombies") {
	W7RulesOn on;
	CHECK(AlmanacZombieCount(26) == 33);
	CHECK(std::string(ZomBotanyNameKey(ZOMBIE_PEA_HEAD)) == "PEASHOOTER_ZOMBIE");
	CHECK(std::string(ZomBotanyNameKey(ZOMBIE_WALLNUT_HEAD)) == "WALLNUT_ZOMBIE");
	CHECK(std::string(ZomBotanyNameKey(ZOMBIE_JALAPENO_HEAD)) == "JALAPENO_ZOMBIE");
	CHECK(std::string(ZomBotanyNameKey(ZOMBIE_GATLING_HEAD)) == "GATLING_PEA_ZOMBIE");
	CHECK(std::string(ZomBotanyNameKey(ZOMBIE_SQUASH_HEAD)) == "SQUASH_ZOMBIE");
	CHECK(std::string(ZomBotanyNameKey(ZOMBIE_TALLNUT_HEAD)) == "TALLNUT_ZOMBIE");
	CHECK(ZomBotanyNameKey(ZOMBIE_REDEYE_GARGANTUAR) == nullptr);   // keeps REDEYED_GARGANTUAR (hooks 9 / 10 skip 32)
	CHECK(ZomBotanyNameKey(ZOMBIE_NORMAL) == nullptr);
	// every type once, the Zomboss last
	std::set<int> seen;
	for (int i = 0; i < 33; ++i) seen.insert(AlmanacZombieAt(i, 33));
	CHECK(seen.size() == 33);
	CHECK(AlmanacZombieAt(24, 33) == ZOMBIE_IMP);
	CHECK(AlmanacZombieAt(25, 33) == ZOMBIE_PEA_HEAD);
	CHECK(AlmanacZombieAt(31, 33) == ZOMBIE_REDEYE_GARGANTUAR);
	CHECK(AlmanacZombieAt(32, 33) == ZOMBIE_BOSS);
	CHECK(AlmanacZombieAt(33, 33) == ZOMBIE_INVALID);
	// the 6 x 6 grid stays left of the zombie card (x 455) and above the bottom buttons (y 567), windows never overlap
	for (int i = 0; i < 33; ++i) {
		const AlmanacSlot s = AlmanacZombieSlot(i, 33);
		const int size = static_cast<int>(76 * s.scale + 0.5f);
		CHECK_MESSAGE(s.x >= 0, i);
		CHECK_MESSAGE(s.x + size <= 450, i);
		CHECK_MESSAGE(s.y >= 80, i);
		CHECK_MESSAGE(s.y + size <= 560, i);
		for (int j = 0; j < i; ++j) {
			const AlmanacSlot o = AlmanacZombieSlot(j, 33);
			const bool apart = o.x + size <= s.x || s.x + size <= o.x || o.y + size <= s.y || s.y + size <= o.y;
			CHECK_MESSAGE(apart, i << " vs " << j);
		}
	}
	// vanilla layout when the count is vanilla
	CHECK(AlmanacZombieSlot(25, 26) == AlmanacSlot{192, 486, 1.0f});
	CHECK(AlmanacZombieSlot(7, 26) == AlmanacSlot{2 * 85 + 22, 1 * 80 + 86, 1.0f});
}

TEST_CASE("World7PackRulesMatchTheseValues") {
	W7RulesOn on;
	for (const auto& k : World7ZombieRuleKeys()) CHECK_MESSAGE(kW7Rules.contains(k), k);
	CHECK(World7ZombieRuleKeys().size() == kW7Rules.size());
	const char* home = std::getenv("HOME");
	fs::path dir = std::getenv("PVZU_PACK_DIR") ? fs::path(std::getenv("PVZU_PACK_DIR"))
	                                             : fs::path(home ? home : "") / "pvz-unnamed-pack";
	fs::path file = dir / "data/rules-world7-zombies.json";
	if (!fs::exists(file)) { MESSAGE("pack not here, skipped: " << file.string()); return; }
	std::ifstream in(file);
	nlohmann::json pack = nlohmann::json::parse(in, nullptr, false);
	REQUIRE(pack.is_object());
	CHECK(pack == kW7Rules);
	// mod.json switches the feature on
	std::ifstream mj(dir / "mod.json");
	nlohmann::json mod = nlohmann::json::parse(mj, nullptr, false);
	REQUIRE(mod.is_object());
	CHECK(mod["features"].value("world7Zombies", false));
	// the English keys of the ZomBotany names exist with a description
	std::ifstream en(dir / "strings/en-corrections-public.txt");
	REQUIRE(en.good());
	const std::string text((std::istreambuf_iterator<char>(en)), std::istreambuf_iterator<char>());
	for (int t = ZOMBIE_PEA_HEAD; t <= ZOMBIE_TALLNUT_HEAD; ++t) {
		const std::string key = ZomBotanyNameKey(static_cast<ZombieType>(t));
		CHECK_MESSAGE(text.find("[" + key + "]") != std::string::npos, key);
		CHECK_MESSAGE(text.find("[" + key + "_DESCRIPTION]") != std::string::npos, key);
	}
}
