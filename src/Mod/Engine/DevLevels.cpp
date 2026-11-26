#include "Mod/Engine/DevLevels.h"
#include "Mod/Engine/ModModes.h"

#include "Mod/CmdLine.h"
#include "Mod/GameData.h"
#include "Mod/Mode.h"
#include "Mod/SeedSlots.h"
#include "Mod/Engine/ModPlants.h" // PVZU Plan 8: -pvzu-dev-imitater
#include "Mod/Engine/LaterLevels.h" // R8: -pvzu-dev-finished before the seed chooser

#include "LawnApp.h"
#include "Lawn/Board.h"
#include "Lawn/Plant.h"
#include "Lawn/Zombie.h"
#include "Lawn/ZenGarden.h"
#include "Lawn/System/PlayerInfo.h"
#include "Lawn/System/ProfileMgr.h"
#include "Lawn/Widget/TitleScreen.h"
#include "widget/WidgetManager.h"
#include "Common.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <format>
#include <string>

namespace pvzu::engine {

namespace {
std::string ZombieName(int theType) {
	const auto& aZombies = GameData::Get().Active().zombies;
	if (theType >= 0 && theType < static_cast<int>(aZombies.size()))
		return aZombies[theType].name;
	return std::format("#{}", theType);
}
int ZombieValue(int theType) {
	const auto& aZombies = GameData::Get().Active().zombies;
	return theType >= 0 && theType < static_cast<int>(aZombies.size()) ? aZombies[theType].zombieValue : 0;
}
const char* ModeName() { return CurrentMode() == GameMode::Unnamed ? "unnamed" : "vanilla"; }
}

bool DevFlag(std::string_view theName) { return CmdFlag(theName); }

void DevLog(std::string_view theLine) {
	static const std::optional<std::string> aPath = CmdOption("-pvzu-log-waves");
	if (!aPath || aPath->empty())
		return;
	if (std::FILE* aFile = std::fopen(aPath->c_str(), "a")) {
		std::fprintf(aFile, "%.*s\n", static_cast<int>(theLine.size()), theLine.data());
		std::fclose(aFile);
	}
	std::fprintf(stderr, "PVZU: %.*s\n", static_cast<int>(theLine.size()), theLine.data());
}

bool DevStartLevel(LawnApp* theApp) {
	if (ModModeDevStart(theApp)) // -pvzu-game-mode 0x67 (mod modes, any challenge)
		return true;
	const int aLevel = CmdOptionInt("-pvzu-level", 0);
	if (aLevel <= 0)
		return false;
	if (theApp->mPlayerInfo == nullptr)
		theApp->mPlayerInfo = theApp->mProfileMgr->AddProfile("pvzu");
	if (theApp->mPlayerInfo == nullptr)
		return false;
	theApp->mPlayerInfo->mLevel = aLevel;
	if (const int aSlots = CmdOptionInt("-pvzu-dev-slots", 0); aSlots > 0) // seed slots (6..15, any mode) for seed panel checks
		theApp->mPlayerInfo->mPurchases[StoreItem::STORE_ITEM_PACKET_UPGRADE] = DevSlotsToUpgrades(aSlots);
	ModPlantsDevProfile(theApp); // PVZU Plan 8: -pvzu-dev-imitater (CARD's second condition)
	DevProfileOverrides(theApp->mPlayerInfo); // R8: -pvzu-dev-finished here too, so Dave's picks (second run) show in the chooser
	if (const int aCoins = CmdOptionInt("-pvzu-dev-coins", 0); aCoins > 0) // coin bank (in 10s, as saved) for Dave's slot offer
		theApp->mPlayerInfo->mCoins = aCoins;
	if (CmdOption("-pvzu-seed")) {
		const int aSeedValue = CmdOptionInt("-pvzu-seed", 0);
		Sexy::SRand(static_cast<ulong>(aSeedValue));
		theApp->mAppRandSeed = aSeedValue;
	}
	DevLog(std::format("start mode={} level={} seed={}", ModeName(), aLevel, CmdOption("-pvzu-seed").value_or("-")));
	if (CmdFlag("-pvzu-dev-continue")) { // save checks: open this profile's saved game (the continue dialog) instead of a new one
		theApp->mWidgetManager->RemoveWidget(theApp->mTitleScreen.get());
		theApp->SafeDeleteWidget(theApp->mTitleScreen.release());
		theApp->PreNewGame(::GameMode::GAMEMODE_ADVENTURE, true);
		return true;
	}
	if (CmdFlag("-pvzu-dev-zen")) { // the Zen Garden instead (Crazy Dave's first-visit talk)
		theApp->FastLoad(::GameMode::GAMEMODE_CHALLENGE_ZEN_GARDEN);
		if (theApp->mZenGarden && theApp->mBoard)
			theApp->mZenGarden->SetupForZenTutorial();
		return true;
	}
	theApp->FastLoad(::GameMode::GAMEMODE_ADVENTURE);
	return true;
}

void DevLogWaves(Board* theBoard) {
	if (!CmdOption("-pvzu-log-waves"))
		return;
	LawnApp* aApp = theBoard->mApp;
	const int aWorld = (theBoard->mLevel - 1) / 10 + 1;
	const int aSub = (theBoard->mLevel - 1) % 10 + 1;
	int aTotal = 0;
	for (int aWave = 0; aWave < theBoard->mNumWaves; aWave++) {
		std::string aList;
		int aBudget = 0;
		for (int i = 0; i < MAX_ZOMBIES_IN_WAVE; i++) {
			const ZombieType aType = theBoard->mZombiesInWave[aWave][i];
			if (aType == ZombieType::ZOMBIE_INVALID)
				break;
			aBudget += ZombieValue(aType);
			if (!aList.empty())
				aList += ',';
			aList += ZombieName(aType);
		}
		aTotal += aBudget;
		DevLog(std::format("waves mode={} level={}-{} firstTime={} wave={}/{} flag={} budget={} zombies={}", ModeName(), aWorld,
			aSub, aApp->IsFirstTimeAdventureMode() ? 1 : 0, aWave + 1, theBoard->mNumWaves, theBoard->IsFlagWave(aWave) ? 1 : 0, aBudget,
			aList));
	}
	DevLog(std::format("total mode={} level={}-{} waves={} budget={}", ModeName(), aWorld, aSub, theBoard->mNumWaves, aTotal));
	if (CmdFlag("-pvzu-quit-after-waves")) {
		std::fflush(nullptr);
		std::_Exit(0);
	}
}

void DevOnStartLevel(Board* theBoard) {
	if (!CmdFlag("-pvzu-dev-vault"))
		return;
	// Row 0: Peashooter col 7; row 1: Tall-nut col 5 (bonk); row 2: Peashooter col 5; row 3: Wall-nut col 1.
	theBoard->AddPlant(7, 0, SeedType::SEED_PEASHOOTER);
	theBoard->AddPlant(5, 1, SeedType::SEED_TALLNUT);
	theBoard->AddPlant(5, 2, SeedType::SEED_PEASHOOTER);
	theBoard->AddPlant(1, 3, SeedType::SEED_WALLNUT);
	for (int aRow = 0; aRow < 4; aRow++)
		theBoard->AddZombieInRow(ZombieType::ZOMBIE_POLEVAULTER, aRow, 0);
	DevLog(std::format("devVault mode={} level={} plants: r0 PEASHOOTER c7, r1 TALLNUT c5, r2 PEASHOOTER c5, r3 WALLNUT c1; 4 POLEVAULTER",
		ModeName(), theBoard->mLevel));
}

} // namespace pvzu::engine
