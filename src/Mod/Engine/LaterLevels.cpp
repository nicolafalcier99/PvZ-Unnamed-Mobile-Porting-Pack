#include "Mod/Engine/LaterLevels.h"

#include "Mod/CmdLine.h"
#include "Mod/Engine/DevLevels.h"
#include "Mod/GameData.h"
#include "Mod/Mode.h"

#include "GameConstants.h"
#include "LawnApp.h"
#include "Lawn/Board.h"
#include "Lawn/GridItem.h"
#include "Lawn/Plant.h"
#include "Lawn/System/PlayerInfo.h"
#include "PvzpLib/PvzpStringFile.h"
#include "widget/Dialog.h"
#include "widget/WidgetManager.h"

#include <algorithm>
#include <array>
#include <format>
#include <optional>
#include <string>
#include <vector>

namespace pvzu::engine {

namespace u = pvzu::unnamed;

namespace {
const char* ModeName() { return CurrentMode() == GameMode::Unnamed ? "unnamed" : "vanilla"; }

// A row for this board: Adventure, feature on, level past the vanilla tables, row in the pack's table.
const u::LaterLevel* Row(const Board* theBoard) {
	if (!LaterLevelsOn() || !theBoard->mApp->IsAdventureMode() || !u::UsesLaterTable(theBoard->mLevel))
		return nullptr;
	return u::FindLaterLevel(theBoard->mLevel);
}

std::vector<std::string> EngineZombieNames() {
	std::vector<std::string> aNames;
	for (const auto& aZombie : GameData::Get().Vanilla().zombies)
		aNames.push_back(aZombie.name);
	return aNames;
}

std::string ZombieName(int theType) {
	const auto& aZombies = GameData::Get().Active().zombies;
	return theType >= 0 && theType < static_cast<int>(aZombies.size()) ? aZombies[theType].name : std::format("#{}", theType);
}

int Graves(const Board* theBoard) {
	int aCount = 0;
	for (GridItem* aGridItem : const_cast<Board*>(theBoard)->mGridItems)
		aCount += !aGridItem->mDead && aGridItem->mGridItemType == GridItemType::GRIDITEM_GRAVESTONE ? 1 : 0;
	return aCount;
}

// Dialog text: the pack's strings (strings/en-corrections-later-levels.txt) when loaded, else plain English.
std::string Text(const char* theKey, const char* theFallback) {
	const std::string aKey = std::format("[{}]", theKey);
	return PvzpStringListExists(aKey) ? std::string(PvzpStringTranslate(aKey)) : std::string(theFallback);
}

u::LevelSkip gSkip;
int gHeldButton = -1;
int gHeldTicks = 0;
} // namespace

bool LaterLevelsOn() { return GameData::Get().Feature("laterLevels"); }

bool StageCountsAsRoof(Board* theBoard) {
	return theBoard->StageHasRoof() || (LaterLevelsOn() && u::BackgroundCountsAsRoof(static_cast<int>(theBoard->mBackground)));
}

bool LaterLevelsBackground(Board* theBoard) {
	const u::LaterLevel* aRow = Row(theBoard);
	if (aRow == nullptr || aRow->background < 0)
		return false;
	theBoard->mBackground = static_cast<BackgroundType>(aRow->background);
	return true;
}

bool LaterLevelsPlantRows(Board* theBoard) {
	if (!LaterLevelsOn() || !theBoard->mApp->IsAdventureMode())
		return false; // Ruling: hooks 26/27 also change the Zombiquarium minigame's rows; minigames are a later plan
	std::array<int, 6> aRows{};
	if (!u::PlantRowsFor(static_cast<int>(theBoard->mBackground), aRows))
		return false;
	for (int y = 0; y < MAX_GRID_SIZE_Y; y++)
		theBoard->mPlantRow[y] = static_cast<PlantRowType>(aRows[y]);
	return true;
}

bool LaterLevelsAllowed(Board* theBoard) {
	const u::LaterLevel* aRow = Row(theBoard);
	if (aRow == nullptr || aRow->allowed.empty())
		return false;
	std::string aErr;
	const std::vector<int> aTypes = u::ResolveZombies(aRow->allowed, EngineZombieNames(), &aErr);
	if (!aErr.empty())
		DevLog(std::format("laterLevels level={} allow list: {}", theBoard->mLevel, aErr));
	for (int aType : aTypes)
		if (aType >= 0 && aType < NUM_ZOMBIE_TYPES)
			theBoard->mZombieAllowed[aType] = true;
	return true;
}

void LaterLevelsFirstWaveCountdown(Board* theBoard) {
	const u::LaterLevel* aRow = Row(theBoard);
	if (aRow != nullptr && aRow->firstWaveCountdown > 0)
		theBoard->mZombieCountDown = aRow->firstWaveCountdown;
}

int LaterLevelsNumWaves(Board* theBoard, int theVanillaWaves) {
	const u::LaterLevel* aRow = Row(theBoard);
	return aRow != nullptr && aRow->waves > 0 ? std::min(aRow->waves, MAX_ZOMBIE_WAVES) : theVanillaWaves;
}

void LaterLevelsStartSun(Board* theBoard) {
	const u::LaterLevel* aRow = Row(theBoard);
	if (aRow != nullptr && aRow->startSun >= 0)
		theBoard->mSunMoney = aRow->startSun;
}

int LaterLevelsPotColumns(Board* theBoard, int theVanillaColumns) {
	const u::LaterLevel* aRow = Row(theBoard);
	return aRow != nullptr && aRow->freePotColumns >= 0 ? std::min(aRow->freePotColumns, MAX_GRID_SIZE_X) : theVanillaColumns;
}

bool LaterLevelsFogColumn(Board* theBoard, int& theColumn) {
	const u::LaterLevel* aRow = Row(theBoard);
	if (aRow == nullptr || aRow->fogColumn < 0)
		return false;
	theColumn = aRow->fogColumn;
	return true;
}

int LaterLevelsAllowIndex(int theLevel) {
	return LaterLevelsOn() ? u::AllowTableIndex(theLevel, NUM_LEVELS) : u::VanillaLevelIndex(theLevel, NUM_LEVELS);
}

int AdventureFinalLevel() { return LaterLevelsOn() ? u::FinalAdventureLevel(FINAL_LEVEL) : FINAL_LEVEL; }

int StageAreaMax(int theVanillaMax) {
	return LaterLevelsOn() ? std::max(theVanillaMax, u::StageArea(1000000, theVanillaMax)) : theVanillaMax;
}

u::SpecialLevel CurrentSpecialLevel(const Board* theBoard) {
	const u::LaterLevel* aRow = theBoard ? Row(theBoard) : nullptr;
	return aRow ? aRow->special : u::SpecialLevel::None;
}

// ---- Level skip ----

void LevelSkipButtonPress(int theButton) {
	gHeldButton = theButton;
	gHeldTicks = 0;
}

void LevelSkipButtonDownTick(int theButton) {
	if (theButton == gHeldButton)
		gHeldTicks++;
}

bool LevelSkipButtonDepress(LawnApp* theApp, u::SkipButton theButton, int theButtonId) {
	const int aHeld = theButtonId == gHeldButton ? gHeldTicks : 0;
	gHeldButton = -1;
	gHeldTicks = 0;
	if (!LaterLevelsOn() || theApp->mPlayerInfo == nullptr)
		return false;
	const int aClick = u::SkipClickFromPointer(theApp->mWidgetManager->mLastDownButtonId, aHeld);
	PlayerInfo* aPlayer = theApp->mPlayerInfo;
	const u::SkipAction aAction = gSkip.Press(theButton, aClick, aPlayer->mLevel, aPlayer->mFinishedAdventure);
	const std::string aHeader = Text("PVZU_LEVEL_SKIP_HEADER", "Level skip");
	switch (aAction.kind) {
	case u::SkipAction::PassThrough:
		return false;
	case u::SkipAction::Opened: {
		DevLog(std::format("levelSkip open click={} held={} target={}", aClick, aHeld, aAction.target));
		const std::string aBody = Text("PVZU_LEVEL_SKIP_ON", "Level skip mode is on.");
		theApp->LawnMessageBox(Dialogs::DIALOG_MESSAGE, aHeader.c_str(), aBody.c_str(), "[DIALOG_BUTTON_OK]", "", Dialog::BUTTONS_FOOTER);
		return true;
	}
	case u::SkipAction::Stepped:
		DevLog(std::format("levelSkip step click={} held={} target={}", aClick, aHeld, aAction.target));
		return true;
	case u::SkipAction::AskConfirm: {
		DevLog(std::format("levelSkip ask target={}", aAction.target));
		std::string aLine = Text("PVZU_LEVEL_SKIP_CONFIRM", "Skip to level {LEVEL}?");
		if (const size_t aAt = aLine.find("{LEVEL}"); aAt != std::string::npos)
			aLine.replace(aAt, 7, u::SkipTargetName(aAction.target));
		const int aResult = theApp->LawnMessageBox(Dialogs::DIALOG_MESSAGE, aHeader.c_str(), aLine.c_str(), "[DIALOG_BUTTON_OK]",
			"[DIALOG_BUTTON_NO]", Dialog::BUTTONS_YES_NO);
		if (aResult == Dialog::ID_YES) {
			aPlayer->mLevel = gSkip.Confirm(); // 0x7a1600: PlayerInfo+0x24 = temp (the menu sign shows it on the next visit)
			DevLog(std::format("levelSkip confirm level={}", aPlayer->mLevel));
		} else {
			gSkip.Decline();
			DevLog("levelSkip declined");
		}
		return true;
	}
	case u::SkipAction::Cancelled: {
		DevLog(std::format("levelSkip cancel click={}", aClick));
		const std::string aBody = Text("PVZU_LEVEL_SKIP_OFF", "Level skip cancelled.");
		theApp->LawnMessageBox(Dialogs::DIALOG_MESSAGE, aHeader.c_str(), aBody.c_str(), "[DIALOG_BUTTON_OK]", "", Dialog::BUTTONS_FOOTER);
		return true;
	}
	}
	return false;
}

// ---- Dev ----

void DevLogLaterLevel(Board* theBoard) {
	if (!CmdOption("-pvzu-log-waves") || !theBoard->mApp->IsAdventureMode() || theBoard->mLevel <= NUM_LEVELS)
		return;
	const u::LaterLevel* aRow = Row(theBoard);
	std::string aAllowed;
	for (int i = 0; i < NUM_ZOMBIE_TYPES; i++)
		if (theBoard->mZombieAllowed[i])
			aAllowed += (aAllowed.empty() ? "" : ",") + ZombieName(i);
	DevLog(std::format("laterLevel mode={} level={} feature={} row={} bg={} waves={} allowed={} special={} fogColumn={}",
		ModeName(), theBoard->mApp->GetStageString(theBoard->mLevel), LaterLevelsOn() ? 1 : 0, aRow ? 1 : 0,
		static_cast<int>(theBoard->mBackground), theBoard->mNumWaves, aAllowed, u::SpecialName(CurrentSpecialLevel(theBoard)),
		theBoard->StageHasFog() ? std::to_string(theBoard->LeftFogColumn()) : std::string("-")));
}

void LaterLevelsDevTick(Board* theBoard, int thePlayTicks) {
	LawnApp* aApp = theBoard->mApp;
	static bool aLogged = false;
	if (!aLogged && aApp->IsAdventureMode() && theBoard->mLevel > NUM_LEVELS) {
		aLogged = true;
		std::string aRows;
		for (int y = 0; y < MAX_GRID_SIZE_Y; y++)
			aRows += std::to_string(static_cast<int>(theBoard->mPlantRow[y]));
		std::string aPots;
		for (Plant* aPlant : theBoard->mPlants)
			if (!aPlant->mDead && aPlant->mSeedType == SeedType::SEED_FLOWERPOT)
				aPots += std::format("{}{}:{}", aPots.empty() ? "" : ",", aPlant->mPlantCol, aPlant->mRow);
		int aAsleep = 0, aPlants = 0;
		for (Plant* aPlant : theBoard->mPlants)
			if (!aPlant->mDead && aPlant->mSeedType != SeedType::SEED_FLOWERPOT) {
				aPlants++;
				aAsleep += aPlant->mIsAsleep ? 1 : 0;
			}
		DevLog(std::format("laterPlay mode={} level={} tick={} wave={}/{} bg={} rows={} startSun={} sunCountDown={} night={} pool={} roof={} fog={} "
			"graves={} pots={} potCells={} zombieCountDown={} plants={} asleep={}",
			ModeName(), aApp->GetStageString(theBoard->mLevel), thePlayTicks, theBoard->mCurrentWave, theBoard->mNumWaves,
			static_cast<int>(theBoard->mBackground), aRows,
			theBoard->mSunMoney, theBoard->mSunCountDown, theBoard->StageIsNight() ? 1 : 0, theBoard->StageHasPool() ? 1 : 0,
			theBoard->StageHasRoof() ? 1 : 0, theBoard->StageHasFog() ? 1 : 0, Graves(theBoard),
			static_cast<int>(std::count(aPots.begin(), aPots.end(), ':')), aPots, theBoard->mZombieCountDown, aPlants, aAsleep));
	}
	static const int aWinAt = CmdOptionInt("-pvzu-dev-win", 0);
	static bool aWon = false;
	if (aWinAt > 0 && !aWon && thePlayTicks >= aWinAt) {
		aWon = true;
		DevLog(std::format("devWin mode={} level={} profileLevel={} finished={}", ModeName(), aApp->GetStageString(theBoard->mLevel),
			aApp->mPlayerInfo->mLevel, aApp->mPlayerInfo->mFinishedAdventure));
		aApp->mBoardResult = BoardResult::BOARDRESULT_WON;
		theBoard->mLevelComplete = true;
	}
}

void DevLogFinish(LawnApp* theApp, int theLevel) {
	if (!CmdOption("-pvzu-log-waves") || !theApp->IsAdventureMode() || theApp->mPlayerInfo == nullptr)
		return;
	DevLog(std::format("finish mode={} level={} finalLevel={} nextProfileLevel={} finishedAdventure={}", ModeName(),
		theApp->GetStageString(theLevel), AdventureFinalLevel(), theApp->mPlayerInfo->mLevel, theApp->mPlayerInfo->mFinishedAdventure));
}

void DevLogAward(int theAwardType) {
	if (CmdOption("-pvzu-log-waves"))
		DevLog(std::format("award mode={} type={}", ModeName(), theAwardType));
}

void DevProfileOverrides(PlayerInfo* thePlayer) {
	if (thePlayer == nullptr)
		return;
	if (auto aFinished = CmdOption("-pvzu-dev-finished")) {
		thePlayer->mFinishedAdventure = CmdOptionInt("-pvzu-dev-finished", 0);
		DevLog(std::format("devFinished profileLevel={} finished={}", thePlayer->mLevel, thePlayer->mFinishedAdventure));
	}
}

} // namespace pvzu::engine
