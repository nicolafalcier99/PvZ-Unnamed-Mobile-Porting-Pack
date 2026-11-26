// PVZU: hook points for the Unnamed special levels 55, 65, 70 and 71-73 (Plan 7 Task 3). Task 1 only places the calls
// and names the level (pack data/levels-51-73.json "special" -> unnamed::SpecialLevel, CurrentSpecialLevel()); every
// function is a no-op until Task 3 fills it. Each returns at once when the board's level has no special id, so
// Vanilla mode and levels 1-50 never get here. Evidence for what each level needs: the research notes (w6-w7-and-new-content)
// section 2.3 (55, 65, 71-73) and section 3 (70, the giant Newspaper boss).
#pragma once

#include "misc/KeyCodes.h"

class Board;

namespace pvzu::engine {

// End of Board::InitLevel (seed bank, sun, lawn set up): 55 bank 0 + Cabbage-pult, 71 hides Blover (chooser), ...
void SpecialLevelInit(Board* theBoard);
// Board::StartLevel (the cut-scene is over, play begins): 55/65 messages, ...
void SpecialLevelStart(Board* theBoard);
// Board::Update while playing (next to the dev tick): 71 adds a plant every 1000 frames, 65 steers the star snake, ...
void SpecialLevelUpdate(Board* theBoard);
// Start of Board::SpawnZombieWave (mCurrentWave = the wave being sent): 70 spawns the boss when mCurrentWave == 10.
void SpecialLevelWave(Board* theBoard);
// Start of Board::KeyDown. true = the key was used (55 arrows move the plant, 65 W/A/S/D steer the head).
bool SpecialLevelKeyDown(Board* theBoard, Sexy::KeyCode theKey);

} // namespace pvzu::engine
