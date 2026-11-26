// PVZU: mobile planting UX (2026-11-22, spec in the design notes (mobile-planting-animation) section 4).
// Pure parts: lawn tile geometry for the crosshair preview, the corner brackets around the selected packet, and
// the tap-tap / drag rules. The engine side (Mod/Engine/TouchPlanting.cpp) feeds them and applies the result.
#pragma once

#include <vector>

namespace pvzu {

struct PxRect {
	int x = 0, y = 0, w = 0, h = 0;
	bool operator==(const PxRect&) const = default;
};

// --- Lawn grid (board coordinates, the rules of Board::PixelToGridX/Y) ---
enum class LawnGrid {
	Day,   // 5 rows of 100 px (day, night, and the fog-free boss roof is not this one)
	Pool,  // 6 rows of 85 px (pool, fog)
	Roof,  // 5 rows of 85 px, columns 0-4 lower by (4 - col) * 20 px (roof, boss)
};
int LawnRows(LawnGrid grid);
// The area of tile (col, row) that maps back to it (9 columns of 80 px from x 40, rows from y 80).
PxRect LawnTileRect(LawnGrid grid, int col, int row);

// Crosshair preview: the target row across the whole lawn and the target column from the lawn's top edge to its
// bottom edge. Both are drawn (additive), so their crossing is the brightest cell. Empty outside the grid.
struct Crosshair {
	std::vector<PxRect> row;
	std::vector<PxRect> column;
};
Crosshair TileCrosshair(LawnGrid grid, int col, int row);
constexpr int kCrosshairAlpha = 70;  // additive white

// --- Corner brackets around the held packet (or the shovel) ---
constexpr int kBracketGap = 3;  // px between the packet's edge and the bracket
struct BracketArm {
	PxRect orange;  // the arm
	PxRect yellow;  // the thin line along its inner side (the side facing the packet)
};
constexpr int kBracketGlow = 3; // the soft glow drawn around the arms (px)
// How far the brackets reach out from the packet: gap, then the arm, then its glow.
struct BracketStyle {
	int gap = kBracketGap;
	int thick = 3;
	int glow = kBracketGlow;
	bool operator==(const BracketStyle&) const = default;
};
// The full style: arms 6% of the packet's width thick (at least 3 px).
BracketStyle DefaultBracketStyle(int packetW);
int BracketOutset(const BracketStyle& style); // gap + thick + glow
// The fullest style that reaches out at most `room` px (phone bug list 2026-11-23: on the HUD they overlapped the sun box and
// the next card): first the glow goes, then the arms thin to 3 px, then the gap closes to 1 px, then the arms thin to
// 2 px (1 + 2 px is the least drawn, even with less room).
BracketStyle FitBracketStyle(int packetW, int room);
// The room around target: the largest outset up to maxRoom by which it can grow on all sides and stay inside bounds
// and off every obstacle (something diagonal counts only once it is reached in both directions).
int BracketRoom(const PxRect& target, const std::vector<PxRect>& obstacles, const PxRect& bounds, int maxRoom);
// Eight arms: top-left (horizontal, vertical), top-right (h, v), bottom-left (h, v), bottom-right (h, v).
// Each arm runs 30% of the packet's width (height) along its edge, style.thick px thick (default style: 6% of the
// width, at least 3 px), style.gap px outside the packet.
std::vector<BracketArm> CornerBrackets(const PxRect& packet);
std::vector<BracketArm> CornerBrackets(const PxRect& packet, const BracketStyle& style);

// --- Tap-tap and drag rules ---
constexpr int kNothingHeld = -1;
constexpr int kShovelHeld = -2;  // otherwise `held` is the seed packet index
constexpr int kDragMinTravel = 20;  // px: less than this between down and up is a tap

enum class TouchTarget { Packet, Shovel, Lawn, Coin, Other };
struct TouchHit {
	TouchTarget target = TouchTarget::Other;
	int packet = -1;     // Packet: index
	bool ready = false;  // Packet: could be picked up now (recharged, affordable)
};

enum class DownAction {
	Vanilla,    // vanilla Board::MouseDown (pick up, buzz, collect a coin, return the seed off the lawn)
	LawnPress,  // finger on the lawn while holding: show the crosshair, act when it lifts
	Deselect,   // the held packet (or shovel) tapped again: put it back
	Switch,     // another ready packet (or the shovel): put the held one back, pick this one up
	Buzz,       // another packet that is not ready: vanilla's buzzer (and red sun), keep the selection
};
DownAction OnTouchDown(int held, const TouchHit& hit);

struct TouchPress {
	bool pickedThisPress = false;  // this press picked the seed or shovel up (Vanilla or Switch)
	bool lawnPress = false;        // DownAction::LawnPress
	int downX = 0, downY = 0;
};
enum class UpAction {
	None,             // keep things as they are (a tap selects; mode (a) waits for the tile)
	Release,          // vanilla's click at the lift point: plant (or dig), advice on a bad tile, return off lawn
	ReleaseOrReturn,  // the same, then return the seed if it is still held (a drag that missed)
};
// upOnOrigin: the finger lifted over the packet (or shovel) this press picked up.
UpAction OnTouchUp(const TouchPress& press, int held, int upX, int upY, bool upOnOrigin);

// The held plant image: only while the finger is down and not over the lawn (over the lawn the finger hides it
// and the see-through plant in the target tile shows instead), drawn kHeldImageLift px above the finger.
bool HeldImageVisible(bool fingerDown, bool overLawn);
constexpr int kHeldImageLift = 40;

} // namespace pvzu
