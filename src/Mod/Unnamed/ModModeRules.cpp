#include "Mod/Unnamed/ModModeRules.h"

#include <algorithm>
#include <cstdlib>

namespace pvzu::unnamed {

int SlotCrisisSeed(int theRoll6) {
	static constexpr int kSeeds[6] = {1, 4, 5, 7, 3, 27};
	return kSeeds[std::clamp(theRoll6, 0, 5)];
}

int UpstreamPush(int theDamage) { return std::min((theDamage >> 1) + 1, 160); }

float ZombiesRunFactor(int theCurrentWave) {
	return theCurrentWave >= 6 ? 1.0f + 0.125f * static_cast<float>(theCurrentWave - 5) : 1.0f;
}

int HeisenbergRow(int theRow, const std::vector<int>& theRolls) {
	for (int r : theRolls)
		if (r != theRow)
			return r;
	return -1;
}

int SowBeansProjectile(int theType) {
	switch (theType) {
	case 0: return 3;
	case 1: return 5;
	case 13: return 3;
	default: return theType;
	}
}

int GaleTick(GaleState& theState, int theRoll1500) {
	if (theState.timer != 0) {
		theState.timer--;
		return 0;
	}
	theState.timer = theRoll1500 + 2500;
	const int aOld = theState.dir;
	if (theState.dir == 0)
		theState.dir = -1;
	theState.dir = -theState.dir;
	if (aOld == 0)
		return 1;
	return theState.dir > 0 ? 2 : 3;
}

int GaleDrift(int theDir, int theRoll) { return theRoll < 2 ? theRoll * theDir : 0; }

void FrequencyTick(FrequencyState& theState, int theCurrentWave, int theRoll201) {
	if (theCurrentWave == 0)
		return;
	theState.timer--;
	if (theState.timer <= 0) {
		theState.timer = theRoll201 + 400;
		theState.f = 3 - (theState.f == 0 ? 2 : theState.f);
	}
}

int LightSunRadius(int theCoinType) {
	switch (theCoinType) {
	case 4: return 2;
	case 5: return 1;
	case 6: return 3;
	default: return 0;
	}
}

int LightSunRow(int theCenterY) { return (theCenterY - 90) / 85; }

std::array<int, 2> ArtExplosionCoins(int theRoll1, int theRoll2) {
	std::array<int, 2> aOut{-1, -1};
	if (theRoll1 < 2)
		return aOut;
	aOut[0] = theRoll1;
	if (theRoll2 >= 2)
		aOut[1] = theRoll2;
	return aOut;
}

int LoneWolfUpgrade(int theSeed, int theSun) {
	if (theSun >= 5000 && theSeed == 18)
		return 40;
	if (theSun >= 2000 && theSeed == 7)
		return 18;
	if (theSun >= 750 && theSeed == 5)
		return 7;
	if (theSun >= 100 && theSeed == 0)
		return 5;
	return theSeed;
}

DownstreamPush DownstreamBite(int theX, int theRoll41) {
	DownstreamPush p;
	p.x = theX - (theRoll41 + 40);
	p.offLawn = p.x <= -32;
	p.col = p.x < 0 ? 0 : p.x / 80;
	return p;
}

bool AttritionFlips(int theCurrentWaveAfterSpawn) {
	return theCurrentWaveAfterSpawn != 0 && theCurrentWaveAfterSpawn % 2 == 0;
}

bool AttritionNocturnal(int theSeed) {
	switch (theSeed) {
	case 8: case 9: case 10: case 12: case 13: case 14: case 15: case 24: case 31: case 42: return true;
	default: return false;
	}
}

// ---- second pass ----

int SunStormCountdown(int theVanilla) { return static_cast<int>(std::min(static_cast<unsigned>(theVanilla) >> 1, 450u)); }

bool SunStormHitsZombie(int theCx, int theCy, int theZombieX, int theZombieY) {
	return std::abs(theZombieY - theCy) <= 120 && theZombieX - 125 <= theCx && theCx <= theZombieX + 130;
}

bool SunStormHitsPlant(int theCx, int theCy, int thePlantX, int thePlantY) {
	return std::abs(thePlantY - theCy) <= 100 && thePlantX - 125 <= theCx && theCx <= thePlantX + 130;
}

int SunStormBlastDamage() { return 270; }

bool NightTerrorCrater(int theCol, int theRow) {
	static constexpr const char* kRows[5] = {"..XXX.XXX", "..X.X.X..", "..X.X.XXX", "..X.X...X", "XXX.XXXXX"};
	return theCol >= 0 && theCol < 9 && theRow >= 0 && theRow < 5 && kRows[theRow][theCol] == 'X';
}

TerrorMove NightTerrorStep(int x, int y, int& r) {
	if (x > 660)
		return TerrorMove::Walk;
	if (x >= 655) { // 655..660
		if (r == 2) r = 3;
		if (r == 3 && y == 350) r = 4;
		if (y >= 250 && y <= 443) return TerrorMove::Hold;
	}
	if (r == 2 && x >= 495 && y == 250)
		return TerrorMove::WalkRight;
	if (x >= 495 && x < 500) {
		if (r == 0) r = 1;
		if (r == 1 && y == 150) r = 2;
		if (y >= 50 && y <= 250) return TerrorMove::Hold;
	}
	if (x > 340)
		return TerrorMove::Walk;
	if (x >= 335) {
		if (r == 4) r = 3;
		if (r == 3 && y == 350) r = 2;
		if (r == 2 && y == 250) r = 1;
		if (r == 1 && y == 150) r = 0;
		return y >= 54 && y <= 450 ? TerrorMove::Hold : TerrorMove::Walk;
	}
	if (x >= 170 && x <= 175) {
		if (r == 0) r = 1;
		if (r == 1 && y == 150) r = 2;
		if (r == 2 && y == 250) r = 3;
		if (r == 3 && y == 350) r = 4;
		if (y >= 50 && y <= 443) return TerrorMove::Hold;
	}
	return TerrorMove::Walk;
}

float NightTerrorYStep(float theGap) { return theGap >= 1.0f ? 0.1875f : theGap; }

int NightTerrorHp(int theHp, int theWave) { return theHp * (theWave + 4) >> 2; }

int LoneWolfLaunchRate(int theSeed) { return theSeed == 40 ? 96 : theSeed == 7 ? 64 : 50; }

int LoneWolfKillSun(int theZombieValue) { return theZombieValue * 5; }

TrickMiss TrickOfTricksSmash(int& theCount, int theHits) {
	TrickMiss m;
	if (theHits != 0)
		return m;
	m.counted = true;
	theCount++;
	m.bag = theCount >= kTricksToWin;
	return m;
}

bool TrickOfTricksLost(int theCount) { return theCount < kTricksToWin; }

int TrickHudX(int theCount) { return theCount >= 10 ? 645 : 660; }

int TerritorialStore(int theStored, int theSeed) {
	if (theStored != 0 || theSeed == 48 || theSeed < 0)
		return theStored;
	return theSeed < 0x50 ? theSeed + 1 : theSeed;
}

int TerritorialSeed(int theStored) {
	if (theStored == 0)
		return -1;
	return theStored < 0x50 ? theStored - 1 : theStored;
}

bool TerritorialRefuses(int theClaimedSeed, int theSeed) {
	if (theClaimedSeed < 0 || theClaimedSeed == theSeed)
		return false;
	switch (theSeed) {
	case 16: case 11: case 30: case 0x5b: case 0x6e: return false; // Lily Pad (0x42562b), 0xc113f0's list
	default: return true;
	}
}

namespace {
constexpr int kCarnivalTiers[13] = {2, -1, 3, 8, 1, 9, -1, 4, 5, 6, 0, 10, 7};       // 0x8f3464
constexpr int kCarnivalGoals[11] = {5, 18, 60, 210, 650, 1300, 2600, 4200, 6500, 8000, 0}; // 0x8d3464
} // namespace

int CarnivalTier(int theType) { return theType >= 0 && theType < 13 ? kCarnivalTiers[theType] : -1; }

int CarnivalGoal(int theTier) { return theTier >= 0 && theTier < 11 ? kCarnivalGoals[theTier] : 0; }

int CarnivalTypeOfTier(int theTier) {
	for (int t = 0; t < 13; t++)
		if (kCarnivalTiers[t] == theTier)
			return t;
	return -1;
}

int CarnivalEatScore(int theTier) { return (theTier + 1) * (theTier + 1); }

int CarnivalWeight(int theType, int thePlayerTier) {
	const int t = CarnivalTier(theType);
	if (t == -1)
		return 1;
	const int d = t - (thePlayerTier + 1);
	if (d >= 3)
		return 1;
	return 800 - 80 * std::abs(d);
}

bool CarnivalSpawnEdge(int theRoll900) {
	const int x = theRoll900 - 50;
	return x < -16 || x >= 810;
}

CarnivalSpawn CarnivalSpawnPlace(int theRoll900, int theRollY) {
	CarnivalSpawn s;
	s.x = theRoll900 - 50;
	int r = theRollY;
	if (!CarnivalSpawnEdge(theRoll900) && r >= 30)
		r += 620;
	s.y = r - 40;
	return s;
}

CarnivalRect CarnivalBulletRect(int theType, int x, int y, int w, int h) {
	CarnivalRect r{x, y, w, h};
	if (theType == kCarnivalWinType) {
		const int q = r.w >> 2;
		r.x += q;
		r.w -= 2 * q;
	} else if (theType != 7 && CarnivalTier(theType) < 6) {
		r.w >>= 1;
		r.h >>= 1;
	}
	return r;
}

bool CarnivalOverlap(const CarnivalRect& a, const CarnivalRect& b) {
	return std::max(a.x, b.x) < std::min(a.x + a.w, b.x + b.w) && std::max(a.y, b.y) < std::min(a.y + a.h, b.y + b.h);
}

int AllTheSameSeed(int thePacketType, int theImitaterType) {
	return thePacketType == 48 ? theImitaterType : thePacketType;
}

int EliteChallengeSlot(int theSlotType, bool theZombieSea) {
	return theSlotType == -1 && !theZombieSea ? -1 : kEliteChallengeZombie;
}

bool CounterRefuses(int thePlantCount) { return thePlantCount >= 6; }

bool SchrodingerKeepsCorpse(int theZombieType) {
	return theZombieType >= 5 && theZombieType != 12 && theZombieType != 20 && theZombieType != 22 && theZombieType != 25;
}

VaseRoll RandomVaseRoll(int r) {
	VaseRoll v;
	if (r == 0) {
		v.kind = VaseKind::Sun;
	} else if (r <= 32 && r - 1 != 25) {
		v.kind = VaseKind::Zombie;
		v.zombie = r - 1;
	}
	return v;
}

bool RandomVaseSeedOk(int s, bool theModRowExists) {
	if (s < 0 || s >= 0x80 || s == 48 || s == 51)
		return false;
	if (s < 53)
		return true;
	return s >= 0x50 && theModRowExists;
}

GloveWar3Spot GloveWar3Start(int k) {
	static constexpr int kBases[10] = {0x09, 0x17, 0x19, 0x1b, 0x1d, 0x20, 0x22, 0x23, 0x25, 0x2a};
	GloveWar3Spot s;
	s.seed = kBases[std::clamp(k, 0, 9)];
	s.packet = s.seed + 0x50;
	s.col = k < 5 ? 0 : 1;
	s.row = k % 5;
	return s;
}

int GloveWar3Toggle(int theSeed) { return theSeed < 0x30 ? theSeed + 0x50 : theSeed - 0x50; }

int GloveWar3NoPlantRefresh(int theRefreshTime) { return theRefreshTime + 3000; }

int GloveWar3CellIndex(int theCol, int theRow) { return theCol * 5 + theRow; }

Cell GloveWar3PlantCell(const std::array<bool, 45>& theFree) {
	for (int aCol = 0; aCol < 9; aCol++)
		for (int aRow = 0; aRow < 5; aRow++)
			if (theFree[GloveWar3CellIndex(aCol, aRow)])
				return Cell{aCol, aRow};
	return Cell{-1, -1};
}

int GloveWar3SunAfter(int theSun, int theCost) { return std::max(0, theSun - theCost); }

void ZengStep(int theKey, int& theCol, int& theRow) {
	switch (theKey) {
	case 'W': theRow--; break;
	case 'S': theRow++; break;
	case 'A': theCol--; break;
	case 'D': theCol++; break;
	default: break;
	}
}

int ZengTouchKey(int thePlantCol, int thePlantRow, const Touch& theTouch, int theTapCol, int theTapRow) {
	if (theTouch.swipe) {
		switch (theTouch.dir) {
		case Steer::Up: return 'W';
		case Steer::Down: return 'S';
		case Steer::Left: return 'A';
		case Steer::Right: return 'D';
		default: return 0;
		}
	}
	if (theTapCol < 0 || theTapCol >= 9 || theTapRow < 0 || theTapRow >= 5)
		return 0;
	const int dc = theTapCol - thePlantCol, dr = theTapRow - thePlantRow;
	if (dc == 0 && dr == 0)
		return 0;
	if (std::abs(dr) >= std::abs(dc))
		return dr < 0 ? 'W' : 'S';
	return dc < 0 ? 'A' : 'D';
}

bool ZengElementWave(int theCurrentWave) { return theCurrentWave == 9; }

bool ZengPuffs(int theCurrentWave) { return theCurrentWave >= 20; }

bool VisualDisorderRecolours(int theCurrentWave, int theMainCounter) { return theCurrentWave >= 12 && (theMainCounter & 0x7f) == 0; }

int ItemModeOf(int theMode) {
	switch (theMode) {
	case 0x6b: return 1;
	case 0x87: return 2;
	case 0x8d: return 4;
	case 0x90: return 3;
	default: return 0;
	}
}

int GloveRenderOrder(int theRow, int theX, int theSeed) {
	int aOrder = theRow * 10000 + 302805 - theX;
	if (theSeed == 16 || theSeed == 33)
		aOrder -= 5;
	else if (theSeed == 30)
		aOrder += 5;
	return aOrder;
}

bool ItemModeHidesSeed(int theItemMode, int theSeed) { return theItemMode != 0 && theItemMode != 3 && theSeed >= 40 && theSeed <= 47; }

bool GloveRefusesColumn(int theItemMode, int theGridX) { return theItemMode == 3 && theGridX > 3; }

bool ImitaterTableAllows(int theSeed) {
	// 128 bytes at 0xc003ce, read from the exe file (1 = allowed)
	static constexpr const char* kTable = "11111111111111111111111111111111" "11111111111111110110100000000000"
	                                      "00000000000000001111001111011100" "00000001010101001011010010110000";
	return theSeed >= 0 && theSeed < 128 && kTable[theSeed] == '1';
}

// ---- V2 ----

int ModeWavePoints(int theMode, int thePointsBefore, int thePointsAfter, int theWave) {
	if (theMode == kLittleZombie)
		return thePointsBefore * 7 * 4; // 0x7a783f imul ecx,ecx,7; 0x409960 add ecx,ecx x2
	if (theMode == kPincerAttack)
		return thePointsBefore * ((theWave >> 3) + 1) * 4; // 0x7a7850..0x7a785a
	return thePointsAfter;
}

bool ModeLittleZombies(int theMode) { return theMode == kLittleZombie; } // hook 516: cmp ecx,0x6e

void GloveWarStartCell(int theSlot, int& theCol, int& theRow) {
	theRow = theSlot < 5 ? 0 : 1;
	theCol = theSlot < 5 ? theSlot : theSlot - 5;
}

int GloveWarStartSeed(int thePacketType, int theImitaterType) {
	const int aSeed = thePacketType == 48 ? theImitaterType : thePacketType;
	return aSeed == -1 ? 33 : aSeed; // -1 -> 0x21 Flower Pot
}

int LimitedSupplyPacketToRestart(const std::vector<LimitedPacket>& thePackets, int thePlantSeed, int thePlantImitater) {
	for (size_t i = 0; i < thePackets.size(); i++) {
		const LimitedPacket& p = thePackets[i];
		if (p.refreshing)
			continue;
		bool aMatch;
		if (p.type == 48) { // 0x30: the packet's copy against the plant's own seed (or its +0x138 while still 0x30)
			const int aTarget = thePlantSeed == 48 ? thePlantImitater : thePlantSeed;
			aMatch = p.imitaterType != -1 && thePlantImitater != -1 && p.imitaterType == aTarget;
		} else {
			aMatch = thePlantImitater == -1 && p.type == thePlantSeed;
		}
		if (aMatch)
			return static_cast<int>(i);
	}
	return -1;
}

bool InvisibleGuardianCardShown(int theSeed) { return theSeed == 1 || theSeed == 9 || theSeed == 0x29; }

} // namespace pvzu::unnamed
