// PVZU: Almanac pages, tooltips and lawn hints for Unnamed's 26 mod plants (feature "modAlmanac", port-only).
// Pure parts of Mod/Unnamed/ModAlmanac.h; the last cases read the pack (PVZU_PACK_DIR or pvz-unnamed-pack).
#include "doctest/doctest.h"
#include "Mod/Unnamed/ModAlmanac.h"
#include "nlohmann/json.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace pvzu::unnamed;

namespace {
fs::path PackDir() {
	const char* home = std::getenv("HOME");
	return std::getenv("PVZU_PACK_DIR") ? fs::path(std::getenv("PVZU_PACK_DIR"))
	                                     : fs::path(home ? home : "") / "pvz-unnamed-pack";
}
std::string ReadFile(const fs::path& p) {
	std::ifstream in(p, std::ios::binary);
	std::stringstream s;
	s << in.rdbuf();
	return s.str();
}
std::vector<int> PackModSeeds() {
	const nlohmann::json j = nlohmann::json::parse(ReadFile(PackDir() / "data" / "mod-plants.json"));
	std::vector<int> seeds;
	for (auto it = j.begin(); it != j.end(); ++it)
		seeds.push_back(std::stoi(it.key()));
	std::sort(seeds.begin(), seeds.end());
	return seeds;
}
} // namespace

TEST_CASE("ModAlmanacTextParses") {
	ModAlmanacTexts t;
	std::string err;
	REQUIRE(ParseModAlmanacText("# comment\n\n[MOD_PLANT_0x50_DESCRIPTION]\nLine one\nLine two\n\n# a section comment\n[MOD_HINT_TAP_0x7f]\nTap!\n", &t, &err));
	CHECK(t.at("MOD_PLANT_0x50_DESCRIPTION") == "Line one\nLine two");
	CHECK(t.at("MOD_HINT_TAP_0x7f") == "Tap!");
	CHECK(t.size() == 2);
	// CRLF files from Windows editors
	REQUIRE(ParseModAlmanacText("[A_B]\r\nx\r\n", &t, &err));
	CHECK(t.at("A_B") == "x");
	CHECK_FALSE(ParseModAlmanacText("text before a key\n[A]\nx\n", &t, &err));
	CHECK_FALSE(err.empty());
	CHECK_FALSE(ParseModAlmanacText("[A\nx\n", &t, &err));
}

TEST_CASE("ModAlmanacKeys") {
	CHECK(ModPlantTextKey(0x50, "DESCRIPTION") == "MOD_PLANT_0x50_DESCRIPTION");
	CHECK(ModPlantTextKey(0x7f, "HOW_TO_USE") == "MOD_PLANT_0x7f_HOW_TO_USE");
	CHECK(ModHintKey("TAP", 0x5d) == "MOD_HINT_TAP_0x5d");
}

TEST_CASE("ModAlmanacRechargeSeconds") {
	CHECK(ModAlmanacRechargeSeconds(1500) == "15");
	CHECK(ModAlmanacRechargeSeconds(750) == "7.5");
	CHECK(ModAlmanacRechargeSeconds(4250) == "42.5");
	CHECK(ModAlmanacRechargeSeconds(12000) == "120");
	CHECK(ModAlmanacRechargeSeconds(0) == "0");
}

