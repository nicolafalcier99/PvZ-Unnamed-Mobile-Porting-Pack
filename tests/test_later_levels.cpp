// PVZU: Unnamed Adventure levels 51-73 (worlds 6 and 7 and the three hidden levels) - infrastructure rules
// (the research notes (w6-w7-and-new-content) sections 1.1, 1.2, 2.1, 2.2, 6, 7.1, 7.3, 7.7). The per-level table is the
// study's table 2.2; the pack's data/levels-51-73.json and data/rules-later-levels.json must hold the same numbers
// (the last tests compare them). Special scripts of 55, 65, 70 and 71-73 are Plan 7 Task 3: only their ids are here.
#include "doctest/doctest.h"
#include "Mod/Unnamed/LaterLevels.h"
#include "Mod/Unnamed/Rules.h"

#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace pvzu::unnamed;

namespace {
// Engine BackgroundType values (ConstEnums.h), the same codes as the 1051 exe's Board+0x554c.
constexpr int kDay = 0, kNight = 1, kPool = 2, kFog = 3, kRoof = 4, kNightRoof = 5, kMushroom = 6, kGreenhouse = 7,
	kAquarium = 8;
// Engine PlantRowType values.
constexpr int kDirt = 0, kNormal = 1, kWater = 2;

// The pack's zombie keys = engine ZombieType names without "ZOMBIE_" (Mod/Engine/EngineTables.cpp), index = type.
const std::vector<std::string> kZombieNames = {"NORMAL", "FLAG", "TRAFFIC_CONE", "POLEVAULTER", "PAIL", "NEWSPAPER",
	"DOOR", "FOOTBALL", "DANCER", "BACKUP_DANCER", "DUCKY_TUBE", "SNORKEL", "ZAMBONI", "BOBSLED", "DOLPHIN_RIDER",
	"JACK_IN_THE_BOX", "BALLOON", "DIGGER", "POGO", "YETI", "BUNGEE", "LADDER", "CATAPULT", "GARGANTUAR", "IMP", "BOSS",
	"PEA_HEAD", "WALLNUT_HEAD", "JALAPENO_HEAD", "GATLING_HEAD", "SQUASH_HEAD", "TALLNUT_HEAD", "REDEYE_GARGANTUAR"};

struct Row {
	int level;
	const char* stage;
	int bg;          // hook 24 -> 0x7a2501 (71-73: the vanilla PickBackground, level > 50 = day)
	int sun;         // hook 30 chain; -1 = vanilla (73: no branch found, vanilla 50)
	int waves;       // hook 19 -> 0x798100 / 0x7a2601 / 0x95c030; 72, 73: vanilla table clamped to index 49 = 30
	std::vector<std::string> allowed; // hook 176 chain (tools/w67_allowed.py)
	int pots;        // hook 222 -> 0x798250; -1 = vanilla (roof: 3 columns)
	int countdown;   // hook 29 -> 0xc134e0; -1 = vanilla 1800
	int fogColumn;   // vanilla 1051 Board::LeftFogColumn 0x41C1CC returns mLevel above 40; -1 = not a fog stage
	int introTimer;  // hook 220 -> 0x799090 (cut-scene timer of 6-10); -1 = none
	SpecialLevel special;
};

const std::vector<Row> kTable = {
	{51, "6-1", kNightRoof, 150, 20, {"NORMAL", "TRAFFIC_CONE", "DOOR", "FOOTBALL", "POGO"}, 4, -1, -1, -1, SpecialLevel::None},
	{52, "6-2", kNightRoof, 150, 20, {"NORMAL", "TRAFFIC_CONE", "POLEVAULTER", "DANCER", "BALLOON"}, 4, -1, -1, -1, SpecialLevel::None},
	{53, "6-3", kNightRoof, 150, 30, {"NORMAL", "TRAFFIC_CONE", "PAIL", "NEWSPAPER", "DIGGER", "WALLNUT_HEAD"}, 4, -1, -1, -1,
		SpecialLevel::None},
	{54, "6-4", kNightRoof, 150, 20, {"NORMAL", "POLEVAULTER", "LADDER", "ZAMBONI", "JACK_IN_THE_BOX", "BUNGEE"}, 4, -1, -1, -1,
		SpecialLevel::None},
	{55, "6-5", kMushroom, 150, 20, {"NORMAL", "TRAFFIC_CONE", "JALAPENO_HEAD", "FOOTBALL", "DANCER"}, 0, -1, -1, -1,
		SpecialLevel::Shooter},
	{56, "6-6", kNightRoof, 150, 30, {"PEA_HEAD", "FOOTBALL", "SQUASH_HEAD", "CATAPULT", "GARGANTUAR"}, 5, -1, -1, -1,
		SpecialLevel::None},
	{57, "6-7", kNightRoof, 150, 30, {"NORMAL", "FOOTBALL", "DANCER", "GATLING_HEAD", "LADDER", "TALLNUT_HEAD"}, 5, -1, -1, -1,
		SpecialLevel::None},
	{58, "6-8", kNightRoof, 150, 30, {"NORMAL", "TRAFFIC_CONE", "DOOR", "NEWSPAPER", "PEA_HEAD", "JALAPENO_HEAD", "GARGANTUAR"}, 5,
		-1, -1, -1, SpecialLevel::None},
	{59, "6-9", kNightRoof, 150, 20, {"NORMAL", "POLEVAULTER", "PAIL", "SQUASH_HEAD", "LADDER", "DIGGER", "CATAPULT"}, 5, -1, -1,
		-1, SpecialLevel::None},
	{60, "6-10", kRoof, 5000, 10, {"NORMAL"}, 5, -1, -1, 4000, SpecialLevel::None},
	{61, "7-1", kGreenhouse, 200, 20, {"NORMAL", "TRAFFIC_CONE", "POLEVAULTER", "PAIL", "JACK_IN_THE_BOX", "LADDER"}, -1, -1, -1,
		-1, SpecialLevel::None},
	{62, "7-2", kGreenhouse, 200, 20, {"NORMAL", "TRAFFIC_CONE", "NEWSPAPER", "ZAMBONI", "BUNGEE"}, -1, -1, -1, -1,
		SpecialLevel::None},
	{63, "7-3", kAquarium, 250, 30, {"NORMAL", "PAIL", "SNORKEL", "BALLOON", "JALAPENO_HEAD"}, -1, -1, -1, -1, SpecialLevel::None},
	{64, "7-4", kAquarium, 250, 30, {"NORMAL", "TRAFFIC_CONE", "PAIL", "DOOR", "DOLPHIN_RIDER", "GARGANTUAR", "GATLING_HEAD"}, -1,
		-1, -1, -1, SpecialLevel::None},
	{65, "7-5", kDay, 0, 30, {"NORMAL", "TRAFFIC_CONE", "FOOTBALL", "DANCER", "ZAMBONI", "GARGANTUAR"}, -1, -1, -1, -1,
		SpecialLevel::StarSnake},
	{66, "7-6", kRoof, 150, 20, {"NORMAL", "POLEVAULTER", "PAIL", "DANCER", "PEA_HEAD", "CATAPULT"}, -1, -1, -1, -1,
		SpecialLevel::None},
	{67, "7-7", kFog, 100, 30,
		{"NORMAL", "JACK_IN_THE_BOX", "DIGGER", "GARGANTUAR", "WALLNUT_HEAD", "JALAPENO_HEAD", "SQUASH_HEAD"}, -1, -1, 67, -1,
		SpecialLevel::None},
	{68, "7-8", kPool, 50, 30, {"NORMAL", "NEWSPAPER", "DOOR", "FOOTBALL", "POGO", "LADDER", "TALLNUT_HEAD"}, -1, -1, -1, -1,
		SpecialLevel::None},
	{69, "7-9", kNight, 150, 30,
		{"NORMAL", "TRAFFIC_CONE", "PAIL", "CATAPULT", "GATLING_HEAD", "SQUASH_HEAD", "REDEYE_GARGANTUAR"}, -1, -1, -1, -1,
		SpecialLevel::None},
	{70, "7-10", kDay, 500000, 10, {"NORMAL"}, -1, -1, -1, -1, SpecialLevel::Boss},
	{71, "8-1", kDay, 150, 30,
		{"NORMAL", "TRAFFIC_CONE", "POLEVAULTER", "PAIL", "DOOR", "FOOTBALL", "DANCER", "JACK_IN_THE_BOX", "POGO", "YETI",
			"GARGANTUAR", "PEA_HEAD", "WALLNUT_HEAD", "JALAPENO_HEAD", "SQUASH_HEAD", "TALLNUT_HEAD", "REDEYE_GARGANTUAR"},
		-1, -1, -1, -1, SpecialLevel::Hidden1},
	{72, "8-2", kDay, 500, 30,
		{"NORMAL", "TRAFFIC_CONE", "POLEVAULTER", "PAIL", "DOOR", "FOOTBALL", "DANCER", "JACK_IN_THE_BOX", "BALLOON", "POGO",
			"GARGANTUAR", "WALLNUT_HEAD", "TALLNUT_HEAD", "REDEYE_GARGANTUAR"},
		-1, -1, -1, -1, SpecialLevel::Hidden2},
	{73, "8-3", kDay, -1, 30,
		{"NORMAL", "TRAFFIC_CONE", "POLEVAULTER", "PAIL", "NEWSPAPER", "DOOR", "FOOTBALL", "ZAMBONI", "GARGANTUAR", "PEA_HEAD",
			"GATLING_HEAD"},
		-1, 5000, -1, -1, SpecialLevel::Hidden3},
};

nlohmann::json TableJson() {
	nlohmann::json t = nlohmann::json::object();
	const char* kBgNames[] = {"DAY", "NIGHT", "POOL", "FOG", "ROOF", "NIGHT_ROOF", "MUSHROOM_GARDEN", "GREENHOUSE", "ZOMBIQUARIUM"};
	for (const Row& r : kTable) {
		nlohmann::json j = {{"stage", r.stage}, {"background", kBgNames[r.bg]}, {"waves", r.waves}, {"allowed", r.allowed}};
		if (r.sun >= 0) j["startSun"] = r.sun;
		if (r.pots >= 0) j["freePotColumns"] = r.pots;
		if (r.countdown >= 0) j["firstWaveCountdown"] = r.countdown;
		if (r.fogColumn >= 0) j["fogColumn"] = r.fogColumn;
		if (r.introTimer >= 0) j["introTimer"] = r.introTimer;
		if (r.special != SpecialLevel::None) j["special"] = SpecialName(r.special);
		t[std::to_string(r.level)] = j;
	}
	return t;
}

// rules-later-levels.json
const nlohmann::json kLaterRules = {
	{"laterLevels.lastVanillaLevel", 50},     // the vanilla level tables end at 50 (GameConstants.h NUM_LEVELS)
	{"laterLevels.finalLevel", 73},           // .text 0x452331 `cmp ecx, 0x49`
	{"laterLevels.allowTableIndexAbove", 0},  // CanZombieSpawnOnLevel 0x40d6c7: level-1 >= 49 reads index 0
	{"laterLevels.stageAreaMax", 8},          // 61-70 are 7-x, 71-73 the hidden 8-x (display only)
	{"laterLevels.skip.needsFinishedAdventure", 1}, // hook 239 0x7a1600: PlayerInfo+0x2c > 0
	{"laterLevels.skip.min", 1},
	{"laterLevels.skip.max", 71},             // `if (0x47 < temp) temp = 0x47`
	{"laterLevels.skip.bigStep", 10},         // Mini-games button: temp += clickCount * 10
	{"laterLevels.skip.smallStep", 1},        // Puzzle button: temp += clickCount
	{"laterLevels.skip.longPressTicks", 50},  // port: a touch held 0.5 s counts as the right click (phone has none)
	{"laterLevels.rows.6", {kNormal, kNormal, kNormal, kNormal, kNormal, kDirt}}, // hook 26 0x7982a5
	{"laterLevels.rows.8", {kNormal, kWater, kWater, kWater, kNormal, kDirt}},    // hook 27 0x7a1e3a
	{"laterLevels.roofBackgrounds", {kGreenhouse}},  // V2: StageHasRoof 0x41c0b6 `cmp eax,7; je` before 4 / 5
};

struct RulesGuard {
	explicit RulesGuard(const nlohmann::json& r) { SetRules(r); }
	~RulesGuard() { SetRules(nlohmann::json::object()); }
};
struct TableGuard {
	TableGuard() {
		std::string err;
		REQUIRE(SetLaterLevels(TableJson(), &err));
	}
	~TableGuard() { ClearLaterLevels(); }
};

fs::path PackFile(const char* theName) {
	const char* home = std::getenv("HOME");
	fs::path dir = std::getenv("PVZU_PACK_DIR") ? fs::path(std::getenv("PVZU_PACK_DIR"))
	                                             : fs::path(home ? home : "") / "pvz-unnamed-pack";
	return dir / "data" / theName;
}
} // namespace

