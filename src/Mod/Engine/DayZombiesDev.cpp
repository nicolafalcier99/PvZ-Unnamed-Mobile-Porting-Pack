// PVZU: desktop dev options for the Day-zombie checks (see DayZombiesDev.h).
#include "Mod/Engine/DayZombiesDev.h"

#include "Mod/CmdLine.h"
#include "Mod/Engine/EngineTables.h"
#include "Mod/GameData.h"

#include "LawnApp.h"
#include "Lawn/Board.h"
#include "Lawn/Plant.h"
#include "Lawn/CursorObject.h"
#include "Lawn/Zombie.h"
#include "Lawn/System/PlayerInfo.h"
#include "Lawn/System/ProfileMgr.h"
#include "Lawn/Widget/TitleScreen.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <utility>
#include <vector>

namespace pvzu::engine {

namespace {

std::vector<std::string> SplitList(const std::string& text) {
	std::vector<std::string> out;
	size_t start = 0;
	while (start <= text.size()) {
		size_t comma = text.find(',', start);
		std::string item = text.substr(start, comma == std::string::npos ? std::string::npos : comma - start);
		if (!item.empty())
			out.push_back(item);
		if (comma == std::string::npos)
			break;
		start = comma + 1;
	}
	return out;
}

int ZombieTypeByName(const std::string& name) {
	RegisterVanillaTables();
	const auto& zombies = GameData::Get().Vanilla().zombies;
	for (size_t i = 0; i < zombies.size(); ++i)
		if (zombies[i].name == name)
			return static_cast<int>(i);
	return -1;
}

int SeedTypeByName(const std::string& name) {
	RegisterVanillaTables();
	const auto& plants = GameData::Get().Vanilla().plants;
	for (size_t i = 0; i < plants.size(); ++i)
		if (plants[i].name == name)
			return static_cast<int>(i);
	return -1;
}

struct Dev {
	bool parsed = false;
	int level = -1;
	bool resume = false;
	bool started = false;
	bool setupDone = false;
	int sun = -1;
	struct DevPlant { int seed, col, tick; bool done; };
	std::vector<DevPlant> plants;
	std::vector<int> spawnTypes;
	int spawnEvery = 100;
	int spawnX = -1;
	int spawnCount = 0;
	int spawned = 0;
	int quitAfter = -1;
	int playTicks = 0;
	int playerLevel = -1;   // PVZU Plan 7: -pvzu-dev-player-level (world-7 gate = the profile level, not the board's)
	int treeFeet = -1;      // PVZU Plan 7: -pvzu-dev-tree (Tree of Wisdom bought, that height)
	int finished = -1;      // PVZU Plan 7: -pvzu-dev-finished (times the adventure was finished; Almanac checks)
	struct DevShovel { int col, tick; bool done; }; // PVZU Plan 9 V1: -pvzu-dev-shovel (glove Pole leap checks)
	std::vector<DevShovel> shovels;
};

Dev& D() {
	static Dev d;
	if (d.parsed)
		return d;
	d.parsed = true;
	d.level = CmdOptionInt("-pvzu-dev-level", -1);
	d.resume = CmdFlag("-pvzu-dev-continue");
	d.sun = CmdOptionInt("-pvzu-dev-sun", -1);
	d.playerLevel = CmdOptionInt("-pvzu-dev-player-level", -1);
	d.treeFeet = CmdOptionInt("-pvzu-dev-tree", -1);
	d.finished = CmdOptionInt("-pvzu-dev-finished", -1);
	for (const auto& item : SplitList(CmdOption("-pvzu-dev-plant").value_or(""))) {
		size_t colon = item.find(':');
		int seed = SeedTypeByName(item.substr(0, colon));
		if (seed < 0 || colon == std::string::npos) {
			std::fprintf(stderr, "PVZU: -pvzu-dev-plant: cannot read '%s' (want SEED:col[@tick])\n", item.c_str());
			continue;
		}
		size_t at = item.find('@');
		d.plants.push_back({seed, std::atoi(item.c_str() + colon + 1), at == std::string::npos ? 1 : std::atoi(item.c_str() + at + 1), false});
	}
	for (const auto& item : SplitList(CmdOption("-pvzu-dev-shovel").value_or(""))) {
		size_t at = item.find('@');
		d.shovels.push_back({std::atoi(item.c_str()), at == std::string::npos ? 1 : std::atoi(item.c_str() + at + 1), false});
	}
	for (const auto& name : SplitList(CmdOption("-pvzu-dev-spawn").value_or(""))) {
		int t = ZombieTypeByName(name);
		if (t < 0)
			std::fprintf(stderr, "PVZU: -pvzu-dev-spawn: unknown zombie type '%s'\n", name.c_str());
		else
			d.spawnTypes.push_back(t);
	}
	d.spawnEvery = std::max(1, CmdOptionInt("-pvzu-dev-spawn-every", 100));
	d.spawnX = CmdOptionInt("-pvzu-dev-spawn-x", -1);
	d.spawnCount = CmdOptionInt("-pvzu-dev-spawn-count", static_cast<int>(d.spawnTypes.size()));
	d.quitAfter = CmdOptionInt("-pvzu-dev-quit", -1);
	return d;
}

void StartLevel(LawnApp* theApp, Dev& d) {
	d.started = true;
	theApp->LoadingCompleted();
	theApp->KillGameSelector();
	if (theApp->mPlayerInfo == nullptr) {
		PlayerInfo* aProfile = theApp->mProfileMgr->GetProfile("pvzu-dev");
		if (aProfile == nullptr)
			aProfile = theApp->mProfileMgr->AddProfile("pvzu-dev");
		theApp->mPlayerInfo = aProfile;
		theApp->mProfileMgr->Save();
	}
	theApp->mPlayerInfo->mLevel = d.level;
	std::fprintf(stderr, "PVZU dev: %s adventure level %d\n", d.resume ? "continuing" : "starting", d.level);
	theApp->PreNewGame(GameMode::GAMEMODE_ADVENTURE, d.resume);
}

} // namespace

void DayZombiesDevUpdate(LawnApp* theApp) {
	Dev& d = D();
	if (d.level < 0 && d.spawnTypes.empty() && d.shovels.empty() && d.quitAfter < 0 && d.playerLevel < 0 && d.treeFeet < 0 && d.finished < 0)
		return;
	if (d.level >= 0 && !d.started) {
		if (theApp->mTitleScreen && theApp->mTitleScreen->mLoadingThreadComplete)
			StartLevel(theApp, d);
		return;
	}
	if (d.resume && theApp->GetDialog(Dialogs::DIALOG_CONTINUE)) { // the Continue button, without a click
		std::fprintf(stderr, "PVZU dev: saved game loaded, continuing\n");
		theApp->KillDialog(Dialogs::DIALOG_CONTINUE);
	}
	Board* aBoard = theApp->mBoard;
	if (aBoard == nullptr || theApp->mGameScene != GameScenes::SCENE_PLAYING)
		return;
	if (!d.setupDone) {
		d.setupDone = true;
		if (d.sun >= 0)
			aBoard->mSunMoney = d.sun;
		if (d.playerLevel >= 0 && theApp->mPlayerInfo) { // the board keeps its level; only the profile changes
			theApp->mPlayerInfo->mLevel = d.playerLevel;
			std::fprintf(stderr, "PVZU dev: profile level %d (board level %d)\n", d.playerLevel, aBoard->mLevel);
		}
		if (d.treeFeet >= 0 && theApp->mPlayerInfo) {
			theApp->mPlayerInfo->mPurchases[StoreItem::STORE_ITEM_TREE_OF_WISDOM] = 1;
			theApp->mPlayerInfo->mChallengeRecords[GameMode::GAMEMODE_TREE_OF_WISDOM - 1] = static_cast<uint32_t>(d.treeFeet);
			std::fprintf(stderr, "PVZU dev: Tree of Wisdom %d ft\n", d.treeFeet);
		}
		if (d.finished >= 0 && theApp->mPlayerInfo) {
			theApp->mPlayerInfo->mFinishedAdventure = static_cast<uint32_t>(d.finished);
			std::fprintf(stderr, "PVZU dev: adventure finished %d times\n", d.finished);
		}
		if (auto aType = CmdOption("-pvzu-dev-almanac")) { // PVZU Plan 7: the Almanac's zombie page on that zombie
			const int t = ZombieTypeByName(*aType);
			std::fprintf(stderr, "PVZU dev: Almanac zombie page %s\n", aType->c_str());
			theApp->DoAlmanacDialog(SeedType::SEED_NONE, t >= 0 ? static_cast<ZombieType>(t) : ZombieType::ZOMBIE_NORMAL);
		}
	}
	++d.playTicks;
	for (auto& p : d.plants) {
		if (p.done || d.playTicks < p.tick)
			continue;
		p.done = true;
		for (int row = 0; row < MAX_GRID_SIZE_Y; ++row)
			if (aBoard->CanPlantAt(p.col, row, static_cast<SeedType>(p.seed)) == PlantingReason::PLANTING_OK)
				aBoard->AddPlant(p.col, row, static_cast<SeedType>(p.seed));
	}
	for (auto& s : d.shovels) { // the real shovel click path (Board::MouseDownWithTool)
		if (s.done || d.playTicks < s.tick)
			continue;
		s.done = true;
		for (int row = 0; row < MAX_GRID_SIZE_Y; ++row)
			if (aBoard->GetTopPlantAt(s.col, row, PlantPriority::TOPPLANT_DIGGING_ORDER)) {
				aBoard->mCursorObject->mCursorType = CursorType::CURSOR_TYPE_SHOVEL; // MouseHitTest reads the held tool
				aBoard->MouseDownWithTool(aBoard->GridToPixelX(s.col, row) + 40, aBoard->GridToPixelY(s.col, row) + 40, 1,
					CursorType::CURSOR_TYPE_SHOVEL);
			}
	}
	if (!d.spawnTypes.empty() && d.spawned < d.spawnCount && d.playTicks % d.spawnEvery == 0) {
		ZombieType aType = static_cast<ZombieType>(d.spawnTypes[d.spawned % d.spawnTypes.size()]);
		const int aWave = std::max(0, aBoard->mCurrentWave);
		if (d.spawnX >= 0) { // one per row, already on the lawn (for screenshots)
			if (Zombie* z = aBoard->AddZombieInRow(aType, d.spawned % (aBoard->StageHas6Rows() ? 6 : 5), aWave)) { // every row
				z->mPosX = static_cast<float>(d.spawnX);
				z->mX = d.spawnX;
			}
		} else {
			aBoard->AddZombie(aType, aWave);
		}
		++d.spawned;
	}
	if (d.quitAfter >= 0 && d.playTicks == d.quitAfter) {
		std::fprintf(stderr, "PVZU dev: quit after %d ticks of play\n", d.playTicks);
		theApp->Shutdown();
	}
}

} // namespace pvzu::engine
