// PVZU: engine side of the Unnamed Pool level rules (pure rules in Mod/Unnamed/PoolLevels.h).
// Callers guard each rule call with GameData::Get().Feature("poolLevels"); the dev log calls are inert unless
// -pvzu-log-waves / -pvzu-dev-pool is given.
//   -pvzu-dev-pool final|final-grave|final-hypno|zamboni   dev scenario once playing (seed chooser and Dave's talk are
//                                        passed without clicks): run the final-wave rise now (pool spawn) / the same
//                                        with a grave placed on lawn row 0 first / the same as a hypnotised Dancer's
//                                        Death Carnival would / spawn a Zomboni in row 1 and log where its ice trail
//                                        starts its timer
//   -pvzu-dev-pool-at <ticks>            play tick of the scenario (default 50)
#pragma once

class Board;
class Zombie;

namespace pvzu::engine {

// Board::SpawnZombiesFromPool after the vanilla counts: the pack's per-level table (exe 0x4128F0). No row = unchanged.
void PoolSpawnCounts(Board* theBoard, int& theCount, int& thePoints);
// Board::SpawnZombiesFromPool after AddZombieInRow, before RiseFromGrave: hook 81 (hypnosis + one Rand(3)).
void PoolRiserBeforeRise(Zombie* theZombie);
// Board::SpawnZombiesFromGraves after SpawnZombiesFromPool: true = continue into the grave loop (hook 86).
bool PoolGravesAfterPool();
// Zombie::UpdateZamboni: x below which the ice trail timer is set (0x52A8A2).
int PoolIceTimerBelowX(int theVanillaX);

// Dev log only (no game effect): the counts SpawnZombiesFromPool uses, and each pool riser after RiseFromGrave.
void PoolSpawnLog(Board* theBoard, int theCount, int thePoints);
void PoolRisen(Zombie* theZombie);
// End of Board::PickZombieWaves (before DevLogWaves): pool rule line to the -pvzu-log-waves file.
void DevLogPool(Board* theBoard);
// Board::Update via DevBoardTick: start-sun line on the first play tick, -pvzu-dev-pool scenarios.
void PoolDevTick(Board* theBoard, int thePlayTicks);

} // namespace pvzu::engine
