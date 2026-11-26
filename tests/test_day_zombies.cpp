// PVZU: Plan 2 Task 4, Day zombies and elite forms (pure rules; evidence in the research notes (zombies)).
#include "doctest/doctest.h"
#include "Mod/Unnamed/DayZombies.h"
#include "Mod/Unnamed/Options.h"
#include "Mod/Unnamed/Rules.h"

#include "nlohmann/json.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <random>

using namespace pvzu::unnamed;
namespace fs = std::filesystem;
namespace eo = pvzu::unnamed::eliteoption;

namespace {

// The Unnamed values (same as pvz-unnamed-pack/data/rules-day-zombies.json; the last test compares the two).
const nlohmann::json kUnnamedRules = {
	{"dayZombies.startOffsetPx", 133},
	{"dayZombies.normal.rollRange", 8},
	{"dayZombies.normal.stickyMaxRoll", 0},
	{"dayZombies.normal.coinMaxRoll", 3},
	{"dayZombies.normal.rollRangeAllElite", 4},
	{"dayZombies.sticky.bodyHp", 640},
	{"dayZombies.sticky.animRateMul", 3.0},
	{"dayZombies.sticky.eatsPlants", 0},
	{"dayZombies.elite.noYuckyFace", 1},
	{"dayZombies.coin.squishWhenCrippled", 1},
	{"dayZombies.coin.copyAtPlantHpMax", 8},
	{"dayZombies.coin.footballAfterPlayerLevel", 60},
	{"dayZombies.coin.footballRollRange", 10},
	{"dayZombies.flag.bodyHp", 640},
	{"dayZombies.flag.drainPeriodCs", 40},
	{"dayZombies.flag.drainMin", 1},
	{"dayZombies.flag.drainMax", 3},
	{"dayZombies.flag.biteRemainderMin", 1},
	{"dayZombies.flag.biteRemainderMax", 5},
	{"dayZombies.cone.rollRange", 11},
	{"dayZombies.cone.rollRangeAllElite", 2},
	{"dayZombies.bucket.rollRange", 9},
	{"dayZombies.bucket.rollRangeAllElite", 2},
	{"dayZombies.pumpkin.bodyHp", 1100},
	{"dayZombies.pumpkin.animRateMul", 0.33333331},
	{"dayZombies.pumpkin.giveBucketHp", 1100},
	{"dayZombies.aura.periodCs", 650},
	{"dayZombies.aura.rangePx", 200},
	{"dayZombies.aura.rows", 1},
	{"dayZombies.dancerHead.summonOnDeath", 1},
	{"dayZombies.umbrella.skyDropOnDeath", 1},
	{"dayZombies.skyDrop.typeRollRange", 25},
	{"dayZombies.pole.bodyHp", 870},
	{"dayZombies.pole.startX", 955},
	{"dayZombies.zamboni.startX", 933},          // R8: hooks 495 / 497 / 512 (the research notes (r8-options) 5)
	{"dayZombies.catapult.startX", 958},
	{"dayZombies.gargantuar.startX", 978},
	{"dayZombies.pole.rollRange", 22},
	{"dayZombies.pole.skyDropRollRange", 19},
	{"dayZombies.pole.rollRangeAllElite", 13},
	{"dayZombies.pole.skyDropRollRangeAllElite", 10},
	{"dayZombies.pole.skyDropRollAdd", 3},
	{"dayZombies.pole.gloveMaxRoll", 2},
	{"dayZombies.pole.roadSignMaxRoll", 9},
	{"dayZombies.pole.vaseMaxRoll", 12},
	{"dayZombies.pole.roadSignWorld", 7},
	{"dayZombies.glove.attackRectExtraPx", 112},
	{"dayZombies.glove.leapOnPlantDeath", 1},
	{"dayZombies.glove.leapOffsetPx", 96},
	{"dayZombies.glove.leapBite", 800},
	{"dayZombies.glove.leapVelX", 0.5},
};

struct UnnamedRulesOn {
	UnnamedRulesOn() { SetRules(kUnnamedRules); }
	~UnnamedRulesOn() { SetRules(nlohmann::json::object()); }
};

// Counts variants over every roll the given range can produce, `rounds` times through a fixed-seed generator.
std::map<int, int> Simulate(ZombieType type, int world, int count, bool skyDropped = false, int option = eo::DEFAULT) {
	std::mt19937 gen(12345);
	std::map<int, int> seen;
	const int n = EliteRollRange(type, skyDropped, option);
	if (n <= 0) return seen;
	std::uniform_int_distribution<int> dist(0, n - 1);
	for (int i = 0; i < count; ++i) seen[RollEliteVariant(type, dist(gen), world, skyDropped, option)]++;
	return seen;
}

} // namespace

TEST_CASE("NoEliteIsZeroNeverAnEliteCode") {
	CHECK(StateVariantFromExe(-1) == elite::NONE);
	CHECK(StateVariantFromExe(0x105) == 0x105);
	for (int v : {elite::STICKY_NOTE, elite::PHONOGRAPH, elite::DANCER_HEAD, elite::PUMPKIN, elite::UMBRELLA,
	              elite::ROAD_SIGN, elite::GLOVE, elite::VASE_POLE, elite::SILVER_COIN})
		CHECK(v != 0);
	CHECK(std::string(EliteName(0)) == "none");
	CHECK(std::string(EliteName(0x105)) == "silver-coin");
}

