// PVZU: Unnamed music (the research notes (modes-ui-shop) section 8 and Final table C). gdi42.dll plays lawnbgm(N).mp3 for
// MusicTune 1-12 (its table at .rdata 0x103dbdf0, played by the PlayMusic detour 0x10018800), and the exe's hook 258
// (0x95c1f0) gives Adventure levels 71-80 Cerebrawl. The pack's data/rules-music.json must hold the same numbers.
#include "doctest/doctest.h"
#include "Mod/Unnamed/Music.h"
#include "Mod/Unnamed/Rules.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using namespace pvzu::unnamed;

namespace {
fs::path PackFile(const char* theName) {
	const char* home = std::getenv("HOME");
	fs::path dir = std::getenv("PVZU_PACK_DIR") ? fs::path(std::getenv("PVZU_PACK_DIR"))
	                                             : fs::path(home ? home : "") / "pvz-unnamed-pack";
	return dir / "data" / theName;
}

// The pack's rules, cleared again at scope end (other tests expect no rules).
struct PackRules {
	PackRules() {
		std::ifstream in(PackFile("rules-music.json"));
		REQUIRE(in.good());
		SetRules(nlohmann::json::parse(in));
	}
	~PackRules() { SetRules(nlohmann::json::object()); }
};

// Study 8.2: MusicTune (vanilla id, Lawn/System/Music.h) -> N of lawnbgm(N).mp3.
constexpr int kTable[][2] = {
	{1, 1},   // DAY_GRASSWALK
	{2, 5},   // NIGHT_MOONGRAINS
	{3, 4},   // POOL_WATERYGRAVES
	{4, 7},   // FOG_RIGORMORMIST
	{5, 12},  // ROOF_GRAZETHEROOF
	{6, 6},   // CHOOSE_YOUR_SEEDS
	{7, 8},   // TITLE_CRAZY_DAVE_MAIN_THEME
	{8, 14},  // ZEN_GARDEN
	{9, 11},  // PUZZLE_CEREBRAWL
	{10, 10}, // MINIGAME_LOONBOON
	{11, 13}, // CONVEYER
	{12, 9},  // FINAL_BOSS_BRAINIAC_MANIAC
};
} // namespace

TEST_CASE("UnnamedMusicTuneTableMatchesStudy") {
	PackRules r;
	for (const auto& row : kTable) {
		CAPTURE(row[0]);
		CHECK(MusicTrackForTune(row[0]) == row[1]);
	}
}

TEST_CASE("UnnamedMusicCreditsAndOutOfRangeTunesStayVanilla") {
	PackRules r;
	for (int aTune : {-1, 0, 13, 14, 100})
		CHECK(MusicTrackForTune(aTune) == 0); // gdi42 0x100188ab: tune index > 12 -> vanilla code
}

TEST_CASE("UnnamedMusicWithoutRulesIsVanilla") {
	SetRules(nlohmann::json::object());
	for (const auto& row : kTable)
		CHECK(MusicTrackForTune(row[0]) == 0);
	CHECK_FALSE(CerebrawlAdventureLevel(71));
	CHECK_FALSE(CerebrawlAdventureLevel(75));
}

TEST_CASE("UnnamedMusicOnlyShippedTrackNumbersPlay") {
	// gdi42 0x10018848: N == 1, N == 4 or 5 <= N <= 14 (files 2 and 3 do not exist).
	for (int aN : {1, 4, 5, 9, 14})
		CHECK(ValidTrackNumber(aN));
	for (int aN : {-1, 0, 2, 3, 15, 99})
		CHECK_FALSE(ValidTrackNumber(aN));
	SetRules({{"music.tracks", {2, 3, 15, 0, 14, 1, 1, 1, 1, 1, 1, 1}}});
	CHECK(MusicTrackForTune(1) == 0);
	CHECK(MusicTrackForTune(2) == 0);
	CHECK(MusicTrackForTune(3) == 0);
	CHECK(MusicTrackForTune(4) == 0);
	CHECK(MusicTrackForTune(5) == 14);
	SetRules({{"music.tracks", {1, 5}}}); // short list: tunes past its end stay vanilla
	CHECK(MusicTrackForTune(2) == 5);
	CHECK(MusicTrackForTune(3) == 0);
	SetRules({{"music.tracks", {"one", 5}}}); // not a number: vanilla
	CHECK(MusicTrackForTune(1) == 0);
	SetRules(nlohmann::json::object());
}

