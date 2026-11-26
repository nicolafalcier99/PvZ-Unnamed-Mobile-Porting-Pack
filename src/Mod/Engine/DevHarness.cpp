#include "Mod/Engine/DevHarness.h"

#include "Mod/CmdLine.h"
#include "Mod/Engine/NightLevels.h"
#include "Mod/Engine/PoolPlantSites.h" // PVZU Plan 4: -pvzu-plant-test on a pool lawn
#include "Mod/Engine/FogPlantSites.h" // PVZU Plan 5: -pvzu-plant-test on a fog lawn
#include "Mod/Engine/RoofPlantSites.h" // PVZU Plan 6: -pvzu-plant-test on a roof
#include "Mod/Engine/PoolLevels.h"
#include "Mod/Engine/FogLevels.h"
#include "Mod/Engine/RoofLevels.h"
#include "Mod/Engine/LaterLevels.h"
#include "Mod/Engine/SmallRules.h" // PVZU Plan 9 R9: -pvzu-dev-small
#include "Mod/Engine/ModPlants.h" // PVZU Plan 8: mod-plant dev runs
#include "Mod/Engine/ModesVanilla.h"
#include "Mod/Engine/ModPlantsSpecials.h" // PVZU Plan 8 part 3b: -pvzu-dev-specials
#include "Mod/Engine/ModPlantsShadow.h" // PVZU Plan 8 part 3c: -pvzu-dev-shadow-test

#include "LawnApp.h"
#include "Lawn/Board.h"
#include "Lawn/Plant.h"
#include "Lawn/Zombie.h"
#include "Lawn/System/PlayerInfo.h"
#include "Lawn/System/ProfileMgr.h"

#include <cstdio>
#include <cstdlib>