TEST_CASE("VanillaWithoutRules") {
	SetRules(nlohmann::json::object());
	for (ZombieType t : {ZOMBIE_NORMAL, ZOMBIE_FLAG, ZOMBIE_TRAFFIC_CONE, ZOMBIE_PAIL, ZOMBIE_POLEVAULTER})
		CHECK(EliteRollRange(t) == 0);
	CHECK(RollEliteVariant(ZOMBIE_NORMAL, 0, 1) == elite::NONE);
	CHECK(StartOffsetPx(ZOMBIE_NORMAL) == 0.0f);
	CHECK(PoleStartBaseX(870) == 870);
	CHECK(ZombieStartBaseX(ZOMBIE_ZAMBONI, 800) == 800);
	CHECK(ZombieStartBodyHp(ZOMBIE_FLAG, 0, 270) == 270);
	CHECK(ZombieStartBodyHp(ZOMBIE_POLEVAULTER, 0, 500) == 500);
	CHECK(ZombieStartBodyHp(ZOMBIE_NORMAL, elite::STICKY_NOTE, 270) == 270);
	CHECK(EliteAnimRateMultiplier(ZOMBIE_NORMAL, elite::STICKY_NOTE) == 1.0f);
	CHECK(EliteEatsPlants(elite::STICKY_NOTE));
	CHECK(EliteShowsYuckyFace(elite::STICKY_NOTE));
	for (int age = 0; age < 16; ++age) CHECK(BitesThisTick(ZOMBIE_FLAG, age, 4) == (age % 4 == 0));
	CHECK_FALSE(FlagDrainsThisTick(ZOMBIE_FLAG, 0));
	CHECK_FALSE(FlagDrainsThisTick(ZOMBIE_FLAG, 40));
	int counter = 0;
	for (int i = 0; i < 2000; ++i) CHECK_FALSE(AuraTick(counter));
	CHECK_FALSE(SummonsDancersOnDeath(elite::DANCER_HEAD));
	CHECK_FALSE(SkyDropsOnDeath(elite::UMBRELLA));
	CHECK(SkyDropTypeRollRange() == 0);
	CHECK_FALSE(CoinSquishes(ZOMBIE_NORMAL, elite::SILVER_COIN, false, true));
	CHECK_FALSE(CoinCopiesOnBite(ZOMBIE_NORMAL, elite::SILVER_COIN, 1));
	CHECK(EliteAttackRectExtraPx(elite::GLOVE) == 0);
}

TEST_CASE("NormalRollBoundaries") {
	UnnamedRulesOn on;
	CHECK(EliteRollRange(ZOMBIE_NORMAL) == 8);
	CHECK(EliteRollRange(ZOMBIE_DUCKY_TUBE) == 8);
	CHECK(RollEliteVariant(ZOMBIE_NORMAL, 0, 1) == elite::STICKY_NOTE);
	CHECK(RollEliteVariant(ZOMBIE_NORMAL, 1, 1) == elite::SILVER_COIN);
	CHECK(RollEliteVariant(ZOMBIE_NORMAL, 3, 1) == elite::SILVER_COIN);
	CHECK(RollEliteVariant(ZOMBIE_NORMAL, 4, 1) == elite::NONE);
	CHECK(RollEliteVariant(ZOMBIE_NORMAL, 7, 1) == elite::NONE);
	CHECK(RollEliteVariant(ZOMBIE_NORMAL, 3, 7) == elite::SILVER_COIN); // no world check on the coin head
	CHECK(RollEliteVariant(ZOMBIE_DUCKY_TUBE, 0, 3) == elite::STICKY_NOTE);
}

TEST_CASE("ConeBucketRollBoundaries") {
	UnnamedRulesOn on;
	CHECK(EliteRollRange(ZOMBIE_TRAFFIC_CONE) == 11);
	CHECK(RollEliteVariant(ZOMBIE_TRAFFIC_CONE, 0, 1) == elite::PHONOGRAPH);
	CHECK(RollEliteVariant(ZOMBIE_TRAFFIC_CONE, 1, 1) == elite::DANCER_HEAD);
	CHECK(RollEliteVariant(ZOMBIE_TRAFFIC_CONE, 2, 1) == elite::NONE);
	CHECK(RollEliteVariant(ZOMBIE_TRAFFIC_CONE, 10, 1) == elite::NONE);
	CHECK(EliteRollRange(ZOMBIE_PAIL) == 9);
	CHECK(RollEliteVariant(ZOMBIE_PAIL, 0, 1) == elite::PUMPKIN);
	CHECK(RollEliteVariant(ZOMBIE_PAIL, 1, 1) == elite::UMBRELLA);
	CHECK(RollEliteVariant(ZOMBIE_PAIL, 2, 1) == elite::NONE);
	CHECK(RollEliteVariant(ZOMBIE_PAIL, 8, 1) == elite::NONE);
	CHECK(EliteRollRange(ZOMBIE_FLAG) == 0);
	CHECK(EliteRollRange(ZOMBIE_NEWSPAPER) == 0); // other worlds' zombies are later plans
}

