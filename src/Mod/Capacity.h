// PVZU: object pool sizes. Vanilla pools hold 1024; Unnamed effects (Jalapeno barrage = 69 fireballs) can fill that,
// and a full DataArray in a release build writes past its end. Pure helpers; tests/test_capacity.cpp.
#pragma once

#include <cstdint>

namespace pvzu {

inline constexpr unsigned kEffectArrayCapacity = 4096; // reanims, particles, emitters, systems, trails, attachments, projectiles

// Board::DrawGameObjects' render list (PVZU Plan 9 C2): vanilla kept 2048 items on the stack, too few for the grown pools
// (7-5's star snake at ~1000 stars overflowed it). Items per object as DrawGameObjects adds them.
struct RenderPools {
	unsigned plants = 1024, zombies = 1024, coins = 1024, mowers = 32, gridItems = 128; // Board::Board
	unsigned projectiles = kEffectArrayCapacity, particleSystems = kEffectArrayCapacity, reanims = kEffectArrayCapacity;
};
constexpr unsigned RenderItemsWorstCase(const RenderPools& p) {
	constexpr unsigned kRowsAndUi = 64; // per-row items (6) + UI layers + cursor preview, with room to spare
	return p.plants * 3 + p.zombies * 3 + p.projectiles * 2 + p.coins + p.mowers + p.gridItems * 2 + p.particleSystems + p.reanims +
		kRowsAndUi;
}
inline constexpr unsigned kRenderItemCapacity = 32768;

// A save stores each pool's size; one made with a smaller pool fits a larger one.
bool SavedArraySizeLoadable(uint32_t theSaved, uint32_t theCurrent);

// How many of theWanted shots to spawn so both pools keep theMargin free slots (a shot = 1 projectile + 2 reanims).
int ShotsThatFit(int theWanted, int theProjectilesFree, int theReanimsFree, int theMargin);

} // namespace pvzu
