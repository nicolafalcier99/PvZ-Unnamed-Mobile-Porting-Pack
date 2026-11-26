#include "doctest/doctest.h"
#include "Mod/GameData.h"
#include "Mod/ModPack.h"

#include <filesystem>
#include <fstream>

using namespace pvzu;
namespace fs = std::filesystem;

namespace {
// A tiny stand-in for the engine's vanilla tables (the engine fills the real ones at start-up).
GameTables MakeVanilla() {
	GameTables t;
	t.plants = {
		{"PEASHOOTER", 100, 750, 150},
		{"POTATOMINE", 25, 3000, 0},
	};
	t.zombies = {
		{"NORMAL", 1, 1, 1, 4000},
		{"PAIL", 4, 8, 1, 3000},
	};
	t.projectiles = {
		{"PEA", 20},
	};
	return t;
}
fs::path Fixture(const char* name) { return fs::path(PVZU_FIXTURES_DIR) / name; }
}

TEST_CASE("VanillaRoundTrip") {
	GameData gd;
	gd.SetVanilla(MakeVanilla());
	ModPack dumped = PackFromTables(gd.Vanilla(), "dump");
	std::string err;
	REQUIRE(gd.ApplyPack(dumped, &err));
	CHECK(err.empty());
	CHECK(gd.Active() == gd.Vanilla());
}

TEST_CASE("PackOverridesOneField") {
	GameData gd;
	gd.SetVanilla(MakeVanilla());
	std::string err;
	auto pack = LoadModPack(Fixture("pack-good"), &err);
	REQUIRE_MESSAGE(pack.has_value(), err);
	CHECK(pack->id == "test-good");
	REQUIRE(gd.ApplyPack(*pack, &err));
	CHECK(gd.Plant("POTATOMINE").seedCost == 50);
	CHECK(gd.Plant("POTATOMINE").refreshTime == 3000);
	CHECK(gd.Plant("PEASHOOTER").seedCost == 100);
	CHECK(gd.Zombie("PAIL").pickWeight == 2500);
	CHECK(gd.Zombie("NORMAL").pickWeight == 4000);
	CHECK(gd.Feature("potatoInstantArm"));
	CHECK_FALSE(gd.Feature("noSuchFeature"));
}

TEST_CASE("BadPackFallsBackToVanilla") {
	for (const char* name : {"pack-bad-json", "pack-bad-name", "pack-bad-type", "pack-does-not-exist"}) {
		CAPTURE(name);
		GameData gd;
		gd.SetVanilla(MakeVanilla());
		std::string err;
		auto pack = LoadModPack(Fixture(name), &err);
		bool applied = pack.has_value() && gd.ApplyPack(*pack, &err);
		CHECK_FALSE(applied);
		CHECK_FALSE(err.empty());
		CHECK(gd.Active() == gd.Vanilla());
		CHECK_FALSE(gd.Feature("potatoInstantArm"));
	}
}

TEST_CASE("ResetRestoresVanilla") {
	GameData gd;
	gd.SetVanilla(MakeVanilla());
	std::string err;
	auto pack = LoadModPack(Fixture("pack-good"), &err);
	REQUIRE(pack.has_value());
	REQUIRE(gd.ApplyPack(*pack, &err));
	gd.ResetToVanilla();
	CHECK(gd.Active() == gd.Vanilla());
	CHECK_FALSE(gd.Feature("potatoInstantArm"));
}

TEST_CASE("DumpActiveTablesWritesLoadablePack") {
	GameData& gd = GameData::Get();
	gd.SetVanilla(MakeVanilla());
	fs::path dir = fs::temp_directory_path() / "pvzu-dump-test";
	fs::remove_all(dir);
	std::string err;
	REQUIRE(DumpActiveTables((dir / "dump.json").string()));
	std::ifstream in(dir / "dump.json");
	nlohmann::json j = nlohmann::json::parse(in);
	CHECK(j["plants"]["POTATOMINE"]["seedCost"] == 25);
	CHECK(j["zombies"]["NORMAL"]["pickWeight"] == 4000);
	CHECK(j["projectiles"]["PEA"]["damage"] == 20);
	fs::remove_all(dir);
}

TEST_CASE("VanillaNamesMustBeUnique") {
	GameData gd;
	GameTables t = MakeVanilla();
	t.zombies.push_back({"NORMAL", 1, 1, 1, 1}); // same name as row 0
	CHECK_FALSE(gd.SetVanilla(t));
	CHECK(gd.SetVanilla(MakeVanilla()));
}
