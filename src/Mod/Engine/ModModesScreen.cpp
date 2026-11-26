// PVZU: the Unnamed mod-only modes on the challenge screen. Where the exe puts its 48 modes (challenge pages > 3,
// hooks 199-203) is not decoded (study U1), so this is our own layout for a phone: an "Unnamed modes" button on the
// mini-game pages opens a scrolling grid laid out like the challenge page (5 columns, same thumbnails, frames and
// names); drag or the mouse wheel scrolls, a tap starts the mode. Modes that are not built show their name greyed
// with a lock and a one-line reason (status from the pack's data/modes.json).
#include "Mod/Engine/ModModes.h"

#include "Mod/Unnamed/ModModes.h"

#include "LawnApp.h"
#include "Lawn/System/PlayerInfo.h"
#include "Lawn/Widget/ChallengeScreen.h"
#include "Lawn/Widget/GameButton.h"
#include "PvzpLib/PvzpCommon.h"
#include "PvzpLib/PvzpStringFile.h"
#include "Resources.h"
#include "graphics/Font.h"
#include "graphics/Graphics.h"
#include "widget/ButtonListener.h"
#include "widget/WidgetManager.h"

#include <cstdlib>
#include <format>
#include <memory>
#include <string>
#include <vector>

namespace pvzu::engine {

namespace u = pvzu::unnamed;

namespace {

constexpr int kOpenButtonId = 0x7a01;
constexpr int kBackButtonId = 0x7a02;

// Challenge thumbnail cell for a background when the row names none (cells used by vanilla minigames on that lawn).
int IconFor(const u::ModModeDef& theDef) {
	if (theDef.icon > 0)
		return theDef.icon;
	static constexpr int kByBackground[] = {0, 16, 18, 3, 14, 19, 20, 7, 7};
	return theDef.background >= 0 && theDef.background < 9 ? kByBackground[theDef.background] : 0;
}

std::string Text(const char* theKey, const char* theFallback) {
	const std::string aKey = std::format("[{}]", theKey);
	return PvzpStringListExists(aKey) ? std::string(PvzpStringTranslate(aKey)) : std::string(theFallback);
}

class ModModesPanel : public Sexy::Widget, public Sexy::ButtonListener {
public:
	explicit ModModesPanel(ChallengeScreen* theScreen) : mScreen(theScreen), mApp(theScreen->mApp) {
		mModes = u::ModModeMenuOrder();
		mOpenButton = MakeNewButton(kOpenButtonId, this, Text("PVZU_MOD_MODES_BUTTON", "Unnamed modes"), nullptr,
			Sexy::IMAGE_SEEDCHOOSER_BUTTON2, Sexy::IMAGE_SEEDCHOOSER_BUTTON2_GLOW, Sexy::IMAGE_SEEDCHOOSER_BUTTON2_GLOW);
		mOpenButton->mTextDownOffsetX = 1;
		mOpenButton->mTextDownOffsetY = 1;
		mOpenButton->SetLabelColor(Sexy::Color(42, 42, 90));
		mOpenButton->SetLabelHiliteColor(Sexy::Color(42, 42, 90));
		mOpenButton->Resize(800 - 18 - 111, 568, 111, 26);
		mBackButton = MakeNewButton(kBackButtonId, this, Text("PVZU_MOD_MODES_BACK", "Mini-games"), nullptr,
			Sexy::IMAGE_SEEDCHOOSER_BUTTON2, Sexy::IMAGE_SEEDCHOOSER_BUTTON2_GLOW, Sexy::IMAGE_SEEDCHOOSER_BUTTON2_GLOW);
		mBackButton->mTextDownOffsetX = 1;
		mBackButton->mTextDownOffsetY = 1;
		mBackButton->SetLabelColor(Sexy::Color(42, 42, 90));
		mBackButton->SetLabelHiliteColor(Sexy::Color(42, 42, 90));
		mBackButton->Resize(18, 568, 111, 26);
		Resize(0, 0, 800, 600);
		SetOpen(false);
	}

	void AttachTo(ChallengeScreen* theScreen) {
		theScreen->AddWidget(mOpenButton.get());
		theScreen->AddWidget(this);
		AddWidget(mBackButton.get());
		// Back from a mod mode (win, loss or menu): open the list again where the mode was.
		if (u::IsModModeValue(static_cast<int>(mApp->mGameMode))) {
			SetOpen(true);
			ScrollTo(static_cast<int>(mApp->mGameMode));
		}
	}

	void DetachFrom(ChallengeScreen* theScreen) {
		RemoveWidget(mBackButton.get());
		theScreen->RemoveWidget(this);
		theScreen->RemoveWidget(mOpenButton.get());
	}

	void SetOpen(bool theOpen) {
		mOpen = theOpen;
		SetVisible(theOpen);
		mMouseVisible = theOpen;
		mBackButton->SetVisible(theOpen);
		UpdateOpenButton();
		if (theOpen && mWidgetManager)
			mWidgetManager->BringToFront(this);
	}

