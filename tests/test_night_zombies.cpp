// PVZU: Plan 3 Task 2, Night zombies and elite forms (pure rules; evidence in the research notes (w2-night) section 2 and
// pvz-unnamed-pack/SOURCES-night-zombies.md).
#include "doctest/doctest.h"
#include "Mod/Unnamed/NightZombies.h"
#include "Mod/Unnamed/Options.h"
#include "Mod/Unnamed/ReanimCompiled32.h"
#include "Mod/Unnamed/Rules.h"
#include "Mod/Unnamed/State.h"

#include "nlohmann/json.hpp"

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <random>

using namespace pvzu::unnamed;
namespace fs = std::filesystem;
namespace ne = pvzu::unnamed::nightelite;
namespace ns = pvzu::unnamed::nightsub;
namespace eo = pvzu::unnamed::eliteoption;

namespace {

// The Unnamed values (same as pvz-unnamed-pack/data/rules-night-zombies.json; the last test compares the two).
const nlohmann::json kNightRules = {
	{"nightZombies.newspaper.bodyHp", 500},
	{"nightZombies.newspaper.knifeBodyHp", 1500},
	{"nightZombies.newspaper.eliteRollRange", 5},
	{"nightZombies.newspaper.eliteRollRangeAllElite", 4},
	{"nightZombies.newspaper.footballHeadMaxRoll", 1},
	{"nightZombies.newspaper.almanacRoll", 2},
	{"nightZombies.newspaper.knifeRoll", 3},
	{"nightZombies.newspaper.almanacAfterPlayerLevel", 60},
	{"nightZombies.newspaper.almanacPaperHp", 300},
	{"nightZombies.newspaper.almanacBodyPerPaperHit", 15},
	{"nightZombies.newspaper.footballHeadWalkMul", 1.5},
	{"nightZombies.newspaper.footballHeadPaperSoaksBody", 1},
	{"nightZombies.newspaper.madChecksPerTick", 3},
	{"nightZombies.newspaper.madBiteRemainderMin", 1},
	{"nightZombies.newspaper.madBiteRemainderMax", 5},
	{"nightZombies.newspaper.madButterCs", 500},
	{"nightZombies.newspaper.eatZombieDamage", 13},
	{"nightZombies.newspaper.eatBossDamage", 2},
	{"nightZombies.knife.madAnimDiv", 3.0},
	{"nightZombies.knife.killDelayCs", 32},
	{"nightZombies.knife.skipsNormalBite", 1},
	{"nightZombies.eatZombie.damageFlags", 8},
	{"nightZombies.door.tallnutRollRange", 5},
	{"nightZombies.door.reflect", 1},
	{"nightZombies.door.reflectCostHp", 1},
	{"nightZombies.door.tallnutShieldFirst", 1},
	{"nightZombies.football.helmHp", 2800},
	{"nightZombies.football.greenRollRange", 4},
	{"nightZombies.football.greenBodyHp", 300},
	{"nightZombies.football.greenHelmHp", 3000},
	{"nightZombies.football.goldRollRange", 13},
	{"nightZombies.football.goldBodyHp", 540},
	{"nightZombies.football.goldHelmHp", 1260},
	{"nightZombies.football.greenGoldBodyHp", 600},
	{"nightZombies.football.greenGoldHelmHp", 1400},
	{"nightZombies.football.extraRollRange", 3},
	{"nightZombies.football.allEliteRoll", 16},
	{"nightZombies.football.allEliteGreenBelow", 13},
	{"nightZombies.football.allEliteRollAbove60", 40},
	{"nightZombies.football.allEliteGreenMaxAbove60", 13},
	{"nightZombies.football.allEliteBlackExtraBelow", 1},
	{"nightZombies.football.goldWalkMul", 1.5},
	{"nightZombies.football.goldSquishes", 1},
	{"nightZombies.football.greenBiteRemainderMax", 1},
	{"nightZombies.football.greenWalkBase", 1.1},
	{"nightZombies.football.greenWalkHelmK", 2.0},
	{"nightZombies.football.greenWalkBodyK", 1.57},
	{"nightZombies.football.greenHelmDropAgeCs", 800},
	{"nightZombies.football.greenHelmDropMaxPlayerLevel", 70},
	{"nightZombies.football.greenChewBodyBonus", 1000},
	{"nightZombies.football.greenChewBonusMaxPlayerLevel", 60},
	{"nightZombies.dancer.bodyHp", 1000},
	{"nightZombies.dancer.eliteRollRange", 60},
	{"nightZombies.dancer.eliteRollRangeAllElite", 15},
	{"nightZombies.dancer.randomFollowerMaxRoll", 2},
	{"nightZombies.dancer.trophyMaxRoll", 6},
	{"nightZombies.dancer.moonwalkMaxRoll", 14},
	{"nightZombies.dancer.trophyHpLoss", 500},
	{"nightZombies.dancer.randomFollowerDanceInCs", 3000},
	{"nightZombies.dancer.randomFollowerRollRange", 33},
	{"nightZombies.dancer.randomFollowerJalapenoAfterPlayerLevel", 70},
	{"nightZombies.dancer.snapWithoutCountdown", 1},
	{"nightZombies.dancer.resummonAll", 1},
	{"nightZombies.dancer.moonwalkVelX", 0.5},
	{"nightZombies.dancer.moonwalkAnimRate", 24.0},
	{"nightZombies.dancer.randomSummonerWhite", 1},
};

struct NightRulesOn {
	NightRulesOn() { SetRules(kNightRules); }
	~NightRulesOn() { SetRules(nlohmann::json::object()); }
};

NightForm F(int v, int s = 0) { return NightForm{v, s}; }

} // namespace

