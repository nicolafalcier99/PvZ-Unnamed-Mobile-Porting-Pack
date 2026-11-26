// PVZU: see TouchPlanting.h.
#include "Mod/TouchPlanting.h"

#include "Mod/Touch.h"

#include <algorithm>
#include <cmath>

namespace pvzu {

namespace {
constexpr int kLawnXMin = 40;   // LAWN_XMIN
constexpr int kLawnYMin = 80;   // LAWN_YMIN
constexpr int kColumns = 9;     // MAX_GRID_SIZE_X
constexpr int kTileW = 80;

int TileH(LawnGrid grid) { return grid == LawnGrid::Day ? 100 : 85; }
int RoofSlope(LawnGrid grid, int col) { return grid == LawnGrid::Roof && col < 5 ? (4 - col) * 20 : 0; }
bool InGrid(LawnGrid grid, int col, int row) { return col >= 0 && col < kColumns && row >= 0 && row < LawnRows(grid); }
}

int LawnRows(LawnGrid grid) { return grid == LawnGrid::Pool ? 6 : 5; }

PxRect LawnTileRect(LawnGrid grid, int col, int row) {
	const int h = TileH(grid);
	return {kLawnXMin + col * kTileW, kLawnYMin + RoofSlope(grid, col) + row * h, kTileW, h};
}

Crosshair TileCrosshair(LawnGrid grid, int col, int row) {
	Crosshair c;
	if (!InGrid(grid, col, row))
		return c;
	if (grid == LawnGrid::Roof) {
		for (int x = 0; x < kColumns; ++x)
			c.row.push_back(LawnTileRect(grid, x, row));
	} else {
		const PxRect t = LawnTileRect(grid, 0, row);
		c.row.push_back({t.x, t.y, kColumns * kTileW, t.h});
	}
	const PxRect top = LawnTileRect(grid, col, 0);
	c.column.push_back({top.x, top.y, top.w, LawnRows(grid) * top.h});
	return c;
}

BracketStyle DefaultBracketStyle(int packetW) {
	BracketStyle s;
	s.thick = std::max(3, static_cast<int>(std::lround(packetW * 0.06)));
	return s;
}

int BracketOutset(const BracketStyle& s) { return s.gap + s.thick + s.glow; }

BracketStyle FitBracketStyle(int packetW, int room) {
	BracketStyle s = DefaultBracketStyle(packetW);
	auto over = [&] { return BracketOutset(s) - room; };
	s.glow = std::max(0, s.glow - std::max(0, over()));
	s.thick = std::max(std::min(s.thick, 3), s.thick - std::max(0, over()));
	s.gap = std::max(1, s.gap - std::max(0, over()));
	s.thick = std::max(2, s.thick - std::max(0, over()));
	return s;
}

int BracketRoom(const PxRect& target, const std::vector<PxRect>& obstacles, const PxRect& bounds, int maxRoom) {
	auto apart = [](const PxRect& a, const PxRect& b) {
		return a.x + a.w <= b.x || b.x + b.w <= a.x || a.y + a.h <= b.y || b.y + b.h <= a.y;
	};
	for (int d = maxRoom; d > 0; --d) {
		const PxRect g{target.x - d, target.y - d, target.w + 2 * d, target.h + 2 * d};
		const bool aInside = g.x >= bounds.x && g.y >= bounds.y && g.x + g.w <= bounds.x + bounds.w && g.y + g.h <= bounds.y + bounds.h;
		if (aInside && std::all_of(obstacles.begin(), obstacles.end(), [&](const PxRect& o) { return apart(g, o); }))
			return d;
	}
	return 0;
}

std::vector<BracketArm> CornerBrackets(const PxRect& p) { return CornerBrackets(p, DefaultBracketStyle(p.w)); }

std::vector<BracketArm> CornerBrackets(const PxRect& p, const BracketStyle& style) {
	const int t = style.thick;
	const int yt = std::max(1, t / 3);
	const int lenH = static_cast<int>(std::lround(p.w * 0.30));
	const int lenV = static_cast<int>(std::lround(p.h * 0.30));
	const int g = style.gap;
	// Top-left corner in packet-local coordinates; the others are mirrors of it.
	const PxRect hArm{-g - t, -g - t, lenH + g + t, t};
	const PxRect hLine{-g - yt, -g - yt, lenH + g + yt, yt};
	const PxRect vArm{-g - t, -g, t, lenV + g};
	const PxRect vLine{-g - yt, -g, yt, lenV + g};
	auto place = [&](const PxRect& r, bool right, bool bottom) {
		PxRect o = r;
		o.x = p.x + (right ? p.w - (r.x + r.w) : r.x);
		o.y = p.y + (bottom ? p.h - (r.y + r.h) : r.y);
		return o;
	};
	std::vector<BracketArm> arms;
	for (int corner = 0; corner < 4; ++corner) {
		const bool right = corner == 1 || corner == 3;
		const bool bottom = corner >= 2;
		arms.push_back({place(hArm, right, bottom), place(hLine, right, bottom)});
		arms.push_back({place(vArm, right, bottom), place(vLine, right, bottom)});
	}
	return arms;
}

DownAction OnTouchDown(int held, const TouchHit& hit) {
	if (held == kNothingHeld)
		return DownAction::Vanilla;
	switch (hit.target) {
	case TouchTarget::Packet:
		if (hit.packet == held)
			return DownAction::Deselect;
		return hit.ready ? DownAction::Switch : DownAction::Buzz;
	case TouchTarget::Shovel:
		return held == kShovelHeld ? DownAction::Deselect : DownAction::Switch;
	case TouchTarget::Lawn:
		return DownAction::LawnPress;
	case TouchTarget::Coin:
	case TouchTarget::Other:
		break;
	}
	return DownAction::Vanilla;
}

UpAction OnTouchUp(const TouchPress& press, int held, int upX, int upY, bool upOnOrigin) {
	if (held == kNothingHeld)
		return UpAction::None;
	if (press.lawnPress)
		return UpAction::Release;
	if (!press.pickedThisPress)
		return UpAction::None;
	const bool aTap = !DragShouldPlantOnRelease(true, press.downX, press.downY, upX, upY, kDragMinTravel);
	if (aTap || upOnOrigin)
		return UpAction::None;
	return UpAction::ReleaseOrReturn;
}

bool HeldImageVisible(bool fingerDown, bool overLawn) { return fingerDown && !overLawn; }

} // namespace pvzu
