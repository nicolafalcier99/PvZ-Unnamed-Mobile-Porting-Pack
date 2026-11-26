#include "Mod/Engine/OptionsPanel.h"

#include "ConstEnums.h"
#include "LawnApp.h"
#include "Lawn/Board.h"
#include "Lawn/Coin.h"
#include "Lawn/LawnCommon.h"
#include "Lawn/System/Music.h"
#include "Lawn/System/SaveGame.h"
#include "Lawn/Plant.h"
#include "Lawn/System/PlayerInfo.h"
#include "Lawn/System/ProfileMgr.h"
#include "Lawn/Widget/TitleScreen.h"
#include "Lawn/Zombie.h"
#include "Mod/CmdLine.h"
#include "Mod/GameData.h"
#include "Mod/Layout.h"
#include "Mod/Mode.h"
#include "Mod/OptionsPanel.h"
#include "Mod/SeedSlots.h"
#include "Mod/Unnamed/Rules.h"
#include "Mod/Engine/ModPlants.h"
#include "Mod/Engine/Startup.h"
#include "Mod/Engine/WideScreen.h"
#include "Mod/Engine/TestCheats.h" // PVZU Plan 9 C1
#include "Mod/Engine/UnnamedMusic.h"
#include "PvzpLib/PvzpCommon.h"
#include "Resources.h"
#include "SexyAppBase.h"
#include "SexyAppFramework/Common.h"
#include "graphics/Font.h"
#include "graphics/Graphics.h"
#include "widget/Dialog.h"
#include "widget/WidgetManager.h"

#include <algorithm>
#include <cstdio>
#include <format>
#include <sstream>
#include <string>

namespace u = pvzu::unnamed;

