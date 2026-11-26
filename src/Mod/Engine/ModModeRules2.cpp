// PVZU: per-mode rules of the Unnamed mod-only modes, second pass (2026-11-23): 0x66 Sun Storm, 0x6d Night Terror,
// 0x72 Lone Wolf, 0x78 Sunflower Commander, 0x7f / 0x90 Trick of Tricks, 0x88 Territorial, 0x8b Bullet Carnival,
// 0x91 All the Same One (and 0x73's six packets), 0x92 Elite Zombies' Challenge. Numbers and evidence:
// pvz-unnamed-pack/SOURCES-mod-modes.md; pure parts: Mod/Unnamed/ModModeRules.h. Each site checks the running mod mode first
// (Mode() is 0 outside one, or with the feature "modModes" off), so Vanilla and the vanilla modes never reach a rule.
#include "Mod/Engine/ModModes.h"

#include "Mod/CmdLine.h"
#include "Mod/Touch.h"
#include "Mod/Engine/DevLevels.h"
#include "Mod/Engine/WideScreen.h"
#include "Mod/Engine/Zoom.h"
#include "Mod/Unnamed/ModModeRules.h"
#include "Mod/Unnamed/ModPlantRules.h"
#include "Mod/Unnamed/ModModes.h"
#include "Mod/Unnamed/SpecialLevels.h"

#include "LawnApp.h"
#include "Lawn/Board.h"
#include "Lawn/Challenge.h"
#include "Lawn/Coin.h"
#include "Lawn/CursorObject.h"
#include "Lawn/GridItem.h"
#include "Lawn/MessageWidget.h"
#include "Lawn/Plant.h"
#include "Lawn/Projectile.h"
#include "Lawn/SeedPacket.h"
#include "Lawn/Widget/SeedChooserScreen.h"
#include "Lawn/Zombie.h"
#include "Lawn/ZenGarden.h"
#include "PvzpLib/Attachment.h"
#include "PvzpLib/PvzpCommon.h"
#include "PvzpLib/PvzpFoley.h"
#include "PvzpLib/PvzpStringFile.h"
#include "PvzpLib/Reanimator.h"
#include "graphics/Graphics.h"
#include "Resources.h"
#include "widget/WidgetManager.h"

#include <algorithm>
#include <format>
#include <map>
#include <string>
#include "Mod/Engine/ModPlants.h"
#include "Mod/Engine/Level70Boss.h"