TEST_CASE("PoleRollBoundaries") {
	UnnamedRulesOn on;
	CHECK(EliteRollRange(ZOMBIE_POLEVAULTER) == 22);
	CHECK(RollEliteVariant(ZOMBIE_POLEVAULTER, 0, 1) == elite::GLOVE);
	CHECK(RollEliteVariant(ZOMBIE_POLEVAULTER, 2, 1) == elite::GLOVE);
	CHECK(RollEliteVariant(ZOMBIE_POLEVAULTER, 3, 1) == elite::NONE);    // road sign only in world 7
	CHECK(RollEliteVariant(ZOMBIE_POLEVAULTER, 9, 1) == elite::NONE);
	CHECK(RollEliteVariant(ZOMBIE_POLEVAULTER, 3, 7) == elite::ROAD_SIGN);
	CHECK(RollEliteVariant(ZOMBIE_POLEVAULTER, 9, 7) == elite::ROAD_SIGN);
	CHECK(RollEliteVariant(ZOMBIE_POLEVAULTER, 10, 1) == elite::VASE_POLE);
	CHECK(RollEliteVariant(ZOMBIE_POLEVAULTER, 12, 1) == elite::VASE_POLE);
	CHECK(RollEliteVariant(ZOMBIE_POLEVAULTER, 13, 1) == elite::NONE);
	CHECK(RollEliteVariant(ZOMBIE_POLEVAULTER, 21, 7) == elite::NONE);
	// bungee-dropped poles roll Rand(19) + 3: never a glove
	CHECK(EliteRollRange(ZOMBIE_POLEVAULTER, true) == 19);
	CHECK(RollEliteVariant(ZOMBIE_POLEVAULTER, 0, 1, true) == elite::NONE);
	CHECK(RollEliteVariant(ZOMBIE_POLEVAULTER, 7, 1, true) == elite::VASE_POLE);
	CHECK(RollEliteVariant(ZOMBIE_POLEVAULTER, 9, 1, true) == elite::VASE_POLE);
	CHECK(RollEliteVariant(ZOMBIE_POLEVAULTER, 10, 1, true) == elite::NONE);
	CHECK(RollEliteVariant(ZOMBIE_POLEVAULTER, 18, 1, true) == elite::NONE);
	CHECK(PoleMakesGraveOnVault(elite::NONE));
	CHECK_FALSE(PoleMakesGraveOnVault(elite::GLOVE));
	CHECK_FALSE(PoleMakesGraveOnVault(elite::VASE_POLE));
}

TEST_CASE("EliteRatesOver2000Rolls") {
	UnnamedRulesOn on;
	auto near = [](int got, double p) { return std::abs(got - 2000 * p) <= 2000 * 0.035; };
	auto n = Simulate(ZOMBIE_NORMAL, 1, 2000);
	CHECK(near(n[elite::STICKY_NOTE], 1.0 / 8));
	CHECK(near(n[elite::SILVER_COIN], 3.0 / 8));
	CHECK(near(n[elite::NONE], 4.0 / 8));
	auto c = Simulate(ZOMBIE_TRAFFIC_CONE, 1, 2000);
	CHECK(near(c[elite::PHONOGRAPH], 1.0 / 11));
	CHECK(near(c[elite::DANCER_HEAD], 1.0 / 11));
	auto b = Simulate(ZOMBIE_PAIL, 1, 2000);
	CHECK(near(b[elite::PUMPKIN], 1.0 / 9));
	CHECK(near(b[elite::UMBRELLA], 1.0 / 9));
	auto p = Simulate(ZOMBIE_POLEVAULTER, 1, 2000);
	CHECK(near(p[elite::GLOVE], 3.0 / 22));
	CHECK(near(p[elite::VASE_POLE], 3.0 / 22));
	CHECK(p[elite::ROAD_SIGN] == 0);
	auto p7 = Simulate(ZOMBIE_POLEVAULTER, 7, 2000);
	CHECK(near(p7[elite::ROAD_SIGN], 7.0 / 22));
	auto f = Simulate(ZOMBIE_FLAG, 1, 1); // range 0: never rolls
	CHECK(f.empty());
}

