// PVZU: Plan 5 Task 2, Fog zombies and elite forms (pure rules; evidence in the research notes (w4-fog) section 3 and
// pvz-unnamed-pack/SOURCES-fog-zombies.md).
#include "doctest/doctest.h"
#include "Mod/Unnamed/DayZombies.h"
#include "Mod/Unnamed/FogZombies.h"
#include "Mod/Unnamed/Rules.h"
#include "Mod/Unnamed/State.h"

#include "nlohmann/json.hpp"

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <random>

using namespace pvzu::unnamed;
namespace fs = std::filesystem;
namespace fe = pvzu::unnamed::fogelite;
namespace eo = pvzu::unnamed::eliteoption;

namespace {

// The Unnamed values (same as pvz-unnamed-pack/data/rules-fog-zombies.json; the last test compares the two).
const nlohmann::json kFogRules = {
	{"fogZombies.jack.bodyHp", 870},
	{"fogZombies.jack.eliteRoll", 8},
	{"fogZombies.jack.eliteCount", 2},
	{"fogZombies.jack.eliteRollAllElite", 2},
	{"fogZombies.jack.iceFreezeCsPerPlant", 350},
	{"fogZombies.jack.potatoPackets", 9},
	{"fogZombies.balloon.flyingHp", 50},
	{"fogZombies.balloon.flyVx", 0.45},
	{"fogZombies.balloon.ignoreMowers", 1},
	{"fogZombies.balloon.eatWhileFlying", 1},
	{"fogZombies.balloon.cloudRoll", 7},
	{"fogZombies.balloon.cloudHit", 3},
	{"fogZombies.balloon.cloudBodyHp", 190},
	{"fogZombies.balloon.cloudNeverPops", 1},
	{"fogZombies.digger.helmHp", 370},
	{"fogZombies.digger.eatsUnderground", 1},
	{"fogZombies.digger.eliteRoll", 5},
	{"fogZombies.digger.eliteBelow", 2},
	{"fogZombies.digger.surfaceRoll", 3},
	{"fogZombies.digger.surfaceStepPx", 80},
	{"fogZombies.digger.surfaceBasePx", 10},
	{"fogZombies.pogo.eliteRoll", 5},
	{"fogZombies.pogo.eliteRollAllElite", 2},
	{"fogZombies.pogo.chocolateHpDelta", -200},
	{"fogZombies.pogo.garlicInvisibleRoll", 4},
	{"fogZombies.pogo.noWindup", 1},
	{"fogZombies.pogo.chocolateBounceDx", 36},
	{"fogZombies.pogo.yuckyCounter", 100},
	{"fogZombies.pogo.landOnZombies", 1},
	{"fogZombies.pogo.landFirstDamage", 1800},
	{"fogZombies.pogo.landOtherDamage", 200},
	{"fogZombies.pogo.chocolateDamageMul", 1.6667},
	{"fogZombies.pogo.landRectGrowPx", 32},
	{"fogZombies.pogo.bonkGiantNuts", 1},
};

struct FogRulesOn {
	FogRulesOn() { SetRules(kFogRules); }
	~FogRulesOn() { SetRules(nlohmann::json::object()); }
};

} // namespace

