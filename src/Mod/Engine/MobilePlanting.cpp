// PVZU: see MobilePlanting.h.
#include "Mod/Engine/MobilePlanting.h"

#include "Mod/Engine/WideScreen.h"
#include "Mod/Engine/Zoom.h"
#include "Mod/TouchPlanting.h"

#include "ConstEnums.h"
#include "GameConstants.h"
#include "LawnApp.h"
#include "Lawn/Board.h"
#include "Lawn/CursorObject.h"
#include "Lawn/MessageWidget.h"
#include "Lawn/Plant.h"
#include "Lawn/SeedPacket.h"
#include "graphics/Graphics.h"
#include "widget/WidgetManager.h"

namespace pvzu::engine {

namespace {

struct Touch {
	const Board* board = nullptr;
	bool fingerDown = false;
	bool releasing = false;      // Board::MouseUp is replaying vanilla's click: let it through
	int heldAtDown = kNothingHeld;
	bool switched = false;       // this press put one seed back and picked another up
	TouchPress press;
} gTouch;

int Held(Board* theBoard) {
	const CursorObject* c = theBoard->mCursorObject.get();
	if (c->mCursorType == CursorType::CURSOR_TYPE_PLANT_FROM_BANK && c->mSeedBankIndex >= 0)
		return c->mSeedBankIndex;
	if (c->mCursorType == CursorType::CURSOR_TYPE_SHOVEL)
		return kShovelHeld;
	return kNothingHeld;
}

LawnGrid GridOf(Board* theBoard) {
	if (theBoard->StageHasRoof())
		return LawnGrid::Roof;
	if (theBoard->StageHasPool())
		return LawnGrid::Pool;
	return LawnGrid::Day;
}

// Tile under board point (x, y) for what is held, or false when it is off the lawn.
bool LawnTileAt(Board* theBoard, int theHeld, int x, int y, int& theCol, int& theRow) {
	if (theHeld == kShovelHeld) {
		theCol = theBoard->PixelToGridX(x, y);
		theRow = theBoard->PixelToGridY(x, y);
	} else {
		const SeedType aSeed = theBoard->GetSeedTypeInCursor();
		theCol = theBoard->PlantingPixelToGridX(x, y, aSeed);
		theRow = theBoard->PlantingPixelToGridY(x, y, aSeed);
	}
	return theCol >= 0 && theCol < MAX_GRID_SIZE_X && theRow >= 0 && theRow < LawnRows(GridOf(theBoard));
}

bool ShovelAt(Board* theBoard, int x, int y) {
	return theBoard->mShowShovel && theBoard->GetShovelButtonRect().Contains(x, y);
}

// The panel packet under (x, y), or -1 (the panel's finger-sized hit areas).
int PacketAt(Board* theBoard, int x, int y) {
	HitResult aHit;
	if (!theBoard->mSeedBank->MouseHitTest(x, y, &aHit) || aHit.mObjectType != GameObjectType::OBJECT_TYPE_SEEDPACKET)
		return -1;
	return static_cast<SeedPacket*>(aHit.mObject)->mIndex;
}

TouchHit Classify(Board* theBoard, int theHeld, int x, int y) {
	TouchHit h;
	if (const int i = PacketAt(theBoard, x, y); i >= 0) {
		h.target = TouchTarget::Packet;
		h.packet = i;
		h.ready = theBoard->mSeedBank->mSeedPackets[i].CanPickUp();
		return h;
	}
	if (ShovelAt(theBoard, x, y)) {
		h.target = TouchTarget::Shovel;
		return h;
	}
	int aCol = 0, aRow = 0;
	LocalToBoard(theBoard, x, y); // the lawn may be zoomed (Mod/Engine/Zoom.h)
	h.target = LawnTileAt(theBoard, theHeld, x, y, aCol, aRow) ? TouchTarget::Lawn : TouchTarget::Other;
	return h;
}

// Put the held seed (or shovel) back. Keeps any planting advice vanilla has just shown (ClearCursor would
// clear it, which would make a bad drop on a lily-pad-only tile look like nothing happened).
void PutBack(Board* theBoard) {
	const int aDuration = theBoard->mAdvice->mDuration;
	theBoard->mAdvice->mDuration = 0;
	if (Held(theBoard) == kShovelHeld)
		theBoard->ClearCursor();
	else
		theBoard->RefreshSeedPacketFromCursor();
	theBoard->mAdvice->mDuration = aDuration;
	theBoard->mApp->PlayFoley(FoleyType::FOLEY_DROP);
}

bool FingerDown(Board* theBoard) {
	return gTouch.fingerDown && gTouch.board == theBoard && (theBoard->mApp->mWidgetManager->mDownButtons & 1) != 0;
}

// The finger in board coordinates.
void FingerPos(Board* theBoard, int& x, int& y) {
	x = MouseBoardX(theBoard, theBoard->mApp->mWidgetManager->mLastMouseX);
	y = MouseBoardY(theBoard, theBoard->mApp->mWidgetManager->mLastMouseY);
}

void FillRect(Sexy::Graphics* g, const PxRect& r) { g->FillRect(r.x, r.y, r.w, r.h); }

} // namespace

bool MobilePlantingActive(Board* theBoard) {
	if (!SeedPanelActive(theBoard))
		return false;
	LawnApp* app = theBoard->mApp;
	// Demos replay recorded clicks; levels with their own click rules keep vanilla's.
	return !app->mPlayingDemoBuffer && !app->mRecordingDemoBuffer && !app->IsIZombieLevel() && !app->IsWhackAZombieLevel();
}

void MobileNoteDown(Board* theBoard, int x, int y) {
	if (gTouch.releasing)
		return;
	gTouch.board = theBoard;
	gTouch.fingerDown = MobilePlantingActive(theBoard);
	gTouch.heldAtDown = Held(theBoard);
	gTouch.switched = false;
	gTouch.press = TouchPress{};
	gTouch.press.downX = x;
	gTouch.press.downY = y;
}

bool MobileMouseDown(Board* theBoard, int x, int y, int theClickCount) {
	if (gTouch.releasing || theClickCount < 0 || !MobilePlantingActive(theBoard))
		return false;
	const int aHeld = Held(theBoard);
	const TouchHit aHit = Classify(theBoard, aHeld, x, y);
	switch (OnTouchDown(aHeld, aHit)) {
	case DownAction::Vanilla:
		return false;
	case DownAction::LawnPress:
		gTouch.press.lawnPress = true;
		return true;
	case DownAction::Deselect:
		PutBack(theBoard);
		return true;
	case DownAction::Switch:
		// Put the held one back quietly; vanilla then picks the new one up (SOUND_SEEDLIFT) from this press.
		if (aHeld == kShovelHeld)
			theBoard->ClearCursor();
		else
			theBoard->RefreshSeedPacketFromCursor();
		gTouch.switched = true;
		gTouch.heldAtDown = kNothingHeld;
		return false;
	case DownAction::Buzz:
		theBoard->mSeedBank->mSeedPackets[aHit.packet].MouseDown(x, y, theClickCount); // buzzer (+ red sun)
		return true;
	}
	return false;
}

void MobileMouseUp(Board* theBoard, int x, int y) {
	if (gTouch.board != theBoard || gTouch.releasing)
		return;
	gTouch.fingerDown = false;
	if (!MobilePlantingActive(theBoard))
		return;
	const int aHeld = Held(theBoard);
	TouchPress aPress = gTouch.press;
	aPress.pickedThisPress = gTouch.heldAtDown == kNothingHeld && aHeld != kNothingHeld;
	const bool aOnOrigin = aHeld == kShovelHeld ? ShovelAt(theBoard, x, y) : (aHeld >= 0 && PacketAt(theBoard, x, y) == aHeld);
	const UpAction aAction = OnTouchUp(aPress, aHeld, x, y, aOnOrigin);
	gTouch.press = TouchPress{};
	if (aAction == UpAction::None)
		return;
	gTouch.releasing = true;
	theBoard->MouseDown(x, y, 1); // vanilla's click: plants (or digs), advice on a bad tile, returns it off the lawn
	gTouch.releasing = false;
	if (aAction == UpAction::ReleaseOrReturn && Held(theBoard) != kNothingHeld)
		PutBack(theBoard);
}

bool DrawPacketAsHeld(Board* theBoard, int theIndex) {
	return MobilePlantingActive(theBoard) && Held(theBoard) == theIndex && theIndex >= 0;
}

bool TilePreviewAllowed(Board* theBoard) {
	return !MobilePlantingActive(theBoard) || FingerDown(theBoard);
}

bool HeldImageShown(Board* theBoard, int& theLiftY) {
	theLiftY = 0;
	if (!MobilePlantingActive(theBoard))
		return true;
	const int aHeld = Held(theBoard);
	if (aHeld == kNothingHeld)
		return true; // other cursors (hammer, cob cannon target...) as vanilla
	if (!FingerDown(theBoard))
		return false;
	if (aHeld == kShovelHeld)
		return true; // the shovel is drawn up and to the right of the finger, over the lawn too
	int x = 0, y = 0, aCol = 0, aRow = 0;
	FingerPos(theBoard, x, y);
	const bool aOverLawn = LawnTileAt(theBoard, aHeld, x, y, aCol, aRow);
	theLiftY = -kHeldImageLift;
	return HeldImageVisible(true, aOverLawn);
}

void DrawPlantingCrosshair(Sexy::Graphics* g, Board* theBoard) {
	if (!MobilePlantingActive(theBoard) || !FingerDown(theBoard))
		return;
	const int aHeld = Held(theBoard);
	if (aHeld == kNothingHeld)
		return;
	int x = 0, y = 0, aCol = -1, aRow = -1;
	FingerPos(theBoard, x, y);
	if (aHeld == kShovelHeld) {
		Plant* aPlant = theBoard->ToolHitTest(x, y);
		if (aPlant == nullptr)
			return;
		aCol = aPlant->mPlantCol;
		aRow = aPlant->mRow;
	} else if (!LawnTileAt(theBoard, aHeld, x, y, aCol, aRow) ||
		theBoard->CanPlantAt(aCol, aRow, theBoard->GetSeedTypeInCursor()) != PlantingReason::PLANTING_OK) {
		return; // only where the plant can go (the rule of vanilla's see-through preview)
	}
	const Crosshair c = TileCrosshair(GridOf(theBoard), aCol, aRow);
	g->SetDrawMode(Sexy::Graphics::DRAWMODE_ADDITIVE);
	g->SetColor(Sexy::Color(255, 255, 255, kCrosshairAlpha));
	for (const PxRect& r : c.row)
		FillRect(g, r);
	for (const PxRect& r : c.column)
		FillRect(g, r);
	g->SetDrawMode(Sexy::Graphics::DRAWMODE_NORMAL);
}

PacketRect SelectedPacketRect(Board* theBoard, int theIndex) {
	// The panel card layout's own rect (screen px), moved into board coordinates.
	const Sexy::Rect r = SeedPacketScreenRect(theBoard->mSeedBank.get(), theIndex);
	return {r.mX - theBoard->mX, r.mY - theBoard->mY, r.mWidth, r.mHeight};
}

void DrawSelectionBrackets(Sexy::Graphics* g, Board* theBoard) {
	if (!MobilePlantingActive(theBoard))
		return;
	const int aHeld = Held(theBoard);
	const int aNumPackets = theBoard->mSeedBank->mNumPackets;
	PxRect aRect;
	int aHudIndex = -2; // the HUD card or shovel box (-1) whose brackets must fit between the HUD's items
	if (aHeld == kShovelHeld) {
		const Sexy::Rect s = theBoard->GetShovelButtonRect();
		aRect = {s.mX, s.mY, s.mWidth, s.mHeight};
		if (HudLayout(theBoard))
			aHudIndex = -1;
	} else if (aHeld >= 0 && aHeld < aNumPackets) {
		const PacketRect p = SelectedPacketRect(theBoard, aHeld);
		aRect = {p.x, p.y, p.w, p.h};
		if (SeedPanelActive(theBoard))
			aHudIndex = aHeld;
	} else {
		return;
	}
	// Phone bug list 2026-11-23 (overlap): on the HUD the full brackets ran into the sun box and the next card; they shrink to the room.
	const BracketStyle aStyle = aHudIndex >= -1 ? HudBracketStyle(aHudIndex, aNumPackets, CurrentLayout()) : DefaultBracketStyle(aRect.w);
	const std::vector<BracketArm> aArms = CornerBrackets(aRect, aStyle);
	// Soft glow first (two widening orange layers, never over the packet: they grow at most kBracketGap px),
	// then the orange arms, then the yellow inner line.
	for (int aGrow : {aStyle.glow, std::min(aStyle.glow, 1)}) {
		if (aGrow <= 0)
			continue;
		g->SetColor(Sexy::Color(240, 138, 0, aGrow == 1 ? 110 : 50));
		for (const BracketArm& a : aArms)
			g->FillRect(a.orange.x - aGrow, a.orange.y - aGrow, a.orange.w + 2 * aGrow, a.orange.h + 2 * aGrow);
	}
	g->SetColor(Sexy::Color(240, 138, 0));
	for (const BracketArm& a : aArms)
		FillRect(g, a.orange);
	g->SetColor(Sexy::Color(255, 224, 64));
	for (const BracketArm& a : aArms)
		FillRect(g, a.yellow);
}

} // namespace pvzu::engine
