// PVZU: engine side of the Unnamed world-7 zombies and of the ZomBotany zombies in Unnamed (Plan 7 Task 2). Each
// function is called from one `// PVZU:` site in Zombie.cpp / Board.cpp / Plant.cpp / AlmanacDialog.cpp or from the
// Day / Night / Roof bridges, and returns the vanilla answer (or does nothing) unless the active pack has the feature
// "world7Zombies". Rules: Mod/Unnamed/World7Zombies.h; evidence: the research notes (w6-w7-and-new-content) sections 2.4, 4
// and pvz-unnamed-pack/SOURCES-world7-zombies.md.
//
// "World 7" = Adventure and the PROFILE level (PlayerInfo::mLevel) 61..70, never Board::mLevel (exe 0x7aa4c0).
//
// Dev options (with the shared ones: -pvzu-force-variant, -pvzu-force-subvariant, -pvzu-log-zombies, -pvzu-dev-*):
//   -pvzu-dev-player-level <n>        once the dev level plays, set the profile level to n (61..70 = world 7) while the
//                                     board keeps its own level (e.g. -pvzu-dev-level 41 -pvzu-dev-player-level 65)
//   -pvzu-force-variant GARGANTUAR:0x2b     an Imp-Gargantuar look-alike is NOT made this way (the form comes from a
//                                     landing Imp); force IMP spawns with the profile in world 7 instead
//   -pvzu-force-variant CATAPULT:0x70       vase catapult (10 balls, bug-spray art; its shots: Mod/Engine/CatapultShots.h)
//   -pvzu-force-variant FOOTBALL:0 -pvzu-force-subvariant FOOTBALL:0x19   black armour
//   -pvzu-force-variant TALLNUT_HEAD:0x101  the Tall-nut head's 1-in-12 form
// Log lines carry the word "world7" (e.g. "t=.. world7 flag-row ...").
#pragma once

#include "ConstEnums.h"

#include <string>

class Board;
class LawnApp;
class Plant;
class Projectile;
class Zombie;
namespace Sexy { class Image; class Color; }

