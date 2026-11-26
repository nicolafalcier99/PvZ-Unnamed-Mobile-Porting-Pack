#include "Mod/Engine/Level65Snake.h"

#include "Mod/Engine/DevLevels.h"
#include "Mod/Engine/ModModes.h"
#include "Mod/Engine/SpecialLevelHooks.h"
#include "Mod/Unnamed/ModModeRules.h"

#include "LawnApp.h"
#include "Lawn/Board.h"
#include "Lawn/Projectile.h"
#include "Lawn/SeedPacket.h"

#include <format>

namespace pvzu::engine {

namespace u = pvzu::unnamed;

namespace {
int gReplayLogs = 0;

const char* DirName(u::Steer theDir) {
	switch (theDir) {
	case u::Steer::Up: return "W";
	case u::Steer::Down: return "S";
	case u::Steer::Left: return "A";
	case u::Steer::Right: return "D";
	default: return "-";
	}
}

u::StarTurns Turns(Board* theBoard) { return u::StarTurns(theBoard->mPvzuState.starTurns); }

unsigned int IndexOf(Projectile* theProjectile) { return theProjectile->mBoard->mProjectiles.DataArrayGetIndex(theProjectile); }
unsigned int IdOf(Projectile* theProjectile) { return theProjectile->mBoard->mProjectiles.DataArrayGetID(theProjectile); }

// One item of the projectile array as the exe's code sees it: the object in that slot (dead or not), or, once the slot is
// freed, the copy of its last state (the exe reads the freed memory, which keeps it until the slot is used again).
struct Slot {
	Projectile* live = nullptr;
	u::StarSlot* dead = nullptr;
	explicit operator bool() const { return live || dead; }
	int Type() const { return live ? static_cast<int>(live->mProjectileType) : dead->type; }
	int X() const { return live ? live->mX : dead->x; }
	int Y() const { return live ? live->mY : dead->y; }
	float& VelX() const { return live ? live->mVelX : dead->velX; }
	float& VelY() const { return live ? live->mVelY : dead->velY; }
	int Age() const { return live ? live->mProjectileAge : dead->age; }
	int Tail() const { return live ? live->mCobTargetRow : dead->tail; }
	void SetTail(int theId) const { (live ? live->mCobTargetRow : dead->tail) = theId; }
	int TurnCount() const { return live ? static_cast<int>(live->mTargetZombieID) : dead->turns; }
	void SetTurnCount(int theTurns) const {
		if (live)
			live->mTargetZombieID = static_cast<ZombieID>(theTurns);
		else
			dead->turns = theTurns;
	}
};

Slot SlotAt(Board* theBoard, unsigned int theIndex) {
	auto& aArray = theBoard->mProjectiles;
	if (theIndex < aArray.mMaxUsedCount && (aArray.DataArrayGetIDAt(theIndex) & DATA_ARRAY_KEY_MASK))
		return {&aArray.DataArrayGetItemAt(theIndex), nullptr};
	for (u::StarSlot& s : theBoard->mPvzuState.starSlots)
		if (s.index == static_cast<int>(theIndex))
			return {nullptr, &s};
	return {};
}

void SetupStar(Projectile* theStar, int theAge) {
	theStar->mMotionType = ProjectileMotion::MOTION_STAR; // +0x58 = 7
	theStar->mDamageRangeFlags = 1;                       // +0x74 = 1
	theStar->mProjectileAge = theAge;
}
} // namespace

bool SnakeOn(const Board* theBoard) {
	if (theBoard == nullptr)
		return false;
	if (ActiveSpecial(theBoard) == u::SpecialLevel::StarSnake)
		return true;
	LawnApp* anApp = theBoard->mApp;
	return static_cast<int>(anApp->mGameMode) == u::kGluttonStar && IsModModeRunning(anApp);
}

void SnakeInit(Board* theBoard) {
	if (!SnakeOn(theBoard))
		return;
	Turns(theBoard).Reset();
	theBoard->mPvzuState.starSlots.clear();
	gReplayLogs = 0;
}

void SnakeStart(Board* theBoard) {
	if (!SnakeOn(theBoard))
		return;
	SnakeInit(theBoard); // 0x7a2231: the table is cleared at every Let's Rock
	const std::vector<u::SnakeStar> aStars = u::SnakeStartStars();
	Projectile* aHead = nullptr;
	Projectile* aLast = nullptr;
	for (const u::SnakeStar& s : aStars) {
		Projectile* aStar = theBoard->AddProjectile(s.x, s.y, u::SnakeStartRenderOrder(), u::SnakeStartRow(),
			ProjectileType::PROJECTILE_STAR);
		SetupStar(aStar, aStar->mProjectileAge + s.age);
		u::ApplyTurn(u::Steer::Right, aStar->mVelX, aStar->mVelY); // vx = 3.0
		if (aHead == nullptr)
			aHead = aStar;
		aLast = aStar;
	}
	if (aHead && aLast && aLast != aHead)
		aHead->mCobTargetRow = static_cast<int>(IdOf(aLast)); // 0x7a22f2: head+0x84 = tail id
	SeedBank* aBank = theBoard->mSeedBank.get();
	const std::vector<int> aSeeds = u::SnakeBankSeeds();
	for (size_t i = 0; aBank && i < aSeeds.size() && i < static_cast<size_t>(aBank->mNumPackets); i++)
		aBank->mSeedPackets[i].mPacketType = static_cast<SeedType>(aSeeds[i]); // 0x7a2320: only the type
	DevLog(std::format("special snake start stars={} head=item{} tail=0x{:x} bank={}", aStars.size(),
		aHead ? IndexOf(aHead) : 0, aHead ? aHead->mCobTargetRow : 0, aSeeds.size()));
}

bool SnakePacketClick(SeedPacket* thePacket) {
	Board* aBoard = thePacket->mBoard;
	if (!SnakeOn(aBoard) || static_cast<int>(thePacket->mPacketType) != u::SnakeGrowSeed())
		return false;
	// 0x7a2124: the Starfruit's cost goes first, whatever follows (the vanilla tests above checked it can be paid)
	const int aCost = aBoard->GetCurrentPlantCost(thePacket->mPacketType, thePacket->mImitaterType);
	aBoard->mSunMoney -= aCost;
	const Slot aHead = SlotAt(aBoard, 0);
	if (!aHead || aHead.Type() != static_cast<int>(ProjectileType::PROJECTILE_STAR) || aHead.Tail() == 0) {
		DevLog(std::format("special snake t={} grow paid={} no head", aBoard->mMainCounter, aCost));
		return true;
	}
	const Slot aTail = SlotAt(aBoard, static_cast<unsigned int>(aHead.Tail()) & DATA_ARRAY_INDEX_MASK); // and edi,0xffff
	if (!aTail) {
		DevLog(std::format("special snake t={} grow paid={} tail slot never used", aBoard->mMainCounter, aCost));
		return true;
	}
	const u::SnakeStar aAt = u::SnakeGrowBehind(aTail.X(), aTail.Y(), aTail.VelX(), aTail.VelY(), aTail.Age());
	const float aVx = aTail.VelX(), aVy = aTail.VelY();
	const int aTurns = aTail.TurnCount();
	Projectile* aStar = aBoard->AddProjectile(aAt.x, aAt.y, u::SnakeGrowRenderOrder(), u::SnakeStartRow(),
		ProjectileType::PROJECTILE_STAR);
	SetupStar(aStar, aAt.age);
	aStar->mVelX = aVx;
	aStar->mVelY = aVy;
	aStar->mPosX = static_cast<float>(aStar->mX);                             // 0x7a21b8
	aStar->mShadowY = static_cast<float>(aStar->mY + u::SnakeGrowShadowYOffset()); // 0x7a21be
	aStar->mTargetZombieID = static_cast<ZombieID>(aTurns);                  // 0x7a21ca: the tail's turn count
	SlotAt(aBoard, 0).SetTail(static_cast<int>(IdOf(aStar)));                // 0x7a21dc: item 0 (maybe the new star itself)
	DevLog(std::format("special snake t={} grow paid={} star=item{} at=({},{}) age={} turns={} sun={}", aBoard->mMainCounter,
		aCost, IndexOf(aStar), aStar->mX, aStar->mY, aStar->mProjectileAge, aTurns, aBoard->mSunMoney));
	return true; // 0x7a21e3: no cursor, no recharge
}

bool SnakeSteer(Board* theBoard, u::Steer theDir, const char* theSource) {
	if (theDir == u::Steer::None || theBoard->mApp->mGameScene != GameScenes::SCENE_PLAYING) // 0x79b3d9: scene 3 only
		return false;
	const Slot aHead = SlotAt(theBoard, 0); // 0x79b411: item 0, whatever it is
	if (!aHead)
		return false;
	float& aVx = aHead.VelX();
	float& aVy = aHead.VelY();
	if (!u::SteerHead(theDir, aVx, aVy))
		return false;
	const int aCount = aHead.TurnCount();
	const bool aRecorded = Turns(theBoard).Record(aCount, aHead.Age(), theDir);
	if (aRecorded)
		aHead.SetTurnCount(aCount + 1);
	DevLog(std::format("special snake t={} steer {} by={} head={} age={} v=({:.1f},{:.1f}) turn={} recorded={} table={}",
		theBoard->mMainCounter, DirName(theDir), theSource, aHead.live ? "live" : "dead", aHead.Age(), aVx, aVy,
		aHead.TurnCount(), aRecorded ? 1 : 0, theBoard->mPvzuState.starTurns.size() / 2));
	return true;
}

bool SnakeKey(Board* theBoard, Sexy::KeyCode theKey) {
	if (!SnakeOn(theBoard))
		return false;
	const u::Steer aDir = u::SteerFromKey(theKey);
	if (aDir == u::Steer::None)
		return false;
	SnakeSteer(theBoard, aDir, "key");
	return true; // the exe marks W / A / S / D as used (0x79b3fa) even when the head cannot turn that way
}

void SnakeProjectileTick(Projectile* theProjectile) {
	if (theProjectile->mProjectileType != ProjectileType::PROJECTILE_STAR) // 0x7a1fd1: type 7 only
		return;
	theProjectile->mVisible = true; // 0x7a1fdb (also the pierce counter's reset, see SnakeShotSurvivesHit)
	theProjectile->mPvzu.pierceHits = 0; // the same reset in the port's counter (Plan 9 R1)
	Board* aBoard = theProjectile->mBoard;
	if (theProjectile->mCobTargetRow != 0) { // 0x7a206d: the head
		if (!u::StarOutsideField(theProjectile->mX, theProjectile->mY) ||
			theProjectile->mApp->mGameScene != GameScenes::SCENE_PLAYING || aBoard->mLevelAwardSpawned)
			return;
		DevLog(std::format("special snake t={} head out at ({},{}) -> level lost", aBoard->mMainCounter, theProjectile->mX,
			theProjectile->mY));
		aBoard->ZombiesWon(nullptr); // exe: mGameScene = 4
		return;
	}
	const int aCount = static_cast<int>(theProjectile->mTargetZombieID);
	u::Steer aDir = u::Steer::None;
	if (!Turns(aBoard).TurnAt(aCount, theProjectile->mProjectileAge, aDir))
		return;
	theProjectile->mTargetZombieID = static_cast<ZombieID>(aCount + 1);
	u::ApplyTurn(aDir, theProjectile->mVelX, theProjectile->mVelY);
	if (gReplayLogs++ < 200)
		DevLog(std::format("special snake t={} replay {} star=item{} age={} turn={} at=({:.0f},{:.0f})", aBoard->mMainCounter,
			DirName(aDir), IndexOf(theProjectile), theProjectile->mProjectileAge, aCount + 1, theProjectile->mPosX,
			theProjectile->mPosY));
}

bool SnakeShotSurvivesHit(Projectile* theProjectile) {
	// Die cave 0x6511e3: a type-7 star with kind (+0x80) <= 0 and motion 7 lives while its counter is below 255; hook 401
	// sets it back to 1 every tick in these modes, so it always lives. Also for the Die of the field edges in
	// CheckForCollision (FogStarSurvivesDie): the head is never removed there, and the next tick's head test loses.
	if (theProjectile->mProjectileType != ProjectileType::PROJECTILE_STAR ||
		theProjectile->mMotionType != ProjectileMotion::MOTION_STAR || theProjectile->mPvzu.modShot != 0 ||
		theProjectile->mPvzu.field80 > 0 || !u::SnakeStarsPierce() || !SnakeOn(theProjectile->mBoard))
		return false;
	static int aHitLogs = 0;
	if (aHitLogs++ < 30)
		DevLog(std::format("special snake t={} star=item{} hit at ({},{}), flies on", theProjectile->mBoard->mMainCounter,
			IndexOf(theProjectile), theProjectile->mX, theProjectile->mY));
	return true;
}

void SnakeProjectileDied(Projectile* theProjectile) {
	Board* aBoard = theProjectile->mBoard;
	if (!SnakeOn(aBoard))
		return;
	const u::StarSlot aCopy{static_cast<int>(IndexOf(theProjectile)), static_cast<int>(theProjectile->mProjectileType),
		theProjectile->mX, theProjectile->mY, theProjectile->mVelX, theProjectile->mVelY, theProjectile->mProjectileAge,
		theProjectile->mCobTargetRow, static_cast<int>(theProjectile->mTargetZombieID)};
	for (u::StarSlot& s : aBoard->mPvzuState.starSlots)
		if (s.index == aCopy.index) {
			s = aCopy;
			return;
		}
	aBoard->mPvzuState.starSlots.push_back(aCopy);
	if (aCopy.tail != 0)
		DevLog(std::format("special snake t={} head died at ({},{}) (item {} keeps its last state)", aBoard->mMainCounter,
			aCopy.x, aCopy.y, aCopy.index));
}

} // namespace pvzu::engine
