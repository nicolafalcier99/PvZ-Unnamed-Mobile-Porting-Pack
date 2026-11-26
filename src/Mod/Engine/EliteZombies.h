// PVZU: engine side of the Unnamed Day zombies (Plan 2 Task 4). Each function is called from one `// PVZU:` site in
// Zombie.cpp / Board.cpp / SaveGame.cpp and does nothing (returns the vanilla answer) unless the active pack has the
// feature "dayZombies". The rules themselves are pure functions in Mod/Unnamed/DayZombies.h.
//
// Dev options (desktop checks):
//   -pvzu-force-variant <TYPE>:<variant>[,...]  every spawned zombie of TYPE (engine name, e.g. PAIL) takes that form
//   -pvzu-log-zombies <file>                    one line per spawn (type, variant, x, HP) and per elite event
#pragma once

#include "ConstEnums.h"
#include "Mod/Unnamed/DayZombies.h"

class Board;
class Plant;
class Zombie;

namespace pvzu::engine {

bool DayZombiesOn();

// ZombieInitialize
float StartOffsetPx(ZombieType theType, int theFromWave);   // right after the generic start x (hook 480)
int PoleStartBaseX(int theVanillaBase);                       // Pole: base + Rand(10) (hook 492)
int ZombieStartBaseX(ZombieType theType, int theVanillaBase); // Zamboni / Catapult / Gargantuars (hooks 495, 497, 512)
void EliteInit(Zombie* theZombie);                            // after the type switch: roll, HP, attack rect, art, log

// Unnamed's gameplay edge (Mod/Unnamed/WideReach.h): vanilla 800 -> 1067 / 1068 so the +133 start can be hit
int ZombieHittableMaxX(int theVanilla);                       // Zombie::EffectedByDamage off-board test
float ProjectileMaxX(float theVanilla);                       // Projectile::CheckForCollision removal x

// per tick / per event
void EliteUpdate(Zombie* theZombie);                          // start of Zombie::Update, before mZombieAge++
float EliteAnimRate(Zombie* theZombie, float theAnimRate);    // ApplyAnimRate, before the chill halving
bool BitesThisTick(Zombie* theZombie, int theTicksBetweenEats);
bool EatsPlants(Zombie* theZombie);
bool ShowsYuckyFace(Zombie* theZombie);
void CoinStep(Zombie* theZombie);                             // CheckForZombieStep
bool EliteEatPlant(Zombie* theZombie, Plant* thePlant);       // EatPlant before the bite; true = plant handled
void EliteOnDeath(Zombie* theZombie);                         // DieNoLoot after TrySpawnLevelAward

// Board::SpawnZombiesFromSky: Unnamed's type picker (hook 85); zombies made inside the scope are "sky-dropped".
ZombieType SkyDropZombieType(Board* theBoard);
struct SkyDropScope {
	SkyDropScope();
	~SkyDropScope();
};

// Plan 9 V1: the glove Pole's leap when a plant dies (hook 361, Mod/Unnamed/DayZombies.h "glove pole leap").
// The exe decides by the caller's EBX; the port names the cause with this scope around the vanilla Die() calls
// that wake it (EatPlant, shovel, KillAllPlantsInRadius, plant replacement, a Squash landing in water).
struct PlantDeathCauseScope {
	explicit PlantDeathCauseScope(unnamed::PlantDeathCause theCause);
	~PlantDeathCauseScope();
	PlantDeathCauseScope(const PlantDeathCauseScope&) = delete;
	PlantDeathCauseScope& operator=(const PlantDeathCauseScope&) = delete;
private:
	unnamed::PlantDeathCause mPrev;
};
void GlovePlantDied(Plant* thePlant);           // Plant::Die right after mDead = true
bool GloveLandingKeepsPole(Zombie* theZombie);  // vault landing: true = back in phase 11 with its pole (hook 557)

// -pvzu-force-variant: the forced form of this type, if any (Night forms use it too)
bool ForcedVariantFor(ZombieType theType, int& theVariant);

// after a saved game is loaded (image overrides outside the resource table are not saved)
void EliteAfterLoad(Zombie* theZombie);

// -pvzu-log-zombies (no-op without the option)
void LogZombieEvent(const char* theFormat, ...);

} // namespace pvzu::engine
