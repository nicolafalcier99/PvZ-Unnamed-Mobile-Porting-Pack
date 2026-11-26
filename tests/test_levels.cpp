// PVZU: Unnamed Day waves and level rules (the research notes (levels)). Rolls are injected; rules come from a
// fixture object with the pack's rules-day-levels.json values.
#include "doctest/doctest.h"
#include "Mod/Unnamed/Levels.h"
#include "Mod/Unnamed/Rules.h"
#include "Mod/Unnamed/State.h"

#include <functional>
#include <vector>

using namespace pvzu::unnamed;

namespace {
// Same numbers as pvz-unnamed-pack/data/rules-day-levels.json.
nlohmann::json DayLevelRules() {
	return nlohmann::json{
		{"dayLevels.waveBudgetMultiplier", 4},
		{"dayLevels.forcedFlag.level", 1},
		{"dayLevels.forcedFlag.wave", 3},
		{"dayLevels.forcedFlag.slot", 1},
		{"dayLevels.allowCheckUsesStartingLevel", 0},
		{"dayLevels.poleGrave.enabled", 1},
		{"dayLevels.poleGrave.colDivisor", 80},
		{"dayLevels.poleGrave.colOffset", -1},
		{"dayLevels.poleGrave.alwaysVariant", 0x29},
		{"dayLevels.poleGrave.skipVariant", 0x32},
		{"dayLevels.poleGrave.vaseVariant", 0x103},
		{"dayLevels.poleBonk.seedTallnut", 23},
		{"dayLevels.poleBonk.seedTallnutPlus", 0x67},
		{"dayLevels.poleBonk.seedGiantWallnut", 0x32},
		{"dayLevels.poleBonk.seedCactus", 0x1a},
		{"dayLevels.poleBonk.cactusState", 0x20},
		{"dayLevels.graveRise.rerollRange", 25},
		{"dayLevels.graveRise.rerollAcceptMask", 25199359},
		{"dayLevels.graveRise.rerollAgainMask", 256},
		{"dayLevels.graveRise.rerollMisses", 2},
		{"dayLevels.graveRise.phaseByType", 1},
		{"dayLevels.graveRise.jackFuseBase", 300},
		{"dayLevels.graveRise.jackFuseRange", 1000},
		{"dayLevels.ashThreshold", 1800},
		{"dayLevels.ashThresholdUsesTotalHp", 1},
	};
}

struct RulesGuard {
	explicit RulesGuard(const nlohmann::json& r) { SetRules(r); }
	~RulesGuard() { SetRules(nlohmann::json::object()); }
};

// Hands out scripted rolls; records the ranges asked for.
struct Script {
	std::vector<int> rolls;
	std::vector<int> ranges;
	size_t next = 0;
	int operator()(int range) {
		ranges.push_back(range);
		return next < rolls.size() ? rolls[next++] : 0;
	}
};
}

TEST_CASE("WaveBudgetIsX4ForEveryAdventureLevel") {
	RulesGuard g(DayLevelRules());
	// 1-1 wave 1: 1 point, vanilla x1 -> 1; Unnamed x4 -> 4
	CHECK(WaveBudget(1, 1, true) == 4);
	// 1-10 (mini-boss, vanilla x3): 3 -> Unnamed 4
	CHECK(WaveBudget(1, 3, true) == 4);
	// 1-5 bowling (vanilla x4 already) stays 4
	CHECK(WaveBudget(1, 4, true) == 4);
	// flag-wave remainder 2 (after the fixed Normals + Flag) -> 8
	CHECK(WaveBudget(2, 2, true) == 8);
	// zero / negative remainders stay what x4 gives (picker spends nothing)
	CHECK(WaveBudget(0, 0, true) == 0);
	CHECK(WaveBudget(-1, -1, true) == -4);
	// not adventure: vanilla value kept (Day scope)
	CHECK(WaveBudget(5, 30, false) == 30);
}

TEST_CASE("WaveBudgetMissingRuleIsVanilla") {
	RulesGuard g(nlohmann::json::object());
	CHECK(WaveBudget(1, 1, true) == 1);
	CHECK(WaveBudget(1, 3, true) == 3);
}