TEST_CASE("FogVanillaWithoutRules") {
	SetRules(nlohmann::json::object());
	CHECK(JackRollRange() == 0);
	CHECK(BalloonRollRange() == 0);
	CHECK(DiggerRollRange() == 0);
	CHECK(PogoRollRange(eo::DEFAULT) == 0);
	CHECK(FogBodyHp(ZOMBIE_JACK_IN_THE_BOX, 0, 500) == 500);
	CHECK(FogBodyHp(ZOMBIE_POGO, fe::CHOCOLATE, 500) == 500);
	CHECK(FogBodyHp(ZOMBIE_BALLOON, fe::CLOUD, 270) == 270);
	CHECK(FogHelmHp(ZOMBIE_DIGGER, 100) == 100);
	CHECK(FogFlyingHp(ZOMBIE_BALLOON, 20) == 20);
	CHECK_FALSE(BalloonVelX(ZOMBIE_BALLOON, 0, false).has_value());
	CHECK_FALSE(MowerSkips(ZOMBIE_BALLOON, 20));
	CHECK_FALSE(FlyersEat());
	CHECK_FALSE(FlyingBalloonSkipsPlant(SEED_WALLNUT));
	CHECK_FALSE(CloudKeepsFlying(fe::CLOUD, true));
	CHECK_FALSE(TunnelingDiggerTargets(SEED_WALLNUT).has_value());
	CHECK(DiggerSurfaceBelowX(160, 10.0f) == doctest::Approx(10.0f));
	CHECK_FALSE(DiggerRisesWithoutAxe(fe::SCREWDRIVER));
	CHECK_FALSE(PogoTargetsZombies());
	CHECK_FALSE(PogoLandsOnZombies());
	CHECK(PogoBonksOn(SEED_TALLNUT));
	CHECK_FALSE(PogoBonksOn(SEED_GIANT_WALLNUT));
	CHECK(IceFreezeCounter(10) == 0);
	// the vanilla pogo: wind-up bounce first, then the forward bounce of (dx + 60) / 80
	CHECK(PogoBounceEnd(0, std::nullopt, false) == PogoNext{PogoMove::BOUNCE, 0.0f});
	CHECK(PogoBounceEnd(0, 40, false) == PogoNext{PogoMove::HIGH_BOUNCE, 0.0f});
	CHECK(PogoBounceEnd(0, 40, true) == PogoNext{PogoMove::FORWARD, 100.0f / 80.0f});
}

TEST_CASE("FogRollBoundaries") {
	FogRulesOn on;
	// Jack: Rand(8) < 2 -> 0xE + roll (hook 500), never in Vasebreaker, ignores the elite option
	CHECK(JackRollRange() == 8);
	CHECK(RollJack(0) == fe::ICE_JACK);
	CHECK(RollJack(1) == fe::POTATO_JACK);
	CHECK(RollJack(2) == 0);
	CHECK(RollJack(7) == 0);
	CHECK(JackRollAllowed(false));
	CHECK_FALSE(JackRollAllowed(true));
	// Balloon: Rand(7) == 3 -> cloud 0x2B (hook 503), not in I, Zombie
	CHECK(BalloonRollRange() == 7);
	CHECK(RollBalloon(2) == 0);
	CHECK(RollBalloon(3) == fe::CLOUD);
	CHECK(RollBalloon(4) == 0);
	CHECK(RollBalloon(0) == 0);
	CHECK(BalloonDiggerRollAllowed(false));
	CHECK_FALSE(BalloonDiggerRollAllowed(true));
	// Digger: Rand(5) < 2 -> screwdriver 0x10 (hook 490); surfacing x = (Rand(3) + 1) * 80
	CHECK(DiggerRollRange() == 5);
	CHECK(RollDigger(0) == fe::SCREWDRIVER);
	CHECK(RollDigger(1) == fe::SCREWDRIVER);
	CHECK(RollDigger(2) == 0);
	CHECK(RollDigger(4) == 0);
	CHECK(DiggerSurfaceRollRange() == 3);
	CHECK(DiggerSurfaceX(0) == 80);
	CHECK(DiggerSurfaceX(1) == 160);
	CHECK(DiggerSurfaceX(2) == 240);
	// Pogo (hook 501): option 1 -> no roll, 2 -> Rand(2), else Rand(5); r 0 chocolate, r 1 garlic
	CHECK(PogoRollRange(eo::DEFAULT) == 5);
	CHECK(PogoRollRange(eo::NONE) == 0);
	CHECK(PogoRollRange(eo::ALL) == 2);
	CHECK(RollPogo(0) == fe::CHOCOLATE);
	CHECK(RollPogo(1) == fe::GARLIC);
	CHECK(RollPogo(2) == 0);
	CHECK(RollPogo(4) == 0);
	CHECK(PogoGarlicVisibleRollRange() == 4);
	CHECK_FALSE(PogoGarlicVisible(0));
	CHECK(PogoGarlicVisible(1));
	CHECK(PogoGarlicVisible(3));
	// types and names
	for (ZombieType t : {ZOMBIE_JACK_IN_THE_BOX, ZOMBIE_BALLOON, ZOMBIE_DIGGER, ZOMBIE_POGO})
		CHECK(IsFogType(t));
	CHECK_FALSE(IsFogType(ZOMBIE_NORMAL));
	CHECK_FALSE(IsFogType(ZOMBIE_SNORKEL));
	CHECK(std::string(FogEliteName(ZOMBIE_JACK_IN_THE_BOX, fe::ICE_JACK)) == "ice-head");
	CHECK(std::string(FogEliteName(ZOMBIE_JACK_IN_THE_BOX, fe::POTATO_JACK)) == "potato-head");
	CHECK(std::string(FogEliteName(ZOMBIE_BALLOON, fe::CLOUD)) == "cloud");
	CHECK(std::string(FogEliteName(ZOMBIE_DIGGER, fe::SCREWDRIVER)) == "screwdriver");
	CHECK(std::string(FogEliteName(ZOMBIE_POGO, fe::CHOCOLATE)) == "chocolate");
	CHECK(std::string(FogEliteName(ZOMBIE_POGO, fe::GARLIC)) == "garlic");
	CHECK(std::string(FogEliteName(ZOMBIE_POGO, 0)) == "none");
	CHECK(std::string(FogEliteName(ZOMBIE_DIGGER, fe::CHOCOLATE)) == "none");   // codes mean a form only on their type
}

