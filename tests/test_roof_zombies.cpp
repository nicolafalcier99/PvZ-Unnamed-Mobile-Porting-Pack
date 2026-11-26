// PVZU: Plan 6 Task 2, Roof zombies and elite forms (pure rules; evidence in the research notes (w5-roof) section 2 and
// pvz-unnamed-pack/SOURCES-roof-zombies.md).
#include "doctest/doctest.h"
#include "Mod/Unnamed/FogZombies.h"
#include "Mod/Unnamed/RoofZombies.h"
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
namespace re = pvzu::unnamed::roofelite;
namespace eo = pvzu::unnamed::eliteoption;

namespace {

// The Unnamed values (same as pvz-unnamed-pack/data/rules-roof-zombies.json; the last test compares the two).
const nlohmann::json kRoofRules = {
	{"roofZombies.bungee.eliteRoll", 12},
	{"roofZombies.bungee.eliteRollAllElite", 3},
	{"roofZombies.bungee.eliteBelow", 3},
	{"roofZombies.bungee.wheelbarrowHpBonus", 750},
	{"roofZombies.bungee.wheelbarrowBottomCs", 2147483647},
	{"roofZombies.bungee.jackBottomCs", 0},
	{"roofZombies.bungee.noPlantRadius", 1},
	{"roofZombies.bungee.jackOffsetPx", 30},
	{"roofZombies.bungee.carryZombies", 1},
	{"roofZombies.bungee.carryMaxHp", 120000},
	{"roofZombies.ladder.shovelRoll", 4},
	{"roofZombies.ladder.shovelHit", 2},
	{"roofZombies.ladder.shovelBelowAllElite", 2},
	{"roofZombies.ladder.shovelShieldBonus", 370},
	{"roofZombies.ladder.secondRoll", 4},
	{"roofZombies.ladder.secondRollAllElite", 2},
	{"roofZombies.ladder.ghostMinPlants", 5},
	{"roofZombies.ladder.pileSpares", 5},
	{"roofZombies.ladder.pileSparesLate", 11},
	{"roofZombies.ladder.pileSparesLateLevel", 60},
	{"roofZombies.ladder.pileLadderHp", 500},
	{"roofZombies.ladder.pileCarrySpeed", 1.6},
	{"roofZombies.ladder.pileFreeSpeed", 3.0},
	{"roofZombies.ladder.ghostSpeed", 2.0},
	{"roofZombies.ladder.ghostTeleportAge", 2},
	{"roofZombies.ladder.ghostTeleportStepPx", 80},
	{"roofZombies.ladder.ghostOneBiteMaxHp", 1000},
	{"roofZombies.ladder.ghostToughBite", 1},
	{"roofZombies.ladder.ghostAlpha", 128},
	{"roofZombies.catapult.hp", 1350},
	{"roofZombies.catapult.poleHeadRoll", 100},
	{"roofZombies.catapult.poleHeadBelow", 50},
	{"roofZombies.catapult.poleHeadAllEliteBelow", 60},
	{"roofZombies.catapult.poleHeadBelowAllElite", 83},
	{"roofZombies.catapult.poleHeadAllEliteBelowAllElite", 100},
	{"roofZombies.catapult.faithLockHp", 190},
	{"roofZombies.garg.gigaHp", 7200},
	{"roofZombies.garg.staffMod", 20},
	{"roofZombies.garg.staffHit", 1},
	{"roofZombies.garg.staffThrowRoll", 33},
	{"roofZombies.garg.staffMaxBoardZombies", 50},
	{"roofZombies.garg.throwAnyDistance", 1},
	{"roofZombies.garg.throwMinDistanceAlways", 1},
	{"roofZombies.imp.thrownSkipsHouse", 1},
};

struct RoofRulesOn {
	RoofRulesOn() { SetRules(kRoofRules); }
	~RoofRulesOn() { SetRules(nlohmann::json::object()); }
};

constexpr int kChew = 0, kDriveOver = 1, kVault = 2, kLadder = 3;   // ZombieAttackType

} // namespace

