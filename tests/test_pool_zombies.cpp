// PVZU: Plan 4 Task 2, Pool zombies and elite forms (pure rules; evidence in the research notes (w3-pool) Z1-Z5 and
// pvz-unnamed-pack/SOURCES-pool-zombies.md).
#include "doctest/doctest.h"
#include "Mod/Unnamed/Options.h"
#include "Mod/Unnamed/PoolZombies.h"
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
namespace pe = pvzu::unnamed::poolelite;
namespace zs = pvzu::unnamed::zamboni;
namespace eo = pvzu::unnamed::eliteoption;

namespace {

// The Unnamed values (same as pvz-unnamed-pack/data/rules-pool-zombies.json; the last test compares the two).
const nlohmann::json kPoolRules = {
	{"poolZombies.snorkel.eliteRoll", 4},
	{"poolZombies.snorkel.eliteHit", 1},
	{"poolZombies.snorkel.lotusIgnoresPlants", 1},
	{"poolZombies.dolphin.hp", 870},
	{"poolZombies.dolphin.eliteRoll", 3},
	{"poolZombies.dolphin.eliteHit", 1},
	{"poolZombies.dolphin.eliteJumpVx", 2.0},
	{"poolZombies.zamboni.freezePlants", 1},
	{"poolZombies.zamboni.freezeRows", 1},
	{"poolZombies.zamboni.freezeHalfWidthPx", 120},
	{"poolZombies.zamboni.freezeTintRed", 0},
	{"poolZombies.zamboni.freezeTintGreen", 160},
	{"poolZombies.zamboni.iceShroomFreezes", 1},
	{"poolZombies.zamboni.iceMaxX", 933},
	{"poolZombies.zamboni.eliteRoll", 9},
	{"poolZombies.zamboni.eliteHit", 0},
	{"poolZombies.zamboni.eliteVx", 0.2},
	{"poolZombies.zamboni.redVx", 0.45},
	{"poolZombies.zamboni.redVxAtBodyHp", 600},
	{"poolZombies.zamboni.eliteArmorFlat", 5},
	{"poolZombies.zamboni.eliteArmorMul", 0.8},
	{"poolZombies.zamboni.eliteBreakHp", 200},
	{"poolZombies.zamboni.eliteRedHp", 350},
	{"poolZombies.zamboni.tintEliteR", 255},
	{"poolZombies.zamboni.tintEliteG", 172},
	{"poolZombies.zamboni.tintEliteB", 0},
	{"poolZombies.zamboni.tintRedR", 192},
	{"poolZombies.zamboni.tintRedG", 16},
	{"poolZombies.zamboni.tintRedB", 16},
};

struct PoolRulesOn {
	PoolRulesOn() { SetRules(kPoolRules); }
	~PoolRulesOn() { SetRules(nlohmann::json::object()); }
};

} // namespace

TEST_CASE("PoolVanillaWithoutRules") {
	SetRules(nlohmann::json::object());
	CHECK(PoolRollRange(ZOMBIE_SNORKEL) == 0);
	CHECK(PoolRollRange(ZOMBIE_DOLPHIN_RIDER) == 0);
	CHECK(PoolRollRange(ZOMBIE_ZAMBONI) == 0);
	CHECK(PoolBodyHp(ZOMBIE_DOLPHIN_RIDER, 500) == 500);
	CHECK(DolphinJumpVelX(pe::FOOTBALL_HEAD, 0.5f) == doctest::Approx(0.5f));
	CHECK(PoolEatsPlants(ZOMBIE_SNORKEL, pe::LOTUS));
	CHECK_FALSE(ZamboniFreezesPlants(zs::NORMAL, false, HEIGHT_ZOMBIE_NORMAL));
	CHECK_FALSE(PoolFreezeGateOn());
	CHECK(ZamboniVelX(zs::NORMAL, 600, 0.1f) == doctest::Approx(0.1f));
	CHECK(ZamboniIceMaxX(800) == 800);
	CHECK(ZamboniEliteHit(zs::ORANGE, 20, 1000).damage == 20);
	CHECK_FALSE(ZamboniTint(zs::ORANGE).has_value());
}

