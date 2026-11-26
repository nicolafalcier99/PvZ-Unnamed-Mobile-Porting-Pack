// PVZU: engine side of the Unnamed level 70 (7-10) boss, the giant Newspaper Zombie (Plan 7 Task 3b). Pure rules:
// Mod/Unnamed/Level70Boss.h; evidence: the research notes (w6-w7-and-new-content) section 3 and
// pvz-unnamed-pack/SOURCES-level70-boss.md. Every function is inert unless the pack's feature "level70Boss" is on and the
// board is level 70 (the zombie sites also need the boss form, a Newspaper with variant 0x7f), so Vanilla mode and
// every other level are unchanged. Boss state lives in vanilla fields (HP, shield, phase, mScaleZombie, mZombieRect,
// mBackground, plant sleep, seed packets) plus ZombieState variant / counter (attack timer) / extra (attack).
// Dev flags (no effect unless given; lines go to -pvzu-log-waves and stderr):
//   -pvzu-dev-b70-wave <n>             at play tick 1 jump to wave n (the next wave sent is the n-th)
//   -pvzu-dev-b70-hp <hp>[@age][,...]  set the boss's body HP when it is <age> ticks old (default 1)
//   -pvzu-dev-b70-hit <dmg>[@age][,...] hit the boss through Zombie::TakeDamage when it is <age> ticks old
//   -pvzu-dev-b70-attack <a>[,<a>...]  force the next attacks (0 dark, 1 light, 2 laser, 3 sky, 4 ray gun, 5 frenzy, 6 rest)
//   -pvzu-dev-b70-timer <cs>           every attack timer (and the first) is this long
//   -pvzu-dev-b70-trace <ticks>        a state line every <ticks> while a boss lives
#pragma once

#include "ConstEnums.h"

class Board;
class Plant;
class Projectile;
class Zombie;

namespace pvzu::engine {

bool Level70BossOn();
bool IsLevel70Boss(Zombie* theZombie);   // the live boss form (0x7aa130)

// SpecialLevels.cpp dispatch (level id "boss").
void Level70BossWave(Board* theBoard);   // start of Board::SpawnZombieWave: the 10th wave brings the boss (0x7a07a0)
void Level70BossUpdate(Board* theBoard); // Board::Update while playing: the attack loop (0x7ab000), dev options

// Zombie.cpp sites
void Level70BossBeforeDamage(Zombie* theZombie);   // Zombie::TakeDamage, before the hit: phases (hook 707)
bool Level70BossSkipsDamageStates(Zombie* theZombie); // UpdateDamageStates: true = skip (hook 597)
bool Level70BossPinsWalk(Zombie* theZombie);       // UpdateZombieWalking: true = it stays at x 640 (hook 603)
bool Level70BossNoPrey(Zombie* theZombie);         // CheckIfPreyCaught: true = it eats nothing (hook 673)
bool Level70BossDrawsShield(Zombie* theZombie);    // DrawReanim: draw the shield group with no shield (hook 650)
bool Level70BossHandsOver(Zombie* theZombie);      // DrawReanim: draw the over-shield group (hook 651)
bool Level70BossButterTick(Zombie* theZombie);     // UpdatePlaying butter countdown: true = handled (hook 631)
// LawnMower::UpdatePlaying: a mower drives through boss-class zombies (0x7955f3 -> 0xc01f50 -> 0x795606)
bool Level70BossMowerSkips(Zombie* theZombie);
// EliteZombies.cpp EliteInit: true while the boss itself is being placed (exe 0x800258 = 1: no elite roll)
bool Level70BossPlacing();

// Board.cpp sites
bool Level70BossProgressMeter(Board* theBoard);    // UpdateProgressMeter: true = width set (hook 101)
// MouseDownWithPlant from the bank: a Sprout card plants an Imitater of a random plant (0x7a071a)
void Level70BossBankPlant(Board* theBoard, SeedType& theSeed, SeedType& theImitater);

// Projectile.cpp site: a ray-gun melon (no damage-range flags, star motion) hits plants (hook 380). true = handled.
bool Level70BossShotCollision(Projectile* theProjectile);

} // namespace pvzu::engine