namespace pvzu::engine {

namespace u = pvzu::unnamed;

namespace {

int Mode(LawnApp* theApp) {
	const u::ModModeDef* aDef = CurrentModMode(theApp);
	return aDef ? aDef->mode : 0;
}

std::string Text(const char* theKey, const char* theFallback) {
	const std::string aKey = std::format("[{}]", theKey);
	return PvzpStringListExists(aKey) ? std::string(PvzpStringTranslate(aKey)) : std::string(theFallback);
}

void LogFew(const char* theTag, const std::string& theLine) {
	static std::map<std::string, int> sCount;
	if (++sCount[theTag] <= 6)
		DevLog(std::format("modModes {} {}", theTag, theLine));
}

bool IsTrickMode(int theMode) { return theMode == u::kTrickOfTricks || theMode == u::kTrickOfTricks2; }

// ---- 0x72 Lone Wolf: the moved plant is the only Peashooter-line plant on the lawn (the bank only holds Ice-shroom) ----
bool IsLoneWolfSeed(int theSeed) { return theSeed == 0 || theSeed == 5 || theSeed == 7 || theSeed == 18 || theSeed == 40; }

Plant* LoneWolfPlant(Board* theBoard) {
	for (Plant* aPlant : theBoard->mPlants)
		if (!aPlant->mDead && IsLoneWolfSeed(aPlant->mSeedType))
			return aPlant;
	return nullptr;
}

void LoneWolfMove(Board* theBoard, Plant* thePlant, int theRow, const char* theBy) {
	if (theRow == thePlant->mRow)
		return;
	// 0x79b0e0 / 0x79b106 (shared with 6-5): mRow +-1, mY +-100; column and mX stay, no CanPlantAt test
	thePlant->mY += (theRow - thePlant->mRow) * 100;
	thePlant->mRow = theRow;
	thePlant->mRenderOrder = thePlant->CalcRenderOrder();
	LogFew("loneWolfMove", std::format("row={} by={}", theRow, theBy));
}

// ---- 0x8b Bullet Carnival: the player is projectile slot 0 with mDamageRangeFlags 0x7f (exe +0x74), its score in
// mCobTargetRow (exe +0x84) ----
constexpr int kCarnivalPlayer = 0x7f;

Projectile* CarnivalPlayer(Board* theBoard) {
	for (Projectile* aProjectile : theBoard->mProjectiles)
		if (!aProjectile->mDead && aProjectile->mDamageRangeFlags == kCarnivalPlayer)
			return aProjectile;
	return nullptr;
}

u::CarnivalRect CarnivalRectOf(const Projectile* theProjectile) {
	return u::CarnivalBulletRect(theProjectile->mProjectileType, theProjectile->mX, theProjectile->mY, theProjectile->mWidth,
		theProjectile->mHeight);
}

// 0x78 / 0x8b glue their object to the pointer (the exe reads the mouse every tick). On the phone a tap on a sun or a
// packet would teleport it there, so a finger (or pen) is followed only while a press that began on bare lawn is held
// (port addition; a mouse is followed always, as in the exe).
bool gFollowPress = false;
int gCarnivalX = 0, gCarnivalY = 0; // 0x8b: where the player was last put (the exe's start is (0, 0))

bool FollowsPointerNow(Board* theBoard) {
	const bool aHeld = gFollowPress && (theBoard->mApp->mWidgetManager->mDownButtons & 1) != 0;
	return pvzu::FollowsPointer(pvzu::LastPointer(), aHeld);
}

void CarnivalTick(Board* theBoard) {
	Projectile* aPlayer = CarnivalPlayer(theBoard);
	if (aPlayer == nullptr)
		return;
	// 1. follow the cursor (0x7b8a9d): row = the cursor preview's row, mX/mY = the cursor object's (mouse - 25 / - 35, as
	// CursorObject::Update writes them; read from the mouse here because the cursor object only updates while it is drawn)
	// (the projectile's own update rewrites mX / mY from mPosX / mPosY, so the place between finger presses is kept here)
	if (FollowsPointerNow(theBoard)) {
		const int aMouseX = MouseBoardX(theBoard, theBoard->mApp->mWidgetManager->mLastMouseX);
		const int aMouseY = MouseBoardY(theBoard, theBoard->mApp->mWidgetManager->mLastMouseY);
		const int aRow = std::clamp(theBoard->PixelToGridY(aMouseX, aMouseY), 0, MAX_GRID_SIZE_Y - 1);
		if (aRow != aPlayer->mRow)
			LogFew("carnivalRow", std::format("row={} x={} y={}", aRow, aMouseX - 25, aMouseY - 35));
		aPlayer->mRow = aRow;
		gCarnivalX = aMouseX - 25;
		gCarnivalY = aMouseY - 35;
	}
	aPlayer->mX = gCarnivalX;
	aPlayer->mPosX = static_cast<float>(aPlayer->mX) + 10.0f;
	aPlayer->mY = gCarnivalY;
	aPlayer->mPosY = static_cast<float>(aPlayer->mY) + 20.0f;
	aPlayer->mShadowY = 1000.0f;
	if (theBoard->mMainCounter % 200 == 0)
		LogFew("carnivalPlayer", std::format("x={} y={} type={} score={}", aPlayer->mX, aPlayer->mY, static_cast<int>(aPlayer->mProjectileType),
			aPlayer->mCobTargetRow));
	// 2. eat one smaller bullet a tick, or die on a bigger one (0x7b8ad0..0x7b8c46)
	u::CarnivalRect aMe = CarnivalRectOf(aPlayer);
	aMe.x += 10;
	aMe.y += 20;
	const int aTier = u::CarnivalTier(aPlayer->mProjectileType);
	for (Projectile* aOther : theBoard->mProjectiles) {
		if (aOther == aPlayer || aOther->mDead || aOther->mProjectileType == ProjectileType::PROJECTILE_FIREBALL)
			continue;
		if (!u::CarnivalOverlap(aMe, CarnivalRectOf(aOther)))
			continue;
		const int aOtherTier = u::CarnivalTier(aOther->mProjectileType);
		if (aOtherTier > aTier) {
			aPlayer->Die();
			// 0x7b8c1a: an invisible Normal zombie at x -100 in row 0: the zombies win at once
			if (Zombie* aZombie = theBoard->AddZombieInRow(ZombieType::ZOMBIE_NORMAL, 0, 0)) {
				aZombie->mVisible = false;
				aZombie->mPosX = -100.0f;
			}
			DevLog(std::format("modModes carnival dies tier={} hit by type={} tier={}", aTier, static_cast<int>(aOther->mProjectileType),
				aOtherTier));
			return;
		}
		aPlayer->mCobTargetRow += u::CarnivalEatScore(aOtherTier);
		aOther->Die();
		LogFew("carnivalEat", std::format("type={} score={}/{}", static_cast<int>(aOther->mProjectileType), aPlayer->mCobTargetRow,
			u::CarnivalGoal(aTier)));
		if (aPlayer->mCobTargetRow >= u::CarnivalGoal(aTier)) {
			const int aNext = u::CarnivalTypeOfTier(aTier + 1);
			if (aNext >= 0) {
				aPlayer->mProjectileType = static_cast<ProjectileType>(aNext);
				aPlayer->mCobTargetRow = 0;
				DevLog(std::format("modModes carnival grows to type={} tier={}", aNext, aTier + 1));
				if (aNext == u::kCarnivalWinType) {
					// 0x7b8bf4: the WIN cheat's writes (FOLEY_WINMUSIC + mBoardFadeOutCounter 600); here the level award path
					theBoard->mApp->PlayFoley(FoleyType::FOLEY_WINMUSIC);
					theBoard->FadeOutLevel();
					DevLog("modModes carnival win");
					return;
				}
			}
		}
		break; // one bullet a tick (0x7b8bb5 / 0x7b8bee leave the loop)
	}
	// 3. a new bullet every 8 ticks (0x7b8c4c)
	if (theBoard->mMainCounter % 8 != 0)
		return;
	PvzpWeightedArray aPick[13];
	for (int t = 0; t < 13; t++) {
		aPick[t].mItem = t;
		aPick[t].mWeight = u::CarnivalWeight(t, u::CarnivalTier(aPlayer->mProjectileType));
	}
	const int aType = static_cast<int>(PvzpPickFromWeightedArray(aPick, 13));
	const int aRollX = Sexy::Rand(900);
	const int aRollY = Sexy::Rand(u::CarnivalSpawnEdge(aRollX) ? 680 : 60);
	const u::CarnivalSpawn s = u::CarnivalSpawnPlace(aRollX, aRollY);
	Projectile* aShot = theBoard->AddProjectile(s.x, s.y, 315000, theBoard->PixelToGridY(s.x, s.y), static_cast<ProjectileType>(aType));
	if (aShot == nullptr)
		return;
	aShot->mMotionType = ProjectileMotion::MOTION_STAR;
	aShot->mRotationSpeed = 0.05f;
	aShot->mDamageRangeFlags = 0;
	aShot->mPosX = static_cast<float>(s.x);
	aShot->mPosY = static_cast<float>(s.y);
	const float aTargetX = static_cast<float>(Sexy::Rand(601) + 100);
	const float aTargetY = static_cast<float>(Sexy::Rand(441) + 80);
	const float aSpeed = RandRangeFloat(0.8f, 1.1f);
	const float dx = aTargetX - aShot->mPosX, dy = aTargetY - aShot->mPosY;
	const float aDist = std::sqrt(dx * dx + dy * dy);
	if (aDist > 0.0f) {
		aShot->mVelX = dx / aDist * aSpeed;
		aShot->mVelY = dy / aDist * aSpeed;
	}
	LogFew("carnivalSpawn", std::format("type={} at {},{} v={:.2f},{:.2f}", aType, s.x, s.y, aShot->mVelX, aShot->mVelY));
}

// ---- 0x69 Visual Disorder: the tint the exe writes into Color::White every tick ----
Sexy::Color gDisorderWhite(255, 255, 255, 255);

// ---- 0x92: the Zombie Sea option (exe Board+0x6b8) lives in Challenge::mChallengeScore ----
bool gEliteWaveSpawning = false;

Sexy::Rect SeaBox(const SeedChooserScreen* theChooser) {
	(void)theChooser;
	// the exe: (464, 100) and (624, 100) in its 1067-wide screen; here the second box goes under the first (the English
	// label is longer), and both move up when the CARD panel takes the place right of the chooser
	const int y = CardOn() ? 10 : 100;
	return Sexy::Rect(464, y + 36, 30, 30);
}

} // namespace

// ---- seed chooser closed (hook 462, .text 0x486e60; 0x798fdf) ----

void ModModeChooserClosed(Board* theBoard) {
	const int aMode = Mode(theBoard->mApp);
	if (aMode == 0)
		return;
	SeedBank* aBank = theBoard->mSeedBank.get();
	if (aMode == u::kNightTerror) {
		// 0x7962d5: 25 permanent craters, then rows 1..4 dirt (zombies and mowers only in row 0; the cells stay plantable)
		int aCraters = 0;
		for (int aRow = 0; aRow < 5; aRow++)
			for (int aCol = 0; aCol < 9; aCol++)
				if (u::NightTerrorCrater(aCol, aRow)) {
					GridItem* aCrater = theBoard->AddACrater(aCol, aRow);
					aCrater->mGridItemCounter = 0x7fffffff;
					aCraters++;
				}
		for (int aRow = 1; aRow <= 4; aRow++)
			theBoard->mPlantRow[aRow] = PlantRowType::PLANTROW_DIRT;
		DevLog(std::format("modModes nightTerror craters={} dirt rows 1-4", aCraters));
	} else if (aMode == u::kLoneWolf) {
		// 0x7983b6: sun 0, one packet that is always Ice-shroom, a Peashooter at (0, 0) firing every 50 cs
		theBoard->mSunMoney = 0;
		aBank->mNumPackets = 1;
		aBank->mSeedPackets[0].mPacketType = SeedType::SEED_ICESHROOM;
		Plant* aPlant = theBoard->AddPlant(0, 0, SeedType::SEED_PEASHOOTER, SeedType::SEED_NONE);
		if (aPlant)
			aPlant->mLaunchRate = u::LoneWolfLaunchRate(SeedType::SEED_PEASHOOTER);
		DevLog(std::format("modModes loneWolf start plant={} rate={} sun=0 packet=ICESHROOM", aPlant ? 0 : -1,
			aPlant ? aPlant->mLaunchRate : -1));
	} else if (aMode == u::kAllTheSame || aMode == u::kSlotCrisis) {
		// 0xc030e0 -> 0x798fdf (0x91 and 0x73): six packets of the chosen seed, refresh times 0
		const int aSeed = u::AllTheSameSeed(aBank->mSeedPackets[0].mPacketType, aBank->mSeedPackets[0].mImitaterType);
		if (aSeed < 0)
			return;
		theBoard->mChallenge->mChallengeScore = 6; // GetNumSeedsInBank says 6 from now on (ModModeSeedSlots)
		aBank->UpdateWidth(); // the exe writes the bank width 446 and mNumPackets 6
		for (int i = 1; i < 6; i++)
			aBank->mSeedPackets[i].mPacketType = static_cast<SeedType>(aSeed);
		for (int i = 0; i < 6; i++)
			aBank->mSeedPackets[i].mRefreshTime = 0;
		DevLog(std::format("modModes sixCopies mode=0x{:x} seed={}", aMode, aSeed));
	} else if (aMode == u::kGloveWar) {
		// V2, 0x7a4c50 (hook 462 chain, item mode 1 = 0x6b only): every chosen seed is planted (rows 0-1, no CanPlantAt),
		// then the bank is two packets, Blover and Cherry Bomb (types only; their refresh fields stay as they were)
		int aPlanted = 0;
		for (int i = 0; i < aBank->mNumPackets; i++) {
			SeedPacket& p = aBank->mSeedPackets[i];
			int aCol = 0, aRow = 0;
			u::GloveWarStartCell(i, aCol, aRow);
			const int aSeed = u::GloveWarStartSeed(p.mPacketType, p.mImitaterType);
			if (theBoard->AddPlant(aCol, aRow, static_cast<SeedType>(aSeed), SeedType::SEED_NONE))
				aPlanted++;
			p.mPacketType = SeedType::SEED_NONE;
			p.mImitaterType = SeedType::SEED_NONE;
		}
		theBoard->mChallenge->mChallengeScore = 2; // GetNumSeedsInBank says 2 from now on (ModModeSeedSlots)
		aBank->UpdateWidth(); // the exe writes mNumPackets 2 and re-places the packets (0x489d28)
		for (int i = 0; i < 2 && i < aBank->mNumPackets; i++)
			aBank->mSeedPackets[i].mPacketType = static_cast<SeedType>(u::kGloveWarBank[i]);
		DevLog(std::format("modModes gloveWar start planted={} bank=BLOVER,CHERRYBOMB", aPlanted));
	} else if (aMode == u::kInvisibleGuardian) {
		// V2, 0x798f01: shuffle the chosen packets (Imitater resolved), then hide every card but the sun producers and
		// end its opening recharge (mRefreshTime 0); hidden cards still work
		const int n = aBank->mNumPackets;
		if (n >= 2) {
			std::vector<int> aTypes;
			for (int i = 0; i < n; i++)
				aTypes.push_back(u::AllTheSameSeed(aBank->mSeedPackets[i].mPacketType, aBank->mSeedPackets[i].mImitaterType));
			u::InvisibleGuardianShuffle(aTypes, [](int theRange) { return Sexy::Rand(theRange); });
			for (int i = 0; i < n; i++)
				aBank->mSeedPackets[i].mPacketType = static_cast<SeedType>(aTypes[i]);
			std::string aLine;
			for (int i = 0; i < n; i++) {
				SeedPacket& p = aBank->mSeedPackets[i];
				if (!u::InvisibleGuardianCardShown(p.mPacketType)) {
					p.mVisible = false;
					p.mRefreshTime = 0;
				}
				aLine += std::format("{}{}{}", i ? "," : "", static_cast<int>(p.mPacketType), p.mVisible ? "" : "(hidden)");
			}
			DevLog(std::format("modModes invisibleGuardian bank {}", aLine));
		}
	}
}

// ---- 0x66 Sun Storm ----

bool ModModeSkySun(Board* theBoard, int& theCountdown) {
	if (Mode(theBoard->mApp) != u::kSunStorm)
		return false;
	const int aVanilla = theCountdown;
	theCountdown = u::SunStormCountdown(aVanilla); // hook 95 (0x7990d0)
	LogFew("sunStorm", std::format("sky sun countdown {} -> {}, large sun", aVanilla, theCountdown));
	return true; // COIN_LARGESUN
}

void ModModeCoinFalling(Coin* theCoin, bool theLanded) {
	if (theCoin->mType != CoinType::COIN_LARGESUN || Mode(theCoin->mApp) != u::kSunStorm)
		return;
	// Hooks 205 / 206 (0x798670, 0x798901): the "charged" flag is Coin+0x68 (mUsableSeedType, unused by a sun)
	Reanimation* aReanim = FindReanimAttachment(theCoin->mAttachmentID);
	if (!theLanded && theCoin->mUsableSeedType != 1) {
		theCoin->mUsableSeedType = static_cast<SeedType>(1);
		if (aReanim)
			aReanim->mFilterEffect = FilterEffect::FILTER_EFFECT_LESS_WASHED_OUT;
	} else if (theLanded && theCoin->mUsableSeedType != 0 && aReanim) {
		theCoin->mUsableSeedType = static_cast<SeedType>(0);
		aReanim->mFilterEffect = FilterEffect::FILTER_EFFECT_NONE;
	}
}

bool ModModeCoinCollected(Coin* theCoin) {
	if (theCoin->mType != CoinType::COIN_LARGESUN || theCoin->mUsableSeedType != 1 || Mode(theCoin->mApp) != u::kSunStorm)
		return false;
	// Hook 208 (0x798721): the charged sun explodes instead of flying to the bank (no sun)
	Board* aBoard = theCoin->mBoard;
	theCoin->mVisible = false;
	theCoin->mWidth >>= 1;
	theCoin->mApp->AddPvzpParticle(theCoin->mPosX + theCoin->mWidth, theCoin->mPosY + theCoin->mWidth, 400000,
		ParticleEffect::PARTICLE_DOOM);
	theCoin->mApp->PlayFoley(FoleyType::FOLEY_CHERRYBOMB);
	const int cx = static_cast<int>(std::round(theCoin->mPosX)) - 30;
	const int cy = static_cast<int>(std::round(theCoin->mPosY)) - 30;
	int aZombies = 0, aPlants = 0;
	for (Zombie* aZombie : aBoard->mZombies)
		if (!aZombie->mDead && u::SunStormHitsZombie(cx, cy, aZombie->mX, aZombie->mY)) {
			aZombie->TakeDamage(u::SunStormBlastDamage(), 0U);
			aZombies++;
		}
	for (Plant* aPlant : aBoard->mPlants)
		if (!aPlant->mDead && u::SunStormHitsPlant(cx, cy, aPlant->mX, aPlant->mY)) {
			aPlant->mPlantHealth -= u::SunStormBlastDamage();
			aPlant->mEatenFlashCountdown = std::max(aPlant->mEatenFlashCountdown, 25);
			aPlants++;
			if (aPlant->mPlantHealth <= 0)
				aPlant->Die(); // the exe writes mDead directly; Die() also cleans up
		}
	theCoin->mUsableSeedType = static_cast<SeedType>(0);
	theCoin->Die();
	DevLog(std::format("modModes sunStorm blast at {},{} zombies={} plants={}", cx, cy, aZombies, aPlants));
	return true;
}

// ---- 0x6d Night Terror ----

bool ModModeZombieWalk(Zombie* theZombie, float theSpeed) {
	if (Mode(theZombie->mApp) != u::kNightTerror || theZombie->mBoard == nullptr)
		return false;
	// Hook 604 (0x7989c0): the scripted path along the craters
	const int aRowBefore = theZombie->mRow;
	int aRow = aRowBefore;
	const u::TerrorMove m = u::NightTerrorStep(theZombie->mX, theZombie->mY, aRow);
	if (aRow != aRowBefore) {
		theZombie->mRow = aRow;
		theZombie->mRenderOrder = Board::MakeRenderOrder(RenderLayer::RENDER_LAYER_ZOMBIE, aRow, 0);
		LogFew("nightTerrorRow", std::format("type={} x={} y={} row {} -> {}", static_cast<int>(theZombie->mZombieType), theZombie->mX,
			theZombie->mY, aRowBefore, aRow));
	}
	if (m == u::TerrorMove::WalkRight)
		theZombie->mPosX += theSpeed;
	else if (m == u::TerrorMove::Walk)
		theZombie->mPosX += theZombie->mZombiePhase == ZombiePhase::PHASE_DANCER_DANCING_IN ? theSpeed : -theSpeed;
	return true;
}

float ModModeZombieYStep(Zombie* theZombie, float theGap) {
	if (Mode(theZombie->mApp) == u::kNightTerror)
		return u::NightTerrorYStep(theGap); // hooks 616 / 617
	return std::min(theGap, 1.0f);
}

bool ModModeNoElites(Zombie* theZombie) {
	// 0x7a3081: elite variant -1 in 0x6d (the boss-class exceptions of 0x7aa320 never spawn here); the port skips the roll
	return Mode(theZombie->mApp) == u::kNightTerror;
}

// ---- 0x72 Lone Wolf ----

bool ModModeLoneWolfKey(Board* theBoard, Sexy::KeyCode theKey) {
	if (Mode(theBoard->mApp) != u::kLoneWolf)
		return false;
	const u::Steer aDir = u::SteerFromArrow(theKey); // hook 138 (0x79b0a0): Up / Down
	if (aDir == u::Steer::None)
		return false;
	Plant* aPlant = LoneWolfPlant(theBoard);
	if (aPlant == nullptr)
		return false;
	LoneWolfMove(theBoard, aPlant, u::ShooterStepRow(aPlant->mRow, aDir, 5), aDir == u::Steer::Up ? "up" : "down");
	return false; // the exe passes the key on to the cheat-word buffer
}

bool ModModeTakesTouch(Board* theBoard) {
	const int aMode = Mode(theBoard->mApp);
	return aMode == u::kLoneWolf || aMode == u::kZengWasd;
}

namespace {
void ZengTouch(Board* theBoard, const u::Touch& theTouch, int theTapX, int theTapY);
}

void ModModeFollowPress(Board* theBoard, int x, int y, bool theOnBareLawn) {
	gFollowPress = false;
	const int aMode = Mode(theBoard->mApp);
	if ((aMode != u::kSunflowerCommander && aMode != u::kBulletCarnival) || !theOnBareLawn || theBoard->mPaused)
		return; // (0x8b runs during the level intro too, as the mouse does)
	LocalToBoard(theBoard, x, y);
	gFollowPress = theBoard->PixelToGridY(x, y) >= 0; // on the lawn: not the sun counter or the bank's empty space
}

void ModModeFollowLift() { gFollowPress = false; }

void ModModeTouch(Board* theBoard, const u::Touch& theTouch, int theTapX, int theTapY) {
	if (Mode(theBoard->mApp) == u::kZengWasd) {
		ZengTouch(theBoard, theTouch, theTapX, theTapY);
		return;
	}
	Plant* aPlant = LoneWolfPlant(theBoard);
	if (aPlant == nullptr)
		return;
	const int aTapRow = theTouch.swipe ? -1 : theBoard->PixelToGridY(theTapX, theTapY);
	LoneWolfMove(theBoard, aPlant, u::ShooterTouchRow(aPlant->mRow, theTouch, aTapRow, 5), theTouch.swipe ? "swipe" : "tap");
}

void ModModeHeadDropped(Zombie* theZombie) {
	if (theZombie->mBoard == nullptr || Mode(theZombie->mApp) != u::kLoneWolf)
		return;
	// Hook 596 -> 0x7984c1: sun += value x 5 (straight into Board+0x5560), then one upgrade step in place
	Board* aBoard = theZombie->mBoard;
	Plant* aPlant = LoneWolfPlant(aBoard);
	if (aPlant == nullptr)
		return;
	aBoard->mSunMoney += u::LoneWolfKillSun(GetZombieDefinition(theZombie->mZombieType).mZombieValue);
	const int aNext = u::LoneWolfUpgrade(aPlant->mSeedType, aBoard->mSunMoney);
	LogFew("loneWolfHead", std::format("type={} sun={} seed={} next={}", static_cast<int>(theZombie->mZombieType), aBoard->mSunMoney,
		static_cast<int>(aPlant->mSeedType), aNext));
	if (aNext == aPlant->mSeedType)
		return;
	const int aCol = aPlant->mPlantCol, aRow = aPlant->mRow;
	aPlant->Die();
	if (Plant* aNew = aBoard->AddPlant(aCol, aRow, static_cast<SeedType>(aNext), SeedType::SEED_NONE)) {
		aNew->mLaunchRate = u::LoneWolfLaunchRate(aNext);
		DevLog(std::format("modModes loneWolf upgrade -> seed={} row={} rate={} sun={}", aNext, aRow, aNew->mLaunchRate, aBoard->mSunMoney));
	}
}

// ---- 0x7f / 0x90 Trick of Tricks (count in Challenge::mChallengeScore, exe Challenge+0x24) ----

namespace {
int gSquashHits = 0;
}

void ModModeSquashSmash(Plant* thePlant, bool theBefore) {
	if (!IsTrickMode(Mode(thePlant->mApp)))
		return;
	if (theBefore) {
		gSquashHits = 0; // hook 297 (0x7a7721): plant+0xcc = 0 before DoSquashDamage
		return;
	}
	Challenge* c = thePlant->mBoard->mChallenge.get();
	const u::TrickMiss m = u::TrickOfTricksSmash(c->mChallengeScore, gSquashHits);
	if (m.counted)
		DevLog(std::format("modModes trick miss count={} bag={} col={} row={}", c->mChallengeScore, m.bag ? 1 : 0, thePlant->mPlantCol,
			thePlant->mRow));
	if (m.bag)
		thePlant->mBoard->AddCoin(thePlant->mX, thePlant->mY, CoinType::COIN_AWARD_MONEY_BAG, CoinMotion::COIN_MOTION_FROM_PLANT);
}

void ModModeSquashHit(Plant* thePlant, Zombie* theZombie) {
	if (thePlant->mSeedType == SeedType::SEED_SQUASH && theZombie->mHasHead && IsTrickMode(Mode(thePlant->mApp)))
		gSquashHits++; // hook 296 (0x7a77d0)
}

bool ModModeFinalWaveLost(Board* theBoard) {
	if (!IsTrickMode(Mode(theBoard->mApp)) || !u::TrickOfTricksLost(theBoard->mChallenge->mChallengeScore))
		return false;
	// Hook 96 (0x7a7c00 -> 0xc02e50): the exe sets mGameScene = 4 directly; the port takes the normal loss path
	DevLog(std::format("modModes trick lost count={} at the final wave", theBoard->mChallenge->mChallengeScore));
	theBoard->ZombiesWon(nullptr);
	return true;
}

// ---- 0x88 Territorial (claims in Challenge::mBeghouledEated[col][row], exe Challenge+0x14 + col + 9*row) ----

void ModModePlantAdded(Plant* thePlant) {
	if (thePlant->mBoard == nullptr || Mode(thePlant->mApp) != u::kTerritorial)
		return;
	if (thePlant->mPlantCol < 0 || thePlant->mPlantCol >= MAX_GRID_SIZE_X || thePlant->mRow < 0 || thePlant->mRow >= MAX_GRID_SIZE_Y)
		return;
	int32_t& aCell = thePlant->mBoard->mChallenge->mBeghouledEated[thePlant->mPlantCol][thePlant->mRow];
	const int aBefore = aCell;
	aCell = u::TerritorialStore(aCell, thePlant->mSeedType); // hook 38 (0x7a8a21)
	if (aCell != aBefore)
		LogFew("territorial", std::format("claim col={} row={} seed={}", thePlant->mPlantCol, thePlant->mRow, static_cast<int>(thePlant->mSeedType)));
}

bool ModModeArtSeed(Board* theBoard, int theGridX, int theGridY, SeedType& theSeed) {
	if (theBoard == nullptr || Mode(theBoard->mApp) != u::kTerritorial)
		return false;
	theSeed = SeedType::SEED_NONE; // hook 168 (0x7a8ba1)
	if (theGridX >= 0 && theGridX < MAX_GRID_SIZE_X && theGridY >= 0 && theGridY < MAX_GRID_SIZE_Y)
		theSeed = static_cast<SeedType>(u::TerritorialSeed(theBoard->mChallenge->mBeghouledEated[theGridX][theGridY]));
	return true;
}

bool ModModeArtGrid(LawnApp* theApp) { return theApp->mBoard && Mode(theApp) == u::kTerritorial; } // hooks 170, 174

bool ModModeArtRefuses(Board* theBoard, int theGridX, int theGridY, SeedType theSeed) {
	SeedType aClaimed = SeedType::SEED_NONE;
	if (!ModModeArtSeed(theBoard, theGridX, theGridY, aClaimed))
		return false;
	const bool aRefuses = u::TerritorialRefuses(aClaimed, theSeed);
	if (aRefuses)
		LogFew("territorialRefuse", std::format("col={} row={} claimed={} seed={}", theGridX, theGridY, static_cast<int>(aClaimed),
			static_cast<int>(theSeed)));
	return aRefuses;
}

// ---- 0x8b Bullet Carnival ----

bool ModModeCarnivalYoung(Projectile* theProjectile) {
	return theProjectile->mProjectileAge < 300 && Mode(theProjectile->mApp) == u::kBulletCarnival; // hook 376 (0xa872d9)
}

bool ModModeCarnivalKeepsStar(Projectile* theProjectile) {
	return Mode(theProjectile->mApp) == u::kBulletCarnival &&
		(theProjectile->mProjectileAge < 300 || theProjectile->mDamageRangeFlags == kCarnivalPlayer); // hook 379 (0xa87303)
}

bool ModModeNonScrolling(LawnApp* theApp) { return Mode(theApp) == u::kBulletCarnival; } // hook 219 (0xa8790b)

bool ModModeSkipsStarCurves(LawnApp* theApp) { return Mode(theApp) == u::kBulletCarnival; } // 0x95b712 (0x89: SnakeOn)

// ---- 0x91 All the Same One ----

bool ModModeFreeUpgrades(Board* theBoard) {
	const int aMode = Mode(theBoard->mApp);
	return aMode == u::kRandomVasebreaker || aMode == u::kAllTheSame || aMode == u::kGloveWar3; // hook 53 (0x7a8770)
}

bool ModModeRequirementsMet(Board* theBoard) { return Mode(theBoard->mApp) == u::kAllTheSame; } // hook 153 (0xc03320)

void ModModeSunLabel(Board* theBoard, std::string& theLabel) {
	if (Mode(theBoard->mApp) == u::kAllTheSame)
		theLabel = Text("PVZU_ALL_THE_SAME_UNLIMITED", "Unlimited"); // hook 471 (0xc031f3 "无限制")
}

// ---- 0x92 Elite Zombies' Challenge ----

bool ModModeIsSecondUncle(LawnApp* theApp) { return theApp && Mode(theApp) == u::kSecondUncle; }

bool ModModeOwnWave(Board* theBoard) {
	if (Mode(theBoard->mApp) == u::kSecondUncle)
		Level70BossWave(theBoard); // 0x7a07be: the boss comes when wave index 10 starts; waves 11..40 go on after it
	if (Mode(theBoard->mApp) != u::kEliteChallenge)
		return false;
	// Hook 90 (0xc05074) inside hook 100's [0x7968eb] = 1: every slot spawns the Dancer, as the White-light Dance King
	const bool aSea = theBoard->mChallenge->mChallengeScore != 0;
	gEliteWaveSpawning = true;
	int aCount = 0;
	for (int i = 0; i < MAX_ZOMBIES_IN_WAVE; i++) {
		const int aType = u::EliteChallengeSlot(theBoard->mZombiesInWave[theBoard->mCurrentWave][i], aSea);
		if (aType < 0)
			break;
		theBoard->AddZombie(static_cast<ZombieType>(aType), theBoard->mCurrentWave);
		aCount++;
	}
	gEliteWaveSpawning = false;
	DevLog(std::format("modModes elite wave={} dancers={} sea={}", theBoard->mCurrentWave, aCount, aSea ? 1 : 0));
	return true;
}

bool ModModeForcesSummoner(Zombie* theZombie) {
	return gEliteWaveSpawning && theZombie->mZombieType == ZombieType::ZOMBIE_DANCER;
}

void ModModeChooserDraw(SeedChooserScreen* theChooser, Sexy::Graphics* g) {
	if (Mode(theChooser->mApp) != u::kEliteChallenge || theChooser->mBoard == nullptr)
		return;
	// Hook 469 chain (0xc04ee0): a ticked box "White-light Dance King" (always on) and the "Zombie Sea" box
	const Sexy::Rect aBox = SeaBox(theChooser);
	const bool aSea = theChooser->mBoard->mChallenge->mChallengeScore != 0;
	g->DrawImage(Sexy::IMAGE_OPTIONS_CHECKBOX1, aBox.mX, aBox.mY - 36);
	PvzpDrawString(g, Text("PVZU_ELITE_DANCE_KING", "White-light Dance King"), aBox.mX + 36, aBox.mY - 36 + 22,
		Sexy::FONT_DWARVENTODCRAFT15, Sexy::Color(255, 255, 255), DS_ALIGN_LEFT);
	g->DrawImage(aSea ? Sexy::IMAGE_OPTIONS_CHECKBOX1 : Sexy::IMAGE_OPTIONS_CHECKBOX0, aBox.mX, aBox.mY);
	PvzpDrawString(g, Text("PVZU_ELITE_ZOMBIE_SEA", "Zombie Sea"), aBox.mX + 36, aBox.mY + 22, Sexy::FONT_DWARVENTODCRAFT15,
		Sexy::Color(255, 255, 255), DS_ALIGN_LEFT);
}

bool ModModeChooserClick(SeedChooserScreen* theChooser, int x, int y) {
	if (Mode(theChooser->mApp) != u::kEliteChallenge || theChooser->mBoard == nullptr)
		return false;
	const Sexy::Rect aBox = SeaBox(theChooser);
	if (!aBox.Contains(x, y))
		return false;
	int& aSea = theChooser->mBoard->mChallenge->mChallengeScore;
	aSea = aSea ? 0 : 1;
	theChooser->mApp->PlayFoley(FoleyType::FOLEY_BLEEP);
	DevLog(std::format("modModes elite zombieSea={}", aSea));
	return true;
}

// ---- per tick and draw (called from ModModeRules.cpp's ModModeUpdate / ModModeDraw) ----

void ModModeUpdate2(Board* theBoard, int theMode) {
	Challenge* c = theBoard->mChallenge.get();
	if (theMode == u::kSunflowerCommander) {
		// Hook 104 (0x7a7580): a 100-HP Sunflower glued to the cursor; when it is gone the level is lost
		Plant* aPlant = c->mSlotMachineRollCount ? theBoard->mPlants.DataArrayTryToGet(static_cast<unsigned int>(c->mSlotMachineRollCount))
		                                         : nullptr;
		if (c->mSlotMachineRollCount == 0) {
			aPlant = theBoard->AddPlant(0, 0, SeedType::SEED_SUNFLOWER, SeedType::SEED_NONE); // the exe uses row 6 (moved next tick)
			aPlant->mPlantCol = -1;
			aPlant->mPlantHealth = aPlant->mPlantMaxHealth = 100;
			aPlant->mLaunchRate = 400;
			c->mSlotMachineRollCount = static_cast<int>(theBoard->mPlants.DataArrayGetID(aPlant));
			DevLog("modModes commander sunflower hp=100 rate=400");
		}
		if (aPlant && !aPlant->mDead) {
			if (FollowsPointerNow(theBoard)) { // a finger only while it holds a lawn press (port addition)
				const int aMouseX = MouseBoardX(theBoard, theBoard->mApp->mWidgetManager->mLastMouseX);
				const int aMouseY = MouseBoardY(theBoard, theBoard->mApp->mWidgetManager->mLastMouseY);
				aPlant->mX = aMouseX - 40;
				aPlant->mY = aMouseY - 40;
				const int aRow = std::clamp(theBoard->PixelToGridY(aMouseX, aMouseY), 0, MAX_GRID_SIZE_Y - 1);
				if (aRow != aPlant->mRow) {
					aPlant->mRow = aRow;
					LogFew("commanderRow", std::format("row={} x={} y={}", aRow, aPlant->mX, aPlant->mY));
				}
			}
			aPlant->mRenderOrder = aPlant->CalcRenderOrder();
		} else if (theBoard->mApp->mGameScene == GameScenes::SCENE_PLAYING) {
			DevLog("modModes commander sunflower eaten: lost");
			theBoard->ZombiesWon(nullptr); // the exe sets mGameScene = 4 directly
		}
	} else if (theMode == u::kBulletCarnival) {
		theBoard->mShowShovel = false; // hook 229
		CarnivalTick(theBoard);
	} else if (theMode == u::kAllTheSame) {
		theBoard->mSunMoney = u::kAllTheSameSun; // hook 471
	} else if (u::ItemModeOf(theMode) && theBoard->mApp->mZenGarden) {
		theBoard->mApp->mZenGarden->mBoard = theBoard; // hook 103 chain 0x7a47f0
		if (theBoard->mCursorObject->mCursorType == CursorType::CURSOR_TYPE_PLANT_FROM_GLOVE && ModModeGlovePlant(theBoard) == nullptr) {
			// port safety: the held lawn plant was eaten; drop the glove (the exe would read a dead plant)
			theBoard->ClearCursor();
			DevLog("modModes glove plant gone: cursor cleared");
		}
	}
	if (theMode == u::kSecondUncle) {
		Level70BossUpdate(theBoard); // the boss's attack loop (0x7ab000)
	} else if (theMode == u::kVisualDisorder) {
		// Hook 103 (0x798ca8): the exe changes the global white (0x72289c = Sexy::Color::White) every tick, so everything
		// drawn with it (reanims, colorized images, fills) turns faint and, from wave 12, tinted. The port keeps the tint in
		// gDisorderWhite and puts it in Color::White only while the board is in play (ModModeVisualWhite, also at the top of
		// Board::Draw): the pause menu, options, almanac and the zombies-won screen are drawn in plain white.
		gDisorderWhite.mAlpha = u::kVisualDisorderAlpha;
		if (u::VisualDisorderRecolours(theBoard->mCurrentWave, theBoard->mMainCounter)) {
			gDisorderWhite.mRed = Sexy::Rand(255);
			gDisorderWhite.mGreen = Sexy::Rand(255);
			gDisorderWhite.mBlue = Sexy::Rand(255);
			LogFew("visualDisorder", std::format("wave={} white=({},{},{},{})", theBoard->mCurrentWave, gDisorderWhite.mRed,
				gDisorderWhite.mGreen, gDisorderWhite.mBlue, gDisorderWhite.mAlpha));
		}
		ModModeVisualWhite(theBoard);
	}
}

void ModModeDraw2(Board* theBoard, Sexy::Graphics* g, int theMode) {
	if (IsTrickMode(theMode)) {
		// Hook 234 (0x7a7f80): "[quote not included]" at x 660 (645 from 10), y 60; red, the exe's >= 15 colour reads as yellow
		const int aCount = theBoard->mChallenge->mChallengeScore;
		const std::string aText = std::vformat(Text("PVZU_TRICK_COUNTER", "Squash tricked: {}/{}"), std::make_format_args(aCount, u::kTricksToWin));
		PvzpDrawString(g, aText, u::TrickHudX(aCount) - 60, 60, Sexy::FONT_DWARVENTODCRAFT15,
			aCount >= u::kTricksToWin ? Sexy::Color(255, 255, 0) : Sexy::Color(255, 0, 0), DS_ALIGN_LEFT);
	} else if (theMode == u::kBulletCarnival) {
		// Hook 127 (0xa877c1): a 40 x 10 bar under the player, filled score / goal
		if (Projectile* aPlayer = CarnivalPlayer(theBoard)) {
			const int aGoal = std::max(1, u::CarnivalGoal(u::CarnivalTier(aPlayer->mProjectileType)));
			const int x = aPlayer->mX, y = aPlayer->mY + 40;
			g->SetColor(Sexy::Color(0, 0, 0));
			g->DrawRect(x, y, 40, 10);
			g->SetColor(Sexy::Color(255, 240, 0));
			g->FillRect(x + 1, y + 1, 40 * aPlayer->mCobTargetRow / aGoal, 9);
		}
	}
}

void ModModeStart2(Board* theBoard, int theMode) {
	if (theMode == u::kLoneWolf) {
		// Hook 160 (0x79a000): "[quote not included]" 0xa20100, MESSAGE_STYLE_BIG_MIDDLE, 250 cs
		theBoard->mAdvice->SetLabel(Text("PVZU_LONE_WOLF_MOVE", "Use the Up and Down keys to move the plant\n(phone: tap a row or swipe)"),
			MessageStyle::MESSAGE_STYLE_BIG_MIDDLE);
		theBoard->mAdvice->mDuration = 250;
	} else if (IsTrickMode(theMode)) {
		// Hook 243 (0x7a7b51): "[quote not included]" (4, 15), MESSAGE_STYLE_TUTORIAL_LATER, 500 cs
		const std::string aText = std::vformat(Text("PVZU_TRICK_GOAL", "Trick the Squash {1} times before flag {0}!"),
			std::make_format_args(u::kTrickFlags, u::kTricksToWin));
		theBoard->mAdvice->SetLabel(aText, MessageStyle::MESSAGE_STYLE_TUTORIAL_LATER);
		theBoard->mAdvice->mDuration = 500;
	}
}

void ModModeInit2(Board* theBoard, int theMode) {
	if (theMode == u::kBulletCarnival) {
		// Hook 159 (0xa8727e, end of Challenge::InitLevel): the player, a Kernel at (0, 0), render 360000, row 0
		Projectile* aPlayer = theBoard->AddProjectile(0, 0, 360000, 0, ProjectileType::PROJECTILE_KERNEL);
		aPlayer->mMotionType = ProjectileMotion::MOTION_STAR;
		aPlayer->mRotationSpeed = 0.05f;
		aPlayer->mDamageRangeFlags = kCarnivalPlayer;
		aPlayer->mCobTargetRow = 0;
		gCarnivalX = gCarnivalY = 0;
		DevLog("modModes carnival player kernel");
	} else if (IsTrickMode(theMode) || theMode == u::kTerritorial || theMode == u::kEliteChallenge) {
		theBoard->mChallenge->mChallengeScore = 0;
		for (auto& aCol : theBoard->mChallenge->mBeghouledEated)
			for (int32_t& aCell : aCol)
				aCell = 0;
	}
	if (theMode == u::kTrickOfTricks2) {
		// 0xc02eb0: Squashes on columns 0..2 of every row
		for (int aRow = 0; aRow < 5; aRow++)
			for (int aCol = 0; aCol < 3; aCol++)
				theBoard->AddPlant(aCol, aRow, SeedType::SEED_SQUASH, SeedType::SEED_NONE);
	}
	ModModeInit3(theBoard, theMode);
}

// ---- third pass: the modes that waited for the mod plants (0x71, 0x76, 0x7d, 0x8d, 0x8e, 0x8f) ----

// 0x76: a dying zombie stays hittable (hook 714) and a hit revives it (hook 706); the corpse of most types never fades
// (hook 725)
bool ModModeDyingHittable(Zombie* theZombie) {
	return theZombie->mZombiePhase == ZombiePhase::PHASE_ZOMBIE_DYING && !theZombie->mDead && Mode(theZombie->mApp) == u::kSchrodinger;
}

void ModModeZombieRevive(Zombie* theZombie) {
	if (!ModModeDyingHittable(theZombie))
		return;
	theZombie->mZombiePhase = ZombiePhase::PHASE_ZOMBIE_NORMAL;
	LogFew("schrodinger", std::format("type={} revived by a hit at x={:.0f}", static_cast<int>(theZombie->mZombieType), theZombie->mPosX));
}

bool ModModeKeepsCorpse(Zombie* theZombie) {
	return Mode(theZombie->mApp) == u::kSchrodinger && u::SchrodingerKeepsCorpse(theZombie->mZombieType);
}

// 0x7d / 0x8f: Vasebreaker rules (0x8f: LawnApp::IsScaryPotterLevel, .text 0x4538f0 -> 0xc01cd0; 0x7d: a port ruling,
// the exe extends only the populate, the slot count and the bank slide-in, so its vases could never be opened)
bool ModModeIsVasebreaker(LawnApp* theApp) {
	const int aMode = Mode(theApp);
	return aMode == u::kRandomVasebreaker || aMode == u::kTallnutProbe;
}

bool ModModeScaryPopulate(Challenge* theChallenge, PvzpWeightedGridArray* theGrid, int theCount) {
	const int aMode = Mode(theChallenge->mApp);
	if (aMode == u::kTallnutProbe)
		return true; // hook 158 reaches the vanilla switch with no case: no vases here (Board::InitLevel places two)
	if (aMode != u::kRandomVasebreaker)
		return false;
	// Hook 187 (0x7a8611): columns 0-1 weight 0, then 35 single pots, each from Rand(65)
	for (int i = 0; i < theCount; i++)
		if (theGrid[i].mX <= 1)
			theGrid[i].mWeight = 0;
	int aSeeds = 0, aModSeeds = 0, aZombies = 0, aSuns = 0;
	for (int i = 0; i < 35; i++) {
		const u::VaseRoll v = u::RandomVaseRoll(Sexy::Rand(65));
		if (v.kind == u::VaseKind::Sun) {
			theChallenge->ScaryPotterPlacePot(ScaryPotType::SCARYPOT_SUN, ZombieType::ZOMBIE_INVALID, SeedType::SEED_NONE, 1, theGrid, theCount);
			aSuns++;
		} else if (v.kind == u::VaseKind::Zombie) {
			theChallenge->ScaryPotterPlacePot(ScaryPotType::SCARYPOT_ZOMBIE, static_cast<ZombieType>(v.zombie), SeedType::SEED_NONE, 1, theGrid,
				theCount);
			aZombies++;
		} else {
			int s = 0;
			do
				s = Sexy::Rand(128); // 0x7b2350
			while (!u::RandomVaseSeedOk(s, IsModSeed(static_cast<SeedType>(s))));
			theChallenge->ScaryPotterPlacePot(ScaryPotType::SCARYPOT_SEED, ZombieType::ZOMBIE_INVALID, static_cast<SeedType>(s), 1, theGrid,
				theCount);
			aSeeds++;
			aModSeeds += s >= 0x50 ? 1 : 0;
		}
	}
	DevLog(std::format("modModes randomVases seeds={} (mod {}) zombies={} suns={}", aSeeds, aModSeeds, aZombies, aSuns));
	return true; // no leaf vases (0x4294e8 skips ScaryPotterChangePotType)
}

void ModModePotZombie(Challenge* theChallenge, GridItem* thePot, Zombie* theZombie) {
	if (theZombie == nullptr || Mode(theChallenge->mApp) != u::kTallnutProbe || thePot->mSunCount != u::kProbeMarker)
		return;
	// Hook 189 (0xc01e00): the marked vase's Tall-nut head becomes the probe boss
	thePot->mSunCount = 0;
	theZombie->mBodyHealth = theZombie->mBodyMaxHealth = u::kProbeBossBody;
	theZombie->mHelmHealth = theZombie->mHelmMaxHealth = u::kProbeBossHelm;
	theZombie->mShieldType = ShieldType::SHIELDTYPE_DOOR; // +0xd8 = 1
	theZombie->mShieldHealth = theZombie->mShieldMaxHealth = u::kProbeBossShield;
	theZombie->AttachShield(); // 0x524370 read as AttachShield (M)
	theZombie->mScaleZombie = u::kProbeBossScale;
	theZombie->mPvzu.variant = 0x100;
	theZombie->UpdateAnimSpeed(); // 0x533000 read as the anim refresh (M)
	int aOpened = 0;
	for (const u::Cell& c : u::kProbeOpenCells) {
		GridItem* aPot = theChallenge->mBoard->GetScaryPotAt(c.col, c.row);
		if (aPot && aPot != thePot && !aPot->mDead) {
			theChallenge->ScaryPotterOpenPot(aPot);
			aOpened++;
		}
	}
	DevLog(std::format("modModes probe boss body={} helm={} shield={} scale={:.2f} opened={}", theZombie->mBodyHealth, theZombie->mHelmHealth,
		theZombie->mShieldHealth, theZombie->mScaleZombie, aOpened));
}

// 0x8d: a packet click toggles its lawn plant between the vanilla and the +80 form (0xc007d0). SeedPacket::MouseDown
// calls this where the exe jumps (0x488dae): after the vanilla recharge, CanTakeSunMoney and PlantingRequirementsMet
// tests, so a packet still recharging or too dear buzzes as usual.
bool ModModePacketClick(SeedPacket* thePacket) {
	if (Mode(thePacket->mApp) != u::kGloveWar3 || thePacket->mBoard == nullptr)
		return false;
	Board* aBoard = thePacket->mBoard;
	const int aType = thePacket->mPacketType;
	if (aType < 0 || aType >= 0x80)
		return false;
	const int aOther = u::GloveWar3Toggle(aType);
	Plant* aFound = nullptr;
	for (Plant* aPlant : aBoard->mPlants)
		if (!aPlant->mDead && static_cast<int>(ModSeedOf(aPlant)) == aOther) {
			aFound = aPlant;
			break;
		}
	u::Cell aCell{0, 0};
	if (aFound == nullptr) {
		// the exe: AddPlant(0, 0) with no CanPlantAt; port ruling: never a second plant in one cell
		std::array<bool, 45> aFree{};
		for (int aCol = 0; aCol < 9; aCol++)
			for (int aRow = 0; aRow < 5; aRow++)
				aFree[u::GloveWar3CellIndex(aCol, aRow)] =
					aBoard->CanPlantAt(aCol, aRow, static_cast<SeedType>(aType)) == PlantingReason::PLANTING_OK;
		aCell = u::GloveWar3PlantCell(aFree);
		if (aCell.col < 0) {
			thePacket->mApp->PlaySample(Sexy::SOUND_BUZZER);
			LogFew("gloveWar3Full", std::format("packet {}: no free cell for form 0x{:x}", thePacket->mIndex, aType));
			return true;
		}
	}
	const int aSunBefore = aBoard->mSunMoney;
	aBoard->mSunMoney = u::GloveWar3SunAfter(aSunBefore, Plant::GetCost(static_cast<SeedType>(aType), SeedType::SEED_NONE));
	const int aRefresh = Plant::GetRefreshTime(static_cast<SeedType>(aOther), SeedType::SEED_NONE);
	if (aFound) {
		thePacket->mRefreshTime = aRefresh + aFound->mPlantMaxHealth - aFound->mPlantHealth;
		const int aCol = aFound->mPlantCol, aRow = aFound->mRow, aHp = aFound->mPlantHealth;
		aFound->Die();
		if (Plant* aNew = aBoard->AddPlant(aCol, aRow, static_cast<SeedType>(aType), SeedType::SEED_NONE)) {
			aNew->mPlantHealth += aHp;
			aNew->mPlantMaxHealth += aHp;
		}
	} else {
		thePacket->mRefreshTime = u::GloveWar3NoPlantRefresh(aRefresh);
		aBoard->AddPlant(aCell.col, aCell.row, static_cast<SeedType>(aType), SeedType::SEED_NONE);
	}
	thePacket->mRefreshCounter = 0;
	thePacket->mActive = false;
	thePacket->mRefreshing = true;
	thePacket->mPacketType = static_cast<SeedType>(aOther);
	LogFew("gloveWar3", std::format("packet {} -> form 0x{:x}{}, now 0x{:x}, refresh={} sun={}->{}", thePacket->mIndex, aType,
		aFound ? std::string() : std::format(" planted at ({},{})", aCell.col, aCell.row), aOther, thePacket->mRefreshTime, aSunBefore,
		aBoard->mSunMoney));
	return true;
}

// 0x8e: the moved plant is plant slot 0 (Board+0xac); its id lives in Challenge::mSlotMachineRollCount
namespace {
Plant* ZengPlant(Board* theBoard) {
	const int aId = theBoard->mChallenge->mSlotMachineRollCount;
	Plant* aPlant = aId ? theBoard->mPlants.DataArrayTryToGet(static_cast<unsigned int>(aId)) : nullptr;
	return aPlant && !aPlant->mDead ? aPlant : nullptr;
}

// One W/A/S/D step of the Gloom-shroom onto a cell where a Peashooter could go (hook 138, 0xc00bd0)
void ZengMove(Board* theBoard, int theKey, const char* theBy) {
	Plant* aPlant = ZengPlant(theBoard);
	if (aPlant == nullptr)
		return;
	int aCol = aPlant->mPlantCol, aRow = aPlant->mRow;
	u::ZengStep(theKey, aCol, aRow);
	if (aCol < 0 || aCol >= MAX_GRID_SIZE_X || aRow < 0 || aRow >= MAX_GRID_SIZE_Y ||
		theBoard->CanPlantAt(aCol, aRow, SeedType::SEED_PEASHOOTER) != PlantingReason::PLANTING_OK)
		return;
	aPlant->mPlantCol = aCol;
	aPlant->mRow = aRow;
	aPlant->mX = theBoard->GridToPixelX(aCol, aRow);
	aPlant->mY = theBoard->GridToPixelY(aCol, aRow);
	aPlant->mRenderOrder = aPlant->CalcRenderOrder();
	LogFew("zengMove", std::format("key={} col={} row={} by={}", static_cast<char>(theKey), aCol, aRow, theBy));
}

// The phone has no W/A/S/D (port addition, as 0x72 / 6-5): a swipe is one step its way, a tap one step toward the cell
void ZengTouch(Board* theBoard, const u::Touch& theTouch, int theTapX, int theTapY) {
	Plant* aPlant = ZengPlant(theBoard);
	if (aPlant == nullptr)
		return;
	const int aTapCol = theTouch.swipe ? -1 : theBoard->PixelToGridX(theTapX, theTapY);
	const int aTapRow = theTouch.swipe ? -1 : theBoard->PixelToGridY(theTapX, theTapY);
	if (const int aKey = u::ZengTouchKey(aPlant->mPlantCol, aPlant->mRow, theTouch, aTapCol, aTapRow))
		ZengMove(theBoard, aKey, theTouch.swipe ? "swipe" : "tap");
}
} // namespace

bool ModModeZengKey(Board* theBoard, Sexy::KeyCode theKey) {
	if (Mode(theBoard->mApp) != u::kZengWasd)
		return false;
	int aKey = static_cast<int>(theKey);
	if (aKey >= 'a' && aKey <= 'z')
		aKey -= 'a' - 'A';
	if (aKey != 'W' && aKey != 'A' && aKey != 'S' && aKey != 'D')
		return false;
	ZengMove(theBoard, aKey, "key");
	return false;
}

bool ModModeRowDamage(Plant* thePlant, Zombie* theZombie, int theDamage, unsigned int theFlags) {
	if (Mode(thePlant->mApp) != u::kZengWasd)
		return false;
	const int aSeed = static_cast<int>(ModSeedOf(thePlant));
	if (aSeed != SeedType::SEED_GLOOMSHROOM && aSeed != u::kModSeedElementShroom)
		return false;
	// Hook 275 (0xc00a60): no Rand(7) gate, the hit goes to the body (0x5312d0 = Zombie::TakeBodyDamage); the Element-shroom
	// adds a homing puff from wave 20
	if (aSeed == u::kModSeedElementShroom && u::ZengPuffs(thePlant->mBoard->mCurrentWave))
		if (Projectile* aPuff = thePlant->mBoard->AddProjectile(thePlant->mX, thePlant->mY, 0, thePlant->mRow, ProjectileType::PROJECTILE_PUFF)) {
			aPuff->mDamageRangeFlags = 1;
			aPuff->mMotionType = ProjectileMotion::MOTION_HOMING;
			aPuff->mTargetZombieID = thePlant->mBoard->ZombieGetID(theZombie);
		}
	theZombie->TakeBodyDamage(theDamage, theFlags);
	LogFew(aSeed == SeedType::SEED_GLOOMSHROOM ? "zengGloomHit" : "zengElementHit", std::format("wave={} type={} body damage {}",
		thePlant->mBoard->mCurrentWave, static_cast<int>(theZombie->mZombieType), theDamage));
	return true;
}

bool ModModePuffImpact(Projectile* theProjectile, Zombie* theZombie, unsigned int theFlags) {
	if (theProjectile->mMotionType != ProjectileMotion::MOTION_HOMING || theProjectile->mProjectileType != ProjectileType::PROJECTILE_PUFF ||
		Mode(theProjectile->mApp) != u::kZengWasd)
		return false;
	theZombie->TakeBodyDamage(u::kZengPuffDamage, theFlags); // hook 396 (0xc01110)
	LogFew("zengPuff", std::format("homing puff hits type={} for {}", static_cast<int>(theZombie->mZombieType), u::kZengPuffDamage));
	return true;
}

void ModModeWave3(Board* theBoard, int theMode) {
	if (theMode == u::kZengWasd && u::ZengElementWave(theBoard->mCurrentWave))
		if (Plant* aPlant = ZengPlant(theBoard)) {
			aPlant->mPvzu.modSeed = static_cast<SeedType>(u::kModSeedElementShroom); // hook 99 (0xc010c0): only the type changes
			DevLog(std::format("modModes zeng wave={} Gloom-shroom -> Element-shroom", theBoard->mCurrentWave));
		}
}

void ModModeInit3(Board* theBoard, int theMode) {
	SeedBank* aBank = theBoard->mSeedBank.get();
	if (theMode == u::kGloveWar3) {
		// 0xc004c8..0xc005cc: ten vanilla plants in columns 0-1, the packets hold their +80 forms
		for (int k = 0; k < 10; k++) {
			const u::GloveWar3Spot s = u::GloveWar3Start(k);
			theBoard->AddPlant(s.col, s.row, static_cast<SeedType>(s.seed), SeedType::SEED_NONE);
			if (k < aBank->mNumPackets)
				aBank->mSeedPackets[k].SetPacketType(static_cast<SeedType>(s.packet), SeedType::SEED_NONE);
		}
		DevLog(std::format("modModes gloveWar3 layout packets={}", aBank->mNumPackets));
	} else if (theMode == u::kZengWasd) {
		// 0xc00b40: packet 0 Blover; a Gloom-shroom at (0, 0) is plant slot 0
		// (0xc00b79 jumps past the vanilla "seeds 0..n in order" fill: the other packets stay empty)
		for (int i = 1; i < aBank->mNumPackets; i++)
			aBank->mSeedPackets[i].mPacketType = SeedType::SEED_NONE;
		if (aBank->mNumPackets > 0)
			aBank->mSeedPackets[0].SetPacketType(SeedType::SEED_BLOVER, SeedType::SEED_NONE);
		if (Plant* aGloom = theBoard->AddPlant(0, 0, SeedType::SEED_GLOOMSHROOM, SeedType::SEED_NONE)) {
			aGloom->SetSleeping(false); // hook 266 skips the PlantInitialize Gloom branch (L: read as "stays awake by day")
			theBoard->mChallenge->mSlotMachineRollCount = static_cast<int>(theBoard->mPlants.DataArrayGetID(aGloom));
		}
	} else if (theMode == u::kTallnutProbe) {
		// 0xc01d00: a seed vase with the Boss Flower at (0, 0) and the marked Tall-nut-head vase at (8, 4)
		struct V {
			u::Cell c;
			ScaryPotType type;
			int seed, zombie, sun;
		};
		const V kVases[2] = {{u::kProbeSeedVase, ScaryPotType::SCARYPOT_SEED, u::kProbeSeed, -1, 0},
			{u::kProbeBossVase, ScaryPotType::SCARYPOT_ZOMBIE, -1, u::kProbeZombie, u::kProbeMarker}};
		for (const V& v : kVases) {
			GridItem* aPot = theBoard->mGridItems.DataArrayAlloc();
			aPot->mGridItemType = GridItemType::GRIDITEM_SCARY_POT;
			aPot->mGridItemState = v.type == ScaryPotType::SCARYPOT_SEED ? GridItemState::GRIDITEM_STATE_SCARY_POT_LEAF
			                                                              : GridItemState::GRIDITEM_STATE_SCARY_POT_ZOMBIE;
			aPot->mGridX = v.c.col;
			aPot->mGridY = v.c.row;
			aPot->mRenderOrder = Board::MakeRenderOrder(RenderLayer::RENDER_LAYER_PLANT, v.c.row, 0);
			aPot->mScaryPotType = v.type;
			aPot->mSeedType = static_cast<SeedType>(v.seed);
			aPot->mZombieType = static_cast<ZombieType>(v.zombie);
			aPot->mSunCount = v.sun;
		}
		theBoard->mChallenge->mScaryPotterPots = theBoard->mChallenge->ScaryPotterCountPots();
		DevLog(std::format("modModes probe vases={}", theBoard->mChallenge->mScaryPotterPots));
	} else if (theMode == u::kIAmLuck) {
		// Board::InitLevel chain 0xc004a0: packet 0 the Imp card (the bank has one slot here, a port reading of hook 142)
		if (aBank->mNumPackets > 0)
			aBank->mSeedPackets[0].SetPacketType(SeedType::SEED_ZOMBIE_IMP, SeedType::SEED_NONE);
		for (int i = 1; i < aBank->mNumPackets; i++)
			aBank->mSeedPackets[i].mPacketType = SeedType::SEED_NONE;
	}
}

void ModModeVisualWhite(Board* theBoard) {
	if (Mode(theBoard->mApp) != u::kVisualDisorder)
		return;
	const bool aInPlay = theBoard->mApp->mGameScene == GameScenes::SCENE_PLAYING && !theBoard->mPaused &&
		theBoard->mApp->GetDialogCount() == 0;
	const Sexy::Color aWhite = aInPlay ? gDisorderWhite : Sexy::Color(255, 255, 255, 255);
	if (Sexy::Color::White != aWhite) {
		Sexy::Color::White = aWhite;
		LogFew("visualDisorderWhite", std::format("inPlay={} scene={} paused={} alpha={}", aInPlay ? 1 : 0,
			static_cast<int>(theBoard->mApp->mGameScene), theBoard->mPaused ? 1 : 0, aWhite.mAlpha));
	}
}

void ModModeBoardGone() {
	gDisorderWhite = Sexy::Color(255, 255, 255, 255);
	// hook 765 (0x798d31) puts the white back when the menu sees the mouse; the port does it when any board goes away
	if (Sexy::Color::White != Sexy::Color(255, 255, 255, 255))
		Sexy::Color::White = Sexy::Color(255, 255, 255, 255);
}

// ---- the Glove Wars' item mode (0x6b 1, 0x87 2, 0x90 3, 0x8d 4): the shovel is the Zen glove ----

int ModModeItemMode(LawnApp* theApp) { return theApp ? u::ItemModeOf(Mode(theApp)) : 0; }

bool ModModeGlovePickUp(Board* theBoard) {
	if (ModModeItemMode(theBoard->mApp) == 0)
		return false;
	theBoard->mCursorObject->mCursorType = CursorType::CURSOR_TYPE_GLOVE; // hook 76 (0x7a48e0)
	theBoard->mApp->PlayFoley(FoleyType::FOLEY_DROP);                     // hook 77 (0x7a49d1)
	return true;
}

bool ModModeGloveMouseDown(Board* theBoard, int x, int y, CursorType theCursorType) {
	if (ModModeItemMode(theBoard->mApp) == 0)
		return false;
	// Hook 73 (0x7a4841): every tool click goes to ZenGarden::MouseDownWithTool, as in the Zen Garden
	theBoard->mApp->mZenGarden->mBoard = theBoard;
	theBoard->mApp->mZenGarden->MouseDownWithTool(x, y, theCursorType);
	LogFew("glove", std::format("tool click at {},{} cursor={}", x, y, static_cast<int>(theBoard->mCursorObject->mCursorType)));
	return true;
}

void ModModeGloveMoved(Plant* thePlant) {
	if (ModModeItemMode(thePlant->mApp) == 0)
		return;
	thePlant->mRenderOrder = u::GloveRenderOrder(thePlant->mRow, thePlant->mX, thePlant->mSeedType); // hook 479 (0x7a4af0)
	LogFew("gloveMove", std::format("seed={} to col={} row={}", static_cast<int>(thePlant->mSeedType), thePlant->mPlantCol, thePlant->mRow));
}

Sexy::Image* ModModeShovelImage(Board* theBoard, Sexy::Image* theVanilla) {
	return ModModeItemMode(theBoard->mApp) ? Sexy::IMAGE_ZEN_GARDENGLOVE : theVanilla; // hook 132 (0x7a4930)
}

bool ModModeHoldsShovel(Board* theBoard) {
	const CursorType t = theBoard->mCursorObject->mCursorType;
	return t == CursorType::CURSOR_TYPE_SHOVEL || (t == CursorType::CURSOR_TYPE_GLOVE && ModModeItemMode(theBoard->mApp)); // hook 133
}

bool ModModeGloveRefuses(Board* theBoard, int theGridX) {
	return theBoard->mCursorObject->mCursorType == CursorType::CURSOR_TYPE_PLANT_FROM_GLOVE &&
		u::GloveRefusesColumn(ModModeItemMode(theBoard->mApp), theGridX); // hook 173 (0xc03210)
}

Plant* ModModeGlovePlant(Board* theBoard) {
	const CursorObject* c = theBoard->mCursorObject.get();
	if (c->mCursorType != CursorType::CURSOR_TYPE_PLANT_FROM_GLOVE)
		return nullptr;
	Plant* aPlant = theBoard->mPlants.DataArrayTryToGet(static_cast<unsigned int>(c->mGlovePlantID));
	return aPlant && !aPlant->mDead ? aPlant : nullptr;
}

bool ModModeGloveLawnSeed(const Plant* thePlant, SeedType& theSeed, SeedType& theImitater) {
	if (thePlant->mPottedPlantIndex >= 0)
		return false;
	theSeed = ModSeedOf(thePlant);
	theImitater = thePlant->mImitaterType;
	if (theImitater == SeedType::SEED_IMITATER) { // a morphed Imitater: DrawSeedType's imitater look takes (IMITATER, copy)
		theImitater = theSeed;
		theSeed = SeedType::SEED_IMITATER;
	}
	return true;
}

// ---- 0x8c I Am Luck: an I, Zombie level full of Imitaters ----

bool ModModeIsIZombie(LawnApp* theApp) { return theApp && Mode(theApp) == u::kIAmLuck; } // .text 0x45380f -> 0xc002e0

int ModModeIZombieColumns(LawnApp* theApp, int theColumns) {
	return Mode(theApp) == u::kIAmLuck ? u::kIAmLuckColumns : theColumns; // hook 194 (0xc00460)
}

void ModModeIZombiePlants(Challenge* theChallenge) {
	if (Mode(theChallenge->mApp) != u::kIAmLuck)
		return;
	theChallenge->IZombiePlacePlants(SeedType::SEED_IMITATER, u::kIAmLuckImitaters, -1); // hook 195 (0xc00340)
	DevLog(std::format("modModes iAmLuck imitaters={}", static_cast<int>(theChallenge->mBoard->mPlants.mSize)));
}

ZombieType ModModeIZombieCard(Board* theBoard, ZombieType theVanilla, SeedType theSeed) {
	if (theSeed != SeedType::SEED_ZOMBIE_IMP || Mode(theBoard->mApp) != u::kIAmLuck)
		return theVanilla;
	const ZombieType aType = static_cast<ZombieType>(Sexy::Rand(u::kIAmLuckImpTypes)); // hook 191 (0xc00630)
	LogFew("iAmLuckCard", std::format("Imp card -> zombie type {}", static_cast<int>(aType)));
	return aType;
}

SeedType ModModeImitaterPick(Plant* theImitater) {
	if (theImitater->mImitaterType != SeedType::SEED_NONE || !ModModesOn())
		return theImitater->mImitaterType;
	// Hook 348 (0xc00380): an Imitater with no copy picks Rand(128) until the table allows it; in 0x8c the plant's cost is
	// taken from the sun
	int s = 0;
	do
		s = Sexy::Rand(128);
	while (!u::ImitaterTableAllows(s) || (s >= 0x50 && !IsModSeed(static_cast<SeedType>(s))));
	if (Mode(theImitater->mApp) == u::kIAmLuck && theImitater->mBoard) {
		const int aCost = Plant::GetCost(static_cast<SeedType>(s), SeedType::SEED_NONE);
		theImitater->mBoard->mSunMoney -= aCost;
		LogFew("iAmLuckMorph", std::format("Imitater col={} row={} -> 0x{:x} cost={} sun={}", theImitater->mPlantCol, theImitater->mRow, s,
			aCost, theImitater->mBoard->mSunMoney));
	}
	return static_cast<SeedType>(s);
}

} // namespace pvzu::engine
