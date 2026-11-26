// PVZU: Plan 7 Task 3b, the 7-10 boss (giant Newspaper Zombie). Pure rules; evidence in
// the research notes (w6-w7-and-new-content) section 3 and pvz-unnamed-pack/SOURCES-level70-boss.md.
#include "doctest/doctest.h"
#include "Mod/Unnamed/Level70Boss.h"
#include "Mod/Unnamed/Rules.h"
#include "Mod/Unnamed/State.h"

#include "nlohmann/json.hpp"

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <random>
#include <set>

using namespace pvzu::unnamed;
namespace fs = std::filesystem;

namespace {

// The Unnamed values (same as pvz-unnamed-pack/data/rules-level70-boss.json; the last test compares the two).
const nlohmann::json kBoss70Rules = {
	{"level70Boss.level", 70},
	{"level70Boss.triggerWave", 10},
	{"level70Boss.row", 2},
	{"level70Boss.col", 8},
	{"level70Boss.scale", 3.0},
	{"level70Boss.hp", 150000},
	{"level70Boss.firstAttackCs", 750},
	{"level70Boss.rectWidthFactor", 3},
	{"level70Boss.rectHeightFactor", 2},
	{"level70Boss.pinX", 640.0},
	{"level70Boss.bossClassMaxHpAbove", 120000},
	{"level70Boss.reinforcementRoll", 33},
	{"level70Boss.reinforcementExclude", {11, 13, 14, 19, 25}},
	{"level70Boss.reinforcementRows", 5},
	{"level70Boss.reinforcementCol", 8},
	{"level70Boss.bankMax", 10},
	{"level70Boss.darkBank", {51, 115, 27, 30, 14, 39, 23, 15, 29, 115}},
	{"level70Boss.lightBank", {25, 11, 27, 93, 14, 39, 23, 2, 18, 115}},
	{"level70Boss.standInFrom", {115, 93, 89}},
	{"level70Boss.standInTo", {35, 13, 9}},
	{"level70Boss.lightRemoveMax", 4},
	{"level70Boss.shieldBelowThirds", 3},
	{"level70Boss.shieldHp", 1},
	{"level70Boss.phase2Hp", 4500},
	{"level70Boss.phase2Sun", 1500},
	{"level70Boss.phase3Hp", 1500},
	{"level70Boss.meterWidth", 150},
	{"level70Boss.attackRollReading", 4},
	{"level70Boss.attackRollMad", 7},
	{"level70Boss.timerRoll", 300},
	{"level70Boss.timerBase", 1200},
	{"level70Boss.rowRoll", 5},
	{"level70Boss.columnTypes", {6, 8, 12, 21, 29}},
	{"level70Boss.columnScale", 1.5},
	{"level70Boss.columnHpFactor", 4},
	{"level70Boss.columnFadeCs", 400},
	{"level70Boss.columnDoorBucketHp", 4400},
	{"level70Boss.rayGunPeriod", 50},
	{"level70Boss.rayGunFireAt", {9, 18, 26, 34}},
	{"level70Boss.rayGunAngleRate", 0.031415924},
	{"level70Boss.rayGunAngleMax", 1.0471976},
	{"level70Boss.rayGunSpeedX", -1.0},
	{"level70Boss.rayGunDx", -24},
	{"level70Boss.rayGunDy", 24},
	{"level70Boss.rayGunProjectile", 3},
	{"level70Boss.drainPlantsCap", 50},
	{"level70Boss.drainScale", 0.080078125},
	{"level70Boss.drainHealShift", 4},
	{"level70Boss.drainHealCap", 1800},
	{"level70Boss.drainFlashCs", 25},
	{"level70Boss.eatRows", 1},
	{"level70Boss.eatReachPx", 80},
	{"level70Boss.eatHealShift", 3},
	{"level70Boss.eatHealCap", 150},
	{"level70Boss.eatFreezePerPlant", 10},
	{"level70Boss.madPacketSeed", 89},
	{"level70Boss.bungeeRows", {0, 0, 0, 1, 2, 2, 2, 2, 3, 4, 4, 4}},
	{"level70Boss.bungeeCols", {1, 3, 5, 3, 1, 2, 4, 5, 3, 1, 3, 5}},
	{"level70Boss.graveFirstCol", 7},
	{"level70Boss.graveLastCol", 8},
	{"level70Boss.sproutSeed", 51},
	{"level70Boss.sproutRoll", 48},
	{"level70Boss.longButterCs", 30000},
	{"level70Boss.headAnimRate", 76},
	{"level70Boss.textCs", 250},
};

struct Boss70RulesOn {
	Boss70RulesOn() { SetRules(kBoss70Rules); }
	~Boss70RulesOn() { SetRules(nlohmann::json::object()); }
};

constexpr int kEngineSeeds = 53; // NUM_SEED_TYPES (0..52): the engine has no seed >= 0x50 yet
constexpr int kZombieBoss = 25, kZombieNewspaper = 5, kZombieGargantuar = 23;
constexpr int kPhaseNormal = 0, kPhaseDying = 1, kPhaseReading = 29, kPhaseMaddening = 30, kPhaseMad = 31;

} // namespace