TEST_CASE("NightVanillaWithoutRules") {
	SetRules(nlohmann::json::object());
	CHECK(NightRollRange(ZOMBIE_NEWSPAPER, false) == 0);
	CHECK(NightRollRange(ZOMBIE_DOOR, false) == 0);
	CHECK(NightRollRange(ZOMBIE_FOOTBALL, false) == 0);
	CHECK(NightRollRange(ZOMBIE_DANCER, false) == 0);
	CHECK(FootballGoldRange() == 0);
	CHECK(FootballExtraRange() == 0);
	CHECK(NightBodyHp(ZOMBIE_NEWSPAPER, F(0), 270) == 270);
	CHECK(NightBodyHp(ZOMBIE_DANCER, F(0), 500) == 500);
	CHECK(NightHelmHp(ZOMBIE_FOOTBALL, F(0), 1400) == 1400);
	CHECK(NightWalkRateMultiplier(ZOMBIE_NEWSPAPER, F(0, ns::FOOTBALL_HEAD), 500, 500, 0, 0) == doctest::Approx(1.0f));
	CHECK_FALSE(NightBitesThisTick(ZOMBIE_NEWSPAPER, F(0), PHASE_NEWSPAPER_MAD, false, 1).has_value());
	CHECK(CheckIfPreyCaughtCallsPerTick(PHASE_NEWSPAPER_MAD) == 1);
	CHECK_FALSE(DancerSnapsWithoutCountdown());
	CHECK_FALSE(DancerResummonsAll());
	CHECK_FALSE(DoorReflects(SHIELDTYPE_DOOR, 0, PROJECTILE_PEA, MOTION_STRAIGHT));
	CHECK(EatZombieDamage(ZOMBIE_NEWSPAPER, PHASE_NEWSPAPER_MAD, ZOMBIE_NORMAL, 4) == 4);
	CHECK(EatZombieFlags(9U) == 9U);
	CHECK(MadButterCs(ZOMBIE_NEWSPAPER, PHASE_NEWSPAPER_MAD, 400) == 400);
	CHECK_FALSE(SkipsNormalBite(ZOMBIE_NEWSPAPER, F(ne::KNIFE)));
}

TEST_CASE("NewspaperRollBoundaries") {
	NightRulesOn on;
	CHECK(NightRollRange(ZOMBIE_NEWSPAPER, false) == 5);
	CHECK(RollNewspaper(0, false) == F(0, ns::FOOTBALL_HEAD));
	CHECK(RollNewspaper(1, false) == F(0, ns::FOOTBALL_HEAD));
	CHECK(RollNewspaper(2, false) == F(0));                // first-time world 2: falls back to plain
	CHECK(RollNewspaper(2, true) == F(ne::ALMANAC_PAPER));
	CHECK(RollNewspaper(3, false) == F(ne::KNIFE));
	CHECK(RollNewspaper(4, false) == F(0));
	// Almanac paper gate: player level > 60 or a finished adventure, never in level 70
	CHECK_FALSE(NewspaperAlmanacAllowed(11, 0, true, 11));
	CHECK_FALSE(NewspaperAlmanacAllowed(60, 0, true, 11));
	CHECK(NewspaperAlmanacAllowed(61, 0, true, 11));
	CHECK(NewspaperAlmanacAllowed(11, 1, true, 11));
	CHECK_FALSE(NewspaperAlmanacAllowed(71, 1, true, 70));
}

TEST_CASE("DoorFootballDancerRollBoundaries") {
	NightRulesOn on;
	CHECK(NightRollRange(ZOMBIE_DOOR, false) == 5);
	CHECK(NightRollRange(ZOMBIE_DOOR, true) == 0);   // not in I, Zombie
	CHECK(RollDoor(0) == F(ne::TALLNUT_DOOR));
	CHECK(RollDoor(1) == F(0));
	CHECK(RollDoor(4) == F(0));

	CHECK(FootballGreenRange() == 4);
	CHECK(FootballGoldRange() == 13);
	CHECK(FootballExtraRange() == 3);
	CHECK(RollFootball(0, 1) == F(0, ns::GREEN));
	CHECK(RollFootball(1, 0) == F(ne::GOLD_FOOTBALL));
	CHECK(RollFootball(0, 0) == F(ne::GOLD_FOOTBALL, ns::GREEN));
	CHECK(RollFootball(3, 12) == F(0));
	CHECK(FootballDrawsExtraRoll(F(0)));
	CHECK(FootballDrawsExtraRoll(F(0, ns::GREEN)));
	CHECK_FALSE(FootballDrawsExtraRoll(F(ne::GOLD_FOOTBALL)));

	CHECK(NightRollRange(ZOMBIE_DANCER, false) == 60);
	CHECK(RollDancer(0, false) == F(ne::RANDOM_SUMMONER));
	CHECK(RollDancer(2, false) == F(ne::RANDOM_SUMMONER));
	CHECK(RollDancer(3, false) == F(ne::TROPHY));
	CHECK(RollDancer(6, false) == F(ne::TROPHY));
	CHECK(RollDancer(7, false) == F(ne::MOONWALKER));
	CHECK(RollDancer(14, false) == F(ne::MOONWALKER));
	CHECK(RollDancer(15, false) == F(0));
	CHECK(RollDancer(59, false) == F(0));
	CHECK(RollDancer(5, true) == F(0));                    // I, Zombie: only the random summoner
	CHECK(RollDancer(1, true) == F(ne::RANDOM_SUMMONER));
	CHECK(NightRollRange(ZOMBIE_BACKUP_DANCER, false) == 0);
}

TEST_CASE("NightEliteRatesOver2000Rolls") {
	NightRulesOn on;
	std::mt19937 gen(4242);
	auto roll = [&](int n) { return std::uniform_int_distribution<int>(0, n - 1)(gen); };
	const int N = 2000;
	auto within = [&](int count, double p) { // 4 standard deviations
		const double sd = std::sqrt(N * p * (1 - p));
		CHECK_MESSAGE(std::abs(count - N * p) <= 4 * sd, count << " vs " << N * p);
	};
	std::map<NightForm, int, bool (*)(const NightForm&, const NightForm&)> seen(
		[](const NightForm& a, const NightForm& b) { return a.variant != b.variant ? a.variant < b.variant : a.subVariant < b.subVariant; });
	for (int i = 0; i < N; ++i) seen[RollNewspaper(roll(5), false)]++;
	within(seen[F(0, ns::FOOTBALL_HEAD)], 0.4);
	within(seen[F(ne::KNIFE)], 0.2);
	within(seen[F(0)], 0.4);
	seen.clear();
	for (int i = 0; i < N; ++i) seen[RollDoor(roll(5))]++;
	within(seen[F(ne::TALLNUT_DOOR)], 0.2);
	seen.clear();
	for (int i = 0; i < N; ++i) seen[RollFootball(roll(4), roll(13))]++;
	within(seen[F(0, ns::GREEN)], 0.25 * 12 / 13);
	within(seen[F(ne::GOLD_FOOTBALL)], 0.75 / 13);
	within(seen[F(ne::GOLD_FOOTBALL, ns::GREEN)], 0.25 / 13);
	within(seen[F(0)], 0.75 * 12 / 13);
	seen.clear();
	for (int i = 0; i < N; ++i) seen[RollDancer(roll(60), false)]++;
	within(seen[F(ne::RANDOM_SUMMONER)], 1.0 / 20);
	within(seen[F(ne::TROPHY)], 1.0 / 15);
	within(seen[F(ne::MOONWALKER)], 2.0 / 15);
	within(seen[F(0)], 46.0 / 60);
}

