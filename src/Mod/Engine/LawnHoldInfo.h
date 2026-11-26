// PVZU: engine side of the lawn's hold-for-stats panel (rules, placement and rows in Mod/LawnHoldInfo.h).
//
// A finger press with nothing held, on a plant or a zombie in play (feature "modVisuals"), starts a HoldPress
// (Mod/HoldInfo.h, the chooser's timing): after kHoldInfoDelay the panel opens above the press point and stays, still,
// until the lift, wherever the finger goes; its stats follow the target live. A plant's own tap action (Cob Cannon
// select, Prism-shroom fire, Element-shroom toggle, mod hints) waits for the lift and runs only for a quick tap that
// stayed on its target, so a hold plants, digs or selects nothing. A mouse, demo playback and recording keep
// vanilla's (and the exe's small right-button tooltip) byte for byte. Five marked engine sites:
//   Board::Update        once per frame -> LawnHoldUpdate
//   Board::MouseDown     before the hit-object switch, nothing held -> LawnHoldDown (true: skip the plant's MouseDown)
//   Board::MouseUp       after MobileMouseUp -> LawnHoldUp (a quick tap replays the plant's MouseDown)
//   Board::DrawUITop     after mToolTip -> DrawLawnHoldInfo (screen space, over the HUD's board layer)
//   ModInfoToolTip       (ModVisuals.cpp) a touch press never fills the small tooltip
//
// Mac check: "-pvzu-input" with fdown / fup on a plant or zombie; stderr prints "PVZU lawn hold: ..." per press.
#pragma once

class Board;
class HitResult;
namespace Sexy {
class Graphics;
}

namespace pvzu::engine {

// true while a touch press may open the panel (or has): the exe's small tooltip stays hidden.
bool LawnHoldTouch(Board* theBoard);
void LawnHoldUpdate(Board* theBoard);
// x, y in board coordinates (zoom applied), as Board::MouseDown's hit test.
bool LawnHoldDown(Board* theBoard, const HitResult* theHitResult, int x, int y, int theClickCount);
// x, y as Board::MouseUp gets them (board-local, before the zoom).
void LawnHoldUp(Board* theBoard, int x, int y);
void DrawLawnHoldInfo(Board* theBoard, Sexy::Graphics* g);

} // namespace pvzu::engine