namespace pvzu::engine {

namespace {

constexpr int kOptionsDialogId = 0x80; // the exe's dialog id (0xb76c02(0x80), hooks 80 / 457)

bool gLoaded = false;
u::Options gOptions;
LongPress gMenuPress;
const void* gMenuOwner = nullptr; // the screen whose Menu button is held (board or seed chooser)

int Tick() { return gSexyAppBase ? static_cast<int>(gSexyAppBase->mUpdateCount) : 0; }

// -pvzu-dev-options eliteZombies=1,doubleSun=1
void ApplyDevOverrides(u::Options& theOptions) {
	const auto aText = CmdOption("-pvzu-dev-options");
	if (!aText)
		return;
	std::map<std::string, int> aMap = u::OptionsToMap(theOptions);
	std::stringstream ss(*aText);
	std::string aPair;
	while (std::getline(ss, aPair, ',')) {
		const size_t eq = aPair.find('=');
		if (eq == std::string::npos)
			continue;
		try {
			aMap[aPair.substr(0, eq)] = std::stoi(aPair.substr(eq + 1));
		} catch (...) {
		}
	}
	theOptions = u::OptionsFromMap(aMap);
}

void SaveOptions() {
	if (CmdOption("-pvzu-dev-options"))
		return; // a dev run never writes its overrides
	SaveUnnamedOptions(u::OptionsToMap(gOptions));
}

bool InPlayableLevel(LawnApp* theApp) {
	Board* b = theApp->mBoard;
	return b && theApp->mGameScene == GameScenes::SCENE_PLAYING && !theApp->IsSurvivalMode() &&
		theApp->mGameMode != ::GameMode::GAMEMODE_CHALLENGE_ZEN_GARDEN && theApp->mGameMode != ::GameMode::GAMEMODE_TREE_OF_WISDOM &&
		b->mBoardFadeOutCounter < 0 && !b->mLevelComplete;
}

// ---- R8 存档 / 读档 (0xb72a00, hooks 244 / 769; r8-options.md 6) ----
bool gQuickSaved = false; // the exe's [0xb72a6b]: load does nothing before the first save of the run

// A level where the board can be saved (Board::TryToSaveGame's own test, 0x408c30) and is not ending.
bool CanQuickSave(LawnApp* theApp) {
	Board* b = theApp->mBoard;
	return b && theApp->mPlayerInfo && b->NeedSaveGame() && b->mBoardFadeOutCounter < 0 && !b->mLevelComplete;
}

std::string QuickSlot(LawnApp* theApp) {
	return u::QuickSaveName(GetSavedGameName(theApp->mGameMode, theApp->mPlayerInfo->mId));
}

bool QuickSave(LawnApp* theApp) {
	if (!CanQuickSave(theApp))
		return false;
	MkDir(GetAppDataPath(pvzu::CurrentUserDataPath("")));
	if (!LawnSaveGame(theApp->mBoard, QuickSlot(theApp)))
		return false;
	gQuickSaved = true;
	return true;
}

// TryLoadGame (0x44f7a0) from the quick slot without the continue dialog (magic 0x114514, hook 244); the new board
// starts paused under the still-open panel (Board+0x164 = 1), the panel's close resumes it.
bool QuickLoad(LawnApp* theApp) {
	if (!gQuickSaved || !CanQuickSave(theApp))
		return false;
	const std::string aSlot = QuickSlot(theApp);
	if (!theApp->FileExists(aSlot))
		return false;
	theApp->mMusic->StopAllMusic();
	theApp->MakeNewBoard();
	if (!theApp->mBoard->LoadGame(aSlot)) {
		theApp->KillBoard();
		theApp->DoBackToMain();
		return false;
	}
	theApp->mBoard->Pause(true);
	return true;
}

// The vanilla dialog frame (LawnDialog::Draw's tiles), at any size: edges and centre tiled and clipped.
void DrawDialogFrame(Sexy::Graphics* g, int x, int y, int w, int h) {
	using namespace Sexy;
	Image* tl = IMAGE_DIALOG_TOPLEFT; Image* tm = IMAGE_DIALOG_TOPMIDDLE; Image* tr = IMAGE_DIALOG_TOPRIGHT;
	Image* cl = IMAGE_DIALOG_CENTERLEFT; Image* cm = IMAGE_DIALOG_CENTERMIDDLE; Image* cr = IMAGE_DIALOG_CENTERRIGHT;
	Image* bl = IMAGE_DIALOG_BOTTOMLEFT; Image* bm = IMAGE_DIALOG_BOTTOMMIDDLE; Image* br = IMAGE_DIALOG_BOTTOMRIGHT;
	if (!tl || !tm || !tr || !cl || !cm || !cr || !bl || !bm || !br) {
		g->SetColor(Color(40, 36, 60));
		g->FillRect(x, y, w, h);
		return;
	}
	const int midX0 = x + tl->mWidth, midX1 = x + w - tr->mWidth;
	const int midY0 = y + tl->mHeight, midY1 = y + h - bl->mHeight;
	auto tileRow = [&](Image* img, int ty, int th) {
		g->SetClipRect(midX0, ty, midX1 - midX0, th);
		for (int tx = midX0; tx < midX1; tx += img->mWidth)
			g->DrawImage(img, tx, ty);
		g->ClearClipRect();
	};
	auto tileCol = [&](Image* img, int tx, int tw) {
		g->SetClipRect(tx, midY0, tw, midY1 - midY0);
		for (int ty = midY0; ty < midY1; ty += img->mHeight)
			g->DrawImage(img, tx, ty);
		g->ClearClipRect();
	};
	g->SetClipRect(midX0, midY0, midX1 - midX0, midY1 - midY0);
	for (int ty = midY0; ty < midY1; ty += cm->mHeight)
		for (int tx = midX0; tx < midX1; tx += cm->mWidth)
			g->DrawImage(cm, tx, ty);
	g->ClearClipRect();
	tileRow(tm, y, tm->mHeight);
	tileRow(bm, midY1, bm->mHeight);
	tileCol(cl, x, cl->mWidth);
	tileCol(cr, midX1, cr->mWidth);
	g->DrawImage(tl, x, y);
	g->DrawImage(tr, midX1, y);
	g->DrawImage(bl, x, midY1);
	g->DrawImage(br, midX1, midY1);
}

// The stone button (GameButton's art) stretched to the touch rect's width, at its own height, centred in the rect.
void DrawStone(Sexy::Graphics* g, const HitRect& r, bool down, const std::string& label) {
	using namespace Sexy;
	Image* l = down ? IMAGE_BUTTON_DOWN_LEFT : IMAGE_BUTTON_LEFT;
	Image* m = down ? IMAGE_BUTTON_DOWN_MIDDLE : IMAGE_BUTTON_MIDDLE;
	Image* rr = down ? IMAGE_BUTTON_DOWN_RIGHT : IMAGE_BUTTON_RIGHT;
	if (!l || !m || !rr)
		return;
	const int y = r.y + (r.h - l->mHeight) / 2 + (down ? 1 : 0);
	const int x0 = r.x + (down ? 1 : 0);
	g->DrawImage(l, x0, y);
	g->SetClipRect(x0 + l->mWidth, y, r.w - l->mWidth - rr->mWidth, m->mHeight);
	for (int x = x0 + l->mWidth; x < x0 + r.w - rr->mWidth; x += m->mWidth)
		g->DrawImage(m, x, y);
	g->ClearClipRect();
	g->DrawImage(rr, x0 + r.w - rr->mWidth, y);
	_Font* f = FONT_DWARVENTODCRAFT18GREENINSET;
	PvzpDrawString(g, label, x0 + r.w / 2, y + (l->mHeight + f->GetAscent()) / 2 - 3, f, Color::White,
		DrawStringJustification::DS_ALIGN_CENTER);
}

// One option: a lighter plate (its whole area is the button), the name on the left, the value on the right; an optional
// hint line under the name (the CARD row: what it still needs).
void DrawOptionRow(Sexy::Graphics* g, const HitRect& r, bool down, const std::string& label, const std::string& value,
	bool nonDefault, const std::string& hint = "") {
	using namespace Sexy;
	const int d = down ? 1 : 0;
	g->SetColor(Color(0, 0, 0, 90));
	g->FillRect(r.x + 3, r.y + 4, r.w, r.h);              // shadow
	g->SetColor(down ? Color(58, 44, 30) : Color(80, 60, 40));
	g->FillRect(r.x + d, r.y + d, r.w, r.h);
	g->SetColor(down ? Color(92, 70, 46) : Color(120, 92, 60));
	g->FillRect(r.x + d + 3, r.y + d + 3, r.w - 6, r.h - 6);
	_Font* f = FONT_DWARVENTODCRAFT18;
	// value pill on the right (wider for a long value: the music row's "Unnamed mod")
	const int pillW = std::max(100, f->StringWidth(value) + 24), pillH = r.h - 20;
	const int pillX = r.x + d + r.w - 10 - pillW, pillY = r.y + d + 10;
	g->SetColor(nonDefault ? Color(56, 120, 30) : Color(70, 52, 36));
	g->FillRect(pillX, pillY, pillW, pillH);
	const int textY = r.y + d + (r.h + f->GetAscent()) / 2 - 2;
	_Font* hf = FONT_BRIANNETOD12;
	const int labelY = hint.empty() || hf == nullptr ? textY : r.y + d + 8 + f->GetAscent();
	PvzpDrawString(g, label, r.x + d + 16, labelY, f, Color(250, 236, 196), DrawStringJustification::DS_ALIGN_LEFT);
	if (!hint.empty() && hf != nullptr)
		PvzpDrawString(g, hint, r.x + d + 16, r.y + d + r.h - 10, hf, Color(255, 214, 120), DrawStringJustification::DS_ALIGN_LEFT);
	PvzpDrawString(g, value, pillX + pillW / 2, textY, f, nonDefault ? Color(220, 255, 170) : Color(225, 205, 160),
		DrawStringJustification::DS_ALIGN_CENTER);
}

class OptionsPanelDialog : public Sexy::Dialog {
public:
	explicit OptionsPanelDialog(LawnApp* theApp)
		: Sexy::Dialog(nullptr, nullptr, kOptionsDialogId, true, "", "", "", Sexy::Dialog::BUTTONS_NONE), mApp(theApp) {
		mWithWin = InPlayableLevel(theApp);
		mWithSave = ExtraOptionsOn() && CanQuickSave(theApp); // R8 存档 / 读档
		mOptions = u::PanelOptions(ExtraOptionsOn(), UnnamedMusicOn()); // PVZU: the music row (feature unnamedMusic)
		mHasAlpha = true; // the dimmed screen shows through
		Resize(0, 0, theApp->mWidth, theApp->mHeight);
		Relayout();
	}

