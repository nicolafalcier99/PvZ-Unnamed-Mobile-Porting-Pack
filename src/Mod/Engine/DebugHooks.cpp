// PVZU: see DebugHooks.h. Desktop testing aid only; inert without -pvzu-* check options.
#include "Mod/Engine/DebugHooks.h"

#include "Mod/CmdLine.h"
#include "Mod/DebugRun.h"
#include "Mod/Engine/HdTextures.h"
#include "Mod/Engine/TestCheats.h" // PVZU Plan 9 C1: -pvzu-autoplay-cheats

#include "LawnApp.h"
#include "Lawn/Board.h"
#include "Lawn/Coin.h"
#include "Lawn/SeedPacket.h"
#include "SexyAppBase.h"
#include "graphics/GLPlatform.h"
#include "imagelib/ImageLib.h"
#include "widget/WidgetManager.h"

#include <SDL3/SDL.h>

#include <cstdio>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace pvzu::engine {

namespace {

struct State {
	bool parsed = false;
	bool active = false;
	std::string shotBase;
	std::vector<int> shotFrames;
	size_t nextShot = 0;
	int quitFrame = -1;
	bool quitDone = false;
	std::vector<InputEvent> input;
	size_t nextInput = 0;
	std::string zombieLogPath;
	FILE* zombieLog = nullptr;
	std::map<const void*, float> zombieLastX;
	std::vector<std::string> textStore; // SDL text events point here
	bool focusMarked = false;
	int autoFrom = -1, autoTo = -1;     // -pvzu-autoplay <from>-<to>
	int autoNextFrame = 0;
	int autoPendingCol = -1, autoPendingRow = -1;
	bool autoCheats = false;            // PVZU Plan 9 C1: -pvzu-autoplay-cheats (Mod/Engine/TestCheats.h)
	std::string winShot;                // -pvzu-autoplay-win-shot <png>: when the level's award drops
	int winShotFrame = -1;
	bool quitOnWin = false;             // -pvzu-autoplay-quit-won: quit 100 frames after the win
	int winQuitFrame = -1;
};

State& S() {
	static State s;
	return s;
}

void Parse() {
	State& s = S();
	if (s.parsed)
		return;
	s.parsed = true;
#if defined(__ANDROID__) || defined(__IPHONEOS__) || defined(__EMSCRIPTEN__) || defined(__SWITCH__)
	return; // desktop only
#else
	if (auto shot = CmdOption("-pvzu-shot")) {
		s.shotBase = *shot;
		s.shotFrames = ParseFrameList(CmdOption("-pvzu-shot-frame").value_or("0"));
	}
	s.quitFrame = CmdOptionInt("-pvzu-quit-frame", -1);
	if (auto script = CmdOption("-pvzu-input")) {
		std::ifstream in(*script);
		std::stringstream buf;
		buf << in.rdbuf();
		std::string err;
		s.input = ParseInputScript(buf.str(), &err);
		if (!err.empty())
			std::fprintf(stderr, "PVZU input: %s\n", err.c_str());
		if (!in)
			std::fprintf(stderr, "PVZU input: cannot read %s\n", script->c_str());
	}
	if (auto log = CmdOption("-pvzu-log-crossings")) { // PVZU merge: renamed (EliteZombies owns -pvzu-log-zombies)
		s.zombieLogPath = *log;
		s.zombieLog = std::fopen(log->c_str(), "w");
	}
	if (auto autoplay = CmdOption("-pvzu-autoplay")) {
		std::vector<int> range = ParseFrameList(*autoplay + "/1000000"); // "<from>-<to>" -> {from}
		size_t dash = autoplay->find('-');
		if (!range.empty() && dash != std::string::npos) {
			s.autoFrom = range.front();
			s.autoTo = std::atoi(autoplay->c_str() + dash + 1);
		}
	}
	if (CmdFlag("-pvzu-autoplay-cheats")) { // PVZU Plan 9 C1
		s.autoCheats = true;
		if (s.autoFrom < 0) {
			s.autoFrom = 0;
			s.autoTo = 1 << 30;
		}
		s.winShot = CmdOption("-pvzu-autoplay-win-shot").value_or("");
		s.quitOnWin = CmdFlag("-pvzu-autoplay-quit-won");
	}
	s.active = s.autoFrom >= 0 || !s.shotBase.empty() || s.quitFrame >= 0 || !s.input.empty() || s.zombieLog != nullptr;
#endif
}

// Logical point -> window point that RemapMouse maps back to exactly that logical point.
float ToWindow(int theLogical, int theDestPos, int theDestSize, int theSrcPos, int theSrcSize) {
	if (theDestSize <= 0 || theSrcSize <= 0)
		return static_cast<float>(theLogical);
	int guess = (theLogical - theDestPos) * theSrcSize / theDestSize + theSrcPos;
	for (int c = guess - 3; c <= guess + 3; ++c) {
		if ((c - theSrcPos) * theDestSize / theSrcSize + theDestPos == theLogical)
			return static_cast<float>(c);
	}
	return static_cast<float>(guess);
}

void PushInput(Sexy::SexyAppBase* theApp, const InputEvent& e) {
	Sexy::WidgetManager* wm = theApp->mWidgetManager.get();
	const Sexy::Rect& dst = wm->mMouseDestRect;
	const Sexy::Rect& src = wm->mMouseSourceRect;
	float wx = ToWindow(e.x, dst.mX, dst.mWidth, src.mX, src.mWidth);
	float wy = ToWindow(e.y, dst.mY, dst.mHeight, src.mY, src.mHeight);
	SDL_WindowID aWin = SDL_GetWindowID(theApp->mWindow.get());
	SDL_Event ev{};
	switch (e.kind) {
	case InputEvent::Kind::Move:
		ev.type = SDL_EVENT_MOUSE_MOTION;
		ev.motion.windowID = aWin;
		ev.motion.which = e.touch ? SDL_TOUCH_MOUSEID : kScriptedInputId; // fmove: a dragged finger
		ev.motion.x = wx;
		ev.motion.y = wy;
		break;
	case InputEvent::Kind::Down:
	case InputEvent::Kind::Up:
		// A motion first so the widget under the point is current, like a real mouse. A "tap" comes from SDL's
		// touch mouse, as a finger on the phone does.
		ev.type = SDL_EVENT_MOUSE_MOTION;
		ev.motion.windowID = aWin;
		ev.motion.which = e.touch ? SDL_TOUCH_MOUSEID : kScriptedInputId;
		ev.motion.x = wx;
		ev.motion.y = wy;
		SDL_PushEvent(&ev);
		ev = SDL_Event{};
		ev.type = e.kind == InputEvent::Kind::Down ? SDL_EVENT_MOUSE_BUTTON_DOWN : SDL_EVENT_MOUSE_BUTTON_UP;
		ev.button.windowID = aWin;
		ev.button.which = e.touch ? SDL_TOUCH_MOUSEID : kScriptedInputId;
		ev.button.button = SDL_BUTTON_LEFT;
		ev.button.down = e.kind == InputEvent::Kind::Down;
		ev.button.clicks = 1;
		ev.button.x = wx;
		ev.button.y = wy;
		break;
	case InputEvent::Kind::Text:
		S().textStore.push_back(e.text);
		ev.type = SDL_EVENT_TEXT_INPUT;
		ev.text.windowID = aWin;
		ev.text.text = S().textStore.back().c_str();
		break;
	case InputEvent::Kind::Key: {
		SDL_Keycode key = e.text == "return" ? SDLK_RETURN : e.text == "escape" ? SDLK_ESCAPE
			: e.text == "space" ? SDLK_SPACE : e.text == "backspace" ? SDLK_BACKSPACE : SDLK_UNKNOWN;
		// PVZU Plan 7 Task 3a: arrows and single letters (6-5 Up / Down, 7-5 W / A / S / D)
		if (key == SDLK_UNKNOWN)
			key = e.text == "up" ? SDLK_UP : e.text == "down" ? SDLK_DOWN : e.text == "left" ? SDLK_LEFT
				: e.text == "right" ? SDLK_RIGHT : (e.text.size() == 1 && e.text[0] >= 'a' && e.text[0] <= 'z')
				? static_cast<SDL_Keycode>(SDLK_A + (e.text[0] - 'a')) : SDLK_UNKNOWN;
		ev.type = SDL_EVENT_KEY_DOWN;
		ev.key.windowID = aWin;
		ev.key.which = kScriptedInputId;
		ev.key.key = key;
		ev.key.down = true;
		SDL_PushEvent(&ev);
		ev.type = SDL_EVENT_KEY_UP;
		ev.key.down = false;
		break;
	}
	}
	SDL_PushEvent(&ev);
}

// A very small player for scripted recordings: collects sun and coins, plants the first ready packet in the
// first free cell (columns 0-4, middle rows first). It only clicks, like a person; the demo records the clicks.
void AutoplayStep(Sexy::SexyAppBase* theApp) {
	State& s = S();
	LawnApp* app = gLawnApp;
	if (!app || !app->mBoard || app->mGameScene != GameScenes::SCENE_PLAYING)
		return;
	Board* board = app->mBoard;
	auto click = [&](int x, int y) {
		InputEvent down{static_cast<int>(theApp->mUpdateCount), InputEvent::Kind::Down, x, y, {}};
		InputEvent up = down;
		up.kind = InputEvent::Kind::Up;
		PushInput(theApp, down);
		PushInput(theApp, up);
	};
	if (s.autoPendingCol >= 0) {
		int col = s.autoPendingCol, row = s.autoPendingRow;
		s.autoPendingCol = -1;
		click(board->mX + board->GridToPixelX(col, row) + 40, board->mY + board->GridToPixelY(col, row) + 50);
		return;
	}
	for (Coin* coin : board->mCoins) {
		if (coin->mDead || coin->mIsBeingCollected || !(coin->IsSun() || coin->IsMoney()))
			continue;
		click(board->mX + static_cast<int>(coin->mPosX) + coin->mWidth / 2, board->mY + static_cast<int>(coin->mPosY) + coin->mHeight / 2); // hit test uses mPosX/Y
		return;
	}
	SeedBank* bank = board->mSeedBank.get();
	for (int i = 0; i < bank->mNumPackets; ++i) {
		SeedPacket& packet = bank->mSeedPackets[i];
		if (!packet.CanPickUp())
			continue;
		static const int kRows[] = {2, 1, 3, 0, 4, 5};
		for (int col = 0; col < 5; ++col) {
			for (int row : kRows) {
				if (board->CanPlantAt(col, row, packet.mPacketType) != PlantingReason::PLANTING_OK)
					continue;
				click(board->mX + bank->mX + packet.mX + packet.mOffsetX + 25, board->mY + bank->mY + packet.mY + 35);
				s.autoPendingCol = col;
				s.autoPendingRow = row;
				return;
			}
		}
	}
}

} // namespace