// The elite option "all" (exe 0x800258 == 2; pvz-unnamed-pack SOURCES-shop-ui.md): Normal 0xc11160 Rand(4), Cone
// 0x795fbb Rand(2) + 2, Pole 0x7a1b91 Rand(13) (sky drop Rand(10) + 3) and the road sign without the world-7 check
// (0x95af49), all read the value directly; Bucket's Rand(9) is patched to Rand(2) (0x7b2000 writes 0x7970ec).
TEST_CASE("AllElitesOver2000Rolls") {
	UnnamedRulesOn on;
	auto near = [](int got, double p) { return std::abs(got - 2000 * p) <= 2000 * 0.035; };
	CHECK(EliteRollRange(ZOMBIE_NORMAL, false, eo::ALL) == 4);
	CHECK(EliteRollRange(ZOMBIE_DUCKY_TUBE, false, eo::ALL) == 4);
	CHECK(EliteRollRange(ZOMBIE_TRAFFIC_CONE, false, eo::ALL) == 2);
	CHECK(EliteRollRange(ZOMBIE_PAIL, false, eo::ALL) == 2);
	CHECK(EliteRollRange(ZOMBIE_POLEVAULTER, false, eo::ALL) == 13);
	CHECK(EliteRollRange(ZOMBIE_POLEVAULTER, true, eo::ALL) == 10);
	CHECK(EliteRollRange(ZOMBIE_FLAG, false, eo::ALL) == 0);
	for (ZombieType t : {ZOMBIE_NORMAL, ZOMBIE_TRAFFIC_CONE, ZOMBIE_PAIL, ZOMBIE_POLEVAULTER})
		CHECK(EliteRollRange(t, false, eo::NONE) == 0);
	auto n = Simulate(ZOMBIE_NORMAL, 1, 2000, false, eo::ALL);
	CHECK(n[elite::NONE] == 0);
	CHECK(near(n[elite::STICKY_NOTE], 1.0 / 4));
	CHECK(near(n[elite::SILVER_COIN], 3.0 / 4));
	auto c = Simulate(ZOMBIE_TRAFFIC_CONE, 1, 2000, false, eo::ALL);
	CHECK(c[elite::NONE] == 0);
	CHECK(near(c[elite::PHONOGRAPH], 0.5));
	CHECK(near(c[elite::DANCER_HEAD], 0.5));
	auto b = Simulate(ZOMBIE_PAIL, 1, 2000, false, eo::ALL);
	CHECK(b[elite::NONE] == 0);
	CHECK(near(b[elite::PUMPKIN], 0.5));
	CHECK(near(b[elite::UMBRELLA], 0.5));
	auto p = Simulate(ZOMBIE_POLEVAULTER, 1, 2000, false, eo::ALL); // world 1: the road sign needs no world 7 now
	CHECK(p[elite::NONE] == 0);
	CHECK(near(p[elite::GLOVE], 3.0 / 13));
	CHECK(near(p[elite::ROAD_SIGN], 7.0 / 13));
	CHECK(near(p[elite::VASE_POLE], 3.0 / 13));
	auto ps = Simulate(ZOMBIE_POLEVAULTER, 1, 2000, true, eo::ALL); // sky drop: Rand(10) + 3, never the glove
	CHECK(ps[elite::NONE] == 0);
	CHECK(ps[elite::GLOVE] == 0);
	CHECK(near(ps[elite::ROAD_SIGN], 7.0 / 10));
	CHECK(near(ps[elite::VASE_POLE], 3.0 / 10));
	CHECK(RollEliteVariant(ZOMBIE_NORMAL, 0, 1, false, eo::NONE) == elite::NONE);
}

TEST_CASE("AllElitesIZombieNormalIsAlwaysCoin") { // 0xc111c7: in I, Zombie option 2 forces 0x105, nothing is drawn
	UnnamedRulesOn on;
	CHECK(NormalForcedCoin(ZOMBIE_NORMAL, eo::ALL, true));
	CHECK(NormalForcedCoin(ZOMBIE_DUCKY_TUBE, eo::ALL, true));
	CHECK_FALSE(NormalForcedCoin(ZOMBIE_NORMAL, eo::ALL, false));
	CHECK_FALSE(NormalForcedCoin(ZOMBIE_NORMAL, eo::DEFAULT, true));
	CHECK_FALSE(NormalForcedCoin(ZOMBIE_TRAFFIC_CONE, eo::ALL, true));
}

TEST_CASE("DefaultOptionMatchesTheOldRolls") { // option 0 through the new parameter = the calls without it
	UnnamedRulesOn on;
	for (ZombieType t : {ZOMBIE_NORMAL, ZOMBIE_DUCKY_TUBE, ZOMBIE_TRAFFIC_CONE, ZOMBIE_PAIL, ZOMBIE_POLEVAULTER, ZOMBIE_FLAG})
		for (bool sky : {false, true}) {
			CHECK(EliteRollRange(t, sky, eo::DEFAULT) == EliteRollRange(t, sky));
			for (int world : {1, 7})
				for (int r = 0; r < 30; ++r)
					CHECK(RollEliteVariant(t, r, world, sky, eo::DEFAULT) == RollEliteVariant(t, r, world, sky));
		}
}

TEST_CASE("StartPositionAndHp") {
	UnnamedRulesOn on;
	CHECK(StartOffsetPx(ZOMBIE_NORMAL) == 133.0f);
	CHECK(StartOffsetPx(ZOMBIE_PAIL) == 133.0f);
	CHECK(StartOffsetPx(ZOMBIE_JACK_IN_THE_BOX) == 0.0f);
	CHECK(StartOffsetPx(ZOMBIE_JALAPENO_HEAD) == 0.0f);
	CHECK(PoleStartBaseX(870) == 955);
	CHECK(ZombieStartBaseX(ZOMBIE_ZAMBONI, 800) == 933);
	CHECK(ZombieStartBaseX(ZOMBIE_CATAPULT, 825) == 958);
	CHECK(ZombieStartBaseX(ZOMBIE_GARGANTUAR, 845) == 978);
	CHECK(ZombieStartBaseX(ZOMBIE_REDEYE_GARGANTUAR, 845) == 978);
	CHECK(ZombieStartBaseX(ZOMBIE_NORMAL, 780) == 780);
	CHECK(ZombieStartBaseX(ZOMBIE_DOLPHIN_RIDER, 780) == 780); // no own start x: the generic one (+133)
	CHECK(ZombieStartBodyHp(ZOMBIE_NORMAL, elite::NONE, 270) == 270);
	CHECK(ZombieStartBodyHp(ZOMBIE_NORMAL, elite::STICKY_NOTE, 270) == 640);
	CHECK(ZombieStartBodyHp(ZOMBIE_DUCKY_TUBE, elite::STICKY_NOTE, 270) == 640);
	CHECK(ZombieStartBodyHp(ZOMBIE_NORMAL, elite::SILVER_COIN, 270) == 270);
	CHECK(ZombieStartBodyHp(ZOMBIE_FLAG, elite::NONE, 270) == 640);
	CHECK(ZombieStartBodyHp(ZOMBIE_PAIL, elite::PUMPKIN, 270) == 1100);
	CHECK(ZombieStartBodyHp(ZOMBIE_PAIL, elite::UMBRELLA, 270) == 270);
	CHECK(ZombieStartBodyHp(ZOMBIE_TRAFFIC_CONE, elite::PHONOGRAPH, 270) == 270);
	CHECK(ZombieStartBodyHp(ZOMBIE_POLEVAULTER, elite::NONE, 500) == 870);
	CHECK(ZombieStartBodyHp(ZOMBIE_POLEVAULTER, elite::GLOVE, 500) == 870);
	CHECK(EliteAttackRectExtraPx(elite::GLOVE) == 112);
	CHECK(EliteAttackRectExtraPx(elite::VASE_POLE) == 0);
}