	void Relayout() {
		const PlayLayout& pl = CurrentLayout();
		mLayout = LayoutOptionsPanel(mWidth, mHeight, pl.safeLeft, pl.safeRight, u::PanelRowCount(ModPlantsOn(), ExtraOptionsOn(), UnnamedMusicOn()),
			mWithWin, mWithSave);
	}

	void Draw(Sexy::Graphics* g) override {
		using namespace Sexy;
		g->SetColor(Color(0, 0, 0, 140));
		g->FillRect(0, 0, mWidth, mHeight);
		const HitRect& p = mLayout.panel;
		DrawDialogFrame(g, p.x, p.y, p.w, p.h);
		_Font* tf = FONT_DWARVENTODCRAFT24;
		PvzpDrawString(g, mApp->GetString("PVZU_OPT_TITLE", "Unnamed Options"), mLayout.title.x + mLayout.title.w / 2,
			mLayout.title.y + (mLayout.title.h + tf->GetAscent()) / 2, tf, Color(224, 187, 98), DrawStringJustification::DS_ALIGN_CENTER);
		const u::Options& o = CurrentOptions();
		for (size_t i = 0; i < mOptions.size(); i++) {
			const u::OptionInfo& info = u::InfoFor(mOptions[i]);
			const int v = o.Get(info.id);
			DrawOptionRow(g, mLayout.rows[i], mDown == Target::Row && mDownRow == static_cast<int>(i), mApp->GetString(info.labelKey, info.label),
				mApp->GetString(u::OptionValueLabelKey(info.id, v), u::OptionValueLabel(info.id, v)), v != info.defaultValue);
		}
		if (IsCardRow(static_cast<int>(mOptions.size()))) { // CARD: mod-only plants in the chooser (w6-w7-and-new-content.md 1.3)
			const u::CardRowText c = u::CardRow(CardSetting(), ImitaterOwned(mApp), CardPending());
			DrawOptionRow(g, mLayout.rows[mOptions.size()], mDown == Target::Row && mDownRow == static_cast<int>(mOptions.size()),
				mApp->GetString(u::kCardRowLabelKey, u::kCardRowLabel), mApp->GetString(c.valueKey, c.value), c.nonDefault,
				*c.hintKey ? mApp->GetString(c.hintKey, c.hint) : std::string());
		}
		DrawStone(g, mLayout.done, mDown == Target::Done, mApp->GetString("PVZU_OPT_DONE", "Done"));
		if (mLayout.win.w > 0)
			DrawStone(g, mLayout.win, mDown == Target::Win, mApp->GetString("PVZU_OPT_WIN", "Win this level"));
		if (TestCheatsOn()) // PVZU Plan 9 C1: the test cheats (feature testCheats)
			DrawStone(g, cheats::OptionsTitleCheatsButton(mLayout), mDown == Target::Cheats, mApp->GetString("PVZU_CHEAT_TITLE", "Test cheats"));
		if (mLayout.save.w > 0) { // R8 存档 / 读档
			DrawStone(g, mLayout.save, mDown == Target::Save, mApp->GetString("PVZU_OPT_SAVE_GAME", "Save game"));
			DrawStone(g, mLayout.load, mDown == Target::Load, mApp->GetString("PVZU_OPT_LOAD_GAME", "Load game"));
		}
	}