TEST_CASE("RoofVanillaWithoutRules") {
	SetRules(nlohmann::json::object());
	CHECK(BungeeRollRange() == 0);
	CHECK(LadderShovelRollRange() == 0);
	CHECK(LadderSecondRollRange(eo::DEFAULT) == 0);
	CHECK(CatapultRollRange() == 0);
	CHECK_FALSE(GargStaffHit(1, eo::DEFAULT));
	CHECK(RoofBodyHp(ZOMBIE_BUNGEE, re::WHEELBARROW, 450) == 450);
	CHECK(RoofBodyHp(ZOMBIE_CATAPULT, 0, 850) == 850);
	CHECK(RoofBodyHp(ZOMBIE_REDEYE_GARGANTUAR, 0, 6000) == 6000);
	CHECK(RoofShieldHp(ZOMBIE_LADDER, re::SHOVEL, 500) == 500);
	CHECK(BungeeBottomCounter(re::WHEELBARROW, 300) == 300);
	CHECK(BungeeBottomCounter(re::JACK_BUNGEE, 300) == 300);
	CHECK_FALSE(WheelbarrowBlocksCell(2, 4, 4, 2));
	CHECK_FALSE(JackBungeeLeavesJack(re::JACK_BUNGEE));
	CHECK_FALSE(BungeeCarriesZombies());
	CHECK_FALSE(LadderTargetsAnyPlant(re::SHOVEL));
	CHECK_FALSE(PileCanTarget(re::LADDER_PILE, kChew, false).has_value());
	CHECK_FALSE(ShovelDigsUpPlant(re::SHOVEL));
	CHECK_FALSE(PileCallsBungee(re::LADDER_PILE));
	CHECK_FALSE(KeepsShieldOnLadder(re::GHOST_LADDER));
	CHECK(PileSpares(10) == 0);
	CHECK(PileBodyToShield(re::LADDER_PILE, true, 100) == 0);
	CHECK_FALSE(PileOverflowsIntoNextLadder(re::LADDER_PILE));
	CHECK(PileBites(re::LADDER_PILE, true));
	CHECK(LadderAnimRateMultiplier(re::LADDER_PILE, true, false) == doctest::Approx(1.0f));
	CHECK(LadderAnimRateMultiplier(re::GHOST_LADDER, true, false) == doctest::Approx(1.0f));
	CHECK(GhostTeleportAge() < 0);
	CHECK(GhostBiteDamage(re::GHOST_LADDER, 300, 300, 4) == 4);
	CHECK(GhostAlpha(re::GHOST_LADDER) == 255);
	CHECK_FALSE(SkipsShieldDamageImages(re::SHOVEL));
	// vanilla catapult targeting: the back-most (smallest column)
	CHECK(CatapultPrefers(0, 2, 5));
	CHECK_FALSE(CatapultPrefers(0, 5, 2));
	CHECK(CatapultPrefers(re::POLE_HEAD, 2, 5));   // without the pack the pole head is just a catapult
	CHECK(CatapultFaith(300, 200, 20, false) == FaithHit{200, 300});
	CHECK_FALSE(CatapultTooToughToAsh(20));
	CHECK_FALSE(GargThrowsAtAnyDistance());
	CHECK(GargThrowDistance(300.0f, 40.0f) == doctest::Approx(300.0f));
	CHECK(StaffThrowRollRange() == 0);
	CHECK_FALSE(StaffKeepsThrowing(3, false));
	CHECK_FALSE(ThrownImpSkipsHouse(PHASE_IMP_GETTING_THROWN));
}