TEST_CASE("ForcedFlagOnlyAtLevel1Wave4Slot2") {
	RulesGuard g(DayLevelRules());
	CHECK(ForceFlagAt(1, 3, 1));
	CHECK_FALSE(ForceFlagAt(1, 3, 0));
	CHECK_FALSE(ForceFlagAt(1, 3, 2));
	CHECK_FALSE(ForceFlagAt(1, 2, 1));
	CHECK_FALSE(ForceFlagAt(1, 4, 1));
	CHECK_FALSE(ForceFlagAt(2, 3, 1));
	CHECK_FALSE(ForceFlagAt(0, 3, 1));
	SetRules(nlohmann::json::object());
	CHECK_FALSE(ForceFlagAt(1, 3, 1));
}

TEST_CASE("AllowCheckIgnoresStartingLevel") {
	{
		RulesGuard g(DayLevelRules());
		CHECK_FALSE(AllowCheckUsesStartingLevel());
	}
	CHECK(AllowCheckUsesStartingLevel());
}

TEST_CASE("VaultGraveColumnBoundaries") {
	RulesGuard g(DayLevelRules());
	CHECK(VaultGraveColumn(0) == -1);
	CHECK(VaultGraveColumn(79) == -1);
	CHECK(VaultGraveColumn(80) == 0);
	CHECK(VaultGraveColumn(159) == 0);
	CHECK(VaultGraveColumn(160) == 1);
	CHECK(VaultGraveColumn(400) == 4);
	CHECK(VaultGraveColumn(799) == 8);
	CHECK(VaultGraveColumn(800) == 9);
	// exe divides the low 16 bits: (uint16)mX
	CHECK(VaultGraveColumn(65536 + 160) == 1);
	CHECK(VaultGraveColumn(-1) == 65535 / 80 - 1);
}

TEST_CASE("PlantBlocksVaultTallnutLike") {
	RulesGuard g(DayLevelRules());
	CHECK(PlantBlocksVault(23, 0));      // Tall-nut
	CHECK(PlantBlocksVault(0x67, 0));    // Tall-nut+80 (mod plant)
	CHECK(PlantBlocksVault(0x32, 0));    // Giant Wall-nut
	CHECK(PlantBlocksVault(0x1a, 0x20)); // Cactus, high
	CHECK_FALSE(PlantBlocksVault(0x1a, 0x1f));
	CHECK_FALSE(PlantBlocksVault(0x1a, 0x21));
	CHECK_FALSE(PlantBlocksVault(3, 0)); // Wall-nut
	CHECK_FALSE(PlantBlocksVault(0, 0)); // Peashooter
	SetRules(nlohmann::json::object());
	CHECK(PlantBlocksVault(23, 0));      // vanilla: Tall-nut only
	CHECK_FALSE(PlantBlocksVault(0x32, 0));
	CHECK_FALSE(PlantBlocksVault(0x1a, 0x20));
}

TEST_CASE("VaultDecision") {
	RulesGuard g(DayLevelRules());
	VaultInput in;
	in.hasPlant = true;
	in.plantSeed = 0; // Peashooter
	auto r = DecideVault(in);
	CHECK_FALSE(r.bonk);
	CHECK(r.grave);

	in.plantSeed = 23; // Tall-nut: bonk, no grave
	r = DecideVault(in);
	CHECK(r.bonk);
	CHECK_FALSE(r.grave);

	in.variant = 0x29; // always-grave variant skips the Tall-nut test
	r = DecideVault(in);
	CHECK_FALSE(r.bonk);
	CHECK(r.grave);

	in = {};
	in.hasPlant = false; // no target in the window: still a grave (exe: null plant -> grave path)
	r = DecideVault(in);
	CHECK_FALSE(r.bonk);
	CHECK(r.grave);

	in.hypnotised = true;
	CHECK_FALSE(DecideVault(in).grave);
	in.hypnotised = false;
	in.graveMade = true;
	CHECK_FALSE(DecideVault(in).grave);
	in.graveMade = false;
	in.variant = 0x32; // road-sign elite
	CHECK_FALSE(DecideVault(in).grave);
	in.variant = 0x103; // vase path, not a grave
	CHECK_FALSE(DecideVault(in).grave);

	SetRules(nlohmann::json::object()); // vanilla: never a grave
	in = {};
	in.hasPlant = true;
	r = DecideVault(in);
	CHECK_FALSE(r.grave);
	CHECK_FALSE(r.bonk);
}

