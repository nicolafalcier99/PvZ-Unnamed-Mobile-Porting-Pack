// PVZU: engine side of the board zoom (Mod/BoardZoom.h). The board draws its scene items (background, plants, zombies,
// coins, particles, the held plant, the street haze) through a GL vertex transform; the HUD items and Crazy Dave are
// drawn without it. Touches on the lawn are mapped back through the inverse. Off at width 800, in demos, where the HUD
// is not used (conveyor, slot machine, Zen Garden...), and outside play: the level-start pan and the seed chooser
// show the unzoomed wide view, and the zoom eases in once the camera is back at rest (option "eased"). The dev option
// -pvzu-dev-zoom-pan zoomed keeps it zoomed through the pan instead (ZoomedPanView); -pvzu-dev-zoom <scale> forces a
// scale for comparisons.
#pragma once

#include "Mod/BoardZoom.h"

namespace Sexy { class Graphics; }
class Board;
class Coin;
class RenderItem;

namespace pvzu::engine {

// Board::Update: ease the zoom in (playing, camera at rest) or out.
void UpdateBoardZoom(Board* theBoard);
// The view the board is drawn with now: logical screen point of a board point. Unzoomed: {1, mX, mY}.
ZoomView CurrentBoardView(const Board* theBoard);
bool BoardZoomed(const Board* theBoard);

// Board-widget-local coordinates (screen minus the board's mX/mY, what Board::MouseDown gets) to board coordinates and
// back. Identity without zoom.
void LocalToBoard(const Board* theBoard, int& x, int& y);
void BoardToLocal(const Board* theBoard, int& x, int& y);
// The view with the camera at rest (no shake): for the finger in board coordinates (MouseBoardX/Y).
ZoomView RestCameraView(const Board* theBoard);

// Board::DrawGameObjects: whether this item is part of the zoomed scene (HUD layers, screen fades and the tutorial
// arrow over a seed card are not).
bool ZoomRenderItem(const Board* theBoard, const RenderItem& theItem);

// While alive, everything drawn goes through the board's current view (nothing when not zoomed or on = false).
class BoardZoomScope {
public:
	BoardZoomScope(const Board* theBoard, bool on = true);
	~BoardZoomScope();
	BoardZoomScope(const BoardZoomScope&) = delete;
	BoardZoomScope& operator=(const BoardZoomScope&) = delete;
private:
	bool mSet = false;
};

// Draw-only shift (board px) for a collected coin: it still flies to its vanilla destination (credited on the vanilla
// frame) but is drawn heading for the HUD counter's place on the screen, through the zoom.
void CoinDrawOffset(Coin* theCoin, float& theDX, float& theDY);

// Board x of the screen's right edge in the current view (the street haze is drawn up to there).
int ViewRightBoardX(const Board* theBoard);

} // namespace pvzu::engine