TEST_CASE("UnnamedMusicFileNames") {
	CHECK(MusicTrackFileName(1) == "lawnbgm(1).mp3");   // gdi42 string 0x103db278 "lawnbgm(*).mp3"
	CHECK(MusicTrackFileName(14) == "lawnbgm(14).mp3");
}

TEST_CASE("UnnamedMusicCerebrawlForAdventure71To80") {
	PackRules r;
	// hook 258 0x95c1f0: cmp eax, 0x46 / jle; cmp eax, 0x50 / jg; MakeSureMusicIsPlaying(9)
	for (int aLevel : {71, 72, 73, 80})
		CHECK(CerebrawlAdventureLevel(aLevel));
	for (int aLevel : {1, 10, 50, 69, 70, 81})
		CHECK_FALSE(CerebrawlAdventureLevel(aLevel));
}

// ---- the music choice (2026-11-24: "Music: Original / Unnamed mod") ----
namespace {
// Engine BackgroundType values (ConstEnums.h) and Board's stage tests (Board.cpp StageIsNight / StageHasPool /
// StageHas6Rows / StageHasRoof).
enum Bg { kDay, kNight, kPool, kFog, kRoof, kNightRoof, kMushroomGarden, kGreenhouse, kZombiquarium };
GameTuneInput Lawn(int theBg) {
	GameTuneInput in;
	in.night = theBg == kNight || theBg == kFog || theBg == kNightRoof || theBg == kMushroomGarden || theBg == kZombiquarium;
	in.pool = in.sixRows = theBg == kPool || theBg == kFog;
	in.roof = theBg == kRoof || theBg == kNightRoof;
	return in;
}
GameTuneInput Adventure(int theLevel, int theBg) {
	GameTuneInput in = Lawn(theBg);
	in.adventure = true;
	in.level = theLevel;
	in.finalBoss = theLevel == 50;
	in.miniGame = theLevel == 5 || theLevel == 15 || theLevel == 35 || theLevel == 45;
	in.stormy = theLevel == 40;
	return in;
}
GameTuneInput ModModeLawn(int theMode, int theBg) {
	GameTuneInput in = Lawn(theBg);
	in.gameMode = theMode;
	return in;
}
// MusicTune values (Lawn/System/Music.h)
enum Tune { kNone = -1, kGrasswalk = 1, kMoongrains, kWateryGraves, kRigorMormist, kGrazeTheRoof, kChooseSeeds, kTitle,
	kZen, kCerebrawl, kLoonboon, kConveyer, kBrainiacManiac };

struct TuneRow {
	const char* what;
	GameTuneInput in;
	int original;
	int mod;
};
// Backgrounds of 51-73: the research notes (w6-w7-and-new-content) 2.2; mod modes: pvz-unnamed-pack/data/modes.json.
const TuneRow kTuneTable[] = {
	{"1-1", Adventure(1, kDay), kGrasswalk, kGrasswalk},
	{"1-5 bowling", Adventure(5, kDay), kLoonboon, kLoonboon},
	{"1-10", Adventure(10, kDay), kConveyer, kConveyer},
	{"2-1", Adventure(11, kNight), kMoongrains, kMoongrains},
	{"3-1", Adventure(21, kPool), kWateryGraves, kWateryGraves},
	{"4-1", Adventure(31, kFog), kRigorMormist, kRigorMormist},
	{"4-10 stormy", Adventure(40, kFog), kNone, kNone},
	{"5-1", Adventure(41, kRoof), kGrazeTheRoof, kGrazeTheRoof},
	{"5-10 boss", Adventure(50, kNightRoof), kBrainiacManiac, kBrainiacManiac},
	{"6-1 night roof", Adventure(51, kNightRoof), kMoongrains, kMoongrains},
	{"6-5 shooter", Adventure(55, kMushroomGarden), kLoonboon, kMoongrains},
	{"6-10 roof", Adventure(60, kRoof), kGrazeTheRoof, kGrazeTheRoof},
	{"7-1 greenhouse", Adventure(61, kGreenhouse), kGrasswalk, kGrasswalk},
	{"7-3 zombiquarium", Adventure(63, kZombiquarium), kMoongrains, kMoongrains},
	{"7-5 star snake", Adventure(65, kDay), kLoonboon, kGrasswalk},
	{"7-6 roof", Adventure(66, kRoof), kGrazeTheRoof, kGrazeTheRoof},
	{"7-7 fog", Adventure(67, kFog), kRigorMormist, kRigorMormist},
	{"7-8 pool", Adventure(68, kPool), kWateryGraves, kWateryGraves},
	{"7-9 night", Adventure(69, kNight), kMoongrains, kMoongrains},
	{"7-10 boss", Adventure(70, kDay), kBrainiacManiac, kGrasswalk},
	{"8-1 (71)", Adventure(71, kDay), kCerebrawl, kCerebrawl},
	{"8-2 (72)", Adventure(72, kDay), kCerebrawl, kCerebrawl},
	{"8-3 (73)", Adventure(73, kDay), kCerebrawl, kCerebrawl},
	{"mode 0x65 Art Is an Explosion", ModModeLawn(0x65, kNight), kMoongrains, kMoongrains},
	{"mode 0x66 Sun Storm", ModModeLawn(0x66, kDay), kGrasswalk, kGrasswalk},
	{"mode 0x72 Lone Wolf (6-5's shooter)", ModModeLawn(0x72, kMushroomGarden), kLoonboon, kMoongrains},
	{"mode 0x83 Second Uncle (7-10's boss)", ModModeLawn(0x83, kDay), kBrainiacManiac, kGrasswalk},
	{"mode 0x89 Glutton Star (7-5's snake)", ModModeLawn(0x89, kDay), kLoonboon, kGrasswalk},
};
} // namespace

