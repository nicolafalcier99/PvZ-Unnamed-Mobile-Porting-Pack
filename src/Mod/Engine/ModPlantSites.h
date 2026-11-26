// PVZU: per-plant behaviour hooks for Unnamed's mod-only plants (Plan 8). Part 1 (infrastructure) calls these from one
// `// PVZU:` site each and leaves the bodies empty, so every mod plant behaves as its base plant. Plan 8 part 2 fills
// the bodies with calls into its pure rules (Mod/Unnamed/ModPlantRules.*), dispatching on the plant's mod seed.
//
// Every hook is called only for a planted mod plant (thePlant->mPvzu.modSeed != 0, feature "modPlants" on), so vanilla
// and ordinary Unnamed plants never reach them. thePlant->mSeedType is the BASE seed (seed - 0x50): the base plant's
// vanilla code runs unless the hook returns true. ModSeedOf(thePlant) / ModSeed(thePlant) gives the mod seed.
// Evidence for the exe's dispatch: the research notes (w6-w7-and-new-content) 5 (hook 311 -> helpers 0x7b0200 .. 0x7b1450,
// 0xc04760; shooters fall through to UpdateShooter).
#pragma once

#include "ConstEnums.h"

class Plant;
class Zombie;

namespace pvzu::engine {

// The plant's mod seed (0x50..0x7f) or SEED_NONE for an ordinary plant. Cheap; safe on any plant.
SeedType ModSeed(const Plant* thePlant);

// Board::AddPlant, right after the plant is made and tagged (exe: Plant init for seeds 0x50+). Set up state here
// (HP, launch rate, reanim tracks).
void ModPlantOnPlanted(Plant* thePlant);

// Plant::UpdateAbilities, after the sleep/bungee checks and the shooting update, before the per-seed dispatch
// (exe hook 311, 0x7a5400). true = handled: the base plant's abilities (and UpdateShooter) are skipped this tick.
bool ModPlantUpdate(Plant* thePlant);

// Plant::Fire at its start. true = handled (the base plant's shot is skipped).
bool ModPlantFire(Plant* thePlant, Zombie* theTargetZombie, int theRow, int thePlantWeapon);

// Plant::DoSpecial at its start (instant plants: Shrapnel Cherry, Rewind Grass, Strong Coffee Bean ...).
// true = handled (the base plant's special is skipped).
bool ModPlantDoSpecial(Plant* thePlant);

// Plant::Die at its start (e.g. Mystic-Ice Shooter turns Peashooters into Snow Peas when it vanishes). Observe only.
void ModPlantOnDie(Plant* thePlant);

} // namespace pvzu::engine
