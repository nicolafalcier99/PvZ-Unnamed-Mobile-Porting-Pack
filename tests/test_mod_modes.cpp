// PVZU: Unnamed 1.3beta2 mod-only game modes 0x65-0x94 (the research notes (modes-ui-shop) 2.3, pvz-unnamed-pack
// SOURCES-mod-modes.md). The table rows here are the exe's: background = hook 25 (0x79a5fb), allow lists = hook 180
// (0x79ac00, tools/modes_allowed.py); the pack's data/modes.json must hold the same numbers (last test case).
#include "doctest/doctest.h"
#include "Mod/Unnamed/ModModes.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace pvzu::unnamed;

namespace {
nlohmann::json Row(int theBg, std::vector<std::string> theAllowed = {}) {
	nlohmann::json r = {{"name", "x"}, {"nameZh", "x"}, {"background", theBg}, {"status", "built"}};
	if (!theAllowed.empty())
		r["allowed"] = theAllowed;
	return r;
}

// hook 25 (0x79a5fb): Board+0x554c per mode 0x65..0x94.
const std::map<int, int> kBackground = {{0x65, 1}, {0x66, 0}, {0x67, 2}, {0x68, 0}, {0x69, 0}, {0x6a, 2}, {0x6b, 0},
	{0x6c, 8}, {0x6d, 1}, {0x6e, 0}, {0x6f, 3}, {0x70, 4}, {0x71, 6}, {0x72, 1}, {0x73, 0}, {0x74, 2}, {0x75, 0},
	{0x76, 4}, {0x77, 0}, {0x78, 2}, {0x79, 0}, {0x7a, 0}, {0x7b, 3}, {0x7c, 3}, {0x7d, 1}, {0x7e, 0}, {0x7f, 7},
	{0x80, 0}, {0x81, 1}, {0x82, 6}, {0x83, 0}, {0x84, 3}, {0x85, 4}, {0x86, 4}, {0x87, 1}, {0x88, 6}, {0x89, 0},
	{0x8a, 0}, {0x8b, 1}, {0x8c, 1}, {0x8d, 0}, {0x8e, 0}, {0x8f, 1}, {0x90, 0}, {0x91, 1}, {0x92, 0}, {0x93, 0},
	{0x94, 0}};

// Final zombie lists (without Yeti) of every mode, from the hook 180 chain 0x79ac00 -> 0x7a5a00 -> 0xc00900 -> ...
// -> 0x7a5e96 (SOURCES-mod-modes.md "allow lists"), written as ModModeZombies() returns them.
const std::map<int, std::string> kZombies = {
	{0x65, "NORMAL TRAFFIC_CONE POLEVAULTER DANCER DIGGER LADDER"},
	{0x66, "NORMAL TRAFFIC_CONE POLEVAULTER FOOTBALL DANCER JACK_IN_THE_BOX BALLOON GARGANTUAR PAIL"},
	{0x67, "NORMAL TRAFFIC_CONE JACK_IN_THE_BOX POGO LADDER CATAPULT PAIL"},
	{0x68, "NORMAL TRAFFIC_CONE DOOR DANCER LADDER GARGANTUAR PAIL"},
	{0x69, "NORMAL TRAFFIC_CONE POLEVAULTER DANCER TALLNUT_HEAD"},
	{0x6a, "NORMAL TRAFFIC_CONE SNORKEL NEWSPAPER GARGANTUAR PAIL"},
	{0x6b, "NORMAL TRAFFIC_CONE POLEVAULTER FOOTBALL DANCER JACK_IN_THE_BOX DIGGER SQUASH_HEAD TALLNUT_HEAD PAIL"},
	{0x6c, "NORMAL DOOR DOLPHIN_RIDER JACK_IN_THE_BOX POGO GARGANTUAR"},
	{0x6d, "NORMAL TRAFFIC_CONE DOOR PAIL"},
	{0x6e, "NORMAL TRAFFIC_CONE FOOTBALL ZAMBONI POGO CATAPULT PEA_HEAD WALLNUT_HEAD REDEYE_GARGANTUAR"},
	{0x6f, "NORMAL TRAFFIC_CONE POLEVAULTER FOOTBALL SNORKEL ZAMBONI PAIL"},
	{0x70, "NORMAL TRAFFIC_CONE BUNGEE LADDER CATAPULT GARGANTUAR GATLING_HEAD PAIL"},
	{0x71, "NORMAL DOOR ZAMBONI DIGGER LADDER CATAPULT"},
	{0x72, "NORMAL TRAFFIC_CONE DIGGER JALAPENO_HEAD JACK_IN_THE_BOX LADDER GARGANTUAR"},
	{0x73, "NORMAL TRAFFIC_CONE POLEVAULTER NEWSPAPER GARGANTUAR"},
	{0x74, "NORMAL DANCER ZAMBONI POGO LADDER CATAPULT PAIL"},
	{0x75, "NORMAL TRAFFIC_CONE POLEVAULTER FOOTBALL DIGGER BUNGEE SQUASH_HEAD JALAPENO_HEAD PAIL"},
	{0x76, "NORMAL NEWSPAPER DANCER LADDER GARGANTUAR SQUASH_HEAD PAIL"},
	{0x77, "NORMAL FOOTBALL NEWSPAPER JACK_IN_THE_BOX DIGGER CATAPULT PAIL"},
	{0x78, "NORMAL TRAFFIC_CONE POLEVAULTER SNORKEL ZAMBONI DIGGER JALAPENO_HEAD TALLNUT_HEAD PAIL"},
	{0x79, "GARGANTUAR REDEYE_GARGANTUAR"},
	{0x7a, "NORMAL TRAFFIC_CONE DOOR JACK_IN_THE_BOX SQUASH_HEAD"},
	{0x7b, "NORMAL FOOTBALL SNORKEL DOLPHIN_RIDER BALLOON DIGGER LADDER GATLING_HEAD PAIL"},
	{0x7c, "NORMAL POLEVAULTER SNORKEL JACK_IN_THE_BOX POGO CATAPULT GARGANTUAR JALAPENO_HEAD PAIL"},
	{0x7d, "NORMAL TRAFFIC_CONE PAIL"},
	{0x7e, "NORMAL DOOR FOOTBALL JACK_IN_THE_BOX BUNGEE WALLNUT_HEAD TALLNUT_HEAD"},
	{0x7f, "NORMAL POLEVAULTER BALLOON GARGANTUAR PAIL"},
	{0x80, "NORMAL TRAFFIC_CONE DOOR DANCER DIGGER POGO CATAPULT PAIL"},
	{0x81, "NORMAL TRAFFIC_CONE POLEVAULTER DOOR FOOTBALL ZAMBONI BUNGEE LADDER GARGANTUAR REDEYE_GARGANTUAR WALLNUT_HEAD "
	       "TALLNUT_HEAD PAIL"},
	{0x82, "NORMAL TRAFFIC_CONE NEWSPAPER ZAMBONI LADDER GARGANTUAR PEA_HEAD GATLING_HEAD REDEYE_GARGANTUAR PAIL"},
	{0x83, "NORMAL TRAFFIC_CONE PAIL"},
	{0x84, "NORMAL TRAFFIC_CONE FOOTBALL DANCER PEA_HEAD GATLING_HEAD GARGANTUAR PAIL"},
	{0x85, "TRAFFIC_CONE DIGGER"},
	{0x86, "NORMAL POLEVAULTER FOOTBALL DANCER POGO LADDER GARGANTUAR"},
	{0x87, "NORMAL PEA_HEAD NEWSPAPER BALLOON CATAPULT GARGANTUAR"},
	{0x88, "NORMAL TRAFFIC_CONE POLEVAULTER POGO CATAPULT LADDER GARGANTUAR JALAPENO_HEAD PAIL"},
	{0x89, "NORMAL NEWSPAPER FOOTBALL DANCER GARGANTUAR SQUASH_HEAD TALLNUT_HEAD REDEYE_GARGANTUAR PAIL"},
	{0x8a, "NORMAL POLEVAULTER BUNGEE PEA_HEAD CATAPULT TALLNUT_HEAD"},
	{0x8b, "NORMAL TRAFFIC_CONE PAIL"},
	{0x8c, "NORMAL TRAFFIC_CONE PAIL"},
	{0x8d, "NORMAL TRAFFIC_CONE PAIL NEWSPAPER DOOR FOOTBALL JACK_IN_THE_BOX CATAPULT GARGANTUAR PEA_HEAD SQUASH_HEAD "
	       "TALLNUT_HEAD PAIL"},
	{0x8e, "NORMAL TRAFFIC_CONE DOOR FOOTBALL LADDER WALLNUT_HEAD SQUASH_HEAD TALLNUT_HEAD PAIL"},
	{0x8f, "NORMAL TRAFFIC_CONE PAIL"},
	{0x90, "NORMAL POLEVAULTER PAIL NEWSPAPER LADDER GARGANTUAR PAIL"},
	{0x91, "NORMAL PAIL FOOTBALL ZAMBONI JACK_IN_THE_BOX GARGANTUAR TALLNUT_HEAD REDEYE_GARGANTUAR PAIL"},
	{0x92, "NORMAL DANCER PAIL"},
	{0x93, "NORMAL TRAFFIC_CONE PAIL"},
	{0x94, "NORMAL TRAFFIC_CONE PAIL"},
};

// hook 20 (0x79a201): wave count per mode.
const std::map<int, int> kWaves = {{0x8b, 0}, {0x7d, 10}, {0x65, 20}, {0x67, 20}, {0x68, 20}, {0x6e, 20}, {0x70, 20},
	{0x77, 20}, {0x84, 20}, {0x92, 25}, {0x91, 60}, {0x6d, 40}, {0x72, 40}, {0x78, 40}, {0x79, 40}, {0x7f, 40}, {0x81, 40},
	{0x82, 40}, {0x83, 40}, {0x85, 40}, {0x8c, 40}, {0x8d, 40}, {0x8f, 40}, {0x90, 40}, {0x93, 40}, {0x94, 40}};
// hooks 30/31: start sun (others 50 = vanilla); hooks 141/142: seed slots; hooks 35/36: no chooser; hook 140: conveyor
const std::map<int, int> kSun = {{0x73, 1000}, {0x7f, 200}, {0x80, 2000}, {0x83, 500000}, {0x8a, 200}, {0x8c, 6750}, {0x92, 500}};
const std::map<int, int> kSlots = {{0x72, 1}, {0x73, 1}, {0x91, 1}, {0x7d, 1}, {0x8f, 1}, {0x89, 3}, {0x87, 7}, {0x76, 9}};
const std::vector<int> kNoChooser = {0x76, 0x79, 0x8b, 0x8d, 0x8e};
const std::vector<int> kConveyor = {0x71, 0x81, 0x84};

std::vector<std::string> Split(const std::string& s) {
	std::vector<std::string> out;
	size_t i = 0;
	while (i < s.size()) {
		const size_t j = s.find(' ', i);
		out.push_back(s.substr(i, j == std::string::npos ? std::string::npos : j - i));
		if (j == std::string::npos)
			break;
		i = j + 1;
	}
	return out;
}
bool In(const std::vector<int>& v, int x) { return std::find(v.begin(), v.end(), x) != v.end(); }

fs::path PackDir() {
	return std::getenv("PVZU_PACK_DIR") ? fs::path(std::getenv("PVZU_PACK_DIR"))
	                                    : fs::path(std::getenv("HOME")) / "pvz-unnamed-pack";
}
} // namespace

