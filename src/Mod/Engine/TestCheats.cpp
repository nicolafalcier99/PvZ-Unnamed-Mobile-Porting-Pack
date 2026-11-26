// PVZU Plan 9 C1: see Mod/Engine/TestCheats.h.
#include "Mod/Engine/TestCheats.h"

#include "LawnApp.h"
#include "Lawn/Board.h"
#include "Lawn/Challenge.h"
#include "Lawn/Coin.h"
#include "Lawn/GridItem.h"
#include "Lawn/Cutscene.h"
#include "Lawn/CursorObject.h"
#include "Lawn/LawnCommon.h"
#include "Lawn/Plant.h"
#include "Lawn/Projectile.h"
#include "Lawn/SeedPacket.h"
#include "Lawn/System/Music.h"
#include "Lawn/System/PlayerInfo.h"
#include "Lawn/Widget/GameButton.h"
#include "Lawn/Widget/GameSelector.h"
#include "Lawn/Widget/SeedChooserScreen.h"
#include "Lawn/Zombie.h"
#include "Mod/CmdLine.h"
#include "Mod/GameData.h"
#include "Mod/Layout.h"
#include "Mod/Engine/Level55Shooter.h"
#include "Mod/Engine/Level65Snake.h"
#include "Mod/Engine/ModModes.h"
#include "Mod/Engine/SpecialLevels.h"
#include "Mod/Engine/OptionsPanel.h"
#include "Mod/Engine/Startup.h"
#include "Mod/Engine/WideScreen.h"
#include "PvzpLib/PvzpCommon.h"
#include "Resources.h"
#include "SexyAppBase.h"
#include "graphics/Font.h"
#include "graphics/Graphics.h"
#include "widget/Dialog.h"
#include "widget/WidgetManager.h"

#include <cstdio>
#include <filesystem>
#include <format>
#include <sstream>
#include <string>

namespace c = pvzu::cheats;
namespace fs = std::filesystem;