TEST_CASE("LaterLevelsTableHasEveryLevel51To73") {
	TableGuard t;
	for (const Row& r : kTable) {
		CAPTURE(r.level);
		const LaterLevel* aRow = FindLaterLevel(r.level);
		REQUIRE(aRow != nullptr);
		CHECK(aRow->level == r.level);
		CHECK(aRow->stage == r.stage);
		CHECK(aRow->background == r.bg);
		CHECK(aRow->startSun == (r.sun >= 0 ? r.sun : kNoLaterRule));
		CHECK(aRow->waves == r.waves);
		CHECK(aRow->allowed == r.allowed);
		CHECK(aRow->freePotColumns == (r.pots >= 0 ? r.pots : kNoLaterRule));
		CHECK(aRow->firstWaveCountdown == (r.countdown >= 0 ? r.countdown : kNoLaterRule));
		CHECK(aRow->fogColumn == (r.fogColumn >= 0 ? r.fogColumn : kNoLaterRule));
		CHECK(aRow->introTimer == (r.introTimer >= 0 ? r.introTimer : kNoLaterRule));
		CHECK(aRow->special == r.special);
	}
}

TEST_CASE("LaterLevelsNoRowOutside51To73") {
	TableGuard t;
	for (int aLevel : {-1, 0, 1, 10, 25, 49, 50, 74, 100})
		CHECK(FindLaterLevel(aLevel) == nullptr);
	ClearLaterLevels();
	CHECK(FindLaterLevel(51) == nullptr); // no pack table (vanilla mode): nothing
}

