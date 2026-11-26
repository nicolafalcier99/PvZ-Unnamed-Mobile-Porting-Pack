// PVZU: engine side of the Unnamed Night level rules (pure rules in Mod/Unnamed/NightLevels.h).
// Callers guard each call with GameData::Get().Feature("nightLevels"), except the dev log and the grave-rise scope
// queries, which are inert unless a dev flag or an active scope is present.
//   -pvzu-dev-night rise|gold|carnival|carnival-hypno   dev scenario once playing (seed chooser and Dave's talk are
//                                        passed without clicks): raise all graves now / spawn a gold Football
//                                        (variant 0x102) in row 2 and kill it / spawn a Dancer (hypnotised, mid-lawn)
//                                        and remove its backups as soon as they have risen, so it keeps summoning
//   -pvzu-dev-night-at <ticks>           play tick of the scenario (default 50)
#pragma once

#include "ConstEnums.h"

class Board;
class Zombie;
namespace Sexy { class MTRand; }

namespace pvzu::engine {

// Board::PickBackground, Adventure night: places the Unnamed start graves. false = no rule (vanilla layout).
bool NightPlaceStartGraves(Board* theBoard, Sexy::MTRand& theLevelRNG);
// Board::CanZombieSpawnOnLevel: true = allowed in Unnamed although PvZ-Portable's table says no (2-1 Conehead).
bool NightAllowsZombie(ZombieType theZombieType, int theLevel);
// Zombie::DieNoLoot: a gold Football raises a Football from every grave.
void NightOnZombieDied(Zombie* theZombie);
// Zombie::SummonBackupDancer after a backup was made: Death Carnival count.
void NightOnBackupSummoned(Zombie* theDancer);

// Board::SpawnZombiesFromGraves while one of the two calls above is raising graves.
class GraveRiseScope {
public:
	GraveRiseScope(ZombieType theForcedType, bool theMindControlled, const char* theReason);
	~GraveRiseScope();
	GraveRiseScope(const GraveRiseScope&) = delete;
	GraveRiseScope& operator=(const GraveRiseScope&) = delete;
};
// true = skip the pool and sky branches (the gold Football call jumps straight to the grave loop, 0xC05587).
bool GraveRiseGravesOnly();
// true = theType was replaced by the scope's forced type (no re-roll for it).
bool GraveRiseForcedType(ZombieType& theType);
// true = the scope passes the hypnosis argument (hypnotised Dancer's Death Carnival); read by the pool spawn (hook 81).
bool GraveRiseHypnotises();
// After RiseFromGrave: hypnosis from the scope, and a dev log line.
void GraveRiseRisen(Zombie* theZombie);

// End of Board::PickZombieWaves (before DevLogWaves): graves and allowed types to the -pvzu-log-waves file.
void DevLogNight(Board* theBoard);
// DevBoardTick before play: with -pvzu-dev-night (or -pvzu-dev-pool / -pvzu-dev-fog), fill the seed chooser at random and start (dev only).
void NightDevBeforePlay(Board* theBoard);
// Board::Update via DevBoardTick: -pvzu-dev-night scenarios.
void NightDevTick(Board* theBoard, int thePlayTicks);

} // namespace pvzu::engine
