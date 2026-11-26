// PVZU: Plan 9 L3, Unnamed's small visual details, engine side; see ModVisuals.h and Mod/Unnamed/ModVisuals.h.
#include "Mod/Engine/ModVisuals.h"

#include "Mod/Engine/LawnHoldInfo.h"
#include "Mod/Engine/ModPlants.h"
#include "Mod/Unnamed/DayPlants.h"
#include "Mod/Unnamed/ModPlantRules.h"
#include "Mod/Unnamed/ModShots.h"
#include "Mod/Unnamed/ModVisuals.h"

#include "LawnApp.h"
#include "Resources.h"
#include "Lawn/Board.h"
#include "Lawn/CursorObject.h"
#include "Lawn/Plant.h"
#include "Lawn/ToolTipWidget.h"
#include "Lawn/Zombie.h"
#include "Lawn/System/ReanimationLawn.h"
#include "PvzpLib/PvzpCommon.h"
#include "PvzpLib/PvzpStringFile.h"
#include "PvzpLib/Reanimator.h"
#include "graphics/Graphics.h"
#include "misc/SexyMatrix.h"
#include "widget/WidgetManager.h"

#include <cstdio>
#include <cstdlib>
#include <format>
#include <map>
#include <string>

namespace pvzu::engine {

namespace u = pvzu::unnamed;

namespace {
constexpr int kSeedSniper = 0x78, kSeedElement = 0x7a;
constexpr int kSniperAiming = 0x22; // the exe's aiming state; the port's Sniper keeps state 0 and aims by its target id

std::string Text(const char* theKey, const char* theFallback) {
	const std::string aKey = std::format("[{}]", theKey);
	return PvzpStringListExists(aKey) ? std::string(PvzpStringTranslate(aKey)) : std::string(theFallback);
}

bool AliveOnBoard(const Plant* p) { return !p->mDead && !p->mSquished && !p->mIsAsleep && p->mIsOnBoard; } // +0x141 == 0x01000000

// The Sniper's target when it aims (hook 13 / 121 conditions), else nullptr.
Zombie* SniperTarget(Plant* p) {
	if (p->mPvzu.modSeed != kSeedSniper || p->mBoard == nullptr)
		return nullptr;
	Zombie* aTarget = p->mBoard->ZombieTryToGet(p->mTargetZombieID);
	const int aId = aTarget ? static_cast<int>(p->mTargetZombieID) : 0;
	if (!u::SniperAims(p->mPvzu.modSeed, aTarget ? kSniperAiming : 0, aId, AliveOnBoard(p)))
		return nullptr;
	return aTarget;
}

Sexy::Image* TargetImage() { // IMAGE_TARGET (0xc10ec4): the pack's images/target.png through the asset overlay
	static SharedImageRef sImage;
	static bool sTried = false;
	if (!sTried) {
		sTried = true;
		sImage = gSexyAppBase->GetSharedImage("images/target");
		if (static_cast<Sexy::Image*>(sImage) == nullptr)
			std::fprintf(stderr, "PVZU: images/target not found, the Sniper mark is not drawn\n");
	}
	return static_cast<Sexy::Image*>(sImage);
}

// -pvzu-dev / log: one line the first few times something new is drawn
void LogFew(const char* theTag, const std::string& theLine) {
	static std::map<std::string, int> sCount;
	if (++sCount[theTag] <= 3)
		u::LogPlantAbility(-1, std::format("MODVISUALS {} {}", theTag, theLine));
}

void DrawElement(Plant* p, Sexy::Graphics* g) {
	if (!u::ElementIconsDrawn(p->mPvzu.modSeed, 1, p->mSquished)) // 0x7a8e29 / 0x7a8e33 (the digit has no count gate)
		return;
	if (const int d = u::ElementShroomCountdownDigit(p->mStateCountdown);
		d > 0 && IMAGE_SELECTORSCREEN_LEVELNUMBERS && d < IMAGE_SELECTORSCREEN_LEVELNUMBERS->mNumCols) // 0x7b1536
		PvzpDrawImageCelF(g, IMAGE_SELECTORSCREEN_LEVELNUMBERS, static_cast<float>(u::ElementDigitX(p->mWidth)),
			static_cast<float>(u::ElementDigitY()), d, 0);
	if (!u::ElementIconsDrawn(p->mPvzu.modSeed, p->mPvzu.elementCount, p->mSquished)) // 0x7b15a1
		return;
	for (const u::ElementIcon& e : u::ElementShroomIcons(p->mPvzu.elementBits, p->mWidth, p->mHeight)) { // 0x7b158f
		if (e.ice)
			g->DrawImage(IMAGE_PROJECTILESNOWPEA, e.x, e.y);
		else // 0x800400 = the fire pea's anim_idle frame (0x7b3000 -> DrawReanimatorFrame, reanim 0x5c)
			p->mApp->mReanimatorCache->DrawReanimatorFrame(g, static_cast<float>(e.x), static_cast<float>(e.y),
				ReanimationType::REANIM_FIRE_PEA, "anim_idle", DrawVariation::VARIATION_NORMAL);
	}
	LogFew("element", std::format("icons bits {} count {} digit {}", p->mPvzu.elementBits, p->mPvzu.elementCount,
		u::ElementShroomCountdownDigit(p->mStateCountdown)));
}

void DrawPeashooterLayers(Plant* p, Sexy::Graphics* g) {
	if (p->mSeedType != SeedType::SEED_PEASHOOTER || p->mPvzu.modSeed != 0 || !u::PeashooterLayersDrawn(0)) // 0xc10faa
		return;
	Sexy::Graphics aG(*g);
	aG.SetFont(Sexy::FONT_DWARVENTODCRAFT18); // 0xc05ea8 [0x6a7314]
	aG.SetColor(Sexy::Color(255, 0, 255, 255)); // 0xc05eb1..0xc05ec6
	aG.DrawString(std::to_string(u::PeashooterLayersGuard(p->mPvzu.counter)), u::kLayersX, u::kLayersY); // 0xc05ecd
}

std::string ZombieTitle(Zombie* z) { return std::format("[{}]", GetZombieDefinition(z->mZombieType).mZombieName); }

std::string ZombieLabel(Zombie* z) {
	u::ZombieInfo i;
	i.body = z->mBodyHealth;
	i.bodyMax = z->mBodyMaxHealth;
	i.helmType = static_cast<int>(z->mHelmType);
	i.helm = z->mHelmHealth;
	i.helmMax = z->mHelmMaxHealth;
	i.shieldType = static_cast<int>(z->mShieldType);
	i.shield = z->mShieldHealth;
	i.shieldMax = z->mShieldMaxHealth;
	i.flying = z->mFlyingHealth;
	i.flyingMax = z->mFlyingMaxHealth;
	i.butterCs = z->mButteredCounter;
	i.chillCs = z->mChilledCounter;
	i.iceCs = z->mIceTrapCounter;
	i.weakenCs = z->mPvzu.weakenCs; // 0x95a1b0 word +0x1a
	u::InfoLabels l;
	l.helm = Text("PVZU_INFO_HELM", "Helm:") + " ";
	l.shield = Text("PVZU_INFO_SHIELD", "Shield:") + " ";
	l.balloon = Text("PVZU_INFO_BALLOON", "Balloon:") + " ";
	l.hp = Text("PVZU_INFO_HP", "HP:") + " ";
	l.butter = Text("PVZU_INFO_BUTTER", "Butter:") + " ";
	l.chill = Text("PVZU_INFO_CHILL", "Chill:") + " ";
	l.frozen = Text("PVZU_INFO_FROZEN", "Frozen:") + " ";
	l.weakened = Text("PVZU_INFO_WEAKENED", "Weakened:") + " ";
	l.cs = Text("PVZU_INFO_CS", "cs");
	return u::ZombieInfoLabel(i, l);
}

std::string PlantTitle(Plant* p) {
	if (p->mPvzu.modSeed != 0)
		if (const auto aName = ModPlantNameString(static_cast<SeedType>(p->mPvzu.modSeed)))
			return *aName; // 0xc06065: the mod table name, no brackets
	return std::format("[{}]", GetPlantDefinition(p->mSeedType).mPlantName); // 0xc05f4b
}

// Phone hold (port addition): ticks the left button (a finger) has been down within 12 px of where it went down.
int HeldTicks(LawnApp* theApp) {
	static int sStart = -1, sX = 0, sY = 0;
	WidgetManager* aWm = theApp->mWidgetManager.get();
	const int aNow = static_cast<int>(theApp->mUpdateCount);
	if (aWm->mDownButtons != 1) {
		sStart = -1;
		return 0;
	}
	if (sStart < 0 || std::abs(aWm->mLastMouseX - sX) > 12 || std::abs(aWm->mLastMouseY - sY) > 12) {
		sStart = aNow;
		sX = aWm->mLastMouseX;
		sY = aWm->mLastMouseY;
	}
	return aNow - sStart;
}
} // namespace

void ModPlantDrawOver(Plant* thePlant, Sexy::Graphics* g) {
	if (thePlant->mPvzu.modSeed == kSeedElement)
		DrawElement(thePlant, g);
	else if (thePlant->mPvzu.modSeed == 0)
		DrawPeashooterLayers(thePlant, g);
}

void ModZombieDrawOver(Zombie* theZombie, Sexy::Graphics* g) {
	if (!u::SniperMarkDrawn() || theZombie->mBoard == nullptr || !u::RepeaterPoisonRecordFor(theZombie->mFromWave)) // 0xc10fd8
		return;
	for (Plant* aPlant : theZombie->mBoard->mPlants) {
		if (SniperTarget(aPlant) != theZombie)
			continue;
		if (Sexy::Image* aMark = TargetImage())
			PvzpDrawImageScaledF(g, aMark, static_cast<float>(u::SniperMarkOffsetX()), static_cast<float>(u::SniperMarkOffsetY()),
				1.0f, 1.0f); // 0xc10ff3..0xc1100c
		return;
	}
}

bool ModReanimOverlay(Reanimation* theReanim, Sexy::SexyTransform2D& theMatrix) {
	if (theReanim->mReanimationType != ReanimationType::REANIM_GATLINGPEA || gLawnApp == nullptr || gLawnApp->mBoard == nullptr ||
		!u::ModPlantsOn())
		return false;
	const ReanimationID aId = gLawnApp->ReanimationGetID(theReanim);
	for (Plant* aPlant : gLawnApp->mBoard->mPlants) {
		if (aPlant->mHeadReanimID != aId) // 0x7a7e9d cmp [plant+0x98], id
			continue;
		Zombie* aTarget = SniperTarget(aPlant);
		if (aTarget == nullptr)
			return false;
		const u::AimMatrix m = u::SniperHeadMatrix(u::SniperHeadAngle(aPlant->mX, aPlant->mY, aTarget->mX, aTarget->mY));
		theMatrix = theReanim->mOverlayMatrix; // m20..m22 kept (the exe writes only the six 2D entries)
		theMatrix.m00 = m.m00;
		theMatrix.m01 = m.m01;
		theMatrix.m02 = m.m02;
		theMatrix.m10 = m.m10;
		theMatrix.m11 = m.m11;
		theMatrix.m12 = m.m12;
		return true;
	}
	return false;
}

bool ModInfoToolTip(Board* theBoard, const HitResult* theHitResult, int theMouseX, int theMouseY) {
	if (!u::ModVisualsOn())
		return false;
	if (theHitResult->mObjectType != GameObjectType::OBJECT_TYPE_NONE && theHitResult->mObjectType != GameObjectType::OBJECT_TYPE_PLANT)
		return false; // 0x955285
	if (LawnHoldTouch(theBoard))
		return false; // a finger gets the big panel above it (Mod/Engine/LawnHoldInfo.h), never this small tooltip
	LawnApp* aApp = theBoard->mApp;
	const int aHeld = HeldTicks(aApp);
	const bool aCursorFree = theBoard->mCursorObject->mCursorType == CursorType::CURSOR_TYPE_NORMAL;
	if (!u::InfoToolTipShown(static_cast<int>(aApp->mGameScene), static_cast<int>(aApp->mGameMode), aApp->mWidgetManager->mDownButtons,
			aCursorFree ? aHeld : 0))
		return false;
	ToolTipWidget* aTip = theBoard->mToolTip.get();
	Rect aRect;
	if (Zombie* aZombie = theBoard->ZombieHitTest(theMouseX, theMouseY)) { // 0x9552e7
		aTip->SetTitle(ZombieTitle(aZombie));
		aTip->SetLabel(ZombieLabel(aZombie));
		aRect = aZombie->GetZombieRect(); // 0x95586d
		LogFew("info", std::format("zombie type {} label \"{}\"", static_cast<int>(aZombie->mZombieType), aTip->mLabel));
	} else if (theHitResult->mObjectType == GameObjectType::OBJECT_TYPE_PLANT && theHitResult->mObject) { // 0xc05f31
		Plant* aPlant = static_cast<Plant*>(theHitResult->mObject);
		aTip->SetTitle(PlantTitle(aPlant));
		aTip->SetLabel(u::PlantInfoLabel(aPlant->mPlantHealth, aPlant->mPlantMaxHealth, Text("PVZU_INFO_HEALTH", "Health:") + " "));
		aRect = aPlant->GetPlantRect(); // 0xc05fb2
		LogFew("info", std::format("plant seed {} label \"{}\"", aPlant->mPvzu.modSeed ? aPlant->mPvzu.modSeed : static_cast<int>(aPlant->mSeedType),
			aTip->mLabel));
	} else {
		return false;
	}
	aTip->SetWarningText("");
	aTip->mX = aRect.mX + aRect.mWidth / 2;
	aTip->mY = aRect.mY + aRect.mHeight;
	aTip->mVisible = true;
	aTip->mCenter = true;
	return true;
}

} // namespace pvzu::engine