TEST_CASE("FogEliteRatesOver2000Rolls") {
	FogRulesOn on;
	std::mt19937 gen(4244);
	auto roll = [&](int n) { return std::uniform_int_distribution<int>(0, n - 1)(gen); };
	const int N = 2000;
	auto within = [&](int count, double p) { // 4 standard deviations
		const double sd = std::sqrt(N * p * (1 - p));
		CHECK_MESSAGE(std::abs(count - N * p) <= 4 * sd, count << " vs " << N * p);
	};
	std::map<int, int> seen;
	for (int i = 0; i < N; ++i) seen[RollJack(roll(JackRollRange()))]++;
	within(seen[fe::ICE_JACK], 1.0 / 8);      // Almanac says 1/4 each: the code wins
	within(seen[fe::POTATO_JACK], 1.0 / 8);
	within(seen[0], 6.0 / 8);
	seen.clear();
	for (int i = 0; i < N; ++i) seen[RollBalloon(roll(BalloonRollRange()))]++;
	within(seen[fe::CLOUD], 1.0 / 7);
	within(seen[0], 6.0 / 7);
	seen.clear();
	for (int i = 0; i < N; ++i) seen[RollDigger(roll(DiggerRollRange()))]++;
	within(seen[fe::SCREWDRIVER], 2.0 / 5);
	within(seen[0], 3.0 / 5);
	seen.clear();
	for (int i = 0; i < N; ++i) seen[DiggerSurfaceX(roll(DiggerSurfaceRollRange()))]++;
	for (int x : {80, 160, 240}) within(seen[x], 1.0 / 3);
	seen.clear();
	for (int i = 0; i < N; ++i) seen[RollPogo(roll(PogoRollRange(eo::DEFAULT)))]++;
	within(seen[fe::CHOCOLATE], 1.0 / 5);
	within(seen[fe::GARLIC], 1.0 / 5);
	within(seen[0], 3.0 / 5);
	seen.clear();
	for (int i = 0; i < N; ++i) seen[RollPogo(roll(PogoRollRange(eo::ALL)))]++;   // "all elites": half and half
	within(seen[fe::CHOCOLATE], 0.5);
	CHECK(seen[0] == 0);
	int visible = 0;
	for (int i = 0; i < N; ++i) visible += PogoGarlicVisible(roll(PogoGarlicVisibleRollRange())) ? 1 : 0;
	within(N - visible, 1.0 / 4);   // 1 in 4 garlic Pogos are invisible
}