// The elite option "all" (exe 0x800258 == 2; pvz-unnamed-pack SOURCES-shop-ui.md). Newspaper 0xc05770 Rand(4) and the
// Almanac paper without its level gate (0x7a1a16), Dancer 0x79683c Rand(15) and trophy/moonwalker in I, Zombie too
// (0xc043eb), Football 0xc12afd .. 0x9dd419 (below); Screen Door: 0x7b2000 turns the jump at 0x796166 into `jo`, so
// the Rand(5) is drawn and always hits.
TEST_CASE("NightAllElitesOver2000Rolls") {
	NightRulesOn on;
	std::mt19937 gen(4242);
	auto roll = [&](int n) { return std::uniform_int_distribution<int>(0, n - 1)(gen); };
	const int N = 2000;
	auto within = [&](int count, double p) { // 4 standard deviations
		const double sd = std::sqrt(N * p * (1 - p));
		CHECK_MESSAGE(std::abs(count - N * p) <= 4 * sd, count << " vs " << N * p);
	};
	CHECK(NightRollRange(ZOMBIE_NEWSPAPER, false, eo::ALL) == 4);
	CHECK(NightRollRange(ZOMBIE_DOOR, false, eo::ALL) == 5);
	CHECK(NightRollRange(ZOMBIE_DANCER, false, eo::ALL) == 15);
	CHECK(NightRollRange(ZOMBIE_DOOR, true, eo::ALL) == 0); // I, Zombie still skips the door (0x796152)
	for (ZombieType t : {ZOMBIE_NEWSPAPER, ZOMBIE_DOOR, ZOMBIE_DANCER, ZOMBIE_FOOTBALL})
		CHECK(NightRollRange(t, false, eo::NONE) == 0);
	std::map<NightForm, int, bool (*)(const NightForm&, const NightForm&)> seen(
		[](const NightForm& a, const NightForm& b) { return a.variant != b.variant ? a.variant < b.variant : a.subVariant < b.subVariant; });
	for (int i = 0; i < N; ++i) seen[RollNewspaper(roll(NightRollRange(ZOMBIE_NEWSPAPER, false, eo::ALL)), false, eo::ALL)]++;
	within(seen[F(0, ns::FOOTBALL_HEAD)], 0.5);
	within(seen[F(ne::ALMANAC_PAPER)], 0.25);
	within(seen[F(ne::KNIFE)], 0.25);
	CHECK(seen[F(0)] == 0);
	seen.clear();
	for (int i = 0; i < N; ++i) seen[RollDoor(roll(NightRollRange(ZOMBIE_DOOR, false, eo::ALL)), eo::ALL)]++;
	CHECK(seen[F(ne::TALLNUT_DOOR)] == N);
	for (bool iZombie : {false, true}) {
		seen.clear();
		for (int i = 0; i < N; ++i) seen[RollDancer(roll(NightRollRange(ZOMBIE_DANCER, iZombie, eo::ALL)), iZombie, eo::ALL)]++;
		within(seen[F(ne::RANDOM_SUMMONER)], 3.0 / 15);
		within(seen[F(ne::TROPHY)], 4.0 / 15);
		within(seen[F(ne::MOONWALKER)], 8.0 / 15);
		CHECK(seen[F(0)] == 0);
	}
	// Football, profile level <= 60: Rand(16) < 13 green (then the gold Rand(13), the extra Rand(3) unless gold), else
	// gold with no more rolls.
	seen.clear();
	int black = 0;
	for (int i = 0; i < N; ++i) {
		const FootballAllEliteForm f = RollFootballAllElite(roll, false);
		seen[f.form]++;
		black += f.black ? 1 : 0;
	}
	within(seen[F(0, ns::GREEN)], 13.0 / 16 * 12 / 13);
	within(seen[F(ne::GOLD_FOOTBALL, ns::GREEN)], 13.0 / 16 / 13);
	within(seen[F(ne::GOLD_FOOTBALL)], 3.0 / 16);
	CHECK(seen[F(0)] == 0);
	CHECK(black == 0);
	// profile level > 60: Rand(40): 0 gold; 1..13 green as above; 14..39 the gold Rand(13): != 0 black, == 0 gold then
	// the extra Rand(3), 0 -> gold and black.
	seen.clear();
	black = 0;
	int goldBlack = 0;
	for (int i = 0; i < N; ++i) {
		const FootballAllEliteForm f = RollFootballAllElite(roll, true);
		seen[f.form]++;
		black += f.black ? 1 : 0;
		goldBlack += f.black && f.form.variant == ne::GOLD_FOOTBALL ? 1 : 0;
	}
	CHECK(seen[F(0)] == black - goldBlack); // every non-green, non-gold one is black
	within(black - goldBlack, 26.0 / 40 * 12 / 13);
	within(goldBlack, 26.0 / 40 / 13 / 3);
	within(seen[F(0, ns::GREEN)], 13.0 / 40 * 12 / 13);
	within(seen[F(ne::GOLD_FOOTBALL)], 1.0 / 40 + 26.0 / 40 / 13);
}

