// PVZU: Unnamed Fog (Adventure 4-1..4-10) level rules (the research notes (w4-fog) sections 4 and 5, U9, U12).
// Rules come from a fixture with the same numbers as pvz-unnamed-pack/data/rules-fog-levels.json (the last test compares
// the two). Waves, flags, allowed zombies, start sun and award plants of 4-1..4-10 are vanilla in the exe tables; they
// are checked on the Mac (-pvzu-log-waves, both modes) and listed in the pack's SOURCES-fog-levels.md.
#include "doctest/doctest.h"
#include "Mod/Unnamed/FogLevels.h"
#include "Mod/Unnamed/Levels.h"
#include "Mod/Unnamed/PoolLevels.h"
#include "Mod/Unnamed/Rules.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;
using namespace pvzu::unnamed;

namespace {
// One row of the study's "Levels 4-1..4-10" table: what the rules decide per level.
struct LevelRow {
	int level;
	const char* name;
	int fogColumn;       // LeftFogColumn (exe 0x41C1CC, vanilla): 4-1 6, 4-2..4-6 5, 4-7..4-10 4
	int poolCount;       // final-wave pool spawn (exe 0x4128F0): 31/32 -> 2/3, 33..35 -> 3/5, else 3/7
	int poolPoints;
	int vanillaMultiplier; // vanilla wave-budget multiplier (4-10 stormy night x3), Unnamed x4 everywhere (hook 22)
};
constexpr LevelRow kLevels[] = {
	{31, "4-1", 6, 2, 3, 1}, {32, "4-2", 5, 2, 3, 1}, {33, "4-3", 5, 3, 5, 1}, {34, "4-4", 5, 3, 5, 1},
	{35, "4-5", 5, 3, 5, 1}, {36, "4-6", 5, 3, 7, 1}, {37, "4-7", 4, 3, 7, 1}, {38, "4-8", 4, 3, 7, 1},
	{39, "4-9", 4, 3, 7, 1}, {40, "4-10", 4, 3, 7, 3},
};

const nlohmann::json kFogRules = [] {
	nlohmann::json r = {{"fogLevels.fullFog", 0}};
	for (const LevelRow& row : kLevels) {
		r["fogLevels.fogColumn." + std::to_string(row.level)] = row.fogColumn;
		const std::string k = "poolLevels.poolSpawn." + std::to_string(row.level);
		r[k + ".count"] = row.poolCount;
		r[k + ".points"] = row.poolPoints;
	}
	return r;
}();

nlohmann::json WithDayBudget(nlohmann::json r) {
	r["dayLevels.waveBudgetMultiplier"] = 4; // rules-day-levels.json (Plan 2, hook 22)
	return r;
}

struct RulesGuard {
	explicit RulesGuard(const nlohmann::json& r) { SetRules(r); }
	~RulesGuard() { SetRules(nlohmann::json::object()); }
};
}

TEST_CASE("FogLevelsPerLevelTable") {
	RulesGuard g(WithDayBudget(kFogRules));
	CHECK_FALSE(FullFogOn());
	for (const LevelRow& row : kLevels) {
		const std::string aName = row.name;
		CAPTURE(aName);
		CHECK(FogStartColumn(row.level, true) == row.fogColumn);
		PoolSpawn s;
		REQUIRE(PoolFinalWaveSpawn(row.level, s));
		CHECK(s.count == row.poolCount);
		CHECK(s.points == row.poolPoints);
		// Hook 22: every adventure level x4, so 4-10 goes from vanilla x3 to x4 and the others from x1 to x4.
		for (int aBefore : {1, 7, 25}) {
			CHECK(WaveBudget(aBefore, aBefore * row.vanillaMultiplier, true) == aBefore * 4);
		}
	}
	// No row outside world 4 or outside Adventure: the engine keeps its own column (5 for minigames, 6 Air Raid).
	CHECK(FogStartColumn(30, true) == kNoFogRule);
	CHECK(FogStartColumn(41, true) == kNoFogRule);
	CHECK(FogStartColumn(31, false) == kNoFogRule);
}

TEST_CASE("FogLevelsMissingRulesAreVanilla") {
	RulesGuard g(nlohmann::json::object());
	CHECK_FALSE(FullFogOn());
	for (const LevelRow& row : kLevels) {
		CHECK(FogStartColumn(row.level, true) == kNoFogRule);
		PoolSpawn s;
		CHECK_FALSE(PoolFinalWaveSpawn(row.level, s));
	}
}

TEST_CASE("FullFogFlagCoversTheWholeLawn") {
	// Hook 145 (0x7A8310): option byte 0x800298 != 0 -> LeftFogColumn returns 0 before any level or mode test.
	nlohmann::json r = kFogRules;
	r["fogLevels.fullFog"] = 1;
	RulesGuard g(r);
	CHECK(FullFogOn());
	for (const LevelRow& row : kLevels)
		CHECK(FogStartColumn(row.level, true) == 0);
	CHECK(FogStartColumn(5, false) == 0); // minigames and Air Raid too (the hook sits at the function's entry)
}

TEST_CASE("FullFogPanelOptionCoversTheWholeLawn") {
	// R8 (the research notes (r8-options) 2): the panel's full-screen fog row sets the same 0x800298 test as the rule.
	RulesGuard g(kFogRules); // fogLevels.fullFog 0
	CHECK_FALSE(FullFogOn());
	CHECK(FullFogOn(true));
	CHECK(FogStartColumn(31, true, true) == 0);
	CHECK(FogStartColumn(5, false, true) == 0);
	CHECK(FogStartColumn(5, false, false) == kNoFogRule);
	CHECK(FogStartColumn(31, true, false) == FogStartColumn(31, true));
}

TEST_CASE("FogColumnRowMustBeOnTheLawn") {
	// LeftFogColumn feeds mFogOffset = 1065 - col * 80 and the fog grid loops (Board.cpp DrawFog): columns 0..8 only.
	for (int aBad : {-1, 9, 40}) {
		CAPTURE(aBad);
		RulesGuard g(nlohmann::json{{"fogLevels.fogColumn.31", aBad}});
		CHECK(FogStartColumn(31, true) == kNoFogRule);
	}
	RulesGuard g(nlohmann::json{{"fogLevels.fogColumn.31", 0}});
	CHECK(FogStartColumn(31, true) == 0);
}

TEST_CASE("FogPackRulesMatchTheseValues") {
	const char* home = std::getenv("HOME");
	fs::path file = std::getenv("PVZU_PACK_DIR") ? fs::path(std::getenv("PVZU_PACK_DIR"))
	                                              : fs::path(home ? home : "") / "pvz-unnamed-pack";
	file /= "data/rules-fog-levels.json";
	if (!fs::exists(file)) { MESSAGE("pack not here, skipped: " << file.string()); return; }
	std::ifstream in(file);
	nlohmann::json pack = nlohmann::json::parse(in, nullptr, false);
	REQUIRE(pack.is_object());
	CHECK(pack == kFogRules);
}
