#include "doctest/doctest.h"
#include "Mod/BoardZoom.h"
#include "Mod/Hud.h"
#include "Mod/Layout.h"
#include "Mod/StreetFog.h"

#include <cmath>

using namespace pvzu;

namespace {
PlayLayout Phone(int width, int safeLeft = 57, int safeRight = 70) {
	PlayLayout l = ComputePlayLayout(width);
	l.safeLeft = safeLeft;
	l.safeRight = safeRight;
	return l;
}
constexpr LawnGrid kGrids[] = {LawnGrid::Day, LawnGrid::Pool, LawnGrid::Roof};
constexpr int kHudBottom = kHudMargin + kTopBarH;
constexpr int kBackgroundLeftX = -220;  // board x of the 1400 px background's left edge (BOARD_OFFSET)
constexpr int kBackgroundRightX = 1180; // and of its right edge
} // namespace

TEST_CASE("BoardZoomIsOffAtWidth800") {
	for (LawnGrid g : kGrids) {
		const ZoomView v = ComputeRestZoom(ComputePlayLayout(800), g);
		CHECK(v.scale == 1.0f);
		CHECK(v.x == 0.0f);
		CHECK(v.y == 0.0f);
	}
}

TEST_CASE("BoardZoomIsOffWhereTheHudDoesNotFit") {
	PlayLayout l = Phone(900, 57, 0);
	REQUIRE_FALSE(HudFits(l));
	const ZoomView v = ComputeRestZoom(l, LawnGrid::Day);
	CHECK(v.scale == 1.0f);
	CHECK(v.x == static_cast<float>(l.boardScreenX));
	CHECK(v.y == 0.0f);
}

TEST_CASE("BoardZoomOnTheS26") {
	// 1300 x 600, camera hole 57 px left, 70 px right. See BoardZoom.h for why this is ~1.08 (top bar clear of the
	// first row, 2026-11-23), not 1.2.
	const ZoomView v = ComputeRestZoom(Phone(1300), LawnGrid::Day);
	CHECK(v.scale > 1.03f);
	CHECK(v.scale < 1.06f);
	CHECK(ZoomScreenX(v, kBackgroundLeftX) == doctest::Approx(0.0f)); // the whole house, its edge on the screen's edge
	CHECK(ZoomScreenY(v, 580) == doctest::Approx(600.0f));             // the lowest row ends on the screen's bottom
}

TEST_CASE("BoardZoomShowsEveryRowAndColumn") {
	for (int w = 1000; w <= 1400; w += 10)
		for (LawnGrid g : kGrids) {
			const PlayLayout l = Phone(w);
			const ZoomView v = ComputeRestZoom(l, g);
			CAPTURE(w);
			CHECK(v.scale >= 1.0f);
			CHECK(v.scale <= kMaxBoardZoom);
			CHECK(ZoomScreenY(v, static_cast<float>(LawnTileRect(g, 8, 0).y)) >= 0.0f);
			CHECK(ZoomScreenY(v, static_cast<float>(LawnBottom(g))) <= kLogicalHeight + 0.01f);
			if (v.scale == 1.0f)
				continue; // unzoomed: the wide layout as before
			// Lawn mowers (board x -25) right of the card column, the lawn's right edge left of the right safe inset.
			CHECK(ZoomScreenX(v, kLawnLeftBoardX) >= l.safeLeft + kHudMargin + kHudColumnW);
			CHECK(ZoomScreenX(v, 760.0f) <= l.width - l.safeRight + 0.01f);
		}
}

TEST_CASE("BoardZoomNeverShowsBlack") {
	for (int w = 800; w <= 1400; w += 10)
		for (LawnGrid g : kGrids) {
			const PlayLayout l = Phone(w);
			const ZoomView v = ComputeRestZoom(l, g);
			CAPTURE(w);
			CHECK(ZoomScreenX(v, kBackgroundLeftX) <= 0.01f);
			CHECK(ZoomScreenX(v, kBackgroundRightX) >= l.width - 0.01f);
			CHECK(ZoomScreenY(v, 0.0f) <= 0.01f);
			CHECK(ZoomScreenY(v, static_cast<float>(kLogicalHeight)) >= kLogicalHeight - 0.01f);
		}
}

TEST_CASE("BoardZoomKeepsTheTopRowClearOfTheTopBar") {
	// 2026-11-23: the top bar (sun, shovel) must not touch the lawn's first row: zoom out until it is clear.
	for (int w = 1000; w <= 1400; w += 10)
		for (LawnGrid g : kGrids) {
			const ZoomView v = ComputeRestZoom(Phone(w), g);
			CAPTURE(w);
			CHECK(ZoomScreenY(v, static_cast<float>(LawnTop(g))) >= kHudBottom + kTopBarLawnGap - 0.01f);
		}
}

