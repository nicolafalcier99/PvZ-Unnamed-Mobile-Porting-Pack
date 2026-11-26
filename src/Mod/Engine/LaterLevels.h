// PVZU: engine side of the Unnamed Adventure levels 51-73 (Plan 7 Task 1; pure rules in Mod/Unnamed/LaterLevels.h,
// evidence the research notes (w6-w7-and-new-content) and pvz-unnamed-pack/SOURCES-later-levels.md). Every function is inert
// (returns false / the vanilla value) unless the pack's feature "laterLevels" is on, so Vanilla mode is unchanged:
// adventure still ends at 50 and the vanilla level tables (gZombieWaves, mAllowedOnLevel) are read as before.
// Dev flags (no effect unless given):
//   -pvzu-dev-win <tick>       once playing: win the level at that play tick (level complete -> profile update)
//   -pvzu-dev-finished <n>     at profile load: set the profile's mFinishedAdventure (level-skip checks)
#pragma once

#include "ConstEnums.h"
#include "Mod/Unnamed/LaterLevels.h"

class Board;
class LawnApp;
class PlayerInfo;

namespace pvzu::engine {

bool LaterLevelsOn();
// Board::PickBackground (Adventure): the table's background for 51..73. false = vanilla chain.
bool LaterLevelsBackground(Board* theBoard);
// Board::PickBackground: lawn rows for a garden background used in Adventure (mushroom garden, Zombiquarium).
bool LaterLevelsPlantRows(Board* theBoard);
// Board::InitZombieWaves (Adventure): mZombieAllowed from the table (the exe runs Challenge::InitZombieWaves here,
// hook 28 / 176). false = vanilla InitZombieWavesForLevel.
bool LaterLevelsAllowed(Board* theBoard);
// Board::InitZombieWaves after the vanilla first-wave countdown (hook 29: level 73 waits 5000 cs).
void LaterLevelsFirstWaveCountdown(Board* theBoard);
// Board::PickZombieWaves: the table's wave count instead of the vanilla gZombieWaves value.
int LaterLevelsNumWaves(Board* theBoard, int theVanillaWaves);
// Board::InitLevel after the vanilla start sun.
void LaterLevelsStartSun(Board* theBoard);
// CutScene::AddFlowerPots: pre-placed pot columns.
int LaterLevelsPotColumns(Board* theBoard, int theVanillaColumns);
// Board::LeftFogColumn: the 1051 exe's value on a fog stage past level 40 (it returns mLevel itself).
bool LaterLevelsFogColumn(Board* theBoard, int& theColumn);
// Board::CanZombieSpawnOnLevel: index into mAllowedOnLevel (never past the vanilla 50 columns).
int LaterLevelsAllowIndex(int theLevel);
// LawnApp::UpdatePlayerProfileForFinishingLevel: the level that ends the adventure (Unnamed 73, vanilla 50).
int AdventureFinalLevel();
// V2 (C25): the exe's called StageHasRoof (0x41c0b0, `cmp eax,7` first): the greenhouse (7-1 / 7-2) counts as roof.
// Only for the sites that call it in the exe and that the port maps with certainty: Board::CanPlantAt (needs ground for
// Spikerock 0x40e23f, needs a pot 0x40e2c4) and Board::InitLawnMowers (0x40bce2). Vanilla StageHasRoof() elsewhere.
bool StageCountsAsRoof(Board* theBoard);
// LawnApp::GetStageString / GameSelector sign: the largest area number shown.
int StageAreaMax(int theVanillaMax);
// Plan 7 Task 3: which special script the current level runs (None outside 51-73 or without the feature).
unnamed::SpecialLevel CurrentSpecialLevel(const Board* theBoard);

// ---- Level skip on the main menu (GameSelector) ----
void LevelSkipButtonPress(int theButton);    // any GameSelector button press (starts the hold count)
void LevelSkipButtonDownTick(int theButton); // GameSelector::ButtonDownTick
// GameSelector::ButtonDepress before the vanilla handling. true = handled (do not run the vanilla action).
bool LevelSkipButtonDepress(LawnApp* theApp, unnamed::SkipButton theButton, int theButtonId);

// ---- Dev (inert without the flags) ----
void DevLogLaterLevel(Board* theBoard);        // end of Board::PickZombieWaves, -pvzu-log-waves
void LaterLevelsDevTick(Board* theBoard, int thePlayTicks); // DevBoardTick: play line, -pvzu-dev-win
void DevLogFinish(LawnApp* theApp, int theLevel); // after UpdatePlayerProfileForFinishingLevel
void DevLogAward(int theAwardType);            // LawnApp::ShowAwardScreen
void DevProfileOverrides(PlayerInfo* thePlayer); // LawnApp profile load: -pvzu-dev-finished

} // namespace pvzu::engine