TEST_CASE("LaterLevelsTableRejectsBadRows") {
	std::string err;
	CHECK_FALSE(SetLaterLevels(nlohmann::json::array(), &err));
	CHECK_FALSE(err.empty());
	err.clear();
	CHECK_FALSE(SetLaterLevels({{"55", {{"background", "MOON"}, {"waves", 20}, {"allowed", {"NORMAL"}}}}}, &err));
	CHECK(err.find("MOON") != std::string::npos);
	CHECK(FindLaterLevel(55) == nullptr); // a bad table leaves no rows
	err.clear();
	CHECK_FALSE(SetLaterLevels({{"six", {{"background", "DAY"}, {"waves", 20}}}}, &err));
	err.clear();
	CHECK_FALSE(SetLaterLevels({{"60", {{"background", "ROOF"}, {"waves", 101}}}}, &err)); // Board.h MAX_ZOMBIE_WAVES 100
	err.clear();
	CHECK_FALSE(SetLaterLevels({{"60", {{"background", "ROOF"}, {"waves", 0}}}}, &err));
	err.clear();
	CHECK_FALSE(SetLaterLevels({{"60", {{"background", "ROOF"}, {"waves", 10}, {"special", "dance"}}}}, &err));
	CHECK(SetLaterLevels({{"60", {{"background", "ROOF"}, {"waves", 10}}}}, &err));
	ClearLaterLevels();
}