TEST_CASE("Boss70VanillaWithoutRules") {
	SetRules(nlohmann::json::object());
	CHECK(Boss70Level() == 0);
	CHECK_FALSE(Boss70IsLevel(70));
	CHECK_FALSE(Boss70Triggers(70, 9));
	CHECK_FALSE(IsBoss70(kZombieNewspaper, boss70::kVariant, 70));
	CHECK(Boss70ProgressWidth(false, false, 0, 0, 3) == -1);
	CHECK(Boss70BeforeHit(kPhaseReading, 1, 150000) == Boss70Phase{});
	CHECK(Boss70Bank(Boss70Light::DARK, 10, kEngineSeeds).empty());
	CHECK_FALSE(Boss70SproutRandom(51, -1));
	CHECK_FALSE(Boss70ButterTick(40000, 150000, kZombieNewspaper, kPhaseMad).cut);
	CHECK(Boss70HitRect({36, 0, 42, 115}) == Boss70Rect{36, 0, 42, 115});
	CHECK_FALSE(IsBossClass(150000, kZombieNewspaper)); // the 120000 line is a rule too
}

TEST_CASE("Boss70TriggerAndSpawn") {
	// 0x7a07a0: level 70 (Board::mLevel, any mode) and mCurrentWave == 10 right after the spawn = the 10th wave sent
	Boss70RulesOn on;
	CHECK(Boss70Level() == 70);
	CHECK(Boss70IsLevel(70));
	CHECK_FALSE(Boss70IsLevel(69));
	CHECK(Boss70Triggers(70, 9));
	CHECK_FALSE(Boss70Triggers(70, 8));
	CHECK_FALSE(Boss70Triggers(70, 10)); // a replay with 20 waves still meets it once, at the 10th
	CHECK_FALSE(Boss70Triggers(60, 9));
	const Boss70Spawn s = Boss70SpawnRules();
	CHECK(s.row == 2);
	CHECK(s.col == 8);
	CHECK(s.scale == doctest::Approx(3.0f));
	CHECK(s.hp == 150000);
	CHECK(s.firstAttackCs == 750);
	// 0x7a0868: width x3; height += height; y -= the new height's added part (the old height)
	CHECK(Boss70HitRect({36, 0, 42, 115}) == Boss70Rect{36, -115, 126, 230});
	CHECK(Boss70HitRect({36, 17, 42, 115}) == Boss70Rect{36, -98, 126, 230});
	CHECK(Boss70PinX() == doctest::Approx(640.0f));
	CHECK(IsBoss70(kZombieNewspaper, boss70::kVariant, 70));
	CHECK_FALSE(IsBoss70(kZombieNewspaper, 0, 70));
	CHECK_FALSE(IsBoss70(kZombieNewspaper, boss70::kVariant, 69));
	CHECK_FALSE(IsBoss70(kZombieBoss, boss70::kVariant, 70));
}

