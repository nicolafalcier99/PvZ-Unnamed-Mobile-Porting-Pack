#include "Mod/Engine/HiddenLevels.h"

#include "Mod/Engine/DevLevels.h"
#include "Mod/Engine/ModPlantsShadow.h"
#include "Mod/Engine/SpecialLevelHooks.h"
#include "Mod/Unnamed/SpecialLevels.h"

#include "LawnApp.h"
#include "Lawn/Board.h"
#include "Lawn/Plant.h"
#include "Lawn/Zombie.h"

#include <format>

namespace pvzu::engine {

namespace u = pvzu::unnamed;

namespace {
int gLastAutoPlant = -1;
} // namespace

void HiddenInit(Board*) { gLastAutoPlant = -1; }

void HiddenUpdate(Board* theBoard) {
	// hook 93 (0x95c140) counts its own Board+0x57cc at the top of Board::UpdateSunSpawning, which runs once per play
	// tick right after mMainCounter++ (Board::UpdateGame): the same count, and mMainCounter is saved with the game
	const int aCounter = theBoard->mMainCounter;
	if (aCounter == gLastAutoPlant || !u::AutoPlantDue(u::SpecialLevel::Hidden1, aCounter))
		return;
	gLastAutoPlant = aCounter;
	const u::AutoPlant p = u::HiddenAutoPlant();
	if (p.seed < 0)
		return;
	// Board::AddPlant(row EAX = -1, column 7, Blover): above the top row, out of every zombie's reach; it blows away
	// flying zombies (and fog) as a planted Blover does
	Plant* aPlant = theBoard->AddPlant(p.col, p.row, static_cast<SeedType>(p.seed), SeedType::SEED_NONE);
	DevLog(std::format("special hidden1 t={} autoplant seed={} col={} row={} ok={}", aCounter, p.seed, p.col, p.row,
		aPlant ? 1 : 0));
}

void HiddenZombieSpawned(Zombie* theZombie) {
	if (theZombie->mHelmHealth <= 0 && theZombie->mHelmMaxHealth <= 0)
		return;
	theZombie->mHelmHealth = u::SpecialHelmHp(u::SpecialLevel::Hidden1, theZombie->mHelmHealth);
	theZombie->mHelmMaxHealth = u::SpecialHelmHp(u::SpecialLevel::Hidden1, theZombie->mHelmMaxHealth);
	DevLog(std::format("special hidden1 t={} helm type={} helm={}/{}", theZombie->mBoard->mMainCounter,
		static_cast<int>(theZombie->mZombieType), theZombie->mHelmHealth, theZombie->mHelmMaxHealth));
}

void HiddenPlantTick(Plant* thePlant) {
	Board* aBoard = thePlant->mBoard;
	if (!thePlant->mIsOnBoard || aBoard == nullptr || ActiveSpecial(aBoard) != u::SpecialLevel::Hidden2)
		return;
	if (!u::PlantDrainDue(u::SpecialLevel::Hidden2, aBoard->mMainCounter))
		return;
	const int aSeed = thePlant->mPvzu.modSeed != 0 ? thePlant->mPvzu.modSeed : static_cast<int>(thePlant->mSeedType);
	if (u::PlantDrainSkips(aSeed, static_cast<int>(thePlant->mState), thePlant->mIsAsleep, aBoard->mApp->IsIZombieLevel()))
		return;
	thePlant->mPlantHealth -= ModPlantBitten(nullptr, thePlant, u::PlantDrainDamage()); // Charmed Tall-nut x1.5 / summons
	if (thePlant->mPlantHealth < 0) {
		DevLog(std::format("special hidden2 t={} drain killed seed={} col={} row={}", aBoard->mMainCounter, aSeed,
			thePlant->mPlantCol, thePlant->mRow));
		thePlant->Die();
	} else if (aBoard->mMainCounter % 600 == 1) {
		DevLog(std::format("special hidden2 t={} drain seed={} col={} row={} hp={}", aBoard->mMainCounter, aSeed,
			thePlant->mPlantCol, thePlant->mRow, thePlant->mPlantHealth));
	}
}

} // namespace pvzu::engine