TEST_CASE("RoofRollBoundaries") {
	RoofRulesOn on;
	// Bungee (hook 488): Rand(12) < 3; (roll & 1) + 0x16 -> 0 / 2 wheelbarrow, 1 Jack head; forced Jack head
	CHECK(BungeeRollRange() == 12);
	CHECK(RollBungee(0, false) == re::WHEELBARROW);
	CHECK(RollBungee(1, false) == re::JACK_BUNGEE);
	CHECK(RollBungee(2, false) == re::WHEELBARROW);
	CHECK(RollBungee(3, false) == 0);
	CHECK(RollBungee(11, false) == 0);
	CHECK(RollBungee(0, true) == re::JACK_BUNGEE);
	CHECK(RollBungee(2, true) == re::JACK_BUNGEE);
	CHECK(RollBungee(3, true) == 0);   // a forced Jack head still needs the 3/12 roll (study B8: 1/4 of boss bungees)
	// Ladder (hook 487): Rand(4) == 2 shovel; else a second Rand(4): 0 pile, 1 ghost (>= 5 plants) or pile
	CHECK(LadderShovelRollRange() == 4);
	CHECK(RollLadderShovel(2) == re::SHOVEL);
	for (int r : {0, 1, 3}) CHECK(RollLadderShovel(r) == 0);
	CHECK(LadderSecondRollRange(eo::DEFAULT) == 4);
	CHECK(LadderSecondRollRange(eo::ALL) == 4);
	CHECK(LadderSecondRollRange(eo::NONE) == 0);
	CHECK(RollLadderSecond(0, 0) == re::LADDER_PILE);
	CHECK(RollLadderSecond(0, 9) == re::LADDER_PILE);
	CHECK(RollLadderSecond(1, 5) == re::GHOST_LADDER);
	CHECK(RollLadderSecond(1, 4) == re::LADDER_PILE);
	CHECK(RollLadderSecond(2, 9) == 0);
	CHECK(RollLadderSecond(3, 9) == 0);
	// Catapult (hook 498): Rand(100) < 50 pole head; 50..59 too with "all elites"
	CHECK(CatapultRollRange() == 100);
	CHECK(RollCatapult(0, eo::DEFAULT) == re::POLE_HEAD);
	CHECK(RollCatapult(49, eo::DEFAULT) == re::POLE_HEAD);
	CHECK(RollCatapult(50, eo::DEFAULT) == 0);
	CHECK(RollCatapult(59, eo::ALL) == re::POLE_HEAD);
	CHECK(RollCatapult(60, eo::ALL) == 0);
	CHECK(RollCatapult(99, eo::DEFAULT) == 0);
	// Gargantuar staff (hook 513): the pole roll % 20 == 1
	for (int r : {1, 21, 41, 61, 81}) CHECK_MESSAGE(GargStaffHit(r, eo::DEFAULT), r);
	for (int r : {0, 2, 20, 22, 99}) CHECK_MESSAGE(!GargStaffHit(r, eo::DEFAULT), r);
	CHECK(GargStaffHit(0, eo::ALL));
	CHECK_FALSE(GargStaffHit(1, eo::NONE));
	// names and types
	for (ZombieType t : {ZOMBIE_BUNGEE, ZOMBIE_LADDER, ZOMBIE_CATAPULT, ZOMBIE_GARGANTUAR, ZOMBIE_REDEYE_GARGANTUAR})
		CHECK(IsRoofType(t));
	CHECK_FALSE(IsRoofType(ZOMBIE_IMP));
	CHECK_FALSE(IsRoofType(ZOMBIE_JACK_IN_THE_BOX));
	CHECK(std::string(RoofEliteName(ZOMBIE_BUNGEE, re::WHEELBARROW)) == "wheelbarrow");
	CHECK(std::string(RoofEliteName(ZOMBIE_BUNGEE, re::JACK_BUNGEE)) == "jack-head");
	CHECK(std::string(RoofEliteName(ZOMBIE_LADDER, re::SHOVEL)) == "shovel");
	CHECK(std::string(RoofEliteName(ZOMBIE_LADDER, re::LADDER_PILE)) == "ladder-pile");
	CHECK(std::string(RoofEliteName(ZOMBIE_LADDER, re::GHOST_LADDER)) == "ghost");
	CHECK(std::string(RoofEliteName(ZOMBIE_CATAPULT, re::POLE_HEAD)) == "pole-head");
	CHECK(std::string(RoofEliteName(ZOMBIE_GARGANTUAR, re::STAFF)) == "staff");
	CHECK(std::string(RoofEliteName(ZOMBIE_REDEYE_GARGANTUAR, re::STAFF)) == "staff");
	CHECK(std::string(RoofEliteName(ZOMBIE_LADDER, re::WHEELBARROW)) == "none");   // codes mean a form only on their type
	CHECK(std::string(RoofEliteName(ZOMBIE_BUNGEE, 0)) == "none");
}