TEST_CASE("NightAllElitesExactRollSequences") {
	NightRulesOn on;
	auto seq = [](std::vector<int> rolls, std::vector<int>& ranges) {
		return [rolls, &ranges, i = size_t(0)](int n) mutable { ranges.push_back(n); return rolls.at(i++); };
	};
	std::vector<int> ranges;
	auto f = RollFootballAllElite(seq({13}, ranges), false); // gold at once, one draw
	CHECK(f.form == F(ne::GOLD_FOOTBALL));
	CHECK(ranges == std::vector<int>{16});
	ranges.clear();
	f = RollFootballAllElite(seq({12, 5, 2}, ranges), false); // green, gold roll misses, extra roll drawn
	CHECK(f.form == F(0, ns::GREEN));
	CHECK(ranges == std::vector<int>{16, 13, 3});
	ranges.clear();
	f = RollFootballAllElite(seq({0, 0}, ranges), false); // green + gold, no extra roll
	CHECK(f.form == F(ne::GOLD_FOOTBALL, ns::GREEN));
	CHECK(ranges == std::vector<int>{16, 13});
	ranges.clear();
	f = RollFootballAllElite(seq({0}, ranges), true); // above 60: 0 -> gold
	CHECK(f.form == F(ne::GOLD_FOOTBALL));
	CHECK_FALSE(f.black);
	CHECK(ranges == std::vector<int>{40});
	ranges.clear();
	f = RollFootballAllElite(seq({13, 4, 0}, ranges), true); // 13 is still green; the extra roll never blacks a green
	CHECK(f.form == F(0, ns::GREEN));
	CHECK_FALSE(f.black);
	CHECK(ranges == std::vector<int>{40, 13, 3});
	ranges.clear();
	f = RollFootballAllElite(seq({14, 1}, ranges), true); // black at once
	CHECK(f.form == F(0));
	CHECK(f.black);
	CHECK(ranges == std::vector<int>{40, 13});
	ranges.clear();
	f = RollFootballAllElite(seq({39, 0, 0}, ranges), true); // gold, extra 0 -> gold and black
	CHECK(f.form == F(ne::GOLD_FOOTBALL));
	CHECK(f.black);
	CHECK(ranges == std::vector<int>{40, 13, 3});
	ranges.clear();
	f = RollFootballAllElite(seq({39, 0, 1}, ranges), true);
	CHECK(f.form == F(ne::GOLD_FOOTBALL));
	CHECK_FALSE(f.black);
}

TEST_CASE("NightDefaultOptionMatchesTheOldRolls") {
	NightRulesOn on;
	for (ZombieType t : {ZOMBIE_NEWSPAPER, ZOMBIE_DOOR, ZOMBIE_DANCER, ZOMBIE_FOOTBALL})
		for (bool iz : {false, true})
			CHECK(NightRollRange(t, iz, eo::DEFAULT) == NightRollRange(t, iz));
	for (int r = 0; r < 60; ++r) {
		for (bool a : {false, true}) CHECK(RollNewspaper(r, a, eo::DEFAULT) == RollNewspaper(r, a));
		CHECK(RollDoor(r, eo::DEFAULT) == RollDoor(r));
		for (bool iz : {false, true}) CHECK(RollDancer(r, iz, eo::DEFAULT) == RollDancer(r, iz));
	}
}

TEST_CASE("NightHitPoints") {
	NightRulesOn on;
	CHECK(NightBodyHp(ZOMBIE_NEWSPAPER, F(0), 270) == 500);
	CHECK(NightBodyHp(ZOMBIE_NEWSPAPER, F(0, ns::FOOTBALL_HEAD), 270) == 500);
	CHECK(NightBodyHp(ZOMBIE_NEWSPAPER, F(ne::KNIFE), 270) == 1500);
	CHECK(NightShieldHp(ZOMBIE_NEWSPAPER, F(0), 150) == 150);
	CHECK(NightShieldHp(ZOMBIE_NEWSPAPER, F(ne::ALMANAC_PAPER), 150) == 300);
	CHECK(NightShieldHp(ZOMBIE_DOOR, F(ne::TALLNUT_DOOR), 1100) == 1100);
	CHECK(NightBodyHp(ZOMBIE_DOOR, F(ne::TALLNUT_DOOR), 270) == 270);
	CHECK(NightHelmHp(ZOMBIE_FOOTBALL, F(0), 1400) == 2800);
	CHECK(NightBodyHp(ZOMBIE_FOOTBALL, F(0), 270) == 270);
	CHECK(NightBodyHp(ZOMBIE_FOOTBALL, F(0, ns::GREEN), 270) == 300);
	CHECK(NightHelmHp(ZOMBIE_FOOTBALL, F(0, ns::GREEN), 1400) == 3000);
	CHECK(NightBodyHp(ZOMBIE_FOOTBALL, F(ne::GOLD_FOOTBALL), 270) == 540);
	CHECK(NightHelmHp(ZOMBIE_FOOTBALL, F(ne::GOLD_FOOTBALL), 1400) == 1260);
	CHECK(NightBodyHp(ZOMBIE_FOOTBALL, F(ne::GOLD_FOOTBALL, ns::GREEN), 270) == 600);
	CHECK(NightHelmHp(ZOMBIE_FOOTBALL, F(ne::GOLD_FOOTBALL, ns::GREEN), 1400) == 1400);
	CHECK(NightBodyHp(ZOMBIE_DANCER, F(0), 500) == 1000);
	CHECK(NightBodyHp(ZOMBIE_DANCER, F(ne::TROPHY), 500) == 500);
	CHECK(NightBodyHp(ZOMBIE_DANCER, F(ne::MOONWALKER), 500) == 1000);
	CHECK(NightBodyHp(ZOMBIE_BACKUP_DANCER, F(0), 270) == 270);
	CHECK(DancerStartPhaseCounter(F(ne::RANDOM_SUMMONER), 305) == 3000);
	CHECK(DancerStartPhaseCounter(F(ne::TROPHY), 305) == 305);
	CHECK(DancerStartStored(F(ne::RANDOM_SUMMONER)) == -1);
	CHECK(DancerStartStored(F(0)) == 0);
}