namespace pvzu::engine {

namespace {

constexpr int kCheatsDialogId = 0x81; // next to the options panel's 0x80
constexpr int kInfiniteSun = 9990;

bool gLoaded = false;
c::Cheats gCheats;

void Log(const std::string& theLine) {
	static const std::optional<std::string> aPath = CmdOption("-pvzu-log-bot");
	if (aPath && !aPath->empty())
		if (FILE* f = std::fopen(aPath->c_str(), "a")) {
			std::fprintf(f, "%s\n", theLine.c_str());
			std::fclose(f);
		}
	std::fprintf(stderr, "PVZU cheats: %s\n", theLine.c_str());
}

bool DevRun() { return CmdOption("-pvzu-dev-cheats").has_value() || CmdFlag("-pvzu-autoplay-cheats"); }

void Save() {
	if (DevRun())
		return; // a dev run never writes its overrides
	Settings aFile = LoadSettings(SettingsFile());
	std::map<std::string, int> aMap = aFile.testCheats;
	for (const auto& [k, v] : c::CheatsToMap(gCheats))
		aMap[k] = v;
	aFile.testCheats = aMap;
	SaveSettings(SettingsFile(), aFile);
}

c::LevelReturn LoadReturn() { return c::ReturnFromMap(LoadSettings(SettingsFile()).testCheats); }
void SaveReturn(const c::LevelReturn& theReturn) {
	Settings aFile = LoadSettings(SettingsFile());
	c::ReturnToMap(theReturn, aFile.testCheats);
	SaveSettings(SettingsFile(), aFile);
}

std::string Text(LawnApp* theApp, const char* theKey, const char* theFallback) { return theApp->GetString(theKey, theFallback); }

void EnsureGameButton(LawnApp* theApp); // the in-game button, below the cheats dialog

} // namespace

bool TestCheatsOn() { return GameData::Get().Feature("testCheats"); }

const c::Cheats& CurrentCheats() {
	if (!gLoaded) {
		gLoaded = true;
		gCheats = c::CheatsFromMap(StartupSettings().testCheats);
		if (CmdFlag("-pvzu-autoplay-cheats")) {
			gCheats.Set(c::CheatId::InfiniteSun, 1);
			gCheats.Set(c::CheatId::NoRecharge, 1);
			gCheats.Set(c::CheatId::PlantAnything, 1);
			gCheats.Set(c::CheatId::Invincible, 1);
			gCheats.Set(c::CheatId::SeedSlots, 10);
		}
		if (const auto aText = CmdOption("-pvzu-dev-cheats")) { // k=v,k=v
			std::map<std::string, int> aMap = c::CheatsToMap(gCheats);
			std::stringstream ss(*aText);
			std::string aPair;
			while (std::getline(ss, aPair, ','))
				if (const size_t eq = aPair.find('='); eq != std::string::npos)
					try {
						aMap[aPair.substr(0, eq)] = std::stoi(aPair.substr(eq + 1));
					} catch (...) {
					}
			gCheats = c::CheatsFromMap(aMap);
		}
	}
	static const c::Cheats kOff;
	return TestCheatsOn() ? gCheats : kOff;
}

bool CheatOn(c::CheatId theId) { return TestCheatsOn() && CurrentCheats().On(theId); }

// ---- effects ----
void TestCheatsBoardTick(Board* theBoard) {
	if (!TestCheatsOn() || theBoard == nullptr || theBoard->mApp->mGameScene != GameScenes::SCENE_PLAYING)
		return;
	if (CheatOn(c::CheatId::GameButton))
		EnsureGameButton(theBoard->mApp); // 2026-11-25: the in-game "Cheats" button (made once, it hides itself)
	if (CheatOn(c::CheatId::InfiniteSun) && theBoard->mSunMoney < kInfiniteSun)
		theBoard->mSunMoney = kInfiniteSun;
	if (CheatOn(c::CheatId::NoRecharge) && !theBoard->HasConveyorBeltSeedBank() && !theBoard->mApp->IsSlotMachineLevel() &&
		theBoard->mSeedBank) {
		for (int i = 0; i < theBoard->mSeedBank->mNumPackets; i++) {
			SeedPacket& p = theBoard->mSeedBank->mSeedPackets[i];
			if (p.mPacketType == SeedType::SEED_NONE)
				continue;
			switch (c::NoRechargeFor(p.mActive, p.mRefreshing)) { // C2: the packet's own update ends a refresh (1-1 tutorial)
			case c::NoRechargeStep::FinishRefresh:
				p.mRefreshCounter = c::NoRechargeCounter(p.mRefreshCounter, p.mRefreshTime);
				break;
			case c::NoRechargeStep::Activate:
				p.mActive = true;
				break;
			case c::NoRechargeStep::Leave:
				break;
			}
		}
	}
	if (CheatOn(c::CheatId::Invincible))
		for (Plant* aPlant : theBoard->mPlants)
			if (!aPlant->mDead && aPlant->mPlantHealth < aPlant->mPlantMaxHealth)
				aPlant->mPlantHealth = aPlant->mPlantMaxHealth;
}

bool TestCheatPlantSurvives(Plant* thePlant) { return thePlant != nullptr && CheatOn(c::CheatId::Invincible); }
bool TestCheatAllSeeds() { return CheatOn(c::CheatId::PlantAnything); }
bool TestCheatFreePlantingRules() { return CheatOn(c::CheatId::PlantAnything); }
bool TestCheatModesOpen() { return CheatOn(c::CheatId::AllModes); }

int TestCheatSeedSlots(int theSlots) {
	const int aSlots = TestCheatsOn() ? c::CheatSeedSlots(CurrentCheats().Get(c::CheatId::SeedSlots)) : 0;
	return aSlots > 0 ? aSlots : theSlots;
}

int TestCheatSpeed(Board* theBoard, int theHudSpeed) {
	if (!TestCheatsOn() || theBoard == nullptr || theBoard->mApp->mPlayingDemoBuffer || theBoard->mApp->mRecordingDemoBuffer)
		return theHudSpeed;
	return c::EffectiveSpeed(theHudSpeed, CurrentCheats().Get(c::CheatId::Speed));
}

// ---- actions ----
bool CheatInLevel(LawnApp* theApp) {
	Board* b = theApp->mBoard;
	return TestCheatsOn() && b && theApp->mGameScene == GameScenes::SCENE_PLAYING && b->mBoardFadeOutCounter < 0 && !b->mLevelComplete;
}

void CheatKillAll(Board* theBoard) {
	int n = 0;
	for (Zombie* z : theBoard->mZombies) {
		if (z->IsDeadOrDying() || z->mMindControlled || z->mZombieType == ZombieType::ZOMBIE_BOSS)
			continue;
		z->DieWithLoot();
		n++;
	}
	Log(std::format("kill all: {} zombies (level {}, wave {}/{})", n, theBoard->mLevel, theBoard->mCurrentWave, theBoard->mNumWaves));
}

void CheatNextWave(Board* theBoard) {
	if (theBoard->mHugeWaveCountDown > 0)
		theBoard->mHugeWaveCountDown = 1;
	else if (theBoard->mCurrentWave < theBoard->mNumWaves && theBoard->mZombieCountDown > 6)
		theBoard->mZombieCountDown = 6; // at 5 a flag wave shows "huge wave" first, as in play
	Log(std::format("next wave: wave {}/{} level {}", theBoard->mCurrentWave + 1, theBoard->mNumWaves, theBoard->mLevel));
}

void CheatFinalWave(Board* theBoard) {
	theBoard->mCurrentWave = c::FinalWaveStart(theBoard->mCurrentWave, theBoard->mNumWaves);
	CheatNextWave(theBoard);
}

namespace {
std::string BackupName(const std::string& theSave) { return theSave + ".pvzu-test-backup"; }

// Back on the main menu (or at start-up) after a jump: the profile's real level, finished count and saved game.
void RestoreReturn(LawnApp* theApp) {
	if (!TestCheatsOn() || theApp->mPlayerInfo == nullptr)
		return;
	const c::LevelReturn r = LoadReturn();
	if (!c::RestoreFor(r, theApp->mPlayerInfo->mId))
		return;
	PlayerInfo* p = theApp->mPlayerInfo;
	Log(std::format("restore after test: level {} -> {}, finished {} -> {}", p->mLevel, r.level, p->mFinishedAdventure, r.finished));
	p->mLevel = r.level;
	p->mFinishedAdventure = r.finished;
	p->SaveDetails();
	for (const std::string& aSave : {GetSavedGameName(::GameMode::GAMEMODE_ADVENTURE, p->mId), GetLegacySavedGameName(::GameMode::GAMEMODE_ADVENTURE, p->mId)}) {
		std::error_code ec;
		fs::remove(aSave, ec); // the test level's save
		if (fs::exists(BackupName(aSave), ec))
			fs::rename(BackupName(aSave), aSave, ec);
	}
	SaveReturn(c::LevelReturn{});
}
} // namespace

void CheatGoToLevel(LawnApp* theApp, int theLevel) {
	PlayerInfo* p = theApp->mPlayerInfo;
	if (!TestCheatsOn() || p == nullptr || theLevel < 1 || theLevel > c::kLastLevel)
		return;
	const c::LevelReturn aBefore = LoadReturn();
	const bool aTesting = c::RestoreFor(aBefore, p->mId);
	if (theApp->mBoard) {
		if (!aTesting && theApp->mGameMode == ::GameMode::GAMEMODE_ADVENTURE && theApp->mGameScene == GameScenes::SCENE_PLAYING)
			theApp->mBoard->TryToSaveGame(); // the player's real adventure game, kept for later
		theApp->mBoardResult = BoardResult::BOARDRESULT_NONE;
	}
	theApp->mMusic->StopAllMusic();
	theApp->KillBoard();
	theApp->KillGameSelector();
	theApp->KillChallengeScreen();
	if (!aTesting) {
		for (const std::string& aSave : {GetSavedGameName(::GameMode::GAMEMODE_ADVENTURE, p->mId), GetLegacySavedGameName(::GameMode::GAMEMODE_ADVENTURE, p->mId)}) {
			std::error_code ec;
			if (fs::exists(aSave, ec))
				fs::rename(aSave, BackupName(aSave), ec);
		}
		SaveReturn(c::BeginJump(aBefore, p->mId, p->mLevel, p->mFinishedAdventure));
	}
	Log(std::format("go to level {} ({}) from profile level {}", theLevel, c::LevelName(theLevel), p->mLevel));
	p->mLevel = theLevel;
	theApp->PreNewGame(::GameMode::GAMEMODE_ADVENTURE, false);
}

void CheatOpenMode(LawnApp* theApp, c::PickerMode theMode) {
	if (!TestCheatsOn() || theApp->mPlayerInfo == nullptr)
		return;
	if (theApp->mBoard) {
		if (theApp->mGameScene == GameScenes::SCENE_PLAYING)
			theApp->mBoard->TryToSaveGame();
		theApp->mBoardResult = BoardResult::BOARDRESULT_NONE;
	}
	theApp->mMusic->StopAllMusic();
	theApp->KillBoard();
	theApp->KillGameSelector();
	theApp->KillChallengeScreen();
	const ChallengePage aPage = theMode == c::PickerMode::Puzzle ? ChallengePage::CHALLENGE_PAGE_PUZZLE
		: theMode == c::PickerMode::Survival ? ChallengePage::CHALLENGE_PAGE_SURVIVAL : ChallengePage::CHALLENGE_PAGE_CHALLENGE;
	Log(std::format("open mode page {}", static_cast<int>(theMode)));
	theApp->ShowChallengeScreen(aPage);
	if (theMode == c::PickerMode::ModModes)
		ModModesOpenList();
}

// ---- the cheats dialog: page 1 cheats + actions, page 2 level / mode picker ----
namespace {

void DrawPlate(Sexy::Graphics* g, const HitRect& r, bool down, bool selected, bool disabled, const std::string& label) {
	using namespace Sexy;
	const int d = down ? 1 : 0;
	g->SetColor(Color(0, 0, 0, 90));
	g->FillRect(r.x + 3, r.y + 4, r.w, r.h);
	g->SetColor(selected ? Color(40, 90, 22) : down ? Color(58, 44, 30) : Color(80, 60, 40));
	g->FillRect(r.x + d, r.y + d, r.w, r.h);
	g->SetColor(selected ? Color(56, 120, 30) : down ? Color(92, 70, 46) : Color(120, 92, 60));
	g->FillRect(r.x + d + 3, r.y + d + 3, r.w - 6, r.h - 6);
	_Font* f = FONT_DWARVENTODCRAFT18;
	PvzpDrawString(g, label, r.x + d + r.w / 2, r.y + d + (r.h + f->GetAscent()) / 2 - 2, f,
		disabled ? Color(150, 130, 100) : selected ? Color(220, 255, 170) : Color(250, 236, 196), DrawStringJustification::DS_ALIGN_CENTER);
	if (disabled) {
		g->SetColor(Color(0, 0, 0, 90));
		g->FillRect(r.x + d, r.y + d, r.w, r.h);
	}
}

void DrawTitle(Sexy::Graphics* g, const HitRect& r, const std::string& theText) {
	using namespace Sexy;
	_Font* tf = FONT_DWARVENTODCRAFT24;
	PvzpDrawString(g, theText, r.x + r.w / 2, r.y + (r.h + tf->GetAscent()) / 2, tf, Color(224, 187, 98), DrawStringJustification::DS_ALIGN_CENTER);
}

class CheatsDialog : public Sexy::Dialog {
public:
	explicit CheatsDialog(LawnApp* theApp)
		: Sexy::Dialog(nullptr, nullptr, kCheatsDialogId, true, "", "", "", Sexy::Dialog::BUTTONS_NONE), mApp(theApp) {
		mHasAlpha = true;
		Resize(0, 0, theApp->mWidth, theApp->mHeight);
		const PlayLayout& pl = CurrentLayout();
		mPage = c::LayoutCheatsPage(mWidth, mHeight, pl.safeLeft, pl.safeRight, static_cast<int>(c::CheatTable().size()));
		mPicker = c::LayoutLevelPicker(mWidth, mHeight, pl.safeLeft, pl.safeRight);
		mInLevel = CheatInLevel(theApp);
		mWorld = c::WorldOfLevel(theApp->mPlayerInfo ? theApp->mPlayerInfo->mLevel : 1);
		mWorld = std::clamp(mWorld, 1, c::kPickerWorlds);
	}

