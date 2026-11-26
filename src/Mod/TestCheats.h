// PVZU Plan 9 C1: the test cheats (2026-11-23: "give the bot cheats so he can plant whatever he wants. Give it
// to me also so I can activate them and turn them off to test the app; when the game is ready I will remove them").
// Not a mod feature: a testing aid. Everything is hidden and off unless the pack's feature "testCheats" is on, so one
// pack switch removes it. Pure rules, layouts and the cheat bot's plan; Mod/Engine/TestCheats.h applies them.
//
// The cheats are saved in pvzu-settings.json ("testCheats", by key), one toggle or value each:
//   infiniteSun     sun stays at 9990
//   noRecharge      seed packets are ready again at once
//   plantAnything   every seed (and the mod plants) in the chooser, upgrades need no base plant, Cob Cannon on
//                   one free square (the vanilla easy-planting cheat's planting rules)
//   invincible      plants never lose health and are never crushed or blown up by zombies
//   seedSlots       Normal, or 6..15 seed slots
//   speed           1x / 2x / 4x / 8x (the faster of this and the HUD's 1x/2x button)
//   allModes        every mini-game, puzzle and survival level open
//   gameButton      a "Cheats" button on the play screen's top bar that opens this page during a level (the developer
//                   2026-11-25: "change cheat setting during a game, so I need to see the button to open cheats menu")
// Actions (buttons): kill all zombies, next wave, final wave, go to a level 1..73 or a mode.
//
// "Go to level" keeps the player's real progress: the first jump saves the profile's level and "finished adventure" count
// (and the adventure saved game is set aside); back on the main menu they come back (LevelReturn).
#pragma once

#include "Mod/OptionsPanel.h"
#include "Mod/Touch.h"

#include <map>
#include <string>
#include <vector>

