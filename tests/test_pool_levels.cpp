// PVZU: Unnamed Pool (Adventure 3-1..3-10) level rules (the research notes (w3-pool) "Pool rules" and "Levels 3-1..3-10").
// Rules come from a fixture with the same numbers as pvz-unnamed-pack/data/rules-pool-levels.json.
#include "doctest/doctest.h"
#include "Mod/Unnamed/PoolLevels.h"
#include "Mod/Unnamed/Rules.h"

#include <string>

using namespace pvzu::unnamed;

namespace {
nlohmann::json PoolLevelRules() {
	nlohmann::json r = {
		{"poolLevels.finalWave.gravesAfterPool", 1},
		{"poolLevels.poolSpawn.passesHypnosis", 1},
		{"poolLevels.poolSpawn.hypnoRollRange", 3},
		{"poolLevels.iceTrail.timerBelowX", 933},
	};
	// exe Board::SpawnZombiesFromPool 0x4128F0..0x41296C: level -> {zombies, points}
	for (int level = 21; level <= 30; level++) {
		const std::string k = "poolLevels.poolSpawn." + std::to_string(level);
		const bool two = level == 21 || level == 22;
		const bool five = level >= 23 && level <= 25;
		r[k + ".count"] = two ? 2 : 3;
		r[k + ".points"] = two ? 3 : five ? 5 : 7;
	}
	return r;
}

struct RulesGuard {
	explicit RulesGuard(const nlohmann::json& r) { SetRules(r); }
	~RulesGuard() { SetRules(nlohmann::json::object()); }
};

// One row of the study's "Levels 3-1..3-10" table: the final-wave pool spawn of each level.
struct LevelRow {
	int level;
	const char* name;
	int poolCount;
	int poolPoints;
};
constexpr LevelRow kLevels[] = {
	{21, "3-1", 2, 3}, {22, "3-2", 2, 3}, {23, "3-3", 3, 5}, {24, "3-4", 3, 5}, {25, "3-5", 3, 5},
	{26, "3-6", 3, 7}, {27, "3-7", 3, 7}, {28, "3-8", 3, 7}, {29, "3-9", 3, 7}, {30, "3-10", 3, 7},
};
}

TEST_CASE("PoolLevelsPerLevelTable") {
	RulesGuard g(PoolLevelRules());
	for (const LevelRow& row : kLevels) {
		const std::string aName = row.name;
		CAPTURE(aName);
		PoolSpawn s;
		REQUIRE(PoolFinalWaveSpawn(row.level, s));
		CHECK(s.count == row.poolCount);
		CHECK(s.points == row.poolPoints);
		// Level-independent pool rules (no .NewSec code compares the level with 21..30)
		CHECK(PoolGravesAfterPoolSpawn());
		CHECK(PoolRisersTakeHypnosis());
		CHECK(PoolHypnosisRollRange() == 3);
		CHECK(IceTrailTimerBelowX(800) == 933);
	}
	// Levels outside world 3 have no row: the engine keeps its own counts (fog 4-1/4-2 also use 2/3 there)
	PoolSpawn s;
	CHECK_FALSE(PoolFinalWaveSpawn(20, s));
	CHECK_FALSE(PoolFinalWaveSpawn(31, s));
	CHECK_FALSE(PoolFinalWaveSpawn(0, s));
}

TEST_CASE("PoolLevelsMissingRulesAreVanilla") {
	RulesGuard g(nlohmann::json::object());
	PoolSpawn s;
	for (const LevelRow& row : kLevels)
		CHECK_FALSE(PoolFinalWaveSpawn(row.level, s));
	CHECK_FALSE(PoolGravesAfterPoolSpawn());
	CHECK_FALSE(PoolRisersTakeHypnosis());
	CHECK(PoolHypnosisRollRange() == 0);
	CHECK(IceTrailTimerBelowX(800) == 800);
}

TEST_CASE("PoolSpawnRowNeedsBothKeysAndSaneValues") {
	PoolSpawn s;
	{
		RulesGuard g(nlohmann::json{{"poolLevels.poolSpawn.21.count", 2}});
		CHECK_FALSE(PoolFinalWaveSpawn(21, s)); // points missing
	}
	{
		RulesGuard g(nlohmann::json{{"poolLevels.poolSpawn.21.count", 0}, {"poolLevels.poolSpawn.21.points", 3}});
		CHECK_FALSE(PoolFinalWaveSpawn(21, s)); // zero zombies is not a row
	}
	{
		// the engine's grid has 4 columns x 2 rows = 8 pool cells: a larger count is capped, never overruns it
		RulesGuard g(nlohmann::json{{"poolLevels.poolSpawn.21.count", 50}, {"poolLevels.poolSpawn.21.points", 3}});
		REQUIRE(PoolFinalWaveSpawn(21, s));
		CHECK(s.count == kPoolSpawnCells);
	}
}