TEST_CASE("Boss70BossClass") {
	// 0x7aa320: max HP > 120000 (0x1d4c0) or type 0x19
	Boss70RulesOn on;
	CHECK(IsBossClass(120001, kZombieNewspaper));
	CHECK_FALSE(IsBossClass(120000, kZombieNewspaper));
	CHECK(IsBossClass(50000, kZombieBoss));
	CHECK_FALSE(IsBossClass(3000, kZombieGargantuar));
	// hook 631: 30000+ cs of butter is cut on a boss-class zombie, hypnotises a maddening Newspaper, else counts down
	CHECK(Boss70ButterTick(30000, 150000, kZombieNewspaper, kPhaseMad).cut);
	CHECK_FALSE(Boss70ButterTick(30000, 150000, kZombieNewspaper, kPhaseMad).hypnotise);
	CHECK_FALSE(Boss70ButterTick(29999, 150000, kZombieNewspaper, kPhaseMad).cut);
	CHECK_FALSE(Boss70ButterTick(40000, 4500, kZombieNewspaper, kPhaseMad).cut);
	const Boss70Butter b = Boss70ButterTick(40000, 340, kZombieNewspaper, kPhaseMaddening);
	CHECK(b.cut);
	CHECK(b.hypnotise);
	CHECK_FALSE(Boss70ButterTick(40000, 270, 0, kPhaseNormal).cut);
}

TEST_CASE("Boss70Reinforcements") {
	// 0x7aa7d0: Rand(33) re-rolled on 11 Snorkel, 13 Bobsled, 14 Dolphin, 19 Yeti, 25 Boss; row Rand(5), column 8
	Boss70RulesOn on;
	CHECK(Boss70ReinforcementRollRange() == 33);
	for (int t : {11, 13, 14, 19, 25})
		CHECK_FALSE(Boss70ReinforcementAccepted(t));
	for (int t : {0, 5, 10, 12, 20, 23, 24, 26, 32})
		CHECK(Boss70ReinforcementAccepted(t));
	CHECK_FALSE(Boss70ReinforcementAccepted(33));
	CHECK(Boss70ReinforcementRows() == 5);
	CHECK(Boss70ReinforcementCol() == 8);
	std::mt19937 rng(70);
	std::set<int> seen;
	for (int i = 0; i < 3000; ++i) {
		const int t = PickBoss70Reinforcement([&](int n) { return static_cast<int>(rng() % static_cast<unsigned>(n)); });
		CHECK(Boss70ReinforcementAccepted(t));
		seen.insert(t);
	}
	CHECK(seen.size() == 28);
}

TEST_CASE("Boss70Banks") {
	// 0x7aa823 / 0x7aa8d0: the pushes, first argument first; 0x73 Strong Coffee Bean, 0x5d Prism-shroom and 0x59
	// Golden Sun-shroom are mod plants the engine lacks: vanilla Coffee Bean, Scaredy-shroom, Sun-shroom stand in
	Boss70RulesOn on;
	CHECK(Boss70Bank(Boss70Light::DARK, 10, kEngineSeeds) == std::vector<int>{51, 35, 27, 30, 14, 39, 23, 15, 29, 35});
	CHECK(Boss70Bank(Boss70Light::DAY, 10, kEngineSeeds) == std::vector<int>{25, 11, 27, 13, 14, 39, 23, 2, 18, 35});
	CHECK(Boss70Bank(Boss70Light::DARK, 6, kEngineSeeds) == std::vector<int>{51, 35, 27, 30, 14, 39}); // min(bank, 10)
	CHECK(Boss70Bank(Boss70Light::DARK, 12, kEngineSeeds).size() == 10);
	// with mod plants in the engine the exe's own seeds are used
	CHECK(Boss70Bank(Boss70Light::DARK, 2, 128) == std::vector<int>{51, 115});
	CHECK(Boss70StandIn(115, kEngineSeeds) == 35);
	CHECK(Boss70StandIn(100, kEngineSeeds) == -1);
	CHECK(Boss70StandIn(14, kEngineSeeds) == 14);
	CHECK(Boss70MadPacketSeed(kEngineSeeds) == 9);
	CHECK(Boss70MadPacketSeed(128) == 89);
	// light: elites (variant 1..0x7e, not boss class) each empty one packet, at most 4 and the bank size
	CHECK(Boss70CountsAsElite(1, false));
	CHECK(Boss70CountsAsElite(0x7e, false));
	CHECK_FALSE(Boss70CountsAsElite(0, false));
	CHECK_FALSE(Boss70CountsAsElite(0x7f, false));
	CHECK_FALSE(Boss70CountsAsElite(0x105, false));
	CHECK_FALSE(Boss70CountsAsElite(5, true));
	CHECK(Boss70LightRemovals(0, 10) == 0);
	CHECK(Boss70LightRemovals(3, 10) == 3);
	CHECK(Boss70LightRemovals(9, 10) == 4);
	CHECK(Boss70LightRemovals(9, 2) == 2);
	// light switch (0x7aa680 with edi 0 / 1): dark = day plants asleep, mushrooms awake; day = the other way
	CHECK(Boss70Background(Boss70Light::DARK) == 1);
	CHECK(Boss70Background(Boss70Light::DAY) == 0);
	CHECK(Boss70PlantSleeps(Boss70Light::DARK, false));
	CHECK_FALSE(Boss70PlantSleeps(Boss70Light::DARK, true));
	CHECK(Boss70PlantSleeps(Boss70Light::DAY, true));
	CHECK_FALSE(Boss70PlantSleeps(Boss70Light::DAY, false));
	// 0x7a071a: a Sprout card (or an Imitater of one) plants an Imitater of Rand(48)
	CHECK(Boss70SproutRandom(51, -1));
	CHECK(Boss70SproutRandom(48, 51));
	CHECK_FALSE(Boss70SproutRandom(48, 3));
	CHECK_FALSE(Boss70SproutRandom(0, -1));
	CHECK(Boss70SproutRollRange() == 48);
}

