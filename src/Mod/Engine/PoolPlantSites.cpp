// PVZU: engine side of the Unnamed Pool plants. Rules: Mod/Unnamed/PoolPlants.h; evidence: the research notes (w3-pool)
// P5 and pvz-unnamed-pack/SOURCES-pool-plants.md.
#include "Mod/Engine/PoolPlantSites.h"

#include "Mod/Capacity.h"
#include "Mod/CmdLine.h"
#include "Mod/Unnamed/DayPlants.h"
#include "Mod/Unnamed/PoolPlants.h"

#include "LawnApp.h"
#include "Lawn/Board.h"
#include "Lawn/GridItem.h"
#include "Lawn/Plant.h"
#include "Lawn/Projectile.h"
#include "Lawn/Zombie.h"
#include "PvzpLib/EffectSystem.h"
#include "PvzpLib/Reanimator.h"

#include <cmath>
#include <cstdio>
#include <format>

namespace u = pvzu::unnamed;

namespace pvzu::engine {

void PoolLogPlant(const Plant* thePlant, std::string_view theText) {
	u::LogPlantAbility(thePlant->mBoard ? static_cast<int>(thePlant->mBoard->mMainCounter) : -1,
		std::format("{} col {} row {}: {}", GetPlantDefinition(thePlant->mSeedType).mPlantName, thePlant->mPlantCol, thePlant->mRow, theText));
}

void PoolJalapenoColumn(Plant* theJalapeno) {
	if (!u::JalapenoBurnsColumn())
		return;
	Board* aBoard = theJalapeno->mBoard;
	LawnApp* aApp = theJalapeno->mApp;
	const int aCol = theJalapeno->mPlantCol;

	// 0x95B98D: one Jalapeno fire per lane at (mX + 40, GridToPixelY(col, lane)), render order lane * 10000 + 307001
	const int aLanes = u::JalapenoColumnLanes(static_cast<int>(aBoard->mBackground));
	for (int aLane = 0; aLane < aLanes; ++aLane) {
		aApp->AddReanimation(static_cast<float>(theJalapeno->mX) + static_cast<float>(u::JalapenoColumnFireDx()),
			static_cast<float>(aBoard->GridToPixelY(aCol, aLane)),
			aLane * RenderLayer::RENDER_LAYER_ROW_OFFSET + RenderLayer::RENDER_LAYER_PARTICLE + 1, ReanimationType::REANIM_JALAPENO_FIRE);
	}

	// 0x95B9DC: zombies whose rect covers the column (any lane): burn, then thaw; a Zomboss iceball in the column dies
	const int aDamageRangeFlags = theJalapeno->GetDamageRangeFlags(PlantWeapon::WEAPON_PRIMARY);
	int aBurned = 0;
	for (Zombie* aZombie : aBoard->mZombies) {
		if (aZombie->mDead)
			continue;
		if (aZombie->mZombieType == ZombieType::ZOMBIE_BOSS && !aZombie->mIsFireBall) {
			Reanimation* aBall = aApp->ReanimationTryToGet(aZombie->mBossFireBallReanimID);
			if (aBall && static_cast<int>(std::nearbyint(aBall->mOverlayMatrix.m02 / 80.0f)) == aCol)
				aZombie->BossDestroyIceballInRow();
		}
		if (!aZombie->EffectedByDamage(static_cast<unsigned int>(aDamageRangeFlags)))
			continue;
		const Rect aRect = aZombie->GetZombieRect();
		if (!u::JalapenoColumnHits(aRect.mX, aRect.mWidth, aCol))
			continue;
		aZombie->ApplyBurn();
		aZombie->RemoveColdEffects();
		++aBurned;
	}

	// 0x95BA86: every row's ice trail starts no further left than mX + 80
	for (int aRow = 0; aRow < MAX_GRID_SIZE_Y; ++aRow)
		aBoard->mIceMinX[aRow] = std::max(aBoard->mIceMinX[aRow], theJalapeno->mX + u::JalapenoIceCutDx());

	// 0x95BAA9: ladders in the column
	int aLadders = 0;
	for (GridItem* aItem : aBoard->mGridItems) {
		if (aItem->mDead)
			continue;
		if (aItem->mGridItemType == GridItemType::GRIDITEM_LADDER && aItem->mGridX == aCol) {
			aItem->GridItemDie();
			++aLadders;
		}
	}
	PoolLogPlant(theJalapeno, std::format("jalapeno.column lanes {} burned {} ladders {} iceMinX>= {}", aLanes, aBurned, aLadders,
		theJalapeno->mX + u::JalapenoIceCutDx()));
}

void PoolJalapenoBarrage(Plant* theJalapeno) {
	const auto aShots = u::JalapenoBarrage();
	if (aShots.empty())
		return;
	Board* aBoard = theJalapeno->mBoard;
	const int aConversions = u::JalapenoBarrageConversions();
	int aTier = 0;
	// PVZU: never fill the pools (a full DataArray overruns in release builds; many barrages on 5-10 crashed)
	const auto& aReanims = gEffectSystem->mReanimationHolder->mReanimations;
	const int aFit = pvzu::ShotsThatFit(static_cast<int>(aShots.size()),
		static_cast<int>(aBoard->mProjectiles.mMaxSize - aBoard->mProjectiles.mSize),
		static_cast<int>(aReanims.mMaxSize - aReanims.mSize), 64);
	if (aFit < static_cast<int>(aShots.size()))
		PoolLogPlant(theJalapeno, std::format("jalapeno.barrage cut to {} of {} (pools nearly full)", aFit, aShots.size()));
	if (aFit == 0)
		return;
	for (int aIndex = 0; aIndex < aFit; ++aIndex) {
		const auto& aShot = aShots[static_cast<size_t>(aIndex)];
		Projectile* aProjectile = aBoard->AddProjectile(theJalapeno->mX + aShot.dx, theJalapeno->mY + aShot.dy,
			u::JalapenoBarrageRenderOrder(), theJalapeno->mRow, ProjectileType::PROJECTILE_PEA);
		aProjectile->mDamageRangeFlags = u::JalapenoBarrageRangeFlags();
		aProjectile->mMotionType = static_cast<ProjectileMotion>(u::JalapenoBarrageMotion());
		aProjectile->mVelX = aShot.vx;
		aProjectile->mVelY = aShot.vy;
		for (int aGridX = 0; aGridX < aConversions; ++aGridX) // 0x956046 / 0x956053: ConvertToFireball(0), then (1)
			aProjectile->ConvertToFireball(aGridX);
		aTier = aProjectile->mPvzu.fireTier;
	}
	PoolLogPlant(theJalapeno, std::format("jalapeno.barrage {} fireballs tier {} from x {} / {}", aShots.size(), aTier,
		theJalapeno->mX + aShots.front().dx, theJalapeno->mX + aShots.back().dx));
}

void PoolJalapenoIceShroom(Plant* theJalapeno) {
	if (!u::PoolPlantsOn() || !u::JalapenoIceEligible(theJalapeno->mPlantHealth, theJalapeno->mPlantMaxHealth))
		return;
	const int aRoll = Sexy::Rand(u::JalapenoIceRollRange());
	if (!u::JalapenoLeavesIce(aRoll)) {
		PoolLogPlant(theJalapeno, std::format("jalapeno.noIce roll {}", aRoll));
		return;
	}
	Plant* aIce = theJalapeno->mBoard->AddPlant(theJalapeno->mPlantCol, theJalapeno->mRow,
		static_cast<SeedType>(u::JalapenoIceSeed()), SeedType::SEED_NONE);
	PoolLogPlant(theJalapeno, std::format("jalapeno.iceShroom roll {} (<= 17)", aRoll));
	if (aIce)
		aIce->DoSpecial(); // freezes every zombie on the lawn at once, then dies
}

// Dev harness ------------------------------------------------------------------------------------------

namespace {
Plant* Put(Board* theBoard, int theCol, int theRow, SeedType theSeed) {
	if (theBoard->GetTopPlantAt(theCol, theRow, PlantPriority::TOPPLANT_ANY))
		return nullptr;
	return theBoard->AddPlant(theCol, theRow, theSeed, SeedType::SEED_NONE);
}

// Tangle Kelp goes through Board::CanPlantAt like a player's planting, so the log shows the placement rule.
void PutTangle(Board* theBoard, int theCol, int theRow) {
	if (theBoard->GetTopPlantAt(theCol, theRow, PlantPriority::TOPPLANT_ANY))
		return;
	const PlantingReason aReason = theBoard->CanPlantAt(theCol, theRow, SeedType::SEED_TANGLEKELP);
	std::fprintf(stderr, "PVZU dev: pool plant-test TANGLE_KELP col %d row %d %s: CanPlantAt %d%s\n", theCol, theRow,
		theBoard->IsPoolSquare(theCol, theRow) ? "water" : "land", static_cast<int>(aReason),
		aReason == PlantingReason::PLANTING_OK ? " (OK)" : "");
	if (aReason == PlantingReason::PLANTING_OK)
		theBoard->AddPlant(theCol, theRow, SeedType::SEED_TANGLEKELP, SeedType::SEED_NONE);
}

void LayOutPool(Board* theBoard) {
	// row 0: Snow Pea + three Torchwoods (snow -> green 3 -> purple 4 -> red 5) and a Tangle Kelp on grass at the end
	Put(theBoard, 0, 0, SeedType::SEED_SNOWPEA);
	// row 1: Peashooter + Threepeater (rows 0..2) + three Torchwoods (pea -> fire 1 -> blue 2 -> red 5)
	Put(theBoard, 0, 1, SeedType::SEED_PEASHOOTER);
	Put(theBoard, 1, 1, SeedType::SEED_THREEPEATER);
	for (int aRow = 0; aRow < 2; ++aRow)
		for (int aCol = 2; aCol < 5; ++aCol)
			Put(theBoard, aCol, aRow, SeedType::SEED_TORCHWOOD);
	PutTangle(theBoard, 8, 0);
	// row 2 (water): a Tangle Kelp for three Ducky Tube zombies
	PutTangle(theBoard, 6, 2);
	// row 4: a Tall-nut for chewers and one Gargantuar
	Put(theBoard, 5, 4, SeedType::SEED_TALLNUT);
	// row 5: Spikeweeds, a Squash behind them
	for (int aCol = 5; aCol < 8; ++aCol)
		Put(theBoard, aCol, 5, SeedType::SEED_SPIKEWEED);
	Put(theBoard, 3, 5, SeedType::SEED_SQUASH);
	std::fprintf(stderr, "PVZU dev: pool plant-test layout placed\n");
}
bool gPoolLaidOut = false;
}

bool PoolPlantTestActive(Board* theBoard) {
	return CmdFlag("-pvzu-plant-test-pool") || (CmdFlag("-pvzu-plant-test") && theBoard->StageHasPool());
}

void PoolPlantTestTick(Board* theBoard, int thePlayTicks) {
	if (!gPoolLaidOut) {
		gPoolLaidOut = true;
		LayOutPool(theBoard);
	}
	if (thePlayTicks % 450 == 0) { // Torchwood rows, the Spikeweed row
		theBoard->AddZombieInRow(ZombieType::ZOMBIE_PAIL, 0, 0);
		theBoard->AddZombieInRow(ZombieType::ZOMBIE_PAIL, 1, 0);
		theBoard->AddZombieInRow(ZombieType::ZOMBIE_TRAFFIC_CONE, 5, 0);
	}
	if (thePlayTicks % 350 == 0) // Tangle Kelp in the water
		theBoard->AddZombieInRow(ZombieType::ZOMBIE_DUCKY_TUBE, 2, 0);
	if (thePlayTicks % 700 == 0) // Tall-nut chewers
		theBoard->AddZombieInRow(ZombieType::ZOMBIE_PAIL, 4, 0);
	if (thePlayTicks == 1200) // the Tall-nut takes three smashes and breaks on the fourth
		theBoard->AddZombieInRow(ZombieType::ZOMBIE_GARGANTUAR, 4, 0);
	// Jalapenos on their own fuse (column + barrage + 18% Ice-shroom) only from tick 8000: their fire and barrage
	// clear rows 3..5, so the Tall-nut, Spikeweed and Squash checks run first
	static const int aJalapenoFrom = CmdOptionInt("-pvzu-dev-pool-jalapeno-from", 8000); // earlier for screenshots
	if (thePlayTicks >= aJalapenoFrom && thePlayTicks % 1100 == 0)
		if (Plant* aJalapeno = Put(theBoard, 8, 4 + (thePlayTicks / 1100) % 2, SeedType::SEED_JALAPENO))
			PoolLogPlant(aJalapeno, "dev.planted");
	if (thePlayTicks % 1100 == 0) { // refills
		Put(theBoard, 3, 5, SeedType::SEED_SQUASH);
		PutTangle(theBoard, 6, 2);
		PutTangle(theBoard, 8, 0);
	}
	if (thePlayTicks % 3000 == 0) // a new Tall-nut once the old one broke
		Put(theBoard, 5, 4, SeedType::SEED_TALLNUT);
}

} // namespace pvzu::engine
