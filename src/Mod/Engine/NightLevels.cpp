#include "Mod/Engine/NightLevels.h"

#include "Mod/CmdLine.h"
#include "Mod/Engine/DevLevels.h"
#include "Mod/GameData.h"
#include "Mod/Mode.h"
#include "Mod/Unnamed/NightLevels.h"

#include "LawnApp.h"
#include "Lawn/Board.h"
#include "Lawn/Cutscene.h"
#include "Lawn/GridItem.h"
#include "Lawn/Widget/SeedChooserScreen.h"
#include "Lawn/Zombie.h"
#include "Common.h"

#include <format>
#include <string>

namespace pvzu::engine {

namespace u = pvzu::unnamed;

namespace {
struct RiseState {
	bool active = false;
	ZombieType forced = ZombieType::ZOMBIE_INVALID;
	bool mindControlled = false;
	const char* reason = "";
};
RiseState gRise;
// GraveRiseScope saves the state it replaces here, one slot per nesting level (a risen gold Football cannot die
// inside the call that raised it, so the depth stays tiny; the cap only guards against a runaway).
constexpr int kMaxDepth = 8;
RiseState gSaved[kMaxDepth];
int gDepth = 0;

std::string Name(int theType) {
	const auto& aZombies = GameData::Get().Active().zombies;
	return theType >= 0 && theType < static_cast<int>(aZombies.size()) ? aZombies[theType].name : std::format("#{}", theType);
}
const char* ModeName() { return CurrentMode() == GameMode::Unnamed ? "unnamed" : "vanilla"; }
std::string LevelName(const Board* theBoard) {
	return std::format("{}-{}", (theBoard->mLevel - 1) / 10 + 1, (theBoard->mLevel - 1) % 10 + 1);
}
int GraveCount(Board* theBoard) {
	int aCount = 0;
	for (GridItem* aGridItem : theBoard->mGridItems)
		if (!aGridItem->mDead && aGridItem->mGridItemType == GridItemType::GRIDITEM_GRAVESTONE)
			aCount++;
	return aCount;
}
}

bool NightPlaceStartGraves(Board* theBoard, Sexy::MTRand& theLevelRNG) {
	u::GraveLayout aLayout{};
	if (!u::NightStartGraves(theBoard->mLevel, aLayout))
		return false;
	for (int aCol = 0; aCol < static_cast<int>(aLayout.size()); aCol++) // ascending columns, as the exe and vanilla
		if (aLayout[aCol] > 0)
			theBoard->AddGraveStones(aCol, aLayout[aCol], theLevelRNG);
	return true;
}

bool NightAllowsZombie(ZombieType theZombieType, int theLevel) {
	return u::NightAllowExtra(static_cast<int>(theZombieType), theLevel);
}

void NightOnZombieDied(Zombie* theZombie) {
	Board* aBoard = theZombie->mBoard;
	// Ruling: only during play (a zombie removed by a cut-scene, the almanac or a stage reset raises nothing).
	if (aBoard == nullptr || theZombie->mApp->mGameScene != GameScenes::SCENE_PLAYING)
		return;
	if (!u::GoldFootballRaisesGraves(static_cast<int>(theZombie->mZombieType), theZombie->mPvzu.variant, theZombie->mFromWave))
		return;
	DevLog(std::format("goldFootball death row={} x={} graves={}", theZombie->mRow, theZombie->mX, GraveCount(aBoard)));
	GraveRiseScope aScope(static_cast<ZombieType>(u::GoldFootballGraveType()), false, "goldFootball");
	aBoard->SpawnZombiesFromGraves();
}

void NightOnBackupSummoned(Zombie* theDancer) {
	const u::CarnivalStep aStep = u::CarnivalOnBackupSummoned(static_cast<int>(theDancer->mZombieType), theDancer->mPvzu.variant,
		theDancer->mPvzu.counter);
	const bool aCounts = aStep.raise || aStep.counter != theDancer->mPvzu.counter;
	theDancer->mPvzu.counter = aStep.counter;
	if (aCounts)
		DevLog(std::format("carnival backup row={} counter={} raise={}", theDancer->mRow, aStep.counter, aStep.raise ? 1 : 0));
	if (!aStep.raise || theDancer->mBoard == nullptr)
		return;
	const bool aHypno = u::CarnivalPassesHypnosis() && theDancer->mMindControlled;
	DevLog(std::format("carnival raise row={} hypno={} graves={}", theDancer->mRow, aHypno ? 1 : 0, GraveCount(theDancer->mBoard)));
	GraveRiseScope aScope(ZombieType::ZOMBIE_INVALID, aHypno, "carnival");
	theDancer->mBoard->SpawnZombiesFromGraves();
}

GraveRiseScope::GraveRiseScope(ZombieType theForcedType, bool theMindControlled, const char* theReason) {
	if (gDepth < kMaxDepth)
		gSaved[gDepth] = gRise;
	gDepth++;
	gRise = {true, theForcedType, theMindControlled, theReason};
}

GraveRiseScope::~GraveRiseScope() {
	gDepth--;
	gRise = gDepth < kMaxDepth ? gSaved[gDepth] : RiseState{};
}

bool GraveRiseGravesOnly() {
	return gRise.active && gRise.forced != ZombieType::ZOMBIE_INVALID;
}

bool GraveRiseForcedType(ZombieType& theType) {
	if (!gRise.active || gRise.forced == ZombieType::ZOMBIE_INVALID)
		return false;
	theType = gRise.forced;
	return true;
}

bool GraveRiseHypnotises() {
	return gRise.active && gRise.mindControlled;
}

void GraveRiseRisen(Zombie* theZombie) {
	if (gRise.active && gRise.mindControlled && !theZombie->mMindControlled)
		theZombie->StartMindControlled();
	if (gRise.active)
		DevLog(std::format("graveRisen by={} type={} row={} col={} hypno={}", gRise.reason, Name(theZombie->mZombieType), theZombie->mRow,
			theZombie->mX / 80, theZombie->mMindControlled ? 1 : 0));
}

void DevLogNight(Board* theBoard) {
	if (!CmdOption("-pvzu-log-waves"))
		return;
	std::string aCells;
	int aCount = 0;
	int aPerCol[MAX_GRID_SIZE_X] = {};
	for (GridItem* aGridItem : theBoard->mGridItems) {
		if (aGridItem->mDead || aGridItem->mGridItemType != GridItemType::GRIDITEM_GRAVESTONE)
			continue;
		aCount++;
		if (aGridItem->mGridX >= 0 && aGridItem->mGridX < MAX_GRID_SIZE_X)
			aPerCol[aGridItem->mGridX]++;
		aCells += std::format("{}{}:{}", aCells.empty() ? "" : ",", aGridItem->mGridX, aGridItem->mGridY);
	}
	std::string aCols;
	for (int aCol = 0; aCol < MAX_GRID_SIZE_X; aCol++)
		if (aPerCol[aCol] > 0)
			aCols += std::format("{}{}:{}", aCols.empty() ? "" : " ", aCol, aPerCol[aCol]);
	DevLog(std::format("graves mode={} level={} count={} perCol={} cells={}", ModeName(), LevelName(theBoard), aCount, aCols, aCells));
	std::string aAllowed;
	for (int aType = 0; aType < ZombieType::NUM_ZOMBIE_TYPES; aType++)
		if (theBoard->mZombieAllowed[aType])
			aAllowed += (aAllowed.empty() ? "" : ",") + Name(aType);
	DevLog(std::format("allowed mode={} level={} zombies={}", ModeName(), LevelName(theBoard), aAllowed));
}

void NightDevBeforePlay(Board* theBoard) {
	static bool aDone = false;
	static int aTicks = 0;
	static int aChooserTicks = 0;
	if (aDone || !(CmdOption("-pvzu-dev-night") || CmdOption("-pvzu-dev-pool") || CmdOption("-pvzu-dev-fog") || CmdOption("-pvzu-dev-roof"))) // Pool, Fog and Roof dev runs too
		return;
	if (theBoard->mCutScene && theBoard->mCutScene->IsShowingCrazyDave()) { // Dave's intro talk: one "click" every 50 ticks
		if (++aTicks % 50 == 0)
			theBoard->mCutScene->AdvanceCrazyDaveDialog(false);
		return;
	}
	SeedChooserScreen* aChooser = theBoard->mApp->mSeedChooserScreen.get();
	if (aChooser == nullptr || aChooser->mChooseState != SeedChooserState::CHOOSE_NORMAL || aChooser->mSeedsInFlight > 0 ||
		!aChooser->mMouseVisible)
		return;
	if (++aChooserTicks < 100) // own count: mSeedChooserAge is never initialised (SeedChooserScreen.cpp), so it can stay negative
		return;
	aDone = true;
	DevLog("devNight seed chooser: random seeds, start");
	aChooser->PickRandomSeeds();
}

void NightDevTick(Board* theBoard, int thePlayTicks) {
	static const std::optional<std::string> aScenario = CmdOption("-pvzu-dev-night");
	if (!aScenario)
		return;
	static const int aAt = CmdOptionInt("-pvzu-dev-night-at", 50);
	static ZombieID aDevZombie = ZombieID::ZOMBIEID_NULL;
	if (thePlayTicks == aAt) {
		DevLog(std::format("devNight {} mode={} level={} graves={}", *aScenario, ModeName(), LevelName(theBoard), GraveCount(theBoard)));
		if (*aScenario == "rise") {
			GraveRiseScope aScope(ZombieType::ZOMBIE_INVALID, false, "dev");
			theBoard->SpawnZombiesFromGraves();
		} else if (*aScenario == "gold" || *aScenario == "carnival" || *aScenario == "carnival-hypno") {
			const bool aGold = *aScenario == "gold";
			Zombie* aZombie = theBoard->AddZombieInRow(aGold ? ZombieType::ZOMBIE_FOOTBALL : ZombieType::ZOMBIE_DANCER, 2, 0);
			if (aZombie && aGold)
				aZombie->mPvzu.variant = 0x102; // dev: the gold form (Plan 3 Task 2 rolls it; forced here)
			if (aZombie && *aScenario == "carnival-hypno") { // a hypnotised Dancer mid-lawn, walking right
				aZombie->mPosX = 200.0f;
				aZombie->mX = 200;
				aZombie->StartMindControlled();
			}
			aDevZombie = aZombie ? theBoard->ZombieGetID(aZombie) : ZombieID::ZOMBIEID_NULL;
		}
		return;
	}
	Zombie* aZombie = theBoard->ZombieTryToGet(aDevZombie);
	if (aZombie == nullptr || thePlayTicks < aAt)
		return;
	if (*aScenario == "gold" && thePlayTicks == aAt + 100) {
		DevLog(std::format("devNight gold kill variant={}", aZombie->mPvzu.variant));
		aZombie->DieNoLoot();
	} else if (*aScenario == "carnival" || *aScenario == "carnival-hypno") {
		for (ZombieID aID : aZombie->mFollowerZombieID) // let it summon again as soon as its backups have risen
			if (Zombie* aBackup = theBoard->ZombieTryToGet(aID); aBackup && aBackup->mZombiePhase != ZombiePhase::PHASE_DANCER_RISING)
				aBackup->DieNoLoot();
	}
}

} // namespace pvzu::engine