	void MouseDown(int x, int y, int theBtnNum, int) override {
		if (theBtnNum != 0) // left button / finger only
			return;
		mDown = Target::None;
		if (TestCheatsOn() && Inside(cheats::OptionsTitleCheatsButton(mLayout), x, y)) { // PVZU Plan 9 C1
			mDown = Target::Cheats;
		} else if (const int r = OptionsRowAt(mLayout, x, y); r >= 0) {
			mDown = Target::Row;
			mDownRow = r;
		} else if (Inside(mLayout.done, x, y)) {
			mDown = Target::Done;
		} else if (Inside(mLayout.win, x, y)) {
			mDown = Target::Win;
		} else if (Inside(mLayout.save, x, y)) {
			mDown = Target::Save;
		} else if (Inside(mLayout.load, x, y)) {
			mDown = Target::Load;
		} else if (!Inside(mLayout.panel, x, y)) {
			mDown = Target::Outside; // a tap outside the panel closes it
		}
		MarkDirty();
	}
	void MouseDrag(int, int) override {}
	void MouseUp(int x, int y, int theBtnNum, int) override {
		const Target aDown = mDown;
		mDown = Target::None;
		MarkDirty();
		if (aDown == Target::None)
			gMenuPress.Up(); // the release of the long press that opened the panel came here
		if (theBtnNum != 0)
			return;
		if (aDown == Target::Cheats && Inside(cheats::OptionsTitleCheatsButton(mLayout), x, y)) { // PVZU Plan 9 C1
			LawnApp* anApp = mApp;
			anApp->PlaySample(Sexy::SOUND_TAP);
			Close();
			OpenTestCheats(anApp);
		} else if (aDown == Target::Row && OptionsRowAt(mLayout, x, y) == mDownRow && IsCardRow(mDownRow)) {
			const bool aOn = !CardSetting();
			SetCard(mApp, aOn, !CmdOption("-pvzu-dev-options")); // a dev run never writes its overrides
			mApp->PlaySample(Sexy::SOUND_TAP);
			DevLog(std::format("option card={} imitater={} pending={}", aOn ? 1 : 0, ImitaterOwned(mApp) ? 1 : 0, CardPending() ? 1 : 0));
		} else if (aDown == Target::Row && OptionsRowAt(mLayout, x, y) == mDownRow && mDownRow < static_cast<int>(mOptions.size())) {
			const u::OptionInfo& i = u::InfoFor(mOptions[mDownRow]);
			const int v = gOptions.Cycle(i.id);
			SaveOptions();
			ApplyUnnamedOptions(mApp);
			if (i.id == u::OptionId::Music)
				UnnamedMusicChoiceChanged(mApp); // the playing tune switches at once
			mApp->PlaySample(Sexy::SOUND_TAP);
			DevLog(std::format("option {}={}", i.key, v));
		} else if ((aDown == Target::Done && Inside(mLayout.done, x, y)) || (aDown == Target::Outside && !Inside(mLayout.panel, x, y))) {
			mApp->PlaySample(Sexy::SOUND_TAP);
			Close();
		} else if (aDown == Target::Win && Inside(mLayout.win, x, y)) {
			LawnApp* anApp = mApp;
			Close();
			if (InPlayableLevel(anApp)) { // WIN (hook 138): end the level as won (Board+0x5600 = 600 = FadeOutLevel's counter)
				DevLog(std::format("option win level={}", anApp->mBoard->mLevel));
				anApp->mBoardResult = BoardResult::BOARDRESULT_WON;
				anApp->mBoard->FadeOutLevel();
			}
		} else if (aDown == Target::Save && Inside(mLayout.save, x, y)) {
			const bool aOk = QuickSave(mApp); // 0xb72a00: the panel stays open, as the exe dialog does
			mApp->PlaySample(aOk ? Sexy::SOUND_TAP : Sexy::SOUND_BUZZER);
			DevLog(std::format("quick save ok={} level={}", aOk ? 1 : 0, mApp->mBoard ? mApp->mBoard->mLevel : -1));
		} else if (aDown == Target::Load && Inside(mLayout.load, x, y)) {
			const bool aOk = QuickLoad(mApp); // 0xb72ac1: TryLoadGame without the continue dialog, board paused
			mApp->PlaySample(aOk ? Sexy::SOUND_TAP : Sexy::SOUND_BUZZER);
			DevLog(std::format("quick load ok={} level={}", aOk ? 1 : 0, mApp->mBoard ? mApp->mBoard->mLevel : -1));
			if (!mApp->mBoard)
				Close(); // the load failed and went back to the menu
		}
	}

