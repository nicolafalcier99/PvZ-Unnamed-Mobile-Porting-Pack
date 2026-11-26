// PVZU: per-mode rules of the Unnamed mod-only modes (see ModModes.h; numbers and evidence in
// pvz-unnamed-pack/SOURCES-mod-modes.md, pure parts in Mod/Unnamed/ModModeRules.h). Each site checks CurrentModMode() first,
// so outside a mod mode nothing here runs; each rule then checks its own mode number.
#include "Mod/Engine/ModModes.h"

#include "Mod/CmdLine.h"
#include "Mod/Engine/DevLevels.h"
#include "Mod/Unnamed/ModModeRules.h"
#include "Mod/Unnamed/ModModes.h"

#include "LawnApp.h"
#include "Lawn/Board.h"
#include "Lawn/Challenge.h"
#include "Lawn/Coin.h"
#include "Lawn/GridItem.h"
#include "Lawn/MessageWidget.h"
#include "Lawn/Plant.h"
#include "Lawn/Projectile.h"
#include "Lawn/SeedPacket.h"
#include "Lawn/Zombie.h"
#include "PvzpLib/PvzpStringFile.h"
#include "graphics/Graphics.h"
#include "PvzpLib/PvzpFoley.h"

#include <algorithm>
#include <format>
#include <map>
#include <string>
#include <vector>