TEST_CASE("mod modes: the range is 0x65..0x94 (48 modes), records sit after the 100 vanilla ones") {
	CHECK(kFirstModMode == 0x65);
	CHECK(kLastModMode == 0x94);
	CHECK(kNumModModes == 48);
	CHECK_FALSE(IsModModeValue(0x48));
	CHECK_FALSE(IsModModeValue(0x64));
	CHECK(IsModModeValue(0x65));
	CHECK(IsModModeValue(0x94));
	CHECK_FALSE(IsModModeValue(0x95));
	// LawnApp::GetCurrentChallengeIndex = mode - 1: 0x65 -> 100, 0x94 -> 147 (the 1051 exe's 100-entry array overflows).
	CHECK(ModModeRecordIndex(0x65) == 100);
	CHECK(ModModeRecordIndex(0x94) == 147);
	CHECK(kNumRecordsWithModModes == 148);
}

TEST_CASE("mod modes: HasBeatenChallenge reads the mod record store, not the vanilla challenge table") {
	// LawnApp::HasBeatenChallenge asserted index < NUM_CHALLENGE_MODES before reading; mode 0x65 is index 100.
	uint32_t aRecords[kNumRecordsWithModModes] = {};
	bool aBeaten = true;
	CHECK_FALSE(ModModeBeaten(0x12, aRecords, aBeaten)); // a vanilla mode: not ours, vanilla path
	CHECK(ModModeBeaten(0x65, aRecords, aBeaten));
	CHECK_FALSE(aBeaten);
	aRecords[100] = 1;
	CHECK(ModModeBeaten(0x65, aRecords, aBeaten));
	CHECK(aBeaten);
	aRecords[147] = 3;
	CHECK(ModModeBeaten(0x94, aRecords, aBeaten));
	CHECK(aBeaten);
}