namespace pvzu::engine {

// DevStartLevel lives in DevLevels.cpp (merged from Task 5, adds -pvzu-seed).

namespace {
int gPlayTicks = 0;
bool gLaidOut = false;
int gCherryCol = 4;

Plant* Put(Board* theBoard, int theCol, int theRow, SeedType theSeed)
{
	if (theBoard->GetTopPlantAt(theCol, theRow, PlantPriority::TOPPLANT_ANY))
		return nullptr;
	return theBoard->AddPlant(theCol, theRow, theSeed, SeedType::SEED_NONE);
}

void LayOut(Board* theBoard)
{
	// row 0: Peashooters (one as if planted from the seed bank), Repeater, Potato Mine
	if (Plant* aPlant = Put(theBoard, 0, 0, SeedType::SEED_PEASHOOTER))
		aPlant->PvzuPlantedFromSeedBank();
	Put(theBoard, 1, 0, SeedType::SEED_PEASHOOTER);
	Put(theBoard, 2, 0, SeedType::SEED_REPEATER);
	Put(theBoard, 6, 0, SeedType::SEED_POTATOMINE);
	// row 1: Sunflowers and a hurt Wall-nut (nothing shoots here)
	for (int aCol = 0; aCol < 5; aCol++)
		Put(theBoard, aCol, 1, SeedType::SEED_SUNFLOWER);
	if (Plant* aNut = Put(theBoard, 7, 1, SeedType::SEED_WALLNUT))
		aNut->mPlantHealth = 2000;
	// row 2: Snow Peas and a Repeater
	Put(theBoard, 0, 2, SeedType::SEED_SNOWPEA);
	Put(theBoard, 1, 2, SeedType::SEED_SNOWPEA);
	Put(theBoard, 2, 2, SeedType::SEED_REPEATER);
	// row 3: Chompers
	Put(theBoard, 5, 3, SeedType::SEED_CHOMPER);
	Put(theBoard, 6, 3, SeedType::SEED_CHOMPER);
	Put(theBoard, 7, 3, SeedType::SEED_CHOMPER);
	std::fprintf(stderr, "PVZU dev: plant-test layout placed\n");
}

// Night layout: every Night plant once, refilled so each ability fires again.
void LogSleep(Plant* thePlant)
{
	if (thePlant)
		std::fprintf(stderr, "PVZU dev: night plant-test seed %d col %d row %d asleep %d\n", static_cast<int>(thePlant->mSeedType),
			thePlant->mPlantCol, thePlant->mRow, thePlant->mIsAsleep ? 1 : 0);
}

void PutGraveBuster(Board* theBoard, int theCol, int theRow)
{
	if (theBoard->GetTopPlantAt(theCol, theRow, PlantPriority::TOPPLANT_ANY))
		return;
	if (!theBoard->GetGraveStoneAt(theCol, theRow))
		theBoard->AddAGraveStone(theCol, theRow);
	LogSleep(theBoard->AddPlant(theCol, theRow, SeedType::SEED_GRAVEBUSTER, SeedType::SEED_NONE));
}

void LayOutNight(Board* theBoard)
{
	// row 0: Puff-shrooms for walkers to eat (freeze), row 1: Sun-shrooms, row 2: Fume-shroom (whole lane),
	// row 3: Scaredy-shroom far left + a Hypno-shroom to be eaten, row 4: a Grave Buster on a grave
	for (int aCol = 3; aCol < 6; aCol++)
		LogSleep(Put(theBoard, aCol, 0, SeedType::SEED_PUFFSHROOM));
	for (int aCol = 0; aCol < 4; aCol++)
		LogSleep(Put(theBoard, aCol, 1, SeedType::SEED_SUNSHROOM));
	LogSleep(Put(theBoard, 0, 2, SeedType::SEED_FUMESHROOM));
	LogSleep(Put(theBoard, 0, 3, SeedType::SEED_SCAREDYSHROOM));
	LogSleep(Put(theBoard, 7, 3, SeedType::SEED_HYPNOSHROOM));
	PutGraveBuster(theBoard, 7, 4);
	std::fprintf(stderr, "PVZU dev: night plant-test layout placed\n");
}

void NightTick(Board* theBoard)
{
	if (gPlayTicks % 400 == 0) // walkers: Puff row, Fume lane, Scaredy/Hypno row (Normal, Cone, Bucket in turn)
	{
		const ZombieType aKinds[] = {ZombieType::ZOMBIE_NORMAL, ZombieType::ZOMBIE_TRAFFIC_CONE, ZombieType::ZOMBIE_PAIL};
		theBoard->AddZombieInRow(ZombieType::ZOMBIE_PAIL, 0, 0); // a Buckethead lives long enough to eat a Puff-shroom
		theBoard->AddZombieInRow(aKinds[(gPlayTicks / 400) % 3], 2, 0);
		theBoard->AddZombieInRow(aKinds[(gPlayTicks / 400 + 1) % 3], 3, 0);
	}
	if (gPlayTicks % 1200 == 0) // refill eaten Puff-shrooms and the Hypno-shroom, a new Grave Buster
	{
		for (int aCol = 3; aCol < 6; aCol++)
			Put(theBoard, aCol, 0, SeedType::SEED_PUFFSHROOM);
		Put(theBoard, 7, 3, SeedType::SEED_HYPNOSHROOM);
		PutGraveBuster(theBoard, 7, 4);
	}
	if (gPlayTicks % 2500 == 0) // an Ice-shroom
		LogSleep(Put(theBoard, 8, 1, SeedType::SEED_ICESHROOM));
	if (gPlayTicks == 7000) // one Doom-shroom (its crater stays)
		LogSleep(Put(theBoard, 8, 4, SeedType::SEED_DOOMSHROOM));
}
}

void DevBoardTick(Board* theBoard)
{
	LawnApp* aApp = theBoard->mApp;
	if (aApp->mGameScene != GameScenes::SCENE_PLAYING)
	{
		NightDevBeforePlay(theBoard); // -pvzu-dev-night: past the seed chooser without clicks
		return;
	}

	if (gPlayTicks == 0)
	{
		const int aSpeed = CmdOptionInt("-pvzu-speed", 0);
		if (aSpeed > 1)
			aApp->mUpdateMultiplier = aSpeed;
	}
	gPlayTicks++;

	const bool aNightTest = CmdFlag("-pvzu-plant-test-night") || (CmdFlag("-pvzu-plant-test") && theBoard->StageIsNight());
	if (RoofPlantTestActive(theBoard)) // Roof-plant layout (Mod/Engine/RoofPlantSites.h)
	{
		theBoard->mSunMoney = 5000;
		RoofPlantTestTick(theBoard, gPlayTicks);
	}
	else if (FogPlantTestActive(theBoard)) // Fog-plant layout (Mod/Engine/FogPlantSites.h); a fog lawn is also night and pool
	{
		theBoard->mSunMoney = 5000;
		FogPlantTestTick(theBoard, gPlayTicks);
	}
	else if (aNightTest)
	{
		if (!gLaidOut)
		{
			gLaidOut = true;
			LayOutNight(theBoard);
		}
		theBoard->mSunMoney = 5000;
		NightTick(theBoard);
	}
	else if (PoolPlantTestActive(theBoard)) // Pool-plant layout (Mod/Engine/PoolPlantSites.h)
	{
		theBoard->mSunMoney = 5000;
		PoolPlantTestTick(theBoard, gPlayTicks);
	}
	else if (CmdFlag("-pvzu-plant-test"))
	{
		if (!gLaidOut)
		{
			gLaidOut = true;
			LayOut(theBoard);
		}
		theBoard->mSunMoney = 5000;
		if (gPlayTicks % 500 == 0) // walkers for the shooters, the mine and the Chompers
		{
			theBoard->AddZombieInRow(ZombieType::ZOMBIE_NORMAL, 0, 0);
			theBoard->AddZombieInRow(ZombieType::ZOMBIE_NORMAL, 2, 0);
			const ZombieType aChomperFood[] = {ZombieType::ZOMBIE_NORMAL, ZombieType::ZOMBIE_TRAFFIC_CONE, ZombieType::ZOMBIE_PAIL};
			theBoard->AddZombieInRow(aChomperFood[(gPlayTicks / 500) % 3], 3, 0);
		}
		if (gPlayTicks % 700 == 0) // walkers for the mines the Cherry Bombs leave on row 4
			theBoard->AddZombieInRow(ZombieType::ZOMBIE_NORMAL, 4, 0);
		if (gPlayTicks % 1500 == 0) // a new Potato Mine where the old one blew up
			Put(theBoard, 6, 0, SeedType::SEED_POTATOMINE);
		if (gPlayTicks % 300 == 0) // Cherry Bombs on row 4, each on its own fuse
		{
			Put(theBoard, gCherryCol, 4, SeedType::SEED_CHERRYBOMB);
			gCherryCol = gCherryCol >= 8 ? 4 : gCherryCol + 1;
		}
	}

	NightDevTick(theBoard, gPlayTicks); // -pvzu-dev-night scenarios (Mod/Engine/NightLevels.h)
	PoolDevTick(theBoard, gPlayTicks); // start-sun log line, -pvzu-dev-pool scenarios (Mod/Engine/PoolLevels.h)
	FogDevTick(theBoard, gPlayTicks); // fog log line on fog lawns, -pvzu-dev-fog (Mod/Engine/FogLevels.h)
	RoofDevTick(theBoard, gPlayTicks); // roof play line, -pvzu-dev-roof scenarios (Mod/Engine/RoofLevels.h)
	LaterLevelsDevTick(theBoard, gPlayTicks); // 51-73 play line, -pvzu-dev-win (Mod/Engine/LaterLevels.h)
	ModPlantsDevTick(theBoard, gPlayTicks); // -pvzu-dev-mod-plants / -list-plants / -mod-bank (Mod/Engine/ModPlants.h)
	ModesDevTick(theBoard, gPlayTicks); // vanilla-mode lines, -pvzu-dev-modes scenarios (Mod/Engine/ModesVanilla.h)
	SpecialsDevTick(theBoard, gPlayTicks); // -pvzu-dev-specials (Mod/Engine/ModPlantsSpecials.h)
	ModPlantsShadowDevTick(theBoard, gPlayTicks); // -pvzu-dev-shadow-test / -shadow-dump (Mod/Engine/ModPlantsShadow.h)
	SmallRulesDevTick(theBoard, gPlayTicks); // -pvzu-dev-small (Mod/Engine/SmallRules.h, Plan 9 R9)

	const int aQuitAfter = CmdOptionInt("-pvzu-quit-after", 0);
	if (aQuitAfter > 0 && gPlayTicks >= aQuitAfter)
	{
		std::fprintf(stderr, "PVZU dev: quit after %d play ticks\n", gPlayTicks);
		std::fflush(nullptr);
		std::_Exit(0);
	}
}

} // namespace pvzu::engine
