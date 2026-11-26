// PVZU: see ModModes.h. Generic part (table, background, zombies, slots, graves, names, records, dev start); the
// per-mode rules are in ModModeRules.cpp, the list screen in ModModesScreen.cpp.
#include "Mod/Engine/ModModes.h"

#include "Mod/CmdLine.h"
#include "Mod/Engine/DevLevels.h"
#include "Mod/Engine/ModPlants.h"
#include "Mod/Engine/Startup.h"
#include "Mod/GameData.h"
#include "Mod/Mode.h"
#include "Mod/Unnamed/LaterLevels.h"
#include "Mod/Unnamed/ModModes.h"

#include "LawnApp.h"
#include "Lawn/Board.h"
#include "Lawn/System/PlayerInfo.h"
#include "Lawn/System/ProfileMgr.h"
#include "Lawn/Widget/ChallengeScreen.h"
#include "Lawn/Widget/TitleScreen.h"
#include "PvzpLib/PvzpCommon.h"
#include "SexyAppBase.h"
#include "widget/WidgetManager.h"
#include "Common.h"

#include <array>
#include <cstdio>
#include <filesystem>
#include <format>
#include <fstream>
#include <string>
#include <vector>
#include "Lawn/Challenge.h"
#include "Mod/Unnamed/ModModeRules.h"

