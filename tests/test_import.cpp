#include "doctest/doctest.h"
#include "Mod/AssetOverlay.h"

#include <filesystem>
#include <fstream>

using namespace pvzu;
namespace fs = std::filesystem;

TEST_CASE("NoOverlayInVanilla") {
	SetOverlayRoot(std::nullopt);
	CHECK_FALSE(OverlayCandidate("images/Zombie.png").has_value());
}

TEST_CASE("OverlayPathsAreNormalised") {
	SetOverlayRoot(fs::path("/root/assets"));
	CHECK(OverlayCandidate("images\\Zombie.png").value() == fs::path("/root/assets/images/Zombie.png"));
	CHECK(OverlayCandidate("./reanim/Zombie.reanim").value() == fs::path("/root/assets/reanim/Zombie.reanim"));
	CHECK(OverlayCandidate("Properties/LawnStrings.txt").value() == fs::path("/root/assets/Properties/LawnStrings.txt"));
	SetOverlayRoot(std::nullopt);
}

TEST_CASE("OverlayRejectsEscapes") {
	SetOverlayRoot(fs::path("/root/assets"));
	CHECK_FALSE(OverlayCandidate("../secret.txt").has_value());
	CHECK_FALSE(OverlayCandidate("images/../../x").has_value());
	CHECK_FALSE(OverlayCandidate("/etc/passwd").has_value());
	CHECK_FALSE(OverlayCandidate("").has_value());
	SetOverlayRoot(std::nullopt);
}

TEST_CASE("OverlayWinsOverPak") {
	fs::path root = fs::temp_directory_path() / "pvzu-overlay-test";
	fs::remove_all(root);
	fs::create_directories(root / "images");
	{ std::ofstream(root / "images" / "Zombie.png") << "x"; }
	SetOverlayRoot(root);
	CHECK(OverlayFileExists("images/Zombie.png"));
	CHECK(OverlayFileExists("IMAGES/zombie.PNG"));  // the game asks with any case
	CHECK_FALSE(OverlayFileExists("images/Missing.png"));
	SetOverlayRoot(std::nullopt);
	CHECK_FALSE(OverlayFileExists("images/Zombie.png"));
	fs::remove_all(root);
}

TEST_CASE("OverlayNeverServesProperties") {
	fs::path root = fs::temp_directory_path() / "pvzu-overlay-props";
	fs::remove_all(root);
	fs::create_directories(root / "properties");
	{ std::ofstream(root / "properties" / "resources.xml") << "x"; }
	{ std::ofstream(root / "properties" / "LawnStrings.txt") << "x"; }
	SetOverlayRoot(root);
	CHECK_FALSE(OverlayFileExists("properties/resources.xml"));   // 1051-era list breaks GOTY resources
	CHECK_FALSE(OverlayFileExists("Properties/LawnStrings.txt")); // Chinese text only via the text option
	SetOverlayRoot(std::nullopt);
	fs::remove_all(root);
}

TEST_CASE("OverlaySkipListHidesTranslatedArt") {
	fs::path root = fs::temp_directory_path() / "pvzu-overlay-skip";
	fs::remove_all(root);
	fs::create_directories(root / "images");
	{ std::ofstream(root / "images" / "SelectorScreen_StartAdventure_Button1.png") << "zh"; }
	{ std::ofstream(root / "images" / "ZombieNoteSmall.png") << "art"; }
	SetOverlayRoot(root, {"images/selectorscreen_startadventure_button1.png"});
	CHECK_FALSE(OverlayFileExists("IMAGES/SelectorScreen_StartAdventure_Button1.png")); // vanilla English wins
	CHECK(OverlayFileExists("images/ZombieNoteSmall.png"));                              // gameplay art still served
	SetOverlayRoot(root);  // Chinese text mode: no skip list
	CHECK(OverlayFileExists("images/SelectorScreen_StartAdventure_Button1.png"));
	SetOverlayRoot(std::nullopt);
	fs::remove_all(root);
}

TEST_CASE("SkipListFileParses") {
	fs::path f = fs::temp_directory_path() / "pvzu-skip.txt";
	{ std::ofstream(f) << "# comment\nimages/A.png\n\n  Reanim/B.png  \r\n"; }
	auto list = LoadOverlaySkipList(f);
	CHECK(list.size() == 2);
	CHECK(list.count("images/a.png") == 1);
	CHECK(list.count("reanim/b.png") == 1);
	CHECK(LoadOverlaySkipList(fs::temp_directory_path() / "missing-skip.txt").empty());
	fs::remove(f);
}

// Unnamed art drawn for its own 1067-wide exe layout (widened menu backs, 13-cell seeds.png) is skipped in every
// language; the English list (translated UI art) only in English.
TEST_CASE("SkipListsMergePerLanguage") {
	fs::path dir = fs::temp_directory_path() / "pvzu-skip-lists";
	fs::remove_all(dir);
	fs::create_directories(dir);
	{ std::ofstream(dir / "overlay-en-skip.txt") << "images/A.png\nimages/B.png\n"; }
	{ std::ofstream(dir / "overlay-skip-always.txt") << "images/seeds.png\nimages/A.png\n"; }
	auto en = LoadOverlaySkipLists(dir, "en");
	CHECK(en.size() == 3); // a.png in both lists counts once
	CHECK(en.count("images/b.png") == 1);
	CHECK(en.count("images/seeds.png") == 1);
	auto zh = LoadOverlaySkipLists(dir, "zh");
	CHECK(zh.size() == 2);
	CHECK(zh.count("images/seeds.png") == 1);
	CHECK(zh.count("images/b.png") == 0);
	fs::remove(dir / "overlay-skip-always.txt");
	CHECK(LoadOverlaySkipLists(dir, "en").size() == 2); // a missing file adds nothing
	CHECK(LoadOverlaySkipLists(dir, "zh").empty());
	fs::remove_all(dir);
}