TEST_CASE("SpeedAndEating") {
	UnnamedRulesOn on;
	CHECK(EliteAnimRateMultiplier(ZOMBIE_NORMAL, elite::STICKY_NOTE) == doctest::Approx(3.0f));
	CHECK(EliteAnimRateMultiplier(ZOMBIE_PAIL, elite::PUMPKIN) == doctest::Approx(0.33333331f));
	CHECK(EliteAnimRateMultiplier(ZOMBIE_NORMAL, elite::NONE) == 1.0f);
	CHECK(EliteAnimRateMultiplier(ZOMBIE_NORMAL, elite::SILVER_COIN) == 1.0f);
	CHECK(EliteAnimRateMultiplier(ZOMBIE_BOSS, elite::STICKY_NOTE) == 1.0f);
	CHECK_FALSE(EliteEatsPlants(elite::STICKY_NOTE));
	CHECK(EliteEatsPlants(elite::NONE));
	CHECK(EliteEatsPlants(elite::SILVER_COIN));
	CHECK(EliteShowsYuckyFace(elite::NONE));
	CHECK_FALSE(EliteShowsYuckyFace(elite::STICKY_NOTE));
	CHECK_FALSE(EliteShowsYuckyFace(elite::SILVER_COIN));
}

TEST_CASE("FlagBitesThreeOfFourTicks") {
	UnnamedRulesOn on;
	int bites4 = 0, bites8 = 0;
	for (int age = 0; age < 8; ++age) {
		CHECK(BitesThisTick(ZOMBIE_NORMAL, age, 4) == (age % 4 == 0));
		bites4 += BitesThisTick(ZOMBIE_FLAG, age, 4);
		bites8 += BitesThisTick(ZOMBIE_FLAG, age, 8);
	}
	CHECK_FALSE(BitesThisTick(ZOMBIE_FLAG, 0, 4));
	CHECK(BitesThisTick(ZOMBIE_FLAG, 1, 4));
	CHECK(BitesThisTick(ZOMBIE_FLAG, 3, 4));
	CHECK(bites4 == 6);   // 3 of every 4
	CHECK(BitesThisTick(ZOMBIE_FLAG, 5, 8));
	CHECK_FALSE(BitesThisTick(ZOMBIE_FLAG, 6, 8));
	CHECK(bites8 == 5);   // chilled: 5 of every 8
}

TEST_CASE("FlagSunDrain") {
	UnnamedRulesOn on;
	CHECK(FlagDrainsThisTick(ZOMBIE_FLAG, 0));
	CHECK(FlagDrainsThisTick(ZOMBIE_FLAG, 40));
	CHECK_FALSE(FlagDrainsThisTick(ZOMBIE_FLAG, 39));
	CHECK_FALSE(FlagDrainsThisTick(ZOMBIE_FLAG, 41));
	CHECK_FALSE(FlagDrainsThisTick(ZOMBIE_NORMAL, 40));
	CHECK(FlagSunDrainRollRange() == 3);
	CHECK(FlagSunDrain(0) == 1);
	CHECK(FlagSunDrain(2) == 3);
	CHECK(FlagDrainAllowedIn(GAMEMODE_ADVENTURE, false, true, 4));
	CHECK_FALSE(FlagDrainAllowedIn(GAMEMODE_ADVENTURE, false, true, 55));
	CHECK(FlagDrainAllowedIn(GAMEMODE_SURVIVAL_NORMAL_STAGE_1, false, false, 55));
	CHECK_FALSE(FlagDrainAllowedIn(GAMEMODE_CHALLENGE_LAST_STAND, false, false, 0));
	CHECK_FALSE(FlagDrainAllowedIn(GAMEMODE_SCARY_POTTER_ENDLESS, false, false, 0));
	CHECK_FALSE(FlagDrainAllowedIn(GAMEMODE_PUZZLE_I_ZOMBIE_1, true, false, 0));
}