TEST_CASE("mod modes: a deleted profile's mod files (records json, game saves of modes 101..148)") {
	const std::vector<std::string> aNames = ModModeUserFileNames(3);
	CHECK(aNames.size() == 1 + 2 * kNumModModes);
	auto Has = [&](const std::string& theName) { return std::find(aNames.begin(), aNames.end(), theName) != aNames.end(); };
	CHECK(Has("user3-modes.json"));
	CHECK(Has("game3_101.v4"));
	CHECK(Has("game3_101.dat"));
	CHECK(Has("game3_148.v4"));
	CHECK(Has("game3_148.dat"));
	CHECK_FALSE(Has("game3_100.v4")); // vanilla modes stay with PlayerInfo::DeleteUserFiles' own loop
	CHECK_FALSE(Has("game3_149.v4"));
}

TEST_CASE("mod modes: table rows parse, bad rows are refused and keep no rows") {
	ClearModModes();
	CHECK(FindModMode(0x67) == nullptr);

	nlohmann::json t = {{"0x67", Row(2, {"TRAFFIC_CONE", "POGO"})}, {"0x68", Row(0)}};
	t["0x68"]["firstWaveCountdown"] = 5500;
	t["0x68"]["status"] = "needsModPlants";
	t["0x67"]["seedSlots"] = 1;
	t["0x67"]["bannedSeeds"] = {"LILYPAD", "TANGLEKELP"};
	std::string err;
	REQUIRE(SetModModes(t, &err));
	CHECK(err.empty());
	const ModModeDef* a = FindModMode(0x67);
	REQUIRE(a != nullptr);
	CHECK(a->mode == 0x67);
	CHECK(a->background == 2);
	CHECK(a->allowed == std::vector<std::string>{"TRAFFIC_CONE", "POGO"});
	CHECK(a->seedSlots == 1);
	CHECK(a->bannedSeeds == std::vector<std::string>{"LILYPAD", "TANGLEKELP"});
	CHECK(a->status == ModModeStatus::Built);
	CHECK(a->Playable());
	const ModModeDef* b = FindModMode(0x68);
	REQUIRE(b != nullptr);
	CHECK(b->firstWaveCountdown == 5500);
	CHECK(b->seedSlots == kNoModRule);
	CHECK(b->status == ModModeStatus::NeedsModPlants);
	CHECK_FALSE(b->Playable());

	// the menu lists every row in mode order, built ones playable
	const std::vector<int> aList = ModModeMenuOrder();
	CHECK(aList == std::vector<int>{0x67, 0x68});

	nlohmann::json bad = {{"0x67", Row(2)}, {"0x30", Row(0)}};
	CHECK_FALSE(SetModModes(bad, &err));
	CHECK(err.find("0x30") != std::string::npos);
	CHECK(FindModMode(0x67) == nullptr);

	nlohmann::json badBg = {{"0x67", Row(42)}};
	CHECK_FALSE(SetModModes(badBg, &err));
	nlohmann::json badStatus = {{"0x67", Row(0)}};
	badStatus["0x67"]["status"] = "maybe";
	CHECK_FALSE(SetModModes(badStatus, &err));
	ClearModModes();
}

