#include "Mod/Unnamed/DayZombies.h"
#include "Mod/Engine/DayLevels.h"

#include "Mod/Engine/DevLevels.h"
#include "Mod/Engine/VasePole.h"
#include "Mod/GameData.h"
#include "Mod/Unnamed/Levels.h"
#include "Mod/Unnamed/State.h"

#include "LawnApp.h"
#include "Lawn/Board.h"
#include "Lawn/Challenge.h"
#include "Lawn/Plant.h"
#include "Lawn/Zombie.h"
#include "Common.h"

#include <format>
#include <string>

namespace pvzu::engine {

namespace {
std::string Name(int theType) {
	const auto& aZombies = GameData::Get().Active().zombies;
	return theType >= 0 && theType < static_cast<int>(aZombies.size()) ? aZombies[theType].name : std::format("#{}", theType);
}
std::string SeedName(int theSeed) {
	const auto& aPlants = GameData::Get().Active().plants;
	return theSeed >= 0 && theSeed < static_cast<int>(aPlants.size()) ? aPlants[theSeed].name : std::format("#{}", theSeed);
}
}

bool UnnamedPoleVault(Zombie* theZombie, Plant* theTarget) {
	unnamed::VaultInput aIn;
	aIn.variant = theZombie->mPvzu.variant;
	aIn.hasPlant = theTarget != nullptr;
	aIn.plantSeed = theTarget ? static_cast<int>(theTarget->mSeedType) : -1;
	aIn.plantState = theTarget ? static_cast<int>(theTarget->mState) : 0;
	aIn.hypnotised = theZombie->mMindControlled;
	aIn.graveMade = (theZombie->mPvzu.flags & unnamed::kZombieFlagVaultGraveMade) != 0;
	unnamed::VaultDecision aDecision = unnamed::DecideVault(aIn);
	if (aDecision.grave && !unnamed::PoleMakesGraveOnVault(aIn.variant)) // PVZU merge: glove/vase-pole forms leave no grave (Task 4)
		aDecision.grave = false;

	const std::string aTarget = theTarget ? std::format("{} c{}", SeedName(aIn.plantSeed), theTarget->mPlantCol) : std::string("none");
	if (aDecision.bonk)
		DevLog(std::format("vault bonk row={} mX={} target={}", theZombie->mRow, theZombie->mX, aTarget));
	if (aDecision.vase && GameData::Get().Feature("vasePole")) // PVZU Plan 9 R2: the vase pole's vault (0xc036e0)
		VasePoleVault(theZombie);
	if (!aDecision.grave)
		return aDecision.bonk;

	theZombie->mPvzu.flags |= unnamed::kZombieFlagVaultGraveMade;
	Board* aBoard = theZombie->mBoard;
	const int aCol = unnamed::VaultGraveColumn(theZombie->mX);
	const int aRow = theZombie->mRow;
	// Ruling: a column off the lawn makes no grave (the exe would write a grid item at col -1).
	const bool aOnLawn = aCol >= 0 && aCol < MAX_GRID_SIZE_X && aRow >= 0 && aRow < MAX_GRID_SIZE_Y;
	if (aOnLawn)
		aBoard->mChallenge->GraveDangerSpawnGraveAt(aCol, aRow);
	DevLog(std::format("vault grave row={} mX={} col={} target={} made={}", aRow, theZombie->mX, aCol, aTarget, aOnLawn ? 1 : 0));
	if (DevFlag("-pvzu-dev-rise") && aBoard->mRiseFromGraveCounter == 0)
		aBoard->mRiseFromGraveCounter = 300; // dev: raise zombies from the graves in 3 s
	return aDecision.bonk;
}

ZombieType UnnamedGraveRiseType(Board* theBoard, ZombieType theVanillaType) {
	(void)theBoard;
	const int aType = unnamed::GraveRiseType(static_cast<int>(theVanillaType), [](int theRange) { return Sexy::Rand(theRange); });
	DevLog(std::format("graveRise vanilla={} type={}", Name(theVanillaType), Name(aType)));
	return static_cast<ZombieType>(aType);
}

void UnnamedGraveRisePhase(Zombie* theZombie) {
	const int aPhase = unnamed::GraveRisePhase(static_cast<int>(theZombie->mZombieType), static_cast<int>(theZombie->mZombiePhase));
	theZombie->mZombiePhase = static_cast<ZombiePhase>(aPhase);
	if (theZombie->mZombiePhase == ZombiePhase::PHASE_JACK_IN_THE_BOX_RUNNING)
		theZombie->mPhaseCounter = unnamed::JackRiseFuse(Sexy::Rand(unnamed::JackRiseFuseRange()));
	DevLog(std::format("graveRisen type={} phase={} counter={}", Name(theZombie->mZombieType), aPhase, theZombie->mPhaseCounter));
}

bool UnnamedTooToughToAsh(const Zombie* theZombie) {
	const int aHelm = theZombie->mHelmType != HelmType::HELMTYPE_NONE ? theZombie->mHelmHealth : 0;
	const int aShield = theZombie->mShieldType != ShieldType::SHIELDTYPE_NONE ? theZombie->mShieldHealth : 0;
	return unnamed::TooToughToAsh(theZombie->mBodyHealth, aHelm, aShield, true);
}

} // namespace pvzu::engine
