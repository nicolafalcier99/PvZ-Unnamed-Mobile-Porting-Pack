// PVZU: 6-5 (level 55), the Mushroom-garden shooter (Plan 7 Task 3a). After the seed chooser the bank is emptied, sun set
// to 0 and one Cabbage-pult that fires every 50 cs stands in row 0, column 0; Up / Down (or a tap / swipe) move it one
// row; each zombie that loses its head pays ZombieValue x 5 sun, and at 500 / 2000 sun the plant becomes a Melon-pult /
// Winter Melon in place; every non-ZomBotany zombie spawns with half HP. Rules: Mod/Unnamed/SpecialLevels.h; called from
// SpecialLevels.cpp and SpecialLevelHooks.cpp only.
#pragma once

#include "Mod/Unnamed/SpecialLevels.h"
#include "misc/KeyCodes.h"

class Board;
class Plant;
class Zombie;

namespace pvzu::engine {

void ShooterStart(Board* theBoard);                    // Board::StartLevel
bool ShooterKey(Board* theBoard, Sexy::KeyCode theKey); // Board::KeyDown; true = an arrow key was used
void ShooterTouch(Board* theBoard, const unnamed::Touch& theTouch, int theTapX, int theTapY);
void ShooterZombieSpawned(Zombie* theZombie);
void ShooterHeadDropped(Zombie* theZombie);
Plant* ShooterPlant(Board* theBoard); // the live shooter plant (found on the lawn, so a loaded game keeps it)

} // namespace pvzu::engine