namespace pvzu::engine {

namespace u = pvzu::unnamed;

namespace {

int Mode(LawnApp* theApp) {
	const u::ModModeDef* aDef = CurrentModMode(theApp);
	return aDef ? aDef->mode : 0;
}

int Rule(LawnApp* theApp, const char* theKey, int theDefault) {
	const u::ModModeDef* aDef = CurrentModMode(theApp);
	return aDef ? aDef->Rule(theKey, theDefault) : theDefault;
}

std::string Text(const char* theKey, const char* theFallback) {
	const std::string aKey = std::format("[{}]", theKey);
	return PvzpStringListExists(aKey) ? std::string(PvzpStringTranslate(aKey)) : std::string(theFallback);
}

// Dev log, at most 4 lines per tag per run (rules that fire every hit or bite).
void LogFew(const char* theTag, const std::string& theLine) {
	static std::map<std::string, int> sCount;
	if (++sCount[theTag] <= 4)
		DevLog(std::format("modModes {} {}", theTag, theLine));
}

// Dev runs (-pvzu-game-mode, hidden): -pvzu-dev-mode-guard keeps the house safe (zombies past x 60 vanish, sun >= 1000)
// so later waves are reached; -pvzu-dev-mode-lawn plants Peashooters in columns 0-1 at the start.
void DevModeTick(Board* theBoard) {
	static const bool aGuard = CmdFlag("-pvzu-dev-mode-guard");
	static const bool aLawn = CmdFlag("-pvzu-dev-mode-lawn");
	static bool aLaid = false;
	if (aLawn && !aLaid) {
		aLaid = true;
		int aCount = 0;
		for (int aRow = 0; aRow < MAX_GRID_SIZE_Y; aRow++)
			for (int aCol = 0; aCol < 2; aCol++)
				if (theBoard->CanPlantAt(aCol, aRow, SeedType::SEED_PEASHOOTER) == PlantingReason::PLANTING_OK) {
					theBoard->AddPlant(aCol, aRow, SeedType::SEED_PEASHOOTER, SeedType::SEED_NONE);
					aCount++;
				}
		DevLog(std::format("modModes devLawn peashooters={}", aCount));
	}
	// -pvzu-dev-mode-eat: a Normal zombie in row 0 at x 150 (next to the column-1 plant) at play tick 100
	static const bool aEat = CmdFlag("-pvzu-dev-mode-eat");
	static int aTicks = 0;
	aTicks++;
	if (aEat && aTicks == 100)
		if (Zombie* aZombie = theBoard->AddZombieInRow(ZombieType::ZOMBIE_NORMAL, 0, 0)) {
			aZombie->mPosX = 150.0f;
			DevLog("modModes devEat zombie row=0 x=150");
		}
	// -pvzu-dev-mode-sun: a sun and a large sun land mid-lawn at play tick 200 (lights for 0x7c)
	if (CmdFlag("-pvzu-dev-mode-sun") && aTicks == 200) {
		theBoard->AddCoin(330, 250, CoinType::COIN_SUN, CoinMotion::COIN_MOTION_COIN);
		theBoard->AddCoin(500, 350, CoinType::COIN_LARGESUN, CoinMotion::COIN_MOTION_COIN);
		DevLog("modModes devSun 2 coins");
	}
	// -pvzu-dev-mode-bank: every 300 ticks plant packet 0 (when ready) on the first free cell from column 2, as a tap would
	// (pay, deactivate, AddPlant, WasPlanted)
	static const bool aBank = CmdFlag("-pvzu-dev-mode-bank");
	if (aBank && aTicks % 300 == 0 && theBoard->mSeedBank && theBoard->mSeedBank->mNumPackets > 0) {
		SeedPacket& aPacket = theBoard->mSeedBank->mSeedPackets[0];
		const SeedType aSeed = aPacket.mPacketType;
		if (aPacket.mActive && aSeed != SeedType::SEED_NONE && theBoard->mSunMoney >= Plant::GetCost(aSeed, aPacket.mImitaterType)) {
			for (int i = 0; i < 45; i++) {
				const int aCol = 2 + i / 5, aRow = i % 5;
				if (aCol >= MAX_GRID_SIZE_X || theBoard->CanPlantAt(aCol, aRow, aSeed) != PlantingReason::PLANTING_OK)
					continue;
				theBoard->mSunMoney -= Plant::GetCost(aSeed, aPacket.mImitaterType);
				aPacket.Deactivate();
				theBoard->AddPlant(aCol, aRow, aSeed, aPacket.mImitaterType);
				aPacket.WasPlanted();
				DevLog(std::format("modModes devBank planted seed={} at {},{} packetNow={} refreshing={}", static_cast<int>(aSeed), aCol,
					aRow, static_cast<int>(aPacket.mPacketType), aPacket.mRefreshing ? 1 : 0));
				break;
			}
		}
	}
	// -pvzu-dev-mode-probe: at play tick 400 the plant on (0, 0) dies, then CanPlantAt(0, 0) is asked for a Sunflower and a
	// Peashooter (0x88 claims)
	// -pvzu-dev-mode-kill <tick>: at that play tick the plant on (2, 0) dies (V2: 0x80 restarts its packet)
	if (static const int aKillAt = CmdOptionInt("-pvzu-dev-mode-kill", -1); aKillAt > 0 && aTicks == aKillAt) {
		if (Plant* aPlant = theBoard->GetTopPlantAt(2, 0, PlantPriority::TOPPLANT_ANY)) {
			DevLog(std::format("modModes devKill seed={} at 2,0", static_cast<int>(aPlant->mSeedType)));
			aPlant->Die();
			if (theBoard->mSeedBank && theBoard->mSeedBank->mNumPackets > 0) {
				const SeedPacket& p = theBoard->mSeedBank->mSeedPackets[0];
				DevLog(std::format("modModes devKill packet 0 refreshing={} counter={} time={}", p.mRefreshing ? 1 : 0,
					p.mRefreshCounter, p.mRefreshTime));
			}
		}
	}
	if (CmdFlag("-pvzu-dev-mode-probe") && aTicks == 400) {
		if (Plant* aPlant = theBoard->GetTopPlantAt(0, 0, PlantPriority::TOPPLANT_ANY))
			aPlant->Die();
		DevLog(std::format("modModes probe (0,0) sunflower={} peashooter={}",
			static_cast<int>(theBoard->CanPlantAt(0, 0, SeedType::SEED_SUNFLOWER)),
			static_cast<int>(theBoard->CanPlantAt(0, 0, SeedType::SEED_PEASHOOTER))));
		// and a Football zombie in row 2 killed by one hit, then hit again (0x76 revives it)
		if (Zombie* aZombie = theBoard->AddZombieInRow(ZombieType::ZOMBIE_FOOTBALL, 2, 0)) {
			aZombie->mPosX = 500.0f;
			aZombie->TakeDamage(20000, 0U);
			const int aPhaseDead = static_cast<int>(aZombie->mZombiePhase);
			const bool aHittable = aZombie->EffectedByDamage(0x1f);
			aZombie->TakeDamage(20, 0U);
			DevLog(std::format("modModes probe zombie phase after kill={} hittable={} after next hit={}", aPhaseDead, aHittable ? 1 : 0,
				static_cast<int>(aZombie->mZombiePhase)));
		}
	}
	// -pvzu-dev-mode-vases: at play tick 300 every vase opens (Vasebreaker modes), then the seed packets that fell are logged
	if (CmdFlag("-pvzu-dev-mode-vases") && aTicks == 300) {
		std::vector<GridItem*> aPots;
		for (GridItem* aItem : theBoard->mGridItems)
			if (!aItem->mDead && aItem->mGridItemType == GridItemType::GRIDITEM_SCARY_POT)
				aPots.push_back(aItem);
		for (GridItem* aPot : aPots)
			if (!aPot->mDead)
				theBoard->mChallenge->ScaryPotterOpenPot(aPot);
		std::string aSeeds;
		for (Coin* aCoin : theBoard->mCoins)
			if (!aCoin->mDead && aCoin->mType == CoinType::COIN_USABLE_SEED_PACKET)
				aSeeds += std::format(" 0x{:x}", static_cast<int>(aCoin->mUsableSeedType));
		DevLog(std::format("modModes devVases opened={} packets:{}", aPots.size(), aSeeds));
	}
	// -pvzu-dev-mode-collect: every falling sun is collected (as a tap) once it is below y 250 (0x66 charged suns)
	if (CmdFlag("-pvzu-dev-mode-collect"))
		for (Coin* aCoin : theBoard->mCoins)
			if (!aCoin->mDead && aCoin->IsSun() && !aCoin->mIsBeingCollected && aCoin->mPosY > 250.0f &&
				aCoin->mPosY + aCoin->mVelY < aCoin->mGroundY) {
				DevLog(std::format("modModes devCollect type={} at {:.0f},{:.0f} charged={}", static_cast<int>(aCoin->mType),
					aCoin->mPosX, aCoin->mPosY, static_cast<int>(aCoin->mUsableSeedType)));
				aCoin->Collect();
			}
	// -pvzu-dev-mode-where: every 500 ticks, where the zombies are (x, y, row)
	if (CmdFlag("-pvzu-dev-mode-where") && aTicks % 500 == 0) {
		std::string aList;
		for (Zombie* aZombie : theBoard->mZombies)
			if (!aZombie->mDead)
				aList += std::format(" {}:{},{},r{}", static_cast<int>(aZombie->mZombieType), aZombie->mX, aZombie->mY, aZombie->mRow);
		DevLog(std::format("modModes where t={}{}", aTicks, aList));
	}
	if (!aGuard)
		return;
	theBoard->mSunMoney = std::max(theBoard->mSunMoney, 1000);
	for (Zombie* aZombie : theBoard->mZombies)
		if (!aZombie->mDead && !aZombie->mMindControlled && aZombie->mPosX < 60.0f && aZombie->mZombieType != ZombieType::ZOMBIE_BOSS)
			aZombie->DieNoLoot();
	static int aLastWave = -1;
	if (theBoard->mCurrentWave != aLastWave) {
		aLastWave = theBoard->mCurrentWave;
		DevLog(std::format("modModes wave {}/{}", theBoard->mCurrentWave, theBoard->mNumWaves));
	}
}

void Message(Board* theBoard, const std::string& theText, MessageStyle theStyle, int theDuration) {
	theBoard->mAdvice->SetLabel(theText, theStyle);
	theBoard->mAdvice->mDuration = theDuration;
}

// The fixed bank of a no-chooser mode (hook 32): packets 0..n-1 as listed, n = min(list, bank size).
void FillFixedBank(Board* theBoard, const u::ModModeDef& theDef) {
	if (theDef.bank.empty() || theBoard->mSeedBank == nullptr)
		return;
	const int aCount = std::min(static_cast<int>(theDef.bank.size()), theBoard->mSeedBank->mNumPackets);
	for (int i = 0; i < aCount; i++) {
		const int aSeed = SeedTypeByName(theDef.bank[i]);
		if (aSeed >= 0)
			theBoard->mSeedBank->mSeedPackets[i].SetPacketType(static_cast<SeedType>(aSeed));
	}
}

// Board::ClearFogAroundPlant for a cell (0x7c passes a stack "plant" with only mPlantCol / mRow set).
void ClearFogAroundCell(Board* theBoard, int theCol, int theRow, int theSize) {
	int aFogFadeOutSpeed = 6;
	if (theBoard->mFogBlownCountDown > 0 && theBoard->mFogBlownCountDown < 2000)
		aFogFadeOutSpeed = 2;
	else if (theBoard->mFogBlownCountDown > 0)
		aFogFadeOutSpeed = 40;
	const int aLeft = theBoard->LeftFogColumn();
	const int aFogOffsetX = (theBoard->mFogOffset + 50) / 100;
	const int aStartX = std::max(theCol - theSize - aFogOffsetX, aLeft);
	const int aEndX = std::min(theCol + theSize - aFogOffsetX, MAX_GRID_SIZE_X - 1);
	const int aStartY = std::max(theRow - theSize, 0);
	const int aEndY = std::min(theRow + theSize, MAX_GRID_SIZE_Y);
	for (int x = aStartX; x <= aEndX; x++)
		for (int y = aStartY; y <= aEndY; y++) {
			if (std::abs(x + aFogOffsetX - theCol) + std::abs(y - theRow) > theSize)
				continue;
			theBoard->mGridCelFog[x][y] = std::max(theBoard->mGridCelFog[x][y] - aFogFadeOutSpeed, 0);
		}
}

} // namespace

// ---- Board ----

void ModModeInitLevel(Board* theBoard) {
	const u::ModModeDef* aDef = CurrentModMode(theBoard->mApp);
	if (aDef == nullptr)
		return;
	if (aDef->startSun >= 0)
		theBoard->mSunMoney = aDef->startSun;
	FillFixedBank(theBoard, *aDef);
	// exe Challenge+0x58 / +0x70 hold the gale and frequency states (hooks 98, 104); zero at a new level.
	theBoard->mChallenge->mChallengeStateCounter = 0;
	theBoard->mChallenge->mSlotMachineRollCount = 0;
	ModModeInit2(theBoard, aDef->mode);
	DevLog(std::format("modModes init mode=0x{:x} bg={} waves={} slots={} sun={}", aDef->mode,
		static_cast<int>(theBoard->mBackground), theBoard->mNumWaves, theBoard->mSeedBank ? theBoard->mSeedBank->mNumPackets : -1,
		theBoard->mSunMoney));
}

void ModModeStartLevel(Board* theBoard) {
	const u::ModModeDef* aDef = CurrentModMode(theBoard->mApp);
	if (aDef == nullptr)
		return;
	// Hook 160 (Challenge::StartLevel, after the Column block; 0x68/0x74 5500) and hook 29 (0x92 7500): the first wave.
	if (aDef->firstWaveCountdown > 0)
		theBoard->mZombieCountDown = aDef->firstWaveCountdown;
	ModModeStart2(theBoard, aDef->mode);
	std::string aAllowed;
	for (int i = 0; i < NUM_ZOMBIE_TYPES; i++)
		if (theBoard->mZombieAllowed[i])
			aAllowed += std::format("{}{}", aAllowed.empty() ? "" : ",", i);
	std::string aBank;
	if (theBoard->mSeedBank)
		for (int i = 0; i < theBoard->mSeedBank->mNumPackets; i++)
			aBank += std::format("{}{}", aBank.empty() ? "" : ",", static_cast<int>(theBoard->mSeedBank->mSeedPackets[i].mPacketType));
	DevLog(std::format("modModes start-level mode=0x{:x} allowed={} bank={} graves={} conveyor={} countdown={} waves={}", aDef->mode,
		aAllowed, aBank, theBoard->StageHasGraveStones() ? 1 : 0, theBoard->HasConveyorBeltSeedBank() ? 1 : 0,
		theBoard->mZombieCountDown, theBoard->mNumWaves));
}

void ModModeUpdate(Board* theBoard) {
	const int aMode = Mode(theBoard->mApp);
	if (aMode == 0 || theBoard->mPaused)
		return;
	DevModeTick(theBoard);
	Challenge* c = theBoard->mChallenge.get();
	if (aMode == u::kForce8Gale && theBoard->mCurrentWave >= Rule(theBoard->mApp, "fromWave", 8)) {
		// Hook 98 (0x7a69c1, at the zombie countdown): gusts; timer = Challenge+0x58, direction = Challenge+0x70.
		u::GaleState s{c->mChallengeStateCounter, c->mSlotMachineRollCount};
		const int aRoll = s.timer == 0 ? Sexy::Rand(1500) : 0;
		const int aMsg = u::GaleTick(s, aRoll);
		c->mChallengeStateCounter = s.timer;
		c->mSlotMachineRollCount = s.dir;
		if (aMsg != 0) {
			static const char* kKeys[] = {"", "PVZU_GALE_RISES_RIGHT", "PVZU_GALE_CHANGED_RIGHT", "PVZU_GALE_CHANGED_LEFT"};
			// 0xa20140 / 0xa20180 / 0xa201a0: "[quote not included]", "[quote not included]", "[quote not included]"
			static const char* kText[] = {"", "The wind rises! It blows to the right.", "The wind turns! It blows to the right.",
				"The wind turns! It blows to the left."};
			Message(theBoard, Text(kKeys[aMsg], kText[aMsg]), MessageStyle::MESSAGE_STYLE_TUTORIAL_LATER, 250);
			DevLog(std::format("modModes gale wave={} dir={} next={}", theBoard->mCurrentWave, s.dir, s.timer));
		}
	} else if (aMode == u::kFrequencySwitch) {
		// Hook 104 (0x7a7c71): f = Challenge+0x70, timer = Challenge+0x58.
		u::FrequencyState s{c->mSlotMachineRollCount, c->mChallengeStateCounter};
		const int aBefore = s.f;
		u::FrequencyTick(s, theBoard->mCurrentWave, Sexy::Rand(201));
		c->mSlotMachineRollCount = s.f;
		c->mChallengeStateCounter = s.timer;
		if (s.f != aBefore)
			DevLog(std::format("modModes frequency f={} next={}", s.f, s.timer));
	}
	ModModeUpdate2(theBoard, aMode);
}

void ModModeDraw(Board* theBoard, Sexy::Graphics* g) {
	const int aMode = Mode(theBoard->mApp);
	if (aMode == u::kRainyDay) {
		// Hook 103 (0x798c61): Color::White becomes (100,100,100) while the level runs, so the whole picture is drawn at
		// 100/255. Our stand-in: a black veil of alpha 255-100 over the board and its HUD.
		g->SetColor(Sexy::Color(0, 0, 0, 255 - Rule(theBoard->mApp, "white", 100)));
		g->FillRect(-theBoard->mX, -theBoard->mY, theBoard->mApp->mWidth, theBoard->mApp->mHeight);
	}
	if (aMode != 0)
		ModModeDraw2(theBoard, g, aMode);
}

bool ModModeKeyDown(Board* theBoard, Sexy::KeyCode theKey) {
	return ModModeLoneWolfKey(theBoard, theKey) || ModModeZengKey(theBoard, theKey);
}

bool ModModeRains(LawnApp* theApp) { return Mode(theApp) == u::kRainyDay; }

bool ModModeObjectsInvisible(LawnApp* theApp) { return theApp && Mode(theApp) == u::kInvisibleGuardian; } // hook 2 (0x798d91)

int ModModeWaveBudget(Board* theBoard, int thePointsBefore, int thePointsAfter, int theWave) {
	const int aMode = Mode(theBoard->mApp);
	if (aMode == 0)
		return thePointsAfter;
	const int aPoints = u::ModeWavePoints(aMode, thePointsBefore, thePointsAfter, theWave);
	if (aPoints != thePointsAfter)
		LogFew("wavePoints", std::format("mode=0x{:x} wave={} before={} vanilla={} now={}", aMode, theWave, thePointsBefore,
			thePointsAfter, aPoints));
	return aPoints;
}

bool ModModeLittleZombies(LawnApp* theApp) { return theApp && u::ModeLittleZombies(Mode(theApp)); } // hook 516

namespace {
int gImitaterMorphing = 0; // 0x7a8966: Plant::Die called from ImitaterMorph (return address 0x466b89) restarts nothing
}
ModModeImitaterMorphScope::ModModeImitaterMorphScope() { ++gImitaterMorphing; }
ModModeImitaterMorphScope::~ModModeImitaterMorphScope() { --gImitaterMorphing; }

void ModModePlantDied(Plant* thePlant) {
	if (gImitaterMorphing > 0 || thePlant->mBoard == nullptr || Mode(thePlant->mApp) != u::kLimitedSupply)
		return;
	SeedBank* aBank = thePlant->mBoard->mSeedBank.get();
	if (aBank == nullptr)
		return;
	std::vector<u::LimitedPacket> aPackets;
	for (int i = 0; i < aBank->mNumPackets; i++) {
		const SeedPacket& p = aBank->mSeedPackets[i];
		aPackets.push_back({static_cast<int>(p.mPacketType), static_cast<int>(p.mImitaterType), p.mRefreshing});
	}
	const int aSeed = thePlant->mPvzu.modSeed != 0 ? thePlant->mPvzu.modSeed : static_cast<int>(thePlant->mSeedType);
	const int aIndex = u::LimitedSupplyPacketToRestart(aPackets, aSeed, static_cast<int>(thePlant->mImitaterType));
	if (aIndex < 0)
		return;
	aBank->mSeedPackets[aIndex].mRefreshing = true; // hook 361: +0x49 = 1; the counter is still 0 from WasPlanted
	LogFew("limitedSupply", std::format("plant seed={} died: packet {} recharges", aSeed, aIndex));
}

bool ModModeBlocksPlanting(Board* theBoard) {
	const int aMode = Mode(theBoard->mApp);
	if (aMode == u::kFrequencySwitch && theBoard->mChallenge->mSlotMachineRollCount == 2)
		return true; // hook 46 -> 0x7a7500: no planting while the zombies are hidden
	if (aMode == u::kTheCounter && u::CounterRefuses(static_cast<int>(theBoard->mPlants.mSize)))
		return true; // hook 46 chain 0x7a7532: at most 6 plants
	return false;
}

bool ModModeHidesRenderItem(Board* theBoard, int theRenderObjectType) {
	if (Mode(theBoard->mApp) != u::kFrequencySwitch)
		return false;
	const int f = theBoard->mChallenge->mSlotMachineRollCount; // hooks 118, 122, 124
	if (f == 1)
		return theRenderObjectType == RenderObjectType::RENDER_ITEM_PLANT;
	if (f == 2)
		return theRenderObjectType == RenderObjectType::RENDER_ITEM_ZOMBIE ||
			theRenderObjectType == RenderObjectType::RENDER_ITEM_ZOMBIE_SHADOW;
	return false;
}

bool ModModeNoWaveGate(Board* theBoard) {
	const int aMode = Mode(theBoard->mApp);
	return aMode == u::kGiantCrisis || aMode == u::kCounterEspionage; // hook 42 (0x7a93a1)
}

bool ModModeBungeeWaves(Board* theBoard) {
	if (Mode(theBoard->mApp) != u::kTroopsFromSky)
		return false;
	LogFew("troopsFromSky", std::format("wave {} dropped by bungees", theBoard->mCurrentWave)); // hook 89
	return true;
}

void ModModeWaveSpawned(Board* theBoard) {
	ModModeWave3(theBoard, Mode(theBoard->mApp));
	if (Mode(theBoard->mApp) != u::kWarOfAttrition || !u::AttritionFlips(theBoard->mCurrentWave))
		return;
	// 0x7995a6: day <-> night, background images only (not PickBackground), then plants re-sleep for the new time.
	theBoard->mBackground = theBoard->mBackground == BackgroundType::BACKGROUND_1_DAY ? BackgroundType::BACKGROUND_2_NIGHT
	                                                                                     : BackgroundType::BACKGROUND_1_DAY;
	theBoard->LoadBackgroundImages();
	const bool aNight = theBoard->mBackground == BackgroundType::BACKGROUND_2_NIGHT;
	int aAsleep = 0;
	for (Plant* aPlant : theBoard->mPlants) {
		if (aPlant->mDead || aPlant->mSeedType == SeedType::SEED_LILYPAD || aPlant->mSeedType == SeedType::SEED_FLOWERPOT ||
			aPlant->mSeedType == SeedType::SEED_INSTANT_COFFEE)
			continue;
		const bool aSleep = u::AttritionNocturnal(aPlant->mSeedType) != aNight;
		aPlant->SetSleeping(aSleep);
		aAsleep += aSleep ? 1 : 0;
	}
	DevLog(std::format("modModes attrition wave={} night={} asleep={}", theBoard->mCurrentWave, aNight ? 1 : 0, aAsleep));
}

bool ModModeFogLight(Board* theBoard) {
	if (Mode(theBoard->mApp) != u::kLightAndMist)
		return false;
	// Hook 135 (0x7a8351): no plant clears fog; every visible sun coin does, radius by size.
	for (Coin* aCoin : theBoard->mCoins) {
		if (aCoin->mDead || !aCoin->mVisible)
			continue;
		const int aRadius = u::LightSunRadius(static_cast<int>(aCoin->mType));
		if (aRadius == 0)
			continue;
		const float x = aCoin->mPosX + aCoin->mWidth * 0.5f * aCoin->mScale;
		const float y = aCoin->mPosY + aCoin->mHeight * 0.5f * aCoin->mScale;
		if (!(x > 0 && x < 750 && y > 45 && y < 600))
			continue;
		ClearFogAroundCell(theBoard, theBoard->PixelToGridX(static_cast<int>(x), static_cast<int>(y)), u::LightSunRow(static_cast<int>(y)),
			aRadius);
		LogFew("lightAndMist", std::format("sun type={} at {:.0f},{:.0f} clears radius {}", static_cast<int>(aCoin->mType), x, y, aRadius));
	}
	return true;
}

bool ModModeFogColumn(Board* theBoard, int& theColumn) {
	if (Mode(theBoard->mApp) != u::kLightAndMist)
		return false;
	theColumn = 0; // hook 145 (0x7a8310)
	return true;
}

void ModModeBlover(Board* theBoard) {
	if (Mode(theBoard->mApp) == u::kLightAndMist)
		theBoard->mFogBlownCountDown = 1; // hook 342 (0x7a8450)
}

// ---- seed packets ----

void ModModePacketPlanted(SeedPacket* thePacket) {
	if (Mode(thePacket->mApp) != u::kSlotCrisis)
		return;
	// Hook 69 (0x799681): before WasPlanted, so the recharge is the new seed's.
	const int aOld = thePacket->mPacketType;
	thePacket->mPacketType = static_cast<SeedType>(u::SlotCrisisSeed(Sexy::Rand(6)));
	DevLog(std::format("modModes slotCrisis {} -> {}", aOld, static_cast<int>(thePacket->mPacketType)));
}

void ModModePacketRecharge(SeedPacket* thePacket) {
	if (Mode(thePacket->mApp) == u::kLimitedSupply) {
		thePacket->mRefreshing = false; // hook 70 (0x7a8901): the packet stays grey until its plant is gone (hook 361)
		LogFew("limitedSupply", std::format("packet {} seed={} waits for its plant", thePacket->mIndex, static_cast<int>(thePacket->mPacketType)));
	}
}

// ---- plants ----

bool ModModePlantBlinkTick(Plant* thePlant) {
	if (thePlant->mBoard == nullptr || Mode(thePlant->mApp) != u::kForce8Gale)
		return false;
	// Hook 318 (0x7a6ab1): drift with the wind.
	const int aDir = thePlant->mBoard->mChallenge->mSlotMachineRollCount;
	if (aDir == 0)
		return false;
	const int aRange = std::max(1, 36 - thePlant->mBoard->mCurrentWave);
	const int aRoll = Sexy::Rand(aRange);
	if (aRoll >= 2)
		return false;
	thePlant->mX += u::GaleDrift(aDir, aRoll);
	if (aRoll == 1)
		LogFew("galeDrift", std::format("seed={} row={} x={} dir={}", static_cast<int>(thePlant->mSeedType), thePlant->mRow, thePlant->mX, aDir));
	if (static_cast<unsigned>(thePlant->mX) > 800u) {
		thePlant->Die();
		return true;
	}
	thePlant->mPlantCol = thePlant->mBoard->PixelToGridX(thePlant->mX + 40, thePlant->mY);
	return false;
}

bool ModModePlantBlinked(Plant* thePlant) {
	if (thePlant->mBoard == nullptr || Mode(thePlant->mApp) != u::kArtIsAnExplosion)
		return false;
	// Hook 319 (0x7a45c0) instead of DoBlink: explode at (mX + mWidth/4, mY + mHeight/2), radius 100, own row, 300 damage
	// to each zombie (hook 155), shake, die, then the sun rolls.
	Board* aBoard = thePlant->mBoard;
	const int x = thePlant->mX + thePlant->mWidth / 4;
	const int y = thePlant->mY + thePlant->mHeight / 2;
	const int aDamage = Rule(thePlant->mApp, "damage", 300);
	int aHit = 0;
	for (Zombie* aZombie : aBoard->mZombies) {
		if (aZombie->mDead || !aZombie->EffectedByDamage(0x4d))
			continue;
		const int aRowDist = aZombie->mZombieType == ZombieType::ZOMBIE_BOSS ? 0 : aZombie->mRow - thePlant->mRow;
		if (aRowDist == 0 && GetCircleRectOverlap(x, y, Rule(thePlant->mApp, "radius", 100), aZombie->GetZombieRect())) {
			aZombie->TakeDamage(aDamage, 18U);
			aHit++;
		}
	}
	thePlant->mApp->AddPvzpParticle(static_cast<float>(x), static_cast<float>(y), thePlant->mRow * 10000 + 307000,
		ParticleEffect::PARTICLE_POTATO_MINE);
	aBoard->mShakeCounter = 12;
	aBoard->mShakeAmountX = 3;
	aBoard->mShakeAmountY = -4;
	const int aPlantX = thePlant->mX, aPlantY = thePlant->mY;
	const int aSeed = thePlant->mSeedType;
	thePlant->Die();
	const int aRoll1 = Sexy::Rand(4);
	const int aRoll2 = aRoll1 >= 2 ? Sexy::Rand(4) : 0;
	const std::array<int, 2> aCoins = u::ArtExplosionCoins(aRoll1, aRoll2);
	if (aCoins[0] >= 0)
		aBoard->AddCoin(aPlantX, aPlantY, CoinType::COIN_LARGESUN, static_cast<CoinMotion>(aCoins[0]));
	if (aCoins[1] >= 0)
		aBoard->AddCoin(aPlantX, aPlantY, CoinType::COIN_SUN, static_cast<CoinMotion>(aCoins[1]));
	DevLog(std::format("modModes art seed={} col={} row={} hit={} largeSun={} sun={}", aSeed, thePlant->mPlantCol, thePlant->mRow,
		aHit, aCoins[0] >= 0 ? 1 : 0, aCoins[1] >= 0 ? 1 : 0));
	return true;
}

// ---- projectiles ----

void ModModeProjectileTick(Projectile* theProjectile) {
	if (Mode(theProjectile->mApp) != u::kSowBeans)
		return;
	// Hook 402 (0x7a81e0): only the type changes; motion stays.
	const int aOld = theProjectile->mProjectileType;
	theProjectile->mProjectileType = static_cast<ProjectileType>(u::SowBeansProjectile(aOld));
	if (aOld != theProjectile->mProjectileType)
		LogFew("sowBeans", std::format("projectile {} -> {}", aOld, static_cast<int>(theProjectile->mProjectileType)));
}

// ---- zombies ----

void ModModeZombieInit(Zombie* theZombie) {
	const int aMode = Mode(theZombie->mApp);
	if (aMode == u::kBigZombie) {
		// Hook 517 -> 0x7961f9 (after the Little Trouble block): x4 health, scale 2, not chilled. Every zombie.
		theZombie->mBodyHealth *= 4;
		theZombie->mHelmHealth *= 4;
		theZombie->mShieldHealth *= 4;
		theZombie->mScaleZombie = 2.0f;
		theZombie->mChilledCounter = 0;
	} else if (aMode == u::kIAmLuck && theZombie->mZombieType == ZombieType::ZOMBIE_BOSS) {
		theZombie->mBodyHealth = u::kIAmLuckBossHp; // hooks 505 / 506 (0xc00a20)
	} else if (aMode == u::kNightTerror) {
		// hook 517 (0x7a3081): body, helm and shield x(mCurrentWave + 4) / 4 (the maxima follow in ZombieInitialize)
		const int aWave = theZombie->mBoard ? theZombie->mBoard->mCurrentWave : 0;
		theZombie->mBodyHealth = u::NightTerrorHp(theZombie->mBodyHealth, aWave);
		theZombie->mHelmHealth = u::NightTerrorHp(theZombie->mHelmHealth, aWave);
		theZombie->mShieldHealth = u::NightTerrorHp(theZombie->mShieldHealth, aWave);
	} else if (aMode == u::kLoneWolf && theZombie->mZombieType < ZombieType::ZOMBIE_PEA_HEAD) {
		theZombie->mBodyHealth >>= 1; // hook 517 -> 0x799787: halved health for the vanilla types
		theZombie->mHelmHealth >>= 1;
		theZombie->mShieldHealth >>= 1;
	} else if (aMode == u::kLeaningLeft && theZombie->mBoard) {
		theZombie->mZombieAttackRect.mX -= Rule(theZombie->mApp, "attackShift", 250); // hook 518 (0x7a92e1)
	} else {
		return;
	}
	if (theZombie->mBoard)
		LogFew("zombieInit", std::format("mode=0x{:x} type={} body={} helm={} shield={} scale={:.1f} attackX={}", aMode,
			static_cast<int>(theZombie->mZombieType), theZombie->mBodyHealth, theZombie->mHelmHealth, theZombie->mShieldHealth,
			theZombie->mScaleZombie, theZombie->mZombieAttackRect.mX));
}

void ModModeZombieAdded(Zombie* theZombie) {
	if (Mode(theZombie->mApp) != u::kPincerAttack || theZombie->mBoard == nullptr)
		return;
	// Hook 44 (0x7a8111): from wave index 6, 1 in 2 wave zombies off the right edge start at x = 20 and walk right.
	const ZombieType t = theZombie->mZombieType;
	if (theZombie->mX < 760 || theZombie->mFromWave < -1 || t == ZombieType::ZOMBIE_DANCER || t == ZombieType::ZOMBIE_BOBSLED ||
		t == ZombieType::ZOMBIE_DIGGER || t == ZombieType::ZOMBIE_YETI || t == ZombieType::ZOMBIE_BUNGEE ||
		theZombie->mBoard->mCurrentWave < Rule(theZombie->mApp, "fromWave", 6) || Sexy::Rand(2) == 0)
		return;
	theZombie->mPosX = 20.0f;
	theZombie->mHasObject = true;
	LogFew("pincer", std::format("wave={} type={} row={} starts left", theZombie->mBoard->mCurrentWave, static_cast<int>(t),
		theZombie->mRow));
}

bool ModModeWalksBackwards(Zombie* theZombie) {
	// Hook 642 (0x7a80b0): every zombie with mHasObject walks right in 0x7e (the Flag Zombie too, literally).
	return theZombie->mBoard && Mode(theZombie->mApp) == u::kPincerAttack && theZombie->mZombiePhase != ZombiePhase::PHASE_ZOMBIE_MOWERED &&
		theZombie->mHasObject;
}

void ModModeZombieHit(Zombie* theZombie, int theDamage) {
	ModModeZombieRevive(theZombie); // 0x76
	if (Mode(theZombie->mApp) != u::kUpstream || theZombie->mDead ||
		theZombie->mZombiePhase == ZombiePhase::PHASE_JACK_IN_THE_BOX_POPPING)
		return;
	const float aBefore = theZombie->mPosX;
	theZombie->mPosX -= static_cast<float>(u::UpstreamPush(theDamage)); // hook 706 (0x7a6900): toward the house
	LogFew("upstream", std::format("type={} damage={} x {:.0f} -> {:.0f}", static_cast<int>(theZombie->mZombieType), theDamage, aBefore,
		theZombie->mPosX));
}

float ModModeAnimRate(Zombie* theZombie, float theRate) {
	if (theZombie->mBoard == nullptr || Mode(theZombie->mApp) != u::kZombiesRun)
		return theRate;
	const float aFactor = u::ZombiesRunFactor(theZombie->mBoard->mCurrentWave); // hook 662 -> 0x7a6b80
	static int sLoggedWave = -1;
	if (aFactor > 1.0f && sLoggedWave != theZombie->mBoard->mCurrentWave) {
		sLoggedWave = theZombie->mBoard->mCurrentWave;
		LogFew("zombiesRun", std::format("wave={} factor={:.3f}", sLoggedWave, aFactor));
	}
	return theRate * aFactor;
}

bool ModModeZombieRowStep(Zombie* theZombie) {
	if (Mode(theZombie->mApp) != u::kHeisenberg || theZombie->mZombieHeight != ZombieHeight::HEIGHT_ZOMBIE_NORMAL ||
		theZombie->mZombieType == ZombieType::ZOMBIE_BOSS || theZombie->mYuckyFace)
		return false;
	// Hook 615 (0x798e27): at the lane's y the zombie is drawn in a random other row; its lane and hit box stay.
	if (theZombie->mZombiePhase == ZombiePhase::PHASE_POLEVAULTER_IN_VAULT)
		return true;
	if (theZombie->mPosY != theZombie->GetPosYBasedOnRow(theZombie->mRow))
		return true;
	int aRow = theZombie->mRow;
	while (aRow == theZombie->mRow)
		aRow = Sexy::Rand(5);
	const int aNewY = aRow * 100 + 50;
	if (theZombie->mBossStompCounter == -1 && theZombie->mBossHeadCounter == -1) {
		theZombie->mBossStompCounter = theZombie->mY; // exe +0x138 / +0x13c (boss counters, -1 on a non-boss zombie)
		theZombie->mBossHeadCounter = theZombie->mZombieRect.mY;
	}
	theZombie->mZombieRect.mY = theZombie->mBossHeadCounter - (aNewY - theZombie->mBossStompCounter);
	theZombie->mPosY = static_cast<float>(aNewY);
	LogFew("heisenberg", std::format("type={} row={} drawnRow={} rectY={}", static_cast<int>(theZombie->mZombieType), theZombie->mRow,
		aRow, theZombie->mZombieRect.mY));
	return true;
}

bool ModModeZombieBites(Zombie* theZombie, Plant* thePlant) {
	if (Mode(theZombie->mApp) != u::kDownstream || thePlant->mDead)
		return false;
	// Hook 679 (0x796560), after StartEating: push the plant 40..80 px toward the house; it dies when the new cell does not
	// take it. The vanilla bite still follows unless the plant died.
	Board* aBoard = theZombie->mBoard;
	const u::DownstreamPush p = u::DownstreamBite(thePlant->mX, Sexy::Rand(41));
	thePlant->mX = p.x;
	bool aDies = p.offLawn;
	if (!aDies && p.col != thePlant->mPlantCol) {
		const PlantingReason aReason = aBoard->CanPlantAt(p.col, thePlant->mRow, thePlant->mSeedType);
		aDies = aReason != PlantingReason::PLANTING_OK && p.col != 8;
		thePlant->mPlantCol = p.col;
	}
	theZombie->mApp->AddPvzpParticle(static_cast<float>(thePlant->mX + 40), static_cast<float>(thePlant->mY + 20), 400000,
		aDies ? ParticleEffect::PARTICLE_MOWER_CLOUD : ParticleEffect::PARTICLE_PRESENT_PICKUP);
	LogFew("downstream", std::format("seed={} row={} x={} col={} dies={}", static_cast<int>(thePlant->mSeedType), thePlant->mRow, p.x,
		p.col, aDies ? 1 : 0));
	if (aDies) {
		theZombie->mApp->PlayFoley(FoleyType::FOLEY_USE_SHOVEL);
		thePlant->Die();
		return true;
	}
	theZombie->mApp->PlayFoley(FoleyType::FOLEY_ART_CHALLENGE);
	return false;
}

void ModModeDiggerSurfaced(Zombie* theZombie) {
	if (Mode(theZombie->mApp) == u::kCounterEspionage && !theZombie->mMindControlled) {
		theZombie->StartMindControlled(); // hook 586 (0x7a85a0)
		LogFew("counterEspionage", std::format("digger row={} x={:.0f} hypnotised", theZombie->mRow, theZombie->mPosX));
	}
}

bool ModModeHypnoEatsPlants(Zombie* theZombie) { return Mode(theZombie->mApp) == u::kCounterEspionage; } // hook 674

} // namespace pvzu::engine