TEST_CASE("LawnTopIsWhereTheGrassArtStarts") {
	// Measured 2026-11-23 in the Unnamed lawn art (board y = image y): day grass from y 61, pool from y 63, both above
	// the first row's tile (y 80); the roof's first row starts at its tile.
	CHECK(LawnTop(LawnGrid::Day) <= 61);
	CHECK(LawnTop(LawnGrid::Pool) <= 63);
	CHECK(LawnTop(LawnGrid::Roof) == LawnTileRect(LawnGrid::Roof, 8, 0).y);
}

TEST_CASE("BoardZoomStillZoomsOnThePhone") {
	for (LawnGrid g : kGrids) {
		CAPTURE(static_cast<int>(g));
		CHECK(ComputeRestZoom(Phone(1300), g).scale > 1.0f);
	}
}

TEST_CASE("BoardZoomNeverShowsMoreStreetThanUnzoomed") {
	// What the zoom leaves of the street lies inside the unzoomed street strip, and the street cover reaches full opacity
	// by the screen edge (test_streetfog NewZombiesAreFullyHidden), so no zombie appears in clear view.
	for (int w = 801; w <= 1400; w += 1) {
		const PlayLayout l = Phone(w);
		const ZoomView v = ComputeRestZoom(l, LawnGrid::Day);
		CAPTURE(w);
		const float aRight = ZoomBoardX(v, static_cast<float>(l.width));
		CHECK(aRight <= ScreenRightBoardX(l) + 0.01f);
		CHECK(aRight >= 800.0f - 0.01f); // the spawn strip is still at (or past) the screen's right edge
		const int aEdge = static_cast<int>(std::ceil(aRight)); // as ViewRightBoardX
		const StreetFogRamp r = ComputeStreetFogRamp(aEdge, kVanillaMinSpawnX + 133);
		if (aEdge > 800)
			CHECK(StreetFogAlpha(static_cast<float>(aEdge), r) == 1.0f);
	}
}

TEST_CASE("BoardZoomMapsBothWays") {
	const ZoomView v = ComputeRestZoom(Phone(1300), LawnGrid::Pool);
	for (float b = -200.0f; b <= 1100.0f; b += 37.0f) {
		CHECK(ZoomBoardX(v, ZoomScreenX(v, b)) == doctest::Approx(b));
		CHECK(ZoomBoardY(v, ZoomScreenY(v, b)) == doctest::Approx(b));
	}
}

TEST_CASE("BoardZoomBlendsFromUnzoomed") {
	const ZoomView a = UnzoomedView(220, 0);
	CHECK(a.scale == 1.0f);
	CHECK(a.x == 220.0f);
	const ZoomView b = ComputeRestZoom(Phone(1300), LawnGrid::Day);
	const ZoomView s = BlendZoom(a, b, 0.0f), e = BlendZoom(a, b, 1.0f), m = BlendZoom(a, b, 0.5f);
	CHECK(s.scale == a.scale);
	CHECK(s.x == a.x);
	CHECK(e.scale == b.scale);
	CHECK(e.y == b.y);
	CHECK(m.scale == doctest::Approx((a.scale + b.scale) / 2));
	CHECK(ZoomEase(0.0f) == 0.0f);
	CHECK(ZoomEase(1.0f) == 1.0f);
	CHECK(ZoomEase(-1.0f) == 0.0f);
	CHECK(ZoomEase(2.0f) == 1.0f);
	float aLast = 0.0f;
	for (float p = 0.0f; p <= 1.0f; p += 0.05f) {
		CHECK(ZoomEase(p) >= aLast);
		aLast = ZoomEase(p);
	}
}

TEST_CASE("ZoomedPanShowsTheHouseAndTheStreetEdges") {
	const PlayLayout l = Phone(1300);
	const ZoomView r = ComputeRestZoom(l, LawnGrid::Day);
	const BoardCamera c = ComputeBoardCamera(l);
	const ZoomView atRest = ZoomedPanView(r, c, c.rest, l.width);
	CHECK(atRest.x == doctest::Approx(r.x));
	CHECK(atRest.scale == r.scale);
	CHECK(atRest.y == r.y);
	CHECK(ZoomScreenX(ZoomedPanView(r, c, c.street, l.width), kBackgroundRightX) == doctest::Approx(1300.0f));
	CHECK(ZoomScreenX(ZoomedPanView(r, c, c.house, l.width), kBackgroundLeftX) == doctest::Approx(0.0f));
	// Halfway along the pan, halfway between the two views.
	const int aMid = (c.rest + c.street) / 2;
	const float aMidX = ZoomedPanView(r, c, aMid, l.width).x;
	CHECK(aMidX < r.x);
	CHECK(aMidX > ZoomedPanView(r, c, c.street, l.width).x);
}