// The elite option "all" as a setting (0x7b2000 patches; pvz-unnamed-pack SOURCES-shop-ui.md): Bungee Rand(3) (0x796eb4),
// Ladder shovel on Rand(4) < 2 (0x796d2c `jge`) then the second roll Rand(2) (0x95aa1f), Catapult pole head below 83
// (0x796e16) and the rest to 100 (0x910002): the world-7 vase above level 60, else the pole head (0x91009b).
TEST_CASE("RoofAllElitesOver2000Rolls") {
	RoofRulesOn on;
	std::mt19937 gen(5151);
	auto roll = [&](int n) { return std::uniform_int_distribution<int>(0, n - 1)(gen); };
	const int N = 2000;
	auto within = [&](int count, double p) { // 4 standard deviations
		const double sd = std::sqrt(N * p * (1 - p));
		CHECK_MESSAGE(std::abs(count - N * p) <= 4 * sd + 1e-9, count << " vs " << N * p);
	};
	CHECK(BungeeRollRange(eo::ALL) == 3);
	CHECK(BungeeRollRange(eo::NONE) == 0);
	CHECK(LadderShovelRollRange(eo::ALL) == 4);
	CHECK(LadderShovelRollRange(eo::NONE) == 0);
	CHECK(LadderSecondRollRange(eo::ALL, eo::ALL) == 2);
	CHECK(LadderSecondRollRange(eo::NONE, eo::ALL) == 0);
	CHECK(CatapultRollRange(eo::ALL) == 100);
	CHECK(CatapultRollRange(eo::NONE) == 0);
	std::map<int, int> seen;
	for (int i = 0; i < N; ++i) seen[RollBungee(roll(BungeeRollRange(eo::ALL)), false)]++;
	within(seen[re::WHEELBARROW], 2.0 / 3);
	within(seen[re::JACK_BUNGEE], 1.0 / 3);
	CHECK(seen[0] == 0);
	auto ladder = [&](int plants) {
		int v = RollLadderShovel(roll(LadderShovelRollRange(eo::ALL)), eo::ALL);
		if (v == 0) v = RollLadderSecond(roll(LadderSecondRollRange(eo::ALL, eo::ALL)), plants);
		return v;
	};
	seen.clear();
	for (int i = 0; i < N; ++i) seen[ladder(8)]++;
	within(seen[re::SHOVEL], 0.5);
	within(seen[re::LADDER_PILE], 0.25);
	within(seen[re::GHOST_LADDER], 0.25);
	CHECK(seen[0] == 0);
	seen.clear();
	for (int i = 0; i < N; ++i) seen[ladder(2)]++;
	within(seen[re::SHOVEL], 0.5);
	within(seen[re::LADDER_PILE], 0.5);
	CHECK(seen[0] == 0);
	seen.clear();
	for (int i = 0; i < N; ++i) seen[RollCatapult(roll(CatapultRollRange(eo::ALL)), eo::ALL, eo::ALL)]++;
	CHECK(seen[re::POLE_HEAD] == N); // below level 61 every catapult has the pole head
	CHECK(RollCatapult(82, eo::ALL, eo::ALL) == re::POLE_HEAD);
	CHECK(RollCatapult(99, eo::ALL, eo::ALL) == re::POLE_HEAD);
	CHECK(RollCatapult(0, eo::NONE, eo::ALL) == 0);
	// the value "all" without the setting (a staff Giga's thrown Catapult): 0x910000 still stops at 60
	CHECK(RollCatapult(59, eo::ALL, eo::DEFAULT) == re::POLE_HEAD);
	CHECK(RollCatapult(60, eo::ALL, eo::DEFAULT) == 0);
	// the setting "none": the roll code is patched to skip, whatever the value
	CHECK(RollCatapult(0, eo::ALL, eo::NONE) == 0);
	// option 0 through the new parameters = the calls without them
	CHECK(BungeeRollRange(eo::DEFAULT) == BungeeRollRange());
	CHECK(LadderShovelRollRange(eo::DEFAULT) == LadderShovelRollRange());
	CHECK(CatapultRollRange(eo::DEFAULT) == CatapultRollRange());
	for (int v : {eo::DEFAULT, eo::NONE, eo::ALL}) CHECK(LadderSecondRollRange(v, eo::DEFAULT) == LadderSecondRollRange(v));
	for (int r = 0; r < 100; ++r) {
		CHECK(RollLadderShovel(r, eo::DEFAULT) == RollLadderShovel(r));
		for (int v : {eo::DEFAULT, eo::NONE, eo::ALL}) CHECK(RollCatapult(r, v, eo::DEFAULT) == RollCatapult(r, v));
	}
}

TEST_CASE("RoofEliteRatesOver2000Rolls") {
	RoofRulesOn on;
	std::mt19937 gen(5150);
	auto roll = [&](int n) { return std::uniform_int_distribution<int>(0, n - 1)(gen); };
	const int N = 2000;
	auto within = [&](int count, double p) { // 4 standard deviations
		const double sd = std::sqrt(N * p * (1 - p));
		CHECK_MESSAGE(std::abs(count - N * p) <= 4 * sd + 1e-9, count << " vs " << N * p);
	};
	std::map<int, int> seen;
	for (int i = 0; i < N; ++i) seen[RollBungee(roll(BungeeRollRange()), false)]++;
	within(seen[re::WHEELBARROW], 2.0 / 12);   // Almanac/video: wheelbarrows are the common one
	within(seen[re::JACK_BUNGEE], 1.0 / 12);
	within(seen[0], 9.0 / 12);
	seen.clear();
	for (int i = 0; i < N; ++i) seen[RollBungee(roll(BungeeRollRange()), true)]++;   // Zomboss bungees (B8)
	within(seen[re::JACK_BUNGEE], 3.0 / 12);
	CHECK(seen[re::WHEELBARROW] == 0);
	// Ladder with a full board (>= 5 plants) and with < 5 plants: the engine order (shovel first, then the second roll)
	auto ladder = [&](int plants) {
		int v = RollLadderShovel(roll(LadderShovelRollRange()));
		if (v == 0) v = RollLadderSecond(roll(LadderSecondRollRange(eo::DEFAULT)), plants);
		return v;
	};
	seen.clear();
	for (int i = 0; i < N; ++i) seen[ladder(8)]++;
	within(seen[re::SHOVEL], 1.0 / 4);
	within(seen[re::LADDER_PILE], 3.0 / 16);
	within(seen[re::GHOST_LADDER], 3.0 / 16);
	within(seen[0], 6.0 / 16);
	seen.clear();
	for (int i = 0; i < N; ++i) seen[ladder(2)]++;
	within(seen[re::SHOVEL], 1.0 / 4);
	within(seen[re::LADDER_PILE], 3.0 / 8);
	CHECK(seen[re::GHOST_LADDER] == 0);
	seen.clear();
	for (int i = 0; i < N; ++i) seen[RollCatapult(roll(CatapultRollRange()), eo::DEFAULT)]++;
	within(seen[re::POLE_HEAD], 0.5);
	int staff = 0;
	for (int i = 0; i < N; ++i) staff += GargStaffHit(roll(100), eo::DEFAULT) ? 1 : 0;
	within(staff, 0.05);
	// the staff's throw: Rand(33) re-rolled until accepted -> 25 types, uniform, never an excluded one
	const std::set<int> excluded = {1, 10, 11, 13, 14, 16, 19, 25};
	std::map<int, int> thrown;
	for (int i = 0; i < N; ++i) {
		int r = 0;
		do r = roll(StaffThrowRollRange());
		while (!StaffThrowTypeAccepted(r));
		thrown[r]++;
	}
	CHECK(thrown.size() == 25);
	for (auto [t, n] : thrown) {
		CHECK_MESSAGE(excluded.count(t) == 0, t);
		within(n, 1.0 / 25);
	}
}