TEST_CASE("PoolRollBoundaries") {
	PoolRulesOn on;
	// Snorkel: Rand(4) == 1 -> lotus (hook 499)
	CHECK(PoolRollRange(ZOMBIE_SNORKEL) == 4);
	CHECK(RollSnorkel(0) == 0);
	CHECK(RollSnorkel(1) == pe::LOTUS);
	CHECK(RollSnorkel(2) == 0);
	CHECK(RollSnorkel(3) == 0);
	// Dolphin: Rand(3) == 1 -> football head (hook 494)
	CHECK(PoolRollRange(ZOMBIE_DOLPHIN_RIDER) == 3);
	CHECK(RollDolphin(0) == 0);
	CHECK(RollDolphin(1) == pe::FOOTBALL_HEAD);
	CHECK(RollDolphin(2) == 0);
	// Zomboni: Rand(9) == 0 -> orange (hook 496); never in mode 0x89 or adventure level 65
	CHECK(PoolRollRange(ZOMBIE_ZAMBONI) == 9);
	CHECK(RollZamboni(0) == zs::ORANGE);
	CHECK(RollZamboni(1) == zs::NORMAL);
	CHECK(RollZamboni(8) == zs::NORMAL);
	CHECK(ZamboniRollAllowed(0, true, 26));
	CHECK_FALSE(ZamboniRollAllowed(0, true, 65));
	CHECK(ZamboniRollAllowed(13, false, 65));
	CHECK_FALSE(ZamboniRollAllowed(0x89, false, 1));
	// other types never roll here (Ducky Tube keeps the Day Normal roll, Bobsled is vanilla)
	CHECK(PoolRollRange(ZOMBIE_DUCKY_TUBE) == 0);
	CHECK(PoolRollRange(ZOMBIE_BOBSLED) == 0);
	CHECK(IsPoolType(ZOMBIE_SNORKEL));
	CHECK(IsPoolType(ZOMBIE_ZAMBONI));
	CHECK(IsPoolType(ZOMBIE_DOLPHIN_RIDER));
	CHECK_FALSE(IsPoolType(ZOMBIE_BOBSLED));
	CHECK(std::string(PoolEliteName(ZOMBIE_SNORKEL, pe::LOTUS, 0)) == "lotus");
	CHECK(std::string(PoolEliteName(ZOMBIE_DOLPHIN_RIDER, pe::FOOTBALL_HEAD, 0)) == "football-head");
	CHECK(std::string(PoolEliteName(ZOMBIE_ZAMBONI, 0, zs::ORANGE)) == "orange");
	CHECK(std::string(PoolEliteName(ZOMBIE_ZAMBONI, 0, zs::RED)) == "red");
	CHECK(std::string(PoolEliteName(ZOMBIE_ZAMBONI, 0, 0)) == "none");
}

TEST_CASE("PoolEliteRatesOver2000Rolls") {
	PoolRulesOn on;
	std::mt19937 gen(4243);
	auto roll = [&](int n) { return std::uniform_int_distribution<int>(0, n - 1)(gen); };
	const int N = 2000;
	auto within = [&](int count, double p) { // 4 standard deviations
		const double sd = std::sqrt(N * p * (1 - p));
		CHECK_MESSAGE(std::abs(count - N * p) <= 4 * sd, count << " vs " << N * p);
	};
	std::map<int, int> seen;
	for (int i = 0; i < N; ++i) seen[RollSnorkel(roll(PoolRollRange(ZOMBIE_SNORKEL)))]++;
	within(seen[pe::LOTUS], 0.25);
	within(seen[0], 0.75);
	seen.clear();
	for (int i = 0; i < N; ++i) seen[RollDolphin(roll(PoolRollRange(ZOMBIE_DOLPHIN_RIDER)))]++;
	within(seen[pe::FOOTBALL_HEAD], 1.0 / 3);
	within(seen[0], 2.0 / 3);
	seen.clear();
	for (int i = 0; i < N; ++i) seen[RollZamboni(roll(PoolRollRange(ZOMBIE_ZAMBONI)))]++;
	within(seen[zs::ORANGE], 1.0 / 9);
	within(seen[zs::NORMAL], 8.0 / 9);
	CHECK(seen.count(zs::RED) == 0);   // red only comes from an orange one breaking
}