TEST_CASE("NightMovement") {
	NightRulesOn on;
	CHECK(NightWalkRateMultiplier(ZOMBIE_NEWSPAPER, F(0, ns::FOOTBALL_HEAD), 500, 500, 0, 0) == doctest::Approx(1.5f));
	CHECK(NightWalkRateMultiplier(ZOMBIE_NEWSPAPER, F(ne::KNIFE), 500, 500, 0, 0) == doctest::Approx(1.0f));
	CHECK(NightWalkRateMultiplier(ZOMBIE_FOOTBALL, F(ne::GOLD_FOOTBALL), 540, 540, 1260, 1260) == doctest::Approx(1.5f));
	CHECK(NightWalkRateMultiplier(ZOMBIE_FOOTBALL, F(0), 270, 270, 2800, 2800) == doctest::Approx(1.0f));
	// green: 1.1 x (2 - helm/max) x (1 + 1.57 |sin(1.57 (1 - body/max))|), 0xC12560
	CHECK(NightWalkRateMultiplier(ZOMBIE_FOOTBALL, F(0, ns::GREEN), 300, 300, 3000, 3000) == doctest::Approx(1.1f));
	CHECK(NightWalkRateMultiplier(ZOMBIE_FOOTBALL, F(0, ns::GREEN), 300, 300, 0, 3000) == doctest::Approx(2.2f));
	CHECK(NightWalkRateMultiplier(ZOMBIE_FOOTBALL, F(0, ns::GREEN), 300, 300, 1500, 3000) == doctest::Approx(1.65f));
	const float half = 1.1f * 2.0f * (1.0f + 1.57f * std::fabs(std::sin(1.57f * 0.5f)));
	CHECK(NightWalkRateMultiplier(ZOMBIE_FOOTBALL, F(0, ns::GREEN), 150, 300, 0, 3000) == doctest::Approx(half));
	CHECK(NightWalkRateMultiplier(ZOMBIE_FOOTBALL, F(ne::GOLD_FOOTBALL, ns::GREEN), 600, 600, 1400, 1400) ==
		doctest::Approx(1.65f));
	CHECK(NightAnimRateMultiplier(ZOMBIE_NEWSPAPER, F(ne::KNIFE), PHASE_NEWSPAPER_MAD) == doctest::Approx(1.0f / 3));
	CHECK(NightAnimRateMultiplier(ZOMBIE_NEWSPAPER, F(ne::KNIFE), PHASE_NEWSPAPER_READING) == doctest::Approx(1.0f));
	CHECK(NightAnimRateMultiplier(ZOMBIE_NEWSPAPER, F(0), PHASE_NEWSPAPER_MAD) == doctest::Approx(1.0f));
	CHECK(DancerSkipsDancerUpdate(F(ne::MOONWALKER)));
	CHECK_FALSE(DancerSkipsDancerUpdate(F(ne::TROPHY)));
	CHECK(MoonwalkVelX() == doctest::Approx(0.5f));
	CHECK(MoonwalkAnimRate() == doctest::Approx(24.0f));
}

TEST_CASE("DancerSummons") {
	NightRulesOn on;
	CHECK(DancerSnapsWithoutCountdown());
	CHECK(DancerResummonsAll());
	CHECK(DancerSkipsSummonCounterCheck(F(ne::TROPHY)));
	CHECK_FALSE(DancerSkipsSummonCounterCheck(F(0)));
	CHECK(DancerCountsCarnival(F(0)));
	CHECK(DancerCountsCarnival(F(ne::TROPHY)));
	CHECK_FALSE(DancerCountsCarnival(F(ne::RANDOM_SUMMONER)));
	CHECK(SummonsRandomFollowers(F(ne::RANDOM_SUMMONER)));
	CHECK_FALSE(SummonsRandomFollowers(F(ne::TROPHY)));
	CHECK(RandomFollowerRollRange() == 33);
	// 0xC03A6F: never Flag, Ladder, Pogo, Gatling head, Zomboss, pool/air types; Jalapeno head only above level 70
	const std::vector<int> ok = {0, 2, 3, 4, 5, 6, 7, 8, 9, 15, 22, 23, 24, 26, 27, 30, 31, 32};
	for (int r = 0; r < 33; ++r) {
		const bool want = std::find(ok.begin(), ok.end(), r) != ok.end();
		CHECK_MESSAGE(RandomFollowerAccepted(r, 20) == want, r);
	}
	CHECK_FALSE(RandomFollowerAccepted(28, 70));
	CHECK(RandomFollowerAccepted(28, 71));
	CHECK(DrawsWhite(ZOMBIE_DANCER, F(ne::RANDOM_SUMMONER)));
	CHECK_FALSE(DrawsWhite(ZOMBIE_DANCER, F(ne::TROPHY)));
}

