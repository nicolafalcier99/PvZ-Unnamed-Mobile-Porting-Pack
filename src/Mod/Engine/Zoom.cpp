// PVZU: see Zoom.h.
#include "Mod/Engine/Zoom.h"

#include "Mod/CmdLine.h"
#include "Mod/Engine/WideScreen.h"
#include "Mod/Hud.h"
#include "Mod/Layout.h"
#include "Mod/TouchPlanting.h"

#include "ConstEnums.h"
#include "LawnApp.h"
#include "Lawn/Board.h"
#include "Lawn/Coin.h"
#include "Lawn/Cutscene.h"
#include "Lawn/SeedPacket.h"
#include "graphics/GLInterface.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace pvzu::engine {

namespace {
constexpr float kZoomStepPerFrame = 1.0f / 50.0f; // 0.5 s from unzoomed to zoomed (board frames are 1/100 s)

bool ZoomedThroughPan() {
	static const bool aZoomed = CmdOption("-pvzu-dev-zoom-pan").value_or("") == "zoomed";
	return aZoomed;
}

float ForcedScale() {
	static const float aScale = [] {
		const std::string s = CmdOption("-pvzu-dev-zoom").value_or("");
		return s.empty() ? 0.0f : std::strtof(s.c_str(), nullptr);
	}();
	return aScale;
}

LawnGrid GridOf(const Board* theBoard) {
	Board* b = const_cast<Board*>(theBoard);
	if (b->StageHasRoof())
		return LawnGrid::Roof;
	if (b->StageHasPool())
		return LawnGrid::Pool;
	return LawnGrid::Day;
}

ZoomView RestView(const Board* theBoard) {
	ZoomView v = ComputeRestZoom(CurrentLayout(), GridOf(theBoard));
	if (const float s = ForcedScale(); s >= 1.0f && HudFits(CurrentLayout())) {
		v.scale = s;
		v.x = kBackgroundLeftSpare * s;
		v.y = std::min(kLogicalHeight - LawnBottom(GridOf(theBoard)) * s, 0.0f);
	}
	return v;
}

bool ZoomAllowed(const Board* theBoard) {
	Board* b = const_cast<Board*>(theBoard);
	if (b == nullptr || !IsWideBoard(b) || !HudLayout(b))
		return false;
	const LawnApp* app = b->mApp;
	if (app->mPlayingDemoBuffer || app->mRecordingDemoBuffer)
		return false;
	return RestView(b).scale > 1.0f;
}

bool ZoomWanted(const Board* theBoard) {
	Board* b = const_cast<Board*>(theBoard);
	const GameScenes aScene = b->mApp->mGameScene;
	if (aScene == GameScenes::SCENE_PLAYING)
		return true;
	if (aScene != GameScenes::SCENE_LEVEL_INTRO || !b->mCutScene)
		return false;
	if (ZoomedThroughPan())
		return true;
	// Eased: once the pan back from the street has ended (lawn mowers roll in, "Ready... Set... Plant!").
	return b->mCutScene->IsAfterSeedChooser() && b->mX == Camera(b).rest && b->mY == 0;
}

// The view with the camera at (camX, camY), at the board's zoom progress.
ZoomView ViewAt(const Board* theBoard, int camX, int camY) {
	const ZoomView aOff = UnzoomedView(camX, camY);
	if (theBoard == nullptr || theBoard->mPvzuZoom <= 0.0f || !ZoomAllowed(theBoard))
		return aOff;
	const ZoomView aRest = RestView(theBoard);
	const BoardCamera aCam = Camera(theBoard);
	ZoomView z = aRest;
	if (ZoomedThroughPan() && theBoard->mApp->mGameScene == GameScenes::SCENE_LEVEL_INTRO)
		z = ZoomedPanView(aRest, aCam, camX, CurrentLayout().width);
	else
		z.x += (camX - aCam.rest) * aRest.scale; // a shake moves the zoomed scene by the zoomed amount
	z.y += camY * aRest.scale;
	return BlendZoom(aOff, z, ZoomEase(theBoard->mPvzuZoom));
}
} // namespace

void UpdateBoardZoom(Board* theBoard) {
	if (!ZoomAllowed(theBoard)) {
		theBoard->mPvzuZoom = 0.0f;
		return;
	}
	const bool aWanted = ZoomWanted(theBoard);
	if (aWanted && ZoomedThroughPan() && theBoard->mApp->mGameScene == GameScenes::SCENE_LEVEL_INTRO) {
		theBoard->mPvzuZoom = 1.0f;
		return;
	}
	theBoard->mPvzuZoom = std::clamp(theBoard->mPvzuZoom + (aWanted ? kZoomStepPerFrame : -kZoomStepPerFrame), 0.0f, 1.0f);
}

