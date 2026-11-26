// PVZU: see LawnHoldInfo.h.
#include "Mod/Engine/LawnHoldInfo.h"

#include "Mod/Engine/ModPlants.h"
#include "Mod/Engine/Zoom.h"
#include "Mod/HoldInfo.h"
#include "Mod/LawnHoldInfo.h"
#include "Mod/Layout.h"
#include "Mod/Touch.h"
#include "Mod/Unnamed/ModVisuals.h"

#include "LawnApp.h"
#include "Resources.h"
#include "Lawn/Board.h"
#include "Lawn/CursorObject.h"
#include "Lawn/Plant.h"
#include "Lawn/Zombie.h"
#include "PvzpLib/PvzpCommon.h"
#include "PvzpLib/PvzpStringFile.h"
#include "graphics/Font.h"
#include "graphics/Graphics.h"
#include "misc/SexyMatrix.h"
#include "widget/WidgetManager.h"

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdio>
#include <format>
#include <string>
#include <vector>

namespace pvzu::engine {

namespace u = pvzu::unnamed;

namespace {

struct LawnHold {
	Board* board = nullptr;
	HoldPress press;
	int screenX = 0, screenY = 0;          // the press point on the screen: the panel's anchor
	int boardX = 0, boardY = 0;            // the press in board coordinates (the plant tap's replay)
	int clickCount = 1;
	uint32_t plantId = 0;                  // the plant the press hit (its tap waits for the lift), 0 = none
	bool replaying = false;
	bool logged = false;
} gLawn;

constexpr int kPlayingScene = static_cast<int>(GameScenes::SCENE_PLAYING);

int Frame(Board* theBoard) { return static_cast<int>(theBoard->mApp->mUpdateCount); }

bool VanillaInput(LawnApp* theApp) { return theApp->mPlayingDemoBuffer || theApp->mRecordingDemoBuffer; }

// The exe's own gates for its info tooltip (feature, scene, mode, a hold rule), with nothing held.
bool HoldAllowed(Board* theBoard) {
	LawnApp* aApp = theBoard->mApp;
	return aApp->mBoard == theBoard && !VanillaInput(aApp) && theBoard->mCursorObject->mCursorType == CursorType::CURSOR_TYPE_NORMAL &&
		aApp->GetDialogCount() == 0 && theBoard->mTimeStopCounter == 0 &&
		u::InfoToolTipShown(static_cast<int>(aApp->mGameScene), static_cast<int>(aApp->mGameMode), 1, INT_MAX);
}

std::string Text(const char* theKey, const char* theFallback) {
	const std::string aKey = std::format("[{}]", theKey);
	return PvzpStringListExists(aKey) ? std::string(PvzpStringTranslate(aKey)) : std::string(theFallback);
}

const LawnInfoText& Labels() {
	static const LawnInfoText sText = [] {
		LawnInfoText t;
		t.health = Text("PVZU_HOLD_HEALTH", "Health");
		t.armour = Text("PVZU_HOLD_ARMOUR", "Armour");
		t.shield = Text("PVZU_HOLD_SHIELD", "Shield");
		t.balloon = Text("PVZU_HOLD_BALLOON", "Balloon");
		t.total = Text("PVZU_HOLD_TOTAL", "Total");
		t.pumpkin = Text("PVZU_HOLD_PUMPKIN", "Pumpkin");
		t.buttered = Text("PVZU_HOLD_BUTTERED", "Buttered");
		t.slowed = Text("PVZU_HOLD_SLOWED", "Slowed");
		t.frozen = Text("PVZU_HOLD_FROZEN", "Frozen");
		t.weakened = Text("PVZU_HOLD_WEAKENED", "Weakened");
		t.seconds = Text("PVZU_HOLD_SECONDS", "s");
		return t;
	}();
	return sText;
}

// What a press at board point (x, y) holds: a zombie first (as the exe's tooltip, 0x9552e7), else the plant hit.
struct Target {
	int key = kHoldNoTarget;
	uint32_t plantId = 0; // the plant the hit test found, also under a zombie
};
Target TargetAt(Board* theBoard, int x, int y) {
	Target t;
	HitResult aHit;
	theBoard->MouseHitTest(x, y, &aHit);
	if (aHit.mObjectType == GameObjectType::OBJECT_TYPE_PLANT && aHit.mObject)
		t.plantId = theBoard->mPlants.DataArrayGetID(static_cast<Plant*>(aHit.mObject));
	if (aHit.mObjectType != GameObjectType::OBJECT_TYPE_NONE && aHit.mObjectType != GameObjectType::OBJECT_TYPE_PLANT)
		return t; // a sun, a coin, a packet, a button: not ours
	if (Zombie* aZombie = theBoard->ZombieHitTest(x, y))
		t.key = LawnHoldZombieKey(theBoard->mZombies.DataArrayGetID(aZombie));
	else if (t.plantId != 0)
		t.key = LawnHoldPlantKey(t.plantId);
	return t;
}

// The finger's point now, in board coordinates.
void FingerOnBoard(Board* theBoard, int& x, int& y) {
	x = theBoard->mApp->mWidgetManager->mLastMouseX - theBoard->mX;
	y = theBoard->mApp->mWidgetManager->mLastMouseY - theBoard->mY;
	LocalToBoard(theBoard, x, y);
}

// The held object, while it is still there.
Plant* HeldPlant(Board* theBoard) {
	if (LawnHoldKeyIsZombie(gLawn.press.Target()))
		return nullptr;
	Plant* aPlant = theBoard->mPlants.DataArrayTryToGet(gLawn.plantId);
	return aPlant && !aPlant->mDead && LawnHoldPlantKey(gLawn.plantId) == gLawn.press.Target() ? aPlant : nullptr;
}
Zombie* HeldZombie(Board* theBoard) {
	if (!LawnHoldKeyIsZombie(gLawn.press.Target()))
		return nullptr;
	for (Zombie* aZombie : theBoard->mZombies) // the key keeps 30 of the id's 32 bits: find it among the living
		if (LawnHoldZombieKey(theBoard->mZombies.DataArrayGetID(aZombie)) == gLawn.press.Target())
			return aZombie->IsDeadOrDying() ? nullptr : aZombie;
	return nullptr;
}

std::string ZombieName(Zombie* z) { return std::string(PvzpStringTranslate(std::format("[{}]", GetZombieDefinition(z->mZombieType).mZombieName))); }

std::string PlantName(Plant* p) {
	if (p->mPvzu.modSeed != 0)
		if (const auto aName = ModPlantNameString(static_cast<SeedType>(p->mPvzu.modSeed)))
			return std::string(PvzpStringTranslate(*aName));
	return Plant::GetNameString(p->mSeedType, p->mImitaterType);
}

std::vector<LawnInfoRow> ZombieRows(Zombie* z) {
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
	i.weakenCs = z->mPvzu.weakenCs;
	return ZombieInfoRows(i, Labels());
}

std::vector<LawnInfoRow> PlantRows(Board* theBoard, Plant* p) {
	int aPumpkinHp = 0, aPumpkinMax = 0;
	if (p->mSeedType != SeedType::SEED_PUMPKINSHELL)
		if (Plant* aPumpkin = theBoard->GetTopPlantAt(p->mPlantCol, p->mRow, PlantPriority::TOPPLANT_ONLY_PUMPKIN); aPumpkin && aPumpkin != p) {
			aPumpkinHp = aPumpkin->mPlantHealth;
			aPumpkinMax = aPumpkin->mPlantMaxHealth;
		}
	return PlantInfoRows(p->mPlantHealth, p->mPlantMaxHealth, aPumpkinHp, aPumpkinMax, Labels());
}

// A filled rectangle with rounded corners, one scanline per corner row (no overlap, so alpha stays even).
void FillRounded(Sexy::Graphics* g, const InfoRect& r, int theRadius, const Sexy::Color& theColor) {
	g->SetColor(theColor);
	const int aR = std::min({theRadius, r.w / 2, r.h / 2});
	for (int i = 0; i < aR; i++) {
		const float aDy = static_cast<float>(aR) - i - 0.5f;
		const int aInset = aR - static_cast<int>(std::lround(std::sqrt(std::max(0.0f, static_cast<float>(aR * aR) - aDy * aDy))));
		g->FillRect(r.x + aInset, r.y + i, r.w - 2 * aInset, 1);
		g->FillRect(r.x + aInset, r.y + r.h - 1 - i, r.w - 2 * aInset, 1);
	}
	g->FillRect(r.x, r.y + aR, r.w, r.h - 2 * aR);
}

void DrawScaled(Sexy::Graphics* g, Sexy::_Font* f, const std::string& s, float x, float theTop, float theScale, const Sexy::Color& c) {
	Sexy::SexyMatrix3 m;
	// matrix drawing ignores the Graphics translation: add it (as the chooser panel's DrawLines)
	PvzpScaleTransformMatrix(m, x + g->mTransX, theTop + f->GetAscent() * theScale + g->mTransY, theScale, theScale);
	PvzpDrawStringMatrix(g, f, m, s, c);
}

const char* SideName(LawnInfoSide s) { return s == LawnInfoSide::Above ? "above" : s == LawnInfoSide::Left ? "left" : "right"; }

} // namespace

bool LawnHoldTouch(Board* theBoard) {
	return HoverEndsOnLift(LastPointer()) && !VanillaInput(theBoard->mApp) && u::ModVisualsOn();
}

bool LawnHoldDown(Board* theBoard, const HitResult* theHitResult, int x, int y, int theClickCount) {
	if (gLawn.replaying || theClickCount <= 0 || !HoverEndsOnLift(LastPointer()) || !HoldAllowed(theBoard))
		return false;
	if (theHitResult->mObjectType != GameObjectType::OBJECT_TYPE_NONE && theHitResult->mObjectType != GameObjectType::OBJECT_TYPE_PLANT)
		return false;
	const Target t = TargetAt(theBoard, x, y);
	if (t.key == kHoldNoTarget)
		return false;
	gLawn = LawnHold{};
	gLawn.board = theBoard;
	gLawn.press.Down(t.key, Frame(theBoard));
	gLawn.screenX = theBoard->mApp->mWidgetManager->mLastMouseX;
	gLawn.screenY = theBoard->mApp->mWidgetManager->mLastMouseY;
	gLawn.boardX = x;
	gLawn.boardY = y;
	gLawn.clickCount = theClickCount;
	gLawn.plantId = t.plantId; // also under a zombie (the panel is the zombie's, a quick tap the plant's)
	std::fprintf(stderr, "PVZU lawn hold: down %s at %d,%d frame %d%s\n", LawnHoldKeyIsZombie(t.key) ? "zombie" : "plant", gLawn.screenX,
		gLawn.screenY, Frame(theBoard), gLawn.plantId ? " (plant tap waits for the lift)" : "");
	return gLawn.plantId != 0;
}

void LawnHoldUp(Board* theBoard, int x, int y) {
	if (gLawn.board != theBoard || !gLawn.press.Active() || gLawn.replaying)
		return;
	LocalToBoard(theBoard, x, y);
	const int aTarget = gLawn.press.Target();
	const HoldPress::Lift aLift = gLawn.press.Up(Frame(theBoard), TargetAt(theBoard, x, y).key);
	const char* aDone = "hold (nothing else)";
	if (aLift == HoldPress::Lift::Tap && gLawn.plantId != 0) {
		if (Plant* aPlant = theBoard->mPlants.DataArrayTryToGet(gLawn.plantId); aPlant && !aPlant->mDead) {
			gLawn.replaying = true;
			aPlant->MouseDown(gLawn.boardX, gLawn.boardY, gLawn.clickCount); // the plant's tap, as vanilla's press
			theBoard->UpdateCursor();
			gLawn.replaying = false;
			aDone = "tap (the plant's tap ran)";
		}
	} else if (aLift == HoldPress::Lift::Tap) {
		aDone = "tap";
	}
	std::fprintf(stderr, "PVZU lawn hold: lift %s frame %d %s; cursor %d\n", LawnHoldKeyIsZombie(aTarget) ? "zombie" : "plant", Frame(theBoard),
		aDone, static_cast<int>(theBoard->mCursorObject->mCursorType));
}

void LawnHoldUpdate(Board* theBoard) {
	if (gLawn.board != theBoard || !gLawn.press.Active())
		return;
	if ((theBoard->mApp->mWidgetManager->mDownButtons & 1) == 0 || !HoldAllowed(theBoard)) { // lifted elsewhere, a dialog, a cursor
		gLawn.press.Reset();
		return;
	}
	int x, y;
	FingerOnBoard(theBoard, x, y);
	gLawn.press.Update(Frame(theBoard), TargetAt(theBoard, x, y).key);
	if (gLawn.press.PanelShown() && !gLawn.logged) {
		gLawn.logged = true;
		std::fprintf(stderr, "PVZU lawn hold: panel %s frame %d\n", LawnHoldKeyIsZombie(gLawn.press.Target()) ? "zombie" : "plant",
			Frame(theBoard));
	}
}

void DrawLawnHoldInfo(Board* theBoard, Sexy::Graphics* g) {
	if (gLawn.board != theBoard || !gLawn.press.PanelShown() || !HoldAllowed(theBoard))
		return;
	std::string aTitle;
	std::vector<LawnInfoRow> aRows;
	if (Zombie* aZombie = HeldZombie(theBoard)) {
		aTitle = ZombieName(aZombie);
		aRows = ZombieRows(aZombie);
	} else if (Plant* aPlant = HeldPlant(theBoard)) {
		aTitle = PlantName(aPlant);
		aRows = PlantRows(theBoard, aPlant);
	} else {
		return; // it died or left the board while held: the panel goes, the lift still does nothing
	}

	Sexy::_Font* tf = Sexy::FONT_DWARVENTODCRAFT24;
	Sexy::_Font* bf = Sexy::FONT_BRIANNETOD16;
	const float s = kLawnInfoTextScale;
	int aLabelW = 0, aValueW = 0;
	for (const LawnInfoRow& r : aRows) {
		aLabelW = std::max(aLabelW, static_cast<int>(std::ceil(bf->StringWidth(r.label) * s)));
		aValueW = std::max(aValueW, static_cast<int>(std::ceil(bf->StringWidth(r.value) * s)));
	}
	const InfoRect aSize = LawnInfoBoxSize(tf->StringWidth(aTitle), aLabelW, aValueW, static_cast<int>(aRows.size()));

	const PlayLayout& pl = CurrentLayout();
	const bool aWide = pl.width > kVanillaWidth;
	const int aSafeL = aWide ? pl.safeLeft : 0, aSafeR = aWide ? pl.safeRight : 0;
	const InfoRect aScreen{aSafeL, 0, pl.width - aSafeL - aSafeR, kLogicalHeight};
	const LawnInfoPlacement p = PlaceLawnInfo(gLawn.screenX, gLawn.screenY, aSize.w, aSize.h, aScreen);
	const InfoRect& b = p.box;

	Sexy::Graphics sg(*g); // screen space
	sg.mTransX -= theBoard->mX;
	sg.mTransY -= theBoard->mY;
	FillRounded(&sg, {b.x - 2, b.y - 2, b.w + 4, b.h + 4}, kLawnInfoRadius + 2, Sexy::Color(224, 187, 98, 235)); // gold rim
	FillRounded(&sg, b, kLawnInfoRadius, Sexy::Color(24, 18, 12, 240));                                           // dark middle

	const int aTitleTop = b.y + kLawnInfoPad;
	PvzpDrawString(&sg, aTitle, b.x + b.w / 2, aTitleTop + (kLawnInfoTitleH + tf->GetAscent()) / 2 - 4, tf, Sexy::Color(224, 187, 98),
		DS_ALIGN_CENTER);
	const float aRowTextH = bf->GetLineSpacing() * s;
	const int aLabelX = b.x + kLawnInfoPad, aValueX = aLabelX + aLabelW + kLawnInfoColumnGap;
	for (size_t i = 0; i < aRows.size(); i++) {
		const float aTop = aTitleTop + kLawnInfoTitleH + static_cast<int>(i) * kLawnInfoRowH + (kLawnInfoRowH - aRowTextH) / 2;
		DrawScaled(&sg, bf, aRows[i].label, static_cast<float>(aLabelX), aTop, s, Sexy::Color(205, 195, 175));
		DrawScaled(&sg, bf, aRows[i].value, static_cast<float>(aValueX), aTop, s, Sexy::Color(255, 255, 255));
	}

	static int sLogged = INT_MIN; // Mac check: once per press
	if (sLogged != gLawn.press.Target()) {
		sLogged = gLawn.press.Target();
		std::fprintf(stderr, "PVZU lawn hold: draw \"%s\" %d rows, box %dx%d at %d,%d (%s of touch %d,%d); row glyph ascent %.1f px, line %.1f px "
			"(old tooltip ascent %d px)\n", aTitle.c_str(), static_cast<int>(aRows.size()), b.w, b.h, b.x, b.y, SideName(p.side), gLawn.screenX,
			gLawn.screenY, bf->GetAscent() * s, aRowTextH, Sexy::FONT_PICO129->GetAscent());
		for (const LawnInfoRow& r : aRows)
			std::fprintf(stderr, "PVZU lawn hold:   %s %s\n", r.label.c_str(), r.value.c_str());
	}
}

} // namespace pvzu::engine
