// PVZU: see WideScreen.h.
#include "Mod/NineSlice.h"
#include "Mod/Engine/WideScreen.h"

#include "Mod/CmdLine.h"
#include "Mod/Engine/Startup.h"
#include "Mod/Engine/Zoom.h"
#include "Mod/Hud.h"
#include "Mod/PanelCard.h"
#include "Mod/Mode.h"
#include "Mod/Screen.h"
#include "Mod/SideFill.h"
#include "Mod/StreetFog.h"
#include "Mod/Engine/EdgeFog.h"
#include "Mod/Engine/EliteZombies.h"
#include "PvzpLib/PvzpCommon.h"

#include "ConstEnums.h"
#include "Lawn/Board.h"
#include "Lawn/Challenge.h"
#include "Lawn/Cutscene.h"
#include "Lawn/Coin.h"
#include "Lawn/SeedPacket.h"
#include "Lawn/CursorObject.h"
#include "Mod/Touch.h"
#include "Resources.h"
#include "graphics/Font.h"
#include "graphics/Graphics.h"
#include <algorithm>
#include <cmath>
#include "Lawn/Widget/GameButton.h"
#include "LawnApp.h"
#include "SexyAppBase.h"
#include "widget/Widget.h"
#include "widget/WidgetManager.h"

#include <SDL3/SDL.h>
#include "Mod/Engine/ModModes.h"
#include "Mod/Engine/TestCheats.h" // PVZU Plan 9 C1