TEST_CASE("Boss70Phases") {
	// hook 707 (0x7a0a70), HP before the hit: reading and HP x 3 < max -> a 1 HP paper; mad and HP < 4500 -> max 4500,
	// sun 1500, PHASE2 (once: only while max > 4500); then HP < 1500 -> max 1500 and the boss form ends
	Boss70RulesOn on;
	CHECK(Boss70BeforeHit(kPhaseReading, 50001, 150000) == Boss70Phase{});
	Boss70Phase p = Boss70BeforeHit(kPhaseReading, 49999, 150000);
	CHECK(p.giveShield);
	CHECK(p.shieldHp == 1);
	CHECK(p.phase2MaxHp == -1);
	CHECK(Boss70BeforeHit(kPhaseMaddening, 100, 150000) == Boss70Phase{});
	CHECK(Boss70BeforeHit(kPhaseMad, 4500, 150000) == Boss70Phase{});
	p = Boss70BeforeHit(kPhaseMad, 4499, 150000);
	CHECK_FALSE(p.giveShield);
	CHECK(p.phase2MaxHp == 4500);
	CHECK(p.phase2Sun == 1500);
	CHECK(p.phase3MaxHp == -1);
	p = Boss70BeforeHit(kPhaseMad, 4499, 4500); // already in phase 2
	CHECK(p.phase2MaxHp == -1);
	p = Boss70BeforeHit(kPhaseMad, 1499, 4500);
	CHECK(p.phase3MaxHp == 1500);
	p = Boss70BeforeHit(kPhaseMad, 1000, 150000); // one big hit can pass both lines
	CHECK(p.phase2MaxHp == 4500);
	CHECK(p.phase3MaxHp == 1500);
	// progress meter (hook 101): lost HP x 150 / max while it lives, 150 when it is dying, 0 before wave 10
	CHECK(Boss70ProgressWidth(true, true, 150000, 150000, 10) == 0);
	CHECK(Boss70ProgressWidth(true, true, 75000, 150000, 10) == 75);
	CHECK(Boss70ProgressWidth(true, false, 0, 1500, 10) == 150);
	CHECK(Boss70ProgressWidth(false, false, 0, 0, 9) == 0);
	CHECK(Boss70ProgressWidth(false, false, 0, 0, 10) == -1);
	CHECK(Boss70ProgressWidth(false, false, 0, 0, 0) == 0);
	// draw: shield group always, hands over it unless maddening / mad
	CHECK(Boss70DrawsShieldGroup());
	CHECK(Boss70HandsOverPaper(kPhaseReading));
	CHECK_FALSE(Boss70HandsOverPaper(kPhaseMaddening));
	CHECK_FALSE(Boss70HandsOverPaper(kPhaseMad));
}

