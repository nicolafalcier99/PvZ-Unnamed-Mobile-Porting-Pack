// PVZU: engine side of the mod-plant guide (port-only, feature "modAlmanac"; rules and layout in Mod/Unnamed/ModAlmanac.h).
// Everything is inert unless Unnamed mode, the pack's features "modAlmanac" and "modPlants" and a mod-plant table are
// on, so the vanilla Almanac and vanilla play are unchanged.
//
// Almanac (AlmanacDialog): on the plants page a "More plants" button (bottom centre) switches the packet grid to the
// 26 mod plants (6 x 5 bigger packets on a plain panel); a mod plant's page shows its base plant's animation in the
// mod tint, its name (game language), cost, recharge in seconds, and the pack's text (strings/mod-almanac-en.txt).
// Shown always in Unnamed mode, whether or not CARD has made the plants available (it is a guide).
//
// Lawn hints (the game's message bar, MESSAGE_STYLE_HINT_*): once per level per plant when the player plants it, on a
// tap that cannot act (unloaded Loaded Cannon, Prism-shroom without sun), and instead of the Coffee Bean message when
// a Magic Devourer / Strong Coffee Bean cannot go there. Rule "modAlmanac.hints" 0 turns them off.
//
// Mac checks: every hint shown writes "PVZU hint: ..." to stderr; open the Almanac with -pvzu-dev-screen almanac and
// tap through it with -pvzu-input.
#pragma once

#include "ConstEnums.h"

#include <string>

class AlmanacDialog;
class Board;
class Plant;
namespace Sexy {
class Graphics;
}

namespace pvzu::engine {

bool ModAlmanacOn(); // Unnamed + features modAlmanac / modPlants + rule modAlmanac.pages

// AlmanacDialog sites. All are no-ops (false / the vanilla value) while ModAlmanacOn() is false.
SeedType ModAlmanacBaseSeed(SeedType theSeedType);                    // the plant drawn for a mod seed: its base
void ModAlmanacTintPlant(Plant* thePlant, SeedType theSelectedSeed);   // the mod tint on that plant's body
bool DrawModAlmanacGrid(AlmanacDialog* theDialog, Sexy::Graphics* g);  // true: the mod grid was drawn (skip vanilla's)
bool DrawModAlmanacCard(AlmanacDialog* theDialog, Sexy::Graphics* g);  // true: a mod plant's card text was drawn
void DrawModAlmanacButton(AlmanacDialog* theDialog, Sexy::Graphics* g);
bool ModAlmanacHitTest(AlmanacDialog* theDialog, int x, int y, SeedType& theSeed); // true: the mod grid answers
bool ModAlmanacMouseDown(AlmanacDialog* theDialog, int x, int y);    // true: the button was pressed (page switched)
bool ModAlmanacOverButton(AlmanacDialog* theDialog, int x, int y);
// A mod plant's page text (what it does, then the "How to use:" line), with the game's colour codes; empty while
// ModAlmanacOn() is false or the pack has no text for it. Also the chooser's hold-for-info panel (Mod/Engine/HoldInfo.h).
std::string ModAlmanacPageText(SeedType theSeedType);
std::string ModAlmanacSecondsWord(); // "s" after a recharge in seconds

// Lawn hints.
void ModHintPlanted(Plant* thePlant);                                  // ModPlantPlanted (Board::AddPlant)
void ModHintPlantTapped(Plant* thePlant);                              // Plant::MouseDown
bool ModHintPlantingRefused(Board* theBoard, PlantingReason theReason); // Board::MouseDownWithPlant; true = shown

} // namespace pvzu::engine