TEST_CASE("LaterLevelsAllowListsResolveToEngineTypes") {
	TableGuard t;
	for (const Row& r : kTable) {
		CAPTURE(r.level);
		std::string err;
		const std::vector<int> aTypes = ResolveZombies(FindLaterLevel(r.level)->allowed, kZombieNames, &err);
		CHECK(err.empty());
		REQUIRE(aTypes.size() == r.allowed.size());
		for (size_t i = 0; i < aTypes.size(); i++)
			CHECK(kZombieNames[aTypes[i]] == r.allowed[i]);
		for (int aType : aTypes)
			CHECK(aType != 1); // the Flag Zombie is never in an allow list (flag waves add it)
	}
	std::string err;
	CHECK(ResolveZombies({"NORMAL", "GHOST"}, kZombieNames, &err) == std::vector<int>{0});
	CHECK(err.find("GHOST") != std::string::npos);
}

TEST_CASE("VanillaLevelTablesAreNeverIndexedPastLevel50") {
	// gZombieWaves[NUM_LEVELS] and ZombieAllowedLevels::mAllowedOnLevel[NUM_LEVELS] (Challenge.h): every index the
	// rules hand back stays inside 0..49, for any level a profile or a save can hold.
	for (const bool aUnnamed : {false, true}) {
		RulesGuard g(aUnnamed ? kLaterRules : nlohmann::json::object());
		for (int aLevel = -5; aLevel <= 200; aLevel++) {
			CAPTURE(aLevel);
			CHECK(VanillaLevelIndex(aLevel, 50) >= 0);
			CHECK(VanillaLevelIndex(aLevel, 50) <= 49);
			CHECK(AllowTableIndex(aLevel, 50) >= 0);
			CHECK(AllowTableIndex(aLevel, 50) <= 49);
		}
	}
}