TEST_CASE("RoofHitPoints") {
	RoofRulesOn on;
	CHECK(RoofBodyHp(ZOMBIE_BUNGEE, re::WHEELBARROW, 450) == 1200);   // Almanac 450 / 1200
	CHECK(RoofBodyHp(ZOMBIE_BUNGEE, re::JACK_BUNGEE, 450) == 450);
	CHECK(RoofBodyHp(ZOMBIE_BUNGEE, 0, 450) == 450);
	CHECK(RoofBodyHp(ZOMBIE_CATAPULT, 0, 850) == 1350);              // every catapult (0x522e87)
	CHECK(RoofBodyHp(ZOMBIE_CATAPULT, re::POLE_HEAD, 850) == 1350);
	CHECK(RoofBodyHp(ZOMBIE_REDEYE_GARGANTUAR, 0, 6000) == 7200);     // 0x523e44
	CHECK(RoofBodyHp(ZOMBIE_REDEYE_GARGANTUAR, re::STAFF, 6000) == 7200);
	CHECK(RoofBodyHp(ZOMBIE_GARGANTUAR, re::STAFF, 3000) == 3000);
	CHECK(RoofBodyHp(ZOMBIE_LADDER, re::SHOVEL, 500) == 500);
	CHECK(RoofBodyHp(ZOMBIE_NORMAL, re::WHEELBARROW, 270) == 270);
	CHECK(RoofShieldHp(ZOMBIE_LADDER, re::SHOVEL, 500) == 870);
	CHECK(RoofShieldHp(ZOMBIE_LADDER, re::LADDER_PILE, 500) == 500);
	CHECK(RoofShieldHp(ZOMBIE_DOOR, re::SHOVEL, 1100) == 1100);
}

TEST_CASE("Bungee") {
	RoofRulesOn on;
	// hook 527: at the bottom the wheelbarrow waits forever, the Jack head grabs at once
	CHECK(BungeeBottomCounter(re::WHEELBARROW, 300) == 2147483647);
	CHECK(BungeeBottomCounter(re::JACK_BUNGEE, 300) == 0);
	CHECK(BungeeBottomCounter(0, 300) == 300);
	// hook 46: no planting in the 3x3 around (row, target column)
	for (int r = 1; r <= 3; ++r)
		for (int c = 3; c <= 5; ++c) CHECK(WheelbarrowBlocksCell(2, 4, c, r));
	CHECK_FALSE(WheelbarrowBlocksCell(2, 4, 6, 2));
	CHECK_FALSE(WheelbarrowBlocksCell(2, 4, 2, 2));
	CHECK_FALSE(WheelbarrowBlocksCell(2, 4, 4, 0));
	CHECK_FALSE(WheelbarrowBlocksCell(2, 4, 4, 4));
	CHECK(WheelbarrowBlocksCell(0, 0, 0, 0));
	// hook 529
	CHECK(JackBungeeLeavesJack(re::JACK_BUNGEE));
	CHECK_FALSE(JackBungeeLeavesJack(re::WHEELBARROW));
	CHECK_FALSE(JackBungeeLeavesJack(0));
	CHECK(JackBungeeJackX(360.0f) == doctest::Approx(330.0f));
	// hook 522: a normal bungee takes a hypnotised zombie, a hypnotised bungee a normal one; never a dead one, a Bungee,
	// the Boss or anything over 120000 max HP
	CHECK(BungeeCarriesZombies());
	CHECK(BungeeCanCarry(false, ZOMBIE_NORMAL, 270, true, false));
	CHECK(BungeeCanCarry(true, ZOMBIE_NORMAL, 270, false, false));
	CHECK_FALSE(BungeeCanCarry(false, ZOMBIE_NORMAL, 270, false, false));
	CHECK_FALSE(BungeeCanCarry(true, ZOMBIE_NORMAL, 270, true, false));
	CHECK_FALSE(BungeeCanCarry(false, ZOMBIE_NORMAL, 270, true, true));    // dead
	CHECK_FALSE(BungeeCanCarry(false, ZOMBIE_BUNGEE, 450, true, false));
	CHECK_FALSE(BungeeCanCarry(false, ZOMBIE_BOSS, 50000, true, false));
	CHECK(BungeeCanCarry(false, ZOMBIE_REDEYE_GARGANTUAR, 120000, true, false));
	CHECK_FALSE(BungeeCanCarry(false, ZOMBIE_REDEYE_GARGANTUAR, 120001, true, false));
	// 0x957680: the rect from x 290 to 360 covers columns round(290/80)-1 = 3 .. round(360/80)-1 = 3 (4.5 rounds to 4)
	CHECK(ZombieCoversColumn(290, 70, 3));
	CHECK_FALSE(ZombieCoversColumn(290, 70, 2));
	CHECK(ZombieCoversColumn(300, 140, 4));   // 300 -> 3.75 -> 4 - 1 = 3; 440 -> 5.5 -> 6 - 1 = 5
	CHECK(ZombieCoversColumn(300, 140, 5));
	CHECK_FALSE(ZombieCoversColumn(300, 140, 6));
}