TEST_CASE("AurasPulseEvery650") {
	UnnamedRulesOn on;
	CHECK(HasAura(elite::PHONOGRAPH));
	CHECK(HasAura(elite::PUMPKIN));
	CHECK_FALSE(HasAura(elite::DANCER_HEAD));
	int counter = 0, pulses = 0, firstPulse = -1;
	for (int tick = 1; tick <= 1300; ++tick)
		if (AuraTick(counter)) { ++pulses; if (firstPulse < 0) firstPulse = tick; }
	CHECK(firstPulse == 650);
	CHECK(pulses == 2);
	CHECK(InAura(2, 400, 2, 600));
	CHECK(InAura(2, 400, 1, 200));
	CHECK(InAura(2, 400, 3, 400));
	CHECK_FALSE(InAura(2, 400, 2, 601));
	CHECK_FALSE(InAura(2, 400, 4, 400));
	CHECK_FALSE(InAura(2, 400, 0, 400));
	CHECK(AuraSkips(ZOMBIE_BOSS, 40000, elite::NONE));
	CHECK(AuraSkips(ZOMBIE_NORMAL, 120001, elite::NONE));
	CHECK_FALSE(AuraSkips(ZOMBIE_NORMAL, 120000, elite::NONE));
	CHECK(AuraSkips(ZOMBIE_NEWSPAPER, 1500, 0x7F));
	for (ZombieType t : {ZOMBIE_NORMAL, ZOMBIE_FLAG, ZOMBIE_TRAFFIC_CONE, ZOMBIE_PAIL, ZOMBIE_DOOR})
		CHECK(PumpkinGivesBucketTo(t, HELMTYPE_NONE));
	CHECK_FALSE(PumpkinGivesBucketTo(ZOMBIE_POLEVAULTER, HELMTYPE_NONE));
	CHECK_FALSE(PumpkinGivesBucketTo(ZOMBIE_NEWSPAPER, HELMTYPE_NONE));
	CHECK_FALSE(PumpkinGivesBucketTo(ZOMBIE_TRAFFIC_CONE, HELMTYPE_TRAFFIC_CONE));
	CHECK_FALSE(PumpkinGivesBucketTo(ZOMBIE_PAIL, HELMTYPE_PAIL));
	CHECK(PumpkinBucketHp() == 1100);
}

TEST_CASE("DeathEffects") {
	UnnamedRulesOn on;
	CHECK(SummonsDancersOnDeath(elite::DANCER_HEAD));
	CHECK_FALSE(SummonsDancersOnDeath(elite::PHONOGRAPH));
	CHECK(SkyDropsOnDeath(elite::UMBRELLA));
	CHECK_FALSE(SkyDropsOnDeath(elite::PUMPKIN));
	CHECK(DeathEffectAllowed(HEIGHT_ZOMBIE_NORMAL, 0));
	CHECK_FALSE(DeathEffectAllowed(HEIGHT_GETTING_BUNGEE_DROPPED, 3));
	CHECK_FALSE(DeathEffectAllowed(HEIGHT_ZOMBIE_NORMAL, -1));
	CHECK(SkyDropTypeRollRange() == 25);
	int accepted = 0;
	for (int r = 0; r < 25; ++r) accepted += SkyDropTypeAccepted(r);
	CHECK(accepted == 13);
	CHECK(SkyDropTypeAccepted(0));
	CHECK(SkyDropTypeAccepted(7));
	CHECK_FALSE(SkyDropTypeAccepted(8));
	CHECK_FALSE(SkyDropTypeAccepted(14));
	CHECK(SkyDropTypeAccepted(15));
	CHECK_FALSE(SkyDropTypeAccepted(20));
	CHECK(SkyDropTypeAccepted(21));
	CHECK(SkyDropTypeAccepted(24));
}

TEST_CASE("SilverCoin") {
	UnnamedRulesOn on;
	CHECK(CoinSquishes(ZOMBIE_NORMAL, elite::SILVER_COIN, false, true));
	CHECK(CoinSquishes(ZOMBIE_NORMAL, elite::SILVER_COIN, true, false));
	CHECK_FALSE(CoinSquishes(ZOMBIE_NORMAL, elite::SILVER_COIN, true, true));
	CHECK_FALSE(CoinSquishes(ZOMBIE_NORMAL, elite::NONE, false, false));
	CHECK_FALSE(CoinSquishes(ZOMBIE_DUCKY_TUBE, elite::SILVER_COIN, false, false));
	CHECK(CoinCopiesOnBite(ZOMBIE_NORMAL, elite::SILVER_COIN, 8));
	CHECK(CoinCopiesOnBite(ZOMBIE_NORMAL, elite::SILVER_COIN, 1));
	CHECK_FALSE(CoinCopiesOnBite(ZOMBIE_NORMAL, elite::SILVER_COIN, 9));
	CHECK_FALSE(CoinCopiesOnBite(ZOMBIE_NORMAL, elite::NONE, 4));
	CHECK_FALSE(CoinCopiesOnBite(ZOMBIE_FLAG, elite::SILVER_COIN, 4));
	CHECK_FALSE(CoinCopyNeedsFootballRoll(10));
	CHECK_FALSE(CoinCopyNeedsFootballRoll(60));
	CHECK(CoinCopyNeedsFootballRoll(61));
	CHECK(CoinCopyFootballRollRange() == 10);
	CHECK(CoinCopyIsFootball(0));
	CHECK_FALSE(CoinCopyIsFootball(1));
	CHECK_FALSE(CoinCopyIsFootball(9));
}

