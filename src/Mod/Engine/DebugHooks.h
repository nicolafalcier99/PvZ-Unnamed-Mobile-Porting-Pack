// PVZU: desktop-only scripted checks, driven by -pvzu-* options. None of this runs unless an option is given.
//   -pvzu-shot <file.png> -pvzu-shot-frame <list>  write the framebuffer at those update frames, keep running
//   -pvzu-quit-frame <N>                           quit cleanly at update frame N (a recording is closed properly)
//   -pvzu-input <script>                           inject mouse/keys at given frames (see Mod/DebugRun.h)
//   -pvzu-log-zombies <file>                       log each zombie's spawn and the frame it crosses board x = 800
// Scripted runs ignore window focus changes so the game never pauses because another app is in front.
#pragma once

namespace Sexy { class SexyAppBase; }
union SDL_Event;

namespace pvzu::engine {

// True when any scripted-check option was given (focus changes are then ignored).
bool DebugRunActive();
// In a scripted run: drop real mouse/keyboard input and focus/enter/leave events.
bool DebugRunIgnoresEvent(const SDL_Event& theEvent);
// Marker on the mouse/key events the script pushes.
constexpr unsigned kScriptedInputId = 0x7076;
// data1 of the window leave Input.cpp sends after a finger lift (it passes the filter above).
constexpr unsigned kTouchLiftLeave = 0x7475;
// Once per update, after the update counter advanced.
void DebugRunUpdate(Sexy::SexyAppBase* theApp);
// After a frame is drawn, before it is presented. viewport: the drawable area in pixels.
void DebugRunAfterDraw(Sexy::SexyAppBase* theApp, int vx, int vy, int vw, int vh);
// Zombie position log (no-op unless -pvzu-log-zombies).
void DebugLogZombie(const void* theZombie, int theType, int theRow, int theFromWave, float thePosX, int theAge);

} // namespace pvzu::engine