// The elite option "all" (pvz-unnamed-pack SOURCES-shop-ui.md, 0x7b2000): Jack Rand(2) (0x796a58 = 2), Balloon and
// Digger roll jumps become `jo` (0x7a62b0, 0x79747e): drawn, always hit.
TEST_CASE("FogAllElitesOver2000Rolls") {
	FogRulesOn on;
	std::mt19937 gen(4245);
	auto roll = [&](int n) { return std::uniform_int_distribution<int>(0, n - 1)(gen); };
	const int N = 2000;
	auto within = [&](int count, double p) { // 4 standard deviations
		const double sd = std::sqrt(N * p * (1 - p));
		CHECK_MESSAGE(std::abs(count - N * p) <= 4 * sd, count << " vs " << N * p);
	};
	CHECK(JackRollRange(eo::ALL) == 2);
	CHECK(BalloonRollRange(eo::ALL) == 7);
	CHECK(DiggerRollRange(eo::ALL) == 5);
	CHECK(JackRollRange(eo::NONE) == 0);
	CHECK(BalloonRollRange(eo::NONE) == 0);
	CHECK(DiggerRollRange(eo::NONE) == 0);
	std::map<int, int> seen;
	for (int i = 0; i < N; ++i) seen[RollJack(roll(JackRollRange(eo::ALL)))]++;
	within(seen[fe::ICE_JACK], 0.5);
	within(seen[fe::POTATO_JACK], 0.5);
	CHECK(seen[0] == 0);
	seen.clear();
	for (int i = 0; i < N; ++i) seen[RollBalloon(roll(BalloonRollRange(eo::ALL)), eo::ALL)]++;
	CHECK(seen[fe::CLOUD] == N);
	seen.clear();
	for (int i = 0; i < N; ++i) seen[RollDigger(roll(DiggerRollRange(eo::ALL)), eo::ALL)]++;
	CHECK(seen[fe::SCREWDRIVER] == N);
	CHECK(JackRollRange(eo::DEFAULT) == JackRollRange());
	CHECK(BalloonRollRange(eo::DEFAULT) == BalloonRollRange());
	CHECK(DiggerRollRange(eo::DEFAULT) == DiggerRollRange());
	for (int r = 0; r < 10; ++r) {
		CHECK(RollBalloon(r, eo::DEFAULT) == RollBalloon(r));
		CHECK(RollDigger(r, eo::DEFAULT) == RollDigger(r));
	}
}

TEST_CASE("FogHitPoints") {
	FogRulesOn on;
	CHECK(FogBodyHp(ZOMBIE_JACK_IN_THE_BOX, 0, 500) == 870);            // every Jack (patch 0x522fc1)
	CHECK(FogBodyHp(ZOMBIE_JACK_IN_THE_BOX, fe::ICE_JACK, 500) == 870);
	CHECK(FogBodyHp(ZOMBIE_BALLOON, 0, 270) == 270);
	CHECK(FogBodyHp(ZOMBIE_BALLOON, fe::CLOUD, 270) == 190);            // Almanac 240 = 190 + 50
	CHECK(FogBodyHp(ZOMBIE_POGO, fe::CHOCOLATE, 500) == 300);
	CHECK(FogBodyHp(ZOMBIE_POGO, fe::GARLIC, 500) == 500);
	CHECK(FogBodyHp(ZOMBIE_DIGGER, fe::SCREWDRIVER, 270) == 270);
	CHECK(FogBodyHp(ZOMBIE_NORMAL, fe::CLOUD, 270) == 270);
	CHECK(FogHelmHp(ZOMBIE_DIGGER, 100) == 370);                        // 640 in all (patch 0x522be9)
	CHECK(FogHelmHp(ZOMBIE_PAIL, 1100) == 1100);
	CHECK(FogFlyingHp(ZOMBIE_BALLOON, 20) == 50);                       // patch 0x5234b9
	CHECK(FogFlyingHp(ZOMBIE_NORMAL, 0) == 0);
}

