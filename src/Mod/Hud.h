// PVZU: the play-screen HUD in the style of the mobile PvZ (2026-11-23, reference video of his phone):
// no side panel; landscape seed cards float in one column over the house at the top-left, a top bar right of them
// holds the sun counter, the shovel and a 1x/2x speed button. Everything is drawn from the PC game's own art
// (seed packet, SeedBank, ShovelBank). Menu stays top-right; coin bank and "Level x-y" bottom-right.
//
//   +------+ +-----------+ +------+ +------+                         [Menu]
//   | card | | (*)  1234 | |shovel| |>> 1x |
//   +------+ +-----------+ +------+ +------+
//   | card |      ...lawn...
//   +------+
//   | card |                                        [coins] Level 1-4
//
// Pure geometry here; Mod/Engine/WideScreen draws it. HUD-local coordinates: logical screen px, x measured from the
// left safe inset (screen x = layout.safeLeft + x), y from the top of the screen.
#pragma once

#include "Mod/Layout.h"
#include "Mod/Touch.h"
#include "Mod/TouchPlanting.h"

#include <vector>

namespace pvzu {

constexpr int kHudMargin = 4;      // from the safe inset and from the top edge
constexpr int kCardW = 114;        // landscape card: the reference's 105 x 62 px at 1200 px wide = 114 x 67 logical
constexpr int kCardH = 67;
constexpr int kCardGap = 6;        // between cards, between the column and the top bar, between top-bar boxes
constexpr int kHudColumnW = kCardW;
constexpr int kCompactCardW = (kHudColumnW - kCardGap) / 2; // 9+ slots: two compact cards per row in the same column
constexpr int kTopBarH = 52;
constexpr int kSunBoxW = 150;
constexpr int kShovelBoxW = 80;
constexpr int kSpeedBoxW = 80;
constexpr int kHudBoxFrame = 8;    // the SeedBank art's frame thickness (the boxes are 9-sliced from it)
constexpr int kLawnLeftBoardX = -25; // left edge of the lawn mowers at rest (LawnMower x -21): the cards stay left of it

// Seed slots. Wide cards while all of them fit one column (1-8); more (9-15, Unnamed allows 15): compact cards, two
// per row in the same column, as tall as a wide card (8 rows fit 600 px), shorter only if the rows would not fit.
struct PanelSlot { int x = 0, y = 0, w = kCardW, h = kCardH; };
enum class PanelMode {
	WideCards,   // one column of landscape cards
	CompactCards, // compact cards, two per row
};
PanelMode SeedPanelMode(int count);
// Packet index of count (row by row for compact cards), HUD-local. Drawing and hit testing both use this rect.
PanelSlot SeedPanelSlot(int index, int count);
// Tap areas (HUD-local): each covers its packet, finger-sized where there is room, split at the midpoint between
// neighbours, never reaching the top bar.
std::vector<HitRect> SeedPanelHitRects(int count);
// Where to show the tutorial arrow for a packet (the vanilla particle sits at a packet's (25, 80): centred under a
// 50x70 packet, 10 px below it): centred under any slot size.
struct PanelPoint { int x = 0, y = 0; };
PanelPoint SeedPanelArrowOrigin(const PanelSlot& slot);

// Top bar, HUD-local. Fixed slots: the speed button never moves, also where the shovel is not shown (yet): the
// shovel appears only when play starts, and on first-time levels 1-1 .. 1-4 not at all.
enum class HudBox { Sun, Shovel, Speed };
HitRect TopBarRect(HudBox box);
// Inside the sun box (box-local): the bank art's sun on the left, its paper label (stretched) with the number.
struct SunBoxParts { HitRect icon; HitRect paper; };
SunBoxParts SunBoxLayout(int w, int h);

// The drawn card of packet index of count (HUD-local): the card frame, centred in its slot (Mod/PanelCard.h).
PxRect HudCardFrame(int index, int count);
// Selection brackets (Mod/TouchPlanting.h) around card index of count, or the top bar's shovel box (index -1): the
// fullest style that keeps them, glow included, off the other cards, the top-bar boxes and the screen's edges.
BracketStyle HudBracketStyle(int index, int count, const PlayLayout& layout);

// The HUD is used only where the card column (right of the safe inset) stays left of the lawn mowers.
bool HudFits(const PlayLayout& layout);

// Crazy Dave and the level intro hide vanilla's bank by moving it up (bankY 0 .. -bankHeight). The cards slide off the
// left edge and the top bar off the top edge, with the same progress.
struct HudSlide { int cardsX = 0; int barY = 0; };
HudSlide HudSlideFor(int bankY, int bankHeight, int safeLeft);

// Coin bank (vanilla screen x before the right-edge anchor): just left of the "Level x-y" label, which vanilla draws
// right-aligned at labelRightX.
constexpr int kCoinBankLabelGap = 10;
int CoinBankVanillaX(int labelRightX, int labelWidth, int bankWidth);

} // namespace pvzu
