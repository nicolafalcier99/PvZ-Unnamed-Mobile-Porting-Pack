// PVZU Plan 9 C1: engine side of the test cheats (rules and layouts: Mod/TestCheats.h). Every function is inert
// (vanilla answer / false / no-op) unless the pack's feature "testCheats" is on, so removing that one key removes all
// of it; Vanilla mode (no pack) never sees it.
//
// Where to find them: the "Test cheats" button on the Unnamed options panel's footer (long press Menu on the board
// or the seed chooser, right click on the Mac), and a "Test cheats" button on the main menu. The page has one row per
// cheat (saved in pvzu-settings.json "testCheats"), the actions Kill zombies / Next wave / Final wave (in a level) and
// "Go to level", which opens a picker of levels 1..73 (world tabs) and the mode screens (Mini-games, Puzzle,
// Survival, Unnamed modes). With the "Cheats button in game" toggle on (2026-11-25), a "Cheats" button left of Menu on
// the play screen opens the same page during a level (the board pauses under it, as under Menu).
//
// Dev flags (Mac checks; no effect unless given):
//   -pvzu-dev-cheats k=v,k=v       override saved cheats for this run, not saved (keys of Mod/TestCheats.h)
//   -pvzu-dev-screen cheats        (OptionsPanel's flag) after loading: the main menu with the cheats page open
//   -pvzu-autoplay-cheats          the bot (Mod/Engine/DebugHooks.h -pvzu-autoplay) turns on infinite sun, no
//                                  recharge, plant anything, invincible plants and 15 slots for this run, picks strong
//                                  seeds in the chooser and plants a full defence (Mod/TestCheats.h BotPlanFor)
//   -pvzu-log-bot <file>           one line per bot event (seeds picked, plantings, award, win)
#pragma once

#include "ConstEnums.h"
#include "Mod/TestCheats.h"

class Board;
class LawnApp;
class Plant;

namespace pvzu::engine {

bool TestCheatsOn();
const cheats::Cheats& CurrentCheats();
bool CheatOn(cheats::CheatId theId);

// ---- effects (engine sites) ----
void TestCheatsBoardTick(Board* theBoard);          // Board::PvzuUpdateOnce: sun, recharge, plant health
bool TestCheatPlantSurvives(Plant* thePlant);       // Plant::Squish, Board::KillAllPlantsInRadius, Jalapeno zombie row
bool TestCheatAllSeeds();                           // LawnApp::HasSeedType / GetSeedsAvailable, CARD + Imitater
int TestCheatSeedSlots(int theSlots);               // Board::GetNumSeedsInBank (the bought-slots branch)
bool TestCheatFreePlantingRules();                  // Board::CanPlantAt / PlantingRequirementsMet / cob spot
int TestCheatSpeed(Board* theBoard, int theHudSpeed); // WideScreen BoardUpdatesPerFrame
bool TestCheatModesOpen();                          // ChallengeScreen::MoreTrophiesNeeded, GameSelector locks

// ---- actions ----
bool CheatInLevel(LawnApp* theApp);                 // a level is being played (kill / waves make sense)
void CheatKillAll(Board* theBoard);
void CheatNextWave(Board* theBoard);
void CheatFinalWave(Board* theBoard);
void CheatGoToLevel(LawnApp* theApp, int theLevel);
void CheatOpenMode(LawnApp* theApp, cheats::PickerMode theMode);

// ---- screens ----
void OpenTestCheats(LawnApp* theApp);               // the cheats page (from the options panel or the main menu)
void TestCheatsMainMenuShown(LawnApp* theApp);      // LawnApp::ShowGameSelector: restore a jump, add the menu button
void TestCheatsMainMenuGone(LawnApp* theApp);       // LawnApp::KillGameSelector

// ---- the bot (-pvzu-autoplay-cheats), from DebugHooks' autoplay ----
bool CheatBotOn();
enum class BotEvent { None, AwardSpawned, Lost }; // AwardSpawned: the level is won; Lost: the zombies won (C2)
BotEvent CheatBotStep(LawnApp* theApp);

} // namespace pvzu::engine