// The elite option "all" (pvz-unnamed-pack SOURCES-shop-ui.md): 0x7b2000 turns the Snorkel / Dolphin roll jumps into
// `jo` (the Rand is drawn, the jump never taken); the Zomboni reads the value (0x95a40d) and is orange with no roll.
TEST_CASE("PoolAllElitesOver2000Rolls") {
	PoolRulesOn on;
	std::mt19937 gen(99);
	auto roll = [&](int n) { return std::uniform_int_distribution<int>(0, n - 1)(gen); };
	const int N = 2000;
	CHECK(PoolRollRange(ZOMBIE_SNORKEL, eo::ALL) == 4);
	CHECK(PoolRollRange(ZOMBIE_DOLPHIN_RIDER, eo::ALL) == 3);
	CHECK(PoolRollRange(ZOMBIE_ZAMBONI, eo::ALL) == 0);
	for (ZombieType t : {ZOMBIE_SNORKEL, ZOMBIE_DOLPHIN_RIDER, ZOMBIE_ZAMBONI}) CHECK(PoolRollRange(t, eo::NONE) == 0);
	int lotus = 0, football = 0;
	for (int i = 0; i < N; ++i) {
		lotus += RollSnorkel(roll(PoolRollRange(ZOMBIE_SNORKEL, eo::ALL)), eo::ALL) == pe::LOTUS ? 1 : 0;
		football += RollDolphin(roll(PoolRollRange(ZOMBIE_DOLPHIN_RIDER, eo::ALL)), eo::ALL) == pe::FOOTBALL_HEAD ? 1 : 0;
	}
	CHECK(lotus == N);
	CHECK(football == N);
	CHECK(ZamboniAlwaysOrange(eo::ALL));
	CHECK_FALSE(ZamboniAlwaysOrange(eo::DEFAULT));
	CHECK_FALSE(ZamboniAlwaysOrange(eo::NONE));
	for (int r = 0; r < 10; ++r) { // option 0 through the new parameter = the calls without it
		CHECK(RollSnorkel(r, eo::DEFAULT) == RollSnorkel(r));
		CHECK(RollDolphin(r, eo::DEFAULT) == RollDolphin(r));
	}
	for (ZombieType t : {ZOMBIE_SNORKEL, ZOMBIE_DOLPHIN_RIDER, ZOMBIE_ZAMBONI}) CHECK(PoolRollRange(t, eo::DEFAULT) == PoolRollRange(t));
}

TEST_CASE("PoolAllElitesNeedThePackRoll") {
	SetRules(nlohmann::json::object());
	CHECK_FALSE(ZamboniAlwaysOrange(eo::ALL));
	CHECK(PoolRollRange(ZOMBIE_SNORKEL, eo::ALL) == 0);
}

TEST_CASE("DolphinRider") {
	PoolRulesOn on;
	CHECK(PoolBodyHp(ZOMBIE_DOLPHIN_RIDER, 500) == 870);   // every Dolphin Rider (0x522D5E)
	CHECK(PoolBodyHp(ZOMBIE_SNORKEL, 270) == 270);
	CHECK(PoolBodyHp(ZOMBIE_ZAMBONI, 1350) == 1350);
	CHECK(DolphinJumpVelX(pe::FOOTBALL_HEAD, 0.5f) == doctest::Approx(2.0f)); // hook 559
	CHECK(DolphinJumpVelX(0, 0.5f) == doctest::Approx(0.5f));
}

TEST_CASE("SnorkelLotusIgnoresPlants") {
	PoolRulesOn on;
	CHECK_FALSE(PoolEatsPlants(ZOMBIE_SNORKEL, pe::LOTUS));   // hook 675 chain 0x795F8B
	CHECK(PoolEatsPlants(ZOMBIE_SNORKEL, 0));
	CHECK(PoolEatsPlants(ZOMBIE_NORMAL, pe::LOTUS));          // 0xB means lotus only on a Snorkel
}

TEST_CASE("ZamboniFreezeBoxAndImmunity") {
	PoolRulesOn on;
	// gate (hook 620): not hypnotised, not bungee-dropped (height 9), not the orange elite
	CHECK(ZamboniFreezesPlants(zs::NORMAL, false, HEIGHT_ZOMBIE_NORMAL));
	CHECK(ZamboniFreezesPlants(zs::RED, false, HEIGHT_ZOMBIE_NORMAL));
	CHECK_FALSE(ZamboniFreezesPlants(zs::ORANGE, false, HEIGHT_ZOMBIE_NORMAL));
	CHECK_FALSE(ZamboniFreezesPlants(zs::NORMAL, true, HEIGHT_ZOMBIE_NORMAL));
	CHECK_FALSE(ZamboniFreezesPlants(zs::NORMAL, false, HEIGHT_GETTING_BUNGEE_DROPPED));
	// box: |row diff| <= 1 and z.x - 120 <= plant.x <= z.x + 120 (0x794320)
	CHECK(InZamboniFreezeBox(2, 500, 2, 500));
	CHECK(InZamboniFreezeBox(2, 500, 1, 380));
	CHECK(InZamboniFreezeBox(2, 500, 3, 620));
	CHECK_FALSE(InZamboniFreezeBox(2, 500, 3, 621));
	CHECK_FALSE(InZamboniFreezeBox(2, 500, 2, 379));
	CHECK_FALSE(InZamboniFreezeBox(2, 500, 0, 500));
	CHECK_FALSE(InZamboniFreezeBox(2, 500, 4, 500));
	// immune: Snow Pea, Ice-shroom, Winter Melon, Torchwood, Jalapeno, Spikeweed, Tangle Kelp, Split Pea, Spikerock,
	// the mod's plant codes 0x7A, 0x56, 0x55, and any plant in the square of a 0x6E (mod Pumpkin)
	for (int s : {5, 14, 44, 22, 20, 21, 19, 28, 46, 0x7A, 0x56, 0x55})
		CHECK_MESSAGE(ZamboniFreezeImmune(s, false), s);
	// Peashooter, Sunflower, Wall-nut, Chomper, Repeater, Lily Pad, Squash, Threepeater, Tall-nut, Starfruit, Pumpkin
	// and Gloom-shroom (the plain 6/42/30 are not the mod's +80 codes)
	for (int s : {0, 1, 3, 6, 7, 16, 17, 18, 23, 29, 30, 42})
		CHECK_MESSAGE(!ZamboniFreezeImmune(s, false), s);
	CHECK(ZamboniFreezeImmune(0, true));
	CHECK(ZamboniFreezeTintRed() == 0);
	CHECK(ZamboniFreezeTintGreen() == 160);
}

