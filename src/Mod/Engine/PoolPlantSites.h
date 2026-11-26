// PVZU: engine side of the Unnamed Pool plants (Plan 4 Task 1). Each function is called from one `// PVZU:` site in
// Plant.cpp and does nothing unless the active pack has the feature "poolPlants". Rules: Mod/Unnamed/PoolPlants.h;
// evidence: the research notes (w3-pool) P1-P8 and pvz-unnamed-pack/SOURCES-pool-plants.md.
//
// Dev option: -pvzu-plant-test on a pool lawn (or -pvzu-plant-test-pool anywhere) lays out every Pool plant and keeps
// zombies coming, like the Day/Night plant tests (Mod/Engine/DevHarness.h); -pvzu-log-plants <file> logs each ability.
// -pvzu-dev-pool-jalapeno-from <tick> (default 8000): first tick a Jalapeno may be planted (every 1100 ticks).
#pragma once

#include <string_view>

class Board;
class Plant;

namespace pvzu::engine {

// Plant::DoSpecial, case Jalapeno, before the vanilla row burn (hook 346): fire down the Jalapeno's column on every
// lane, burn the zombies whose rect covers it, cut the ice trails back, break ladders in that column, and destroy a
// Zomboss iceball there.
void PoolJalapenoColumn(Plant* theJalapeno);
// Plant::DoSpecial, case Jalapeno, after Die() (hook 347): the 69-fireball "无名" barrage.
void PoolJalapenoBarrage(Plant* theJalapeno);
// Plant::UpdateAbilities when the Jalapeno's own fuse ran out (cave 0x651E6C): untouched 300/300 HP, Rand(100) <= 17
// plants an Ice-shroom on its square and sets it off at once. Draws the Rand only for an untouched Jalapeno.
void PoolJalapenoIceShroom(Plant* theJalapeno);

// One -pvzu-log-plants line for a Pool ability, "<SEED> col c row r: text" like the Day/Night lines.
void PoolLogPlant(const Plant* thePlant, std::string_view theText);

// Dev harness (Mod/Engine/DevHarness.cpp): true when the pool plant test runs on this board.
bool PoolPlantTestActive(Board* theBoard);
void PoolPlantTestTick(Board* theBoard, int thePlayTicks);

} // namespace pvzu::engine
