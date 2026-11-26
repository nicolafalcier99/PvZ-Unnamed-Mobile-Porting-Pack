#include "doctest/doctest.h"
#include "Mod/SideFill.h"

#include <cmath>

using namespace pvzu;

TEST_CASE("SideFillOffAtVanillaWidth") {
	SideFillPlan p = PlanSideFill(800);
	CHECK_FALSE(p.active);
}

TEST_CASE("SideFillCoversBothSides") {
	SideFillPlan p = PlanSideFill(1300);
	REQUIRE(p.active);
	// The 800-wide screen sits at 250..1050; the sides are 0..250 and 1050..1300, full height.
	CHECK(p.left.dstX == 0);
	CHECK(p.left.dstW == 250);
	CHECK(p.right.dstX == 1050);
	CHECK(p.right.dstW == 250);
}

TEST_CASE("SideFillOddExtraWidth") {
	SideFillPlan p = PlanSideFill(1067); // 16:9
	REQUIRE(p.active);
	CHECK(p.left.dstW + 800 + p.right.dstW == 1067); // no gap, no overlap
	CHECK(p.right.dstX == p.left.dstW + 800);
}

TEST_CASE("SideFillMirrorsTheScreenAtTheSeam") {
	// Each side shows the screen's own edge region folded out at the seam (u across the 800 screen, 0 = its left
	// edge): the colours at the seam continue the screen's, and nothing is stretched (1 logical px = 1 screen px),
	// so no streaks (2026-11-23: "looks made from multiple lines").
	SideFillPlan p = PlanSideFill(1300);
	REQUIRE(p.active);
	CHECK(p.left.uAtDstX == doctest::Approx(250.0f / 800.0f)); // outer edge of the left side
	CHECK(p.left.uAtDstEnd == doctest::Approx(0.0f));           // the seam: the screen's left edge
	CHECK(p.right.uAtDstX == doctest::Approx(1.0f));             // the seam: the screen's right edge
	CHECK(p.right.uAtDstEnd == doctest::Approx(1.0f - 250.0f / 800.0f));
	for (const SideFillPart& s : {p.left, p.right})
		CHECK(std::fabs(s.uAtDstEnd - s.uAtDstX) * 800.0f == doctest::Approx(static_cast<float>(s.dstW)));
}

TEST_CASE("SideFillMirrorNeverReadsPastTheScreen") {
	SideFillPlan p = PlanSideFill(2600); // sides wider than the screen: hold the far edge instead of wrapping
	REQUIRE(p.active);
	for (const SideFillPart& s : {p.left, p.right}) {
		CHECK(s.uAtDstX >= 0.0f);
		CHECK(s.uAtDstX <= 1.0f);
		CHECK(s.uAtDstEnd >= 0.0f);
		CHECK(s.uAtDstEnd <= 1.0f);
	}
}

TEST_CASE("SideFillShrinkChainHalvesToASmallTexture") {
	// Phone (2340x1080 px for 1300x600 logical): the centred screen is 1440x1080 px. Each step halves (2x2 average
	// with linear filtering) until the width is at most 128.
	auto c = SideFillShrinkChain(1440, 1080, 128);
	REQUIRE(c.size() == 4);
	CHECK(c[0].w == 720);
	CHECK(c[0].h == 540);
	CHECK(c[1].w == 360);
	CHECK(c[2].w == 180);
	CHECK(c[3].w == 90);
	CHECK(c[3].h == 68); // 67.5 rounded up: never loses the last row
	CHECK(SideFillShrinkChain(100, 75, 128).empty()); // already small: blur it as it is
	auto tiny = SideFillShrinkChain(1000, 1, 128);
	CHECK(tiny.back().h == 1);
}

TEST_CASE("SideFillBlurSigmaFollowsTheScreenNotThePixels") {
	// The same blur radius in logical px whatever the texture size the chain ends on.
	const float a = SideFillBlurSigma(90), b = SideFillBlurSigma(180);
	CHECK(b == doctest::Approx(2.0f * a));
	CHECK(a * 800.0f / 90.0f == doctest::Approx(kSideFillBlurLogicalPx));
}

TEST_CASE("SideFillBlurTapsAreANormalisedGaussian") {
	// 9 texel weights read as 5 linear-filtered taps (centre + 2 pairs either side).
	SideFillBlurTaps t = SideFillBlurTapsFor(2.0f);
	CHECK(t.weight[0] + 2.0f * (t.weight[1] + t.weight[2]) == doctest::Approx(1.0f));
	CHECK(t.offset[0] == doctest::Approx(0.0f));
	CHECK(t.offset[1] > 1.0f);
	CHECK(t.offset[1] < 2.0f);
	CHECK(t.offset[2] > 3.0f);
	CHECK(t.offset[2] < 4.0f);
	// sigma 2: texel weights 0.2042, 0.1802, 0.1238, 0.0663, 0.0276 (normalised over -4..4).
	CHECK(t.weight[0] == doctest::Approx(0.2042f).epsilon(0.01));
	CHECK(t.weight[1] == doctest::Approx(0.1802f + 0.1238f).epsilon(0.01));
	CHECK(t.offset[1] == doctest::Approx(1.0f + 0.1238f / (0.1802f + 0.1238f)).epsilon(0.01));
	CHECK(t.weight[2] == doctest::Approx(0.0663f + 0.0276f).epsilon(0.02));
}

TEST_CASE("SideFillShadeFadesFromTheSeam") {
	// Brightest next to the screen (a soft edge, no hard line), darker further out so the real screen stays the focus;
	// never black, so the camera-hole safe insets at the far edges still show the backdrop.
	CHECK(SideFillShade(0.0f) == doctest::Approx(kSideFillShadeSeam));
	CHECK(SideFillShade(kSideFillFeatherPx) == doctest::Approx(kSideFillShadeOuter));
	CHECK(SideFillShade(1000.0f) == doctest::Approx(kSideFillShadeOuter));
	CHECK(SideFillShade(kSideFillFeatherPx / 2) < kSideFillShadeSeam);
	CHECK(SideFillShade(kSideFillFeatherPx / 2) > kSideFillShadeOuter);
	CHECK(kSideFillShadeOuter >= 0.5f);
	CHECK(kSideFillShadeSeam <= 1.0f);
	CHECK(kSideFillSaturation < 1.0f);
	CHECK(kSideFillSaturation > 0.5f);
}

TEST_CASE("SideFillIgnoresTheParkedSeedChooser") {
	// Every game makes the screen-wide seed chooser and parks it below the screen (y 516, SEED_CHOOSER_OFFSET_Y), in
	// front of the board. While Crazy Dave talks in the Zen Garden the intro never ends, so it stays there: it must not
	// count as a screen that draws its own sides (black sides beside the 800-wide garden, phone bug list 2026-11-23).
	CHECK_FALSE(ScreenWideWidgetOwnsSides(516, 600, 600));
	CHECK_FALSE(ScreenWideWidgetOwnsSides(600, 600, 600));  // fully below
	CHECK_FALSE(ScreenWideWidgetOwnsSides(-600, 600, 600)); // fully above
	// On the screen (the chooser up, or sliding with most of it showing) it does.
	CHECK(ScreenWideWidgetOwnsSides(0, 600, 600));
	CHECK(ScreenWideWidgetOwnsSides(300, 600, 600));
	CHECK(ScreenWideWidgetOwnsSides(-5, 600, 600));        // a small shake
	CHECK_FALSE(ScreenWideWidgetOwnsSides(301, 600, 600)); // less than half of the screen
}