TEST_CASE("AllowTableAbove50ReadsLevel1Column") {
	{
		RulesGuard g(kLaterRules);
		CHECK(AllowTableIndex(10, 50) == 9);
		CHECK(AllowTableIndex(50, 50) == 49); // 5-10 keeps its own column
		CHECK(AllowTableIndex(51, 50) == 0);  // 0x40d6c7: index 0 = level 1 (Normal only)
		CHECK(AllowTableIndex(73, 50) == 0);
	}
	RulesGuard none(nlohmann::json::object()); // vanilla: PvZ-Portable's clamp to 49
	CHECK(AllowTableIndex(51, 50) == 49);
	CHECK(AllowTableIndex(73, 50) == 49);
}

TEST_CASE("UsesLaterTableOnlyAboveTheLastVanillaLevel") {
	TableGuard t;
	RulesGuard g(kLaterRules);
	CHECK_FALSE(UsesLaterTable(50));
	CHECK(UsesLaterTable(51));
	CHECK(UsesLaterTable(73));
	CHECK_FALSE(UsesLaterTable(74)); // no row: vanilla engine path
	ClearLaterLevels();
	CHECK_FALSE(UsesLaterTable(51));
}

TEST_CASE("FinishingLevelsRunsTo73") {
	RulesGuard g(kLaterRules);
	CHECK(FinalAdventureLevel(50) == 73);
	FinishStep s = FinishAdventureLevel(50, FinalAdventureLevel(50));
	CHECK(s.nextLevel == 51); // 5-10 -> 6-1
	CHECK_FALSE(s.finishedAdventure);
	s = FinishAdventureLevel(60, 73);
	CHECK(s.nextLevel == 61);
	s = FinishAdventureLevel(70, 73);
	CHECK(s.nextLevel == 71); // 7-10 -> the hidden levels
	s = FinishAdventureLevel(73, 73);
	CHECK(s.nextLevel == 1); // 0x452331: level 1, mFinishedAdventure += 1
	CHECK(s.finishedAdventure);
}

TEST_CASE("FinishingLevel50InVanillaEndsTheAdventure") {
	RulesGuard none(nlohmann::json::object());
	CHECK(FinalAdventureLevel(50) == 50);
	const FinishStep s = FinishAdventureLevel(50, FinalAdventureLevel(50));
	CHECK(s.nextLevel == 1);
	CHECK(s.finishedAdventure);
	CHECK(FinishAdventureLevel(49, 50).nextLevel == 50);
	CHECK_FALSE(FinishAdventureLevel(49, 50).finishedAdventure);
}

TEST_CASE("StageNamesReachWorld7AndTheHiddenLevels") {
	{
		RulesGuard g(kLaterRules);
		CHECK(StageName(1, 6) == "1-1");
		CHECK(StageName(50, 6) == "5-10");
		CHECK(StageName(51, 6) == "6-1");
		CHECK(StageName(60, 6) == "6-10");
		CHECK(StageName(61, 6) == "7-1");
		CHECK(StageName(70, 6) == "7-10");
		CHECK(StageName(71, 6) == "8-1");
		CHECK(StageName(73, 6) == "8-3");
		CHECK(StageArea(73, 6) == 8);
		CHECK(StageArea(95, 6) == 8);
	}
	RulesGuard none(nlohmann::json::object()); // vanilla clamp (LawnApp::GetStageString, ADVENTURE_AREAS + 1)
	CHECK(StageName(61, 6) == "6-11");
	CHECK(StageArea(61, 6) == 6);
}

