// PVZU: engine side of the Unnamed Fog zombies (Plan 5 Task 2). Each function is called from one `// PVZU:` site in
// Zombie.cpp / Plant.cpp / LawnMower.cpp / Board.cpp / SaveGame.cpp / EliteZombies.cpp and returns the vanilla answer
// (or does nothing) unless the active pack has the feature "fogZombies". Rules: Mod/Unnamed/FogZombies.h; evidence:
// the research notes (w4-fog) section 3 and pvz-unnamed-pack/SOURCES-fog-zombies.md.
//
// Dev options (shared with the Day/Night/Pool zombies):
//   -pvzu-force-variant JACK_IN_THE_BOX:0xE   ice head (0xF potato head)
//   -pvzu-force-variant BALLOON:0x2B          cloud
//   -pvzu-force-variant DIGGER:0x10           screwdriver (its surfacing x still rolls 80/160/240)
//   -pvzu-force-variant POGO:0x11             chocolate (0x12 garlic; its 1-in-4 invisibility still rolls)
//   -pvzu-log-zombies <file>                  spawn lines plus "fog ..." event lines (explosions, the plant freeze,
//                                             cloud landing, digger surfacing / underground bites, pogo bounces)
#pragma once

#include "ConstEnums.h"

#include <cstdint>
#include <optional>

enum ZombieAttackType : int32_t;
class Board;
class Plant;
class Zombie;

namespace pvzu::engine {

bool FogZombiesOn();

// ZombieInitialize (from EliteInit, after the Pool forms) and after a saved game is loaded
void FogEliteInit(Zombie* theZombie, bool theOnBoard);
void FogAfterLoad(Zombie* theZombie);

// Balloon
bool FogPickSpeed(Zombie* theZombie);     // top of PickRandomSpeed: true = mVelX set (0.45 while flying, hook 521)
bool FogCloudLands(Zombie* theZombie);    // top of LandFlyer: true = handled (the cloud only sinks, hook 551)
bool FogFlyerEats(Zombie* theZombie);     // eating update: true = a flyer no longer returns (patch 0x52f63e)
bool FogMowerSkips(Zombie* theZombie);    // LawnMower::Update: a Balloon with its balloon is skipped (cave 0x651615)
// CanTargetPlant, before the tunneling test: the Digger's underground bites (hook 654), the flying Balloon's no-eat
// list (hook 676). nullopt = the vanilla answer.
std::optional<bool> FogCanTargetPlant(Zombie* theZombie, Plant* thePlant, ZombieAttackType theAttackType);

// Digger
float FogDiggerSurfaceX(Zombie* theZombie, float theVanilla);                 // tunneling ends at x < this (hook 585)
ZombiePhase FogDiggerRisePhase(Zombie* theZombie, ZombiePhase theVanilla);    // RISE_WITHOUT_AXE for the screwdriver (hook 586)

// Jack-in-the-Box: the explosion (UpdateZombieJackInTheBox, after FOLEY_EXPLOSION). true = handled (no vanilla blast).
bool FogJackExplodes(Zombie* theZombie);
// top of Plant::Update (after the Pool freeze): true = this plant skips its update (ice-head freeze, hook 315)
bool FogIceFreezeSkips(Plant* thePlant);
void FogBoardAfterLoad(Board* theBoard);   // FixBoardAfterLoad: log line only

// Pogo (UpdateZombiePogo)
bool FogPogoBonks(Zombie* theZombie, Plant* thePlant);   // FORWARD_BOUNCE_2 at 70: vanilla `seed == SEED_TALLNUT`
bool FogPogoLandsOnZombies(Zombie* theZombie);          // FORWARD_BOUNCE_2 at 70: true = it hit zombies, bonk path
bool FogPogoBounceEnd(Zombie* theZombie);               // mPhaseCounter 0: true = Unnamed's bounce end ran
// Board: draw the shadow of an invisible garlic Pogo (the exe still draws it)
bool FogDrawsHiddenShadow(Zombie* theZombie);

} // namespace pvzu::engine