	void ScrollTo(int theMode) {
		for (size_t i = 0; i < mModes.size(); i++)
			if (mModes[i] == theMode)
				mScroll = mLayout.ClampScroll(static_cast<int>(i) / mLayout.cols * mLayout.rowH - mLayout.rowH, Count());
	}

	void UpdateOpenButton() {
		const bool aPage = mScreen->mPageIndex == CHALLENGE_PAGE_CHALLENGE || mScreen->mPageIndex == CHALLENGE_PAGE_LIMBO;
		mOpenButton->SetVisible(!mOpen && aPage && !mModes.empty());
	}

	int Count() const { return static_cast<int>(mModes.size()); }

	void Update() override {
		Widget::Update();
		UpdateOpenButton();
		if (mOpen)
			MarkDirty();
	}

	void Draw(Sexy::Graphics* g) override {
		if (!mOpen)
			return;
		g->SetLinearBlend(true);
		g->DrawImage(Sexy::IMAGE_CHALLENGE_BACKGROUND, 0, 0);
		PvzpDrawString(g, Text("PVZU_MOD_MODES_TITLE", "Unnamed Modes"), 400, 58, Sexy::FONT_HOUSEOFTERROR28,
			Sexy::Color(220, 220, 220), DS_ALIGN_CENTER);
		int aBuilt = 0;
		for (int aMode : mModes)
			if (const u::ModModeDef* d = u::FindModMode(aMode); d && d->Playable())
				aBuilt++;
		PvzpDrawString(g, std::format("{}/{}", aBuilt, Count()), 739, 73, Sexy::FONT_DWARVENTODCRAFT15, Sexy::Color(255, 240, 0),
			DS_ALIGN_CENTER);

		Sexy::Graphics aClip(*g);
		aClip.ClipRect(0, mLayout.top - 4, 800, mLayout.viewH + 8);
		for (int i = 0; i < Count(); i++)
			DrawCell(&aClip, i);

		// scroll bar
		if (mLayout.MaxScroll(Count()) > 0) {
			const int aTrackH = mLayout.viewH;
			const int aContentH = mLayout.Rows(Count()) * mLayout.rowH;
			const int aThumbH = std::max(30, aTrackH * aTrackH / aContentH);
			const int aThumbY = mLayout.top + (aTrackH - aThumbH) * mScroll / mLayout.MaxScroll(Count());
			g->SetColor(Sexy::Color(0, 0, 0, 90));
			g->FillRect(784, mLayout.top, 6, aTrackH);
			g->SetColor(Sexy::Color(255, 240, 0, 200));
			g->FillRect(784, aThumbY, 6, aThumbH);
		}

		if (!mHint.empty())
			PvzpDrawString(g, mHint, 400, 560, Sexy::FONT_BRIANNETOD12, Sexy::Color(255, 240, 0), DS_ALIGN_CENTER);
	}

	void DrawCell(Sexy::Graphics* g, int theIndex) {
		const u::ModModeDef* aDef = u::FindModMode(mModes[theIndex]);
		if (aDef == nullptr)
			return;
		int x = 0, y = 0;
		mLayout.CellPos(theIndex, mScroll, x, y);
		if (y + mLayout.cellH < mLayout.top - 4 || y > mLayout.top + mLayout.viewH + 4)
			return;
		const bool aPlayable = aDef->Playable();
		const bool aDown = theIndex == mPressIndex && !mDragging;
		if (aDown) {
			x++;
			y++;
		}
		if (!aPlayable) {
			g->SetColor(Sexy::Color(92, 92, 92));
			g->SetColorizeImages(true);
		}
		g->DrawImageCel(Sexy::IMAGE_CHALLENGE_THUMBNAILS, x + 13, y + 4, IconFor(*aDef));
		g->SetColorizeImages(false);
		g->DrawImage(aDown ? Sexy::IMAGE_CHALLENGE_WINDOW : Sexy::IMAGE_CHALLENGE_WINDOW_HIGHLIGHT, x - 6, y - 2);
		const Sexy::Color aTextColor = aDown ? Sexy::Color(250, 40, 40) : aPlayable ? Sexy::Color(42, 42, 90) : Sexy::Color(90, 90, 90);
		DrawName(g, ModModeDisplayName(*aDef), x, y, aTextColor);
		if (!aPlayable)
			PvzpDrawImageScaledF(g, Sexy::IMAGE_LOCK, x + 24, y + 9, 0.7f, 0.7f);
		else if (mApp->mPlayerInfo && mApp->mPlayerInfo->mChallengeRecords[u::ModModeRecordIndex(aDef->mode)] > 0)
			g->DrawImage(Sexy::IMAGE_MINIGAME_TROPHY, x - 6, y - 2);
	}

