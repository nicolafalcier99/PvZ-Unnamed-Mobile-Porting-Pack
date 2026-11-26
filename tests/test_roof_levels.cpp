// PVZU: Unnamed Roof (Adventure 5-1..5-9) level rules (the research notes (w5-roof) sections 0, 3 and 5: G1, G5, G6,
// R1..R5). Rules come from a fixture with the same numbers as pvz-unnamed-pack/data/rules-roof-levels.json (the last test
// compares the two). Waves, flags, allowed zombies, start sun, pre-placed pots and award plants of 5-1..5-9 are vanilla
// in the exe tables; they are checked on the Mac (-pvzu-log-waves, both modes) and listed in SOURCES-roof-levels.md.
// 5-10 (the Dr. Zomboss fight) is Plan 6 Task 4.
#include "doctest/doctest.h"
#include "Mod/Unnamed/Levels.h"
#include "Mod/Unnamed/RoofLevels.h"
#include "Mod/Unnamed/Rules.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <random>
#include <set>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace pvzu::unnamed;

namespace {
// One row of the study's "Per-level table 5-1..5-10": what the rules decide per level.
struct LevelRow {
	int level;
	const char* name;
	int skyCount;          // final-wave sky drop (exe Board::SpawnZombiesFromSky 0x412C30): 41/42 -> 2/3,
	int skyPoints;         // 43..45 -> 3/5, else 3/7 (vanilla, R1)
	int vanillaMultiplier; // vanilla wave-budget multiplier: 5-5 Bungee Blitz x2, the others x1; Unnamed x4 (G1, R3)
};
constexpr LevelRow kLevels[] = {
	{41, "5-1", 2, 3, 1}, {42, "5-2", 2, 3, 1}, {43, "5-3", 3, 5, 1}, {44, "5-4", 3, 5, 1}, {45, "5-5", 3, 5, 2},
	{46, "5-6", 3, 7, 1}, {47, "5-7", 3, 7, 1}, {48, "5-8", 3, 7, 1}, {49, "5-9", 3, 7, 1},
};

const nlohmann::json kRoofRules = [] {
	nlohmann::json r = nlohmann::json::object();
	for (const LevelRow& row : kLevels) {
		const std::string k = "roofLevels.skyDrop." + std::to_string(row.level);
		r[k + ".count"] = row.skyCount;
		r[k + ".points"] = row.skyPoints;
	}
	return r;
}();

// The other packs' keys these rules read: rules-day-levels.json (hook 22 x4) and rules-day-zombies.json (hook 85:
// one Rand(25) site for every sky drop, the Buckethead umbrella's and the roof's final wave).
nlohmann::json WithDayKeys(nlohmann::json r) {
	r["dayLevels.waveBudgetMultiplier"] = 4;
	r["dayZombies.skyDrop.typeRollRange"] = 25;
	return r;
}

struct RulesGuard {
	explicit RulesGuard(const nlohmann::json& r) { SetRules(r); }
	~RulesGuard() { SetRules(nlohmann::json::object()); }
};

// R1: the 13 zombie types a sky drop may bring (ZombieType values): Normal, Flag, Cone, Pole, Bucket, Newspaper,
// Screen Door, Football, Jack-in-the-Box, Ladder, Catapult, Gargantuar, Imp.
const std::set<int> kSkyTypes = {0, 1, 2, 3, 4, 5, 6, 7, 15, 21, 22, 23, 24};
constexpr int kCone = 2; // what the vanilla picker returns in these tests
}

TEST_CASE("RoofLevelsPerLevelTable") {
	RulesGuard g(WithDayKeys(kRoofRules));
	for (const LevelRow& row : kLevels) {
		const std::string aName = row.name;
		CAPTURE(aName);
		SkyDrop s;
		REQUIRE(RoofFinalWaveSkyDrop(row.level, s));
		CHECK(s.count == row.skyCount);
		CHECK(s.points == row.skyPoints);
		// G1 / R3: every adventure level x4, so 5-5 Bungee Blitz goes from vanilla x2 to x4, the others from x1.
		for (int aBefore : {1, 7, 25})
			CHECK(WaveBudget(aBefore, aBefore * row.vanillaMultiplier, true) == aBefore * 4);
	}
	// 5-10 is the boss level (Task 4) and the other worlds have no sky drop row: the engine keeps its own counts.
	SkyDrop s;
	CHECK_FALSE(RoofFinalWaveSkyDrop(40, s));
	CHECK_FALSE(RoofFinalWaveSkyDrop(50, s));
}

TEST_CASE("RoofLevelsMissingRulesAreVanilla") {
	RulesGuard g(nlohmann::json::object());
	for (const LevelRow& row : kLevels) {
		SkyDrop s;
		CHECK_FALSE(RoofFinalWaveSkyDrop(row.level, s));
		CHECK(WaveBudget(5, 5 * row.vanillaMultiplier, true) == 5 * row.vanillaMultiplier);
	}
	int aCalls = 0, aPicks = 0;
	CHECK(SkyDropType([&] { ++aPicks; return kCone; }, [&](int) { ++aCalls; return 0; }) == kCone); // vanilla picker only
	CHECK(aPicks == 1);
	CHECK(aCalls == 0);
}