TEST_CASE("JackExplosions") {
	FogRulesOn on;
	CHECK(JackExplosion(0, false, false) == JackBlast::VANILLA);
	CHECK(JackExplosion(0, true, false) == JackBlast::VANILLA);
	CHECK(JackExplosion(fe::ICE_JACK, false, false) == JackBlast::FREEZE_PLANTS);
	CHECK(JackExplosion(fe::ICE_JACK, false, true) == JackBlast::FREEZE_PLANTS);
	CHECK(JackExplosion(fe::ICE_JACK, true, false) == JackBlast::ICE_ZOMBIES);
	CHECK(JackExplosion(fe::POTATO_JACK, false, false) == JackBlast::PACKETS_COOL);
	CHECK(JackExplosion(fe::POTATO_JACK, false, true) == JackBlast::VANILLA);   // I, Zombie: vanilla blast
	CHECK(JackExplosion(fe::POTATO_JACK, true, false) == JackBlast::PACKETS_READY);
	CHECK(PotatoPacketCount() == 9);
	CHECK(IceFreezeCounter(12) == 4200);
	CHECK(IceFreezeCounter(0) == 0);
}

TEST_CASE("IceJackFreezeLastsOneCounterPerPlantUpdate") {
	FogRulesOn on;
	// 3 plants, one update each per tick: 350 ticks of freeze
	IceFreeze f{1, IceFreezeCounter(3)};
	int frozenTicks = 0;
	for (int tick = 0; tick < 1000 && f.active; ++tick) {
		bool anySkipped = false;
		for (int p = 0; p < 3; ++p) anySkipped = IceFreezeStep(f) || anySkipped;
		frozenTicks += anySkipped ? 1 : 0;
	}
	CHECK(frozenTicks == 350);
	CHECK(f.active == 0);
	// boundary: counter 2 -> the first update is skipped (1 left), the second ends it and runs
	IceFreeze g{1, 2};
	CHECK(IceFreezeStep(g));
	CHECK(g == IceFreeze{1, 1});
	CHECK_FALSE(IceFreezeStep(g));
	CHECK(g.active == 0);
	CHECK_FALSE(IceFreezeStep(g));   // inactive: nothing changes
	CHECK(g.counter == 0);
	// an ice head on an empty lawn: flag set, counter 0, the next plant ends it at once
	IceFreeze h{1, IceFreezeCounter(0)};
	CHECK_FALSE(IceFreezeStep(h));
	CHECK(h.active == 0);
}

TEST_CASE("IceJackFreezeSurvivesSaveLoad") {
	BoardState s;
	s.iceFreezeActive = 1;
	s.iceFreezeCounter = 1234;
	auto b = Encode(s);
	CHECK(b.size() == 8);
	BoardState s2;
	REQUIRE(Decode(b.data(), b.size(), s2));
	CHECK(s2 == s);
	BoardState s3{1, 9};
	CHECK_FALSE(Decode(b.data(), 4, s3));   // too short: zeroed, no freeze
	CHECK(s3 == BoardState{});
	CHECK(BoardState{}.IsDefault());
	CHECK_FALSE(s.IsDefault());   // only a running freeze is saved (vanilla and ordinary saves stay unchanged)
}

