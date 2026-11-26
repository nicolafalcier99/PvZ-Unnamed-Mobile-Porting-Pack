#include "Mod/Engine/SpecialLevelHooks.h"

#include "Mod/Engine/DevLevels.h"
#include "Mod/Engine/HiddenLevels.h"
#include "Mod/Engine/LaterLevels.h"
#include "Mod/Engine/Level55Shooter.h"
#include "Mod/Engine/Level65Snake.h"
#include "Mod/Engine/Zoom.h"
#include "Mod/GameData.h"
#include "Mod/Unnamed/SpecialLevels.h"

#include "LawnApp.h"
#include "Lawn/Board.h"
#include "Lawn/CursorObject.h"
#include "Lawn/Projectile.h"
#include "Lawn/System/PlayerInfo.h"
#include "Lawn/Zombie.h"

#include <bitset>
#include <format>
#include "Mod/Engine/ModModes.h"

namespace pvzu::engine {

namespace u = pvzu::unnamed;

namespace {
struct Press {
	bool active = false;
	int x = 0, y = 0;
};
Press gPress;

int ProfileLevel(Board* theBoard) {
	return theBoard->mApp->mPlayerInfo ? theBoard->mApp->mPlayerInfo->mLevel : 0;
}
} // namespace

bool SpecialLevelsOn() { return GameData::Get().Feature("specialLevels"); }

u::SpecialLevel ActiveSpecial(const Board* theBoard) {
	return theBoard && SpecialLevelsOn() ? CurrentSpecialLevel(theBoard) : u::SpecialLevel::None;
}

int SpecialBankSlots(Board* theBoard, int theVanilla) {
	return u::SpecialSeedSlots(ActiveSpecial(theBoard), theVanilla);
}

void SpecialZombieSpawned(Zombie* theZombie) {
	switch (ActiveSpecial(theZombie->mBoard)) {
	case u::SpecialLevel::Shooter: ShooterZombieSpawned(theZombie); break;
	case u::SpecialLevel::Hidden1: HiddenZombieSpawned(theZombie); break;
	default: break;
	}
}

void SpecialHeadDropped(Zombie* theZombie) {
	if (ActiveSpecial(theZombie->mBoard) == u::SpecialLevel::Shooter)
		ShooterHeadDropped(theZombie);
}

void SpecialProjectileTick(Projectile* theProjectile) {
	if (SnakeOn(theProjectile->mBoard))
		SnakeProjectileTick(theProjectile);
}

void SpecialChooserClosed(Board* theBoard) { SnakeStart(theBoard); }

bool SpecialPacketClicked(SeedPacket* thePacket) { return SnakePacketClick(thePacket); }

bool SpecialShotSurvivesHit(Projectile* theProjectile) { return SnakeShotSurvivesHit(theProjectile); }

void SpecialProjectileDied(Projectile* theProjectile) { SnakeProjectileDied(theProjectile); }

int SpecialWaveBudget(Board* theBoard, int thePointsBefore, int thePointsAfter) {
	const u::SpecialLevel aSpecial = ActiveSpecial(theBoard);
	const int aPoints = u::SpecialWavePoints(aSpecial, thePointsBefore, thePointsAfter);
	if (aPoints != thePointsAfter)
		DevLog(std::format("special {} wave points before={} vanilla={} now={}", u::SpecialName(aSpecial), thePointsBefore,
			thePointsAfter, aPoints));
	return aPoints;
}

bool SpecialSeedBarred(Board* theBoard, SeedType theSeedType) {
	const u::SpecialLevel aSpecial = ActiveSpecial(theBoard);
	const bool aBarred = u::SpecialSeedBanned(aSpecial, static_cast<int>(theSeedType));
	static std::bitset<NUM_SEED_TYPES> aLogged;
	if (aBarred && theSeedType >= 0 && theSeedType < NUM_SEED_TYPES && !aLogged.test(theSeedType)) {
		aLogged.set(theSeedType);
		DevLog(std::format("special {} chooser barred seed={}", u::SpecialName(aSpecial), static_cast<int>(theSeedType)));
	}
	return aBarred;
}

bool SpecialPlantsColumn(Board* theBoard, const char* theSite) {
	const bool aColumn = u::SpecialColumnPlanting(ActiveSpecial(theBoard), ProfileLevel(theBoard),
		static_cast<int>(theBoard->mPlants.mSize), static_cast<int>(theBoard->mZombies.mSize));
	if (aColumn && theSite)
		DevLog(std::format("special hidden3 t={} column {} plants={} zombies={}", theBoard->mMainCounter, theSite,
			theBoard->mPlants.mSize, theBoard->mZombies.mSize));
	return aColumn;
}

int SpecialFirstWave(Board* theBoard, ZombieType theZombieType, int theVanilla) {
	const int aWave = u::SpecialFirstAllowedWave(ActiveSpecial(theBoard), theBoard->mApp->IsAdventureMode(),
		ProfileLevel(theBoard), static_cast<int>(theZombieType), theVanilla);
	static bool aLogged = false;
	if (aWave != theVanilla && !aLogged) {
		aLogged = true;
		DevLog(std::format("special hidden3 first wave type={} vanilla={} now={}", static_cast<int>(theZombieType),
			theVanilla, aWave));
	}
	return aWave;
}

int SpecialIntroBossTime(Board* theBoard, int theVanilla) {
	if (!SpecialLevelsOn() || !LaterLevelsOn() || !theBoard->mApp->IsAdventureMode() || !u::UsesLaterTable(theBoard->mLevel))
		return theVanilla;
	const u::LaterLevel* aRow = u::FindLaterLevel(theBoard->mLevel);
	if (aRow == nullptr || aRow->introTimer == u::kNoLaterRule)
		return theVanilla;
	DevLog(std::format("special intro level={} bossTime={}", theBoard->mLevel, aRow->introTimer));
	return aRow->introTimer;
}

void SpecialTouchDown(Board* theBoard, int x, int y, bool theOnBareLawn) {
	ModModeFollowPress(theBoard, x, y, theOnBareLawn); // 0x78 / 0x8b follow a finger only from a lawn press
	gPress.active = false;
	const u::SpecialLevel aSpecial = ActiveSpecial(theBoard);
	if (aSpecial != u::SpecialLevel::Shooter && !SnakeOn(theBoard) && !ModModeTakesTouch(theBoard))
		return;
	if (!theOnBareLawn || theBoard->mApp->mGameScene != GameScenes::SCENE_PLAYING || theBoard->mPaused ||
		theBoard->mCursorObject->mCursorType != CursorType::CURSOR_TYPE_NORMAL)
		return;
	gPress = {true, x, y};
}

void SpecialTouchUp(Board* theBoard, int x, int y) {
	ModModeFollowLift();
	if (!gPress.active)
		return;
	gPress.active = false;
	const u::SpecialLevel aSpecial = ActiveSpecial(theBoard);
	if (theBoard->mApp->mGameScene != GameScenes::SCENE_PLAYING || theBoard->mPaused)
		return;
	LocalToBoard(theBoard, x, y);
	const u::Touch aTouch = u::ClassifyTouch(x - gPress.x, y - gPress.y);
	if (aSpecial == u::SpecialLevel::Shooter)
		ShooterTouch(theBoard, aTouch, gPress.x, gPress.y);
	else if (ModModeTakesTouch(theBoard)) // mod modes 0x72 (Peashooter) and 0x8e (Gloom-shroom) move the same way
		ModModeTouch(theBoard, aTouch, gPress.x, gPress.y);
	else if (SnakeOn(theBoard) && aTouch.swipe) // 7-5 and mode 0x89
		SnakeSteer(theBoard, aTouch.dir, "swipe");
}

} // namespace pvzu::engine