TEST_CASE("RoofSkyDropCountStaysOnTheDropGrid") {
	// Board::SetupBungeeDrop: columns 4..8 x rows 0..4 = 25 cells; the engine also clamps to that count.
	{
		RulesGuard g(nlohmann::json{{"roofLevels.skyDrop.41.count", 40}, {"roofLevels.skyDrop.41.points", 3}});
		SkyDrop s;
		REQUIRE(RoofFinalWaveSkyDrop(41, s));
		CHECK(s.count == kSkyDropCells);
	}
	for (const nlohmann::json& r : {nlohmann::json{{"roofLevels.skyDrop.41.count", 0}, {"roofLevels.skyDrop.41.points", 3}},
	                                nlohmann::json{{"roofLevels.skyDrop.41.count", 2}},
	                                nlohmann::json{{"roofLevels.skyDrop.41.count", 2}, {"roofLevels.skyDrop.41.points", -1}}}) {
		RulesGuard g(r);
		SkyDrop s;
		CHECK_FALSE(RoofFinalWaveSkyDrop(41, s));
	}
}

TEST_CASE("SkyDropTypeIsUniformOverThirteenTypes") {
	// R1 / hook 85 (0x795C27): Rand(25) drawn again until it is 0..7, 15 or 21..24. 2000 drops never bring an excluded
	// type (Dancer, Backup, the pool/air types, Yeti, Bungee) and each of the 13 comes about 1/13 of the time.
	RulesGuard g(WithDayKeys(kRoofRules));
	std::mt19937 aGen(20261123);
	int aDraws = 0;
	auto aRand = [&](int n) {
		CHECK(n == 25);
		++aDraws;
		return std::uniform_int_distribution<int>(0, n - 1)(aGen);
	};
	std::map<int, int> aSeen;
	constexpr int kDrops = 2000;
	int aPicks = 0;
	for (int i = 0; i < kDrops; ++i) {
		const int aType = SkyDropType([&] { ++aPicks; return kCone; }, aRand);
		CAPTURE(aType);
		REQUIRE(kSkyTypes.count(aType) == 1);
		aSeen[aType]++;
	}
	CHECK(aSeen.size() == kSkyTypes.size());
	CHECK(aPicks == kDrops); // the vanilla pick is still drawn once per drop, its result unused
	// Expected 2000/13 = 153.8 each, sd about 11.9: 5 sd either side.
	for (const auto& [aType, aCount] : aSeen) {
		CAPTURE(aType);
		CHECK(aCount > 94);
		CHECK(aCount < 214);
	}
	// 12 of 25 rolls are drawn again: about 25/13 = 1.92 draws per drop.
	CHECK(aDraws > kDrops * 18 / 10);
	CHECK(aDraws < kDrops * 21 / 10);
}

TEST_CASE("SkyDropRerollsEveryExcludedRoll") {
	RulesGuard g(WithDayKeys(kRoofRules));
	for (int aExcluded = 0; aExcluded < 25; ++aExcluded) {
		if (kSkyTypes.count(aExcluded))
			continue;
		CAPTURE(aExcluded);
		std::vector<int> aRolls = {aExcluded, aExcluded, 21};
		size_t aNext = 0;
		CHECK(SkyDropType([] { return kCone; }, [&](int) { return aRolls[aNext++]; }) == 21);
		CHECK(aNext == 3);
	}
	for (int aType : kSkyTypes) {
		int aCalls = 0;
		CHECK(SkyDropType([] { return kCone; }, [&](int) { ++aCalls; return aType; }) == aType);
		CHECK(aCalls == 1);
	}
}

TEST_CASE("SkyDropDrawsTheVanillaPickFirst") {
	// Board::SpawnZombiesFromSky 0x412CA3 still calls PickGraveRisingZombieType (0x40D770, weighted Rand) before the
	// hook's jmp at 0x412CA8, so the exe's random sequence has that draw in front of the Rand(25) loop.
	RulesGuard g(WithDayKeys(kRoofRules));
	std::vector<std::string> aOrder;
	CHECK(SkyDropType([&] { aOrder.push_back("pick"); return kCone; }, [&](int) { aOrder.push_back("rand"); return 22; }) == 22);
	CHECK(aOrder == std::vector<std::string>{"pick", "rand"});
}

TEST_CASE("SkyDropRerollCannotHang") {
	RulesGuard g(WithDayKeys(kRoofRules));
	int aCalls = 0;
	CHECK(SkyDropType([] { return kCone; }, [&](int) { ++aCalls; return 9; }) == kCone); // a broken roll source: vanilla pick
	CHECK(aCalls > 0);
	CHECK(aCalls <= 1000);
}

TEST_CASE("RoofPackRulesMatchTheseValues") {
	const char* home = std::getenv("HOME");
	fs::path file = std::getenv("PVZU_PACK_DIR") ? fs::path(std::getenv("PVZU_PACK_DIR"))
	                                              : fs::path(home ? home : "") / "pvz-unnamed-pack";
	file /= "data/rules-roof-levels.json";
	if (!fs::exists(file)) { MESSAGE("pack not here, skipped: " << file.string()); return; }
	std::ifstream in(file);
	nlohmann::json pack = nlohmann::json::parse(in, nullptr, false);
	REQUIRE(pack.is_object());
	CHECK(pack == kRoofRules);
}
