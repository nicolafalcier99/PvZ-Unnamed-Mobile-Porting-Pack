// PVZU: zombie-side statuses that Unnamed's mod plants put on zombies (Plan 8 part 3c). State lives in
// Zombie::mPvzu (pvzu::unnamed::ZombieState: toxicKind / toxicTimer / repeaterPoisonCs / weakenCs, saved in the zombie's
// PVZU TLV field). This is the ONE implementation of these statuses: the shooter plants (ModPlantsShooters.cpp: Dusk-pult
// splash, Dream Starfruit / Spirit-tail grey star, the poison pea) and the special plants (ModPlantsSpecials.cpp:
// Fire-dragon Grass) call the functions below; nothing else writes, ticks, doubles or draws them; rules in Mod/Unnamed/ModPlantRules.h + ModPlantShadow.h; evidence the research notes (mod-plants-behaviour) 1.3.
//
//   * Toxic poison (Toxic Shadow-shroom 0x58, set when a zombie eats it: ShadowPlantEatenToDeath in ModPlantsShadow.h):
//     20 damage every 64 cs for the rest of the level; kind 2 (eaten while shadowed) hits the body only.
//   * Repeater poison (Primal Repeater's poison pea 0x23; the shooter side calls ModZombieApplyPoison on impact):
//     +500 cs per hit, stacking; 1 damage per cs while it runs.
//   * Weakened (Dusk-pult splash +200, shadowed Fire-dragon +400, Dream Starfruit's grey star +400: the callers use
//     ModZombieAddWeakened): every TakeDamage is doubled while > 0 (the reading football-head Newspaper: Rand(2), normal
//     or a quarter); -1 per Zombie::UpdateActions.
// All of it is inert unless the pack has the feature "modPlants" (vanilla mode: nothing runs, nothing is saved).
#pragma once

#include <optional>

namespace Sexy {
class Color;
}
class Zombie;

namespace pvzu::engine {

// Primal Repeater poison pea impact (0xc10c71: side record +8 += 500). Pass PrimalPoisonAddCs(projectileType).
void ModZombieApplyPoison(Zombie* theZombie, int theAddCs);
// Add to the weakened word (exe `add word [zombie+0x1a], n`).
void ModZombieAddWeakened(Zombie* theZombie, int theAddCs);
int ModZombieWeakenedCs(const Zombie* theZombie);
// 0x7aa320: zombies the mod plants never touch (max HP > 120000, Zomboss, the 7-10 Newspaper boss form). The one
// engine helper for it: the shooter plants, the special plants and the statuses here all call it.
bool ModZombieBossClass(const Zombie* theZombie);

// Zombie::Update at its start (hook 618 -> 0xc10cd0 Repeater poison, then the chain to 0x7a6740 Toxic poison).
void ModZombieStatusTick(Zombie* theZombie);
// Zombie::UpdateActions at its start (hook 622 -> 0x95a140): weakened -1.
void ModZombieWeakenTick(Zombie* theZombie);
// Zombie::TakeDamage after the Catapult HP lock (0x95a120 -> 0x95a12b): the damage the zombie really takes.
int ModZombieDamageTaken(Zombie* theZombie, int theDamage);
// Zombie::DrawReanim after the vanilla colour choice: Repeater poison green (0xc10d30), else weakened grey (0x95a365),
// wave zombies only, as colour and additive (u::ZombieStatusTint); they replace the mind-control and chill colours.
void ModZombieTint(const Zombie* theZombie, Sexy::Color& theColor, Sexy::Color& theAdditive, bool& theEnableAdditive);

// Gargantuar smash in Unnamed (hooks 563 / 569 -> 0x7a82a1 / 0x7a8261): a zombie of the other side in front is
// smashed first (TakeDamage 500, Red-eye 1000); a hypnotised Gargantuar never smashes plants (Charmed Tall-nut's
// summons walk right through the player's plants). Under the "modPlants" feature (the summons need it).
// Start: nullopt = the vanilla plant / pot / brain test decides; true / false = smash or not.
std::optional<bool> ModGargantuarStartsSmash(Zombie* theGargantuar);
// The smash lands: true = handled here (the zombie took the hit, or a hypnotised one hit nothing); false = vanilla.
bool ModGargantuarSmashLands(Zombie* theGargantuar);

} // namespace pvzu::engine
