// PVZU: small pure helpers for the engine glue of Unnamed's special (non-shooter) mod plants (Plan 8 part 3b,
// Mod/Engine/ModPlantsSpecials.*). The plant rules are in ModPlantRules.h; evidence the research notes (mod-plants-behaviour).
#pragma once

#include <functional>

namespace pvzu::unnamed {

// Mod shot kinds kept in ProjectileState::modShot (State.h): kModShotNone etc. The two Shrapnel kinds equal the exe's
// Projectile+0x84 value (0x794017..0x7941da); the others are the port's own tags.
int ShrapnelKindOfShot(int modShot); // the exe's +0x84 kind (1 shard, 2 core) or 0

// Boss Flower drop (hook 292, 0xc000e0): `do r = Rand(BossFlowerRollRange()) while table[r] != 1`. theUsable filters
// out seeds the engine cannot plant (a mod seed without a row in data/mod-plants.json); the exe's table names only
// rows it has. -1 when the feature is off or no allowed seed came up in theMaxTries rolls.
int BossFlowerDrawSeed(const std::function<int(int)>& theRand, const std::function<bool(int)>& theUsable,
	int theMaxTries = 10000);

// A seed a usable seed-packet coin may carry: a vanilla seed 0..52, or a mod seed 0x50..0x7f with a table row.
bool PacketCoinSeedUsable(int theSeed, const std::function<bool(int)>& theModSeedHasRow);

} // namespace pvzu::unnamed
