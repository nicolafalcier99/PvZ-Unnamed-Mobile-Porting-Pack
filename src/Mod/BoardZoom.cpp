// PVZU: see BoardZoom.h.
#include "Mod/BoardZoom.h"

#include "Mod/Hud.h"

#include <algorithm>

namespace pvzu {

namespace {
constexpr int kHudBottom = kHudMargin + kTopBarH;  // the top bar's lower edge (screen y)
constexpr int kBackgroundRightX = 1180;            // board x of the background's right edge
constexpr int kLawnRightX = 760;                   // board x of the lawn's right edge (LAWN_XMIN + 9 * 80)
constexpr float kZoomStep = 0.001f;
}

float ZoomScreenX(const ZoomView& v, float boardX) { return v.x + v.scale * boardX; }
float ZoomScreenY(const ZoomView& v, float boardY) { return v.y + v.scale * boardY; }
float ZoomBoardX(const ZoomView& v, float screenX) { return (screenX - v.x) / v.scale; }
float ZoomBoardY(const ZoomView& v, float screenY) { return (screenY - v.y) / v.scale; }

int LawnTop(LawnGrid grid) {
	// Column 8: the roof's flat (highest) part; day/pool rows are level. Grass lawns' art starts above the first
	// tile (measured in the Unnamed art: day y 61, pool y 63 for tiles at y 80); the roof's tiles have no fringe.
	constexpr int kGrassFringe = 20;
	return LawnTileRect(grid, 8, 0).y - (grid == LawnGrid::Roof ? 0 : kGrassFringe);
}

int TopRowPlantTop(LawnGrid grid) {
	// Plants stand at Board::GridToPixelY: the tile's top on day and pool lawns, 10 px above it on the roof's flat part.
	const int aPlantY = LawnTileRect(grid, 8, 0).y - (grid == LawnGrid::Roof ? 10 : 0);
	return aPlantY + kPlantHeadInset;
}

int LawnBottom(LawnGrid grid) {
	const PxRect r = LawnTileRect(grid, 0, LawnRows(grid) - 1); // column 0: the roof's lowest tile
	return r.y + r.h;
}

ZoomView ComputeRestZoom(const PlayLayout& layout, LawnGrid grid) {
	const ZoomView aOff = UnzoomedView(layout.boardScreenX, 0);
	if (!HudFits(layout))
		return aOff;
	const float W = static_cast<float>(layout.width), H = static_cast<float>(kLogicalHeight);
	const float aTop = static_cast<float>(LawnTop(grid)), aBottom = static_cast<float>(LawnBottom(grid));
	// Largest zoom with the lowest row on the screen and the lawn's first row (plus a small gap) below the top bar.
	float s = std::min((H - (kHudBottom + kTopBarLawnGap)) / (aBottom - aTop), kMaxBoardZoom);
	const float aColumnRight = static_cast<float>(layout.safeLeft + kHudMargin + kHudColumnW);
	const float aMaxRightX = static_cast<float>(std::max(kVanillaWidth, ScreenRightBoardX(layout)));
	for (; s > 1.0f; s -= kZoomStep) {
		// The house's edge on the screen's edge (no black), unless the lawn would then not fit left of the right safe
		// inset: then move left, cutting some house.
		const float x = std::min(kBackgroundLeftSpare * s, W - layout.safeRight - kLawnRightX * s);
		const bool aMowersClear = x + kLawnLeftBoardX * s >= aColumnRight;
		const bool aStreetCovered = x + kBackgroundRightX * s >= W;
		const bool aNoMoreStreet = W - x <= aMaxRightX * s; // never more street than the unzoomed view (haze strip)
		if (aMowersClear && aStreetCovered && aNoMoreStreet)
			return {s, x, std::min(H - aBottom * s, 0.0f)};
	}
	return aOff;
}

ZoomView UnzoomedView(int boardX, int boardY) {
	return {1.0f, static_cast<float>(boardX), static_cast<float>(boardY)};
}

ZoomView BlendZoom(const ZoomView& a, const ZoomView& b, float t) {
	if (t <= 0.0f)
		return a;
	if (t >= 1.0f)
		return b;
	return {a.scale + (b.scale - a.scale) * t, a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t};
}

float ZoomEase(float progress) {
	const float t = std::clamp(progress, 0.0f, 1.0f);
	return t * t * (3.0f - 2.0f * t);
}

ZoomView ZoomedPanView(const ZoomView& rest, const BoardCamera& camera, int cameraX, int width) {
	ZoomView v = rest;
	const float aHouseX = kBackgroundLeftSpare * rest.scale;                     // background's left edge at x 0
	const float aStreetX = static_cast<float>(width) - kBackgroundRightX * rest.scale; // its right edge at the right
	if (cameraX < camera.rest && camera.street < camera.rest) {
		const float t = static_cast<float>(camera.rest - cameraX) / static_cast<float>(camera.rest - camera.street);
		v.x = rest.x + (aStreetX - rest.x) * std::min(t, 1.0f);
	} else if (cameraX > camera.rest && camera.house > camera.rest) {
		const float t = static_cast<float>(cameraX - camera.rest) / static_cast<float>(camera.house - camera.rest);
		v.x = rest.x + (aHouseX - rest.x) * std::min(t, 1.0f);
	}
	return v;
}

} // namespace pvzu
