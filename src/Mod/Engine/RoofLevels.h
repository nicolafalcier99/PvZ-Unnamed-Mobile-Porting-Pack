// PVZU: engine side of the Unnamed Roof level rules (pure rules in Mod/Unnamed/RoofLevels.h).
// The caller guards the rule call with GameData::Get().Feature("roofLevels"); the dev log calls are inert unless
// -pvzu-log-waves / -pvzu-dev-roof is given.
//   -pvzu-dev-roof log|final|final-grave   once playing (Dave's talk and the seed chooser are passed without clicks):
//                                        log only / run the final-wave spawn now (sky drop, then the grave loop) /
//                                        the same with a risen grave placed on lawn row 0 first
//   -pvzu-dev-roof-at <ticks>            play tick of the scenario (default 50)
#pragma once

#include "ConstEnums.h"

class Board;

namespace pvzu::engine {

// Board::SpawnZombiesFromSky after the vanilla counts: the pack's per-level table (exe 0x412C30). No row = unchanged.
void RoofSkyDropCounts(Board* theBoard, int& theCount, int& thePoints);

// Dev log only (no game effect): the counts SpawnZombiesFromSky uses, and the type of each dropped zombie.
void RoofSkyDropLog(Board* theBoard, int theCount, int thePoints);
void RoofSkyDropped(Board* theBoard, ZombieType theType);
// End of Board::PickZombieWaves (before DevLogWaves): roof rule line to the -pvzu-log-waves file.
void DevLogRoof(Board* theBoard);
// Board::Update via DevBoardTick: start sun / award / pots line on the first play tick, -pvzu-dev-roof scenarios.
void RoofDevTick(Board* theBoard, int thePlayTicks);

} // namespace pvzu::engine
