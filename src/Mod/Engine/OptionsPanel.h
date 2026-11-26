// PVZU: engine side of Unnamed's settings/cheats panel (feature "shopUi"; rules Mod/Unnamed/Options.h, geometry
// Mod/OptionsPanel.h, evidence the research notes (modes-ui-shop) sections 3-7 and pvz-unnamed-pack SOURCES-shop-ui.md).
// Every function is inert (vanilla value / false) unless the pack's feature "shopUi" is on, so Vanilla mode is
// unchanged. The options are saved in pvzu-settings.json ("unnamedOptions").
//
// Opening: right click on the Menu button (board or seed chooser), as the exe (hooks 80, 457), or a long press of
// shopUi.panel.longPressTicks on it (touch). The panel is a modal dialog, so the board pauses while it is open.
//
// Dev flags (no effect unless given):
//   -pvzu-dev-options k=v,k=v     override saved options for this run (keys of Mod/Unnamed/Options.h), not saved
//   -pvzu-dev-screen store|almanac|zen|options   after loading: open that screen (makes a "pvzu" profile if none)
//   -pvzu-dev-coins <n>           with -pvzu-dev-screen: the profile's coins (tens: 100 = $1,000)
//   -pvzu-log-store <file>        append one line per shop purchase (item, price, coins before/after, purchases)
#pragma once

#include "Mod/Touch.h"
#include "Mod/Unnamed/Options.h"

#include <string>

namespace Sexy { class Graphics; }
class Board;
class Coin;
class LawnApp;
class Plant;
class Zombie;

namespace pvzu::engine {

bool OptionsPanelOn();
const unnamed::Options& CurrentOptions();
// Called after the app's start-up mode is known (LawnApp::Init): applies options that live in the app (free planting).
void ApplyUnnamedOptions(LawnApp* theApp);

// ---- what the options change ----
int EliteOption();              // eliteoption::DEFAULT / NONE / ALL in effect (the setting: the exe's patched roll code)
bool EliteRollsOff();           // "no elites": the zombie rolls are skipped (0x7b2100 patches, hook 517 gate)
// PVZU elite-all: what exe 0x800258 holds now, for the rolls that read it directly (Normal, Cone, Pole, Newspaper,
// Dancer, Football, Pogo, Zomboni, Ladder's second roll gate, Catapult 0x91009b, Gargantuar staff, Tall-nut head):
// the setting, or "all" while a staff Giga adds its thrown zombie (hook 565: `0x800258 = 2` around the call; the
// patched code keeps following the setting).
int EliteValue();
struct EliteValueAllScope { EliteValueAllScope(); ~EliteValueAllScope(); };
int CollectedSunValue(int theValue);      // Coin::ScoreCoin, double sun (hook 204)
// V2: Challenge/SeedBank::UpdateConveyorBelt step (hooks 161, 473): 1, x2 double sun, x2 HUD fast (the exe's Fast);
// 1 when the panel's feature is off.
int ConveyorStep(Board* theBoard);
bool CoinCollectsItself(Coin* theCoin);   // Coin::Update, auto-collect (hook 207)
bool DoorClosed();                        // Zombie::CheckForBoardEdge: no "zombies won" (hook 630)
bool ZombiesStopped();                    // Zombie::UpdateZombieWalking, STOP (hook 603)
bool TreePowerOn();                       // World7PlantHp (hook 268), HARDWS off

// ---- R8: the rest of the live dialog (feature "extraOptions" with "shopUi"; the research notes (r8-options)) ----
// Every one is off (vanilla) unless both features are on.
bool ExtraOptionsOn();
int RainOption();                         // 0 / 1 rain / 2 storm: render list (117), Challenge::Update (163, 164), DrawWeather (185, 186)
bool GravestonesOn();                     // Challenge::SpawnZombieWave: Grave Danger waves (hook 182)
bool NoSpawnLimitOn();                    // Board::PickZombieType: no first-wave / points gate (hook 42, HARDSP)
bool DaveNoPickOn();                      // SeedChooserScreen: Dave picks nothing, his picks are freed (hooks 436, 447)
bool SpawnForwardOn();                    // ZombieInitialize: no +133 px start (hooks 480, 492, 495, 497, 512)
bool FullFogOptionOn();                   // Board::LeftFogColumn: fog from column 0 in fog stages (hook 145, port row)
// SeedChooserScreen: the vanilla "Dave picks" gate (adventure, not the first run) through hook 436.
bool DavePicksSeeds(LawnApp* theApp, bool theVanillaPicks);
// SeedChooserScreen: a picked seed in the bank stays locked (hook 447); with 戴夫不选卡 the flag is cleared.
bool DavePickLocks(bool& thePicked);

// ---- opening the panel ----
// The Menu button got a press: a right click (theClickCount -1) opens the panel at once (true = handled);
// a left press starts the long-press count. theOwner: the screen with the Menu button (board or seed chooser).
bool OptionsMenuDown(LawnApp* theApp, const void* theOwner, int theClickCount);
// Every update of the screen that owns the Menu button (only the owner of the press counts it).
void OptionsMenuPoll(LawnApp* theApp, const void* theOwner, bool theOverMenu);
// The press lifted: true = it was a long press, skip the Menu button's own action.
bool OptionsMenuUp();
void OpenOptionsPanel(LawnApp* theApp);

// ---- dev ----
bool DevStartScreen(LawnApp* theApp);     // TitleScreen once loading is done (-pvzu-dev-screen)
void DevLogPurchase(LawnApp* theApp, int theItem, int theCost, int theCoinsBefore); // StoreScreen::PurchaseItem

// PVZU Plan 9 C1: the panel's art, shared with the test-cheats pages (Mod/Engine/TestCheats.cpp).
void PanelDrawFrame(Sexy::Graphics* g, int x, int y, int w, int h);
void PanelDrawStone(Sexy::Graphics* g, const HitRect& r, bool down, const std::string& label);
void PanelDrawRow(Sexy::Graphics* g, const HitRect& r, bool down, const std::string& label, const std::string& value, bool nonDefault);

} // namespace pvzu::engine