TEST_CASE("MusicChoiceTunePerLevel") {
	PackRules r;
	for (const TuneRow& row : kTuneTable) {
		CAPTURE(row.what);
		CHECK(GameTune(row.in, musicchoice::ORIGINAL) == row.original);
		CHECK(GameTune(row.in, musicchoice::MOD) == row.mod);
	}
}

TEST_CASE("MusicChoiceVanillaScreensAndModes") {
	PackRules r;
	for (int aChoice : {musicchoice::ORIGINAL, musicchoice::MOD}) {
		CAPTURE(aChoice);
		GameTuneInput zen = Lawn(kMushroomGarden);
		zen.zenOrTree = true;
		CHECK(GameTune(zen, aChoice) == kZen);
		GameTuneInput column = Lawn(kDay);
		column.column = true;
		CHECK(GameTune(column, aChoice) == kConveyer);
		GameTuneInput puzzle = Lawn(kNight);
		puzzle.puzzle = true;
		CHECK(GameTune(puzzle, aChoice) == kCerebrawl);
		GameTuneInput speed = Lawn(kDay);
		speed.miniGame = true;
		CHECK(GameTune(speed, aChoice) == kLoonboon);
		GameTuneInput finalBoss = Lawn(kNightRoof);
		finalBoss.finalBoss = true;
		CHECK(GameTune(finalBoss, aChoice) == kBrainiacManiac);
		GameTuneInput stormy = Lawn(kFog);
		stormy.stormy = true;
		CHECK(GameTune(stormy, aChoice) == kNone);
		// the twins' level numbers only count in Adventure; outside it the mode number does
		GameTuneInput quick = Lawn(kDay);
		quick.level = 70;
		CHECK(GameTune(quick, aChoice) == kGrasswalk);
	}
}

TEST_CASE("MusicChoiceWithoutRulesHasNoCerebrawlLevels") {
	SetRules(nlohmann::json::object());
	CHECK(GameTune(Adventure(71, kDay), musicchoice::ORIGINAL) == kGrasswalk);
	CHECK(GameTune(Adventure(71, kDay), musicchoice::MOD) == kGrasswalk);
	CHECK(GameTune(Adventure(70, kDay), musicchoice::ORIGINAL) == kBrainiacManiac);
}
