// PVZU: bridge between the engine's definition tables and pvzu::GameData.
// The engine's constinit tables stay the vanilla source; lookups return the active (pack-applied) copy.
#pragma once

#include "ConstEnums.h"

class PlantDefinition;
class ZombieDefinition;
class ProjectileDefinition;

namespace pvzu::engine {

// Registers the engine's vanilla tables with GameData (idempotent; called on first use).
void RegisterVanillaTables();
// Copies GameData::Active() into the engine-facing tables. Call after ApplyPack/ResetToVanilla.
void SyncActiveTables();

const PlantDefinition& ActivePlant(SeedType theSeedType);
const ZombieDefinition& ActiveZombie(ZombieType theZombieType);
const ProjectileDefinition& ActiveProjectile(ProjectileType theProjectileType);

} // namespace pvzu::engine