TEST_CASE("ZamboniEliteSpeedArmourTint") {
	PoolRulesOn on;
	// speed (hook 598): orange 0.2 fixed, red 0.45; any Zomboni at exactly 600 body HP 0.45; else the vanilla curve
	CHECK(ZamboniVelX(zs::ORANGE, 1350, 0.25f) == doctest::Approx(0.2f));
	CHECK(ZamboniVelX(zs::RED, 300, 0.05f) == doctest::Approx(0.45f));
	CHECK(ZamboniVelX(zs::NORMAL, 600, 0.1f) == doctest::Approx(0.45f));
	CHECK(ZamboniVelX(zs::NORMAL, 601, 0.1f) == doctest::Approx(0.1f));
	// armour (0x95A476): d' = round((d - 5) x 0.8), d <= 5 -> 0; d' + 200 >= HP drops the hit and turns it red
	CHECK(ZamboniEliteHit(zs::ORANGE, 5, 1350).damage == 0);
	CHECK(ZamboniEliteHit(zs::ORANGE, 6, 1350).damage == 1);
	CHECK(ZamboniEliteHit(zs::ORANGE, 20, 1350).damage == 12);
	CHECK(ZamboniEliteHit(zs::ORANGE, 20, 1350).state == zs::ORANGE);
	CHECK(ZamboniEliteHit(zs::ORANGE, 40, 1350).damage == 28);
	const ZamboniHit big = ZamboniEliteHit(zs::ORANGE, 1800, 1350);   // a Jalapeno: d' 1436 + 200 >= 1350
	CHECK(big.damage == 0);
	CHECK(big.state == zs::RED);
	CHECK(ZamboniEliteHit(zs::ORANGE, 20, 212).state == zs::RED);   // 12 + 200 >= 212
	CHECK(ZamboniEliteHit(zs::ORANGE, 20, 213).damage == 12);       // 212 < 213
	CHECK(ZamboniEliteHit(zs::RED, 20, 300).damage == 20);          // red: normal damage
	CHECK(ZamboniEliteHit(zs::NORMAL, 20, 300).damage == 20);
	// hook 613: an orange one at 350 HP or less turns red
	CHECK(ZamboniStateAfterTick(zs::ORANGE, 351) == zs::ORANGE);
	CHECK(ZamboniStateAfterTick(zs::ORANGE, 350) == zs::RED);
	CHECK(ZamboniStateAfterTick(zs::NORMAL, 100) == zs::NORMAL);
	// tint (hook 647)
	REQUIRE(ZamboniTint(zs::ORANGE).has_value());
	CHECK(*ZamboniTint(zs::ORANGE) == Rgb{255, 172, 0});
	CHECK(*ZamboniTint(zs::RED) == Rgb{192, 16, 16});
	CHECK_FALSE(ZamboniTint(zs::NORMAL).has_value());
	// wide lawn: ice timer bound 800 -> 933
	CHECK(ZamboniIceMaxX(800) == 933);
}