TEST_CASE("Ladder") {
	RoofRulesOn on;
	// hook 655: the shovel and the pile target every plant in the ladder phases
	CHECK(LadderTargetsAnyPlant(re::SHOVEL));
	CHECK(LadderTargetsAnyPlant(re::LADDER_PILE));
	CHECK_FALSE(LadderTargetsAnyPlant(re::GHOST_LADDER));
	CHECK_FALSE(LadderTargetsAnyPlant(0));
	// the pile's own targeting: chews only laddered plants, ladders only unladdered ones
	REQUIRE(PileCanTarget(re::LADDER_PILE, kChew, false).has_value());
	CHECK_FALSE(*PileCanTarget(re::LADDER_PILE, kChew, false));
	CHECK_FALSE(PileCanTarget(re::LADDER_PILE, kChew, true).has_value());   // the vanilla top-plant check
	CHECK(*PileCanTarget(re::LADDER_PILE, kLadder, false));
	CHECK_FALSE(*PileCanTarget(re::LADDER_PILE, kLadder, true));
	CHECK_FALSE(PileCanTarget(re::LADDER_PILE, kVault, false).has_value());
	CHECK_FALSE(PileCanTarget(re::LADDER_PILE, kDriveOver, false).has_value());
	CHECK_FALSE(PileCanTarget(re::SHOVEL, kChew, false).has_value());
	CHECK_FALSE(PileCanTarget(0, kLadder, false).has_value());
	// hooks 600, 601
	CHECK(ShovelDigsUpPlant(re::SHOVEL));
	CHECK_FALSE(ShovelDigsUpPlant(re::LADDER_PILE));
	CHECK(PileCallsBungee(re::LADDER_PILE));
	CHECK_FALSE(PileCallsBungee(re::SHOVEL));
	CHECK(KeepsShieldOnLadder(re::GHOST_LADDER));
	CHECK_FALSE(KeepsShieldOnLadder(re::LADDER_PILE));
	CHECK_FALSE(KeepsShieldOnLadder(0));
	// hook 487 (0xc02cc0): 5 spares, 11 from PlayerInfo level 60; hook 722: each new ladder 500
	CHECK(PileSpares(1) == 5);
	CHECK(PileSpares(59) == 5);
	CHECK(PileSpares(60) == 11);
	CHECK(PileLadderHp() == 500);
	// hooks 702, 710, 672
	CHECK(PileBodyToShield(re::LADDER_PILE, true, 101) == 50);
	CHECK(PileBodyToShield(re::LADDER_PILE, false, 100) == 0);
	CHECK(PileBodyToShield(re::GHOST_LADDER, true, 100) == 0);
	CHECK(PileOverflowsIntoNextLadder(re::LADDER_PILE));
	CHECK_FALSE(PileBites(re::LADDER_PILE, true));
	CHECK(PileBites(re::LADDER_PILE, false));
	CHECK(PileBites(re::GHOST_LADDER, true));
	// hook 662 / 0x7a6be3: pile x1.6 with a ladder, x3 without; ghost x2 while not eating
	CHECK(LadderAnimRateMultiplier(re::LADDER_PILE, true, false) == doctest::Approx(1.6f));
	CHECK(LadderAnimRateMultiplier(re::LADDER_PILE, false, true) == doctest::Approx(3.0f));
	CHECK(LadderAnimRateMultiplier(re::GHOST_LADDER, true, false) == doctest::Approx(2.0f));
	CHECK(LadderAnimRateMultiplier(re::GHOST_LADDER, true, true) == doctest::Approx(1.0f));
	CHECK(LadderAnimRateMultiplier(re::SHOVEL, true, false) == doctest::Approx(1.0f));
	// hook 619: teleport on its 2nd tick to (last unladdered plant column + 1) * 80
	CHECK(GhostTeleportAge() == 2);
	CHECK(GhostTeleportX(-1) == doctest::Approx(0.0f));
	CHECK(GhostTeleportX(0) == doctest::Approx(80.0f));
	CHECK(GhostTeleportX(4) == doctest::Approx(400.0f));
	CHECK(GhostTeleportX(8) == doctest::Approx(720.0f));
	// hook 790: one bite kills a plant of max HP <= 1000, tougher plants lose 1 per bite
	CHECK(GhostBiteDamage(re::GHOST_LADDER, 300, 300, 4) == 301);
	CHECK(GhostBiteDamage(re::GHOST_LADDER, 1000, 12, 4) == 13);
	CHECK(GhostBiteDamage(re::GHOST_LADDER, 4000, 4000, 4) == 1);
	CHECK(GhostBiteDamage(re::LADDER_PILE, 300, 300, 4) == 4);
	CHECK(GhostBiteDamage(0, 300, 300, 4) == 4);
	// hook 647: drawn at alpha 128; hook 696: the shovel keeps its image
	CHECK(GhostAlpha(re::GHOST_LADDER) == 128);
	CHECK(GhostAlpha(re::LADDER_PILE) == 255);
	CHECK(SkipsShieldDamageImages(re::SHOVEL));
	CHECK_FALSE(SkipsShieldDamageImages(re::LADDER_PILE));
}

