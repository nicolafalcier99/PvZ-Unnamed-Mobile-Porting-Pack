#include "Mod/Engine/Level55Shooter.h"

#include "Mod/Engine/DevLevels.h"
#include "Mod/Engine/SpecialLevelHooks.h"

#include "LawnApp.h"
#include "Lawn/Board.h"
#include "Lawn/MessageWidget.h"
#include "Lawn/Plant.h"
#include "Lawn/SeedPacket.h"
#include "Lawn/Zombie.h"
#include "PvzpLib/PvzpStringFile.h"

#include <format>
#include <string>

namespace pvzu::engine {

namespace u = pvzu::unnamed;

namespace {
constexpr int kRows = 5; // the exe moves the plant inside rows 0..4 (0x79b0d6 `row == 0`, 0x79b0f9 `row != 4`)

std::string Text(const char* theKey, const char* theFallback) {
	const std::string aKey = std::format("[{}]", theKey);
	return PvzpStringListExists(aKey) ? std::string(PvzpStringTranslate(aKey)) : std::string(theFallback);
}

void MoveTo(Board* theBoard, Plant* thePlant, int theRow, const char* theSource) {
	if (theRow == thePlant->mRow)
		return;
	// 0x79b0e0 / 0x79b106: Plant+0x1c (mRow) +-1 and +0xc (mY) +-100; the render order follows the row here as well
	thePlant->mRow = theRow;
	thePlant->mY = theBoard->GridToPixelY(thePlant->mPlantCol, theRow);
	thePlant->mRenderOrder = thePlant->CalcRenderOrder();
	DevLog(std::format("special shooter t={} move row={} by={}", theBoard->mMainCounter, theRow, theSource));
}
} // namespace

Plant* ShooterPlant(Board* theBoard) {
	for (Plant* aPlant : theBoard->mPlants)
		if (!aPlant->mDead && u::ShooterLaunchRate(aPlant->mSeedType) >= 0)
			return aPlant;
	return nullptr;
}

void ShooterStart(Board* theBoard) {
	// SeedChooserScreen::OnStartButton (hook 462, 0x79842f): runs here, once play begins, so a dev run without the chooser
	// gets it too; the plant shows a few seconds later than in the exe (at the end of the cut-scene, not its start)
	const u::ShooterSetup s = u::ShooterStart();
	if (s.seed < 0)
		return;
	if (s.startSun >= 0)
		theBoard->mSunMoney = s.startSun;
	theBoard->mSeedBank->mNumPackets = 0;
	Plant* aPlant = theBoard->AddPlant(s.col, s.row, static_cast<SeedType>(s.seed), SeedType::SEED_NONE);
	if (aPlant && s.launchRate >= 0)
		aPlant->mLaunchRate = s.launchRate;
	// The exe also writes 100 to Plant+0xc8: that is mMagnetItems[0].mPosX in the 1051 Plant, not its health (+0x40).

	// Board::StartLevel (hook 160, 0x79a000): first wave in 250 cs, message 0xa20100 in MESSAGE_STYLE_BIG_MIDDLE for 250 cs
	if (const int aCountdown = u::ShooterFirstWaveCountdown(); aCountdown >= 0)
		theBoard->mZombieCountDown = aCountdown;
	if (const int aTime = u::ShooterMessageTime(); aTime > 0) {
		theBoard->mAdvice->SetLabel(Text("PVZU_SHOOTER_MOVE", "Press Up or Down to move the plant\n(phone: tap a row or swipe)"),
			MessageStyle::MESSAGE_STYLE_BIG_MIDDLE);
		theBoard->mAdvice->mDuration = aTime;
	}
	DevLog(std::format("special shooter start plant={} row={} col={} rate={} sun={} bank={} countdown={}", s.seed, s.row,
		s.col, aPlant ? aPlant->mLaunchRate : -1, theBoard->mSunMoney, theBoard->mSeedBank->mNumPackets,
		theBoard->mZombieCountDown));
}

bool ShooterKey(Board* theBoard, Sexy::KeyCode theKey) {
	const u::Steer aDir = u::SteerFromArrow(theKey);
	if (aDir == u::Steer::None || theBoard->mApp->mGameScene != GameScenes::SCENE_PLAYING)
		return false;
	Plant* aPlant = ShooterPlant(theBoard);
	if (aPlant == nullptr)
		return false;
	MoveTo(theBoard, aPlant, u::ShooterStepRow(aPlant->mRow, aDir, kRows), aDir == u::Steer::Up ? "up" : "down");
	return true;
}

void ShooterTouch(Board* theBoard, const u::Touch& theTouch, int theTapX, int theTapY) {
	Plant* aPlant = ShooterPlant(theBoard);
	if (aPlant == nullptr)
		return;
	const int aTapRow = theTouch.swipe ? -1 : theBoard->PixelToGridY(theTapX, theTapY);
	MoveTo(theBoard, aPlant, u::ShooterTouchRow(aPlant->mRow, theTouch, aTapRow, kRows), theTouch.swipe ? "swipe" : "tap");
}

void ShooterZombieSpawned(Zombie* theZombie) {
	if (!u::ShooterHalvesHp(theZombie->mZombieType))
		return;
	u::ZombieHp aHp{theZombie->mBodyHealth, theZombie->mBodyMaxHealth, theZombie->mHelmHealth, theZombie->mHelmMaxHealth,
		theZombie->mShieldHealth, theZombie->mShieldMaxHealth};
	u::ShooterHalveHp(aHp);
	theZombie->mBodyHealth = aHp.body;
	theZombie->mHelmHealth = aHp.helm;
	theZombie->mHelmMaxHealth = aHp.helmMax;
	theZombie->mShieldHealth = aHp.shield;
	theZombie->mShieldMaxHealth = aHp.shieldMax;
	DevLog(std::format("special shooter t={} halfhp type={} body={}/{} helm={} shield={}", theZombie->mBoard->mMainCounter,
		static_cast<int>(theZombie->mZombieType), theZombie->mBodyHealth, theZombie->mBodyMaxHealth,
		theZombie->mHelmHealth, theZombie->mShieldHealth));
}

void ShooterHeadDropped(Zombie* theZombie) {
	Board* aBoard = theZombie->mBoard;
	Plant* aPlant = ShooterPlant(aBoard);
	if (aPlant == nullptr) // 0x7984cf: no plant pointer, no sun
		return;
	const int aSun = u::ShooterKillSun(GetZombieDefinition(theZombie->mZombieType).mZombieValue);
	aBoard->mSunMoney += aSun; // straight into Board+0x5560, past the 9990 cap of AddSunMoney, as the exe does
	DevLog(std::format("special shooter t={} kill type={} sun+={} sun={}", aBoard->mMainCounter,
		static_cast<int>(theZombie->mZombieType), aSun, aBoard->mSunMoney));
	const int aNext = u::ShooterUpgrade(aPlant->mSeedType, aBoard->mSunMoney);
	if (aNext < 0)
		return;
	const int aOld = aPlant->mSeedType, aRow = aPlant->mRow, aCol = aPlant->mPlantCol;
	aPlant->Die(); // the exe sets Plant+0x141 (mDead) and plants the next one in the same cell (0x798606)
	Plant* aNew = aBoard->AddPlant(aCol, aRow, static_cast<SeedType>(aNext), SeedType::SEED_NONE);
	if (aNew && u::ShooterLaunchRate(aNext) >= 0)
		aNew->mLaunchRate = u::ShooterLaunchRate(aNext);
	DevLog(std::format("special shooter t={} upgrade {}->{} row={} rate={} sun={}", aBoard->mMainCounter, aOld, aNext, aRow,
		aNew ? aNew->mLaunchRate : -1, aBoard->mSunMoney));
}

} // namespace pvzu::engine