bool DebugRunIgnoresEvent(const SDL_Event& e) {
	if (!DebugRunActive())
		return false;
	switch (e.type) {
	case SDL_EVENT_WINDOW_FOCUS_GAINED:
		return e.window.data1 != static_cast<Sint32>(kScriptedInputId); // our own "active" marker passes
	case SDL_EVENT_WINDOW_MOUSE_LEAVE:
		return e.window.data1 != static_cast<Sint32>(kTouchLiftLeave); // the leave after a finger lift passes
	case SDL_EVENT_WINDOW_FOCUS_LOST:
	case SDL_EVENT_WINDOW_MOUSE_ENTER:
	case SDL_EVENT_WINDOW_MINIMIZED:
	case SDL_EVENT_WINDOW_RESTORED:
	case SDL_EVENT_WILL_ENTER_BACKGROUND:
	case SDL_EVENT_DID_ENTER_FOREGROUND:
	case SDL_EVENT_MOUSE_WHEEL:
		return true;
	case SDL_EVENT_MOUSE_MOTION: // scripted mouse, or a scripted "tap" (the Mac has no touch screen)
		return e.motion.which != kScriptedInputId && e.motion.which != SDL_TOUCH_MOUSEID;
	case SDL_EVENT_MOUSE_BUTTON_DOWN:
	case SDL_EVENT_MOUSE_BUTTON_UP:
		return e.button.which != kScriptedInputId && e.button.which != SDL_TOUCH_MOUSEID;
	case SDL_EVENT_KEY_DOWN:
	case SDL_EVENT_KEY_UP:
		return e.key.which != kScriptedInputId;
	default:
		return false;
	}
}

