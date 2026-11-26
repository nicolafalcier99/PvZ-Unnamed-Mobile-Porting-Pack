// PVZU: engine side of the Unnamed Pool zombies (Plan 4 Task 2). Each function is called from one `// PVZU:` site in
// Zombie.cpp / Plant.cpp / SaveGame.cpp / EliteZombies.cpp and returns the vanilla answer (or does nothing) unless the
// active pack has the feature "poolZombies". Rules: Mod/Unnamed/PoolZombies.h; evidence: the research notes (w3-pool) Z1-Z5.
//
// Dev options (shared with the Day/Night zombies):
//   -pvzu-force-variant SNORKEL:0xB        lotus Snorkel
//   -pvzu-force-variant DOLPHIN_RIDER:0xC  football-head Dolphin Rider
//   -pvzu-force-variant ZAMBONI:1          orange Zomboni (ZAMBONI:2 = red); for the Zomboni the code is its +0x148
//                                          state (ZombieState.extra), not a +0x134 variant
//   -pvzu-log-zombies <file>               spawn lines, plus "freeze" (a Zomboni froze a plant), "zamboni-red",
//                                          "zamboni-armour" and "load frozen plant" lines
#pragma once

class Plant;
class Zombie;

namespace pvzu::engine {

bool PoolZombiesOn();

// ZombieInitialize (from EliteInit, after the Night forms) and after a saved game is loaded
void PoolEliteInit(Zombie* theZombie, bool theOnBoard);
void PoolAfterLoad(Zombie* theZombie);

// Zombie::Update, alive branch, after UpdatePlaying: the Zomboni's freeze box and its orange -> red step
void PoolZombieTick(Zombie* theZombie);
float PoolDolphinJumpVelX(Zombie* theZombie, float theVanilla);   // UpdateZombieDolphinRider, jump start
float PoolZamboniVelX(Zombie* theZombie, float theVanilla);       // UpdateZamboni, tyres whole
int PoolZamboniIceMaxX(int theVanilla);                           // UpdateZamboni ice timer bound
int PoolBodyDamage(Zombie* theZombie, int theDamage);             // TakeBodyDamage: the orange Zomboni's armour
// DrawReanim: the orange / red Zomboni tint, over the plain (white) colour only
void PoolTint(Zombie* theZombie, int& theRed, int& theGreen, int& theBlue);
bool PoolEatsPlants(Zombie* theZombie);                           // CheckIfPreyCaught: the lotus Snorkel never bites
// CanBeFrozen: true = Unnamed's gate replaces `if (!CanBeChilled()) return false;` (hook 712)
bool PoolFreezeGateOn();
bool PoolFreezeGate(Zombie* theZombie);
bool PoolBossRefusesFreeze(Zombie* theZombie);                    // CanBeFrozen, hook 713

// plants frozen by a Zomboni
bool PoolFrozenPlantUpdate(Plant* thePlant);   // top of Plant::Update: true = frozen, the update is skipped
void PoolPlantAfterLoad(Plant* thePlant);       // FixBoardAfterLoad

} // namespace pvzu::engine
