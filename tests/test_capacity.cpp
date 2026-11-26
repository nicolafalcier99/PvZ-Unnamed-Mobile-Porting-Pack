// PVZU: effect pools must never overflow (5-10 Jalapeno barrages crashed at 1024 reanims) and old saves must load.
#include "doctest/doctest.h"
#include "Mod/Capacity.h"

TEST_CASE("effect pools hold at least 16 simultaneous Jalapeno barrages") {
	// one Unnamed barrage = 69 fireballs, each with a fire-pea reanim, plus one fire reanim per splash hit
	CHECK(pvzu::kEffectArrayCapacity >= 16 * 69 * 2);
	CHECK(pvzu::kEffectArrayCapacity <= 65536); // DataArray index mask
}

TEST_CASE("saved pool sizes load when not larger than the current pool") {
	CHECK(pvzu::SavedArraySizeLoadable(1024, 1024));
	CHECK(pvzu::SavedArraySizeLoadable(1024, 4096)); // saves made before the pools grew
	CHECK_FALSE(pvzu::SavedArraySizeLoadable(4096, 1024));
	CHECK_FALSE(pvzu::SavedArraySizeLoadable(0, 1024));
}

TEST_CASE("barrage shots are cut to what still fits in the pools") {
	CHECK(pvzu::ShotsThatFit(69, 4000, 4000, 64) == 69);
	CHECK(pvzu::ShotsThatFit(69, 100, 4000, 64) == 36);
	CHECK(pvzu::ShotsThatFit(69, 4000, 90, 64) == 13); // each shot also needs a reanim
	CHECK(pvzu::ShotsThatFit(69, 50, 4000, 64) == 0);
	CHECK(pvzu::ShotsThatFit(0, 4000, 4000, 64) == 0);
}

// PVZU Plan 9 C2: Board::DrawGameObjects' render list had the vanilla 2048 items on the stack while the grown pools
// (4096 projectiles, 2 render items each) can need far more: 7-5's star snake at ~1000 stars ended in __stack_chk_fail
// (bot run r2-l65, crash report pvz-sweep4-2026-11-23-183801). The list must hold the worst case of every pool.
TEST_CASE("the board render list holds every object every pool can hold") {
	pvzu::RenderPools p; // Board.cpp / effect holder pool sizes
	CHECK(pvzu::RenderItemsWorstCase(p) > 2048);
	CHECK(pvzu::kRenderItemCapacity >= pvzu::RenderItemsWorstCase(p));
	p.projectiles = 1024;
	CHECK(pvzu::RenderItemsWorstCase(p) < pvzu::RenderItemsWorstCase(pvzu::RenderPools{}));
}
