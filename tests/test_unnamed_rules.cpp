#include "doctest/doctest.h"
#include "Mod/GameData.h"
#include "Mod/ModPack.h"
#include "Mod/Unnamed/Rules.h"
#include "Mod/Unnamed/State.h"

#include <filesystem>

using namespace pvzu;
namespace fs = std::filesystem;

TEST_CASE("RulesLoadFromPack") {
	std::string err;
	auto pack = LoadModPack(fs::path(PVZU_FIXTURES_DIR) / "pack-good", &err);
	REQUIRE(pack.has_value());
	CHECK(pack->rules.is_object());
	unnamed::SetRules(pack->rules);
	CHECK(unnamed::RuleInt("snowPea.fireballMaxRoll", 0) == 33);
	unnamed::SetRules(nlohmann::json::object());
}

TEST_CASE("MissingRuleKeyFallsBackToVanilla") {
	unnamed::SetRules(nlohmann::json::object());
	CHECK(unnamed::RuleInt("snowPea.fireballMaxRoll", -1) == -1);
	unnamed::SetRules(nlohmann::json{{"x", "not a number"}});
	CHECK(unnamed::RuleInt("x", 7) == 7);
	CHECK(unnamed::RuleFloat("y", 1.5f) == doctest::Approx(1.5f));
	unnamed::SetRules(nlohmann::json{{"y", 3}});
	CHECK(unnamed::RuleFloat("y", 1.5f) == doctest::Approx(3.0f));
	unnamed::SetRules(nlohmann::json::object());
}

TEST_CASE("UnnamedStateSurvivesSaveLoad") {
	unnamed::ZombieState z{5, 17, 2};
	unnamed::PlantState p{12, -3};
	auto zb = unnamed::Encode(z);
	auto pb = unnamed::Encode(p);
	unnamed::ZombieState z2;
	unnamed::PlantState p2;
	REQUIRE(unnamed::Decode(zb.data(), zb.size(), z2));
	REQUIRE(unnamed::Decode(pb.data(), pb.size(), p2));
	CHECK(z2 == z);
	CHECK(p2 == p);
	// short / empty blobs (older format) decode as zeros and report false
	unnamed::ZombieState z3{9, 9, 9};
	CHECK_FALSE(unnamed::Decode(zb.data(), 3, z3));
	CHECK(z3 == unnamed::ZombieState{});
}

TEST_CASE("LevelTablesMergeFromSeveralFiles") {
	// data/levels-*.json (Plan 7: levels-51-73.json): one object keyed by level number, merged like rules-*.json.
	std::string err;
	auto pack = LoadModPack(fs::path(PVZU_FIXTURES_DIR) / "pack-levels", &err);
	REQUIRE_MESSAGE(pack.has_value(), err);
	REQUIRE(pack->levels.is_object());
	CHECK(pack->levels.size() == 2);
	CHECK(pack->levels["51"]["background"] == "NIGHT_ROOF");
	CHECK(pack->levels["61"]["waves"] == 20);
	auto none = LoadModPack(fs::path(PVZU_FIXTURES_DIR) / "pack-good", &err);
	REQUIRE(none.has_value());
	CHECK(none->levels.is_object());
	CHECK(none->levels.empty());
}

TEST_CASE("LevelTableRowInTwoFilesIsAnError") {
	std::string err;
	CHECK_FALSE(LoadModPack(fs::path(PVZU_FIXTURES_DIR) / "pack-levels-dup", &err).has_value());
	CHECK(err.find("51") != std::string::npos);
}

TEST_CASE("RulesMergeFromSeveralFiles") {
	std::string err;
	auto pack = LoadModPack(fs::path(PVZU_FIXTURES_DIR) / "pack-rules-split", &err);
	REQUIRE_MESSAGE(pack.has_value(), err);
	CHECK(pack->rules["a.one"] == 1);
	CHECK(pack->rules["dayLevels.waveBudgetMultiplier"] == 4);
	CHECK(pack->rules["dayPlants.x"] == 2);
}