	void DevLog(const std::string& theLine) {
		if (const auto f = CmdOption("-pvzu-log-store"); f) {
			if (FILE* out = std::fopen(f->c_str(), "a")) {
				std::fprintf(out, "%s\n", theLine.c_str());
				std::fclose(out);
			}
		}
		std::fprintf(stderr, "PVZU options: %s\n", theLine.c_str());
	}

private:
	enum class Target { None, Row, Done, Win, Save, Load, Outside, Cheats }; // PVZU: R8 Save/Load, Plan 9 C1 Cheats
	// The CARD row follows the shown options when the pack has mod plants.
	bool IsCardRow(int theRow) const {
		return theRow == static_cast<int>(mOptions.size()) && theRow < static_cast<int>(mLayout.rows.size());
	}
	void Close() { mApp->KillDialog(kOptionsDialogId); }

	LawnApp* mApp;
	OptionsPanelLayout mLayout;
	std::vector<u::OptionId> mOptions; // the rows, in order (u::PanelOptions)
	bool mWithWin = false;
	bool mWithSave = false;
	Target mDown = Target::None;
	int mDownRow = -1;
};

} // namespace

bool OptionsPanelOn() { return GameData::Get().Feature("shopUi"); }

const u::Options& CurrentOptions() {
	if (!gLoaded) {
		gLoaded = true;
		gOptions = u::OptionsFromMap(StartupSettings().unnamedOptions);
		ApplyDevOverrides(gOptions);
	}
	static const u::Options kDefaults;
	return OptionsPanelOn() ? gOptions : kDefaults;
}

void ApplyUnnamedOptions(LawnApp* theApp) {
	if (theApp == nullptr || !OptionsPanelOn())
		return;
	theApp->mEasyPlantingCheat = CurrentOptions().On(u::OptionId::FreePlanting); // LawnApp+0x814, the vanilla cheat
}

int EliteOption() { return u::EffectiveEliteOption(CurrentOptions().Get(u::OptionId::EliteZombies), OptionsPanelOn()); }
bool EliteRollsOff() { return EliteOption() == u::eliteoption::NONE; }

namespace {
int gEliteValueAllDepth = 0; // PVZU elite-all: > 0 inside EliteValueAllScope (hook 565)
}
int EliteValue() { return gEliteValueAllDepth > 0 ? u::eliteoption::ALL : EliteOption(); }
EliteValueAllScope::EliteValueAllScope() { ++gEliteValueAllDepth; }
EliteValueAllScope::~EliteValueAllScope() { --gEliteValueAllDepth; }

int CollectedSunValue(int theValue) {
	return OptionsPanelOn() ? u::CollectedSunValue(theValue, CurrentOptions().On(u::OptionId::DoubleSun)) : theValue;
}

int ConveyorStep(Board* theBoard) {
	if (!OptionsPanelOn())
		return 1;
	return u::ConveyorStep(CurrentOptions().On(u::OptionId::DoubleSun), BoardUpdatesPerFrame(theBoard) >= 2);
}

bool CoinCollectsItself(Coin* theCoin) {
	if (!OptionsPanelOn() || theCoin == nullptr || theCoin->mBoard == nullptr)
		return false;
	return u::CoinCollectsItself(static_cast<int>(theCoin->mType), theCoin->mIsBeingCollected, CurrentOptions().On(u::OptionId::AutoCollect));
}

bool DoorClosed() { return OptionsPanelOn() && CurrentOptions().On(u::OptionId::CloseDoor); }
bool ZombiesStopped() { return OptionsPanelOn() && CurrentOptions().On(u::OptionId::FreezeZombies); }
bool TreePowerOn() { return !OptionsPanelOn() || CurrentOptions().On(u::OptionId::TreePower); }

// ---- R8 ----
bool ExtraOptionsOn() { return OptionsPanelOn() && GameData::Get().Feature("extraOptions"); }
namespace {
int ExtraOption(u::OptionId theId) { return ExtraOptionsOn() ? CurrentOptions().Get(theId) : 0; }
} // namespace
int RainOption() { return ExtraOption(u::OptionId::Rain); }
bool GravestonesOn() { return ExtraOption(u::OptionId::Gravestones) != 0; }
bool NoSpawnLimitOn() { return ExtraOption(u::OptionId::NoSpawnLimit) != 0; }
bool DaveNoPickOn() { return ExtraOption(u::OptionId::DaveNoPick) != 0; }
bool SpawnForwardOn() { return ExtraOption(u::OptionId::SpawnForward) != 0; }
bool FullFogOptionOn() { return ExtraOption(u::OptionId::FullFog) != 0; }

bool DavePicksSeeds(LawnApp* theApp, bool theVanillaPicks) {
	if (!ExtraOptionsOn() || theApp == nullptr || theApp->mPlayerInfo == nullptr)
		return theVanillaPicks;
	const bool aPicks = u::DavePicksSeeds(theVanillaPicks, theApp->mPlayerInfo->mLevel, DaveNoPickOn()); // hook 436: PlayerInfo+0x24
	std::fprintf(stderr, "PVZU options: dave picks=%d vanilla=%d level=%d daveNoPick=%d\n", aPicks ? 1 : 0, theVanillaPicks ? 1 : 0,
		theApp->mPlayerInfo->mLevel, DaveNoPickOn() ? 1 : 0);
	return aPicks;
}

bool DavePickLocks(bool& thePicked) {
	if (thePicked && DaveNoPickOn())
		thePicked = false; // hook 447: `mov byte [ecx+0x38], 0`
	return u::DavePickLocks(thePicked, DaveNoPickOn());
}

bool OptionsMenuDown(LawnApp* theApp, const void* theOwner, int theClickCount) {
	if (!OptionsPanelOn())
		return false;
	if (theClickCount == -1) { // the exe: right click on Menu (hook 80) or on the seed chooser (hook 457)
		gMenuPress.Up();
		OpenOptionsPanel(theApp);
		return true;
	}
	if (theClickCount > 0) {
		gMenuPress.Down(Tick());
		gMenuOwner = theOwner;
	}
	return false;
}

void OptionsMenuPoll(LawnApp* theApp, const void* theOwner, bool theOverMenu) {
	if (!OptionsPanelOn() || !gMenuPress.Pressed() || theOwner != gMenuOwner) // the board's hidden Menu must not cancel the chooser's press
		return;
	const bool aHeld = theOverMenu && theApp->mWidgetManager->mDownButtons != 0;
	if (gMenuPress.Poll(Tick(), aHeld, u::RuleInt("shopUi.panel.longPressTicks", 0)))
		OpenOptionsPanel(theApp);
}

bool OptionsMenuUp() { return OptionsPanelOn() && gMenuPress.Up(); }

void OpenOptionsPanel(LawnApp* theApp) {
	if (!OptionsPanelOn() || theApp->GetDialog(kOptionsDialogId))
		return;
	CurrentOptions();
	theApp->PlaySample(Sexy::SOUND_PAUSE);
	OptionsPanelDialog* aDialog = new OptionsPanelDialog(theApp);
	theApp->AddDialog(kOptionsDialogId, aDialog);
	theApp->mWidgetManager->SetFocus(aDialog);
	std::fprintf(stderr, "PVZU options: panel open (scene %d)\n", static_cast<int>(theApp->mGameScene));
}

bool DevStartScreen(LawnApp* theApp) {
	const auto aScreen = CmdOption("-pvzu-dev-screen");
	if (!aScreen || CmdOption("-pvzu-level"))
		return false;
	if (theApp->mPlayerInfo == nullptr)
		theApp->mPlayerInfo = theApp->mProfileMgr->AddProfile("pvzu");
	if (theApp->mPlayerInfo == nullptr)
		return false;
	PlayerInfo* p = theApp->mPlayerInfo;
	if (p->mLevel < 45)
		p->mLevel = 45; // every shop item and the Zen Garden are open from 5-5
	if (const int c = CmdOptionInt("-pvzu-dev-coins", -1); c >= 0)
		p->mCoins = c;
	if (const int s = CmdOptionInt("-pvzu-dev-slots", 0); s > 0)
		p->mPurchases[StoreItem::STORE_ITEM_PACKET_UPGRADE] = DevSlotsToUpgrades(s);
	std::fprintf(stderr, "PVZU dev: screen %s coins=%d slots-upgrades=%d\n", aScreen->c_str(), p->mCoins,
		p->mPurchases[StoreItem::STORE_ITEM_PACKET_UPGRADE]);
	if (*aScreen == "zen") {
		theApp->mWidgetManager->RemoveWidget(theApp->mTitleScreen.get());
		theApp->SafeDeleteWidget(theApp->mTitleScreen.release());
		theApp->PreNewGame(::GameMode::GAMEMODE_CHALLENGE_ZEN_GARDEN, false);
		return true;
	}
	theApp->LoadingCompleted(); // the main menu
	if (*aScreen == "store")
		theApp->ShowStoreScreen();
	else if (*aScreen == "almanac")
		theApp->DoAlmanacDialog();
	else if (*aScreen == "options")
		OpenOptionsPanel(theApp);
	else if (*aScreen == "cheats") // PVZU Plan 9 C1
		OpenTestCheats(theApp);
	return true;
}

void DevLogPurchase(LawnApp* theApp, int theItem, int theCost, int theCoinsBefore) {
	const auto aFile = CmdOption("-pvzu-log-store");
	if (!aFile || theApp->mPlayerInfo == nullptr)
		return;
	const PlayerInfo* p = theApp->mPlayerInfo;
	const std::string aLine = std::format("purchase mode={} item={} price={} coinsBefore={} coinsAfter={} purchases={} seedSlots={}",
		CurrentMode() == GameMode::Unnamed ? "unnamed" : "vanilla", theItem, theCost, theCoinsBefore, p->mCoins,
		p->mPurchases[theItem], kBaseSeedSlots + p->mPurchases[StoreItem::STORE_ITEM_PACKET_UPGRADE]);
	if (FILE* out = std::fopen(aFile->c_str(), "a")) {
		std::fprintf(out, "%s\n", aLine.c_str());
		std::fclose(out);
	}
	std::fprintf(stderr, "PVZU store: %s\n", aLine.c_str());
}

// PVZU Plan 9 C1: the panel's art for the test-cheats pages (Mod/Engine/TestCheats.cpp).
void PanelDrawFrame(Sexy::Graphics* g, int x, int y, int w, int h) { DrawDialogFrame(g, x, y, w, h); }
void PanelDrawStone(Sexy::Graphics* g, const HitRect& r, bool down, const std::string& label) { DrawStone(g, r, down, label); }
void PanelDrawRow(Sexy::Graphics* g, const HitRect& r, bool down, const std::string& label, const std::string& value, bool nonDefault) {
	DrawOptionRow(g, r, down, label, value, nonDefault);
}

} // namespace pvzu::engine
