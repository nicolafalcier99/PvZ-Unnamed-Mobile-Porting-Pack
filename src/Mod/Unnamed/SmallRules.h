// PVZU: Plan 9 R9 small rules (the research notes (r9-small-rules), exe reads in pvz-unnamed-pack/SOURCES-small-rules.md).
// Feature "smallRules"; keys smallRules.* in data/rules-small-rules.json. Missing feature or key = vanilla.
//
// Pity hypnotise: Projectile::DoImpact, non-splash hit on a zombie. The damage step (hook 396 -> 0x7b0890 -> 0xc10000)
// doubles a star's table damage and adds 1 to a global star counter (0xc10033) outside I, Zombie levels; then only
// stars go on (0xc0571e), and a plain star (+0x80 == 0, not the Shrapnel core +0x84 == 2) on a zombie that is not boss
// class (0x7aa320) hypnotises it when the counter is full (>= 14370) or Rand(14370) == 0, and the counter goes back to
// 0 (0xc10050..0xc1009b). The engine side is Mod/Engine/SmallRules.h.
#pragma once

namespace pvzu::unnamed {

bool SmallRulesOn();

int PityRollRange();  // Rand range of the pity roll; 0 = no pity rule
int PityCounterAt();  // a counter at or above this hypnotises without a roll
// The damage step counts this projectile's hit (exe type 7 only, never in I, Zombie).
bool PityCountsStar(int projectileType, bool iZombieLevel);
// The hit may hypnotise: exe projectile type 7, its +0x80 (star kind, mod damage) is 0, not the Shrapnel core.
bool PityCandidate(int projectileType, int field80, bool shrapnelCore, bool zombieBossClass);
// A Rand(PityRollRange()) is drawn only while the counter is below PityCounterAt().
bool PityNeedsRoll(int counter);
// roll: the Rand(PityRollRange()) result, or anything when PityNeedsRoll(counter) is false.
bool PityHypnotises(int counter, int roll);

// V2: Zombie::EffectedByDamage, last ground test (hook 715, 0x531c52 -> 0xc11cb0): a flying zombie (IsFlying) asked with
// the GROUND bit is hit when the flags' low byte equals smallRules.flyingHitByFlags (13: Cabbage, Kernel, Melon, Winter
// Melon, mod pults, Squash). 0 / missing = vanilla (never).
bool FlagsHitFlying(unsigned theFlags);

} // namespace pvzu::unnamed