TEST_CASE("NightEating") {
	NightRulesOn on;
	// mad Newspaper (phase 0x1F): bites on remainders 1..5, and CheckIfPreyCaught runs 3 times per tick (hook 632)
	CHECK(CheckIfPreyCaughtCallsPerTick(PHASE_NEWSPAPER_MAD) == 3);
	CHECK(CheckIfPreyCaughtCallsPerTick(PHASE_NEWSPAPER_READING) == 1);
	CHECK(NightBitesThisTick(ZOMBIE_NEWSPAPER, F(0), PHASE_NEWSPAPER_MAD, false, 0) == false);
	CHECK(NightBitesThisTick(ZOMBIE_NEWSPAPER, F(0), PHASE_NEWSPAPER_MAD, false, 1) == true);
	CHECK(NightBitesThisTick(ZOMBIE_NEWSPAPER, F(0), PHASE_NEWSPAPER_MAD, true, 5) == true);
	CHECK(NightBitesThisTick(ZOMBIE_NEWSPAPER, F(0), PHASE_NEWSPAPER_MAD, true, 6) == false);
	CHECK_FALSE(NightBitesThisTick(ZOMBIE_NEWSPAPER, F(0), PHASE_NEWSPAPER_READING, false, 0).has_value());
	// green Football (0xC02170): remainders 0..1 when not chilled; every call when chilled
	CHECK(NightBitesThisTick(ZOMBIE_FOOTBALL, F(0, ns::GREEN), PHASE_ZOMBIE_NORMAL, false, 1) == true);
	CHECK(NightBitesThisTick(ZOMBIE_FOOTBALL, F(0, ns::GREEN), PHASE_ZOMBIE_NORMAL, false, 2) == false);
	CHECK(NightBitesThisTick(ZOMBIE_FOOTBALL, F(0, ns::GREEN), PHASE_ZOMBIE_NORMAL, true, 7) == true);
	CHECK_FALSE(NightBitesThisTick(ZOMBIE_FOOTBALL, F(ne::GOLD_FOOTBALL), PHASE_ZOMBIE_NORMAL, false, 1).has_value());
	// knife: no normal bite; the mad knife's chew marks the plant to die 32 cs later (0x10020); a pending plant
	// bitten again dies at the next update (0x10001); Potato Mine and a hypnotised knife: vanilla flash
	CHECK(SkipsNormalBite(ZOMBIE_NEWSPAPER, F(ne::KNIFE)));
	CHECK_FALSE(SkipsNormalBite(ZOMBIE_NEWSPAPER, F(0)));
	CHECK(KnifeChewFlash(25, ZOMBIE_NEWSPAPER, F(ne::KNIFE), PHASE_NEWSPAPER_MAD, false, SEED_PEASHOOTER) == 0x10020);
	CHECK(KnifeChewFlash(25, ZOMBIE_NEWSPAPER, F(ne::KNIFE), PHASE_NEWSPAPER_READING, false, SEED_PEASHOOTER) == 25);
	CHECK(KnifeChewFlash(25, ZOMBIE_NEWSPAPER, F(ne::KNIFE), PHASE_NEWSPAPER_MAD, true, SEED_PEASHOOTER) == 25);
	CHECK(KnifeChewFlash(25, ZOMBIE_NEWSPAPER, F(ne::KNIFE), PHASE_NEWSPAPER_MAD, false, SEED_POTATOMINE) == 25);
	CHECK(KnifeChewFlash(25, ZOMBIE_NEWSPAPER, F(0), PHASE_NEWSPAPER_MAD, false, SEED_PEASHOOTER) == 25);
	CHECK(KnifeChewFlash(0x10015, ZOMBIE_NORMAL, F(0), PHASE_ZOMBIE_NORMAL, false, SEED_PEASHOOTER) == 0x10001);
	CHECK(KnifeKillsNow(0x10000));
	CHECK_FALSE(KnifeKillsNow(0x10001));
	CHECK_FALSE(KnifeKillsNow(24));
	CHECK(SkipsGarlic(ZOMBIE_NEWSPAPER, F(0, ns::FOOTBALL_HEAD)));
	CHECK_FALSE(SkipsGarlic(ZOMBIE_NEWSPAPER, F(ne::KNIFE)));
	CHECK(MadButterCs(ZOMBIE_NEWSPAPER, PHASE_NEWSPAPER_MAD, 400) == 500);
	CHECK(MadButterCs(ZOMBIE_NEWSPAPER, PHASE_NEWSPAPER_READING, 400) == 400);
	CHECK(MadButterCs(ZOMBIE_NORMAL, PHASE_ZOMBIE_NORMAL, 400) == 400);
	// hypnotised eaters (hook 689): Newspaper 13 (2 vs Zomboss unless reading), others 4; flags 8 for all
	CHECK(EatZombieDamage(ZOMBIE_NEWSPAPER, PHASE_NEWSPAPER_MAD, ZOMBIE_NORMAL, 4) == 13);
	CHECK(EatZombieDamage(ZOMBIE_NEWSPAPER, PHASE_NEWSPAPER_MAD, ZOMBIE_BOSS, 4) == 2);
	CHECK(EatZombieDamage(ZOMBIE_NEWSPAPER, PHASE_NEWSPAPER_READING, ZOMBIE_BOSS, 4) == 13);
	CHECK(EatZombieDamage(ZOMBIE_NORMAL, PHASE_ZOMBIE_NORMAL, ZOMBIE_NORMAL, 4) == 4);
	CHECK(EatZombieFlags(9U) == 8U);
	// green Football chew event (0xC12CAA): helmet on and age >= 800 -> drop it, +1000 body (player level <= 60)
	CHECK(GreenDropsHelmOnChew(F(0, ns::GREEN), true, 800, 20));
	CHECK_FALSE(GreenDropsHelmOnChew(F(0, ns::GREEN), true, 799, 20));
	CHECK_FALSE(GreenDropsHelmOnChew(F(0, ns::GREEN), false, 900, 20));
	CHECK_FALSE(GreenDropsHelmOnChew(F(0), true, 900, 20));
	CHECK_FALSE(GreenDropsHelmOnChew(F(0, ns::GREEN), true, 900, 71));
	CHECK(GreenChewBodyBonus(20) == 1000);
	CHECK(GreenChewBodyBonus(61) == 0);
}

TEST_CASE("NightDamage") {
	NightRulesOn on;
	CHECK(ReadingPaperSoaksBodyDamage(ZOMBIE_NEWSPAPER, F(0, ns::FOOTBALL_HEAD), PHASE_NEWSPAPER_READING));
	CHECK_FALSE(ReadingPaperSoaksBodyDamage(ZOMBIE_NEWSPAPER, F(0, ns::FOOTBALL_HEAD), PHASE_NEWSPAPER_MAD));
	CHECK_FALSE(ReadingPaperSoaksBodyDamage(ZOMBIE_NEWSPAPER, F(0), PHASE_NEWSPAPER_READING));
	CHECK(ShieldAlwaysFirst(ZOMBIE_DOOR, F(ne::TALLNUT_DOOR)));
	CHECK_FALSE(ShieldAlwaysFirst(ZOMBIE_DOOR, F(0)));
	CHECK_FALSE(ShieldAlwaysFirst(ZOMBIE_BOSS, F(ne::TALLNUT_DOOR)));
	CHECK(SkipsShieldDamageImages(ZOMBIE_DOOR, F(ne::TALLNUT_DOOR)));
	CHECK(SkipsShieldDamageImages(ZOMBIE_LADDER, F(0x18)));
	CHECK_FALSE(SkipsShieldDamageImages(ZOMBIE_DOOR, F(0)));
	CHECK(AlmanacPaperBodyPerHit(ZOMBIE_NEWSPAPER, F(ne::ALMANAC_PAPER)) == 15);
	CHECK(AlmanacPaperBodyPerHit(ZOMBIE_NEWSPAPER, F(0)) == 0);
	// reflect: door shields only, not bypass/both/no-flash shots, not fire peas or homing spikes; stars only die
	CHECK(DoorReflects(SHIELDTYPE_DOOR, 0, PROJECTILE_PEA, MOTION_STRAIGHT));
	CHECK(DoorReflects(SHIELDTYPE_DOOR, 4, PROJECTILE_SNOWPEA, MOTION_STRAIGHT)); // freeze bit is not in mask 0xB
	CHECK_FALSE(DoorReflects(SHIELDTYPE_DOOR, 1, PROJECTILE_PEA, MOTION_STRAIGHT));
	CHECK_FALSE(DoorReflects(SHIELDTYPE_DOOR, 2, PROJECTILE_PEA, MOTION_STRAIGHT));
	CHECK_FALSE(DoorReflects(SHIELDTYPE_DOOR, 8, PROJECTILE_PEA, MOTION_STRAIGHT));
	CHECK_FALSE(DoorReflects(SHIELDTYPE_DOOR, 0, PROJECTILE_FIREBALL, MOTION_STRAIGHT));
	CHECK_FALSE(DoorReflects(SHIELDTYPE_DOOR, 0, PROJECTILE_STAR, MOTION_STAR));
	CHECK_FALSE(DoorReflects(SHIELDTYPE_DOOR, 0, PROJECTILE_SPIKE, MOTION_HOMING));
	CHECK_FALSE(DoorReflects(SHIELDTYPE_NEWSPAPER, 0, PROJECTILE_PEA, MOTION_STRAIGHT));
	CHECK(DoorReflectShieldBonus(20) == 19);
	CHECK(ReflectedShot(false).motion == MOTION_BACKWARDS);
	CHECK(ReflectedShot(false).damageRangeFlags == 0);
	CHECK(ReflectedShot(true).motion == MOTION_STRAIGHT);
	CHECK(ReflectedShot(true).damageRangeFlags == 1);
	CHECK(GoldSquishes(ZOMBIE_FOOTBALL, F(ne::GOLD_FOOTBALL)));
	CHECK_FALSE(GoldSquishes(ZOMBIE_FOOTBALL, F(0, ns::GREEN)));
	CHECK(GoldRaisesGraveFootballs(ZOMBIE_FOOTBALL, F(ne::GOLD_FOOTBALL, ns::GREEN)));
	CHECK_FALSE(GoldRaisesGraveFootballs(ZOMBIE_FOOTBALL, F(0)));
}

