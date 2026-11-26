// PVZU: Unnamed Day-plant rules (the research notes (plants)). Rolls are injected; boundary rolls 0, threshold,
// threshold+1 and max for every chance.
#include "doctest/doctest.h"
#include "Mod/Unnamed/DayPlants.h"
#include "Mod/Unnamed/Rules.h"
#include "Mod/Unnamed/State.h"

using namespace pvzu::unnamed;

namespace {
// Same values as pvz-unnamed-pack/data/rules-day-plants.json.
nlohmann::json PackRules() {
	return nlohmann::json{
		{"dayPlants.peashooter.raiseRollRange", 2},
		{"dayPlants.peashooter.raiseStepCs", 5},
		{"dayPlants.peashooter.minLaunchRateCs", 25},
		{"dayPlants.peashooter.raiseWhenBlinkCountdownAtMost", 1},
		{"dayPlants.peashooter.firstAdventureLaunchRateCs", 80},
		{"dayPlants.peashooter.firstAdventureLayers", 15},
		{"dayPlants.peashooter.fireAtLaunchCounter", 1},
		{"dayPlants.sunflower.bonusMaxRoll", 11},
		{"dayPlants.sunflower.bonusRollRange", 100},
		{"dayPlants.cherryBomb.mineMaxRoll", 20},
		{"dayPlants.cherryBomb.mineRollRange", 100},
		{"dayPlants.cherryBomb.mineRequiresHp", 300},
		{"dayPlants.wallnut.healDelayCs", 4800},
		{"dayPlants.wallnut.healInBowling", 0},
		{"dayPlants.potatoMine.armingCs", 0},
		{"dayPlants.potatoMine.blastRadiusPx", 100},
		{"dayPlants.potatoMine.blastBurns", 1},
		{"dayPlants.snowPea.fireMaxRoll", 33},
		{"dayPlants.snowPea.fireRollRange", 100},
		{"dayPlants.snowPea.fireTier", 3},
		{"dayPlants.fireTier.chillCs", 750},
		{"dayPlants.fireTier.chillMinTier", 3},
		{"dayPlants.fireTier.chillMaxTier", 5},
		{"dayPlants.fireTier.armBodyDamage", 10},
		{"dayPlants.fireTier.armDamageFlags", 10},
		{"dayPlants.fireTier.tier3TintR", 122},
		{"dayPlants.fireTier.tier3TintG", 230},
		{"dayPlants.fireTier.tier3TintB", 143},
		{"dayPlants.fireTier.damage1", 40},
		{"dayPlants.fireTier.damage2", 60},
		{"dayPlants.fireTier.damage3", 40},
		{"dayPlants.fireTier.damage4", 60},
		{"dayPlants.fireTier.damage5", 80},
		{"dayPlants.chomper.digestSplitHp", 1500},
		{"dayPlants.chomper.digestLowMul", 2},
		{"dayPlants.chomper.digestLowAdd", 1000},
		{"dayPlants.chomper.digestHighDiv", 2},
		{"dayPlants.chomper.digestHighAdd", 3250},
		{"dayPlants.chomper.digestMinCs", 1000},
		{"dayPlants.chomper.digestFallbackCs", 2700},
		{"dayPlants.chomper.sunRollRange", 5650},
		{"dayPlants.chomper.smallSunMaxHp", 870},
		{"dayPlants.chomper.sunMaxHp", 1370},
		{"dayPlants.chomper.sunFlashCs", 100},
		{"dayPlants.chomper.unswallowableMaxHpAbove", 120000},
		{"dayPlants.chomper.refuseNewspaperWithoutPaper", 1},
		{"dayPlants.chomper.refuseFootball", 1},
		{"dayPlants.repeater.diagMaxRoll", 32},
		{"dayPlants.repeater.diagRollRange", 100},
		{"dayPlants.repeater.diagVelX", 2.8211734},
		{"dayPlants.repeater.diagVelY", 1.7864928},
	};
}
struct WithPackRules {
	WithPackRules() { SetRules(PackRules()); }
	~WithPackRules() { SetRules(nlohmann::json::object()); }
};
constexpr int kSmallSun = 5, kSun = 4, kLargeSun = 6; // engine CoinType values (also the exe's)
}

