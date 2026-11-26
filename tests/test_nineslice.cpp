#include "doctest/doctest.h"
#include "Mod/NineSlice.h"

using namespace pvzu;

TEST_CASE("NineSliceCoversDestExactly") {
	const Box src{0, 0, 446, 87};
	const Box dst{10, 0, 197, 600};
	const auto parts = NineSlice(src, /*inset=*/8, dst);
	REQUIRE(parts.size() == 9);
	long area = 0;
	for (const auto& p : parts) {
		CHECK(p.dst.w > 0);
		CHECK(p.dst.h > 0);
		CHECK(p.dst.x >= dst.x);
		CHECK(p.dst.y >= dst.y);
		CHECK(p.dst.x + p.dst.w <= dst.x + dst.w);
		CHECK(p.dst.y + p.dst.h <= dst.y + dst.h);
		area += static_cast<long>(p.dst.w) * p.dst.h;
	}
	CHECK(area == static_cast<long>(dst.w) * dst.h);  // no gaps, no overlaps
}

TEST_CASE("NineSliceCornersKeepTheirSize") {
	const auto parts = NineSlice(Box{0, 0, 446, 87}, 8, Box{0, 0, 140, 600});
	// order: TL, T, TR, L, C, R, BL, B, BR
	CHECK(parts[0].src.w == 8); CHECK(parts[0].dst.w == 8); CHECK(parts[0].dst.h == 8);
	CHECK(parts[2].src.x == 446 - 8); CHECK(parts[2].dst.x == 140 - 8);
	CHECK(parts[8].dst.x == 132); CHECK(parts[8].dst.y == 592);
	CHECK(parts[4].src.w == 446 - 16); CHECK(parts[4].dst.w == 140 - 16); CHECK(parts[4].dst.h == 600 - 16);
}

TEST_CASE("TileFillsWithoutStretching") {
	// a 100x40 texture tiled into 250x90 -> 3x3 pieces, every piece <= texture size, last ones cropped
	const auto tiles = TileRects(Box{0, 0, 100, 40}, Box{5, 5, 250, 90});
	CHECK(tiles.size() == 9);
	long area = 0;
	for (const auto& t : tiles) {
		CHECK(t.src.w == t.dst.w);  // 1:1, never stretched
		CHECK(t.src.h == t.dst.h);
		area += static_cast<long>(t.dst.w) * t.dst.h;
	}
	CHECK(area == 250L * 90);
}
