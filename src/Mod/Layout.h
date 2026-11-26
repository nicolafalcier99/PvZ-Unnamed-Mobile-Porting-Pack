// PVZU: wide-screen layout math. Logical height is always 600; logical width follows the screen's
// aspect ratio (800..1400). The lawn rules stay 800 wide: extra width only shows more background
// (backgrounds are 1400x600; vanilla shows x 220..1020 of them).
#pragma once

namespace pvzu {

constexpr int kLogicalHeight = 600;
constexpr int kVanillaWidth = 800;
constexpr int kMaxWidth = 1400;          // background image width
constexpr int kBackgroundLeftSpare = 220; // background pixels left of the vanilla view (BOARD_OFFSET)

// clamp(round(600 * screenW / screenH), 800, 1400); 800 for invalid sizes.
int ComputeLogicalWidth(int screenW, int screenH);

struct PlayLayout {
	int width = kVanillaWidth;  // logical screen width
	int boardScreenX = 0;       // screen x where vanilla's x=0 of the play view now is
	int leftExtra = 0;          // extra px left of the vanilla view (house)
	int rightExtra = 0;         // extra px shown right of the vanilla view (street, under the haze strip)
	int houseExtra = 0;         // background shown left of the vanilla view (up to 220)
	int safeLeft = 0;           // phone safe-area insets (camera hole, rounded corners), logical px;
	int safeRight = 0;          // they move Left/Right-anchored UI only, never the lawn or the background
};

// 2026-11-23 (mobile-PvZ style HUD, Mod/Hud.h): no side panel; the seed cards float over the house. Extra width
// shows up to 220 px of house on the left (all the background has: its left edge then sits on the screen's edge,
// so nothing black ever shows), the rest is street on the right. Width is clamped to 800..1400 first.
PlayLayout ComputePlayLayout(int logicalWidth);

// Board x (vanilla play coordinates) at the right edge of the screen.
int VisibleRightBoardX(const PlayLayout& layout);

enum class Anchor {
	Left,    // keeps its distance from the left screen edge
	Right,   // keeps its distance from the right screen edge
	Center,  // keeps its offset from the screen centre
	Board,   // moves with the lawn
};

// Screen x for an item that sits at vanillaX on an 800-wide screen.
int AnchorX(int vanillaX, Anchor anchor, const PlayLayout& layout);

// Board widget x positions (the camera) for the level-start pan: at rest during play, scrolled left to the
// house, scrolled right to the street. Vanilla (800): 0, 220, -380.
struct BoardCamera {
	int rest = 0;
	int house = 0;
	int street = 0;
};
BoardCamera ComputeBoardCamera(const PlayLayout& layout);

// Board-local x for something drawn by the board at vanillaX on an 800-wide screen, when the board rests at
// layout.boardScreenX. Left/Right/Center items keep their screen edge; Board items do not move.
// Smooth (linear) texture filtering when the screen enlarges the game by an uneven factor (e.g. 1.8x on the
// S26 FHD+): nearest-neighbour then doubles some pixels and not others. Exact integer scales stay nearest.
bool ShouldSmoothUpscale(int viewportH, int logicalH);

// Seed chooser ("Choose your plants"): the whole screen moves right of the left safe inset; buttons that vanilla
// puts at the right edge get chooser-local x that keeps them inside the right safe inset.
int ChooserScreenX(const PlayLayout& layout);
int ChooserRightButtonX(int vanillaX, const PlayLayout& layout);
// Chooser-local x of a screen (widget manager) mouse x, for its hover hit tests (tooltip, hand cursor).
int ChooserMouseX(int screenX, const PlayLayout& layout);

// Screen x where the board draws Crazy Dave's vanilla screen x 0: the wide screen's left edge on a lawn board, the
// centred 800 screen's left edge on an 800-wide board (Zen Garden, Tree of Wisdom).
int CrazyDaveScreenX(bool wideBoard, const PlayLayout& layout);

// Board x at the physical right screen edge (ignores safe insets: backgrounds and haze go edge to edge).
int ScreenRightBoardX(const PlayLayout& layout);

int BoardLocalX(int vanillaX, Anchor anchor, const PlayLayout& layout);

// Draw-only shift for a collected coin flying to a counter that moved by fullShift: 0 where it was clicked,
// fullShift when it arrives (startDist/curDist: distance to the vanilla counter then and now).
float CollectDrawShift(float fullShift, float startDist, float curDist);

// The layout in use (set at start-up and on resize; vanilla until then).
const PlayLayout& CurrentLayout();
void SetCurrentLayout(const PlayLayout& layout);

} // namespace pvzu