TEST_CASE_FIXTURE(WithPackRules, "PeashooterRaise: once per blink cycle, 50%, -5 cs, floor 25") {
	CHECK(PeashooterRaiseRollRange() == 2);
	CHECK(PeashooterRaiseDue(0));
	CHECK(PeashooterRaiseDue(1));      // threshold
	CHECK_FALSE(PeashooterRaiseDue(2)); // threshold + 1
	CHECK_FALSE(PeashooterRaiseDue(799));

	auto none = PeashooterRaise(0, 150, 0); // Rand(2) == 0: no raise
	CHECK_FALSE(none.raised);
	CHECK(none.launchRate == 150);
	CHECK(none.layers == 0);

	auto r = PeashooterRaise(1, 150, 0); // max roll
	CHECK(r.raised);
	CHECK(r.launchRate == 145);
	CHECK(r.layers == 1);

	auto atFloor = PeashooterRaise(1, 30, 24); // 30 - 5 = 25 <= 25: floor, layers unchanged
	CHECK(atFloor.launchRate == 25);
	CHECK(atFloor.layers == 24);
	auto aboveFloor = PeashooterRaise(1, 31, 24); // 26 > 25: layer counted
	CHECK(aboveFloor.launchRate == 26);
	CHECK(aboveFloor.layers == 25);
	CHECK(PeashooterRaise(1, 25, 3).launchRate == 25);

	CHECK(PeashooterLayersGuard(-1) == 1);
	CHECK(PeashooterLayersGuard(0) == 1); // the exe never holds 0: +0x80 starts at -1 (PlantInitialize 0x45dc0a), shown as 1
	CHECK(PeashooterLayersGuard(15) == 15);
}

TEST_CASE_FIXTURE(WithPackRules, "PeashooterShotTiming: fire at launch counter 1, end shot at 0") {
	CHECK(PeashooterShotTiming(0) == ShotTiming::EndShot);
	CHECK(PeashooterShotTiming(1) == ShotTiming::FireNow);
	CHECK(PeashooterShotTiming(2) == ShotTiming::Vanilla);
	CHECK(PeashooterShotTiming(150) == ShotTiming::Vanilla);
}

TEST_CASE_FIXTURE(WithPackRules, "PeashooterFirstAdventureStart: 15 layers, rate 80 until Adventure is finished") {
	auto s = PeashooterFirstAdventureStart(false);
	CHECK(s.applies);
	CHECK(s.launchRate == 80);
	CHECK(s.layers == 15);
	CHECK_FALSE(PeashooterFirstAdventureStart(true).applies);
}

TEST_CASE_FIXTURE(WithPackRules, "SunflowerBonusSun: Rand(100) <= 11 (12%)") {
	CHECK(SunflowerBonusRollRange() == 100);
	CHECK(SunflowerBonusSun(0));
	CHECK(SunflowerBonusSun(11));
	CHECK_FALSE(SunflowerBonusSun(12));
	CHECK_FALSE(SunflowerBonusSun(99));
}

TEST_CASE_FIXTURE(WithPackRules, "CherryBombLeavesMine: Rand(100) <= 20 (21%), undamaged 300/300, not I-Zombie") {
	CHECK(CherryMineRollRange() == 100);
	CHECK(CherryBombLeavesMine(0, 300, 300, false));
	CHECK(CherryBombLeavesMine(20, 300, 300, false));
	CHECK_FALSE(CherryBombLeavesMine(21, 300, 300, false));
	CHECK_FALSE(CherryBombLeavesMine(99, 300, 300, false));
	CHECK_FALSE(CherryBombLeavesMine(0, 299, 300, false)); // damaged
	CHECK_FALSE(CherryBombLeavesMine(0, 400, 400, false)); // max HP not 300 (e.g. a pack change)
	CHECK_FALSE(CherryBombLeavesMine(0, 300, 300, true));  // I-Zombie level
}