TEST_CASE("ModAlmanacLayoutFitsTheDialogAndPhone") {
	const AlmanacRect aArea = ModAlmanacGridArea();
	const AlmanacRect aDialog{0, 0, 800, 600};
	CHECK(kModAlmanacColumns * kModAlmanacRows >= 26);
	for (int i = 0; i < kModAlmanacColumns * kModAlmanacRows; ++i) {
		const AlmanacRect c = ModAlmanacCell(i);
		INFO("cell " << i);
		CHECK(c.Inside(aArea));
		CHECK(ModAlmanacPacket(i).Inside(c));
		CHECK_FALSE(c.Overlaps(ModAlmanacCard()));
		CHECK_FALSE(c.Overlaps(ModAlmanacButtonHit()));
		CHECK_FALSE(c.Overlaps(ModAlmanacIndexButton()));
		CHECK_FALSE(c.Overlaps(ModAlmanacCloseButton()));
		// big touch targets: larger than the vanilla 52 x 78 cell, at least 64 x 84 logical (115 x 151 px on the S26)
		CHECK(c.w >= 64);
		CHECK(c.h >= 84);
		for (int j = i + 1; j < kModAlmanacColumns * kModAlmanacRows; ++j)
			CHECK_FALSE(c.Overlaps(ModAlmanacCell(j)));
		CHECK(ModAlmanacCellAt(c.x + c.w / 2, c.y + c.h / 2, 30) == i);
	}
	CHECK(ModAlmanacPacketScale() > 1.0f);
	CHECK(ModAlmanacCellAt(ModAlmanacCell(25).x + 5, ModAlmanacCell(25).y + 5, 26) == 25);
	CHECK(ModAlmanacCellAt(ModAlmanacCell(26).x + 5, ModAlmanacCell(26).y + 5, 26) == -1); // empty cell
	CHECK(ModAlmanacCellAt(600, 300, 26) == -1);
	CHECK(ModAlmanacButton().Inside(ModAlmanacButtonHit()));
	CHECK(ModAlmanacButtonHit().Inside(aDialog));
	CHECK(ModAlmanacButtonHit().h >= 40);
	CHECK_FALSE(ModAlmanacButtonHit().Overlaps(ModAlmanacIndexButton()));
	CHECK_FALSE(ModAlmanacButtonHit().Overlaps(ModAlmanacCloseButton()));
	// the dialog is centred: 1300 wide (the phone, left safe inset 57) and the vanilla 800
	for (int aWidth : {1300, 800}) {
		const int aLeft = (aWidth - 800) / 2;
		const int aInset = aWidth == 1300 ? 57 : 0;
		INFO("width " << aWidth);
		CHECK(aLeft + aArea.x >= aInset);
		CHECK(aLeft + ModAlmanacButtonHit().x >= aInset);
		CHECK(aLeft + ModAlmanacCloseButton().x + ModAlmanacCloseButton().w <= aWidth);
	}
}

TEST_CASE("ModAlmanacPlantedHintChoice") {
	// a shadow plant planted without Moonflowers: its shadow text
	CHECK(ChoosePlantedHint(true, 0, 1, false) == PlantedHint::Shadow);
	CHECK(ChoosePlantedHint(true, 2, 3, true) == PlantedHint::Shadow);
	// enough points: the planted text, or nothing
	CHECK(ChoosePlantedHint(true, 1, 1, true) == PlantedHint::Planted);
	CHECK(ChoosePlantedHint(true, 3, 3, false) == PlantedHint::None);
	// not a shadow plant (threshold 0)
	CHECK(ChoosePlantedHint(false, 0, 0, true) == PlantedHint::Planted);
	CHECK(ChoosePlantedHint(true, 0, 0, true) == PlantedHint::Planted);
	CHECK(ChoosePlantedHint(false, 0, 0, false) == PlantedHint::None);
}

TEST_CASE("ModAlmanacTapAndRefusalHints") {
	CHECK(CannonTapNeedsHint(0x7f, 0x23));         // waiting for a load (ARMING)
	CHECK_FALSE(CannonTapNeedsHint(0x7f, 0x25));   // ready: the tap aims
	CHECK_FALSE(CannonTapNeedsHint(0x7f, 0x24));   // loading
	CHECK_FALSE(CannonTapNeedsHint(0, 0x23));      // a vanilla Cob Cannon re-arms by itself
	CHECK(PrismTapNeedsHint(0x5d, true, false));
	CHECK_FALSE(PrismTapNeedsHint(0x5d, true, true));
	CHECK_FALSE(PrismTapNeedsHint(0x5d, false, false)); // asleep / busy: the tap does nothing anyway
	CHECK_FALSE(PrismTapNeedsHint(0x5c, true, false));
	CHECK(RefusalHasOwnHint(0x5b, true));
	CHECK(RefusalHasOwnHint(0x73, true));
	CHECK_FALSE(RefusalHasOwnHint(0x5b, false));
	CHECK_FALSE(RefusalHasOwnHint(35, true)); // the vanilla Coffee Bean keeps its message
	CHECK(HintMayShow(false, false));
	CHECK(HintMayShow(true, true));
	CHECK_FALSE(HintMayShow(true, false));
}