TEST_CASE("mod modes: zombie list = Normal + own list (Cone when none) + Pail unless the 0x79af0b exit") {
	// Vanilla Challenge::InitZombieWaves' last branch writes Normal, then the mod's jump at .text 0x425cad replaces the
	// Cone write. A mode block writes its own types; a mode with no block falls through to 0x7a5e96 (Cone). Exits
	// 0x79af0d keep the vanilla Pail write, 0x79af0b (xor ebx,ebx) writes Pail false; 0x79/0x85 write Normal false.
	ModModeDef d;
	CHECK(ModModeZombies(d) == std::vector<std::string>{"NORMAL", "TRAFFIC_CONE", "PAIL"});
	d.allowed = {"DOOR", "FOOTBALL"};
	CHECK(ModModeZombies(d) == std::vector<std::string>{"NORMAL", "DOOR", "FOOTBALL", "PAIL"});
	d.pail = false;
	CHECK(ModModeZombies(d) == std::vector<std::string>{"NORMAL", "DOOR", "FOOTBALL"});
	d.normal = false;
	d.allowed = {"TRAFFIC_CONE", "DIGGER"};
	CHECK(ModModeZombies(d) == std::vector<std::string>{"TRAFFIC_CONE", "DIGGER"});
}

TEST_CASE("mod modes: level-name colours (hook 130)") {
	CHECK(ModModeNameColor("gold", 0) == Rgb{224, 187, 98});
	CHECK(ModModeNameColor("red", 0) == Rgb{194, 19, 37});
	CHECK(ModModeNameColor("purple", 0) == Rgb{92, 0, 170});
	CHECK(ModModeNameColor("blue", 0) == Rgb{20, 126, 220});
	CHECK(ModModeNameColor("green", 0) == Rgb{10, 204, 20});
	// pulse: v = mMainCounter & 0xff; v <= 127 -> (61, v, 61), else (v/2+36, 61, 61)  (0x7a3606)
	CHECK(ModModeNameColor("pulse", 0x100 + 40) == Rgb{61, 40, 61});
	CHECK(ModModeNameColor("pulse", 127) == Rgb{61, 127, 61});
	CHECK(ModModeNameColor("pulse", 200) == Rgb{136, 61, 61});
}

