// PVZU: engine side of the Unnamed Dr. Zomboss, default settings (Plan 6 Task 4). Each function is called from one
// `// PVZU:` site in Zombie.cpp / Plant.cpp and returns the vanilla answer (or does nothing) unless the active pack has
// the feature "zomboss". Rules: Mod/Unnamed/Zomboss.h; evidence: the research notes (w5-roof) section 4.1 and
// pvz-unnamed-pack/SOURCES-zomboss.md.
//
// Log: -pvzu-log-zombies <file> gets "boss ..." lines (HP at start, late-spawn picks, heal, crushes, umbrella block,
// ball pick, bungees, ball breaks). None are written while the feature is off.
//
// Dev options (desktop checks, inert unless given; they act on the 5-10 boss once it is playing):
//   -pvzu-dev-boss-hp <n>                   set the boss's body HP at its first update in play
//   -pvzu-dev-boss-age <cs>                 set the boss's mZombieAge then (e.g. 12500 = late spawn list at once)
//   -pvzu-dev-boss-yeti-killed              mark the board's mKilledYeti (Yeti leaves the late list)
//   -pvzu-dev-boss-attack NAME@tick[,...]   from that boss tick, at the next idle: stomp, bungee, summon, head, rv
//   -pvzu-dev-boss-hypno <x>@<tick>         at that boss tick, a hypnotised Normal zombie in rows 0-4 at board x
#pragma once

#include "ConstEnums.h"

#include <optional>

class Board;
class Plant;
class Zombie;

namespace pvzu::engine {

bool ZombossOn();

// ZombieInitialize, case ZOMBIE_BOSS: 50000 in Adventure, 80000 elsewhere (B1)
int ZombossStartHp(Zombie* theBoss, int theVanilla);

// UpdateBoss, after the intro check: dev options only
void ZombossDevTick(Zombie* theBoss);

// BossSpawnContact: the late list (B3); nullopt = the vanilla pick. Afterwards the spawned zombie copies the boss's
// hypnosis (0x957970). Its elite roll already ran in ZombieInitialize (B10).
std::optional<ZombieType> ZombossLateSpawnType(Zombie* theBoss);
void ZombossSpawned(Zombie* theBoss, Zombie* theZombie);

// Stomp (B4, B5): an empty stomp heals; a row with hypnotised zombies or pots can be stomped; the contact crushes them.
void ZombossEmptyStomp(Zombie* theBoss);                        // BossStompAttack, `aRowsCount == 0`
std::optional<bool> ZombossCanStompRow(Zombie* theBoss, int theRow); // top of BossCanStompRow; nullopt = vanilla test
bool ZombossStompContact(Zombie* theBoss);                      // top of BossStompContact: true = crush no plant

// RV (B5, B6): top of BossRVLanding. true = crush no plant (blocked by an Umbrella Leaf, or a hypnotised boss).
bool ZombossRVLanding(Zombie* theBoss);

// Head spit (B7): BossHeadSpit, in place of `mIsFireBall = RandRangeInt(0, 1) == 0`. true = mIsFireBall set.
bool ZombossPickBall(Zombie* theBoss);

// Bungees (B8). The scope is the exe flag 0xC04476: it is open while BossBungeeSpawn makes its bungees, so the Roof
// zombies' Bungee elite roll can force its passing roll to the Jack head (0x17).
struct ZombossBungeeScope {
	ZombossBungeeScope();
	~ZombossBungeeScope();
};
bool ZombossBungeeSpawning();
void ZombossBungeeSpawned(Zombie* theBoss, Zombie* theBungee);  // after AddZombieInRow in BossBungeeSpawn
// Jack-head bungee behaviour, used only while the pack has no "roofZombies" feature (that task owns the Bungee elite;
// with it on these return the vanilla answer so nothing runs twice).
int ZombossBungeeGrabCs(Zombie* theBungee, int theVanilla);     // BungeeLanding, phase counter at the bottom
void ZombossBungeeLifted(Zombie* theBungee, Plant* thePlant);   // BungeeLiftTarget: the Jack drops at the plant

// Freezing (B9)
bool ZombossBreakFireballs(Board* theBoard);                    // Plant::IceZombies: true = handled (every boss)
bool ZombossBreakIceballs(Board* theBoard, int theRow);         // Plant::BurnRow: true = handled (every boss)
bool ZombossRefusesFreeze(Zombie* theZombie);                   // top of CanBeFrozen: the boss at 1 HP

} // namespace pvzu::engine
