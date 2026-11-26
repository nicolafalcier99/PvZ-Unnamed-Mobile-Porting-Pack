// PVZU: Plan 9 R3, the world-7 Catapult projectiles 0x12 (vase) and 0x20 (bug spray) and the vase Catapult's volley
// (pure rules; evidence in the research notes (r3-catapult-shots) and pvz-unnamed-pack/SOURCES-catapult-shots.md).
#include "doctest/doctest.h"
#include "Mod/Unnamed/CatapultShots.h"
#include "Mod/Unnamed/ModShots.h"
#include "Mod/Unnamed/Rules.h"

#include "nlohmann/json.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <random>

using namespace pvzu::unnamed;
namespace fs = std::filesystem;

namespace {

// The Unnamed values (same as pvz-unnamed-pack/data/rules-catapult-shots.json; the last test compares the two).
const nlohmann::json kShotRules = {
	{"catapultShots.volleyRoll", 100},
	{"catapultShots.volleyBelow", 35},
	{"catapultShots.volleyShotRoll", 3},
	{"catapultShots.volleyLastCol", 8},
	{"catapultShots.volleyMinBalls", 1},
	{"catapultShots.volleySkipSeeds", {21, 46}},
	{"catapultShots.volleySkipModSeeds", {107}},
	{"catapultShots.ballTracksBelow", 5},
	{"catapultShots.vaseDamage", 300},
	{"catapultShots.vaseZombieRoll", 33},
	{"catapultShots.vaseZombieReroll", {11, 12, 13, 14, 16, 20, 25}},
	{"catapultShots.vaseReflectedFlags", 1},
	{"catapultShots.sprayDamage", 100},
	{"catapultShots.sprayKillRoll", 20},
	{"catapultShots.sprayRateStep", 75},
	{"catapultShots.sprayRateCapExtra", 750},
	{"catapultShots.sprayFoley", 71},
	{"catapultShots.sprayParticle", 88},
	{"catapultShots.sprayParticleDx", 34},
	{"catapultShots.sprayParticleDy", 28},
	{"catapultShots.sprayParticleRenderOrder", 400000},
};

struct ShotRulesOn {
	ShotRulesOn() { SetRules(kShotRules); }
	~ShotRulesOn() { SetRules(nlohmann::json::object()); }
};

constexpr int kVase = 0x70, kPoleHead = 0x19;

} // namespace

TEST_CASE("CatapultShots: without the pack's keys every rule plays vanilla") {
	SetRules(nlohmann::json::object());
	CHECK(CatapultVolleyRollRange(kVase, false) == 0);
	CHECK_FALSE(CatapultVolleys(0));
	CHECK(CatapultHiddenBallTracks(0) == -1);   // -1 = the vanilla == 4 / 3 / 2 / 1 hides
	CHECK_FALSE(VaseShotMakesVase(0x12, 0));
	CHECK(SprayKillRollRange() == 0);
	CHECK(SprayLaunchRate(150, 150) == 150);
	CHECK(VaseZombieRollRange() == 0);
}

TEST_CASE("CatapultShots: the vase catapult's volley roll (hook 547, 0x910135)") {
	ShotRulesOn on;
	CHECK(CatapultVolleyRollRange(kVase, false) == 100);
	CHECK(CatapultVolleyRollRange(kVase, true) == 0);       // hypnotised: never (0x958ed7)
	CHECK(CatapultVolleyRollRange(kPoleHead, false) == 0);  // only 0x70
	CHECK(CatapultVolleyRollRange(0, false) == 0);
	CHECK(CatapultVolleys(34));
	CHECK_FALSE(CatapultVolleys(35));
	// volley shots: (Rand(3) & 1) ? 0x12 : 0x20 (0x7a1961)
	CHECK(VolleyShotRollRange() == 3);
	CHECK(VolleyShotType(0) == kShotBugSpray);
	CHECK(VolleyShotType(1) == kShotVase);
	CHECK(VolleyShotType(2) == kShotBugSpray);
	CHECK(VolleyBallsAfter(-3) == 1);
	CHECK(VolleyBallsAfter(0) == 1);
	CHECK(VolleyBallsAfter(6) == 6);
}

TEST_CASE("CatapultShots: volley targets are the first plant of each column 0..8 in the row, no spikes, no 0x6b") {
	ShotRulesOn on;
	// iteration order of Board::IteratePlants: {col, seed, modSeed}
	const std::vector<VolleyCandidate> plants = {
		{3, 0, 0},     // Peashooter col 3
		{3, 30, 0},    // Pumpkin in the same cell: the column is taken
		{5, 21, 0},    // Spikeweed: skipped, the column stays free
		{5, 3, 0},     // Wall-nut col 5
		{6, 46, 0},    // Spikerock: skipped
		{7, 27, 107},  // mod 0x6b (on a Blover carrier): skipped
		{9, 0, 0},     // column 9: out of the 0..8 range
		{-1, 0, 0},
		{0, 16, 0},    // Lily Pad col 0
	};
	const std::vector<int> t = VolleyTargets(plants);
	CHECK(t == std::vector<int>{0, 3, 8});
	CHECK(VolleyTargets({}).empty());
}

TEST_CASE("CatapultShots: ball tracks hidden as balls drop below 5 (hook 549)") {
	ShotRulesOn on;
	CHECK(CatapultHiddenBallTracks(9) == 0);
	CHECK(CatapultHiddenBallTracks(5) == 0);
	CHECK(CatapultHiddenBallTracks(4) == 1);
	CHECK(CatapultHiddenBallTracks(3) == 2);
	CHECK(CatapultHiddenBallTracks(1) == 4);
	CHECK(CatapultHiddenBallTracks(0) == 4);   // the exe's fifth name (anim_pop) is no catapult track
}

