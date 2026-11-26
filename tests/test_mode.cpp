#include "doctest/doctest.h"
#include "Mod/Mode.h"

#include <filesystem>
#include <fstream>

using namespace pvzu;
namespace fs = std::filesystem;

TEST_CASE("ModeSavesAreSeparate") {
	CHECK(std::string(UserDataDirName(GameMode::Vanilla)) == "userdata");  // upstream path, old saves keep working
	CHECK(std::string(UserDataDirName(GameMode::Unnamed)) == "userdata-unnamed");
	CHECK(UserDataPath(GameMode::Vanilla, "user1.dat") == "userdata/user1.dat");
	CHECK(UserDataPath(GameMode::Unnamed, "user1.dat") == "userdata-unnamed/user1.dat");
	CHECK(UserDataPath(GameMode::Unnamed, "") == "userdata-unnamed");
}

TEST_CASE("ModePersists") {
	fs::path dir = fs::temp_directory_path() / "pvzu-mode-test";
	fs::remove_all(dir);
	fs::create_directories(dir);
	Settings s;
	s.mode = GameMode::Unnamed;
	s.language = "zh";
	REQUIRE(SaveSettings(dir / "pvzu-settings.json", s));
	Settings back = LoadSettings(dir / "pvzu-settings.json");
	CHECK(back.mode == GameMode::Unnamed);
	CHECK(back.language == "zh");
	fs::remove_all(dir);
}

TEST_CASE("MissingOrBrokenSettingsAreVanilla") {
	fs::path dir = fs::temp_directory_path() / "pvzu-mode-test2";
	fs::remove_all(dir);
	fs::create_directories(dir);
	CHECK(LoadSettings(dir / "nope.json").mode == GameMode::Vanilla);
	{ std::ofstream(dir / "bad.json") << "{ not json"; }
	Settings bad = LoadSettings(dir / "bad.json");
	CHECK(bad.mode == GameMode::Vanilla);
	CHECK(bad.language == "en");
	{ std::ofstream(dir / "odd.json") << R"({"mode":"martian","language":7})"; }
	CHECK(LoadSettings(dir / "odd.json").mode == GameMode::Vanilla);
	fs::remove_all(dir);
}

TEST_CASE("SwitchWithoutPackFails") {
	CHECK(EffectiveMode(GameMode::Unnamed, /*packLoaded=*/false) == GameMode::Vanilla);
	CHECK(EffectiveMode(GameMode::Unnamed, /*packLoaded=*/true) == GameMode::Unnamed);
	CHECK(EffectiveMode(GameMode::Vanilla, /*packLoaded=*/true) == GameMode::Vanilla);
}

TEST_CASE("SpeedSettingPersists") {
	// 2026-11-23: a 1x/2x speed button on the play screen; the choice is kept in pvzu-settings.json.
	fs::path dir = fs::temp_directory_path() / "pvzu-mode-test3";
	fs::remove_all(dir);
	fs::create_directories(dir);
	CHECK(Settings{}.speed == 1);
	Settings s;
	s.mode = GameMode::Unnamed;
	s.speed = 2;
	REQUIRE(SaveSettings(dir / "pvzu-settings.json", s));
	Settings back = LoadSettings(dir / "pvzu-settings.json");
	CHECK(back.speed == 2);
	CHECK(back.mode == GameMode::Unnamed); // the other settings survive
	s.speed = 1;
	REQUIRE(SaveSettings(dir / "pvzu-settings.json", s));
	CHECK(LoadSettings(dir / "pvzu-settings.json").speed == 1);
	fs::remove_all(dir);
}

TEST_CASE("SpeedSettingOnlyOneOrTwo") {
	fs::path dir = fs::temp_directory_path() / "pvzu-mode-test4";
	fs::remove_all(dir);
	fs::create_directories(dir);
	{ std::ofstream(dir / "old.json") << R"({"mode":"unnamed","language":"en"})"; } // files from before the setting
	CHECK(LoadSettings(dir / "old.json").speed == 1);
	{ std::ofstream(dir / "two.json") << R"({"speed":2})"; }
	CHECK(LoadSettings(dir / "two.json").speed == 2);
	for (const char* aBad : {R"({"speed":3})", R"({"speed":0})", R"({"speed":-2})", R"({"speed":"2"})", R"({"speed":1.5})"}) {
		{ std::ofstream(dir / "bad.json") << aBad; }
		CAPTURE(aBad);
		CHECK(LoadSettings(dir / "bad.json").speed == 1);
	}
	fs::remove_all(dir);
}

TEST_CASE("SpeedButtonCycles") {
	CHECK(NextSpeed(1) == 2);
	CHECK(NextSpeed(2) == 1);
	CHECK(NextSpeed(7) == 1); // anything odd goes back to normal
	CHECK(SpeedLabel(1) == "1x");
	CHECK(SpeedLabel(2) == "2x");
}