namespace pvzu::engine {

bool World7ZombiesOn();
bool InWorld7(LawnApp* theApp);           // Adventure + profile level 61..70
bool ProfileAbove60(LawnApp* theApp);     // profile level > 60 (any mode)
const char* World7FormName(Zombie* theZombie);   // nullptr when not a world-7 form

// ZombieInitialize, after the type switch and before EliteInit: ZomBotany HP / armour (every Unnamed mode) and the
// Tall-nut head's 1-in-12 form
void ZomBotanyInit(Zombie* theZombie);
// EliteInit, after the Roof forms: the Tall-nut head's 1-in-12 form and the ZomBotany log line
void World7EliteInit(Zombie* theZombie, bool theOnBoard);
// after a saved game is loaded (image overrides outside the resource table are not saved)
void World7AfterLoad(Zombie* theZombie);
// EliteInit: the Day roll's world number (the Pole Vaulter road sign 0x29 needs 7)
int World7WorldNumber(Zombie* theZombie, int theBoardWorldNumber);

// Board::SpawnZombieWave, right after a wave zombie is added: a Flag in world 7 brings one bucketed Flag per row
void World7FlagWave(Board* theBoard, Zombie* theZombie);
// UpdateZombieImp, landing done (before PHASE_ZOMBIE_NORMAL): world 7, 1 in 3 becomes a Gargantuar
void World7ImpLanded(Zombie* theZombie);
// Zombie::TakeDamage, after the vanilla dying test: true = the hit popped an elite Jack and does no damage
bool World7JackPopsOnHit(Zombie* theZombie);

// Football (NightEliteInit): the Rand(3) roll the Night chain draws -> black armour (returns the subVariant)
int World7FootballExtraRoll(Zombie* theZombie, int theSubVariant, int theVariant, int theRoll);
// PVZU elite-all: NightEliteInit's option-2 Football (NightZombies RollFootballAllElite): profile level > 60, and the
// black paint (only with feature world7Zombies; 0x9dd419 / 0xc11df0)
bool World7ProfileAbove60(Zombie* theZombie);
int World7AllEliteFootballBlack(int theSubVariant, bool theBlack);
bool World7ForcedGoldFootball();          // NightEliteInit: the silver coin's world-7 copy is gold, no rolls
void World7FootballLook(Zombie* theZombie);   // NightEliteInit end: black HP 800 / 4500 and art
Sexy::Image* World7HelmDamageImage(Zombie* theZombie, int theDamageIndex);   // black helmet damage images, or nullptr
struct World7GoldFootballScope {          // EliteEatPlant around the Football copy's AddZombieInRow
	World7GoldFootballScope();
	~World7GoldFootballScope();
};

// Catapult (RoofEliteInit): the Rand(100) spawn roll -> 0x70 (vase) or 0; then balls and art
int World7CatapultForm(Zombie* theZombie, int theRoll, int theEliteSetting); // PVZU elite-all: the player's option
void World7CatapultInit(Zombie* theZombie);
void World7CatapultShot(Zombie* theZombie, Projectile* theShot);   // end of ZombieCatapultFire (types the shot, Plan 9 R3)

// Plant::PlantInitialize before mPlantMaxHealth = mPlantHealth: Tree of Wisdom HP
int World7PlantHp(Plant* thePlant, int theHp);

// ZomBotany behaviour
void HeadPeaFired(Zombie* theZombie, Projectile* theProjectile, bool theGatling);   // after each head pea
// Plan 9 R5 (feature "peaHeadRamp", hook 572's tail 0x7958fe): UpdateZombiePeaHead after its pea, in place of
// `mPhaseCounter = 150`: a Peashooter Zombie rolls Rand(75) to raise its count (mPvzu.stored) and waits 150 - count.
// Returns theVanilla for any other zombie or without the feature. Log: "t=.. world7 pea-ramp PEA_HEAD#id ...".
int PeaHeadNextShot(Zombie* theZombie, int theVanilla);
bool World7JalapenoBurn(Zombie* theZombie);   // UpdateZombieJalapenoHead before the burn: true = burn done (hypnotised)
bool World7SquashWantsJump(Zombie* theZombie);   // UpdateZombieSquashHead: a plant (or enemy) in reach
int World7SquashDestX(Zombie* theZombie, int theVanillaDestX);   // rising / falling: the target's x
// the landing frame: hits the other side's zombies under it; returns the column whose plants are squashed (the
// target's column when it jumped at one, else theVanillaCol)
int World7SquashLanded(Zombie* theZombie, int theDestX, int theVanillaCol);

// Wall-nut Zombie self-copy (Plan 9 R6, feature "wallnutZombieCopy", rules Mod/Unnamed/World7Zombies.h WallnutCopy*;
// the research notes (r6-wallnut-zombie)). Zombie::Update, after the poison tick (hook 618 -> 0xc10cd0 -> 0x7971ea):
// 3600 ticks without its helm HP going down -> a Wall-nut Zombie at its row and x with the same helm HP. Log lines
// "t=.. wallnut-copy ..." with -pvzu-log-zombies.
void WallnutZombieCopyTick(Zombie* theZombie);

// Almanac (AlmanacDialog): 33 zombies, smaller windows, English keys for 26..31
int AlmanacZombieCount(int theVanilla);
ZombieType AlmanacZombieType(int theIndex, ZombieType theVanilla);
bool AlmanacZombieSlot(ZombieType theType, int& theX, int& theY, float& theScale);   // false = vanilla position
float AlmanacWindowScale();                       // 1 = vanilla 76 px windows
bool AlmanacShowsType(ZombieType theType);        // types above the Zomboss follow the same shown rule
std::string AlmanacZombieNameKey(ZombieType theType, const char* theVanilla);

// ---- Tall-nut Zombie (Plan 9 R7, feature "tallnutZombie"; the research notes (r7-tallnut-zombie)) ----
// Log lines carry "tallnut" (with -pvzu-log-zombies). Dev: -pvzu-dev-spawn TALLNUT_HEAD, and
// -pvzu-force-variant TALLNUT_HEAD:0x101 for the summoner form.
bool TallnutZombieOn();
// Zombie::Update after ModZombieStatusTick, before mZombieAge++ (hook 618 -> 0x79729c): the 0x101 form's summon
// (0xc033f0), then Spirit Regeneration of the tall-nut helm
void TallnutZombieTick(Zombie* theZombie);
// Zombie::DrawReanim, before ModZombieTint (0xc03360): the 0x101 form in the mind-control colour at alpha 176
void TallnutZombieTint(const Zombie* theZombie, Sexy::Color& theColor, Sexy::Color& theAdditive, bool& theEnableAdditive);

} // namespace pvzu::engine
