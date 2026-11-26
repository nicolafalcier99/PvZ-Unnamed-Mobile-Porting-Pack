// PVZU: the hidden levels 71 / 72 / 73 (Plan 7 Task 3a). 71: wave points x112 (hook 21), a Blover at column 7, row -1
// every 1000 ticks (hook 93), helmets x2 (hook 515), Blover barred in the chooser (hook 425). 72 (V2): wave points x80 (hook 21) and
// every plant loses 1 HP per 6 ticks (hook 316). 73: first wave after 5000 cs (table), whole-column planting while the lawn has no zombie and at most 29
// plants (hooks 71, 217), eleven instant / utility seeds barred (hook 425), Newspaper from wave 5 (hook 43). The
// stateless 71 / 73 rules are answered in SpecialLevelHooks.cpp; this file holds the ones that act on the board.
#pragma once

class Board;
class Plant;
class Zombie;

namespace pvzu::engine {

void HiddenInit(Board* theBoard);            // Board::InitLevel end
void HiddenUpdate(Board* theBoard);          // Board::Update while playing: 71's Blover
void HiddenZombieSpawned(Zombie* theZombie); // ZombieInitialize: 71's helmets
// V2: Plant::Update after the vanilla `if (mPlantHealth < 0) Die()` (hook 316, 0xc13150): on 72 a plant on the board
// takes the bite function (0x950000, damage 1) when mMainCounter % 6 == 1, and dies at once below 0 HP (0x950201).
void HiddenPlantTick(Plant* thePlant);

} // namespace pvzu::engine