TEST_CASE("Boss70AttackLoop") {
	// 0x7ab000 preconditions: +0x74 fade < 0, no butter / ice trap, not hypnotised, has head, not dead, phase not
	// 1/2/3 (dying, burned, mowed) nor 0x1e (maddening)
	Boss70RulesOn on;
	CHECK(Boss70TickRuns(false, false, false, false, true, false, kPhaseReading));
	CHECK(Boss70TickRuns(false, false, false, false, true, false, kPhaseMad));
	CHECK_FALSE(Boss70TickRuns(true, false, false, false, true, false, kPhaseReading));
	CHECK_FALSE(Boss70TickRuns(false, true, false, false, true, false, kPhaseReading));
	CHECK_FALSE(Boss70TickRuns(false, false, true, false, true, false, kPhaseReading));
	CHECK_FALSE(Boss70TickRuns(false, false, false, true, true, false, kPhaseReading));
	CHECK_FALSE(Boss70TickRuns(false, false, false, false, false, false, kPhaseReading));
	CHECK_FALSE(Boss70TickRuns(false, false, false, false, true, true, kPhaseReading));
	CHECK_FALSE(Boss70TickRuns(false, false, false, false, true, false, kPhaseDying));
	CHECK_FALSE(Boss70TickRuns(false, false, false, false, true, false, kPhaseMaddening));
	CHECK(Boss70AttackRollRange(false) == 4);
	CHECK(Boss70AttackRollRange(true) == 7);
	CHECK(Boss70TimerRollRange() == 300);
	CHECK(Boss70NextTimer(0, false) == 1200);
	CHECK(Boss70NextTimer(299, false) == 1499);
	CHECK(Boss70NextTimer(0, true) == 800);   // imul 0x55555556 (/3), add eax,eax (x2)
	CHECK(Boss70NextTimer(1, true) == 800);   // 1201 / 3 = 400
	CHECK(Boss70NextTimer(299, true) == 998); // 1499 / 3 = 499
	CHECK(Boss70RowRollRange() == 5);
	CHECK(std::string(Boss70AttackName(4)) == "rayGun");
	// mad column (0x7ab13b): Rand(5) -> 6 Door, 8 Dancer, 12 Zomboni, 21 Ladder, 29 Gatling head
	CHECK(Boss70ColumnRollRange() == 5);
	CHECK(Boss70ColumnType(0) == 6);
	CHECK(Boss70ColumnType(4) == 29);
	CHECK(Boss70ColumnType(5) == 0);
	CHECK(Boss70ColumnScale() == doctest::Approx(1.5f));
	CHECK(Boss70ColumnHpFactor() == 4);
	CHECK(Boss70ColumnFadeCs() == 400);
	CHECK(Boss70ColumnDoorBucketHp() == 4400);
	// ray gun (mode 4): t % 50 in {9, 18, 26, 34}
	CHECK(Boss70RayGunFires(9));
	CHECK(Boss70RayGunFires(59));
	CHECK(Boss70RayGunFires(84));
	CHECK_FALSE(Boss70RayGunFires(10));
	CHECK_FALSE(Boss70RayGunFires(0));
	CHECK(Boss70RayGunAngle(0) == doctest::Approx(0.0f));
	CHECK(Boss70RayGunAngle(50) == doctest::Approx(1.0471976f)); // sin(pi/2) x pi/3
	CHECK(Boss70RayGunAngle(150) == doctest::Approx(-1.0471976f).epsilon(0.001));
	CHECK(Boss70RayGunSpeedX() == doctest::Approx(-1.0f));
	CHECK(Boss70RayGunDx() == -24);
	CHECK(Boss70RayGunDy() == 24);
	CHECK(Boss70RayGunProjectile() == 3);
	// mad dark drain: f = (p/50 + 50/p) x 0.080078125
	CHECK(Boss70DrainFactor(50) == doctest::Approx(0.16015625f));
	CHECK(Boss70DrainFactor(80) == doctest::Approx(0.16015625f)); // capped at 50
	CHECK(Boss70DrainFactor(10) == doctest::Approx(0.416406f).epsilon(0.0001));
	CHECK(Boss70DrainHp(300, Boss70DrainFactor(10)) == 125); // fistp rounds to nearest: 124.92
	CHECK(Boss70DrainHp(300, Boss70DrainFactor(1)) == 1202);  // one plant: 4.0055 x its max HP (1201.65)
	CHECK(Boss70DrainHeal(1000) == 62);
	CHECK(Boss70DrainHeal(100000) == 1800);
	CHECK(Boss70DrainFlashCs() == 25);
	// mad light eat: |row - boss row| <= 1 and zombie x + rect width + 80 >= boss x
	CHECK(Boss70EatReaches(2, 640, 1, 700, 42)); // any zombie right of the boss
	CHECK(Boss70EatReaches(2, 640, 3, 518, 42));
	CHECK_FALSE(Boss70EatReaches(2, 640, 3, 517, 42));
	CHECK_FALSE(Boss70EatReaches(2, 640, 4, 700, 42));
	CHECK(Boss70EatHeal(270) == 33);
	CHECK(Boss70EatHeal(5000) == 150);
	CHECK(Boss70EatFreeze(7, 2) == 140);
	// sky (0x7aaa70): twelve bungees, graves on columns 7..8
	const auto cells = Boss70BungeeCells();
	REQUIRE(cells.size() == 12);
	CHECK(cells.front() == std::pair<int, int>{0, 1});
	CHECK(cells[4] == std::pair<int, int>{2, 1});
	CHECK(cells.back() == std::pair<int, int>{4, 5});
	CHECK(Boss70GraveFirstCol() == 7);
	CHECK(Boss70GraveLastCol() == 8);
	CHECK(Boss70HeadAnimRate() == 76);
	CHECK(Boss70TextCs() == 250);
}

