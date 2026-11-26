// PVZU: engine side of Plan 9 V1 (rules: Mod/Unnamed/ZombieGapsV1.h; evidence:
// the research notes (v1-zombie-contradictions)). Every function returns the vanilla / earlier-port answer unless the
// pack has the feature "zombieGapsV1". Each is called from one `// PVZU:` site.
#pragma once

#include "ConstEnums.h"

class Plant;
class Zombie;

namespace pvzu::engine {

bool ZombieGapsV1On();

// Zombie::TakeDamage, after the dying test: red-eye reading clears the hit flags; the white summoner survives a lethal
// hit once (true = the hit is cancelled, its body is already set to half its max and its arm is gone).
unsigned int V1HitFlags(Zombie* theZombie, unsigned int theFlags);
bool V1MoonwalkerSavesHit(Zombie* theZombie, int theDamage);
// Zombie::TakeBodyDamage: the gasping Newspaper's body HP is never written.
int V1BodyDamage(Zombie* theZombie, int theDamage);
// Zombie::ApplyBurn first: true = take TakeDamage(1800, 0x12) instead (every Newspaper; the white summoner dancing in).
bool V1BurnBecomesHit(Zombie* theZombie);
// Zombie::CanBeFrozen first: the white summoner dancing in cannot be frozen or buttered.
bool V1RefusesFreeze(Zombie* theZombie);
// Zombie::UpdateZombieDancer, the `mIsEating` return: the white summoner summons right after its bite.
void V1DancerEating(Zombie* theZombie);
// Garlic aura (RoofGarlicPoison): true = the red-eye's paper was ripped instead of the 1-damage poke.
bool V1GarlicRipsPaper(Zombie* theZombie);
// Zombie::EatPlant: the knife's no-bite needs mad + knife + no ice freeze + an unfrozen plant (true = bite after all).
bool V1KnifeBitesAnyway(Zombie* theZombie, Plant* thePlant);
// Zombie::EatPlant after the coin step: the green Football's extra bite (plant HP lowered, I, Zombie large sun).
void V1GreenFootballBite(Zombie* theZombie, Plant* thePlant);
// UpdateZombiePolevaulter vault start: true = keep mVelX (road sign 0x29).
bool V1VaultKeepsSpeed(Zombie* theZombie);
// CheckForZombieStep: the garlic Pogo squishes plants like a Zomboni.
void V1ZombieStep(Zombie* theZombie);
// Pogo landing on a zombie: does this hit make the yucky face (vanilla-port answer passed in).
bool V1PogoLandYucky(Zombie* thePogo, bool theFirstHit, bool thePortAnswer);
// UpdateZamboni: a plain Zomboni's speed (the vanilla curve passed in).
float V1ZamboniVelX(Zombie* theZombie, float theCurve);

} // namespace pvzu::engine