TEST_CASE("CatapultTargetsAndFaith") {
	RoofRulesOn on;
	// hook 544: the pole head keeps the largest column (front-most), others the smallest (vanilla)
	CHECK(CatapultPrefers(re::POLE_HEAD, 5, 2));
	CHECK_FALSE(CatapultPrefers(re::POLE_HEAD, 2, 5));
	CHECK_FALSE(CatapultPrefers(re::POLE_HEAD, 3, 3));
	CHECK(CatapultPrefers(0, 2, 5));
	CHECK_FALSE(CatapultPrefers(0, 3, 3));
	// hook 705 (0xc00000): body - damage <= 190 with balls left -> damage & 1, body 190
	CHECK(CatapultFaith(1350, 100, 20, false) == FaithHit{100, 1350});
	CHECK(CatapultFaith(300, 110, 20, false) == FaithHit{0, 190});      // 190 left: locked
	CHECK(CatapultFaith(300, 109, 20, false) == FaithHit{109, 300});    // 191 left: a normal hit
	CHECK(CatapultFaith(300, 111, 20, false) == FaithHit{1, 190});
	CHECK(CatapultFaith(300, 1800, 1, false) == FaithHit{0, 190});
	CHECK(CatapultFaith(190, 20, 5, false) == FaithHit{0, 190});
	CHECK(CatapultFaith(190, 21, 5, false) == FaithHit{1, 190});
	CHECK(CatapultFaith(300, 1800, 0, false) == FaithHit{1800, 300});   // no balls: vanilla
	CHECK(CatapultFaith(300, 1800, 20, true) == FaithHit{1800, 300});   // Wall-nut Bowling: vanilla
	CHECK(CatapultTooToughToAsh(1));
	CHECK_FALSE(CatapultTooToughToAsh(0));
	// HP never leaves its bounds through the lock: 1000 hits of 7 to 1800 from 1350
	int hp = 1350;
	for (int i = 0; i < 1000; ++i) {
		const FaithHit h = CatapultFaith(hp, 7 + (i * 37) % 1800, 20, false);
		hp = h.bodyHp - h.damage;
		CHECK(hp >= 189);
	}
	CHECK(hp <= 190);
}