namespace pvzu::engine {

namespace u = pvzu::unnamed;

namespace {
bool gLoaded = false;

void LoadTable() {
	if (gLoaded)
		return;
	gLoaded = true;
	const std::filesystem::path aFile = UnnamedPackDir() / "data" / "modes.json";
	std::ifstream aIn(aFile);
	if (!aIn) {
		PvzpLogLn("PVZU: modModes on but {} is missing", aFile.string());
		return;
	}
	try {
		const nlohmann::json aTable = nlohmann::json::parse(aIn);
		std::string aErr;
		if (!u::SetModModes(aTable, &aErr))
			PvzpLogLn("PVZU: modes table ignored: {}", aErr);
	} catch (const nlohmann::json::exception& e) {
		PvzpLogLn("PVZU: modes table ignored: {}", e.what());
	}
}

std::vector<std::string> EngineZombieNames() {
	std::vector<std::string> aNames;
	for (const auto& aZombie : GameData::Get().Vanilla().zombies)
		aNames.push_back(aZombie.name);
	return aNames;
}

const char* ModeLogName() { return CurrentMode() == GameMode::Unnamed ? "unnamed" : "vanilla"; }

// Level names for ChallengeDefinition::mChallengeName (kept alive here).
std::array<std::string, u::kNumModModes> gNames;
std::array<ChallengeDefinition, u::kNumModModes> gDefs;
} // namespace

bool ModModesOn() {
	if (!GameData::Get().Feature("modModes"))
		return false;
	LoadTable();
	return true;
}

const u::ModModeDef* CurrentModMode(LawnApp* theApp) {
	if (theApp == nullptr || !u::IsModModeValue(static_cast<int>(theApp->mGameMode)) || !ModModesOn())
		return nullptr;
	return u::FindModMode(static_cast<int>(theApp->mGameMode));
}

bool IsModModeRunning(LawnApp* theApp) { return CurrentModMode(theApp) != nullptr; }

std::string ModModeDisplayName(const u::ModModeDef& theDef) {
	return StartupSettings().language == "zh" && !theDef.nameZh.empty() ? theDef.nameZh : theDef.name;
}

const ChallengeDefinition* ModModeChallengeDef(LawnApp* theApp) {
	const u::ModModeDef* aDef = CurrentModMode(theApp);
	if (aDef == nullptr)
		return nullptr;
	const int i = aDef->mode - u::kFirstModMode;
	gNames[i] = ModModeDisplayName(*aDef);
	gDefs[i] = ChallengeDefinition{static_cast<::GameMode>(aDef->mode), aDef->icon, ChallengePage::CHALLENGE_PAGE_CHALLENGE, 0, 0,
		gNames[i].c_str()};
	return &gDefs[i];
}

bool ModModeBackground(Board* theBoard) {
	const u::ModModeDef* aDef = CurrentModMode(theBoard->mApp);
	if (aDef == nullptr)
		return false;
	theBoard->mBackground = static_cast<BackgroundType>(aDef->background);
	return true;
}

bool ModModePlantRows(Board* theBoard) {
	if (CurrentModMode(theBoard->mApp) == nullptr)
		return false;
	std::array<int, 6> aRows{};
	if (!u::PlantRowsFor(static_cast<int>(theBoard->mBackground), aRows)) // laterLevels.rows.<bg> (hooks 26/27)
		return false;
	for (int y = 0; y < MAX_GRID_SIZE_Y; y++)
		theBoard->mPlantRow[y] = static_cast<PlantRowType>(aRows[y]);
	return true;
}

void ModModeAllowed(Board* theBoard) {
	const u::ModModeDef* aDef = CurrentModMode(theBoard->mApp);
	if (aDef == nullptr)
		return;
	// Challenge::InitZombieWaves' last branch (Normal, Cone, Pail, Yeti) ran; the mod's chain replaces it (hook 180 ->
	// 0x7a5a00 -> ... -> 0x7a5e96). Yeti is the vanilla line after the Pail write, so it follows Pail.
	const bool aYeti = theBoard->mZombieAllowed[ZOMBIE_YETI];
	memset(theBoard->mZombieAllowed, false, sizeof(theBoard->mZombieAllowed));
	std::string aErr;
	for (int aType : u::ResolveZombies(u::ModModeZombies(*aDef), EngineZombieNames(), &aErr))
		if (aType >= 0 && aType < NUM_ZOMBIE_TYPES)
			theBoard->mZombieAllowed[aType] = true;
	if (!aErr.empty())
		DevLog(std::format("modModes mode=0x{:x} allow list: {}", aDef->mode, aErr));
	theBoard->mZombieAllowed[ZOMBIE_YETI] = aYeti && aDef->pail;
}

int ModModeNumWaves(Board* theBoard, int theVanilla) {
	const u::ModModeDef* aDef = CurrentModMode(theBoard->mApp);
	return aDef != nullptr && aDef->waves >= 0 ? std::min(aDef->waves, MAX_ZOMBIE_WAVES) : theVanilla;
}

bool ModModeChooser(Board* theBoard, bool& theShow) {
	const u::ModModeDef* aDef = CurrentModMode(theBoard->mApp);
	if (aDef == nullptr || aDef->chooser)
		return false;
	theShow = false;
	return true;
}

bool ModModeConveyor(Board* theBoard) {
	const u::ModModeDef* aDef = CurrentModMode(theBoard->mApp);
	return aDef != nullptr && aDef->conveyor;
}

bool ModModeConveyorSeeds(Board* theBoard, PvzpWeightedArray* theArray, int& theCount) {
	const u::ModModeDef* aDef = CurrentModMode(theBoard->mApp);
	if (aDef == nullptr || !aDef->conveyor)
		return false;
	theCount = 0;
	for (const auto& [aName, aWeight] : aDef->conveyorSeeds) {
		const int aSeed = SeedTypeByName(aName);
		if (aSeed < 0 || theCount >= 20)
			continue;
		theArray[theCount].mItem = aSeed;
		theArray[theCount].mWeight = aWeight;
		theCount++;
	}
	static int sLogged = 0;
	if (sLogged++ < 2) {
		std::string aList;
		for (int i = 0; i < theCount; i++)
			aList += std::format("{}{}:{}", i ? "," : "", theArray[i].mItem, theArray[i].mWeight);
		DevLog(std::format("modModes conveyor mode=0x{:x} seeds={}", aDef->mode, aList));
	}
	return theCount > 0;
}

bool ModModeNoSkySun(Board* theBoard) {
	const u::ModModeDef* aDef = CurrentModMode(theBoard->mApp);
	return aDef != nullptr && !aDef->skySun;
}

void ModModeLevelNameColor(Board* theBoard, Sexy::Color& theColor) {
	const u::ModModeDef* aDef = CurrentModMode(theBoard->mApp);
	if (aDef == nullptr)
		return;
	const u::Rgb c = u::ModModeNameColor(aDef->nameColor, theBoard->mMainCounter);
	theColor = Sexy::Color(c.r, c.g, c.b, theColor.mAlpha);
}

int SeedTypeByName(std::string_view theName) {
	// "0x72": a mod plant seed (0x50..0x7f), only when the pack's mod plants are loaded
	if (theName.size() > 2 && theName[0] == '0' && theName[1] == 'x') {
		const int aSeed = static_cast<int>(std::strtol(std::string(theName).c_str(), nullptr, 16));
		return IsModSeed(static_cast<SeedType>(aSeed)) ? aSeed : -1;
	}
	const auto& aPlants = GameData::Get().Vanilla().plants;
	for (int i = 0; i < static_cast<int>(aPlants.size()); i++)
		if (aPlants[i].name == theName)
			return i;
	return -1;
}

bool ModModeSeedSlots(Board* theBoard, int& theSlots) {
	const u::ModModeDef* aDef = CurrentModMode(theBoard->mApp);
	if (aDef != nullptr && aDef->mode == 0x8d) {
		theSlots = 10; // port ruling: Board::InitLevel writes ten packets (0xc004c8) and hook 142 has no 0x8d branch
		return true;
	}
	if (aDef != nullptr && aDef->mode == 0x8c) {
		theSlots = 1; // port ruling: only packet 0 (the Imp card) is written (0xc004a0); hook 142 has no 0x8c branch
		return true;
	}
	if (aDef != nullptr && aDef->mode == 0x6b && theBoard->mChallenge && theBoard->mChallenge->mChallengeScore == 2) {
		theSlots = 2; // V2: Glove War's bank after the chooser is Blover + Cherry Bomb (0x7a4c50 writes mNumPackets 2)
		return true;
	}
	if (aDef == nullptr || aDef->seedSlots < 0)
		return false;
	theSlots = std::min(aDef->seedSlots, SEEDBANK_MAX);
	// 0x73 / 0x91: one pick in the chooser, six packets after it (0x798fdf; Challenge::mChallengeScore = 6 marks it)
	if ((aDef->mode == 0x73 || aDef->mode == 0x91) && theBoard->mChallenge && theBoard->mChallenge->mChallengeScore == 6)
		theSlots = 6;
	return true;
}

bool ModModeGraves(Board* theBoard, bool& theHas) {
	const u::ModModeDef* aDef = CurrentModMode(theBoard->mApp);
	if (aDef == nullptr || aDef->graves < 0)
		return false;
	theHas = aDef->graves > 0 && theBoard->mBackground == BackgroundType::BACKGROUND_2_NIGHT;
	return true;
}

bool ModModeHidesSeed(LawnApp* theApp, int theSeedType) {
	const u::ModModeDef* aDef = CurrentModMode(theApp);
	if (aDef != nullptr && u::ItemModeHidesSeed(u::ItemModeOf(aDef->mode), theSeedType))
		return true; // hooks 438 / 452: no upgrade plants in item modes 1, 2, 4
	if (aDef == nullptr || aDef->bannedSeeds.empty())
		return false;
	const auto& aPlants = GameData::Get().Vanilla().plants;
	if (theSeedType < 0 || theSeedType >= static_cast<int>(aPlants.size()))
		return false;
	for (const std::string& aName : aDef->bannedSeeds)
		if (aName == aPlants[theSeedType].name)
			return true;
	return false;
}

// ---- records: mChallengeRecords[100..147] in <userdata-unnamed>/user<id>-modes.json ----

namespace {
std::string RecordsFile(const PlayerInfo* thePlayer) {
	return Sexy::GetAppDataPath(CurrentUserDataPath(std::format("user{}-modes.json", thePlayer->mId)));
}
} // namespace

void ModModeRecordsLoad(PlayerInfo* thePlayer) {
	if (thePlayer == nullptr || CurrentMode() != GameMode::Unnamed)
		return;
	for (int i = u::kNumVanillaRecords; i < u::kNumRecordsWithModModes; i++)
		thePlayer->mChallengeRecords[i] = 0;
	std::ifstream aIn(RecordsFile(thePlayer));
	if (!aIn)
		return;
	try {
		const nlohmann::json j = nlohmann::json::parse(aIn);
		for (int aMode = u::kFirstModMode; aMode <= u::kLastModMode; aMode++) {
			const std::string aKey = std::format("0x{:x}", aMode);
			if (j.contains(aKey) && j[aKey].is_number_unsigned())
				thePlayer->mChallengeRecords[u::ModModeRecordIndex(aMode)] = j[aKey].get<uint32_t>();
		}
	} catch (const nlohmann::json::exception& e) {
		PvzpLogLn("PVZU: mode records ignored: {}", e.what());
	}
}

void ModModeRecordsSave(PlayerInfo* thePlayer) {
	if (thePlayer == nullptr || CurrentMode() != GameMode::Unnamed)
		return;
	nlohmann::json j = nlohmann::json::object();
	bool aAny = false;
	for (int aMode = u::kFirstModMode; aMode <= u::kLastModMode; aMode++) {
		const uint32_t aRecord = thePlayer->mChallengeRecords[u::ModModeRecordIndex(aMode)];
		if (aRecord > 0) {
			j[std::format("0x{:x}", aMode)] = aRecord;
			aAny = true;
		}
	}
	const std::string aFile = RecordsFile(thePlayer);
	if (!aAny) {
		std::remove(aFile.c_str());
		return;
	}
	std::ofstream aOut(aFile);
	aOut << j.dump(1, '\t') << '\n';
}

void ModModeDeleteUserFiles(PlayerInfo* thePlayer) {
	if (thePlayer == nullptr || CurrentMode() != GameMode::Unnamed)
		return;
	int aErased = 0;
	for (const std::string& aName : u::ModModeUserFileNames(thePlayer->mId)) {
		const std::string aFile = Sexy::GetAppDataPath(CurrentUserDataPath(aName));
		if (std::filesystem::exists(aFile))
			aErased++;
		gSexyAppBase->EraseFile(aFile);
	}
	DevLog(std::format("modModes profile {} deleted: mod files erased={}", thePlayer->mId, aErased));
}

// ---- dev start ----

bool ModModeDevStart(LawnApp* theApp) {
	const auto aOpt = CmdOption("-pvzu-game-mode");
	if (!aOpt)
		return false;
	const bool aScreen = *aOpt == "screen"; // -pvzu-game-mode screen: the challenge screen (mini-games page)
	const int aMode = aScreen ? 0 : static_cast<int>(std::strtol(aOpt->c_str(), nullptr, 0));
	if (aMode <= 0 && !aScreen)
		return false;
	if (theApp->mPlayerInfo == nullptr)
		theApp->mPlayerInfo = theApp->mProfileMgr->AddProfile("pvzu");
	if (theApp->mPlayerInfo == nullptr)
		return false;
	if (CmdFlag("-pvzu-dev-delete-profile")) { // dev: delete the dev profile instead of starting (DeleteUserFiles checks)
		const std::string aName = theApp->mPlayerInfo->mName;
		theApp->mPlayerInfo = nullptr;
		DevLog(std::format("modModes dev delete profile {} ok={}", aName, theApp->mProfileMgr->DeleteProfile(aName) ? 1 : 0));
		return false;
	}
	if (CmdOption("-pvzu-seed")) {
		const int aSeedValue = CmdOptionInt("-pvzu-seed", 0);
		Sexy::SRand(static_cast<ulong>(aSeedValue));
		theApp->mAppRandSeed = aSeedValue;
	}
	// A player who reached the challenge pages: Adventure finished, every vanilla plant owned (dev profile only).
	theApp->mPlayerInfo->mLevel = CmdOptionInt("-pvzu-dev-profile-level", 51);
	theApp->mPlayerInfo->mFinishedAdventure = std::max<uint32_t>(theApp->mPlayerInfo->mFinishedAdventure, 1);
	if (const int aSlots = CmdOptionInt("-pvzu-dev-slots", 0); aSlots > 6)
		theApp->mPlayerInfo->mPurchases[StoreItem::STORE_ITEM_PACKET_UPGRADE] = aSlots - 6;
	ModPlantsDevProfile(theApp); // PVZU Plan 9 R9: -pvzu-dev-imitater here too (CARD's mod seeds in a mode's chooser)
	if (aScreen) {
		DevLog(std::format("modModes screen run={} feature={}", ModeLogName(), ModModesOn() ? 1 : 0));
		theApp->mWidgetManager->RemoveWidget(theApp->mTitleScreen.get());
		theApp->SafeDeleteWidget(theApp->mTitleScreen.release());
		theApp->ShowChallengeScreen(ChallengePage::CHALLENGE_PAGE_CHALLENGE);
		return true;
	}
	if (u::IsModModeValue(aMode) && !ModModesOn()) {
		DevLog(std::format("modModes start mode=0x{:x} refused: feature off ({})", aMode, ModeLogName()));
		return false;
	}
	const u::ModModeDef* aDef = u::IsModModeValue(aMode) ? u::FindModMode(aMode) : nullptr;
	DevLog(std::format("modModes start mode=0x{:x} name=\"{}\" status={} run={} seed={}", aMode, aDef ? aDef->name : "-",
		aDef ? u::ModModeStatusName(aDef->status) : "-", ModeLogName(), CmdOption("-pvzu-seed").value_or("-")));
	theApp->mWidgetManager->RemoveWidget(theApp->mTitleScreen.get());
	theApp->SafeDeleteWidget(theApp->mTitleScreen.release());
	theApp->PreNewGame(static_cast<::GameMode>(aMode), CmdFlag("-pvzu-dev-continue"));
	return true;
}

} // namespace pvzu::engine