	// As ChallengeScreen::DrawButton: one line at y+96, or two at y+88 / y+102 split at the space nearest the middle.
	static void DrawName(Sexy::Graphics* g, const std::string& theName, int x, int y, const Sexy::Color& theColor) {
		Sexy::_Font* aFont = Sexy::FONT_BRIANNETOD12;
		if (aFont->StringWidth(theName) <= 96) {
			PvzpDrawString(g, theName, x + 52, y + 96, aFont, theColor, DS_ALIGN_CENTER);
			return;
		}
		size_t aSplit = std::string::npos;
		for (size_t i = 0; i < theName.size(); i++)
			if (theName[i] == ' ' && (aSplit == std::string::npos ||
				std::abs(static_cast<int>(i) - static_cast<int>(theName.size()) / 2) <
					std::abs(static_cast<int>(aSplit) - static_cast<int>(theName.size()) / 2)))
				aSplit = i;
		if (aSplit == std::string::npos) {
			PvzpDrawString(g, theName, x + 52, y + 96, aFont, theColor, DS_ALIGN_CENTER);
			return;
		}
		PvzpDrawString(g, theName.substr(0, aSplit), x + 52, y + 88, aFont, theColor, DS_ALIGN_CENTER);
		PvzpDrawString(g, theName.substr(aSplit + 1), x + 52, y + 102, aFont, theColor, DS_ALIGN_CENTER);
	}

	void MouseDown(int x, int y, int theBtnNum, int theClickCount) override {
		Widget::MouseDown(x, y, theBtnNum, theClickCount);
		mPressY = y;
		mPressScroll = mScroll;
		mDragging = false;
		mPressIndex = mLayout.HitTest(x, y, mScroll, Count());
	}

	void MouseDrag(int x, int y) override {
		Widget::MouseDrag(x, y);
		if (!mDragging && mLayout.IsDrag(mPressY, y))
			mDragging = true;
		if (mDragging)
			mScroll = mLayout.ClampScroll(mPressScroll + (mPressY - y), Count());
	}

	void MouseUp(int x, int y, int theBtnNum, int theClickCount) override {
		Widget::MouseUp(x, y, theBtnNum, theClickCount);
		const int aPressed = mPressIndex;
		mPressIndex = -1;
		if (mDragging) {
			mDragging = false;
			return;
		}
		// WidgetManager::MouseUp passes x - mX without the parent offset (upstream), so the tap is the press cell when
		// the finger did not drag.
		if (aPressed < 0)
			return;
		const u::ModModeDef* aDef = u::FindModMode(mModes[aPressed]);
		if (aDef == nullptr)
			return;
		if (!aDef->Playable()) {
			mApp->PlaySample(Sexy::SOUND_BUZZER);
			mHint = aDef->note.empty() ? std::string(u::ModModeStatusName(aDef->status)) : aDef->note;
			return;
		}
		mApp->PlaySample(Sexy::SOUND_BUTTONCLICK);
		mApp->KillChallengeScreen();
		mApp->PreNewGame(static_cast<::GameMode>(aDef->mode), true);
	}

	void MouseWheel(int theDelta) override {
		mScroll = mLayout.ClampScroll(mScroll - theDelta * 40, Count());
	}

	void ButtonPress(int) override { mApp->PlaySample(Sexy::SOUND_BUTTONCLICK); }
	void ButtonDownTick(int) override {}
	void ButtonMouseEnter(int) override {}
	void ButtonMouseLeave(int) override {}
	void ButtonMouseMove(int, int, int) override {}
	void ButtonDepress(int theId) override {
		if (theId == kOpenButtonId)
			SetOpen(true);
		else if (theId == kBackButtonId) {
			mHint.clear();
			SetOpen(false);
		}
	}

private:
	ChallengeScreen* mScreen;
	LawnApp* mApp;
	std::unique_ptr<NewLawnButton> mOpenButton;
	std::unique_ptr<NewLawnButton> mBackButton;
	std::vector<int> mModes;
	u::ModeListLayout mLayout;
	bool mOpen = false;
	int mScroll = 0;
	int mPressY = 0;
	int mPressScroll = 0;
	int mPressIndex = -1;
	bool mDragging = false;
	std::string mHint;
};

ModModesPanel* gPanel = nullptr;
ChallengeScreen* gPanelScreen = nullptr;

} // namespace

void ModModesAttach(ChallengeScreen* theScreen) {
	if (!ModModesOn() || gPanel != nullptr)
		return;
	gPanel = new ModModesPanel(theScreen);
	gPanelScreen = theScreen;
	gPanel->AttachTo(theScreen);
}

void ModModesDetach(ChallengeScreen* theScreen) {
	if (gPanel == nullptr || gPanelScreen != theScreen)
		return;
	gPanel->DetachFrom(theScreen);
	// Deferred: the panel's own MouseUp may be what closed the screen.
	theScreen->mApp->SafeDeleteWidget(gPanel);
	gPanel = nullptr;
	gPanelScreen = nullptr;
}

void ModModesOpenList() {
	if (gPanel != nullptr)
		gPanel->SetOpen(true);
}

} // namespace pvzu::engine
