#include "doctest/doctest.h"
#include "Mod/Screen.h"

#include <initializer_list>

using namespace pvzu;

TEST_CASE("ViewportFillsMatchingAspect") {
	// S26 Ultra: 3120x1440 pixels shows a 1300x600 logical screen with no bars.
	Viewport v = ComputeViewport(3120, 1440, 1300, 600);
	CHECK(v.x == 0);
	CHECK(v.y == 0);
	CHECK(v.w == 3120);
	CHECK(v.h == 1440);
}

TEST_CASE("ViewportPillarboxesWiderWindow") {
	// Vanilla 800x600 in a 16:9 window: bars left and right, like upstream's 4:3 letterbox.
	Viewport v = ComputeViewport(1920, 1080, 800, 600);
	CHECK(v.w == 1440);
	CHECK(v.h == 1080);
	CHECK(v.x == 240);
	CHECK(v.y == 0);
}

TEST_CASE("ViewportLetterboxesTallerWindow") {
	// A 1300-wide logical screen in a 4:3 window: bars top and bottom.
	Viewport v = ComputeViewport(1600, 1200, 1300, 600);
	CHECK(v.w == 1600);
	CHECK(v.h == 738);
	CHECK(v.x == 0);
	CHECK(v.y == (1200 - 738) / 2);
}

TEST_CASE("ViewportVanillaMatchesUpstream4by3") {
	// Upstream: vw = height*4/3 when too wide, vh = width*3/4 when too tall.
	for (int w : {640, 800, 1024, 1366, 1920, 2560}) {
		for (int h : {480, 600, 768, 1080, 1440}) {
			Viewport v = ComputeViewport(w, h, 800, 600);
			int vx = 0, vy = 0, vw = w, vh = h;
			if (w * 3 > h * 4) { vw = h * 4 / 3; vx = (w - vw) / 2; }
			else if (w * 3 < h * 4) { vh = w * 3 / 4; vy = (h - vh) / 2; }
			CHECK(v.x == vx);
			CHECK(v.y == vy);
			CHECK(v.w == vw);
			CHECK(v.h == vh);
		}
	}
}

TEST_CASE("ViewportRejectsBadSizes") {
	Viewport v = ComputeViewport(0, 0, 800, 600);
	CHECK(v.w == 0);
	CHECK(v.h == 0);
}

TEST_CASE("DesktopWindowIsDoubleSizeWhenItFits") {
	WindowSize s = DesktopWindowSize(1300, 600, 3000, 1800);
	CHECK(s.w == 2600);
	CHECK(s.h == 1200);
}

TEST_CASE("DesktopWindowShrinksToFitKeepingAspect") {
	WindowSize s = DesktopWindowSize(1300, 600, 1512, 900);
	CHECK(s.w <= 1512);
	CHECK(s.h <= 900);
	CHECK(s.w == 1512);
	CHECK(s.h == 698);  // 1512 * 600 / 1300
	WindowSize t = DesktopWindowSize(800, 600, 0, 0);  // unknown display: logical size
	CHECK(t.w == 800);
	CHECK(t.h == 600);
}

TEST_CASE("ChooseWidthDesktopDefaultsToVanilla") {
	CHECK(ChooseLogicalWidth(0, 1512, 982, false) == 800);
	CHECK(ChooseLogicalWidth(1300, 1512, 982, false) == 1300);
	CHECK(ChooseLogicalWidth(5000, 1512, 982, false) == 1400);  // forced width still clamped
	CHECK(ChooseLogicalWidth(10, 1512, 982, false) == 800);
}

TEST_CASE("ChooseWidthPhoneFollowsDisplay") {
	CHECK(ChooseLogicalWidth(0, 3120, 1440, true) == 1300);
	CHECK(ChooseLogicalWidth(0, 1440, 3120, true) == 1300);  // portrait-reported display is landscape in game
	CHECK(ChooseLogicalWidth(0, 2048, 1536, true) == 800);   // 4:3 tablet
	CHECK(ChooseLogicalWidth(1100, 3120, 1440, true) == 1100);  // forced wins (debug)
	CHECK(ChooseLogicalWidth(0, 0, 0, true) == 800);
}

TEST_CASE("CenteredScreenX") {
	CHECK(CenteredScreenX(800) == 0);
	CHECK(CenteredScreenX(1300) == 250);
	CHECK(CenteredScreenX(1067) == 133);
}