namespace pvzu::cheats {

enum class CheatId { InfiniteSun, NoRecharge, PlantAnything, Invincible, SeedSlots, Speed, AllModes, GameButton };

struct CheatInfo {
	CheatId id;
	const char* key;      // pvzu-settings.json "testCheats" key
	int values;           // cycles 0 .. values-1; 0 = off / normal
	const char* labelKey; // string id (pack strings), English fallback below
	const char* label;
};
const std::vector<CheatInfo>& CheatTable();
const CheatInfo& InfoFor(CheatId id);

class Cheats {
public:
	Cheats();
	int Get(CheatId id) const;
	void Set(CheatId id, int value); // out of range -> 0
	int Cycle(CheatId id);
	bool On(CheatId id) const { return Get(id) != 0; }
	bool operator==(const Cheats&) const = default;

private:
	std::vector<int> mValues;
};
Cheats CheatsFromMap(const std::map<std::string, int>& theMap);
std::map<std::string, int> CheatsToMap(const Cheats& theCheats);
const char* CheatValueLabel(CheatId id, int value);

int CheatSeedSlots(int theValue);       // 0 = no change, else 6..15
int CheatSpeedFactor(int theValue);     // 1, 2, 4, 8
int EffectiveSpeed(int theHudSpeed, int theCheatValue);

// "No recharge" on one packet (C2): a recharging packet gets its counter set to the end, so the packet's own update
// finishes the refresh (Activate + FlashIfReady, where the 1-1 / 1-2 tutorials move on); an inactive idle packet is
// activated (C1 behaviour).
enum class NoRechargeStep { Leave, FinishRefresh, Activate };
NoRechargeStep NoRechargeFor(bool theActive, bool theRefreshing);
int NoRechargeCounter(int theCounter, int theRefreshTime);

// ---- level jump ----
struct LevelReturn {
	int profileId = -1; // -1: no jump active
	int level = 0;
	int finished = 0;
	bool Active() const { return profileId >= 0; }
	bool operator==(const LevelReturn&) const = default;
};
LevelReturn ReturnFromMap(const std::map<std::string, int>& theMap);
void ReturnToMap(const LevelReturn& theReturn, std::map<std::string, int>& theMap); // inactive: keys removed
// The first jump keeps the profile's real level; later jumps (still testing) keep that first snapshot.
LevelReturn BeginJump(const LevelReturn& theCurrent, int theProfileId, int theLevel, int theFinished);
bool RestoreFor(const LevelReturn& theReturn, int theProfileId);

constexpr int kPickerWorlds = 8; // 1..7 and the hidden 71..73 ("8-1".."8-3")
constexpr int kLastLevel = 73;
int LevelsInWorld(int theWorld);
int LevelFor(int theWorld, int theIndex); // 0 = no level
int WorldOfLevel(int theLevel);
std::string LevelName(int theLevel);      // "5-10", "8-1"

// Board::mCurrentWave (the next wave to send) for "final wave": never goes back.
int FinalWaveStart(int theCurrentWave, int theNumWaves);

// ---- layouts (screen px; inside the safe insets, big touch targets) ----
enum class CheatAction { KillAll, NextWave, FinalWave, GoToLevel };
constexpr int kCheatActions = 4;
struct CheatsPageLayout {
	HitRect panel, title;
	std::vector<HitRect> rows;    // one per cheat, column by column
	std::vector<HitRect> actions; // CheatAction order
	HitRect back, done;
	int columns = 1;
};
CheatsPageLayout LayoutCheatsPage(int theScreenW, int theScreenH, int theSafeLeft, int theSafeRight, int theRows);

enum class PickerMode { Minigames, Puzzle, Survival, ModModes };
constexpr int kPickerModes = 4;
struct PickerLayout {
	HitRect panel, title;
	std::vector<HitRect> worlds; // tabs 1..8
	std::vector<HitRect> levels; // 10 per world (fewer used in world 8)
	std::vector<HitRect> modes;  // PickerMode order
	HitRect back;
};
PickerLayout LayoutLevelPicker(int theScreenW, int theScreenH, int theSafeLeft, int theSafeRight);
int HitIndex(const std::vector<HitRect>& theRects, int x, int y); // -1 none

// "Test cheats" on the options panel: the right end of its title band (the footer belongs to the panel's own
// buttons). Uses only the panel, title and rows of the layout.
HitRect OptionsTitleCheatsButton(const OptionsPanelLayout& theLayout);
// "Test cheats" on the main menu: top right of the centred 800-px menu, in the sky over the tombstone (outside the
// 800 px the side fill is drawn over everything, so it has to be inside).
HitRect MainMenuCheatsButton(int theMenuLeft);

// ---- the in-game "Cheats" button (cheat gameButton) ----
// Shown only while a level is being played with the pack's testCheats feature and the toggle both on: not on menus,
// the seed chooser, the level intro, after the award, under any dialog (Menu, options, the cheats page itself), while
// paused or while Crazy Dave talks (the top bar slides away), and not in the Zen Garden or the Tree of Wisdom.
struct GameButtonState {
	bool feature = false;      // pack feature testCheats
	bool toggle = false;       // the gameButton cheat
	bool inLevel = false;      // a board in SCENE_PLAYING, not fading out, not complete (engine CheatInLevel)
	bool dialogOpen = false;
	bool paused = false;
	bool daveTalking = false;
	bool gardenOrTree = false;
};
bool GameButtonShown(const GameButtonState& theState);
// Where (logical screen px): a twin of Menu's stone (2026-11-26 "not aligned with the menu one"): Menu's top,
// height and width (so the same art and the label at the same height), kCardGap left of Menu, when that fits right of
// the top bar's last box (HUD speed button, or the vanilla bank's shovel at 800 wide); else (800 wide with 7+ slots)
// in Menu's column right under it, above the lawn. theMenu: the Menu button's rect as placed at run time (vanilla
// y -10, h 46, 117 wide; x moved by the safe insets); theBarRight: right edge of the top bar's last box. hit: the tap
// area (the stone's columns, from the screen top down to finger size, never onto the lawn).
constexpr int kGameButtonUnderMenuH = 40; // 800 wide only (the Mac): the tap area under Menu, the lawn starts 2 px lower
struct GameButtonPlace {
	HitRect draw, hit;
	bool underMenu = false;
};
GameButtonPlace GameButtonRect(const HitRect& theMenu, int theBarRight);

// ---- the cheat bot (-pvzu-autoplay-cheats) ----
namespace seed { // engine SeedType values (ConstEnums.h)
constexpr int LILYPAD = 16, TORCHWOOD = 22, TALLNUT = 23, CACTUS = 26, PUMPKINSHELL = 30, FLOWERPOT = 33,
	GATLINGPEA = 40, GLOOMSHROOM = 42, CATTAIL = 43, WINTERMELON = 44, SPIKEROCK = 46;
}
enum class BotLawn { Day, Night, Pool, Fog, Roof };
// What the bot plants on one square: base (pot / lily pad, -1 none), main plant, armour (pumpkin, -1 none).
struct BotCellPlan { int base = -1, main = -1, armour = -1; };
BotCellPlan BotPlanFor(BotLawn theLawn, int theCol, bool theWater);
// The seeds the bot picks in the chooser (every seed its plan plants, at most 15).
std::vector<int> BotChooserSeeds(BotLawn theLawn);
// C2: a zombie the bot's guard kills because it would chew an invincible plant forever out of every plant's reach
// (an Unnamed digger biting while tunneling, a balloon eating while flying).
bool BotStuckEater(bool theEating, bool theTunneling, bool theFlying, bool theInvincible);
// C2: no zombie hurt, killed or added for theLimit frames while some are alive = a stall (the bot then kills them all,
// logged). Update returns true once per stall and starts watching again.
struct BotStallWatch {
	int since = -1, count = 0, hp = 0;
	bool Update(int theFrame, int theCount, int theHpSum, int theLimit);
};
// C2: the keyboard levels. 6-5's shooter: -1 (Up) / +1 (Down) / 0 towards theTargetRow (-1 = no zombie).
int BotShooterStep(int thePlantRow, int theTargetRow);
// 7-5 / 0x89's star snake: the head runs a rectangle x 140..760, y 120..500 (it loses off x 0..899, y 40..599).
enum class BotTurn { None, Up, Down, Left, Right };
BotTurn BotSnakeTurn(float theX, float theY, float theVx, float theVy);

} // namespace pvzu::cheats