TEST_CASE("Balloon") {
	FogRulesOn on;
	// hook 521: non-cloud, not walking -> 0.45; cloud or walking -> vanilla pick; other types untouched
	REQUIRE(BalloonVelX(ZOMBIE_BALLOON, 0, false).has_value());
	CHECK(*BalloonVelX(ZOMBIE_BALLOON, 0, false) == doctest::Approx(0.45f));
	CHECK_FALSE(BalloonVelX(ZOMBIE_BALLOON, 0, true).has_value());
	CHECK_FALSE(BalloonVelX(ZOMBIE_BALLOON, fe::CLOUD, false).has_value());
	CHECK_FALSE(BalloonVelX(ZOMBIE_NORMAL, 0, false).has_value());
	// cave 0x651615: mowers skip a Balloon while it has its balloon
	CHECK(MowerSkips(ZOMBIE_BALLOON, 50));
	CHECK(MowerSkips(ZOMBIE_BALLOON, 1));
	CHECK_FALSE(MowerSkips(ZOMBIE_BALLOON, 0));
	CHECK_FALSE(MowerSkips(ZOMBIE_NORMAL, 50));
	// patch 0x52f63e + hook 676
	CHECK(FlyersEat());
	for (int s : {int(SEED_WALLNUT), int(SEED_POTATOMINE), int(SEED_TALLNUT), int(SEED_PUMPKINSHELL), int(SEED_EXPLODE_O_NUT), int(SEED_GIANT_WALLNUT), int(SEED_GARLIC), 103})
		CHECK_MESSAGE(FlyingBalloonSkipsPlant(s), s);
	for (int s : {SEED_PEASHOOTER, SEED_SUNFLOWER, SEED_CHOMPER, SEED_SPIKEWEED, SEED_PLANTERN, SEED_CACTUS})
		CHECK_MESSAGE(!FlyingBalloonSkipsPlant(s), s);
	// hook 676 compares the raw seed (0x959420): the Charmed Tall-nut 0x67 is listed, Holy-fire Pumpkin 0x6e is not
	CHECK(FlyingBalloonSkipsPlant(0x67));
	CHECK_FALSE(FlyingBalloonSkipsPlant(0x6e));
	// hook 551: the cloud never pops while flying
	CHECK(CloudKeepsFlying(fe::CLOUD, true));
	CHECK_FALSE(CloudKeepsFlying(fe::CLOUD, false));
	CHECK_FALSE(CloudKeepsFlying(0, true));
}

TEST_CASE("Digger") {
	FogRulesOn on;
	// hook 654: a tunneling Digger bites these from underground
	for (int s : {SEED_LILYPAD, SEED_FLOWERPOT, SEED_WALLNUT, SEED_TALLNUT, SEED_PUMPKINSHELL, SEED_CHOMPER, SEED_TORCHWOOD,
	              SEED_CACTUS, SEED_SPIKEWEED, SEED_SPIKEROCK}) {
		REQUIRE(TunnelingDiggerTargets(s).has_value());
		CHECK_MESSAGE(*TunnelingDiggerTargets(s), s);
	}
	CHECK_FALSE(TunnelingDiggerTargets(SEED_POTATOMINE).has_value());   // vanilla: only while unarmed
	REQUIRE(TunnelingDiggerTargets(SEED_PEASHOOTER).has_value());
	CHECK_FALSE(*TunnelingDiggerTargets(SEED_PEASHOOTER));
	CHECK_FALSE(*TunnelingDiggerTargets(SEED_PLANTERN));
	// hook 654 compares the raw seed (0x795787): mod plants on a Wall-nut / Tall-nut / Pumpkin / Chomper base are not bitten
	for (int s : {0x53, 0x67, 0x6e, 0x56}) {
		REQUIRE(TunnelingDiggerTargets(s).has_value());
		CHECK_FALSE_MESSAGE(*TunnelingDiggerTargets(s), s);
	}
	// hook 585: surfaces at x < surfaceX + 10 (normal Diggers have surfaceX 0 -> x < 10 as vanilla)
	CHECK(DiggerSurfaceBelowX(0, 10.0f) == doctest::Approx(10.0f));
	CHECK(DiggerSurfaceBelowX(80, 10.0f) == doctest::Approx(90.0f));
	CHECK(DiggerSurfaceBelowX(160, 10.0f) == doctest::Approx(170.0f));
	CHECK(DiggerSurfaceBelowX(240, 10.0f) == doctest::Approx(250.0f));
	// hook 586
	CHECK(DiggerRisesWithoutAxe(fe::SCREWDRIVER));
	CHECK_FALSE(DiggerRisesWithoutAxe(0));
}

