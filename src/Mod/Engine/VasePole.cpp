#include "Mod/Engine/VasePole.h"

#include "Mod/Engine/DevLevels.h"
#include "Mod/GameData.h"
#include "Mod/Unnamed/Level70Boss.h"
#include "Mod/Unnamed/State.h"
#include "Mod/Unnamed/VasePole.h"

#include "LawnApp.h"
#include "Lawn/Board.h"
#include "Lawn/Challenge.h"
#include "Lawn/GridItem.h"
#include "Lawn/Plant.h"
#include "Lawn/Zombie.h"
#include "PvzpLib/Attachment.h"
#include "Common.h"

#include <format>
#include <string>

namespace pvzu::engine {

namespace u = pvzu::unnamed;

namespace {

bool gLandingOpens = false; // a landing is opening vases (the ScaryPotterOpenPot tail asks)

std::string Name(int theType) {
	const auto& aZombies = GameData::Get().Active().zombies;
	return theType >= 0 && theType < static_cast<int>(aZombies.size()) ? aZombies[theType].name : std::format("#{}", theType);
}

u::VaseRect ToVase(const Rect& r) { return {r.mX, r.mY, r.mWidth, r.mHeight}; }

// 0x7aa320, intended reading (study 4): max HP above 120000, Zomboss, the level-70 boss form.
bool BossClass(Zombie* z) {
	const int aType = static_cast<int>(z->mZombieType);
	return u::IsBossClass(z->mBodyMaxHealth, aType) || u::IsBoss70(aType, z->mPvzu.variant, z->mBoard->mLevel);
}

// 0xc01d87: GridItem type 7 at (col, row), render order row * 10000 + 302000, state 5, zombie vase; +0x54 = side.
GridItem* PlaceVase(Board* theBoard, int theCol, int theRow, ZombieType theType, bool theHypnotised) {
	if (theBoard->mGridItems.mSize >= theBoard->mGridItems.mMaxSize)
		return nullptr;
	GridItem* aPot = theBoard->mGridItems.DataArrayAlloc();
	aPot->mGridItemType = GridItemType::GRIDITEM_SCARY_POT;
	aPot->mGridItemState = static_cast<GridItemState>(u::VasePolePotState());
	aPot->mGridX = theCol;
	aPot->mGridY = theRow;
	aPot->mRenderOrder = Board::MakeRenderOrder(RenderLayer::RENDER_LAYER_PLANT, theRow, 0);
	aPot->mZombieType = theType;
	aPot->mSeedType = SeedType::SEED_PEASHOOTER; // exe writes 0
	aPot->mScaryPotType = ScaryPotType::SCARYPOT_ZOMBIE;
	aPot->mSunCount = 0;
	aPot->mPvzu.potHypnotised = theHypnotised ? 1 : 0;
	return aPot;
}

// The exe sets mDead = 1 only (no death animation, loot or death effects); the port also frees what DieNoLoot frees
// so nothing leaks (ruling, study 4).
void RemoveSilently(Zombie* z) {
	z->StopZombieSound();
	AttachmentDie(z->mAttachmentID);
	z->mApp->RemoveReanimation(z->mBodyReanimID);
	z->mApp->RemoveReanimation(z->mMoweredReanimID);
	z->mApp->RemoveReanimation(z->mSpecialHeadReanimID);
	z->mApp->RemoveReanimation(z->mZombatarHeadReanimID);
	z->mDead = true;
}

} // namespace

void VasePoleVault(Zombie* thePole) {
	thePole->mPvzu.flags |= u::kZombieFlagVaultGraveMade; // 0xc03711: set before every test below
	Board* aBoard = thePole->mBoard;
	const bool aHypnotised = thePole->mMindControlled;
	const int aCol = u::VasePoleVaultColumn(thePole->mX, aHypnotised);
	const int aRow = thePole->mRow;
	// Ruling: a column off the lawn does nothing (the exe would put the vase, and the catches, at an unreachable cell).
	if (aCol < 0 || aCol >= MAX_GRID_SIZE_X || aRow < 0 || aRow >= MAX_GRID_SIZE_Y) {
		DevLog(std::format("vasePole vault row={} mX={} col={} off-lawn", aRow, thePole->mX, aCol));
		return;
	}

	std::string aOwn = "none";
	if (u::VasePoleMakesOwnVase(aHypnotised)) {
		int aPlants = 0;
		for (Plant* aPlant : aBoard->mPlants)
			if (!aPlant->mDead && aPlant->mRow == aRow && aPlant->mPlantCol == aCol)
				aPlants++;
		thePole->SquishAllInSquare(aCol, aRow, ZombieAttackType::ATTACKTYPE_CHEW); // 0x52e920
		for (Plant* aPlant : aBoard->mPlants) {
			if (aPlant->mRow != aRow || aPlant->mPlantCol != aCol)
				continue;
			if (u::VasePolePlantLeft(aPlant->mDead, aPlant->mSquished, static_cast<int>(aPlant->mOnBungeeState))) {
				DevLog(std::format("vasePole vault row={} mX={} col={} plants={} cancelled: seed {} left", aRow, thePole->mX, aCol,
					aPlants, static_cast<int>(aPlant->mSeedType)));
				return; // 0xc03788: nothing else happens
			}
		}
		const int aType = u::VasePoleRollZombie([](int theRange) { return Sexy::Rand(theRange); });
		if (PlaceVase(aBoard, aCol, aRow, static_cast<ZombieType>(aType), false))
			aOwn = Name(aType);
		aOwn += std::format(" squished={}", aPlants);
	}

	if (!aHypnotised && DevFlag("-pvzu-dev-vase-catch")) { // dev: one of the player's hypnotised zombies in the pole's reach
		Zombie* aBait = aBoard->AddZombieInRow(ZombieType::ZOMBIE_NORMAL, aRow, 0);
		aBait->mPosX = thePole->mPosX;
		aBait->mX = static_cast<int>(aBait->mPosX);
		aBait->mMindControlled = true;
	}

	// 0xc037c7: the other side's zombies in reach become vases in the same cell.
	const unsigned aFlags = u::VasePoleCatchFlags(aHypnotised);
	const u::VaseRect aReach = ToVase(thePole->GetZombieAttackRect());
	std::string aCaught;
	for (Zombie* aZombie : aBoard->mZombies) {
		if (aZombie->mDead)
			continue;
		u::VaseCatchInput aIn;
		aIn.self = aZombie == thePole;
		aIn.bossClass = BossClass(aZombie);
		aIn.sameRow = aZombie->mRow == aRow;
		if (aIn.self || aIn.bossClass || !aIn.sameRow)
			continue;
		aIn.overlap = u::VasePoleRectsOverlap(aReach, ToVase(aZombie->GetZombieRect()));
		aIn.effected = aIn.overlap && aZombie->EffectedByDamage(aFlags);
		if (!u::VasePoleCatches(aIn))
			continue;
		const bool aPotHypnotised = u::VasePoleCaughtPotHypnotised(aZombie->mMindControlled);
		const ZombieType aType = aZombie->mZombieType;
		RemoveSilently(aZombie);
		PlaceVase(aBoard, aCol, aRow, aType, aPotHypnotised);
		aCaught += std::format("{}{}{}", aCaught.empty() ? "" : ",", Name(aType), aPotHypnotised ? "(hypno)" : "");
	}
	DevLog(std::format("vasePole vault row={} mX={} col={} hypno={} vase={} caught={}", aRow, thePole->mX, aCol,
		aHypnotised ? 1 : 0, aOwn, aCaught.empty() ? "none" : aCaught));
}

void VasePoleLanded(Zombie* thePole) {
	if (!u::IsVasePole(thePole->mPvzu.variant) || (thePole->mPvzu.flags & u::kZombieFlagVaultGraveMade) == 0)
		return; // 0xc038c0..0xc038d7
	Board* aBoard = thePole->mBoard;
	const bool aHypnotised = thePole->mMindControlled;
	const int aCol = u::VasePoleLandColumn(aBoard->PixelToGridX(thePole->mX, thePole->mY), aHypnotised);
	int aOpened = 0;
	gLandingOpens = true;
	while (GridItem* aPot = aBoard->GetScaryPotAt(aCol, thePole->mRow)) { // 0xc03900..0xc03927
		aBoard->mChallenge->ScaryPotterOpenPot(aPot);
		if (++aOpened >= MAX_GRID_SIZE_X * MAX_GRID_SIZE_Y)
			break;
	}
	gLandingOpens = false;
	DevLog(std::format("vasePole land row={} mX={} col={} opened={}", thePole->mRow, thePole->mX, aCol, aOpened));
}

void VasePolePotZombie(GridItem* thePot, Zombie* theZombie) {
	if (theZombie == nullptr || !u::VasePolePotSetsSide())
		return;
	theZombie->mMindControlled = thePot->mPvzu.potHypnotised != 0; // a raw byte write, as the exe
	if (thePot->mPvzu.potHypnotised != 0)
		DevLog(std::format("vasePole potZombie {} col={} row={} hypno=1", Name(static_cast<int>(theZombie->mZombieType)),
			thePot->mGridX, thePot->mGridY));
}

bool VasePoleAwardAllowed(Challenge* theChallenge) {
	return !gLandingOpens || u::VasePoleAwardAllowed(theChallenge->mApp->IsScaryPotterLevel());
}

} // namespace pvzu::engine
