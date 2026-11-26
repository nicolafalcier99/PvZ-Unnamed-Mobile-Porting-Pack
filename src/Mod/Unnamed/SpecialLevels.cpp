#include "Mod/Unnamed/SpecialLevels.h"

#include "Mod/Unnamed/Rules.h"

#include <cmath>
#include <cstdlib>

namespace pvzu::unnamed {

namespace {
int R(const char* theKey, int theVanilla) { return RuleInt(theKey, theVanilla); }

bool InList(const char* theKey, int theValue) {
	const nlohmann::json* aList = RuleArray(theKey);
	if (aList == nullptr)
		return false;
	for (const nlohmann::json& v : *aList)
		if (v.is_number_integer() && v.get<int>() == theValue)
			return true;
	return false;
}

float Speed() { return RuleFloat("specialLevels.starSnake.speed", 0.0f); }
} // namespace

int SpecialSeedSlots(SpecialLevel theSpecial, int theVanilla) {
	int aSlots = -1;
	if (theSpecial == SpecialLevel::Shooter)
		aSlots = R("specialLevels.shooter.seedSlots", -1);
	else if (theSpecial == SpecialLevel::StarSnake)
		aSlots = R("specialLevels.starSnake.seedSlots", -1);
	return aSlots < 0 ? theVanilla : aSlots;
}

// ---- 6-5 ----

ShooterSetup ShooterStart() {
	ShooterSetup s;
	s.seed = R("specialLevels.shooter.plantSeed", -1);
	s.row = R("specialLevels.shooter.plantRow", 0);
	s.col = R("specialLevels.shooter.plantCol", 0);
	s.launchRate = R("specialLevels.shooter.launchRate", -1);
	s.startSun = R("specialLevels.shooter.startSun", -1);
	return s;
}

int ShooterMessageTime() { return R("specialLevels.shooter.messageTime", -1); }
int ShooterFirstWaveCountdown() { return R("specialLevels.shooter.firstWaveCountdown", -1); }

int ShooterKillSun(int theZombieValue) { return theZombieValue * R("specialLevels.shooter.killSunMultiplier", 0); }

int ShooterUpgrade(int theSeed, int theSun) {
	// 0x7985c8..0x798601: sun >= 2000 and a Melon-pult -> Winter Melon; else (sun >= 500) a Cabbage-pult -> Melon-pult
	const int aFrom2 = R("specialLevels.shooter.upgrade2.from", -1);
	if (aFrom2 >= 0 && theSeed == aFrom2 && theSun >= R("specialLevels.shooter.upgrade2.sun", 0))
		return R("specialLevels.shooter.upgrade2.to", -1);
	const int aFrom1 = R("specialLevels.shooter.upgrade1.from", -1);
	if (aFrom1 >= 0 && theSeed == aFrom1 && theSun >= R("specialLevels.shooter.upgrade1.sun", 0))
		return R("specialLevels.shooter.upgrade1.to", -1);
	return -1;
}

int ShooterLaunchRate(int theSeed) {
	const int aRate = R("specialLevels.shooter.launchRate", -1);
	if (aRate < 0)
		return -1;
	// 0x798629..0x798643: 0x32 for every plant of this path (0x60 / 0x40 are mode 0x72's Gloom-shroom / Repeater)
	const bool aShooterPlant = theSeed == R("specialLevels.shooter.plantSeed", -1) ||
		theSeed == R("specialLevels.shooter.upgrade1.to", -1) || theSeed == R("specialLevels.shooter.upgrade2.to", -1);
	return aShooterPlant ? aRate : -1;
}

Steer SteerFromArrow(int theKeyCode) {
	if (theKeyCode == 0x26)
		return Steer::Up;
	if (theKeyCode == 0x28)
		return Steer::Down;
	return Steer::None;
}

int ShooterStepRow(int theRow, Steer theDir, int theNumRows) {
	if (theDir == Steer::Up && theRow > 0)
		return theRow - 1;
	if (theDir == Steer::Down && theRow < theNumRows - 1)
		return theRow + 1;
	return theRow;
}

bool ShooterHalvesHp(int theZombieType) {
	const int aBelow = R("specialLevels.shooter.halfHpBelowType", -1);
	return aBelow >= 0 && theZombieType >= 0 && theZombieType < aBelow;
}

void ShooterHalveHp(ZombieHp& theHp) {
	// 0x799787 also halves +0xd4 / +0xe0 (helmet / shield max), but ZombieInitialize then stores the full values there
	// (0x523fc4 / 0x523fca); the body max was stored before (0x797e04). So only the current HP ends up halved.
	theHp.body >>= 1;
	theHp.helm >>= 1;
	theHp.shield >>= 1;
}

// ---- touch ----

Touch ClassifyTouch(int theDx, int theDy) {
	Touch t;
	const int aMin = R("specialLevels.touch.swipeMin", 40);
	if (std::abs(theDx) < aMin && std::abs(theDy) < aMin)
		return t;
	t.swipe = true;
	if (std::abs(theDy) >= std::abs(theDx))
		t.dir = theDy < 0 ? Steer::Up : Steer::Down;
	else
		t.dir = theDx < 0 ? Steer::Left : Steer::Right;
	return t;
}

int ShooterTouchRow(int thePlantRow, const Touch& theTouch, int theTapRow, int theNumRows) {
	if (!theTouch.swipe)
		return theTapRow >= 0 && theTapRow < theNumRows ? theTapRow : thePlantRow;
	return ShooterStepRow(thePlantRow, theTouch.dir, theNumRows);
}

// ---- 7-5 ----

Steer SteerFromKey(int theKeyCode) {
	switch (theKeyCode) {
	case 'W': return Steer::Up;
	case 'A': return Steer::Left;
	case 'S': return Steer::Down;
	case 'D': return Steer::Right;
	default: return Steer::None;
	}
}

void ApplyTurn(Steer theDir, float& theVx, float& theVy) {
	const float aSpeed = Speed();
	switch (theDir) {
	case Steer::Up: theVy = -aSpeed; theVx = 0.0f; break;
	case Steer::Down: theVy = aSpeed; theVx = 0.0f; break;
	case Steer::Left: theVx = -aSpeed; theVy = 0.0f; break;
	case Steer::Right: theVx = aSpeed; theVy = 0.0f; break;
	case Steer::None: break;
	}
}

bool SteerHead(Steer theDir, float& theVx, float& theVy) {
	if (Speed() <= 0.0f || theDir == Steer::None)
		return false;
	// 0x79b40e: `if (head->mVelY == 0)` W / S, else A / D
	const bool aLevel = theVy == 0.0f;
	if (aLevel != (theDir == Steer::Up || theDir == Steer::Down))
		return false;
	ApplyTurn(theDir, theVx, theVy);
	return true;
}

bool StarTurns::Record(int theIndex, int theAge, Steer theDir) {
	if (theIndex < 0 || theIndex >= R("specialLevels.starSnake.maxTurns", 0))
		return false;
	const size_t aAt = 2 * static_cast<size_t>(theIndex);
	if (aAt >= mTurns->size())
		mTurns->resize(aAt + 2, 0);
	(*mTurns)[aAt] = theAge;
	(*mTurns)[aAt + 1] = static_cast<int32_t>(theDir);
	return true;
}

bool StarTurns::TurnAt(int theIndex, int theAge, Steer& theDir) const {
	const size_t aAt = 2 * static_cast<size_t>(theIndex);
	if (theIndex < 0 || aAt + 1 >= mTurns->size())
		return false;
	const int32_t aAge = (*mTurns)[aAt];
	const int32_t aDir = (*mTurns)[aAt + 1];
	if (aAge == 0 || aAge != theAge || aDir <= static_cast<int32_t>(Steer::None) || aDir > static_cast<int32_t>(Steer::Right))
		return false;
	theDir = static_cast<Steer>(aDir);
	return true;
}

std::vector<SnakeStar> SnakeStartStars() {
	std::vector<SnakeStar> aStars;
	const nlohmann::json* aList = RuleArray("specialLevels.starSnake.startStars");
	if (aList == nullptr)
		return aStars;
	for (const nlohmann::json& v : *aList)
		if (v.is_array() && v.size() == 3)
			aStars.push_back({v[0].get<int>(), v[1].get<int>(), v[2].get<int>()});
	return aStars;
}

int SnakeStartRow() { return R("specialLevels.starSnake.startRow", 2); }
int SnakeStartRenderOrder() { return R("specialLevels.starSnake.startRenderOrder", 0); }

std::vector<int> SnakeBankSeeds() {
	std::vector<int> aSeeds;
	if (const nlohmann::json* aList = RuleArray("specialLevels.starSnake.bankSeeds"))
		for (const nlohmann::json& v : *aList)
			if (v.is_number_integer())
				aSeeds.push_back(v.get<int>());
	return aSeeds;
}

int SnakeGrowSeed() { return R("specialLevels.starSnake.growSeed", -1); }

SnakeStar SnakeGrowBehind(int theTailX, int theTailY, float theTailVx, float theTailVy, int theTailAge) {
	// 0x7a2172..0x7a218d: fild tail.mY; fld vy; fimul gap; fsubp; fistp (round to nearest, the x87 default)
	const int aGap = R("specialLevels.starSnake.growGapTicks", 0);
	SnakeStar s;
	s.x = static_cast<int>(std::nearbyint(static_cast<double>(theTailX) - static_cast<double>(theTailVx) * aGap));
	s.y = static_cast<int>(std::nearbyint(static_cast<double>(theTailY) - static_cast<double>(theTailVy) * aGap));
	s.age = theTailAge - aGap; // 0x7a21a3
	return s;
}

int SnakeGrowRenderOrder() { return R("specialLevels.starSnake.growRenderOrder", 0); }
int SnakeGrowShadowYOffset() { return R("specialLevels.starSnake.growShadowYOffset", 0); }
bool SnakeStarsPierce() { return R("specialLevels.starSnake.starsPierce", 0) != 0; }

bool StarOutsideField(int theX, int theY) {
	return theX < R("specialLevels.starSnake.fieldMinX", 0) || theX > R("specialLevels.starSnake.fieldMaxX", 899) ||
		theY < R("specialLevels.starSnake.fieldMinY", 40) || theY > R("specialLevels.starSnake.fieldMaxY", 599);
}

// ---- hidden ----

int SpecialWavePoints(SpecialLevel theSpecial, int thePointsBefore, int thePointsAfter) {
	const char* aKey = theSpecial == SpecialLevel::Hidden1   ? "specialLevels.hidden1.wavePointsMultiplier"
	                   : theSpecial == SpecialLevel::Hidden2 ? "specialLevels.hidden2.wavePointsMultiplier" // V2: 0xc13070
	                   : theSpecial == SpecialLevel::Hidden3 ? "specialLevels.hidden3.wavePointsMultiplier" // V2: 0xc13300
	                                                         : nullptr;
	const int aMul = aKey ? R(aKey, -1) : -1;
	return aMul < 0 ? thePointsAfter : thePointsBefore * aMul;
}

bool PlantDrainDue(SpecialLevel theSpecial, int theMainCounter) {
	if (theSpecial != SpecialLevel::Hidden2)
		return false;
	const int aEvery = R("specialLevels.hidden2.drainEvery", -1); // 0xc13033 `mov ebx,6; div ebx; cmp edx,1`
	return aEvery > 0 && theMainCounter >= 0 && theMainCounter % aEvery == R("specialLevels.hidden2.drainPhase", 0);
}

int PlantDrainDamage() { return R("specialLevels.hidden2.drainDamage", 0); } // 0xc13045 push 1 (the damage argument)

bool PlantDrainSkips(int theSeed, int theState, bool theAsleep, bool theIZombieLevel) {
	if (theAsleep || theState == 1) // 0x950005 / 0x950015
		return false;
	if (InList("specialLevels.hidden2.drainSkipStates", theState)) // 0x95001a..0x950031
		return true;
	return !theIZombieLevel && InList("specialLevels.hidden2.drainSkipSeeds", theSeed); // 0x95005c..0x95007d
}

bool AutoPlantDue(SpecialLevel theSpecial, int theCounter) {
	const int aEvery = theSpecial == SpecialLevel::Hidden1 ? R("specialLevels.hidden1.autoPlantEvery", -1) : -1;
	return aEvery > 0 && theCounter > 0 && theCounter % aEvery == 0;
}

AutoPlant HiddenAutoPlant() {
	AutoPlant p;
	p.seed = R("specialLevels.hidden1.autoPlantSeed", -1);
	p.col = R("specialLevels.hidden1.autoPlantCol", 0);
	p.row = R("specialLevels.hidden1.autoPlantRow", 0);
	return p;
}

int SpecialHelmHp(SpecialLevel theSpecial, int theHelmHp) {
	if (theSpecial != SpecialLevel::Hidden1)
		return theHelmHp;
	return theHelmHp * R("specialLevels.hidden1.helmMultiplier", 1);
}

bool SpecialSeedBanned(SpecialLevel theSpecial, int theSeed) {
	if (theSpecial == SpecialLevel::Hidden1)
		return InList("specialLevels.hidden1.bannedSeeds", theSeed);
	if (theSpecial == SpecialLevel::Hidden3)
		return InList("specialLevels.hidden3.bannedSeeds", theSeed);
	return false;
}

bool SpecialColumnPlanting(SpecialLevel theSpecial, int theProfileLevel, int thePlants, int theZombies) {
	if (theSpecial != SpecialLevel::Hidden3 || theProfileLevel != R("specialLevels.hidden3.level", -1))
		return false;
	const int aMax = R("specialLevels.hidden3.columnMaxPlants", -1);
	return aMax >= 0 && thePlants <= aMax && theZombies == 0;
}

int SpecialFirstAllowedWave(SpecialLevel theSpecial, bool theAdventure, int theProfileLevel, int theZombieType,
	int theVanilla) {
	if (theSpecial != SpecialLevel::Hidden3 || !theAdventure || theProfileLevel != R("specialLevels.hidden3.level", -1) ||
		theZombieType != R("specialLevels.hidden3.newspaperType", -1))
		return theVanilla;
	return R("specialLevels.hidden3.newspaperFirstWave", theVanilla);
}

} // namespace pvzu::unnamed