TEST_CASE("ModAlmanacHintLatchOncePerLevelPerPlant") {
	HintLatch l;
	CHECK(l.Take(1, 0x5d));
	CHECK_FALSE(l.Take(1, 0x5d));
	CHECK(l.Take(1, 0x7a));
	CHECK(l.Take(2, 0x5d)); // next level
	CHECK(l.Take(2, 0x7a)); // 0x7a not shown on level 2 yet
	CHECK_FALSE(l.Take(2, 0x7a));
}

// --- the pack -----------------------------------------------------------------------------------------------------
TEST_CASE("ModAlmanacPackTextCompleteForEveryModPlant") {
	const fs::path aFile = PackDir() / "strings" / "mod-almanac-en.txt";
	REQUIRE_MESSAGE(fs::exists(aFile), aFile.string());
	ModAlmanacTexts t;
	std::string err;
	REQUIRE_MESSAGE(ParseModAlmanacText(ReadFile(aFile), &t, &err), err);
	const std::vector<int> aSeeds = PackModSeeds();
	REQUIRE(aSeeds.size() == 26);
	const std::regex aHex("0x[0-9a-fA-F]");
	for (int aSeed : aSeeds) {
		for (const char* aWhat : {"DESCRIPTION", "HOW_TO_USE"}) {
			const std::string aKey = ModPlantTextKey(aSeed, aWhat);
			INFO(aKey);
			REQUIRE(t.count(aKey) == 1);
			CHECK_FALSE(t.at(aKey).empty());
			CHECK_FALSE(std::regex_search(t.at(aKey), aHex)); // plain words, no exe addresses
		}
		CHECK(t.at(ModPlantTextKey(aSeed, "HOW_TO_USE")).find('\n') == std::string::npos); // one line
		CHECK(t.at(ModPlantTextKey(aSeed, "DESCRIPTION")).size() <= 420); // fits the card (checked on screenshots)
	}
	for (const auto& [aKey, aText] : t) {
		INFO(aKey);
		if (aKey.starts_with("MOD_HINT_")) {
			CHECK(aText.find('\n') == std::string::npos); // one short line
			CHECK(aText.size() <= 56); // fits the message bar on the 800-wide screen too
			CHECK_FALSE(std::regex_search(aText, aHex));
		}
	}
	// the two hints asked for by name
	CHECK(t.at(ModHintKey("TAP", 0x7f)) == "Drop a Kernel-pult or Doom-shroom card on it to load it");
	CHECK(t.at(ModHintKey("REFUSED", 0x5b)) == "Plant the Magic Devourer on top of one of your plants");
	for (const char* aKey : {"MOD_ALMANAC_MORE_PLANTS", "MOD_ALMANAC_BACK", "MOD_ALMANAC_HOW_TO_USE", "MOD_ALMANAC_SECONDS"})
		CHECK_MESSAGE(t.count(aKey) == 1, aKey);
}

TEST_CASE("ModAlmanacPackTooltipsOneLineWithHowToUse") {
	const nlohmann::json j = nlohmann::json::parse(ReadFile(PackDir() / "data" / "mod-plants.json"));
	for (auto it = j.begin(); it != j.end(); ++it) {
		const std::string aTip = it.value()["tooltip"]["en"].get<std::string>();
		INFO(it.key() << ": " << aTip);
		CHECK_FALSE(aTip.empty()); // the Loaded Cannon's was empty
		CHECK(aTip.find('\n') == std::string::npos);
		CHECK(aTip.size() <= 110);
	}
}

TEST_CASE("ModAlmanacPackFeatureAndRules") {
	const nlohmann::json m = nlohmann::json::parse(ReadFile(PackDir() / "mod.json"));
	CHECK(m["features"].value("modAlmanac", false));
	const nlohmann::json r = nlohmann::json::parse(ReadFile(PackDir() / "data" / "rules-mod-almanac.json"));
	CHECK(r["modAlmanac.hints"] == 1);
	CHECK(r["modAlmanac.pages"] == 1);
}