bool DebugRunActive() {
	Parse();
	return S().active;
}

void DebugRunUpdate(Sexy::SexyAppBase* theApp) {
	Parse();
	State& s = S();
	if (!s.active)
		return;
	const int frame = theApp->mUpdateCount;
	if (!theApp->mPlayingDemoBuffer && !s.focusMarked) {
		// Scripted runs act as focused; say so once through SDL, so a recording carries it and any build
		// (also one without these options, whose window may not have focus) replays the same game.
		s.focusMarked = true;
		SDL_Event ev{};
		ev.type = SDL_EVENT_WINDOW_FOCUS_GAINED;
		ev.window.windowID = SDL_GetWindowID(theApp->mWindow.get());
		ev.window.data1 = static_cast<Sint32>(kScriptedInputId);
		SDL_PushEvent(&ev);
	}
	if (!theApp->mPlayingDemoBuffer) {
		while (s.nextInput < s.input.size() && s.input[s.nextInput].frame <= frame)
			PushInput(theApp, s.input[s.nextInput++]);
		if (s.autoFrom >= 0 && frame >= s.autoFrom && frame <= s.autoTo && frame >= s.autoNextFrame) {
			s.autoNextFrame = frame + (s.autoCheats ? 3 : 15);
			if (!s.autoCheats) {
				AutoplayStep(theApp);
			} else { // PVZU Plan 9 C1: the cheat bot; a screenshot when the award drops, quit after the win
				const BotEvent e = CheatBotStep(gLawnApp);
				if (e == BotEvent::AwardSpawned && !s.winShot.empty())
					s.winShotFrame = frame + 10;
				if (e == BotEvent::AwardSpawned && s.quitOnWin)
					s.winQuitFrame = frame + 300;
				if (e == BotEvent::Lost && s.quitOnWin) // PVZU Plan 9 C2: a lost run quits too
					s.winQuitFrame = frame + 300;
			}
		}
		if (s.winQuitFrame >= 0 && frame >= s.winQuitFrame && !s.quitDone) {
			s.quitDone = true;
			std::fprintf(stderr, "PVZU autoplay: quit after the end of the level (frame %d)\n", frame);
			theApp->Shutdown();
		}
	}
	if (s.quitFrame >= 0 && frame >= s.quitFrame && !s.quitDone) {
		s.quitDone = true;
		if (s.zombieLog)
			std::fflush(s.zombieLog);
		hd::LogSummary("quit"); // -pvzu-log-hd
		theApp->Shutdown();
	}
}