TEST_CASE("GardenBackgroundsGetLawnRows") {
	RulesGuard g(kLaterRules);
	std::array<int, 6> aRows{};
	REQUIRE(PlantRowsFor(kMushroom, aRows));
	CHECK(aRows == std::array<int, 6>{kNormal, kNormal, kNormal, kNormal, kNormal, kDirt});
	REQUIRE(PlantRowsFor(kAquarium, aRows));
	CHECK(aRows == std::array<int, 6>{kNormal, kWater, kWater, kWater, kNormal, kDirt});
	for (int aBg : {kDay, kNight, kPool, kFog, kRoof, kNightRoof, kGreenhouse, 9})
		CHECK_FALSE(PlantRowsFor(aBg, aRows)); // vanilla rows (the greenhouse is on the day branch)
	RulesGuard none(nlohmann::json::object());
	CHECK_FALSE(PlantRowsFor(kAquarium, aRows));
}

TEST_CASE("SpecialLevelIdsForTask3") {
	TableGuard t;
	CHECK(FindLaterLevel(55)->special == SpecialLevel::Shooter);
	CHECK(FindLaterLevel(65)->special == SpecialLevel::StarSnake);
	CHECK(FindLaterLevel(70)->special == SpecialLevel::Boss);
	CHECK(FindLaterLevel(71)->special == SpecialLevel::Hidden1);
	CHECK(FindLaterLevel(72)->special == SpecialLevel::Hidden2);
	CHECK(FindLaterLevel(73)->special == SpecialLevel::Hidden3);
	CHECK(FindLaterLevel(60)->special == SpecialLevel::None);
	for (SpecialLevel s : {SpecialLevel::None, SpecialLevel::Shooter, SpecialLevel::StarSnake, SpecialLevel::Boss,
	                       SpecialLevel::Hidden1, SpecialLevel::Hidden2, SpecialLevel::Hidden3})
		CHECK(SpecialFromName(SpecialName(s)) == s);
}

// ---- Level skip (hook 239, 0x7a1600) ----

TEST_CASE("PointerClicksMapToTheExeClickCounts") {
	RulesGuard g(kLaterRules);
	// WidgetManager::mLastDownButtonId: 1 left, -1 right, 2 middle. The exe's click counts: 1, -1, 3.
	CHECK(SkipClickFromPointer(1, 0) == 1);
	CHECK(SkipClickFromPointer(1, 49) == 1);
	CHECK(SkipClickFromPointer(1, 50) == -1); // a long press is the phone's right click
	CHECK(SkipClickFromPointer(-1, 0) == -1);
	CHECK(SkipClickFromPointer(2, 0) == 3);
	CHECK(SkipClickFromPointer(2, 80) == 3);
}

TEST_CASE("LevelSkipNeedsAFinishedAdventure") {
	RulesGuard g(kLaterRules);
	LevelSkip s;
	CHECK(s.Press(SkipButton::Adventure, -1, 30, 0).kind == SkipAction::PassThrough);
	CHECK_FALSE(s.Active());
	CHECK(s.Press(SkipButton::Adventure, 1, 30, 1).kind == SkipAction::PassThrough); // a plain click starts the adventure
	CHECK_FALSE(s.Active());
	const SkipAction a = s.Press(SkipButton::Adventure, -1, 30, 1);
	CHECK(a.kind == SkipAction::Opened);
	CHECK(a.target == 30);
	CHECK(s.Active());
}

TEST_CASE("LevelSkipStepsAndClamps") {
	RulesGuard g(kLaterRules);
	LevelSkip s;
	REQUIRE(s.Press(SkipButton::Adventure, -1, 45, 1).kind == SkipAction::Opened);
	CHECK(s.Press(SkipButton::Minigame, 1, 45, 1).target == 55);
	CHECK(s.Press(SkipButton::Minigame, 1, 45, 1).target == 65);
	CHECK(s.Press(SkipButton::Minigame, 1, 45, 1).target == 71); // clamp 0x47
	CHECK(s.Press(SkipButton::Puzzle, 1, 45, 1).target == 71);
	CHECK(s.Press(SkipButton::Puzzle, -1, 45, 1).target == 70);
	const SkipAction a = s.Press(SkipButton::Minigame, -1, 45, 1);
	CHECK(a.kind == SkipAction::Stepped);
	CHECK(a.target == 60);
	for (int i = 0; i < 8; i++)
		s.Press(SkipButton::Minigame, -1, 45, 1);
	CHECK(s.Target() == 1); // clamp 1
	CHECK(s.Active());
}