	void Draw(Sexy::Graphics* g) override {
		using namespace Sexy;
		g->SetColor(Color(0, 0, 0, 140));
		g->FillRect(0, 0, mWidth, mHeight);
		if (mShowPicker)
			DrawPicker(g);
		else
			DrawCheats(g);
	}

	void MouseDown(int x, int y, int theBtnNum, int) override {
		if (theBtnNum != 0)
			return;
		mDownList = -1;
		mDownIndex = -1;
		const std::vector<const std::vector<HitRect>*> aLists = Lists();
		for (size_t l = 0; l < aLists.size(); l++)
			if (const int i = c::HitIndex(*aLists[l], x, y); i >= 0) {
				mDownList = static_cast<int>(l);
				mDownIndex = i;
			}
		if (mDownList < 0 && !Inside(mShowPicker ? mPicker.panel : mPage.panel, x, y))
			mDownList = kOutside;
		MarkDirty();
	}
	void MouseDrag(int, int) override {}
	void MouseUp(int x, int y, int theBtnNum, int) override {
		const int aList = mDownList, aIndex = mDownIndex;
		mDownList = -1;
		mDownIndex = -1;
		MarkDirty();
		if (theBtnNum != 0 || aList < 0)
			return;
		if (aList == kOutside) {
			if (!Inside(mShowPicker ? mPicker.panel : mPage.panel, x, y))
				Close();
			return;
		}
		const auto aLists = Lists();
		if (c::HitIndex(*aLists[aList], x, y) != aIndex)
			return; // slid off
		mApp->PlaySample(Sexy::SOUND_TAP);
		if (mShowPicker)
			PickerTap(aList, aIndex);
		else
			CheatsTap(aList, aIndex);
	}

private:
	static constexpr int kOutside = 100;

	// the tappable lists of the shown page, in a fixed order (the index of a list is what MouseDown remembers)
	std::vector<const std::vector<HitRect>*> Lists() {
		mSingles.clear();
		if (mShowPicker) {
			mSingles = {mPicker.back};
			return {&mPicker.worlds, &mPicker.levels, &mPicker.modes, &mSingles};
		}
		mSingles = {mPage.back, mPage.done};
		return {&mPage.rows, &mPage.actions, &mSingles};
	}

	bool ActionEnabled(int theAction) const { return theAction == static_cast<int>(c::CheatAction::GoToLevel) || mInLevel; }