void DebugRunAfterDraw(Sexy::SexyAppBase* theApp, int vx, int vy, int vw, int vh) {
	State& s = S();
	if (!s.active || vw <= 0 || vh <= 0)
		return;
	const int frame = theApp->mUpdateCount;
	std::string path;
	int wanted = frame;
	if (s.winShotFrame >= 0 && frame >= s.winShotFrame) { // PVZU Plan 9 C1: -pvzu-autoplay-win-shot
		s.winShotFrame = -1;
		path = s.winShot;
	} else {
		if (s.shotBase.empty() || s.nextShot >= s.shotFrames.size() || frame < s.shotFrames[s.nextShot])
			return;
		wanted = s.shotFrames[s.nextShot];
		while (s.nextShot < s.shotFrames.size() && s.shotFrames[s.nextShot] <= frame)
			++s.nextShot;
		path = ShotFileName(s.shotBase, wanted, s.shotFrames.size() > 1);
	}

	std::vector<uint8_t> rgba(static_cast<size_t>(vw) * vh * 4);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(vx, vy, vw, vh, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
	ImageLib::Image img;
	img.mWidth = vw;
	img.mHeight = vh;
	img.mBits = std::make_unique<uint32_t[]>(static_cast<size_t>(vw) * vh);
	for (int y = 0; y < vh; ++y) {
		const uint8_t* src = rgba.data() + static_cast<size_t>(vh - 1 - y) * vw * 4; // GL rows are bottom-up
		uint32_t* dst = img.mBits.get() + static_cast<size_t>(y) * vw;
		for (int x = 0; x < vw; ++x, src += 4)
			dst[x] = 0xFF000000u | (uint32_t(src[0]) << 16) | (uint32_t(src[1]) << 8) | uint32_t(src[2]);
	}
	bool ok = ImageLib::WritePNGImage(path, &img);
	hd::LogSummary("shot"); // -pvzu-log-hd
	std::fprintf(stderr, "PVZU shot %s frame %d (asked %d) %dx%d %s\n", path.c_str(), frame, wanted, vw, vh, ok ? "ok" : "FAILED");
}

void DebugLogZombie(const void* theZombie, int theType, int theRow, int theFromWave, float thePosX, int theAge) {
	State& s = S();
	if (!s.zombieLog)
		return;
	const int frame = Sexy::gSexyAppBase ? Sexy::gSexyAppBase->mUpdateCount : 0;
	auto it = s.zombieLastX.find(theZombie);
	if (it == s.zombieLastX.end() || theAge <= 1) {
		std::fprintf(s.zombieLog, "frame %d spawn type %d row %d wave %d x %.2f\n", frame, theType, theRow, theFromWave, thePosX);
		s.zombieLastX[theZombie] = thePosX;
		return;
	}
	if (it->second >= 800.0f && thePosX < 800.0f)
		std::fprintf(s.zombieLog, "frame %d cross800 type %d row %d wave %d x %.2f\n", frame, theType, theRow, theFromWave, thePosX);
	it->second = thePosX;
}

} // namespace pvzu::engine