TEST_CASE("IceShroomFreezesZamboni") {
	PoolRulesOn on;
	// hook 712: CanBeChilled is replaced (the Zomboni is no longer refused); new refusals
	CHECK(PoolFreezeGateOn());
	CHECK(PoolFreezeGate(0, false, true, 1350));
	CHECK(PoolFreezeGate(0, false, true, 1000));
	CHECK_FALSE(PoolFreezeGate(0x100, false, true, 1350));
	CHECK_FALSE(PoolFreezeGate(0, true, true, 1350));
	CHECK_FALSE(PoolFreezeGate(0, false, false, 1350));
	for (int hp : {256, 512, 768, 1024, 1280}) // low byte of the body HP == 0 refuses (exe quirk)
		CHECK_MESSAGE(!PoolFreezeGate(0, false, true, hp), hp);
	// hook 713: a Zomboss with 1 HP cannot be frozen
	CHECK(BossRefusesFreeze(ZOMBIE_BOSS, 1));
	CHECK_FALSE(BossRefusesFreeze(ZOMBIE_BOSS, 2));
	CHECK_FALSE(BossRefusesFreeze(ZOMBIE_ZAMBONI, 1));
}

TEST_CASE("PoolEliteArtTable") {
	auto lotus = PoolArtFor(ZOMBIE_SNORKEL, pe::LOTUS);
	REQUIRE(lotus.images.size() == 1);
	CHECK(lotus.images[0] == PoolArtImage{"anim_head1", "IMAGE_BLANK"});
	CHECK(lotus.hidePrefixes == std::vector<std::string>{"anim_hair", "anim_head2"});
	CHECK(lotus.lilyPadHead);
	auto dolphin = PoolArtFor(ZOMBIE_DOLPHIN_RIDER, pe::FOOTBALL_HEAD);
	REQUIRE(dolphin.images.size() == 1);
	CHECK(dolphin.images[0] == PoolArtImage{"anim_head1", "particles/ZombieFootballHead"});
	CHECK(dolphin.showTracks == std::vector<std::string>{"anim_head2"});
	CHECK_FALSE(dolphin.lilyPadHead);
	CHECK(PoolArtFor(ZOMBIE_ZAMBONI, 0).empty());
	CHECK(PoolArtFor(ZOMBIE_SNORKEL, 0).empty());
	CHECK(PoolArtFor(ZOMBIE_DOLPHIN_RIDER, 0).empty());
	// without the head file the base look is drawn (the form keeps its behaviour); IMAGE_BLANK is never asked
	auto none = [](const std::string&) { return false; };
	CHECK(ResolvePoolArt(ZOMBIE_DOLPHIN_RIDER, pe::FOOTBALL_HEAD, none).empty());
	CHECK(ResolvePoolArt(ZOMBIE_SNORKEL, pe::LOTUS, none).lilyPadHead);
}

TEST_CASE("ZamboniFreezeSurvivesSaveLoad") {
	// the frozen flag lives in PlantState and goes into the save TLV
	PlantState p{7, 9, 1};
	auto b = Encode(p);
	CHECK(b.size() >= 28); // 12 in Plan 4; Plan 5 added the Split Pea sweep fields; Plan 8 the mod seed
	PlantState p2;
	REQUIRE(Decode(b.data(), b.size(), p2));
	CHECK(p2 == p);
	CHECK(p2.frozen == 1);
	// 8-byte blobs (before Plan 4) still load, not frozen
	PlantState p3{1, 2, 1};
	REQUIRE(Decode(b.data(), 8, p3));
	CHECK(p3.counter == 7);
	CHECK(p3.frozen == 0);
	// the Zomboni's orange/red state is ZombieState.extra (saved since Plan 2)
	ZombieState z{0, 0, zs::RED, 0, 0, 0};
	auto zb = Encode(z);
	ZombieState z2;
	REQUIRE(Decode(zb.data(), zb.size(), z2));
	CHECK(z2.extra == zs::RED);
}

TEST_CASE("PoolPackRulesMatchTheseValues") {
	for (const auto& k : PoolZombieRuleKeys()) CHECK_MESSAGE(kPoolRules.contains(k), k);
	CHECK(PoolZombieRuleKeys().size() == kPoolRules.size());
	const char* home = std::getenv("HOME");
	fs::path file = std::getenv("PVZU_PACK_DIR") ? fs::path(std::getenv("PVZU_PACK_DIR"))
	                                              : fs::path(home ? home : "") / "pvz-unnamed-pack";
	file /= "data/rules-pool-zombies.json";
	if (!fs::exists(file)) { MESSAGE("pack not here, skipped: " << file.string()); return; }
	std::ifstream in(file);
	nlohmann::json pack = nlohmann::json::parse(in, nullptr, false);
	REQUIRE(pack.is_object());
	CHECK(pack == kPoolRules);
}