TEST_CASE("CatapultShots: the vase shot kills the cell and leaves a random-zombie vase (hook 393, 0x7aa380)") {
	ShotRulesOn on;
	CHECK(VaseShotMakesVase(kShotVase, 0));
	CHECK(VaseShotMakesVase(kShotVase, 0x8d));
	CHECK_FALSE(VaseShotMakesVase(kShotVase, 1));    // a Leaf-Shield reflected shot
	CHECK_FALSE(VaseShotMakesVase(kShotBugSpray, 0));
	CHECK_FALSE(VaseShotMakesVase(0, 0));
	CHECK(VaseShotDamage() == 300);
	// plant filter: {seed, state, squished, dead, bungeeState}
	CHECK(VaseShotKillsPlant(0, 0, false, false, 0));
	CHECK(VaseShotKillsPlant(30, 0, false, false, 0));   // Pumpkin
	CHECK(VaseShotKillsPlant(17, 4, false, false, 0));   // Squash on the ground
	CHECK_FALSE(VaseShotKillsPlant(17, 5, false, false, 0));
	CHECK_FALSE(VaseShotKillsPlant(17, 7, false, false, 0));
	CHECK(VaseShotKillsPlant(0, 6, false, false, 0));    // states 5..7 matter only for the Squash
	CHECK_FALSE(VaseShotKillsPlant(0, 0, true, false, 0));
	CHECK_FALSE(VaseShotKillsPlant(0, 0, false, true, 0));
	CHECK_FALSE(VaseShotKillsPlant(0, 0, false, false, 2));
	CHECK(VaseShotKillsPlant(0, 0, false, false, 1));
	// zombie: Rand(33) re-rolled on 11-14, 16, 20, 25
	CHECK(VaseZombieRollRange() == 33);
	for (int z : {11, 12, 13, 14, 16, 20, 25}) CHECK_FALSE_MESSAGE(VaseZombieAllowed(z), z);
	for (int z : {0, 1, 10, 15, 17, 19, 21, 22, 23, 24, 26, 31, 32}) CHECK_MESSAGE(VaseZombieAllowed(z), z);
	CHECK_FALSE(VaseZombieAllowed(33));
	CHECK_FALSE(VaseZombieAllowed(-1));
}

TEST_CASE("CatapultShots: the bug spray hit (hook 394, 0x9102e7)") {
	ShotRulesOn on;
	CHECK(SprayDamage() == 100);
	CHECK(SprayKillRollRange() == 20);
	CHECK(SprayKills(0));
	CHECK_FALSE(SprayKills(1));
	CHECK_FALSE(SprayKills(19));
	// cap = base + 750; cap >= rate -> rate + 75, else rate = cap
	CHECK(SprayLaunchRate(150, 150) == 225);
	CHECK(SprayLaunchRate(899, 150) == 974);   // one step past the cap, as the exe
	CHECK(SprayLaunchRate(900, 150) == 975);
	CHECK(SprayLaunchRate(974, 150) == 900);   // then back to the cap
	CHECK(SprayLaunchRate(0, 0) == 75);        // non-shooters get the number too
	CHECK(SprayFoley() == 71);
	CHECK(SprayParticle() == 88);
	CHECK(SprayParticleOffset() == std::pair<int, int>{34, 28});
	CHECK(SprayParticleRenderOrder() == 400000);
	// rates: 1 in 20 kills
	std::mt19937 rng(20261123);
	int kills = 0;
	for (int i = 0; i < 4000; ++i) kills += SprayKills(std::uniform_int_distribution<int>(0, 19)(rng));
	CHECK(kills > 150);
	CHECK(kills < 250);
}

TEST_CASE("CatapultShots: 0x12 and 0x20 ride on the basketball carrier") {
	CHECK(ModShotCarrier(kShotVase) == 9);
	CHECK(ModShotCarrier(kShotBugSpray) == 9);
	CHECK(IsModShotType(kShotVase));
	CHECK(IsModShotType(kShotBugSpray));
	CHECK(IsCatapultShotType(kShotVase));
	CHECK(IsCatapultShotType(kShotBugSpray));
	CHECK_FALSE(IsCatapultShotType(9));
	CHECK_FALSE(IsCatapultShotType(kShotPrism));
	CHECK(ModShotTint(kShotVase, 0) == 0);   // own art, no tint
	CHECK(ModShotTint(kShotBugSpray, 0) == 0);
}

TEST_CASE("CatapultShots: pack rules match these values") {
	for (const auto& k : CatapultShotRuleKeys()) CHECK_MESSAGE(kShotRules.contains(k), k);
	CHECK(CatapultShotRuleKeys().size() == kShotRules.size());
	const char* home = std::getenv("HOME");
	fs::path dir = std::getenv("PVZU_PACK_DIR") ? fs::path(std::getenv("PVZU_PACK_DIR"))
	                                             : fs::path(home ? home : "") / "pvz-unnamed-pack";
	fs::path file = dir / "data/rules-catapult-shots.json";
	if (!fs::exists(file)) { MESSAGE("pack not here, skipped: " << file.string()); return; }
	std::ifstream in(file);
	nlohmann::json pack = nlohmann::json::parse(in, nullptr, false);
	REQUIRE(pack.is_object());
	CHECK(pack == kShotRules);
	std::ifstream mj(dir / "mod.json");
	nlohmann::json mod = nlohmann::json::parse(mj, nullptr, false);
	REQUIRE(mod.is_object());
	CHECK(mod["features"].value("catapultShots", false));
}