TEST_CASE("Boss70Texts") {
	Boss70RulesOn on;
	for (Boss70Text t : {Boss70Text::DARK_HUNT, Boss70Text::PHASE2, Boss70Text::PHASE3, Boss70Text::LIGHT,
	         Boss70Text::LASER, Boss70Text::RAY_GUN}) {
		CHECK(Boss70TextKey(t).rfind("PVZU_BOSS70_", 0) == 0);
		CHECK(std::string(Boss70TextFallback(t)).size() > 10);
	}
	CHECK(Boss70TextKey(Boss70Text::NONE).empty());
}

TEST_CASE("Boss70StateSurvivesSaveLoad") {
	// The boss keeps its form, attack timer and attack in ZombieState (variant, counter, extra); the scale, the hit rect,
	// HP, shield, phase, background, plant sleep and seed packets are vanilla fields the save already has.
	ZombieState s;
	s.variant = boss70::kVariant;
	s.counter = 431;
	s.extra = static_cast<int>(Boss70Attack::RAY_GUN);
	auto b = Encode(s);
	ZombieState back;
	REQUIRE(Decode(b.data(), b.size(), back));
	CHECK(back == s);
}

TEST_CASE("Boss70PackRulesMatchTheseValues") {
	for (const auto& k : Boss70RuleKeys()) CHECK_MESSAGE(kBoss70Rules.contains(k), k);
	CHECK(Boss70RuleKeys().size() == kBoss70Rules.size());
	const char* home = std::getenv("HOME");
	fs::path dir = std::getenv("PVZU_PACK_DIR") ? fs::path(std::getenv("PVZU_PACK_DIR"))
	                                             : fs::path(home ? home : "") / "pvz-unnamed-pack";
	fs::path file = dir / "data/rules-level70-boss.json";
	if (!fs::exists(file)) { MESSAGE("pack not here, skipped: " << file.string()); return; }
	std::ifstream in(file);
	nlohmann::json pack = nlohmann::json::parse(in, nullptr, false);
	REQUIRE(pack.is_object());
	CHECK(pack == kBoss70Rules);
	std::ifstream mod(dir / "mod.json");
	nlohmann::json m = nlohmann::json::parse(mod, nullptr, false);
	REQUIRE(m.is_object());
	CHECK(m["features"].value("level70Boss", false));
	// the level row names the script
	std::ifstream levels(dir / "data/levels-51-73.json");
	nlohmann::json l = nlohmann::json::parse(levels, nullptr, false);
	REQUIRE(l.is_object());
	CHECK(l["70"].value("special", "") == "boss");
	CHECK(l["70"].value("waves", 0) == 10);
	CHECK(l["70"].value("startSun", 0) == 500000);
}