TEST_CASE("EliteArtTable") {
	auto sticky = EliteArtFor(ZOMBIE_NORMAL, elite::STICKY_NOTE);
	REQUIRE(sticky.images.size() == 1);
	CHECK(sticky.images[0] == EliteArtImage{"anim_head1", "images/ZombieNoteSmall"});
	CHECK(sticky.hidePrefixes == std::vector<std::string>{"anim_head2"});
	CHECK(EliteArtFor(ZOMBIE_NORMAL, elite::SILVER_COIN).images[0].file == "reanim/Coin_silver_dollar");
	// 2026-11-22 "coin head has some hairs": Unnamed hides only the jaw for every head elite but the umbrella
	// (shared path 0x794A3C..0x794A6B: ReanimShowPrefix("anim_head2") only; anim_hair is hidden only at 0x794814,
	// the umbrella). The hair stays, as in Unnamed.
	for (auto [t, v] : {std::pair{ZOMBIE_NORMAL, elite::STICKY_NOTE}, {ZOMBIE_NORMAL, elite::SILVER_COIN},
	                    {ZOMBIE_TRAFFIC_CONE, elite::PHONOGRAPH}, {ZOMBIE_TRAFFIC_CONE, elite::DANCER_HEAD},
	                    {ZOMBIE_PAIL, elite::PUMPKIN}, {ZOMBIE_POLEVAULTER, elite::GLOVE}})
		CHECK(EliteArtFor(t, v).hidePrefixes == std::vector<std::string>{"anim_head2"});
	CHECK(EliteArtFor(ZOMBIE_PAIL, elite::UMBRELLA).hidePrefixes == std::vector<std::string>{"anim_hair", "anim_head2"});
	CHECK(EliteArtFor(ZOMBIE_TRAFFIC_CONE, elite::PHONOGRAPH).images[0].file == "images/Phonograph");
	CHECK(EliteArtFor(ZOMBIE_TRAFFIC_CONE, elite::DANCER_HEAD).images[0].file == "particles/ZombieDancerHead");
	CHECK(EliteArtFor(ZOMBIE_PAIL, elite::PUMPKIN).images[0].file == "reanim/Pumpkin_damage1");
	auto umbrella = EliteArtFor(ZOMBIE_PAIL, elite::UMBRELLA);
	CHECK(umbrella.umbrellaLeaf);
	CHECK(umbrella.images[0] == EliteArtImage{"anim_head1", "IMAGE_BLANK"});
	CHECK(EliteArtFor(ZOMBIE_POLEVAULTER, elite::GLOVE).images[0].file == "images/Zen_GardenGlove");
	auto vase = EliteArtFor(ZOMBIE_POLEVAULTER, elite::VASE_POLE);
	REQUIRE(vase.images.size() == 2);
	CHECK(vase.images[0] == EliteArtImage{"Zombie_polevaulter_pole2", "reanim/Zombie_gargantuar_telephonepole"});
	CHECK(vase.images[1] == EliteArtImage{"Zombie_polevaulter_pole", "particles/Zombie_seaweed"});
	CHECK(vase.hidePrefixes.empty());
	CHECK(EliteArtFor(ZOMBIE_POLEVAULTER, elite::ROAD_SIGN).images[0].file == "reanim/Zombie_gargantuar_duckxing");
	CHECK(EliteArtFor(ZOMBIE_NORMAL, elite::NONE).empty());
	CHECK(EliteArtFor(ZOMBIE_FLAG, elite::NONE).empty());
}

TEST_CASE("EliteWithoutArtDrawsBase") {
	UnnamedRulesOn on;
	auto none = [](const std::string&) { return false; };
	auto all = [](const std::string&) { return true; };
	CHECK(ResolveEliteArt(ZOMBIE_NORMAL, elite::STICKY_NOTE, none).empty()); // no override, jaw stays: base zombie
	CHECK(ResolveEliteArt(ZOMBIE_NORMAL, elite::STICKY_NOTE, all).images.size() == 1);
	// vase pole: one of its two images missing -> base pole, not half an elite
	auto onlyPole = [](const std::string& f) { return f == "reanim/Zombie_gargantuar_telephonepole"; };
	CHECK(ResolveEliteArt(ZOMBIE_POLEVAULTER, elite::VASE_POLE, onlyPole).empty());
	// IMAGE_BLANK is the engine's own and never asked for
	auto umbrellaFiles = [](const std::string& f) { REQUIRE(f != "IMAGE_BLANK"); return true; };
	CHECK(ResolveEliteArt(ZOMBIE_PAIL, elite::UMBRELLA, umbrellaFiles).umbrellaLeaf);
	// the behaviour does not depend on the art
	CHECK_FALSE(EliteEatsPlants(elite::STICKY_NOTE));
	CHECK(ZombieStartBodyHp(ZOMBIE_NORMAL, elite::STICKY_NOTE, 270) == 640);
}

TEST_CASE("GloveLeapVanillaWithoutRules") {
	SetRules(nlohmann::json::object());
	for (auto c : {PlantDeathCause::EATEN, PlantDeathCause::SHOVEL, PlantDeathCause::BLAST, PlantDeathCause::OTHER})
		CHECK_FALSE(PlantDeathWakesGlove(c));
	GloveLeapCheck c{ZOMBIE_POLEVAULTER, elite::GLOVE, PHASE_POLEVAULTER_POST_VAULT, false, 2, 500, 2, 300};
	CHECK_FALSE(GloveLeapsFor(c));
	CHECK_FALSE(GloveLandingKeepsPole(elite::GLOVE, 1));
}