namespace pvzu::engine {

void InitLogicalWidth(Sexy::SexyAppBase* theApp) {
	const int aForced = CmdOptionInt("-pvzu-width", 0);
	int aDisplayW = 0, aDisplayH = 0;
	bool aIsPhone = false;
#if (defined(__ANDROID__) && !defined(__TERMUX__)) || defined(__IPHONEOS__)
	// The window does not exist yet; the full-screen window covers the display, so its aspect is the display's.
	aIsPhone = true;
	if (SDL_InitSubSystem(SDL_INIT_VIDEO)) {
		if (const SDL_DisplayMode* aMode = SDL_GetDesktopDisplayMode(SDL_GetPrimaryDisplay())) {
			aDisplayW = aMode->w;
			aDisplayH = aMode->h;
		}
		// Video stays initialised (MakeWindow's SDL_Init only adds a reference); a quit/re-init cycle of the
		// Android video backend before the window exists is not worth the risk.
	}
#endif
	theApp->mWidth = ChooseLogicalWidth(aForced, aDisplayW, aDisplayH, aIsPhone);
	theApp->mHeight = kLogicalHeight;
	SetCurrentLayout(ComputePlayLayout(theApp->mWidth));
	SDL_Log("PVZU: logical screen %d x %d (display %d x %d, forced %d)", theApp->mWidth, theApp->mHeight, aDisplayW, aDisplayH, aForced);
}

const PlayLayout& Wide() { return CurrentLayout(); }

int ScreenWidth() { return CurrentLayout().width; }

int CenteredX() { return CenteredScreenX(CurrentLayout().width); }

bool IsWideBoard(const Board* theBoard) {
	return theBoard != nullptr && theBoard->mBackground <= BackgroundType::BACKGROUND_6_BOSS;
}

BoardCamera Camera(const Board* theBoard) {
	if (IsWideBoard(theBoard))
		return ComputeBoardCamera(CurrentLayout());
	// 800 px boards never pan into house or street: keep them centred.
	const int aCentre = CenteredX();
	BoardCamera c;
	c.rest = aCentre;
	c.house = aCentre + kBackgroundLeftSpare;
	c.street = aCentre + (kVanillaWidth - 1180);
	return c;
}

int UiX(const Board* theBoard, int vanillaX, Anchor theAnchor) {
	if (!IsWideBoard(theBoard))
		return vanillaX;
	return BoardLocalX(vanillaX, theAnchor, CurrentLayout());
}

int MouseBoardX(const Board* theBoard, int screenX) {
	if (theBoard == nullptr)
		return screenX;
	return static_cast<int>(std::floor(ZoomBoardX(RestCameraView(theBoard), static_cast<float>(screenX)))); // PVZU zoom
}

int MouseBoardY(const Board* theBoard, int screenY) {
	if (theBoard == nullptr)
		return screenY;
	return static_cast<int>(std::floor(ZoomBoardY(RestCameraView(theBoard), static_cast<float>(screenY))));
}

int VisibleRightBoardX(const Board* theBoard) {
	return UiX(theBoard, kVanillaWidth, Anchor::Right);
}

float CoinDrawShift(Coin* theCoin) {
	float aFull = 0.0f, aProgress = 0.0f;
	return CoinCollectShift(theCoin, aFull, aProgress) ? aFull * aProgress : 0.0f;
}

bool CoinCollectShift(Coin* theCoin, float& theFullShift, float& theProgress) {
	theFullShift = 0.0f;
	theProgress = 0.0f;
	Board* aBoard = theCoin->mBoard;
	if (!theCoin->mIsBeingCollected || !IsWideBoard(aBoard) || gLawnApp == nullptr)
		return false;
	// Destinations as in Coin::UpdateCollected (vanilla screen coordinates).
	float aDestX = 0.0f, aDestY = 0.0f;
	Anchor aAnchor = Anchor::Left;
	if (theCoin->IsSun()) {
		aDestX = 15.0f;
		aDestY = 0.0f;
	} else if (theCoin->IsMoney()) {
		if (gLawnApp->GetDialog(static_cast<int>(Dialogs::DIALOG_STORE)) ||
			gLawnApp->mGameMode == ::GameMode::GAMEMODE_CHALLENGE_ZEN_GARDEN || gLawnApp->mCrazyDaveState != CrazyDaveState::CRAZY_DAVE_OFF)
			return false;
		aDestX = 39.0f;
		aDestY = 558.0f;
	} else if (theCoin->IsPresentWithAdvice()) {
		aDestX = 35.0f;
		aDestY = 487.0f;
	} else if (theCoin->IsLevelAward()) {
		aDestX = 400.0f - theCoin->mWidth / 2;
		aDestY = 200.0f - theCoin->mHeight / 2;
		aAnchor = Anchor::Center;
	} else {
		return false;
	}
	float aFull = static_cast<float>(UiX(aBoard, static_cast<int>(aDestX), aAnchor)) - static_cast<int>(aDestX);
	if (theCoin->IsSun() && SeedPanelActive(aBoard)) {
		// The HUD's sun counter: the sun lands on the box's sun icon where vanilla's lands on the bank's (x 39).
		const HitRect aBox = TopBarRect(HudBox::Sun);
		const SunBoxParts aParts = SunBoxLayout(aBox.w, aBox.h);
		aFull = static_cast<float>(aBoard->mSeedBank->mX + aBox.x + aParts.icon.x + aParts.icon.w / 2 - 39);
	}
	if (theCoin->IsMoney()) // the coin bank: bottom-right, left of the level label (DrawUICoinBank); vanilla x 57
		aFull = static_cast<float>(CoinBankX(aBoard) - 57);
	const float aStart = std::hypot(theCoin->mCollectX - aDestX, theCoin->mCollectY - aDestY);
	const float aNow = std::hypot(theCoin->mPosX - aDestX, theCoin->mPosY - aDestY);
	theFullShift = aFull;
	theProgress = CollectDrawShift(1.0f, aStart, aNow); // 0 where it was clicked, 1 on arrival
	return true;
}

bool HudLayout(Board* theBoard) {
	if (theBoard == nullptr || !IsWideBoard(theBoard) || !HudFits(CurrentLayout()) || !theBoard->mSeedBank)
		return false;
	LawnApp* app = theBoard->mApp;
	return !app->IsSlotMachineLevel() && !theBoard->HasConveyorBeltSeedBank() && !app->IsChallengeWithoutSeedBank();
}

bool SeedPanelActive(Board* theBoard) {
	if (!HudLayout(theBoard))
		return false;
	// Also during the level intro (the bank slides in at "Ready... Set... Plant!": the HUD slides with it), except up to
	// the end of seed choosing (the chooser flies packets into vanilla's bank slots; the HUD takes over as it leaves).
	LawnApp* app = theBoard->mApp;
	if (app->mGameScene == GameScenes::SCENE_PLAYING)
		return true;
	return app->mGameScene == GameScenes::SCENE_LEVEL_INTRO && theBoard->mCutScene &&
		(!theBoard->ChooseSeedsOnCurrentLevel() || theBoard->mCutScene->IsAfterSeedChooser());
}

void ApplySeedBankLayout(Board* theBoard) {
	if (theBoard == nullptr || !theBoard->mSeedBank)
		return;
	SeedBank* aBank = theBoard->mSeedBank.get();
	if (SeedPanelActive(theBoard)) {
		aBank->mX = UiX(theBoard, 0, Anchor::Left); // HUD-local x 0: the left safe inset
		aBank->mWidth = kHudMargin + kHudColumnW;
		aBank->mHeight = kLogicalHeight;
		for (int i = 0; i < SEEDBANK_MAX; i++) {
			const PanelSlot s = SeedPanelSlot(i, aBank->mNumPackets);
			SeedPacket& p = aBank->mSeedPackets[i];
			p.mX = s.x;
			p.mY = s.y;
			p.mWidth = s.w; // SeedPacket::Draw: a wide or compact card; hit tests use SeedPanelHitRects
			p.mHeight = s.h;
		}
		return;
	}
	if (theBoard->mApp->mGameScene != GameScenes::SCENE_PLAYING)
		return;
	for (SeedPacket& p : aBank->mSeedPackets) { // back to vanilla size on the top bank
		p.mWidth = SEED_PACKET_WIDTH;
		p.mHeight = SEED_PACKET_HEIGHT;
	}
	// Top bank: at the screen's left edge (also for games saved at another width).
	const int aRestX = UiX(theBoard, SEED_BANK_OFFSET_X_END, Anchor::Left);
	if (aBank->mX == SEED_BANK_OFFSET_X_END || aBank->mX == aRestX)
		aBank->mX = aRestX;
}

HudSlide HudSlideOf(Board* theBoard) {
	const PlayLayout& l = CurrentLayout();
	if (theBoard == nullptr || !theBoard->mSeedBank)
		return HudSlideFor(-1, 1, l.safeLeft);
	return HudSlideFor(theBoard->mSeedBank->mY, Sexy::IMAGE_SEEDBANK->GetHeight(), l.safeLeft);
}

Sexy::Rect SeedPacketScreenRect(SeedBank* theBank, int theIndex) {
	const SeedPacket& p = theBank->mSeedPackets[theIndex];
	Board* aBoard = theBank->mBoard;
	const bool aPanel = SeedPanelActive(aBoard);
	const int aBankX = theBank->mX + (aPanel ? HudSlideOf(aBoard).cardsX : 0); // as SeedBank::Draw: the cards slide,
	const int aBankY = aPanel ? 0 : theBank->mY;                                 // the top bank moves up and down
	if (aPanel && (p.mWidth != SEED_PACKET_WIDTH || p.mHeight != SEED_PACKET_HEIGHT)) { // a card is drawn as its frame, centred in the slot (PanelCard.h)
		const Box f = PanelCardLayout(p.mWidth, p.mHeight, 0, 0).frame;
		return Sexy::Rect(aBoard->mX + aBankX + p.mX + p.mOffsetX + f.x, aBoard->mY + aBankY + p.mY + f.y, f.w, f.h);
	}
	return Sexy::Rect(aBoard->mX + aBankX + p.mX + p.mOffsetX, aBoard->mY + aBankY + p.mY, p.mWidth, p.mHeight);
}

void SeedPacketArrowOrigin(Board* theBoard, int theIndex, int& theX, int& theY) {
	SeedBank* aBank = theBoard->mSeedBank.get();
	const SeedPacket& p = aBank->mSeedPackets[theIndex];
	theX = aBank->mX + p.mX;
	theY = aBank->mY + p.mY;
	if (!SeedPanelActive(theBoard))
		return;
	const PanelPoint o = SeedPanelArrowOrigin({p.mX, p.mY, p.mWidth, p.mHeight});
	theX = aBank->mX + o.x;
	theY = o.y; // the cards are never raised (SeedBank::Draw), they slide
}

namespace {
// A brown box styled like the original seed bank (2026-11-22): the bank art's frame as a 9-slice, the inside
// tiled 1:1 with the bank's wood texture (no stretching, so no blur).
void DrawBankBox(Sexy::Graphics* g, int x, int y, int w, int h) {
	Sexy::Image* aBank = Sexy::IMAGE_SEEDBANK;
	if (aBank == nullptr) {
		g->SetColor(Sexy::Color(62, 33, 12));
		g->FillRect(x, y, w, h);
		g->SetColor(Sexy::Color(104, 58, 24));
		g->FillRect(x + 4, y + 4, w - 8, h - 8);
		return;
	}
	const Box aFrameSrc{0, 0, aBank->mWidth, aBank->mHeight};
	const Box aWoodSrc{90, 18, 330, 50};  // middle of the packet area (no frame shadow), right of the sun divider
	const auto aParts = NineSlice(aFrameSrc, kHudBoxFrame, {x, y, w, h});
	for (const Piece& t : TileRects(aWoodSrc, aParts[4].dst))
		g->DrawImage(aBank, t.dst.x, t.dst.y, Sexy::Rect(t.src.x, t.src.y, t.src.w, t.src.h));
	for (size_t i = 0; i < aParts.size(); ++i) {
		if (i == 4)
			continue;
		const Piece& p = aParts[i];
		const Sexy::Rect aDstRect(p.dst.x, p.dst.y, p.dst.w, p.dst.h);
		if (i == 3 || i == 6) {
			// Left edge and bottom-left corner: the bank's left side carries the sun's paper label, so use the
			// clean right side (pieces 5 and 8), mirrored.
			const Piece& r = aParts[i + 2];
			g->DrawImageMirror(aBank, aDstRect, Sexy::Rect(r.src.x, r.src.y, r.src.w, r.src.h));
			continue;
		}
		if (i == 7) {
			// Bottom edge: only the part right of the sun section (the paper label sits on the left).
			const int aFrom = 80;
			g->DrawImage(aBank, aDstRect, Sexy::Rect(aFrom, p.src.y, p.src.x + p.src.w - aFrom, p.src.h));
			continue;
		}
		g->DrawImage(aBank, aDstRect, Sexy::Rect(p.src.x, p.src.y, p.src.w, p.src.h));
	}
}

// The HUD's top-bar box in board coordinates. The sun and speed boxes slide up with the vanilla bank; the shovel
// does not (vanilla's shovel bank never slides: it shows whenever mShowShovel, e.g. during Crazy Dave's shovel lesson).
Sexy::Rect TopBarBoardRect(Board* theBoard, HudBox theBox) {
	const HitRect r = TopBarRect(theBox);
	const int aSlideY = theBox == HudBox::Shovel ? 0 : HudSlideOf(theBoard).barY;
	int aPanX = 0, aPanY = 0; // outside play the HUD stays on the screen while the camera pans (as SeedBank::Draw)
	if (theBoard->mApp->mGameScene != GameScenes::SCENE_PLAYING) {
		aPanX = theBoard->mX - Camera(theBoard).rest;
		aPanY = theBoard->mY;
	}
	return Sexy::Rect(UiX(theBoard, 0, Anchor::Left) + r.x - aPanX, r.y + aSlideY - aPanY, r.w, r.h);
}

constexpr Box kBankSunSrc{16, 9, 46, 46};     // SeedBank.png: the round sun portrait
constexpr Box kBankPaperSrc{12, 59, 55, 24};  // SeedBank.png: the paper label under it (tape at both ends)
constexpr int kBankPaperCap = 12;             // its taped ends, kept unstretched horizontally
} // namespace

void DrawHudSunBox(Sexy::Graphics* g, Board* theBoard) {
	// Bank-local coordinates with y 0 at the screen's top (SeedBank::Draw has cancelled the bank's raise).
	const HitRect aBox = TopBarRect(HudBox::Sun);
	const int x = aBox.x, y = aBox.y + HudSlideOf(theBoard).barY;
	if (y + aBox.h <= 0)
		return;
	DrawBankBox(g, x, y, aBox.w, aBox.h);
	Sexy::Image* aBank = Sexy::IMAGE_SEEDBANK;
	const SunBoxParts p = SunBoxLayout(aBox.w, aBox.h);
	if (aBank) {
		g->DrawImage(aBank, Sexy::Rect(x + p.icon.x, y + p.icon.y, p.icon.w, p.icon.h),
			Sexy::Rect(kBankSunSrc.x, kBankSunSrc.y, kBankSunSrc.w, kBankSunSrc.h));
		// The paper: its two taped ends at their own width, the middle stretched; all stretched to the paper's height.
		const int px = x + p.paper.x, py = y + p.paper.y, pw = p.paper.w, ph = p.paper.h;
		const int aCap = std::min(kBankPaperCap, pw / 2);
		const int aMidSrcW = kBankPaperSrc.w - 2 * kBankPaperCap;
		g->DrawImage(aBank, Sexy::Rect(px, py, aCap, ph), Sexy::Rect(kBankPaperSrc.x, kBankPaperSrc.y, kBankPaperCap, kBankPaperSrc.h));
		g->DrawImage(aBank, Sexy::Rect(px + aCap, py, pw - 2 * aCap, ph),
			Sexy::Rect(kBankPaperSrc.x + kBankPaperCap, kBankPaperSrc.y, aMidSrcW, kBankPaperSrc.h));
		g->DrawImage(aBank, Sexy::Rect(px + pw - aCap, py, aCap, ph),
			Sexy::Rect(kBankPaperSrc.x + kBankPaperSrc.w - kBankPaperCap, kBankPaperSrc.y, kBankPaperCap, kBankPaperSrc.h));
	}
	// The number: vanilla's colours (black, red while flashing "not enough sun"), larger.
	std::string aLabel = std::to_string(std::max(theBoard->mSunMoney, 0));
	ModModeSunLabel(theBoard, aLabel); // mod mode 0x91 "Unlimited" (hook 471)
	Sexy::Color aColor(0, 0, 0);
	if (theBoard->mOutOfMoneyCounter > 0 && theBoard->mOutOfMoneyCounter % 20 < 10)
		aColor = Sexy::Color(255, 0, 0);
	Sexy::_Font* aFont = Sexy::FONT_DWARVENTODCRAFT24;
	if (aFont->StringWidth(aLabel) > p.paper.w - 6) // a word (mod mode 0x91 "Unlimited") or a very long sum: the smaller font
		aFont = Sexy::FONT_DWARVENTODCRAFT15;
	const int aBaseY = y + p.paper.y + (p.paper.h + aFont->GetAscent()) / 2 - 2;
	PvzpDrawString(g, aLabel, x + p.paper.x + p.paper.w / 2, aBaseY, aFont, aColor, DrawStringJustification::DS_ALIGN_CENTER);
}

Sexy::Rect HudShovelRect(Board* theBoard) { return TopBarBoardRect(theBoard, HudBox::Shovel); }

bool DrawHudShovel(Sexy::Graphics* g, Board* theBoard, bool theDrawShovel) {
	if (!HudLayout(theBoard))
		return false;
	const Sexy::Rect r = HudShovelRect(theBoard);
	if (r.mY + r.mHeight <= 0)
		return true;
	// The vanilla shovel bank, 9-sliced to the top-bar box (its sprout corner kept), the shovel scaled into it.
	Sexy::Image* aBankImg = Sexy::IMAGE_SHOVELBANK;
	constexpr int kInset = 18;
	for (const Piece& p : NineSlice({0, 0, aBankImg->mWidth, aBankImg->mHeight}, kInset, {r.mX, r.mY, r.mWidth, r.mHeight}))
		g->DrawImage(aBankImg, Sexy::Rect(p.dst.x, p.dst.y, p.dst.w, p.dst.h), Sexy::Rect(p.src.x, p.src.y, p.src.w, p.src.h));
	if (theDrawShovel) {
		Sexy::Image* aShovel = ModModeShovelImage(theBoard, Sexy::IMAGE_SHOVEL); // the glove in mod item modes (hook 132)
		const float aScale = static_cast<float>(r.mHeight) / aShovel->mHeight * 1.05f;
		const int sw = static_cast<int>(aShovel->mWidth * aScale), sh = static_cast<int>(aShovel->mHeight * aScale);
		if (theBoard->mChallenge->mChallengeState == static_cast<ChallengeState>(15)) // as Board::DrawShovel: the shovel tutorial
		{
			g->SetColorizeImages(true);
			g->SetColor(GetFlashingColor(theBoard->mMainCounter, 75));
		}
		g->DrawImage(aShovel, Sexy::Rect(r.mX + (r.mWidth - sw) / 2, r.mY + (r.mHeight - sh) / 2, sw, sh),
			Sexy::Rect(0, 0, aShovel->mWidth, aShovel->mHeight));
		g->SetColorizeImages(false);
	}
	return true;
}

namespace {
int gPlaySpeed = 0; // 0: not read yet
int PlaySpeed() {
	if (gPlaySpeed == 0)
		gPlaySpeed = StartupSettings().speed == 2 ? 2 : 1;
	return gPlaySpeed;
}

bool SpeedAllowed(Board* theBoard) {
	LawnApp* app = theBoard->mApp;
	return SeedPanelActive(theBoard) && !app->mPlayingDemoBuffer && !app->mRecordingDemoBuffer;
}

// Fast-forward mark: two right-pointing triangles (each theW wide, h tall) drawn row by row with a dark shadow.
void DrawFastForward(Sexy::Graphics* g, int x, int y, int theW, int h, const Sexy::Color& theColor) {
	const int aHalf = std::max(1, h / 2);
	for (int aPass = 0; aPass < 2; ++aPass) {
		const int o = aPass == 0 ? 1 : 0; // shadow first, 1 px down-right
		g->SetColor(aPass == 0 ? Sexy::Color(40, 20, 8) : theColor);
		for (int i = 0; i < h; ++i) {
			const int aLen = (std::min(i, h - 1 - i) + 1) * theW / aHalf;
			g->FillRect(x + o, y + i + o, aLen, 1);
			g->FillRect(x + theW + o, y + i + o, aLen, 1);
		}
	}
}
} // namespace

int BoardUpdatesPerFrame(Board* theBoard) {
	if (theBoard == nullptr || theBoard->mApp->mBoard != theBoard || theBoard->mPaused)
		return 1;
	return TestCheatSpeed(theBoard, SpeedAllowed(theBoard) ? PlaySpeed() : 1); // PVZU Plan 9 C1: test cheat 4x / 8x (any level)
}

bool HudSpeedMouseDown(Board* theBoard, int x, int y, int theClickCount) {
	if (theClickCount <= 0 || !SpeedAllowed(theBoard) || theBoard->mCursorObject->mCursorType != CursorType::CURSOR_TYPE_NORMAL)
		return false;
	if (!TopBarBoardRect(theBoard, HudBox::Speed).Contains(x, y))
		return false;
	gPlaySpeed = NextSpeed(PlaySpeed());
	Settings aSettings = StartupSettings();
	aSettings.speed = gPlaySpeed;
	SaveWantedSettings(aSettings);
	theBoard->mApp->PlaySample(Sexy::SOUND_TAP);
	return true;
}

void DrawHudSpeedButton(Sexy::Graphics* g, Board* theBoard) {
	if (!SeedPanelActive(theBoard))
		return;
	const Sexy::Rect r = TopBarBoardRect(theBoard, HudBox::Speed);
	if (r.mY + r.mHeight <= 0)
		return;
	DrawBankBox(g, r.mX, r.mY, r.mWidth, r.mHeight);
	const bool aFast = PlaySpeed() == 2;
	const int aMarkH = 17, aMarkW = 9;
	const int aMarkX = r.mX + kHudBoxFrame + 3, aMarkY = r.mY + (r.mHeight - aMarkH) / 2;
	DrawFastForward(g, aMarkX, aMarkY, aMarkW, aMarkH, aFast ? Sexy::Color(150, 255, 70) : Sexy::Color(110, 190, 60));
	Sexy::_Font* aFont = Sexy::FONT_DWARVENTODCRAFT24;
	const int aTextLeft = aMarkX + 2 * aMarkW + 3;
	const int aTextX = aTextLeft + (r.mX + r.mWidth - kHudBoxFrame - aTextLeft) / 2;
	const int aShown = TestCheatSpeed(theBoard, PlaySpeed()); // PVZU Plan 9 C1: the test cheat's 4x / 8x shows here
	PvzpDrawString(g, aShown > 2 ? std::to_string(aShown) + "x" : SpeedLabel(PlaySpeed()), aTextX, r.mY + (r.mHeight + aFont->GetAscent()) / 2 - 2, aFont,
		aFast ? Sexy::Color(255, 236, 130) : Sexy::Color(240, 222, 180), DrawStringJustification::DS_ALIGN_CENTER);
}

int CoinBankX(Board* theBoard) {
	if (!SeedPanelActive(theBoard))
		return UiX(theBoard, 57, Anchor::Board); // bottom-left of the lawn view, as before
	const int aLabelRight = theBoard->HasProgressMeter() ? 593 : 780; // Board::DrawLevel
	const int aVanillaX = CoinBankVanillaX(aLabelRight, theBoard->mPvzuLevelLabelWidth, Sexy::IMAGE_COINBANK->GetWidth());
	return UiX(theBoard, aVanillaX, Anchor::Right);
}

bool CoinBankOverHaze(Board* theBoard) {
	return SeedPanelActive(theBoard) && HasStreetFog(theBoard);
}

void UpdateStreetFog(Board* theBoard) {
	const bool aAtRest = std::abs(theBoard->mX - Camera(theBoard).rest) <= 12; // a shake is still "at rest"
	const bool aWanted = IsWideBoard(theBoard) && CurrentLayout().rightExtra > 0 && aAtRest &&
		!theBoard->mApp->IsFinalBossLevel() && theBoard->mApp->mGameScene != GameScenes::SCENE_ZOMBIES_WON;
	EdgeFogState& s = theBoard->mPvzuEdgeFog;
	EdgeFogInput aIn;
	aIn.wanted = aWanted;
	// Fog levels: vanilla's fog is blown off to the right through the intro (mFogBlownCountDown 200) and rolls back in
	// as CutScene counts it down (Board::UpdateGame eases mFogOffset to 0); the cover comes in with it (2026-11-26).
	const bool aIntroFog = theBoard->StageHasFog() && theBoard->mApp->mGameScene == GameScenes::SCENE_LEVEL_INTRO;
	aIn.waitForRoll = aIntroFog && theBoard->mFogBlownCountDown >= 200;
	aIn.rollActive = aIntroFog && theBoard->mFogBlownCountDown > 0 && theBoard->mFogBlownCountDown < 200;
	// Zombies that are not the level-start preview: the cover must be in place now (Mod/StreetFog.h StepEdgeFog). Only
	// looked up while it is not, or could be moved by vanilla's roll.
	if (aWanted && !(s.started && s.in && s.out <= 0.0f && !aIn.rollActive)) {
		for (Zombie* aZombie : theBoard->mZombies) {
			if (!aZombie->mDead && aZombie->mFromWave != Zombie::ZOMBIE_WAVE_CUTSCENE) {
				aIn.zombies = true;
				break;
			}
		}
	}
	s = StepEdgeFog(s, aIn);
}

bool HasStreetFog(const Board* theBoard) {
	return IsWideBoard(theBoard) && CurrentLayout().rightExtra > 0;
}

void DrawStreetFog(Sexy::Graphics* g, Board* theBoard) {
	const PlayLayout& l = CurrentLayout();
	const EdgeFogState& s = theBoard->mPvzuEdgeFog;
	if (!s.started || l.rightExtra <= 0)
		return;
	// To the physical edge (not the UI safe inset), a little past for shakes; zoomed, the edge is at less street.
	const int aScreenRight = std::max(ScreenRightBoardX(l), ViewRightBoardX(theBoard));
	const int aRight = aScreenRight + static_cast<int>(kEdgeFogHiddenMargin);
	// Clear at the lawn's edge, opaque where the first zombie of a wave is drawn (2026-11-25: no popping in).
	const int aMinSpawnX = kVanillaMinSpawnX + static_cast<int>(StartOffsetPx(ZombieType::ZOMBIE_NORMAL, 0));
	const StreetFogRamp aRamp = ComputeStreetFogRamp(ViewRightBoardX(theBoard), aMinSpawnX);
	// Every level: the game's own fog clouds (Mod/Engine/EdgeFog.h; 2026-11-26), vanilla's colouring at night and
	// lighter by day. Never faded: it slides right out of sight while the camera is away and back in at rest; on Fog
	// levels it comes in on vanilla's own fog roll (2026-11-26).
	const float aHidden = EdgeFogHiddenShift(aRamp, aScreenRight);
	const float aShift = EdgeFogShift(s, aHidden, theBoard->mFogOffset);
	if (aShift >= aHidden)
		return; // out of sight
	DrawEdgeFog(g, theBoard, aRamp, aShift, aRight, EdgeFogNightLook(theBoard->mBackground));
}

void UpdateSafeInsets(Sexy::SexyAppBase* theApp) {
	PlayLayout l = CurrentLayout();
	int aWinW = 0, aWinH = 0;
	SDL_Rect aSafe{};
	if (!theApp->mWindow || !SDL_GetWindowSize(theApp->mWindow.get(), &aWinW, &aWinH) || aWinW <= 0 ||
		!SDL_GetWindowSafeArea(theApp->mWindow.get(), &aSafe))
		return;
	// Window points -> logical px across the drawn width (the viewport fills the window on the phone).
	l.safeLeft = aSafe.x * l.width / aWinW;
	l.safeRight = (aWinW - aSafe.x - aSafe.w) * l.width / aWinW;
	if (const int aDevLeft = CmdOptionInt("-pvzu-dev-safe-left", 0); aDevLeft > 0) // Mac check of a phone camera hole
		l.safeLeft = aDevLeft;
	if (const int aDevRight = CmdOptionInt("-pvzu-dev-safe-right", 0); aDevRight > 0) // and its rounded right corners
		l.safeRight = aDevRight;
	if (l.safeLeft != CurrentLayout().safeLeft || l.safeRight != CurrentLayout().safeRight) {
		SetCurrentLayout(l);
		SDL_Log("PVZU: safe insets left %d right %d (logical px)", l.safeLeft, l.safeRight);
	}
}

void PlaceBoard(Board* theBoard) {
	const PlayLayout& l = CurrentLayout();
	theBoard->Resize(Camera(theBoard).rest, 0, kVanillaWidth, kLogicalHeight);
	// Right-edge buttons (menu, store) keep their distance from the screen's right edge.
	const int aShift = UiX(theBoard, 0, Anchor::Right);
	const int aDelta = aShift - theBoard->mPvzuUiShift;
	// Board buttons are drawn in board coordinates; their hit test must be too (the board is not at x=0 when wide).
	if (theBoard->mMenuButton) {
		theBoard->mMenuButton->mX += aDelta;
		theBoard->mMenuButton->mParentWidget = theBoard;
	}
	if (theBoard->mStoreButton) {
		theBoard->mStoreButton->mX += aDelta;
		theBoard->mStoreButton->mParentWidget = theBoard;
	}
	theBoard->mPvzuUiShift = aShift;
	if (IsWideBoard(theBoard))
		theBoard->mMouseInsets = Sexy::Insets(-l.leftExtra, 0, -l.rightExtra, 0);
	else
		theBoard->mMouseInsets = Sexy::Insets(0, 0, 0, 0);
}

bool SideFillWanted() {
	const PlayLayout& l = CurrentLayout();
	if (l.width <= kVanillaWidth || gLawnApp == nullptr || !gLawnApp->mWidgetManager)
		return false;
	const Sexy::WidgetList& aWidgets = gLawnApp->mWidgetManager->mWidgets; // back to front
	for (auto it = aWidgets.rbegin(); it != aWidgets.rend(); ++it) {
		Sexy::Widget* w = *it;
		if (!w->mVisible || w->mWidth < kVanillaWidth || w->mHeight < kLogicalHeight)
			continue;
		if (w->mWidth > kVanillaWidth) {
			// A screen-wide widget (seed chooser) draws its own sides, but only on the screen: every game parks the
			// chooser below it, in front of the board, and it stays there while Crazy Dave talks in the Zen Garden.
			if (!ScreenWideWidgetOwnsSides(w->mY, w->mHeight, kLogicalHeight))
				continue;
			return false;
		}
		if (w == gLawnApp->mBoard)
			return !IsWideBoard(gLawnApp->mBoard);
		return true; // an 800-wide screen on top
	}
	return true; // nothing full-screen: fill anyway (never show bars)
}

void PlaceCenteredScreen(Sexy::Widget* theWidget, int theVanillaX, int theVanillaY) {
	theWidget->Resize(CenteredX() + theVanillaX, theVanillaY, kVanillaWidth, kLogicalHeight);
}

} // namespace pvzu::engine
