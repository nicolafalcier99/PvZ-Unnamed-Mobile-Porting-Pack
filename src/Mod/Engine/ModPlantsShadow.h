// PVZU: engine side of Unnamed's shadow state and the plants built on it (Plan 8 part 3c): the shadow state of the
// shadow plants (Moonflower 0x69 gives points to shadow plants in its 3x3; thresholds 3 Moonflower / Fire-dragon /
// Dream Starfruit, 2 Charmed Tall-nut, 1 others), its colour (hook 313), Moonflower's own sun, Toxic Shadow-shroom's
// poison on its eater, Charmed Tall-nut's summons / shadow rules, and the mod plants' placement rules (hooks 39-41,
// 47, 50-52, 62-65). The zombie side of the statuses is Mod/Engine/ModZombieStatus.h.
// Rules: Mod/Unnamed/ModPlantRules.h, Mod/Unnamed/ModPlantShadow.h; evidence: the research notes (mod-plants-behaviour)
// 1.1-1.3, 0x58, 0x67, 0x69 and pvz-unnamed-pack/SOURCES-mod-plants.md. State: Plant::mPvzu.shadow / charmedBitten (saved).
// Everything is inert unless the pack has the feature "modPlants" (and only mod plants, mPvzu.modSeed != 0, are touched).
//
// Dev options (Mac checks; logs go to -pvzu-log-plants <file>):
//   -pvzu-dev-shadow-test <s>  at the first tick of play lay out a scenario on the lawn and keep it fed:
//                                shadow | toxic | charmed | charmed-shadow | status | place | base (ModPlantsShadow.cpp)
//   -pvzu-dev-shadow-dump <tick> at that play tick log every mod plant's shadow state / bite counter and every zombie's
//                                statuses (save checks: quit, then -pvzu-dev-continue and dump again)
#pragma once

#include "ConstEnums.h"

#include <optional>

class Board;
class Plant;
class Zombie;
namespace Sexy {
class Color;
}

namespace pvzu::engine {

// The agreed API: is this plant in the shadow state (exe Plant+0xcc == 1)? Any plant; false for non-shadow plants.
bool ModPlantShadowed(const Plant* thePlant);

// From ModPlantUpdate (hook 311 -> 0x7a5400): the shadow update (every tick, before the plant's own update) and
// Moonflower's sun (0x7b0200). The base plant's code still runs afterwards.
void ModPlantShadowTick(Plant* thePlant);
// From ModPlantOnPlanted: Moonflower's first sun timer (hook 265 0x7a4770: Rand(launchRate + 1)).
void ModPlantShadowOnPlanted(Plant* thePlant);
// From ModPlantOnDie: a log line when a shadow plant leaves (dev checks).
void ModPlantShadowOnDie(Plant* thePlant);
// From ModPlantColor (hook 313 -> 0xc00e00): a shadowed shadow plant draws in (80, 80, 240); true = theColor set.
bool ModPlantShadowColor(const Plant* thePlant, Sexy::Color& theColor);

// Zombie::EatPlant, the bite (cave 0x65122d -> 0x950000): the HP the plant loses. Charmed Tall-nut: x1.5 in shadow,
// counts bitten HP and summons a hypnotised Newspaper / Football / Gargantuar (first after 1500, then every 1200).
int ModPlantBitten(Zombie* theEater, Plant* thePlant, int theDamage);
// Zombie::EatPlant when the bite took the plant to HP <= 0 (hook 688 -> 0x7a65b0): Toxic Shadow-shroom poisons its eater.
void ShadowPlantEatenToDeath(Zombie* theEater, Plant* thePlant);
// Zombie::AnimateChewEffect (the chew hook 0xc021a0, where the Tall-nut thorns are): a shadowed Charmed Tall-nut hurts
// (8) and 1 in 200 charms its eater.
void ModPlantChewed(Zombie* theEater, Plant* thePlant);
// Plant's Tall-nut regeneration site (cave 0x651dd8): true = a Charmed Tall-nut, handled here (it regenerates only in
// the shadow state, 0xc02230); the caller then skips the vanilla Tall-nut regeneration.
bool ModPlantRegenTick(Plant* thePlant);

// Board::CanPlantAt after the Zen Garden / roof checks, before the Grave Buster and Coffee Bean rules. nullopt = the
// vanilla rules decide. Uses the mod seed in the cursor (the planting seed is the base, Mod/Engine/ModPlants.h).
std::optional<PlantingReason> ModPlantCanPlantAt(Board* theBoard, int theGridX, int theGridY, SeedType thePlantingSeed);
// Board::GetPlantsOnLawn (hook 41, 0x7a5040): Magic Devourer, Bowling Wall-nut and Rewind Grass sit in the flying
// (Coffee Bean) slot.
bool ModPlantFlyingSlot(const Plant* thePlant);
// Board::GetPlantsOnLawn: true = a rolling Bowling Wall-nut whose middle has left its planting square (or its row);
// it then holds no square (review fix: the exe's hook 41 keeps mPlantCol, so the first square stayed taken).
bool ModPlantLeftItsSquare(const Plant* thePlant);
// Raw-seed engine checks must not take a mod plant for its base: Moonflower (base Plantern) clears no fog, halves no
// reload, heals nothing (the exe compares 0x19 there, cave 0x651670); those sites test `mPvzu.modSeed == 0`.

// -pvzu-dev-shadow-test / -pvzu-dev-shadow-dump (DevBoardTick).
void ModPlantsShadowDevTick(Board* theBoard, int thePlayTicks);

} // namespace pvzu::engine