TEST_CASE("GraveRerollAccept") {
	RulesGuard g(DayLevelRules());
	for (int r : {0, 1, 2, 3, 4, 5, 6, 7, 9, 15, 23, 24})
		CHECK_MESSAGE(GraveRerollAccept(r), r);
	for (int r : {8, 10, 11, 12, 13, 14, 16, 17, 18, 19, 20, 21, 22, 25, -1})
		CHECK_MESSAGE(!GraveRerollAccept(r), r);
}

TEST_CASE("GraveRiseTypeRerolls") {
	RulesGuard g(DayLevelRules());
	const int kVanilla = 2; // Conehead from the vanilla pick
	Script s;
	s.rolls = {0};
	CHECK(GraveRiseType(kVanilla, std::ref(s)) == 0);
	CHECK(s.ranges == std::vector<int>{25});

	s = {};
	s.rolls = {24};
	CHECK(GraveRiseType(kVanilla, std::ref(s)) == 24);

	s = {};
	s.rolls = {8, 8, 23}; // Dancer always rolls again and does not count as a miss
	CHECK(GraveRiseType(kVanilla, std::ref(s)) == 23);

	s = {};
	s.rolls = {10, 9}; // first miss rolls again
	CHECK(GraveRiseType(kVanilla, std::ref(s)) == 9);

	s = {};
	s.rolls = {10, 22}; // second miss keeps the vanilla pick
	CHECK(GraveRiseType(kVanilla, std::ref(s)) == kVanilla);
	CHECK(s.next == 2);

	s = {};
	s.rolls = {14, 8, 16}; // miss, dancer, miss -> vanilla
	CHECK(GraveRiseType(kVanilla, std::ref(s)) == kVanilla);
	CHECK(s.next == 3);
}

TEST_CASE("GraveRiseTypeVanillaWithoutRules") {
	RulesGuard g(nlohmann::json::object());
	Script s;
	s.rolls = {0};
	CHECK(GraveRiseType(4, std::ref(s)) == 4);
	CHECK(s.ranges.empty()); // no roll consumed: vanilla RNG stream unchanged
}

TEST_CASE("GraveRisePhaseByType") {
	RulesGuard g(DayLevelRules());
	CHECK(GraveRisePhase(3, 0) == 11);  // Pole Vaulter -> PRE_VAULT
	CHECK(GraveRisePhase(5, 0) == 29);  // Newspaper -> READING
	CHECK(GraveRisePhase(15, 0) == 15); // Jack-in-the-Box -> RUNNING
	CHECK(GraveRisePhase(0, 0) == 0);
	CHECK(GraveRisePhase(23, 0) == 0);
	CHECK(JackRiseFuse(0) == 300);
	CHECK(JackRiseFuse(999) == 1299);
	SetRules(nlohmann::json::object());
	CHECK(GraveRisePhase(3, 0) == 0);
}

TEST_CASE("AshThresholdUsesBodyHelmShield") {
	RulesGuard g(DayLevelRules());
	CHECK(AshThresholdHp(270, 1100, 0, true) == 1370);   // Buckethead still ashes
	CHECK(AshThresholdHp(270, 1100, 1100, true) == 2470);
	CHECK(AshThresholdHp(270, 1100, 1100, false) == 270);
	CHECK(AshThreshold() == 1800);
	CHECK_FALSE(TooToughToAsh(1799, 0, 0, true));
	CHECK(TooToughToAsh(1800, 0, 0, true));
	CHECK(TooToughToAsh(700, 1100, 0, true));  // 1800 total
	CHECK_FALSE(TooToughToAsh(699, 1100, 0, true));
	CHECK_FALSE(TooToughToAsh(700, 1100, 0, false));
	SetRules(nlohmann::json::object());
	CHECK_FALSE(TooToughToAsh(700, 1100, 0, true)); // missing rule: vanilla body-only test
	CHECK(TooToughToAsh(1800, 0, 0, true));
}

TEST_CASE("VaultGraveFlagSurvivesSaveLoad") {
	ZombieState z{0, 0, 0, kZombieFlagVaultGraveMade};
	auto b = Encode(z);
	ZombieState z2;
	REQUIRE(Decode(b.data(), b.size(), z2));
	CHECK(z2 == z);
	// a 12-byte blob from before the flags field: loads, flags read as 0
	ZombieState z3{7, 8, 9, 1};
	REQUIRE(Decode(b.data(), 12, z3));
	CHECK(z3.flags == 0);
}
