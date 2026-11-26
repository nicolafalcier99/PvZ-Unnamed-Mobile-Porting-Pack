// PVZU: board zoom during play, like the EA mobile PvZ (2026-11-23): the lawn scene (background, plants,
// zombies, projectiles, coins, particles) is drawn larger; the HUD (cards, top bar, menu, coin bank, level label) and
// every 800 x 600 screen stay at their logical size. Pure presentation: board coordinates, rules and spawns never
// change; the engine draws the board scene through the view below and maps touches back through its inverse.
//
// Why not 1.2 (measured 2026-11-23 on the 1300 x 600 S26 layout, PC art):
//  - the 1400 px background holds only 1020 px from the house's left edge (board x -220) to the end of the sidewalk
//    strip at board x 800; a 1300 px screen at zoom z shows 1300 / z board px, so "no black on the left" and "no
//    street on the right" both hold only for z >= 1.275 (1.232 if the sidewalk to board x 835 may show);
//  - 5 rows (board y 80..580) fit 600 px only for z <= 1.2, and at 1.2 the top row sits under the HUD top bar
//    (screen y 4..56). The 2026-11-23 review wants the bar clear of the lawn's first row, grass edge included (the art's
//    grass starts ~20 px above the first tile): with a 2 px gap that is z ~1.042 on day lawns (was 1.126, which let
//    the bar cover the top row's plant heads by up to 10 px).
// So the rest zoom is the largest that keeps every row on the screen and the lawn's first row clear of the top bar;
// the background's left edge stays on the screen's left edge (no black) and the narrow street strip that remains on
// the right keeps the street haze (Mod/StreetFog.h), which hides the spawn line as before.
#pragma once

#include "Mod/Layout.h"
#include "Mod/TouchPlanting.h"

namespace pvzu {

// Logical screen point of board point (bx, by): (x + scale * bx, y + scale * by). "Board point" means the board's
// own coordinates (vanilla's: lawn tile (0,0) at 40,80); x/y are where board point (0,0) lands on the screen.
struct ZoomView {
	float scale = 1.0f;
	float x = 0.0f;
	float y = 0.0f;
};

float ZoomScreenX(const ZoomView& v, float boardX);
float ZoomScreenY(const ZoomView& v, float boardY);
float ZoomBoardX(const ZoomView& v, float screenX);
float ZoomBoardY(const ZoomView& v, float screenY);

constexpr float kMaxBoardZoom = 1.25f;
constexpr int kTopBarLawnGap = 2;     // 2026-11-23: the top bar must not touch the lawn's first row (screen px)
constexpr int kPlantHeadInset = 8;    // measured: a top-row Peashooter starts 8 px below its row's top (y 88 at row 80)

// Board y of the lawn's first row's top (roof: its highest, flat part), where the top row's plants start, and where
// the lowest row ends (roof: the slope's low left end).
int LawnTop(LawnGrid grid);
int TopRowPlantTop(LawnGrid grid);
int LawnBottom(LawnGrid grid);

// The zoomed view at rest during play, from the logical size (layout.width x kLogicalHeight) and the safe insets.
// Unzoomed ({1, layout.boardScreenX, 0}) where the HUD does not fit (width 800 included).
ZoomView ComputeRestZoom(const PlayLayout& layout, LawnGrid grid);

// The board without zoom at camera position (boardX, boardY) (the board widget's mX/mY).
ZoomView UnzoomedView(int boardX, int boardY);
// a -> b at t in 0..1 (t already eased).
ZoomView BlendZoom(const ZoomView& a, const ZoomView& b, float t);
// Smoothstep of a linear progress 0..1.
float ZoomEase(float progress);

// Zoomed throughout the level-start pan (option B): the rest view moved so that the camera's house stop shows the
// background's left edge on the screen's left edge and its street stop the background's right edge (board x 1180)
// on the screen's right edge; linear in between. cameraX is the board widget's x on that pan.
ZoomView ZoomedPanView(const ZoomView& rest, const BoardCamera& camera, int cameraX, int width);

} // namespace pvzu