	void DrawCheats(Sexy::Graphics* g) {
		PanelDrawFrame(g, mPage.panel.x, mPage.panel.y, mPage.panel.w, mPage.panel.h);
		DrawTitle(g, mPage.title, Text(mApp, "PVZU_CHEAT_TITLE", "Test cheats"));
		const c::Cheats& o = CurrentCheats();
		const auto& t = c::CheatTable();
		for (size_t i = 0; i < t.size(); i++) {
			const int v = o.Get(t[i].id);
			PanelDrawRow(g, mPage.rows[i], mDownList == 0 && mDownIndex == static_cast<int>(i), Text(mApp, t[i].labelKey, t[i].label),
				c::CheatValueLabel(t[i].id, v), v != 0);
		}
		static const char* const kActionKeys[] = {"PVZU_CHEAT_KILL", "PVZU_CHEAT_NEXT_WAVE", "PVZU_CHEAT_FINAL_WAVE", "PVZU_CHEAT_GO"};
		static const char* const kActions[] = {"Kill zombies", "Next wave", "Final wave", "Go to level"};
		for (int i = 0; i < c::kCheatActions; i++)
			DrawPlate(g, mPage.actions[i], mDownList == 1 && mDownIndex == i, false, !ActionEnabled(i), Text(mApp, kActionKeys[i], kActions[i]));
		PanelDrawStone(g, mPage.back, mDownList == 2 && mDownIndex == 0, Text(mApp, "PVZU_CHEAT_OPTIONS", "Options"));
		PanelDrawStone(g, mPage.done, mDownList == 2 && mDownIndex == 1, Text(mApp, "PVZU_OPT_DONE", "Done"));
	}

	void DrawPicker(Sexy::Graphics* g) {
		PanelDrawFrame(g, mPicker.panel.x, mPicker.panel.y, mPicker.panel.w, mPicker.panel.h);
		DrawTitle(g, mPicker.title, Text(mApp, "PVZU_CHEAT_PICK", "Go to a level or a mode"));
		for (int w = 1; w <= c::kPickerWorlds; w++)
			DrawPlate(g, mPicker.worlds[w - 1], mDownList == 0 && mDownIndex == w - 1, w == mWorld, false,
				w <= 7 ? std::to_string(w) : std::string("71+"));
		for (int i = 0; i < static_cast<int>(mPicker.levels.size()); i++) {
			const int aLevel = c::LevelFor(mWorld, i);
			if (aLevel > 0)
				DrawPlate(g, mPicker.levels[i], mDownList == 1 && mDownIndex == i, false, false, c::LevelName(aLevel));
		}
		static const char* const kModeKeys[] = {"PVZU_CHEAT_MINIGAMES", "PVZU_CHEAT_PUZZLE", "PVZU_CHEAT_SURVIVAL", "PVZU_CHEAT_MOD_MODES"};
		static const char* const kModes[] = {"Mini-games", "Puzzle", "Survival", "Unnamed modes"};
		for (int i = 0; i < c::kPickerModes; i++)
			DrawPlate(g, mPicker.modes[i], mDownList == 2 && mDownIndex == i, false, false, Text(mApp, kModeKeys[i], kModes[i]));
		PanelDrawStone(g, mPicker.back, mDownList == 3 && mDownIndex == 0, Text(mApp, "PVZU_CHEAT_BACK", "Back"));
	}

	void CheatsTap(int theList, int theIndex) {
		if (theList == 0) {
			const c::CheatInfo& i = c::CheatTable()[theIndex];
			const int v = gCheats.Cycle(i.id);
			Save();
			Log(std::format("cheat {}={}", i.key, v));
			return;
		}
		if (theList == 1) {
			if (!ActionEnabled(theIndex))
				return;
			LawnApp* anApp = mApp;
			switch (static_cast<c::CheatAction>(theIndex)) {
			case c::CheatAction::KillAll:
				Close();
				CheatKillAll(anApp->mBoard);
				break;
			case c::CheatAction::NextWave:
				Close();
				CheatNextWave(anApp->mBoard);
				break;
			case c::CheatAction::FinalWave:
				Close();
				CheatFinalWave(anApp->mBoard);
				break;
			case c::CheatAction::GoToLevel:
				mShowPicker = true;
				break;
			}
			return;
		}
		if (theIndex == 0) { // back to the Unnamed options
			LawnApp* anApp = mApp;
			Close();
			OpenOptionsPanel(anApp);
		} else {
			Close();
		}
	}

	void PickerTap(int theList, int theIndex) {
		LawnApp* anApp = mApp;
		if (theList == 0) {
			mWorld = theIndex + 1;
		} else if (theList == 1) {
			if (const int aLevel = c::LevelFor(mWorld, theIndex); aLevel > 0) {
				Close();
				CheatGoToLevel(anApp, aLevel);
			}
		} else if (theList == 2) {
			Close();
			CheatOpenMode(anApp, static_cast<c::PickerMode>(theIndex));
		} else {
			mShowPicker = false;
		}
	}

	void Close() { mApp->KillDialog(kCheatsDialogId); }