ZoomView CurrentBoardView(const Board* theBoard) {
	return ViewAt(theBoard, theBoard->mX, theBoard->mY);
}

bool BoardZoomed(const Board* theBoard) {
	const ZoomView v = CurrentBoardView(theBoard);
	return v.scale != 1.0f || v.x != static_cast<float>(theBoard->mX) || v.y != static_cast<float>(theBoard->mY);
}

void LocalToBoard(const Board* theBoard, int& x, int& y) {
	if (!BoardZoomed(theBoard))
		return;
	const ZoomView v = CurrentBoardView(theBoard);
	x = static_cast<int>(std::floor(ZoomBoardX(v, static_cast<float>(x + theBoard->mX))));
	y = static_cast<int>(std::floor(ZoomBoardY(v, static_cast<float>(y + theBoard->mY))));
}

void BoardToLocal(const Board* theBoard, int& x, int& y) {
	if (!BoardZoomed(theBoard))
		return;
	const ZoomView v = CurrentBoardView(theBoard);
	x = static_cast<int>(std::lround(ZoomScreenX(v, static_cast<float>(x)))) - theBoard->mX;
	y = static_cast<int>(std::lround(ZoomScreenY(v, static_cast<float>(y)))) - theBoard->mY;
}

ZoomView RestCameraView(const Board* theBoard) {
	return ViewAt(theBoard, Camera(theBoard).rest, 0);
}

bool ZoomRenderItem(const Board* theBoard, const RenderItem& theItem) {
	switch (theItem.mRenderObjectType) {
	case RenderObjectType::RENDER_ITEM_BOTTOM_UI:   // seed cards, top bar, menu button
	case RenderObjectType::RENDER_ITEM_COIN_BANK:
	case RenderObjectType::RENDER_ITEM_TOP_UI:      // zooms its own scene parts (haze, held plant)
	case RenderObjectType::RENDER_ITEM_SCREEN_FADE:
		return false;
	case RenderObjectType::RENDER_ITEM_PARTICLE:
		// The tutorial arrow points at a seed card or the shovel. Everything else, also the big texts ("Ready... Set...
		// Plant!", "A huge wave...", placed at the vanilla view's centre), belongs to the lawn: zoomed, they stay centred
		// on it.
		return theBoard->mApp->ParticleGetID(theItem.mParticleSytem) != theBoard->mTutorialParticleID;
	default:
		return true;
	}
}

BoardZoomScope::BoardZoomScope(const Board* theBoard, bool on) {
	if (!on || theBoard == nullptr || !BoardZoomed(theBoard))
		return;
	// Vertices arrive in screen coordinates of the unzoomed board (board point + mX/mY): s * v + (view - s * mX).
	const ZoomView v = CurrentBoardView(theBoard);
	Sexy::GLInterface::PvzuSetWorldTransform(v.scale, v.x - v.scale * theBoard->mX, v.y - v.scale * theBoard->mY);
	mSet = true;
}

BoardZoomScope::~BoardZoomScope() {
	if (mSet)
		Sexy::GLInterface::PvzuClearWorldTransform();
}

void CoinDrawOffset(Coin* theCoin, float& theDX, float& theDY) {
	float aFull = 0.0f, aProgress = 0.0f;
	const bool aFlying = CoinCollectShift(theCoin, aFull, aProgress);
	theDX = aFull * aProgress; // unzoomed: heads for the HUD counter's place (board-local px)
	theDY = 0.0f;
	Board* aBoard = theCoin->mBoard;
	if (!aFlying || aBoard == nullptr || !BoardZoomed(aBoard))
		return;
	// Zoomed: from where the zoomed coin is (no jump when it is picked) to the counter's place on the screen (where the
	// unzoomed coin ends), as the coin flies; then back through the zoom into a board-px offset.
	const ZoomView v = CurrentBoardView(aBoard);
	const float aZoomedX = ZoomScreenX(v, theCoin->mPosX), aZoomedY = ZoomScreenY(v, theCoin->mPosY);
	const float aHudX = theCoin->mPosX + aFull + aBoard->mX, aHudY = theCoin->mPosY + aBoard->mY;
	const float aScreenX = aZoomedX + (aHudX - aZoomedX) * aProgress, aScreenY = aZoomedY + (aHudY - aZoomedY) * aProgress;
	theDX = ZoomBoardX(v, aScreenX) - theCoin->mPosX;
	theDY = ZoomBoardY(v, aScreenY) - theCoin->mPosY;
}

int ViewRightBoardX(const Board* theBoard) {
	return static_cast<int>(std::ceil(ZoomBoardX(CurrentBoardView(theBoard), static_cast<float>(CurrentLayout().width))));
}

} // namespace pvzu::engine