TEST_CASE("Pogo") {
	FogRulesOn on;
	// no wind-up (patch 0x5256eb): a target in reach -> forward bounce at once, vx = (|dx| + 60) / 80
	CHECK(PogoBounceEnd(0, std::nullopt, false) == PogoNext{PogoMove::BOUNCE, 0.0f});
	CHECK(PogoBounceEnd(0, 40, false) == PogoNext{PogoMove::FORWARD, 100.0f / 80.0f});
	CHECK(PogoBounceEnd(0, -40, false) == PogoNext{PogoMove::FORWARD, 100.0f / 80.0f});
	CHECK(PogoBounceEnd(fe::GARLIC, 0, false) == PogoNext{PogoMove::FORWARD, 60.0f / 80.0f});
	// chocolate: always the forward bounce, dx fixed 36 -> 1.2 px/cs, even with nothing in reach
	CHECK(PogoBounceEnd(fe::CHOCOLATE, std::nullopt, false) == PogoNext{PogoMove::FORWARD, 96.0f / 80.0f});
	CHECK(PogoBounceEnd(fe::CHOCOLATE, 200, false) == PogoNext{PogoMove::FORWARD, 96.0f / 80.0f});
	CHECK(PogoTargetsZombies());
	// garlic: yucky face every bounce (lane change)
	CHECK(PogoTurnsYucky(fe::GARLIC));
	CHECK_FALSE(PogoTurnsYucky(fe::CHOCOLATE));
	CHECK_FALSE(PogoTurnsYucky(0));
	CHECK(PogoYuckyCounter() == 100);
	// hook 532: Tall-nut, Giant Wall-nut, Tall-nut+80 (103)
	CHECK(PogoBonksOn(SEED_TALLNUT));
	CHECK(PogoBonksOn(SEED_GIANT_WALLNUT));
	CHECK(PogoBonksOn(103));
	CHECK_FALSE(PogoBonksOn(SEED_WALLNUT));
	CHECK_FALSE(PogoBonksOn(SEED_PUMPKINSHELL));
	// hook 531: landing on opposite-side zombies: 1800 first / 200 others, chocolate x1.6667 (3000 / 333)
	CHECK(PogoLandsOnZombies());
	CHECK(PogoLandRectGrowPx() == 32);
	CHECK(PogoLandDamage(0, true) == 1800);
	CHECK(PogoLandDamage(0, false) == 200);
	CHECK(PogoLandDamage(fe::GARLIC, true) == 1800);
	CHECK(PogoLandDamage(fe::CHOCOLATE, true) == 3000);
	CHECK(PogoLandDamage(fe::CHOCOLATE, false) == 333);
}

TEST_CASE("SpawnOffsetSkipsJack") {
	// X3 (hook 480 0x959509): the +133 px start offset skips Jack-in-the-Box (Day rules, already in place)
	SetRules({{"dayZombies.startOffsetPx", 133}});
	CHECK(StartOffsetPx(ZOMBIE_JACK_IN_THE_BOX) == 0.0f);
	CHECK(StartOffsetPx(ZOMBIE_BALLOON) == 133.0f);
	CHECK(StartOffsetPx(ZOMBIE_DIGGER) == 133.0f);
	CHECK(StartOffsetPx(ZOMBIE_POGO) == 133.0f);
	SetRules(nlohmann::json::object());
}

