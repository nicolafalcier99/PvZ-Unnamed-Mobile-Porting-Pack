// PVZU: Unnamed 1.3beta2 Adventure levels 51-73 (world 6 = 51-60, world 7 = 61-70, hidden 71-73). Evidence:
// the research notes (w6-w7-and-new-content) sections 1.1, 1.2, 2.1, 2.2, 6, 7 (and pvz-unnamed-pack/SOURCES-later-levels.md).
// Pure functions. The per-level table comes from the pack's data/levels-51-73.json (SetLaterLevels), the constants
// from data/rules-later-levels.json ("laterLevels.*"); with neither (vanilla) every function gives the vanilla answer.
// The engine guards each call with GameData::Get().Feature("laterLevels") (Mod/Engine/LaterLevels.h).
#pragma once

#include "nlohmann/json.hpp"

#include <array>
#include <string>
#include <string_view>
#include <vector>

namespace pvzu::unnamed {

inline constexpr int kNoLaterRule = -1;

// The levels with their own script (Plan 7 Task 3). The table only names them; the scripts are not here.
enum class SpecialLevel {
	None,
	Shooter,   // 55 (6-5): Mushroom-garden shooter (hooks 462, 160, 138, 596, 517)
	StarSnake, // 65 (7-5): star snake (hooks 138, 401, 496)
	Boss,      // 70 (7-10): giant Newspaper boss at wave 10 (0x7a07a0, section 3)
	Hidden1,   // 71: own wave branch, a plant every 1000 frames, helmets x2, no Blover (hooks 21, 93, 515, 425)
	Hidden2,   // 72: (no own code found besides the table)
	Hidden3,   // 73: 5000 cs first wave, planting/cursor code, chooser bans, Newspaper from wave 5 (hooks 71, 217, 43)
};
const char* SpecialName(SpecialLevel theSpecial);           // "none", "shooter", "starSnake", "boss", "hidden1".."3"
SpecialLevel SpecialFromName(std::string_view theName);     // unknown -> None

struct LaterLevel {
	int level = 0;
	std::string stage;                    // "6-1" (for people reading the file; the engine names stages itself)
	int background = kNoLaterRule;        // engine BackgroundType value (= the exe's Board+0x554c code)
	int startSun = kNoLaterRule;          // kNoLaterRule: vanilla InitLevel sun
	int waves = kNoLaterRule;             // replaces the vanilla gZombieWaves value (the replay +10 rule still applies)
	std::vector<std::string> allowed;     // pack zombie keys (engine names without "ZOMBIE_"); mZombieAllowed
	int freePotColumns = kNoLaterRule;    // pre-placed Flower Pot columns; kNoLaterRule: vanilla (roof 3)
	int firstWaveCountdown = kNoLaterRule;// cs before the first wave; kNoLaterRule: vanilla 1800
	int fogColumn = kNoLaterRule;         // Board::LeftFogColumn on a fog stage
	int introTimer = kNoLaterRule;        // hook 220 cut-scene boss time (6-10: 4000; SpecialIntroBossTime)
	SpecialLevel special = SpecialLevel::None;
};

// Background names in the table, index = engine BackgroundType: DAY NIGHT POOL FOG ROOF NIGHT_ROOF MUSHROOM_GARDEN
// GREENHOUSE ZOMBIQUARIUM. -1 for an unknown name.
int BackgroundCode(std::string_view theName);

// Replaces the table with an object keyed by level ("51": {...}). On any bad row: false, *err says which, no rows kept.
bool SetLaterLevels(const nlohmann::json& theTable, std::string* err);
void ClearLaterLevels();
const LaterLevel* FindLaterLevel(int theLevel); // nullptr: no row

// laterLevels.lastVanillaLevel (50): the vanilla level tables end there. True when theLevel is past it and has a row.
bool UsesLaterTable(int theLevel);

// Pack zombie keys -> engine ZombieType values (index in theEngineNames). Unknown keys are skipped and named in *err.
std::vector<int> ResolveZombies(const std::vector<std::string>& theNames, const std::vector<std::string>& theEngineNames,
	std::string* err);

// Index into a vanilla table sized theNumVanillaLevels (gZombieWaves): always 0..theNumVanillaLevels-1.
int VanillaLevelIndex(int theLevel, int theNumVanillaLevels);
// Index into ZombieAllowedLevels::mAllowedOnLevel for CanZombieSpawnOnLevel: past the last vanilla level the exe reads
// index laterLevels.allowTableIndexAbove (0 = level 1's column, 0x40d6c7); without the rule PvZ-Portable's clamp.
int AllowTableIndex(int theLevel, int theNumVanillaLevels);

// Level finish (LawnApp::UpdatePlayerProfileForFinishingLevel): the final level is laterLevels.finalLevel (73).
int FinalAdventureLevel(int theVanillaFinalLevel);
struct FinishStep {
	int nextLevel = 1;
	bool finishedAdventure = false; // mFinishedAdventure += 1
};
FinishStep FinishAdventureLevel(int theLevel, int theFinalLevel);

// "6-1", "7-10", "8-3": the area is clamped to laterLevels.stageAreaMax (vanilla: theVanillaMaxArea).
int StageArea(int theLevel, int theVanillaMaxArea);
std::string StageName(int theLevel, int theVanillaMaxArea);

// Plant rows (engine PlantRowType values) for a garden background used as a lawn (laterLevels.rows.<bg>).
bool PlantRowsFor(int theBackground, std::array<int, 6>& theRows);
// V2 (C25): StageHasRoof (0x41c0b0) also answers true for these backgrounds (laterLevels.roofBackgrounds: the
// greenhouse of 7-1 / 7-2), at its called sites only (CanPlantAt, InitLawnMowers; inlined `bg==4||bg==5` elsewhere).
bool BackgroundCountsAsRoof(int theBackground);

// ---- Level skip (跳关) on the main menu (hook 239, 0x7a1600) ----
enum class SkipButton { Adventure, Minigame, Puzzle, Survival, Other };
// The exe's click count from the pointer: theButtonId is WidgetManager::mLastDownButtonId (1 left, -1 right,
// 2 middle); a left press held laterLevels.skip.longPressTicks or longer counts as the right click (touch).
int SkipClickFromPointer(int theButtonId, int theHeldTicks);
struct SkipAction {
	enum Kind { PassThrough, Opened, Stepped, AskConfirm, Cancelled };
	Kind kind = PassThrough;
	int target = 0; // the level the mode would skip to
};
class LevelSkip {
public:
	// theClick: 1, -1 or 3 (SkipClickFromPointer). PassThrough = let the menu handle the button as usual.
	SkipAction Press(SkipButton theButton, int theClick, int theProfileLevel, int theFinishedAdventure);
	int Confirm();  // "OK" on the confirm dialog: the level to write into the profile; the mode ends
	void Decline(); // "No": the mode stays on
	void Reset();
	bool Active() const { return mActive; }
	int Target() const { return mTarget; }

private:
	bool mActive = false;
	int mTarget = 0;
};
std::string SkipTargetName(int theLevel); // "X-Y" as the exe builds it (temp / 10 + 1, temp % 10; 0 -> previous area, 10)

} // namespace pvzu::unnamed
