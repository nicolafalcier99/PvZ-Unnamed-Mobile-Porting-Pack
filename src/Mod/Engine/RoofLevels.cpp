#include "Mod/Engine/RoofLevels.h"

#include "Mod/CmdLine.h"
#include "Mod/Engine/DevLevels.h"
#include "Mod/GameData.h"
#include "Mod/Mode.h"
#include "Mod/Unnamed/DayZombies.h"
#include "Mod/Unnamed/RoofLevels.h"

#include "LawnApp.h"
#include "Lawn/Board.h"
#include "Lawn/GridItem.h"
#include "Lawn/Plant.h"

#include <format>
#include <optional>
#include <string>

namespace pvzu::engine {

namespace u = pvzu::unnamed;

namespace {
std::string Name(int theType) {
	const auto& aZombies = GameData::Get().Active().zombies;
	return theType >= 0 && theType < static_cast<int>(aZombies.size()) ? aZombies[theType].name : std::format("#{}", theType);
}
const char* ModeName() { return CurrentMode() == GameMode::Unnamed ? "unnamed" : "vanilla"; }
std::string LevelName(const Board* theBoard) {
	return std::format("{}-{}", (theBoard->mLevel - 1) / 10 + 1, (theBoard->mLevel - 1) % 10 + 1);
}
bool RoofOn() { return GameData::Get().Feature("roofLevels"); }
int Graves(Board* theBoard) {
	int aCount = 0;
	for (GridItem* aGridItem : theBoard->mGridItems)
		aCount += !aGridItem->mDead && aGridItem->mGridItemType == GridItemType::GRIDITEM_GRAVESTONE ? 1 : 0;
	return aCount;
}
}

void RoofSkyDropCounts(Board* theBoard, int& theCount, int& thePoints) {
	u::SkyDrop aDrop;
	if (!u::RoofFinalWaveSkyDrop(theBoard->mLevel, aDrop))
		return;
	theCount = aDrop.count;
	thePoints = aDrop.points;
}

void RoofSkyDropLog(Board* theBoard, int theCount, int thePoints) {
	DevLog(std::format("skyDrop mode={} level={} wave={}/{} count={} points={} graves={} iceTrap={}", ModeName(), LevelName(theBoard),
		theBoard->mCurrentWave, theBoard->mNumWaves, theCount, thePoints, Graves(theBoard), theBoard->mIceTrapCounter));
}

void RoofSkyDropped(Board* theBoard, ZombieType theType) {
	DevLog(std::format("skyDropped mode={} level={} type={}", ModeName(), LevelName(theBoard), Name(theType)));
}

void DevLogRoof(Board* theBoard) {
	if (!CmdOption("-pvzu-log-waves") || !theBoard->StageHasRoof())
		return;
	const bool aOn = RoofOn();
	u::SkyDrop aDrop;
	const bool aRow = aOn && u::RoofFinalWaveSkyDrop(theBoard->mLevel, aDrop);
	const int aTypeRoll = GameData::Get().Feature("dayZombies") ? u::SkyDropTypeRollRange() : 0;
	DevLog(std::format("roof mode={} level={} feature={} skyDrop={} skyTypeRoll={} bungeeBlitz={} conveyor={}", ModeName(),
		LevelName(theBoard), aOn ? 1 : 0, aRow ? std::format("{}x{}pts", aDrop.count, aDrop.points) : std::string("engine"),
		aTypeRoll > 0 ? std::format("Rand({})->13types", aTypeRoll) : std::string("engine"), theBoard->mApp->IsBungeeBlitzLevel() ? 1 : 0,
		theBoard->HasConveyorBeltSeedBank() ? 1 : 0));
}

void RoofDevTick(Board* theBoard, int thePlayTicks) {
	if (!theBoard->StageHasRoof())
		return;
	if (thePlayTicks == 1) {
		std::string aPots;
		int aPotCount = 0;
		for (Plant* aPlant : theBoard->mPlants) {
			if (aPlant->mDead || aPlant->mSeedType != SeedType::SEED_FLOWERPOT)
				continue;
			aPotCount++;
			aPots += std::format("{}{}:{}", aPots.empty() ? "" : ",", aPlant->mPlantCol, aPlant->mRow);
		}
		DevLog(std::format("roofPlay mode={} level={} startSun={} awardSeed={} pots={} potCells={} conveyor={}", ModeName(),
			LevelName(theBoard), theBoard->mSunMoney, static_cast<int>(LawnApp::GetAwardSeedForLevel(theBoard->mLevel)), aPotCount,
			aPots, theBoard->HasConveyorBeltSeedBank() ? 1 : 0));
	}
	static const std::optional<std::string> aScenario = CmdOption("-pvzu-dev-roof");
	static const int aAt = CmdOptionInt("-pvzu-dev-roof-at", 50);
	if (!aScenario || thePlayTicks != aAt)
		return;
	DevLog(std::format("devRoof {} mode={} level={} wave={}/{}", *aScenario, ModeName(), LevelName(theBoard), theBoard->mCurrentWave,
		theBoard->mNumWaves));
	if (*aScenario == "final-grave") {
		if (GridItem* aGrave = theBoard->AddAGraveStone(6, 0)) {
			aGrave->mGridItemCounter = 100; // dev: fully risen, so the grave loop may use it now
			DevLog(std::format("devRoof grave col={} row={}", aGrave->mGridX, aGrave->mGridY));
		}
	}
	if (*aScenario == "final" || *aScenario == "final-grave") {
		const size_t aBefore = theBoard->mZombies.mSize;
		theBoard->SpawnZombiesFromGraves();
		DevLog(std::format("devRoof spawned={} graves={}", static_cast<int>(theBoard->mZombies.mSize - aBefore), Graves(theBoard)));
	}
}

} // namespace pvzu::engine