TEST_CASE("FogEliteArtTable") {
	auto ice = FogArtFor(ZOMBIE_JACK_IN_THE_BOX, fe::ICE_JACK);
	REQUIRE(ice.images.size() == 1);
	CHECK(ice.images[0] == FogArtImage{"anim_head1", "IMAGE_BLANK"});
	CHECK(ice.hidePrefixes == std::vector<std::string>{"anim_hair", "anim_head2"});
	CHECK(ice.headReanim == ReanimationType::REANIM_ICESHROOM);
	auto potato = FogArtFor(ZOMBIE_JACK_IN_THE_BOX, fe::POTATO_JACK);
	CHECK(potato.headReanim == ReanimationType::REANIM_POTATOMINE);
	CHECK(potato.hidePrefixes == ice.hidePrefixes);
	// helper 0x794800 plays the literal "anim_head_idle" (0x668c34) when 0x800120 is 0, as it is for the Jack (hook 500)
	CHECK(ice.headAnim == "anim_head_idle");
	CHECK(potato.headAnim == "anim_head_idle");
	auto cloud = FogArtFor(ZOMBIE_BALLOON, fe::CLOUD);
	REQUIRE(cloud.images.size() == 1);
	CHECK(cloud.images[0] == FogArtImage{"Zombie_balloon_top", "particles/Zombie_boss_iceball_particles"});
	CHECK(cloud.hidePrefixes.empty());
	auto screw = FogArtFor(ZOMBIE_DIGGER, fe::SCREWDRIVER);
	REQUIRE(screw.images.size() == 1);
	CHECK(screw.images[0] == FogArtImage{"Zombie_digger_pickaxe", "particles/Zomboss_particles"});
	auto choc = FogArtFor(ZOMBIE_POGO, fe::CHOCOLATE);
	REQUIRE(choc.images.size() == 1);
	CHECK(choc.images[0] == FogArtImage{"anim_head1", "images/Chocolate"});
	CHECK(choc.hidePrefixes == std::vector<std::string>{"anim_head2"});
	auto garlic = FogArtFor(ZOMBIE_POGO, fe::GARLIC);
	REQUIRE(garlic.images.size() == 1);
	CHECK(garlic.images[0] == FogArtImage{"anim_head1", "reanim/Garlic_body3"});
	CHECK(garlic.hidePrefixes == std::vector<std::string>{"anim_head2"});
	for (ZombieType t : {ZOMBIE_JACK_IN_THE_BOX, ZOMBIE_BALLOON, ZOMBIE_DIGGER, ZOMBIE_POGO})
		CHECK(FogArtFor(t, 0).empty());
	CHECK(FogArtFor(ZOMBIE_NORMAL, fe::CHOCOLATE).empty());
	// without the file the base look is drawn (the form keeps its behaviour); IMAGE_BLANK is never asked
	auto none = [](const std::string&) { return false; };
	CHECK(ResolveFogArt(ZOMBIE_POGO, fe::CHOCOLATE, none).empty());
	CHECK(ResolveFogArt(ZOMBIE_JACK_IN_THE_BOX, fe::ICE_JACK, none).headReanim == ReanimationType::REANIM_ICESHROOM);
}

TEST_CASE("FogHeadAnimTrackFallsBackToTrackZero") {
	// Reanimation::FindTrackIndex 0x472f30 returns 0 for a missing name, so "anim_head_idle" on a plant reanim without
	// that track plays the span of its first track (the research notes (l1-jack-pogo-looks)).
	const std::vector<std::string> potato = {"anim_armed", "anim_rise", "anim_idle", "anim_blink", "anim_mashed", "PotatoMine_rock4"};
	const std::vector<std::string> ice = {"anim_idle", "anim_sleep", "anim_face", "IceShroom_base"};
	const std::vector<std::string> pea = {"anim_idle", "anim_head_idle", "anim_shooting"};
	CHECK(FogHeadAnimTrack("anim_head_idle", potato) == "anim_armed");   // the armed, popped-up Potato Mine
	CHECK(FogHeadAnimTrack("anim_head_idle", ice) == "anim_idle");       // the Ice-shroom: same look as before
	CHECK(FogHeadAnimTrack("anim_head_idle", pea) == "anim_head_idle");  // found: played as named
	CHECK(FogHeadAnimTrack("ANIM_HEAD_IDLE", pea) == "ANIM_HEAD_IDLE");  // the match ignores case (stricmp)
	CHECK(FogHeadAnimTrack("anim_head_idle", {}) == "anim_head_idle");   // no tracks: nothing to fall back to
}

TEST_CASE("FogPackRulesMatchTheseValues") {
	for (const auto& k : FogZombieRuleKeys()) CHECK_MESSAGE(kFogRules.contains(k), k);
	CHECK(FogZombieRuleKeys().size() == kFogRules.size());
	const char* home = std::getenv("HOME");
	fs::path file = std::getenv("PVZU_PACK_DIR") ? fs::path(std::getenv("PVZU_PACK_DIR"))
	                                              : fs::path(home ? home : "") / "pvz-unnamed-pack";
	file /= "data/rules-fog-zombies.json";
	if (!fs::exists(file)) { MESSAGE("pack not here, skipped: " << file.string()); return; }
	std::ifstream in(file);
	nlohmann::json pack = nlohmann::json::parse(in, nullptr, false);
	REQUIRE(pack.is_object());
	CHECK(pack == kFogRules);
}