TEST_CASE("NightEliteArtTable") {
	auto fh = NightArtFor(ZOMBIE_NEWSPAPER, F(0, ns::FOOTBALL_HEAD));
	REQUIRE(fh.images.size() == 1);
	CHECK(fh.images[0] == NightArtImage{"anim_head1", "particles/ZombieFootballHead"});
	CHECK(fh.hidePrefixes.empty());
	CHECK(NewspaperMadHeadFile(F(0, ns::FOOTBALL_HEAD)) == "particles/ZombieFootballHead");
	CHECK(NewspaperMadHeadFile(F(ne::KNIFE)).empty());
	auto knife = NightArtFor(ZOMBIE_NEWSPAPER, F(ne::KNIFE));
	CHECK(knife.images.empty());
	CHECK(knife.showTracks == std::vector<std::string>{"Zombie_knife"});
	CHECK(NightArtFor(ZOMBIE_NEWSPAPER, F(ne::ALMANAC_PAPER)).images[0] ==
		NightArtImage{"Zombie_paper_paper", "images/SelectorScreen_AlmanacHighlight"});
	CHECK(NightArtFor(ZOMBIE_DOOR, F(ne::TALLNUT_DOOR)).images[0] == NightArtImage{"anim_screendoor", "reanim/Tallnut_cracked2"});
	auto green = NightArtFor(ZOMBIE_FOOTBALL, F(0, ns::GREEN));
	CHECK(green.images.size() == 8);
	CHECK(green.images[0] == NightArtImage{"zombie_football_helmet", "images/green_football_helmet"});
	auto gold = NightArtFor(ZOMBIE_FOOTBALL, F(ne::GOLD_FOOTBALL));
	REQUIRE(gold.images.size() == 1);
	CHECK(gold.images[0] == NightArtImage{"anim_head1", "reanim/coin_gold_dollar"});
	CHECK(NightArtFor(ZOMBIE_FOOTBALL, F(ne::GOLD_FOOTBALL, ns::GREEN)).images.size() == 9);
	auto trophy = NightArtFor(ZOMBIE_DANCER, F(ne::TROPHY));
	CHECK(trophy.images[0] == NightArtImage{"anim_head1", "images/trophy"});
	CHECK(trophy.hidePrefixes == std::vector<std::string>{"anim_hair", "anim_head2"});
	auto moon = NightArtFor(ZOMBIE_DANCER, F(ne::MOONWALKER));
	CHECK(moon.images[0] == NightArtImage{"anim_head1", "reanim/Zombie_paper_madhead"});
	CHECK(moon.hidePrefixes == std::vector<std::string>{"anim_hair"});
	CHECK(NightArtFor(ZOMBIE_DANCER, F(ne::RANDOM_SUMMONER)).empty());
	CHECK(NightArtFor(ZOMBIE_NEWSPAPER, F(0)).empty());
	CHECK(GreenHelmImage(1) == "images/green_football_helmet2");
	CHECK(GreenHelmImage(2) == "images/green_football_helmet3");
	CHECK(GreenHelmImage(0).empty());
	CHECK(std::string(KnifeAnim("anim_gasp")) == "anim_gasp1");
	CHECK(std::string(KnifeAnim("anim_eat_nopaper")) == "anim_eat_nopaper1");
	CHECK(UnnamedOnlyNewspaperTracks() == std::vector<std::string>{"Zombie_knife", "anim_football"});
}

TEST_CASE("NightEliteWithoutArtDrawsBase") {
	auto none = [](const std::string&) { return false; };
	auto all = [](const std::string&) { return true; };
	CHECK(ResolveNightArt(ZOMBIE_DANCER, F(ne::TROPHY), none).empty());   // hair and jaw stay: the base dancer
	CHECK(ResolveNightArt(ZOMBIE_DANCER, F(ne::TROPHY), all).images.size() == 1);
	auto noHelmet = [](const std::string& f) { return f != "images/green_football_upperbody3"; };
	CHECK(ResolveNightArt(ZOMBIE_FOOTBALL, F(0, ns::GREEN), noHelmet).empty());
	// the knife needs no image file (its track comes with the merged reanim)
	CHECK(ResolveNightArt(ZOMBIE_NEWSPAPER, F(ne::KNIFE), none).showTracks.size() == 1);
	NightRulesOn on;
	CHECK(NightBodyHp(ZOMBIE_DANCER, F(ne::TROPHY), 500) == 500);   // behaviour does not depend on the art
}