TEST_CASE("GloveLeapWhichDeathsWakeIt") {
	UnnamedRulesOn on;
	// exe hook 361 runs only while the caller's EBX <= 0x1000000 (the research notes (v1-zombie-contradictions) 1)
	CHECK(PlantDeathWakesGlove(PlantDeathCause::EATEN));
	CHECK(PlantDeathWakesGlove(PlantDeathCause::SHOVEL));
	CHECK(PlantDeathWakesGlove(PlantDeathCause::BLAST));
	CHECK(PlantDeathWakesGlove(PlantDeathCause::REPLACED));
	CHECK(PlantDeathWakesGlove(PlantDeathCause::POOL_SQUASH));
	CHECK(PlantDeathWakesGlove(PlantDeathCause::LEAP_BITE));
	CHECK_FALSE(PlantDeathWakesGlove(PlantDeathCause::OTHER)); // ash plants, Gargantuar / Zomboni squish, Bungee
}

TEST_CASE("GloveLeapWhoLeaps") {
	UnnamedRulesOn on;
	const GloveLeapCheck base{ZOMBIE_POLEVAULTER, elite::GLOVE, PHASE_POLEVAULTER_POST_VAULT, false, 2, 500, 2, 300};
	CHECK(GloveLeapsFor(base));
	auto with = [&](auto f) { GloveLeapCheck c = base; f(c); return GloveLeapsFor(c); };
	CHECK(with([](GloveLeapCheck& c) { c.phase = PHASE_POLEVAULTER_PRE_VAULT; }));    // still has its pole
	CHECK_FALSE(with([](GloveLeapCheck& c) { c.phase = PHASE_POLEVAULTER_IN_VAULT; })); // mid-vault (phase 12)
	CHECK_FALSE(with([](GloveLeapCheck& c) { c.variant = elite::VASE_POLE; }));
	CHECK_FALSE(with([](GloveLeapCheck& c) { c.variant = elite::NONE; }));
	CHECK_FALSE(with([](GloveLeapCheck& c) { c.type = ZOMBIE_NORMAL; }));
	CHECK_FALSE(with([](GloveLeapCheck& c) { c.deadOrDying = true; }));
	CHECK_FALSE(with([](GloveLeapCheck& c) { c.plantRow = 3; }));
	CHECK_FALSE(with([](GloveLeapCheck& c) { c.zombieX = 300; })); // `jle`: must stand strictly right of the plant
	CHECK(with([](GloveLeapCheck& c) { c.zombieX = 301; }));
	CHECK(with([](GloveLeapCheck& c) { c.plantX = 0; }));           // any plant to its left in the row
}

TEST_CASE("GloveLeapWhereAndHow") {
	UnnamedRulesOn on;
	// 0x95ae7e: posX = plant.mX + 96; with its pole (phase 11): +0x148 = 1, mVelX 0; else +0x148 = 2, mVelX 0.5
	CHECK(GloveLeapAt(320, PHASE_POLEVAULTER_PRE_VAULT) == GloveLeap{416.0f, 1, 0.0f, 800});
	CHECK(GloveLeapAt(320, PHASE_POLEVAULTER_POST_VAULT) == GloveLeap{416.0f, 2, 0.5f, 800});
	CHECK(GloveLeapAt(320, PHASE_ZOMBIE_NORMAL) == GloveLeap{416.0f, 2, 0.5f, 800});
	CHECK(GloveLandingKeepsPole(elite::GLOVE, 1));
	CHECK_FALSE(GloveLandingKeepsPole(elite::GLOVE, 2));
	CHECK_FALSE(GloveLandingKeepsPole(elite::GLOVE, 0));
	CHECK_FALSE(GloveLandingKeepsPole(elite::VASE_POLE, 1));
}

TEST_CASE("ForcedVariantFlag") {
	CHECK(ParseForcedVariant("PAIL:4") == ForcedVariant{"PAIL", 4});
	CHECK(ParseForcedVariant("normal:0x105") == ForcedVariant{"NORMAL", 0x105});
	CHECK(ParseForcedVariant("POLEVAULTER:50") == ForcedVariant{"POLEVAULTER", 50});
	CHECK(ParseForcedVariant("ZOMBIE_TRAFFIC_CONE:3") == ForcedVariant{"TRAFFIC_CONE", 3});
	CHECK_FALSE(ParseForcedVariant("PAIL").has_value());
	CHECK_FALSE(ParseForcedVariant(":4").has_value());
	CHECK_FALSE(ParseForcedVariant("PAIL:x").has_value());
	CHECK_FALSE(ParseForcedVariant("PAIL:-1").has_value());
}

TEST_CASE("PackRulesMatchTheseValues") {
	// Every key the code reads is in the test values (so a new key cannot be forgotten in the pack)...
	for (const auto& k : DayZombieRuleKeys()) CHECK_MESSAGE(kUnnamedRules.contains(k), k);
	CHECK(DayZombieRuleKeys().size() == kUnnamedRules.size());
	// ...and the pack file says the same, when the pack is on this machine.
	const char* home = std::getenv("HOME");
	fs::path file = std::getenv("PVZU_PACK_DIR") ? fs::path(std::getenv("PVZU_PACK_DIR"))
	                                              : fs::path(home ? home : "") / "pvz-unnamed-pack";
	file /= "data/rules-day-zombies.json";
	if (!fs::exists(file)) { MESSAGE("pack not here, skipped: " << file.string()); return; }
	std::ifstream in(file);
	nlohmann::json pack = nlohmann::json::parse(in, nullptr, false);
	REQUIRE(pack.is_object());
	CHECK(pack == kUnnamedRules);
}
