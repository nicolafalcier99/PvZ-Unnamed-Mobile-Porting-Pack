// PVZU: engine side of the wide layout (spec item 5). The logical screen is W x 600 (W = 800..1400, fixed at
// start-up). Lawn boards with 1400 px backgrounds rest at layout.boardScreenX and show more house and street;
// every other screen (and boards with 800 px art: Zen Garden, aquarium, tree) is 800 wide and centred, the sides
// filled by Mod/Engine/SideFill. At W = 800 every function here returns the vanilla value.
#pragma once

#include "Mod/Hud.h"
#include "Mod/Layout.h"
#include "misc/Rect.h"

namespace Sexy { class SexyAppBase; class Widget; class Graphics; }
class Board;
class Coin;
class SeedBank;

namespace pvzu::engine {

// Once, before the window exists (LawnApp::PreDisplayHook): picks the logical width and sets app->mWidth.
void InitLogicalWidth(Sexy::SexyAppBase* theApp);

const PlayLayout& Wide();             // current layout (width, boardScreenX, extras)
int ScreenWidth();                    // logical width W
int CenteredX();                      // x of an 800-wide screen centred on W

// Lawn boards (backgrounds 1-6, 1400 px art) use the wide layout; the others are centred 800 screens.
bool IsWideBoard(const Board* theBoard);
// Board widget x when the board is not panning, and the pan stops for the level-start camera.
BoardCamera Camera(const Board* theBoard);
// Board-local x for a UI item the board draws at vanillaX on an 800 screen (seed bank, menu button, ...).
int UiX(const Board* theBoard, int vanillaX, Anchor theAnchor);
// Board x of a screen (widget manager) mouse x, for code that assumed the board at x 0: held plant or shovel
// image, planting preview, plant highlights. Uses the rest position, so a shaking board moves them as in vanilla.
int MouseBoardX(const Board* theBoard, int screenX);
// The same for y (board y = screen y without the zoom, Mod/Engine/Zoom.h).
int MouseBoardY(const Board* theBoard, int screenY);
// Board x of the screen's right edge with the board at rest (vanilla 800).
int VisibleRightBoardX(const Board* theBoard);
// Draw-only x shift for a coin being collected: it still flies to the vanilla counter position (so it is
// credited on the vanilla frame) but is drawn heading for the counter's moved place.
float CoinDrawShift(Coin* theCoin);
// The parts of it: the full shift at arrival and the flight's progress (0 where clicked, 1 there); false when the coin
// is not flying to a moved counter.
bool CoinCollectShift(Coin* theCoin, float& theFullShift, float& theProgress);
// 2026-11-23 layout (mobile-PvZ style, Mod/Hud.h): seed cards in a column over the house and a top bar (sun,
// shovel, speed) while playing, where the HUD fits (not on conveyor-belt or slot machine levels, which keep the
// vanilla top bank). The name is kept from the earlier side panel.
bool SeedPanelActive(Board* theBoard);
// The board uses the HUD layout in any scene (SeedPanelActive adds: while playing). The shovel follows this one.
bool HudLayout(Board* theBoard);
// Board::Update: put the seed bank and its packets where the current layout wants them.
void ApplySeedBankLayout(Board* theBoard);
// HUD offsets: the cards slide out to the left and the top bar up while vanilla hides the seed bank above the screen
// (Crazy Dave talking, level intro), and back in with it.
HudSlide HudSlideOf(Board* theBoard);
// Seed packet theIndex's rect as drawn, in logical screen px (the widget manager's mouse coordinates): a card's frame or a
// (scaled) tall packet of the HUD, sliding with it; a vanilla packet on the top bank. For overlays such as a
// selection mark (subtract the board's mX/mY to draw from the board).
Sexy::Rect SeedPacketScreenRect(SeedBank* theBank, int theIndex);
// Where Board::TutorialArrowShow should point for seed packet theIndex: the vanilla spot (packet origin) on the top
// bank, centred under the packet or card of the HUD.
void SeedPacketArrowOrigin(Board* theBoard, int theIndex, int& theX, int& theY);
// SeedBank::Draw with the HUD: the sun counter box of the top bar (bank-local coordinates, y 0 = screen top).
void DrawHudSunBox(Sexy::Graphics* g, Board* theBoard);
// Board::GetShovelButtonRect with the HUD: the top bar's shovel box (board coordinates).
Sexy::Rect HudShovelRect(Board* theBoard);
// Board::DrawShovel with the HUD: draws the shovel box (and the shovel when theDrawShovel); false without the HUD.
bool DrawHudShovel(Sexy::Graphics* g, Board* theBoard, bool theDrawShovel);
// Speed button (1x/2x, setting "speed" in pvzu-settings.json). Board::Update runs this many board updates per frame:
// the setting while the HUD is up and playing (never paused, in demo recordings/replays or at width 800), else 1.
int BoardUpdatesPerFrame(Board* theBoard);
// Board::MouseDown: true when the press hit the speed button (nothing held) and switched the speed.
bool HudSpeedMouseDown(Board* theBoard, int x, int y, int theClickCount);
// Board::DrawUIBottom, after the shovel: the speed button.
void DrawHudSpeedButton(Sexy::Graphics* g, Board* theBoard);
// Board-local x of the coin bank: with the HUD at the bottom-right, left of the "Level x-y" label.
int CoinBankX(Board* theBoard);
// True when the coin bank is drawn in Board::DrawUITop (above the street haze it now sits on) instead of its layer.
bool CoinBankOverHaze(Board* theBoard);
// Board::Update: slide the street cover in at rest, out off the right edge during camera pans, game over and on the
// boss level (never faded; Mod/StreetFog.h StepEdgeFog).
void UpdateStreetFog(Board* theBoard);
// True when the board has a street strip (the menu button is then drawn above the haze).
bool HasStreetFog(const Board* theBoard);
// Board::DrawUITop: draw the street haze (above zombies, below the UI).
void DrawStreetFog(Sexy::Graphics* g, Board* theBoard);
// Drag-to-plant and tap-tap planting: Mod/Engine/MobilePlanting.h.
// Read the window's safe area (camera hole, rounded corners) into the layout; call after the window exists.
void UpdateSafeInsets(Sexy::SexyAppBase* theApp);
// Position the board widget at rest (size 800 x 600). Its mouse area grows by the extra width on each side,
// so at rest it covers the whole screen and during pans it moves with the board, as in vanilla.
void PlaceBoard(Board* theBoard);

// True when the frame shows an 800-wide screen (menus, Almanac, Zen Garden...) whose sides need filling:
// the topmost full-screen widget is not a wide lawn board and not a screen-wide widget.
bool SideFillWanted();

// Place an 800 x 600 screen widget in the middle of the wide screen.
void PlaceCenteredScreen(Sexy::Widget* theWidget, int theVanillaX = 0, int theVanillaY = 0);

} // namespace pvzu::engine