TEST_CASE("NightStateSurvivesSaveLoad") {
	ZombieState z{ne::GOLD_FOOTBALL, 5, 0, 1, ns::GREEN, -1};
	auto b = Encode(z);
	CHECK(b.size() == 40); // Plan 8 part 3 added the mod-plant statuses (toxicKind, toxicTimer, repeaterPoisonCs, weakenCs)
	ZombieState z2;
	REQUIRE(Decode(b.data(), b.size(), z2));
	CHECK(z2 == z);
	// 16-byte blobs (Plan 2 Task 5) and 12-byte blobs (Task 2) still load; the Night fields read 0
	ZombieState z3{1, 2, 3, 4, 5, 6};
	REQUIRE(Decode(b.data(), 16, z3));
	CHECK(z3.flags == 1);
	CHECK(z3.subVariant == 0);
	CHECK(z3.stored == 0);
	REQUIRE(Decode(b.data(), 12, z3));
	CHECK(z3.flags == 0);
	CHECK(z3.subVariant == 0);
}

TEST_CASE("CompiledReanim32Decode") {
	// a 2-track definition in the 1051 layout: hash, {tracks*, count, fps, atlas*}, track size 12, raw tracks,
	// then per track: name, transform size 44, raw transforms, per transform image/font/text strings
	std::vector<uint8_t> b;
	auto u32 = [&](uint32_t v) { for (int i = 0; i < 4; ++i) b.push_back(static_cast<uint8_t>(v >> (8 * i))); };
	auto f32 = [&](float v) { uint32_t u; std::memcpy(&u, &v, 4); u32(u); };
	auto str = [&](const std::string& s) { u32(static_cast<uint32_t>(s.size())); b.insert(b.end(), s.begin(), s.end()); };
	u32(0x1234); u32(0); u32(2); f32(12.0f); u32(0);
	u32(12);
	u32(0); u32(0); u32(1);
	u32(0); u32(0); u32(2);
	str("anim_walk"); u32(44);
	for (float v : {1.f, 2.f, 0.f, 0.f, 1.f, 1.f, 0.f, 1.f}) f32(v);
	u32(0); u32(0); u32(0);
	str(""); str(""); str("");
	str("Zombie_knife"); u32(44);
	for (int k = 0; k < 2; ++k) {
		for (float v : {5.f, 6.f, 0.f, 0.f, 1.f, 1.f, 0.f, 1.f}) f32(v);
		u32(0); u32(0); u32(0);
	}
	str("IMAGE_REANIM_KNIFE"); str(""); str("");
	str(""); str(""); str("");
	std::string err;
	auto r = DecodeReanim32(b.data(), b.size(), &err);
	REQUIRE_MESSAGE(r.has_value(), err);
	CHECK(r->fps == doctest::Approx(12.0f));
	REQUIRE(r->tracks.size() == 2);
	CHECK(r->tracks[0].name == "anim_walk");
	CHECK(r->tracks[0].transforms.size() == 1);
	CHECK(r->tracks[1].transforms.size() == 2);
	CHECK(r->tracks[1].transforms[0].image == "IMAGE_REANIM_KNIFE");
	CHECK(r->tracks[1].transforms[0].x == doctest::Approx(5.0f));
	CHECK(r->tracks[1].transforms[1].image.empty());
	CHECK_FALSE(DecodeReanim32(b.data(), b.size() - 1).has_value());   // truncated
	b.push_back(0);
	CHECK_FALSE(DecodeReanim32(b.data(), b.size()).has_value());       // trailing bytes

	const uint8_t hdr[8] = {0xD4, 0xFE, 0xAD, 0xDE, 0x10, 0, 0, 0};
	CHECK(CompiledUncompressedSize(hdr, 8) == 16u);
	const uint8_t bad[8] = {0, 0, 0, 0, 0x10, 0, 0, 0};
	CHECK_FALSE(CompiledUncompressedSize(bad, 8).has_value());
}

TEST_CASE("ReanimTrackMergePlan") {
	Reanim32 u;
	auto track = [](const char* n, int frames) { Reanim32Track t; t.name = n; t.transforms.resize(frames); return t; };
	u.tracks = {track("anim_walk", 3), track("hands", 3), track("Zombie_knife", 3), track("body", 3)};
	auto plan = PlanTrackMerge({"anim_walk", "hands", "body"}, {3, 3, 3}, u);
	REQUIRE(plan.has_value());
	CHECK(*plan == std::vector<MergeSlot>{{true, 0}, {true, 1}, {false, 2}, {true, 2}});
	CHECK(PlanTrackMerge({"ANIM_WALK", "hands", "body"}, {3, 3, 3}, u).has_value());   // case-insensitive
	CHECK_FALSE(PlanTrackMerge({"anim_walk", "hands", "head"}, {3, 3, 3}, u).has_value()); // vanilla track missing
	CHECK_FALSE(PlanTrackMerge({"anim_walk", "hands", "body"}, {3, 3, 4}, u).has_value()); // GOTY track longer
	CHECK(PlanTrackMerge({"anim_walk", "hands", "body"}, {2, 2, 2}, u).has_value());        // Unnamed appended frames
	u.tracks[2].transforms.resize(2);
	CHECK_FALSE(PlanTrackMerge({"anim_walk", "hands", "body"}, {3, 3, 3}, u).has_value());
}

TEST_CASE("NightPackRulesMatchTheseValues") {
	for (const auto& k : NightZombieRuleKeys()) CHECK_MESSAGE(kNightRules.contains(k), k);
	CHECK(NightZombieRuleKeys().size() == kNightRules.size());
	const char* home = std::getenv("HOME");
	fs::path file = std::getenv("PVZU_PACK_DIR") ? fs::path(std::getenv("PVZU_PACK_DIR"))
	                                              : fs::path(home ? home : "") / "pvz-unnamed-pack";
	file /= "data/rules-night-zombies.json";
	if (!fs::exists(file)) { MESSAGE("pack not here, skipped: " << file.string()); return; }
	std::ifstream in(file);
	nlohmann::json pack = nlohmann::json::parse(in, nullptr, false);
	REQUIRE(pack.is_object());
	CHECK(pack == kNightRules);
}