TEST_CASE_FIXTURE(WithPackRules, "WallnutHealTick: full heal after 4800 cs of unchanged HP, any change resets") {
	PlantState s{};
	// first tick: lastHp (0) differs -> reset, remember HP
	CHECK(WallnutHealTick(s, 1000, 4000, false) == 1000);
	CHECK(s.counter == 0);
	CHECK(s.extra == 1000);
	for (int i = 1; i < 4800; i++)
		REQUIRE(WallnutHealTick(s, 1000, 4000, false) == 1000);
	CHECK(s.counter == 4799);                                 // threshold - 1
	CHECK(WallnutHealTick(s, 1000, 4000, false) == 4000);     // tick 4800: heal
	CHECK(s.counter == 0);
	CHECK(s.extra == 4000);

	PlantState t{4000, 1000}; // counter way up, then HP changes
	CHECK(WallnutHealTick(t, 999, 4000, false) == 999);
	CHECK(t.counter == 0);
	CHECK(t.extra == 999);

	PlantState b{4799, 1000}; // bowling: never heals, state untouched
	CHECK(WallnutHealTick(b, 1000, 4000, true) == 1000);
	CHECK(b.counter == 4799);
}

TEST_CASE_FIXTURE(WithPackRules, "PotatoMine: arming 0, radius 100, burns") {
	CHECK(PotatoMineArmingCs() == 0);
	CHECK(PotatoMineBlastRadius() == 100);
	CHECK(PotatoMineBlastBurns());
}

TEST_CASE_FIXTURE(WithPackRules, "SnowPeaFireShot: Rand(100) <= 33 (34%), tier 3") {
	CHECK(SnowPeaFireRollRange() == 100);
	CHECK(SnowPeaFireShot(0));
	CHECK(SnowPeaFireShot(33));
	CHECK_FALSE(SnowPeaFireShot(34));
	CHECK_FALSE(SnowPeaFireShot(99));
	CHECK(SnowPeaFireTier() == 3);
}

TEST_CASE_FIXTURE(WithPackRules, "FireTier: damage table, chill and arm hit for tiers 3..5, tier 3 tint") {
	CHECK(FireTierDamage(0, 40) == 40); // untiered fireball: table damage
	CHECK(FireTierDamage(1, 99) == 40);
	CHECK(FireTierDamage(2, 99) == 60);
	CHECK(FireTierDamage(3, 99) == 40);
	CHECK(FireTierDamage(4, 99) == 60);
	CHECK(FireTierDamage(5, 99) == 80);
	CHECK(FireTierDamage(6, 99) == 99);
	CHECK_FALSE(FireTierChills(2));
	CHECK(FireTierChills(3));
	CHECK(FireTierChills(5));
	CHECK_FALSE(FireTierChills(6));
	CHECK(FireTierChillCs() == 750);
	CHECK(FireTierArmDamage() == 10);
	CHECK(FireTierArmDamageFlags() == 10U);
	int r = 0, g = 0, b = 0;
	REQUIRE(FireTierTint(3, r, g, b));
	CHECK(r == 122);
	CHECK(g == 230);
	CHECK(b == 143);
	CHECK_FALSE(FireTierTint(1, r, g, b));
}

TEST_CASE_FIXTURE(WithPackRules, "ChomperDigestCs: 2h+1000 up to 1500 HP, h/2+3250 above") {
	CHECK(ChomperDigestCs(0) == 1000);
	CHECK(ChomperDigestCs(270) == 1540);
	CHECK(ChomperDigestCs(1370) == 3740);
	CHECK(ChomperDigestCs(1500) == 4000);
	CHECK(ChomperDigestCs(1501) == 4000); // 750 + 3250 (integer halving)
	CHECK(ChomperDigestCs(1503) == 4001);
	CHECK(ChomperDigestCountdown(1000) == 1000);
	CHECK(ChomperDigestCountdown(999) == 2700);
}

TEST_CASE_FIXTURE(WithPackRules, "ChomperSunCoin: small <= 870, sun <= 1370, large above") {
	CHECK(ChomperSunCoin(0) == kSmallSun);
	CHECK(ChomperSunCoin(870) == kSmallSun);
	CHECK(ChomperSunCoin(871) == kSun);
	CHECK(ChomperSunCoin(1370) == kSun);
	CHECK(ChomperSunCoin(1371) == kLargeSun);
}