TEST_CASE("mod modes: list screen layout scrolls and hit-tests") {
	ModeListLayout l; // 5 columns like the challenge page, 155 px apart, rows 119 px, view 93..533
	CHECK(l.Rows(48) == 10);
	CHECK(l.MaxScroll(48) == std::max(0, 10 * l.rowH - l.viewH));
	CHECK(l.ClampScroll(-30, 48) == 0);
	CHECK(l.ClampScroll(100000, 48) == l.MaxScroll(48));
	CHECK(l.MaxScroll(5) == 0);
	int x = 0, y = 0;
	l.CellPos(0, 0, x, y);
	CHECK(x == l.left);
	CHECK(y == l.top);
	l.CellPos(6, 0, x, y); // second row, second column
	CHECK(x == l.left + l.colW);
	CHECK(y == l.top + l.rowH);
	l.CellPos(6, 50, x, y);
	CHECK(y == l.top + l.rowH - 50);
	CHECK(l.HitTest(l.left + 10, l.top + 10, 0, 48) == 0);
	CHECK(l.HitTest(l.left + l.colW + 10, l.top + l.rowH + 10, 0, 48) == 6);
	CHECK(l.HitTest(l.left + l.colW + 10, l.top + 10, l.rowH, 48) == 6);
	CHECK(l.HitTest(l.left + l.cellW + 5, l.top + 10, 0, 48) == -1); // the gap between cells
	CHECK(l.HitTest(l.left + 10, l.top - 5, 0, 48) == -1);           // above the view
	CHECK(l.HitTest(l.left + 10, l.top + l.viewH + 1, 0, 48) == -1); // below the view
	CHECK(l.HitTest(l.left + 10, l.top + 10, 0, 0) == -1);
	// a drag of more than dragSlop pixels is a scroll, not a tap
	CHECK_FALSE(l.IsDrag(0, l.dragSlop));
	CHECK(l.IsDrag(0, l.dragSlop + 1));
}

TEST_CASE("mod modes: the pack's data/modes.json matches the exe tables") {
	const fs::path file = PackDir() / "data" / "modes.json";
	REQUIRE(fs::exists(file));
	std::ifstream in(file);
	const nlohmann::json j = nlohmann::json::parse(in);
	std::string err;
	REQUIRE_MESSAGE(SetModModes(j, &err), err);
	CHECK(j.size() == 48);
	for (int aMode = kFirstModMode; aMode <= kLastModMode; aMode++) {
		CAPTURE(aMode);
		const ModModeDef* d = FindModMode(aMode);
		REQUIRE(d != nullptr);
		CHECK(d->background == kBackground.at(aMode));
		CHECK_FALSE(d->name.empty());
		CHECK_FALSE(d->nameZh.empty());
		CHECK(ModModeZombies(*d) == Split(kZombies.at(aMode)));
		CHECK(d->waves == (kWaves.count(aMode) ? kWaves.at(aMode) : 30));
		CHECK(d->startSun == (kSun.count(aMode) ? kSun.at(aMode) : kNoModRule));
		CHECK(d->seedSlots == (kSlots.count(aMode) ? kSlots.at(aMode) : kNoModRule));
		CHECK(d->chooser == !In(kNoChooser, aMode));
		CHECK(d->conveyor == In(kConveyor, aMode));
		CHECK(d->graves == ((aMode == 0x6d || aMode == 0x91) ? 0 : kNoModRule));
		if (d->status == ModModeStatus::Built)
			CHECK_FALSE(d->key.empty());
	}
	ClearModModes();
}
