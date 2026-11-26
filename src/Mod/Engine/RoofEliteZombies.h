// PVZU: engine side of the Unnamed Roof zombies (Plan 6 Task 2). Each function is called from one `// PVZU:` site in
// Zombie.cpp / Board.cpp / EliteZombies.cpp and returns the vanilla answer (or does nothing) unless the active pack has
// the feature "roofZombies". Rules: Mod/Unnamed/RoofZombies.h; evidence: the research notes (w5-roof) section 2 and
// pvz-unnamed-pack/SOURCES-roof-zombies.md.
//
// Dr. Zomboss (Plan 6 Task 4, study B8): its ZombossBungeeScope makes a bungee whose 3/12 elite roll passes the
// Jack-head bungee here (instant grab, leaves a popping Jack); its own fallback runs only without "roofZombies".
//
// Dev options (shared with the other worlds):
//   -pvzu-force-variant BUNGEE:0x16          wheelbarrow (0x17 Jack head)
//   -pvzu-force-variant LADDER:0x18          shovel (0x35 ladder pile, 0x37 ghost)
//   -pvzu-force-variant CATAPULT:0x19        pole head
//   -pvzu-force-variant GARGANTUAR:0x106     staff (REDEYE_GARGANTUAR:0x106 for the Giga)
//   -pvzu-log-zombies <file>                 spawn lines plus "roof ..." event lines
#pragma once

#include "ConstEnums.h"

#include <optional>

enum ZombieAttackType : int32_t;
class Board;
class Plant;
class Zombie;
namespace Sexy { class Color; }

namespace pvzu::engine {

bool RoofZombiesOn();

// ZombieInitialize (from EliteInit, after the Fog forms) and after a saved game is loaded
void RoofEliteInit(Zombie* theZombie, bool theOnBoard);
void RoofAfterLoad(Zombie* theZombie);
void RoofGargPoleRoll(int thePoleHit);   // Gargantuar case of ZombieInitialize: the vanilla aPoleHit (hook 513)

// Zomboss bungees (study B8): the Bungee roll asks ZombossBungeeSpawning() (Mod/Engine/ZombossSites.h, Plan 6 Task 4:
// open while BossBungeeSpawn makes its bungees = exe flag 0xc04476) and then forces a passing roll to the Jack head.
// Until that task is merged a weak `false` stands in for it (RoofEliteZombies.cpp).

// Bungee
int RoofBungeeBottomCounter(Zombie* theZombie, int theVanilla);          // BungeeLanding: 300 (hook 527)
bool RoofBungeeStealsZombie(Zombie* theZombie);                           // BungeeStealTarget: true = took a zombie (hook 522)
bool RoofPlantAlreadyGrabbed(Plant* thePlant);                            // BungeeStealTarget: steal guard
bool RoofBungeeLiftsZombie(Zombie* theZombie);                            // BungeeLiftTarget: true = rises with it (hook 524)
void RoofBungeeLiftedPlant(Zombie* theZombie, Plant* thePlant);           // BungeeLiftTarget: the Jack head's Jack (hook 529)
bool RoofBlocksPlanting(Board* theBoard, int theGridX, int theGridY);     // Board::CanPlantAt: wheelbarrow 3x3 (hook 46)

// Ladder
bool RoofLadderTargetsAnyPlant(Zombie* theZombie);                        // CanTargetPlant: aPlaceLadder start (hook 655)
std::optional<bool> RoofCanTargetPlant(Zombie* theZombie, Plant* thePlant, ZombieAttackType theAttackType);   // pile
bool RoofLadderPlaces(Zombie* theZombie, Plant* thePlant);                // UpdateLadder: true = the shovel dug it up (hook 600)
void RoofLadderPlaced(Zombie* theZombie, Plant* thePlant);                // UpdateLadder after AddALadder: the pile's bungee
bool RoofKeepsShieldOnLadder(Zombie* theZombie);                          // UpdateLadder: the ghost skips DetachShield (601)
bool RoofReattachLadder(Zombie* theZombie);                               // DetachShield top: the pile's next ladder (722)
bool RoofKeepsLadderShield(Zombie* theZombie);                            // DropShield end: keep the new ladder (694)
void RoofBodyDamageHitsLadder(Zombie* theZombie, int theDamage);          // TakeBodyDamage top (hook 702)
int RoofOverflowIntoLadders(Zombie* theZombie, int theRemaining, unsigned int theDamageFlags);   // TakeDamage (710)
// Plan 9 V1: the pile's bite cadence (hook 672 0x79c300): 3 of 4 ticks with a ladder in hand, every tick without
std::optional<bool> RoofPileBitesThisTick(Zombie* theZombie);
bool RoofEatsPlants(Zombie* theZombie);                                   // EatsPlants: no bites while carrying (672)
float RoofAnimRate(Zombie* theZombie, float theAnimRate);                 // ApplyAnimRate (hook 662)
void RoofZombieTick(Zombie* theZombie);                                   // Zombie::Update top: the ghost's teleport (619)
int RoofBiteDamage(Zombie* theZombie, Plant* thePlant, int theVanilla);   // EatPlant (hook 790)
void RoofTint(Zombie* theZombie, int& theAlpha);                          // DrawReanim colour (hook 647)
bool RoofSkipsShieldDamageImages(Zombie* theZombie);                      // TakeShieldDamage (hook 696)

// Catapult (and for the Kernel-pult butter rule of the Roof plants: a catapult in the locked state, study 1.3)
bool RoofCatapultPrefers(Zombie* theZombie, int theCandidateCol, int theCurrentCol);   // FindCatapultTarget (hook 544)
int RoofCatapultFaith(Zombie* theZombie, int theDamage);                  // TakeDamage top (hook 705)
bool RoofCatapultTooTough(Zombie* theZombie);                             // ApplyBurn (hook 793)
bool RoofCatapultLocked(const Zombie* theZombie);                         // has balls left (Basketball Faith active)

// Gargantuar / Giga
bool RoofGargThrowsAnywhere();                                            // the throw test lost "distance > 40"
float RoofGargThrowDistance(float theComputed, float theMinimum);         // always the minimum (0x527056)
// UpdateZombieGargantuar, in place of `AddZombie(ZOMBIE_IMP, mFromWave)`: the staff throws one of 25 random types
// (a staff Giga's as forced elites, hook 565); everyone else the Imp.
Zombie* RoofGargAddThrown(Zombie* theGarg, bool theHeldImp);   // theHeldImp: mHasObject before the throw (V1)
// right after: true = a thrown non-Imp was set down where the Imp would land (it has no throw animation), skip the Imp code
bool RoofThrownLandsAtOnce(Zombie* theGarg, Zombie* theThrown, bool theHeldImp);
ZombiePhase RoofGargAfterThrowPhase(Zombie* theGarg, ZombiePhase theVanilla);   // keeps throwing (hook 568)

// thrown Imp
bool RoofThrownImpSkipsHouse(Zombie* theZombie);                          // CheckForBoardEdge (hook 630)

} // namespace pvzu::engine