TEST_CASE_FIXTURE(WithPackRules, "ChomperDropsSun: Rand(5650) <= digest time T (not eaten HP)") {
	CHECK(ChomperSunRollRange() == 5650);
	const int T = ChomperDigestCs(270); // 1540
	CHECK(ChomperDropsSun(0, T));
	CHECK(ChomperDropsSun(1540, T));
	CHECK_FALSE(ChomperDropsSun(1541, T));
	CHECK_FALSE(ChomperDropsSun(5649, T));
	CHECK(ChomperDropsSun(5649, 6000));   // T above the range: always
	CHECK_FALSE(ChomperDropsSun(0, 0));   // nothing digested (missed bite)
	CHECK(ChomperSunFlashCs() == 100);
}

TEST_CASE_FIXTURE(WithPackRules, "ChomperRefusesSwallow: boss-class HP, Newspaper without paper, Football") {
	CHECK_FALSE(ChomperRefusesSwallow(270, false, false));
	CHECK_FALSE(ChomperRefusesSwallow(120000, false, false));
	CHECK(ChomperRefusesSwallow(120001, false, false));
	CHECK(ChomperRefusesSwallow(270, true, false));
	CHECK(ChomperRefusesSwallow(270, false, true));
}

TEST_CASE_FIXTURE(WithPackRules, "RepeaterDiagonal: Rand(100) <= 32 (33%), velocity 2.821 / +-1.786") {
	CHECK(RepeaterDiagRollRange() == 100);
	CHECK(RepeaterDiagonal(0));
	CHECK(RepeaterDiagonal(32));
	CHECK_FALSE(RepeaterDiagonal(33));
	CHECK_FALSE(RepeaterDiagonal(99));
	CHECK(RepeaterDiagVelX() == doctest::Approx(2.8211734f));
	CHECK(RepeaterDiagVelY() == doctest::Approx(1.7864928f));
}

TEST_CASE("DayPlantsMissingRulesAreVanilla") {
	SetRules(nlohmann::json::object());
	CHECK_FALSE(PeashooterRaise(1, 150, 0).raised);
	CHECK(PeashooterRaise(1, 150, 0).launchRate == 150);
	CHECK(PeashooterShotTiming(1) == ShotTiming::Vanilla);
	CHECK(PeashooterShotTiming(0) == ShotTiming::Vanilla);
	CHECK_FALSE(PeashooterFirstAdventureStart(false).applies);
	CHECK_FALSE(SunflowerBonusSun(0));
	CHECK_FALSE(CherryBombLeavesMine(0, 300, 300, false));
	PlantState s{4799, 1000};
	CHECK(WallnutHealTick(s, 1000, 4000, false) == 1000);
	CHECK(PotatoMineArmingCs() == 1500);
	CHECK(PotatoMineBlastRadius() == 60);
	CHECK_FALSE(PotatoMineBlastBurns());
	CHECK_FALSE(SnowPeaFireShot(0));
	CHECK(FireTierDamage(3, 40) == 40);
	CHECK_FALSE(FireTierChills(3));
	int r, g, b;
	CHECK_FALSE(FireTierTint(3, r, g, b));
	CHECK(ChomperDigestCs(270) == 4000);
	CHECK(ChomperDigestCountdown(ChomperDigestCs(270)) == 4000);
	CHECK_FALSE(ChomperDropsSun(0, 4000));
	CHECK_FALSE(ChomperRefusesSwallow(200000, true, true));
	CHECK_FALSE(RepeaterDiagonal(0));
}

TEST_CASE("ProjectileStateSurvivesSaveLoad") {
	ProjectileState p{3};
	auto blob = Encode(p);
	ProjectileState q;
	REQUIRE(Decode(blob.data(), blob.size(), q));
	CHECK(q == p);
	ProjectileState z{7};
	CHECK_FALSE(Decode(blob.data(), 2, z));
	CHECK(z == ProjectileState{});
}

TEST_CASE("PlantStateOlderShortBlobDecodesAsZeros") {
	PlantState s{9, 9};
	const uint8_t four[4] = {1, 0, 0, 0};
	CHECK_FALSE(Decode(four, sizeof four, s));
	CHECK(s == PlantState{});
}