TEST_CASE("LevelSkipConfirmWritesTheLevel") {
	RulesGuard g(kLaterRules);
	LevelSkip s;
	s.Press(SkipButton::Adventure, -1, 50, 1);
	s.Press(SkipButton::Puzzle, 1, 50, 1);
	const SkipAction a = s.Press(SkipButton::Survival, 1, 50, 1);
	CHECK(a.kind == SkipAction::AskConfirm);
	CHECK(a.target == 51);
	CHECK(SkipTargetName(a.target) == "6-1");
	s.Decline(); // "No": the mode stays on
	CHECK(s.Active());
	CHECK(s.Press(SkipButton::Survival, 1, 50, 1).kind == SkipAction::AskConfirm);
	CHECK(s.Confirm() == 51);
	CHECK_FALSE(s.Active());
	CHECK(SkipTargetName(70) == "7-10");
	CHECK(SkipTargetName(71) == "8-1");
	CHECK(SkipTargetName(10) == "1-10");
}

TEST_CASE("LevelSkipCancels") {
	RulesGuard g(kLaterRules);
	LevelSkip s;
	s.Press(SkipButton::Adventure, -1, 12, 1);
	CHECK(s.Press(SkipButton::Survival, -1, 12, 1).kind == SkipAction::Cancelled); // right click on Survival
	CHECK_FALSE(s.Active());
	s.Press(SkipButton::Adventure, -1, 12, 1);
	CHECK(s.Press(SkipButton::Puzzle, 3, 12, 1).kind == SkipAction::Cancelled); // middle click anywhere
	CHECK_FALSE(s.Active());
	s.Press(SkipButton::Adventure, -1, 12, 1);
	CHECK(s.Press(SkipButton::Other, 3, 12, 1).kind == SkipAction::Cancelled);
	CHECK_FALSE(s.Active());
}

TEST_CASE("LevelSkipLetsOtherButtonsThrough") {
	RulesGuard g(kLaterRules);
	LevelSkip s;
	s.Press(SkipButton::Adventure, -1, 12, 1);
	s.Press(SkipButton::Puzzle, 1, 12, 1);
	CHECK(s.Press(SkipButton::Other, 1, 12, 1).kind == SkipAction::PassThrough); // e.g. the Almanac: mode stays on
	CHECK(s.Active());
	CHECK(s.Target() == 13);
	// Right click on Adventure while the mode is on: the exe resets the target and starts the adventure.
	const SkipAction a = s.Press(SkipButton::Adventure, -1, 12, 1);
	CHECK(a.kind == SkipAction::PassThrough);
	CHECK(s.Target() == 12);
	CHECK(s.Active());
}

TEST_CASE("LevelSkipOffWithoutRules") {
	RulesGuard none(nlohmann::json::object());
	LevelSkip s;
	CHECK(s.Press(SkipButton::Adventure, -1, 12, 3).kind == SkipAction::PassThrough);
	CHECK_FALSE(s.Active());
}

// ---- The pack holds the same numbers ----

TEST_CASE("LaterLevelsPackTableMatchesTheStudy") {
	const fs::path file = PackFile("levels-51-73.json");
	if (!fs::exists(file)) { MESSAGE("pack not here, skipped: " << file.string()); return; }
	std::ifstream in(file);
	nlohmann::json pack = nlohmann::json::parse(in, nullptr, false);
	REQUIRE(pack.is_object());
	CHECK(pack == TableJson());
	std::string err;
	CHECK(SetLaterLevels(pack, &err));
	CHECK(err.empty());
	ClearLaterLevels();
}

TEST_CASE("LaterLevelsPackRulesMatchTheseValues") {
	const fs::path file = PackFile("rules-later-levels.json");
	if (!fs::exists(file)) { MESSAGE("pack not here, skipped: " << file.string()); return; }
	std::ifstream in(file);
	nlohmann::json pack = nlohmann::json::parse(in, nullptr, false);
	REQUIRE(pack.is_object());
	CHECK(pack == kLaterRules);
}

TEST_CASE("GreenhouseCountsAsRoofAtTheCalledStageHasRoof") {
	{
		RulesGuard g(kLaterRules);
		CHECK(BackgroundCountsAsRoof(kGreenhouse));
		for (int aBg : {kDay, kNight, kPool, kFog, kRoof, kNightRoof, kMushroom, 8, 9})
			CHECK_FALSE(BackgroundCountsAsRoof(aBg)); // the vanilla 4 / 5 stay StageHasRoof's own answer
	}
	RulesGuard none(nlohmann::json::object());
	CHECK_FALSE(BackgroundCountsAsRoof(kGreenhouse));
}