TEST_CASE("Gargantuar") {
	RoofRulesOn on;
	CHECK(GargThrowsAtAnyDistance());
	CHECK(GargThrowDistance(300.0f, 40.0f) == doctest::Approx(40.0f));
	CHECK(GargThrowDistance(-500.0f, -140.0f) == doctest::Approx(-140.0f));
	CHECK(StaffThrowRollRange() == 33);
	for (int t : {0, 2, 3, 4, 5, 6, 7, 8, 9, 12, 15, 17, 18, 20, 21, 22, 23, 24, 26, 27, 28, 29, 30, 31, 32})
		CHECK_MESSAGE(StaffThrowTypeAccepted(t), t);
	for (int t : {1, 10, 11, 13, 14, 16, 19, 25, 33, -1})
		CHECK_MESSAGE(!StaffThrowTypeAccepted(t), t);
	// hook 568: keeps throwing while the board has <= 50 zombies and it is not hypnotised
	CHECK(StaffKeepsThrowing(50, false));
	CHECK_FALSE(StaffKeepsThrowing(51, false));
	CHECK_FALSE(StaffKeepsThrowing(3, true));
	// hook 630
	CHECK(ThrownImpSkipsHouse(PHASE_IMP_GETTING_THROWN));
	CHECK(ThrownImpSkipsHouse(PHASE_IMP_LANDING));
	CHECK_FALSE(ThrownImpSkipsHouse(PHASE_ZOMBIE_NORMAL));
}

TEST_CASE("RoofEliteArtTable") {
	using V = std::vector<RoofArtImage>;
	CHECK(RoofArtFor(ZOMBIE_BUNGEE, re::WHEELBARROW) == V{{"anim_head1", "images/Zen_WheelBarrow"}});
	CHECK(RoofArtFor(ZOMBIE_BUNGEE, re::JACK_BUNGEE) == V{{"anim_head1", "reanim/Zombie_jackbox_box"}});
	CHECK(RoofArtFor(ZOMBIE_LADDER, re::SHOVEL) == V{{"Zombie_ladder_1", "images/Shovel_hi_res"}});
	CHECK(RoofArtFor(ZOMBIE_LADDER, re::LADDER_PILE).empty());     // the vanilla ladder (re-set on every new ladder)
	CHECK(RoofArtFor(ZOMBIE_LADDER, re::GHOST_LADDER).empty());    // tint only
	CHECK(RoofArtFor(ZOMBIE_CATAPULT, re::POLE_HEAD) == V{{"Zombie_catapult_driver_head", "particles/ZombiePoleVaulterHead"}});
	CHECK(RoofArtFor(ZOMBIE_GARGANTUAR, re::STAFF) == V{{"Zombie_gargantuar_telephonepole", "reanim/Zombie_gargantuar_staff"}});
	CHECK(RoofArtFor(ZOMBIE_REDEYE_GARGANTUAR, re::STAFF) == RoofArtFor(ZOMBIE_GARGANTUAR, re::STAFF));
	for (ZombieType t : {ZOMBIE_BUNGEE, ZOMBIE_LADDER, ZOMBIE_CATAPULT, ZOMBIE_GARGANTUAR})
		CHECK(RoofArtFor(t, 0).empty());
	CHECK(RoofArtFor(ZOMBIE_NORMAL, re::WHEELBARROW).empty());
	auto none = [](const std::string&) { return false; };
	auto all = [](const std::string&) { return true; };
	CHECK(ResolveRoofArt(ZOMBIE_BUNGEE, re::WHEELBARROW, none).empty());
	CHECK(ResolveRoofArt(ZOMBIE_BUNGEE, re::WHEELBARROW, all) == RoofArtFor(ZOMBIE_BUNGEE, re::WHEELBARROW));
}

TEST_CASE("RoofFormsSurviveSaveLoad") {
	// the pile keeps its spare-ladder count in ZombieState::counter (exe Board+0x5888 side array), saved since Plan 2
	ZombieState s;
	s.variant = re::LADDER_PILE;
	s.counter = 3;
	ZombieState s2;
	auto b = Encode(s);
	REQUIRE(Decode(b.data(), b.size(), s2));
	CHECK(s2 == s);
	ZombieState g;
	g.variant = re::STAFF;
	auto gb = Encode(g);
	ZombieState g2;
	REQUIRE(Decode(gb.data(), gb.size(), g2));
	CHECK(g2.variant == 0x106);
}

TEST_CASE("RoofPackRulesMatchTheseValues") {
	for (const auto& k : RoofZombieRuleKeys()) CHECK_MESSAGE(kRoofRules.contains(k), k);
	CHECK(RoofZombieRuleKeys().size() == kRoofRules.size());
	const char* home = std::getenv("HOME");
	fs::path file = std::getenv("PVZU_PACK_DIR") ? fs::path(std::getenv("PVZU_PACK_DIR"))
	                                              : fs::path(home ? home : "") / "pvz-unnamed-pack";
	file /= "data/rules-roof-zombies.json";
	if (!fs::exists(file)) { MESSAGE("pack not here, skipped: " << file.string()); return; }
	std::ifstream in(file);
	nlohmann::json pack = nlohmann::json::parse(in, nullptr, false);
	REQUIRE(pack.is_object());
	CHECK(pack == kRoofRules);
}
