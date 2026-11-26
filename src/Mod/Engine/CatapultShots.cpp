// PVZU: Plan 9 R3, engine side of the Catapult projectiles 0x12 / 0x20 and the vase Catapult's volley.
#include "Mod/Engine/CatapultShots.h"

#include "Mod/CmdLine.h"
#include "Mod/Engine/EliteZombies.h"
#include "Mod/GameData.h"
#include "Mod/Unnamed/CatapultShots.h"
#include "Mod/Unnamed/ModPlants.h"

#include "LawnApp.h"
#include "Lawn/Board.h"
#include "Lawn/GridItem.h"
#include "Lawn/Plant.h"
#include "Lawn/Projectile.h"
#include "Lawn/Zombie.h"
#include "PvzpLib/PvzpCommon.h"
#include "PvzpLib/Reanimator.h"

#include <string>
#include <vector>

namespace u = pvzu::unnamed;

namespace pvzu::engine {

namespace {

bool gVolley = false;   // exe [0x701240]: set while the vase Catapult throws its volley

int BoardTime(const Zombie* z) { return z && z->mBoard ? static_cast<int>(z->mBoard->mMainCounter) : 0; }
int Id(Zombie* z) { return z && z->mBoard ? static_cast<int>(z->mBoard->ZombieGetID(z)) & 0xFFFF : -1; }
int ShotTime(const Projectile* p) { return p->mBoard ? static_cast<int>(p->mBoard->mMainCounter) : 0; }

const std::string& DevShot() {
	static const std::string s = CmdOption("-pvzu-dev-catapult-shot").value_or("");
	return s;
}

constexpr const char* kTracks[4] = {"Zombie_catapult_basketball", "Zombie_catapult_basketball2",
	"Zombie_catapult_basketball3", "Zombie_catapult_basketball4"};

int BaseLaunchRate(const Plant* thePlant) { // 0x91038e: vanilla table < 0x50, the mod table for mod seeds
	if (thePlant->mPvzu.modSeed != 0)
		if (const u::ModPlant* p = u::FindModPlant(thePlant->mPvzu.modSeed); p && p->launchRate > 0)
			return p->launchRate;
	return GetPlantDefinition(thePlant->mSeedType).mLaunchRate;
}

int RollVaseZombie() { // 0x7a1426..0x7a1451
	const int aRange = u::VaseZombieRollRange();
	for (int i = 0; i < 10000; ++i) {
		const int z = Sexy::Rand(aRange);
		if (u::VaseZombieAllowed(z))
			return z;
	}
	return static_cast<int>(ZombieType::ZOMBIE_NORMAL);
}

void LeaveVase(Board* theBoard, int theCol, int theRow, int theZombie) { // 0x7aa450
	GridItem* aPot = theBoard->mGridItems.DataArrayAlloc();
	aPot->mGridItemType = GridItemType::GRIDITEM_SCARY_POT;
	aPot->mGridItemState = GridItemState::GRIDITEM_STATE_SCARY_POT_QUESTION;   // +0xc = 3
	aPot->mGridX = theCol;
	aPot->mGridY = theRow;
	aPot->mRenderOrder = Board::MakeRenderOrder(RenderLayer::RENDER_LAYER_PLANT, theRow, 0);   // 302000 + row * 10000
	aPot->mScaryPotType = ScaryPotType::SCARYPOT_ZOMBIE;                        // +0x44 = 2
	aPot->mZombieType = static_cast<ZombieType>(theZombie);
	aPot->mSeedType = SeedType::SEED_NONE;                                     // +0x40 = -1
	aPot->mSunCount = -1;                                                      // +0x50 = -1
}

} // namespace

bool CatapultShotsOn() { return GameData::Get().Feature("catapultShots"); }

bool CatapultVolley(Zombie* theCatapult) { // hooks 547 / 548: 0x958ec0 -> 0x910135..0x910234
	if (!CatapultShotsOn() || theCatapult->mBoard == nullptr)
		return false;
	const int aRange = u::CatapultVolleyRollRange(theCatapult->mPvzu.variant, theCatapult->mMindControlled);
	if (aRange <= 0)
		return false;
	int aRoll = Sexy::Rand(aRange);
	if (DevShot() == "volley")
		aRoll = 0;
	else if (DevShot() == "spray")
		aRoll = aRange - 1;
	if (!u::CatapultVolleys(aRoll))
		return false;
	std::vector<Plant*> aPlants;
	std::vector<u::VolleyCandidate> aCandidates;
	for (Plant* aPlant : theCatapult->mBoard->mPlants) { // Board::IteratePlants 0x41c950 (skips dead ones)
		if (aPlant->mDead || aPlant->mRow != theCatapult->mRow)
			continue;
		aPlants.push_back(aPlant);
		aCandidates.push_back({aPlant->mPlantCol, static_cast<int>(aPlant->mSeedType), aPlant->mPvzu.modSeed});
	}
	const std::vector<int> aTargets = u::VolleyTargets(aCandidates);
	const int aBefore = theCatapult->mSummonCounter;
	gVolley = true;
	for (int i : aTargets) {
		theCatapult->mSummonCounter--;                 // 0x910201
		theCatapult->ZombieCatapultFire(aPlants[static_cast<size_t>(i)]);
	}
	gVolley = false;
	theCatapult->mSummonCounter = u::VolleyBallsAfter(theCatapult->mSummonCounter);   // 0x910217
	LogZombieEvent("t=%d catapultShots volley CATAPULT#%d row=%d shots=%d balls=%d->%d", BoardTime(theCatapult),
		Id(theCatapult), theCatapult->mRow, static_cast<int>(aTargets.size()), aBefore, theCatapult->mSummonCounter);
	return true;
}

bool CatapultVolleyShot(Projectile* theShot) { // hook 537 0x7a1945: [0x701240] == 1
	if (!gVolley)
		return false;
	const int aRange = u::VolleyShotRollRange();
	int aRoll = aRange > 0 ? Sexy::Rand(aRange) : 0;
	if (DevShot() == "vase")
		aRoll = 1;
	theShot->mPvzu.modType = u::VolleyShotType(aRoll);
	return true;
}

void CatapultTagShot(Zombie* theCatapult, Projectile* theShot, int theExeType) {
	if (!CatapultShotsOn() || !u::IsCatapultShotType(theExeType))
		return;
	theShot->mPvzu.modType = theExeType;
	LogZombieEvent("t=%d catapultShots shot CATAPULT#%d form=0x%x type=0x%x row=%d", BoardTime(theCatapult), Id(theCatapult),
		theCatapult->mPvzu.variant, theExeType, theShot->mRow);
}

int CatapultDevShotRoll(int theVariant, int theRoll) {
	if ((DevShot() == "spray" && theVariant == 0x70) || (DevShot() == "vase" && theVariant == 0x19))
		return 0;
	return theRoll;
}

void CatapultBallTracks(Zombie* theCatapult) { // hook 549 0x91027a
	if (!CatapultShotsOn())
		return;
	const int n = u::CatapultHiddenBallTracks(theCatapult->mSummonCounter);
	for (int i = 0; i < n && i < 4; ++i)
		theCatapult->ReanimShowTrack(kTracks[i], RENDER_GROUP_HIDDEN);
}

int CatapultShotHitsPlant(Projectile* theShot, Plant* thePlant, int theVanillaDamage) {
	const int aType = theShot->mPvzu.modType;
	if (!CatapultShotsOn() || !u::IsCatapultShotType(aType))
		return theVanillaDamage;
	Board* aBoard = theShot->mBoard;
	if (aType == u::kShotVase) {
		if (!u::VaseShotMakesVase(aType, theShot->mDamageRangeFlags))
			return theVanillaDamage;
		const int aCol = thePlant->mPlantCol, aRow = thePlant->mRow;
		int aKilled = 0;
		for (Plant* p : aBoard->mPlants) { // 0x7aa380: every plant of the cell
			if (p->mDead || p->mPlantCol != aCol || p->mRow != aRow)
				continue;
			if (!u::VaseShotKillsPlant(static_cast<int>(p->mSeedType), static_cast<int>(p->mState), p->mSquished, p->mDead,
					static_cast<int>(p->mOnBungeeState)))
				continue;
			aBoard->mPlantsEaten++;   // Board+0x5798
			p->Die();
			++aKilled;
		}
		const int aZombie = RollVaseZombie();
		LeaveVase(aBoard, aCol, aRow, aZombie);
		LogZombieEvent("t=%d catapultShots vase col=%d row=%d killed=%d zombie=%d", ShotTime(theShot), aCol, aRow, aKilled, aZombie);
		const int aDamage = u::VaseShotDamage();
		return aDamage > 0 ? aDamage : theVanillaDamage;
	}
	// bug spray 0x20 (hook 394 0x940031 -> 0x9102e7)
	const int aDamage = u::SprayDamage() > 0 ? u::SprayDamage() : theVanillaDamage;
	thePlant->mPlantHealth -= aDamage;
	if (const int aFoley = u::SprayFoley(); aFoley >= 0)
		theShot->mApp->PlayFoley(static_cast<FoleyType>(aFoley));
	if (const int aParticle = u::SprayParticle(); aParticle >= 0) {
		const auto [dx, dy] = u::SprayParticleOffset();
		theShot->mApp->AddPvzpParticle(thePlant->mX + dx, thePlant->mY + dy, u::SprayParticleRenderOrder(),
			static_cast<ParticleEffect>(aParticle));
	}
	const int aRange = u::SprayKillRollRange();
	const int aRateBefore = thePlant->mLaunchRate;
	const bool aKills = aRange > 0 && u::SprayKills(Sexy::Rand(aRange));
	if (aKills)
		thePlant->mPlantHealth -= thePlant->mPlantHealth + 1;   // 0x910384: damage HP + 1
	else
		thePlant->mLaunchRate = u::SprayLaunchRate(thePlant->mLaunchRate, BaseLaunchRate(thePlant));
	LogZombieEvent("t=%d catapultShots spray seed=%d col=%d row=%d damage=%d %s hp=%d rate=%d->%d", ShotTime(theShot),
		static_cast<int>(thePlant->mSeedType), thePlant->mPlantCol, thePlant->mRow, aDamage, aKills ? "kills" : "slows",
		thePlant->mPlantHealth, aRateBefore, thePlant->mLaunchRate);
	return 0;
}

} // namespace pvzu::engine
