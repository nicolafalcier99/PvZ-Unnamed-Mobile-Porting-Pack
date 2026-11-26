// PVZU: Unnamed 1.3beta2 Pool (Adventure 3-1..3-10) level rules. Evidence: the research notes (w3-pool) sections "Pool
// rules" and "Levels 3-1..3-10", and the r2 reads in pvz-unnamed-pack/SOURCES-pool-levels.md. Pure functions; every
// constant is a "poolLevels.*" rule (pack data/rules-pool-levels.json); a missing key gives the vanilla behaviour.
// The level tables themselves (waves, flags, allowed zombies, start sun) are vanilla in the exe (0x6A34E8, 0x6A35B0):
// world 3 differs only through global rules (x4 budgets, zombie values, elites, +133 px start) built elsewhere.
#pragma once

namespace pvzu::unnamed {

// Pool cells the final-wave spawn picks from (columns 5..8 x rows 2..3, Board.cpp SpawnZombiesFromPool).
inline constexpr int kPoolSpawnCells = 8;

struct PoolSpawn {
	int count = 0;  // zombies that rise from the pool
	int points = 0; // the engine's (unused) point budget, kept for the table
};

// Final-wave pool spawn of an Adventure pool level (exe Board::SpawnZombiesFromPool 0x4128F0..0x41296C).
// false = no rule for this level: the engine keeps its own counts. count is capped at kPoolSpawnCells.
bool PoolFinalWaveSpawn(int level, PoolSpawn& out);

// Hook 86 (0x9588A0 -> 0x9588BD call 0x4128F0 -> 0x9588D9 jmp 0x412D24): after the pool spawn, the grave loop of
// SpawnZombiesFromGraves still runs (PvZ-Portable returns), so graves on a pool lawn (Pole Vaulter vault graves) rise.
bool PoolGravesAfterPoolSpawn();

// Hook 81 (0x959790): when SpawnZombiesFromGraves was called with the hypnosis argument (the hypnotised Dancer's
// Death Carnival, 0x959750 pushes byte [dancer+0xB8]), every pool riser is hypnotised and one Rand(range) is drawn.
bool PoolRisersTakeHypnosis();
int PoolHypnosisRollRange(); // 0 = no extra roll

// Zombie::UpdateZamboni in-place edit 0x52A8A2 `cmp eax,0x3a5`: the ice trail's timer is set once the ice reaches
// x < 933 (vanilla 800), matching the +133 px zombie start of the wide lawn.
int IceTrailTimerBelowX(int vanillaX);

} // namespace pvzu::unnamed