	LawnApp* mApp;
	c::CheatsPageLayout mPage;
	c::PickerLayout mPicker;
	std::vector<HitRect> mSingles;
	bool mInLevel = false;
	bool mShowPicker = false;
	int mWorld = 1;
	int mDownList = -1;
	int mDownIndex = -1;
};

// "Test cheats" on the main menu (a top-level widget over the GameSelector; hidden while the menu slides away).
class MenuButton : public Sexy::Widget {
public:
	explicit MenuButton(LawnApp* theApp) : mApp(theApp) {
		const HitRect r = c::MainMenuCheatsButton(CenteredX());
		Resize(r.x, r.y, r.w, r.h);
		mRestX = theApp->mGameSelector ? theApp->mGameSelector->mX : 0;
	}
	void Update() override {
		Widget::Update();
		GameSelector* s = mApp->mGameSelector.get();
		const bool aShow = s && s->mX == mRestX && s->mY == 0 && !s->mStartingGame && mApp->mGameScene == GameScenes::SCENE_MENU;
		if (aShow != mVisible) {
			SetVisible(aShow);
			mMouseVisible = aShow;
		}
	}
	void Draw(Sexy::Graphics* g) override { PanelDrawStone(g, HitRect{0, 0, mWidth, mHeight}, mDown, Text(mApp, "PVZU_CHEAT_TITLE", "Test cheats")); }
	void MouseDown(int, int, int theBtnNum, int) override {
		mDown = theBtnNum == 0;
		MarkDirty();
	}
	void MouseUp(int x, int y, int theBtnNum, int) override {
		const bool aDown = mDown;
		mDown = false;
		MarkDirty();
		if (theBtnNum == 0 && aDown && x >= 0 && y >= 0 && x < mWidth && y < mHeight) {
			mApp->PlaySample(Sexy::SOUND_TAP);
			OpenTestCheats(mApp);
		}
	}

private:
	LawnApp* mApp;
	int mRestX = 0;
	bool mDown = false;
};

MenuButton* gMenuButton = nullptr;

// 2026-11-25 "Cheats" on the play screen (cheat gameButton; rules Mod/TestCheats.h GameButtonShown / GameButtonRect):
// a top-level widget over the board, made once when the toggle is first seen on in a level. Every update it decides
// whether it shows and where (left of Menu, Menu's twin: its top, height and art), and comes to the front of a new board. A tap
// opens the cheats page, a modal dialog, so the board pauses as with Menu (LawnApp::ModalOpen) and the changed
// cheats apply from the next board update after the page closes.
class GameCheatsButton : public Sexy::Widget {
public:
	explicit GameCheatsButton(LawnApp* theApp) : mApp(theApp) {
		SetVisible(false);
		mMouseVisible = false;
		mClip = false;
	}
	void Update() override {
		Widget::Update();
		LawnApp* app = mApp;
		Board* b = app->mBoard;
		c::GameButtonState s;
		s.feature = TestCheatsOn();
		s.toggle = CurrentCheats().On(c::CheatId::GameButton);
		s.inLevel = CheatInLevel(app);
		s.dialogOpen = app->GetDialogCount() > 0;
		s.paused = b && b->mPaused;
		s.daveTalking = app->mCrazyDaveState != CrazyDaveState::CRAZY_DAVE_OFF;
		s.gardenOrTree = app->mGameMode == ::GameMode::GAMEMODE_CHALLENGE_ZEN_GARDEN || app->mGameMode == ::GameMode::GAMEMODE_TREE_OF_WISDOM;
		const bool aShow = c::GameButtonShown(s) && b->mMenuButton;
		if (aShow) {
			const HitRect aMenu{b->mX + b->mMenuButton->mX, b->mY + b->mMenuButton->mY, b->mMenuButton->mWidth, b->mMenuButton->mHeight};
			const Sexy::Rect aShovel = b->GetShovelButtonRect(); // board coordinates (HUD top bar or vanilla bank)
			int aBarRight = b->mX + aShovel.mX + aShovel.mWidth;
			if (b->mSeedBank)
				aBarRight = std::max(aBarRight, b->mX + b->mSeedBank->mX + b->mSeedBank->mWidth);
			if (SeedPanelActive(b)) {
				const HitRect aSpeed = TopBarRect(HudBox::Speed);
				aBarRight = std::max(aBarRight, b->mX + UiX(b, 0, Anchor::Left) + aSpeed.x + aSpeed.w);
			}
			const c::GameButtonPlace p = c::GameButtonRect(aMenu, aBarRight);
			if (p.hit.x != mX || p.hit.y != mY || p.hit.w != mWidth || p.hit.h != mHeight)
				Resize(p.hit.x, p.hit.y, p.hit.w, p.hit.h);
			mDraw = {p.draw.x - p.hit.x, p.draw.y - p.hit.y, p.draw.w, p.draw.h};
			if (!mVisible || b != mBoard)
				app->mWidgetManager->BringToFront(this); // a new board is added over it
			mBoard = b;
		} else {
			mDown = false;
		}
		if (aShow != mVisible) {
			SetVisible(aShow);
			mMouseVisible = aShow;
			MarkDirty();
		}
	}
	// 2026-11-26: "not aligned with the menu one": Menu's own stone draw (GameButton DrawStoneButton) on Menu's
	// rect beside it, so the art, font and label height are Menu's. mClip off: the stone starts above the screen top
	// like Menu's (y -10) and the tap area is not the drawn rect.
	void Draw(Sexy::Graphics* g) override {
		DrawStoneButton(g, mDraw.x, mDraw.y, mDraw.w, mDraw.h, mDown && mIsOver, mIsOver, Text(mApp, "PVZU_CHEAT_BUTTON", "Cheats"));
	}
	void MouseDown(int, int, int theBtnNum, int) override {
		mDown = theBtnNum == 0;
		MarkDirty();
	}
	void MouseUp(int x, int y, int theBtnNum, int) override {
		const bool aDown = mDown;
		mDown = false;
		MarkDirty();
		if (theBtnNum != 0 || !aDown || x < 0 || y < 0 || x >= mWidth || y >= mHeight || !mVisible)
			return;
		if (mApp->mBoard)
			mApp->mBoard->ClearCursor(); // a seed or the shovel in hand is put back, as Menu does
		mApp->PlaySample(Sexy::SOUND_PAUSE);
		Log("in-game button");
		OpenTestCheats(mApp);
	}

private:
	LawnApp* mApp;
	Board* mBoard = nullptr;
	HitRect mDraw;
	bool mDown = false;
};

GameCheatsButton* gGameButton = nullptr;

void EnsureGameButton(LawnApp* theApp) {
	if (gGameButton != nullptr || !theApp->mWidgetManager)
		return;
	gGameButton = new GameCheatsButton(theApp);
	theApp->mWidgetManager->AddWidget(gGameButton);
}

} // namespace

void OpenTestCheats(LawnApp* theApp) {
	if (!TestCheatsOn() || theApp->GetDialog(kCheatsDialogId))
		return;
	CurrentCheats();
	CheatsDialog* aDialog = new CheatsDialog(theApp);
	theApp->AddDialog(kCheatsDialogId, aDialog);
	theApp->mWidgetManager->SetFocus(aDialog);
	Log(std::format("page open (scene {}{})", static_cast<int>(theApp->mGameScene),
		theApp->mBoard ? std::format(", board paused {}", theApp->mBoard->mPaused ? 1 : 0) : std::string()));
}

void TestCheatsMainMenuShown(LawnApp* theApp) {
	if (!TestCheatsOn())
		return;
	RestoreReturn(theApp);
	TestCheatsMainMenuGone(theApp);
	gMenuButton = new MenuButton(theApp);
	theApp->mWidgetManager->AddWidget(gMenuButton);
	theApp->mWidgetManager->BringToFront(gMenuButton);
}

void TestCheatsMainMenuGone(LawnApp* theApp) {
	if (gMenuButton == nullptr)
		return;
	theApp->mWidgetManager->RemoveWidget(gMenuButton);
	theApp->SafeDeleteWidget(gMenuButton);
	gMenuButton = nullptr;
}

// ---- the bot ----
namespace {

struct Bot {
	bool picked = false;
	bool awardLogged = false;
	bool lostLogged = false; // PVZU Plan 9 C2
	int awardFrame = 0;
	int nextStatus = 0;
	int nextDave = 0;
	int nextVase = 0; // PVZU Plan 9 C2
	c::BotStallWatch stall; // C2
	int stallKills = 0;
	int guardKills = 0;
	int plantings = 0;
	int lastLevel = -1;
};
Bot gBot;
constexpr int kStallFrames = 6000; // C2: app frames (60 s at 1x)

c::BotLawn LawnOf(Board* b) {
	// C2: the greenhouse of 7-1 / 7-2 / 0x7f is planted like the roof since V2 (pots first) without StageHasRoof
	if (b->StageHasRoof() || b->CanPlantAt(4, 2, SeedType::SEED_PEASHOOTER) == PlantingReason::PLANTING_NEEDS_POT)
		return c::BotLawn::Roof;
	if (b->StageHasFog())
		return c::BotLawn::Fog;
	if (b->StageHasPool())
		return c::BotLawn::Pool;
	return b->StageIsNight() ? c::BotLawn::Night : c::BotLawn::Day;
}

// The chooser: the bot's seeds first, then any other owned seed until the bank is full (as the debug "random" pick).
void BotPickSeeds(LawnApp* theApp, SeedChooserScreen* s) {
	Board* b = theApp->mBoard;
	std::vector<int> aWanted = c::BotChooserSeeds(LawnOf(b));
	static const int kPad[] = {SEED_CHERRYBOMB, SEED_JALAPENO, SEED_ICESHROOM, SEED_SQUASH, SEED_BLOVER, SEED_MAGNETSHROOM,
		SEED_UMBRELLA, SEED_KERNELPULT, SEED_SNOWPEA, SEED_REPEATER, SEED_THREEPEATER, SEED_SPLITPEA, SEED_STARFRUIT, SEED_GARLIC};
	for (int aSeed : kPad)
		aWanted.push_back(aSeed);
	std::string aNames;
	for (int aSeed : aWanted) {
		if (s->mSeedsInBank >= b->mSeedBank->mNumPackets)
			break;
		if (aSeed < 0 || aSeed >= 0x80 || !theApp->HasSeedType(static_cast<SeedType>(aSeed)) || s->SeedNotAllowedToPick(static_cast<SeedType>(aSeed)))
			continue;
		ChosenSeed& cs = s->mChosenSeeds[aSeed];
		if (cs.mSeedState != ChosenSeedState::SEED_IN_CHOOSER)
			continue;
		cs.mTimeStartMotion = 0;
		cs.mTimeEndMotion = 0;
		cs.mStartX = cs.mX;
		cs.mStartY = cs.mY;
		s->GetSeedPositionInBank(s->mSeedsInBank, cs.mEndX, cs.mEndY);
		cs.mSeedState = ChosenSeedState::SEED_IN_BANK;
		cs.mSeedIndexInBank = s->mSeedsInBank;
		s->mSeedsInBank++;
		aNames += std::format("{} ", aSeed);
	}
	for (int aSeed = 0; aSeed < 0x80; aSeed++)
		if (s->mChosenSeeds[aSeed].mSeedState == ChosenSeedState::SEED_IN_BANK)
			s->LandFlyingSeed(s->mChosenSeeds[aSeed]);
	Log(std::format("bot level {} lawn {} picked {} of {} slots: {}", b->mLevel, static_cast<int>(LawnOf(b)), s->mSeedsInBank,
		b->mSeedBank->mNumPackets, aNames));
	s->CloseSeedChooser();
}

int PacketWith(Board* b, int theSeed) {
	for (int i = 0; i < b->mSeedBank->mNumPackets; i++) {
		SeedPacket& p = b->mSeedBank->mSeedPackets[i];
		if (p.mPacketType == theSeed && p.CanPickUp())
			return i;
	}
	return -1;
}

bool Plant1(Board* b, int thePacket, int theCol, int theRow) {
	SeedPacket& p = b->mSeedBank->mSeedPackets[thePacket];
	const SeedType aSeed = p.mPacketType;
	p.MouseDown(0, 0, 1); // picks the packet up, as a tap
	if (b->mCursorObject->mCursorType == CursorType::CURSOR_TYPE_NORMAL)
		return false;
	b->MouseDownWithPlant(b->GridToPixelX(theCol, theRow) + 40, b->GridToPixelY(theCol, theRow) + 40, 1);
	if (b->mCursorObject->mCursorType != CursorType::CURSOR_TYPE_NORMAL)
		b->ClearCursor();
	gBot.plantings++;
	if (gBot.plantings <= 400)
		Log(std::format("bot plant {} col {} row {} (wave {}/{})", static_cast<int>(aSeed), theCol, theRow, b->mCurrentWave, b->mNumWaves));
	return true;
}

// One planting per step: bases (pots, lily pads) first, then the main plants, then pumpkins; on a conveyor level
// any packet the plan has no square for goes to the first square that takes it (from the right).
bool BotPlant(Board* b) {
	if (b->mCursorObject->mCursorType != CursorType::CURSOR_TYPE_NORMAL)
		b->ClearCursor();
	const c::BotLawn aLawn = LawnOf(b);
	for (int aPass = 0; aPass < 3; aPass++)
		for (int col = 0; col < MAX_GRID_SIZE_X; col++)
			for (int row = 0; row < MAX_GRID_SIZE_Y; row++) {
				if (b->mPlantRow[row] == PlantRowType::PLANTROW_DIRT)
					continue;
				const c::BotCellPlan plan = c::BotPlanFor(aLawn, col, b->mPlantRow[row] == PlantRowType::PLANTROW_POOL);
				const int aSeed = aPass == 0 ? plan.base : aPass == 1 ? plan.main : plan.armour;
				if (aSeed < 0 || b->CanPlantAt(col, row, static_cast<SeedType>(aSeed)) != PlantingReason::PLANTING_OK)
					continue;
				if (aPass == 1 && aSeed == c::seed::CATTAIL && b->GetTopPlantAt(col, row, PlantPriority::TOPPLANT_ONLY_NORMAL_POSITION))
					continue; // a Cattail goes on a bare lily pad only (CanPlantAt also answers OK over another plant)
				const int aPacket = PacketWith(b, aSeed);
				if (aPacket >= 0)
					return Plant1(b, aPacket, col, row);
			}
	for (int i = 0; i < b->mSeedBank->mNumPackets; i++) {
		SeedPacket& p = b->mSeedBank->mSeedPackets[i];
		if (p.mPacketType == SeedType::SEED_NONE || !p.CanPickUp())
			continue;
		bool aTried = false; // C2: a packet that is never picked up (7-5's Starfruit grows the snake) lets the next one plant
		for (int col = MAX_GRID_SIZE_X - 1; col >= 0 && !aTried; col--)
			for (int row = 0; row < MAX_GRID_SIZE_Y && !aTried; row++)
				if (b->CanPlantAt(col, row, p.mPacketType) == PlantingReason::PLANTING_OK && (b->HasConveyorBeltSeedBank() || col < 8)) {
					if (Plant1(b, i, col, row))
						return true;
					aTried = true;
				}
	}
	return false;
}

// PVZU Plan 9 C2: the keyboard levels, as a player with the keys. 6-5: the shooter goes towards the row of the zombie
// nearest the house; 7-5 / 0x89: the snake's head runs a rectangle inside the field (Mod/TestCheats.h BotSnakeTurn).
void BotKeyboardLevels(Board* b) {
	if (SnakeOn(b)) {
		auto& aArray = b->mProjectiles;
		if (aArray.mMaxUsedCount == 0 || !(aArray.DataArrayGetIDAt(0) & DATA_ARRAY_KEY_MASK))
			return;
		Projectile& aHead = aArray.DataArrayGetItemAt(0);
		static const unnamed::Steer kSteer[] = {unnamed::Steer::None, unnamed::Steer::Up, unnamed::Steer::Down, unnamed::Steer::Left,
			unnamed::Steer::Right};
		const c::BotTurn aTurn = c::BotSnakeTurn(aHead.mPosX, aHead.mPosY, aHead.mVelX, aHead.mVelY);
		if (aTurn != c::BotTurn::None)
			SnakeSteer(b, kSteer[static_cast<int>(aTurn)], "bot");
		return;
	}
	if (b->mLevel != 55 || !b->mApp->IsAdventureMode() || !GameData::Get().Feature("specialLevels"))
		return;
	Plant* aShooter = ShooterPlant(b);
	if (aShooter == nullptr)
		return;
	const Zombie* aNearest = nullptr;
	for (Zombie* z : b->mZombies)
		if (!z->IsDeadOrDying() && z->mHasHead && !z->mMindControlled && z->mPosX < 800.0f && (!aNearest || z->mPosX < aNearest->mPosX))
			aNearest = z;
	const int aStep = c::BotShooterStep(aShooter->mRow, aNearest ? aNearest->mRow : -1);
	if (aStep != 0)
		SpecialLevelKeyDown(b, aStep < 0 ? Sexy::KEYCODE_UP : Sexy::KEYCODE_DOWN);
}

} // namespace

bool CheatBotOn() { return CmdFlag("-pvzu-autoplay-cheats"); }

BotEvent CheatBotStep(LawnApp* theApp) {
	Board* b = theApp->mBoard;
	if (b == nullptr || !TestCheatsOn())
		return BotEvent::None;
	if (b->mLevel != gBot.lastLevel) {
		gBot = Bot{};
		gBot.lastLevel = b->mLevel;
	}
	if (theApp->mGameScene == GameScenes::SCENE_ZOMBIES_WON) { // PVZU Plan 9 C2: a lost run ends (and says so) at once
		if (!gBot.lostLogged) {
			gBot.lostLogged = true;
			Log(std::format("bot LOST level {} ({}): wave {}/{}, {} plantings, {} guard kills", b->mLevel, c::LevelName(b->mLevel),
				b->mCurrentWave, b->mNumWaves, gBot.plantings, gBot.guardKills));
			return BotEvent::Lost;
		}
		return BotEvent::None;
	}
	if (b->mCutScene && b->mCutScene->IsShowingCrazyDave() && theApp->mUpdateCount >= gBot.nextDave) { // his talk: tap on
		gBot.nextDave = theApp->mUpdateCount + 40;
		b->mCutScene->MouseDown(0, 0);
		return BotEvent::None;
	}
	if (b->mTutorialState == TutorialState::TUTORIAL_SHOVEL_PICKUP || b->mTutorialState == TutorialState::TUTORIAL_SHOVEL_DIG ||
		b->mTutorialState == TutorialState::TUTORIAL_SHOVEL_KEEP_DIGGING) { // PVZU Plan 9 C2: 1-5's shovel lesson, as a player
		for (Plant* p : b->mPlants)
			if (!p->mDead && p->mSeedType == SeedType::SEED_PEASHOOTER) {
				b->PickUpTool(GameObjectType::OBJECT_TYPE_SHOVEL);
				if (b->mCursorObject->mCursorType == CursorType::CURSOR_TYPE_SHOVEL) {
					Log(std::format("bot shovel lesson: dig col {} row {}", p->mPlantCol, p->mRow));
					b->MouseDownWithTool(p->mX + p->mWidth / 2, p->mY + p->mHeight / 2, 1, CursorType::CURSOR_TYPE_SHOVEL);
				}
				return BotEvent::None;
			}
	}
	if (SeedChooserScreen* s = theApp->mSeedChooserScreen.get(); s && !gBot.picked && s->mMouseVisible &&
		s->mChooseState == SeedChooserState::CHOOSE_NORMAL && s->mSeedsInFlight == 0 && theApp->GetDialogCount() == 0) {
		gBot.picked = true;
		BotPickSeeds(theApp, s);
		return BotEvent::None;
	}
	if (b->mLevelAwardSpawned) { // the last zombie dropped the award: the level is won (BOARDRESULT_WON, Zombie::DropLoot)
		if (!gBot.awardLogged) {
			gBot.awardLogged = true;
			gBot.awardFrame = theApp->mUpdateCount;
			Log(std::format("bot WON level {} ({}): award dropped at wave {}/{}, {} plantings, {} guard kills, result {}, stall kills {}", b->mLevel,
				c::LevelName(b->mLevel), b->mCurrentWave, b->mNumWaves, gBot.plantings, gBot.guardKills,
				static_cast<int>(theApp->mBoardResult), gBot.stallKills));
			return BotEvent::AwardSpawned; // the screenshot is taken before the award is picked up
		}
		if (theApp->mUpdateCount - gBot.awardFrame > 60)
			for (Coin* coin : b->mCoins)
				if (!coin->mDead && !coin->mIsBeingCollected && coin->IsLevelAward()) {
					coin->MouseDown(0, 0, 1);
					break;
				}
		return BotEvent::None;
	}
	if (theApp->mUpdateCount >= gBot.nextStatus && theApp->mGameScene == GameScenes::SCENE_PLAYING) {
		gBot.nextStatus = theApp->mUpdateCount + 1500;
		int aZombies = 0, aPlants = 0, aTopHp = 0, aTopType = -1;
		for (Zombie* z : b->mZombies)
			if (!z->IsDeadOrDying()) {
				aZombies++;
				const int aHp = z->mBodyHealth + z->mHelmHealth + z->mShieldHealth;
				if (aHp > aTopHp) {
					aTopHp = aHp;
					aTopType = static_cast<int>(z->mZombieType);
				}
			}
		for (Plant* p : b->mPlants)
			aPlants += p->mDead ? 0 : 1;
		std::string aWhere; // PVZU Plan 9 C2: where the live zombies are (a stuck level shows why)
		int aListed = 0;
		for (Zombie* z : b->mZombies)
			if (!z->IsDeadOrDying() && aListed++ < 10)
				aWhere += std::format(" [t{} r{} x{} s{} hp{}{}]", static_cast<int>(z->mZombieType), z->mRow, static_cast<int>(z->mPosX),
					static_cast<int>(z->mZombiePhase), z->mBodyHealth + z->mHelmHealth + z->mShieldHealth, z->mIsEating ? " eating" : "");
		if (!aWhere.empty())
			Log(std::format("bot zombies{}", aWhere));
		Log(std::format("bot status level {} frame {} wave {}/{} zombies {} plants {} toughest type {} hp {} plantings {} guard kills {}",
			b->mLevel, theApp->mUpdateCount, b->mCurrentWave, b->mNumWaves, aZombies, aPlants, aTopType, aTopHp, gBot.plantings,
			gBot.guardKills));
	}
	// The bot's guard (a cheat, logged): a zombie that got past every plant (risen from a grave or dropped behind the
	// defence) is killed at the house before it can win; the boss is never touched.
	if (theApp->mGameScene == GameScenes::SCENE_PLAYING)
		for (Zombie* z : b->mZombies)
			if (!z->IsDeadOrDying() && !z->mMindControlled && z->mZombieType != ZombieType::ZOMBIE_BOSS && z->mHasHead) {
				// C2: also a zombie chewing an invincible plant out of every plant's reach (it would never move on)
				const bool aStuck = c::BotStuckEater(z->mIsEating, z->mZombiePhase == ZombiePhase::PHASE_DIGGER_TUNNELING,
					z->mZombiePhase == ZombiePhase::PHASE_BALLOON_FLYING, CheatOn(c::CheatId::Invincible));
				if (z->mX >= 0 && !aStuck)
					continue;
				gBot.guardKills++;
				Log(std::format("bot guard kill type {} row {} x {}{} (wave {}/{}, kills {})", static_cast<int>(z->mZombieType), z->mRow,
					z->mX, aStuck ? " stuck eater" : "", b->mCurrentWave, b->mNumWaves, gBot.guardKills));
				z->DieWithLoot();
			}
	if (theApp->mGameScene == GameScenes::SCENE_PLAYING && !b->mPaused) { // C2: a stall (nothing hurt for a long time) ends
		int aCount = 0, aHp = 0;
		for (Zombie* z : b->mZombies)
			if (!z->IsDeadOrDying() && !z->mMindControlled && z->mZombieType != ZombieType::ZOMBIE_BOSS) {
				aCount++;
				aHp += z->mBodyHealth + z->mHelmHealth + z->mShieldHealth + z->mFlyingHealth;
			}
		if (gBot.stall.Update(theApp->mUpdateCount, aCount, aHp, kStallFrames)) {
			gBot.stallKills += aCount;
			Log(std::format("bot STALL: {} zombies unhurt for {} frames (wave {}/{}): kill all (stall kills {})", aCount, kStallFrames,
				b->mCurrentWave, b->mNumWaves, gBot.stallKills));
			CheatKillAll(b);
		}
	}
	if (theApp->mGameScene == GameScenes::SCENE_PLAYING && !b->mPaused && theApp->GetDialogCount() == 0)
		BotKeyboardLevels(b); // PVZU Plan 9 C2: 6-5's shooter, 7-5 / 0x89's star snake
	if (theApp->mGameScene == GameScenes::SCENE_PLAYING && !b->mPaused && theApp->GetDialogCount() == 0 && theApp->IsScaryPotterLevel() &&
		b->mChallenge->mChallengeState != ChallengeState::STATECHALLENGE_SCARY_POTTER_MALLETING &&
		theApp->mUpdateCount >= gBot.nextVase) { // PVZU Plan 9 C2: Vasebreaker (4-5, vase modes): break the vases, right to left
		GridItem* aPot = nullptr;
		for (GridItem* g : b->mGridItems)
			if (!g->mDead && g->mGridItemType == GridItemType::GRIDITEM_SCARY_POT && (!aPot || g->mGridX > aPot->mGridX))
				aPot = g;
		if (aPot) {
			gBot.nextVase = theApp->mUpdateCount + 40;
			Log(std::format("bot vase col {} row {}", aPot->mGridX, aPot->mGridY));
			b->mChallenge->ScaryPotterMalletPot(aPot);
			return BotEvent::None;
		}
	}
	if (theApp->mGameScene == GameScenes::SCENE_PLAYING && !b->mPaused && theApp->GetDialogCount() == 0 && !BotPlant(b) &&
		theApp->mGameMode == ::GameMode::GAMEMODE_CHALLENGE_LAST_STAND && b->mStoreButton && !b->mStoreButton->mDisabled &&
		b->mChallenge->mChallengeState != ChallengeState::STATECHALLENGE_LAST_STAND_ONSLAUGHT) {
		// PVZU Plan 9 C2: Last Stand, nothing left to plant: "Start onslaught" (what Board::MouseUp does for the button)
		Log(std::format("bot last stand: start onslaught (round {})", b->mChallenge->mSurvivalStage));
		b->mChallenge->mChallengeState = ChallengeState::STATECHALLENGE_LAST_STAND_ONSLAUGHT;
		b->mStoreButton->mBtnNoDraw = true;
		b->mStoreButton->mDisabled = true;
		b->mZombieCountDown = 10;
		b->mZombieCountDownStart = 10;
	}
	return BotEvent::None;
}

} // namespace pvzu::engine
