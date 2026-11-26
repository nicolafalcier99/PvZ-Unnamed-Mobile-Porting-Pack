#include "doctest/doctest.h"
#include "Mod/Layout.h"

#include <initializer_list>

using namespace pvzu;

TEST_CASE("LayoutS26Ultra") {
	// 2026-11-23 (mobile-PvZ style): no side panel; the cards float over the house, so the extra width shows
	// all 220 px of house on the left (the background's left edge on the screen's edge), the rest is street.
	CHECK(ComputeLogicalWidth(3120, 1440) == 1300);
	PlayLayout l = ComputePlayLayout(1300);
	CHECK(l.width == 1300);
	CHECK(l.houseExtra == 220);
	CHECK(l.leftExtra == 220);
	CHECK(l.rightExtra == 280);
	CHECK(l.boardScreenX == 220);
	CHECK(VisibleRightBoardX(l) == 1080);
}

TEST_CASE("LayoutHouseFirstThenStreet") {
	PlayLayout a = ComputePlayLayout(1067);  // 16:9: all the house, 47 px of street
	CHECK(a.houseExtra == 220);
	CHECK(a.rightExtra == 47);
	PlayLayout b = ComputePlayLayout(900);   // house only
	CHECK(b.houseExtra == 100);
	CHECK(b.rightExtra == 0);
	PlayLayout c = ComputePlayLayout(1400);  // widest: the rest is street (under the haze)
	CHECK(c.houseExtra == 220);
	CHECK(c.rightExtra == 380);
}

TEST_CASE("LayoutNeverShowsPastTheBackground") {
	// No black anywhere: at rest the 1400 px background (board x -220 .. 1180) covers the whole screen.
	for (int w = 800; w <= 1400; ++w) {
		PlayLayout l = ComputePlayLayout(w);
		CHECK(l.boardScreenX - kBackgroundLeftSpare <= 0); // background's left edge at or left of the screen edge
		CHECK(l.boardScreenX + 1180 >= w);                 // and its right edge at or right of the other
	}
}

TEST_CASE("LayoutWidthClamps") {
	CHECK(ComputeLogicalWidth(2048, 1536) == 800);   // 4:3 tablet
	CHECK(ComputeLogicalWidth(1920, 1080) == 1067);  // 16:9
	CHECK(ComputeLogicalWidth(2520, 1080) == 1400);  // 21:9 fold, capped
	CHECK(ComputeLogicalWidth(1000, 1000) == 800);   // square, floored
	CHECK(ComputeLogicalWidth(0, 0) == 800);         // bad input never divides by zero
	for (int w = 800; w <= 1400; ++w) {
		PlayLayout l = ComputePlayLayout(w);
		CHECK(l.leftExtra + l.rightExtra == w - 800);
		CHECK(l.leftExtra == l.houseExtra);
		CHECK(220 - l.houseExtra >= 0);              // visible background starts inside the 1400 px background
		CHECK(1020 + l.rightExtra <= 1400);          // and ends inside it
		CHECK(l.boardScreenX == l.leftExtra);
	}
}

TEST_CASE("LayoutVanillaWidthIsVanilla") {
	PlayLayout l = ComputePlayLayout(800);
	CHECK(l.leftExtra == 0);
	CHECK(l.rightExtra == 0);
	CHECK(l.houseExtra == 0);
	CHECK(l.boardScreenX == 0);
	CHECK(AnchorX(700, Anchor::Right, l) == 700);
	CHECK(AnchorX(10, Anchor::Left, l) == 10);
}

TEST_CASE("AnchorRightKeepsDistance") {
	PlayLayout l = ComputePlayLayout(1300);
	CHECK(AnchorX(700, Anchor::Right, l) == 1200);
	CHECK(AnchorX(10, Anchor::Left, l) == 10);
	CHECK(AnchorX(400, Anchor::Center, l) == 650);
	CHECK(AnchorX(100, Anchor::Board, l) == 320);    // things that live on the lawn move with the board
}

TEST_CASE("BoardCameraStops") {
	BoardCamera v = ComputeBoardCamera(ComputePlayLayout(800));
	CHECK(v.rest == 0);       // vanilla: board widget at x 0
	CHECK(v.house == 220);    // vanilla: background's left edge on the screen's left edge
	CHECK(v.street == -380);  // vanilla: background's right edge (1180) on the screen's right edge
	BoardCamera w = ComputeBoardCamera(ComputePlayLayout(1300));
	CHECK(w.rest == 220);
	CHECK(w.house == 220);    // background's left edge on the screen's left edge (no panel)
	CHECK(w.street == 120);   // 1180 + 120 == 1300
	for (int width = 800; width <= 1400; width += 50) {
		BoardCamera c = ComputeBoardCamera(ComputePlayLayout(width));
		CHECK(c.house >= c.rest);
		CHECK(c.house == kBackgroundLeftSpare); // the pan never shows past the background's left edge
		CHECK(c.street <= c.rest);
		CHECK(c.street + 1180 == width);
	}
}

TEST_CASE("CollectDrawShiftFollowsProgress") {
	// A collected sun flies to the vanilla sun counter (logic unchanged); the picture is shifted towards the
	// moved counter in step with how far it has flown.
	CHECK(CollectDrawShift(-220.0f, 300.0f, 300.0f) == doctest::Approx(0.0f));    // just clicked
	CHECK(CollectDrawShift(-220.0f, 300.0f, 150.0f) == doctest::Approx(-110.0f)); // half way
	CHECK(CollectDrawShift(-220.0f, 300.0f, 0.0f) == doctest::Approx(-220.0f));   // arrived
	CHECK(CollectDrawShift(-220.0f, 0.0f, 0.0f) == doctest::Approx(-220.0f));     // clicked on the counter
	CHECK(CollectDrawShift(-220.0f, 300.0f, 400.0f) == doctest::Approx(0.0f));    // never beyond the start
	CHECK(CollectDrawShift(0.0f, 300.0f, 100.0f) == doctest::Approx(0.0f));       // vanilla width: no shift
}

TEST_CASE("BoardLocalXForUi") {
	PlayLayout l = ComputePlayLayout(1300);
	CHECK(BoardLocalX(10, Anchor::Left, l) == -210);   // seed bank: stays at the screen's left edge
	CHECK(BoardLocalX(681, Anchor::Right, l) == 961);  // menu button: stays at the right edge (screen 1181)
	CHECK(BoardLocalX(300, Anchor::Board, l) == 300);  // lawn things do not move in board coordinates
	CHECK(BoardLocalX(400, Anchor::Center, l) == 430); // screen 650 = centre of 1300
	PlayLayout v = ComputePlayLayout(800);
	for (int x : {0, 10, 400, 681, 799}) {
		CHECK(BoardLocalX(x, Anchor::Left, v) == x);
		CHECK(BoardLocalX(x, Anchor::Right, v) == x);
		CHECK(BoardLocalX(x, Anchor::Center, v) == x);
	}
}

TEST_CASE("SmoothUpscaleOnlyAtUnevenScales") {
	CHECK(ShouldSmoothUpscale(1080, 600));        // S26 FHD+: 1.8x -> linear (nearest looks uneven)
	CHECK(ShouldSmoothUpscale(1440, 600));        // QHD+: 2.4x
	CHECK_FALSE(ShouldSmoothUpscale(1200, 600));  // Mac test window: exact 2x stays crisp nearest
	CHECK_FALSE(ShouldSmoothUpscale(600, 600));   // 1:1
	CHECK_FALSE(ShouldSmoothUpscale(1800, 600));  // 3x
	CHECK_FALSE(ShouldSmoothUpscale(0, 600));
}

TEST_CASE("ChooserKeepsClearOfSafeInsets") {
	PlayLayout l = ComputePlayLayout(1300);
	l.safeLeft = 57;
	l.safeRight = 70;
	CHECK(ChooserScreenX(l) == 57);  // the whole "Choose your plants" screen moves right of the camera hole
	// a right-edge button at vanilla x 681 (width 117) ends inside the right safe edge, in chooser-local coords
	const int local = ChooserRightButtonX(681, l);
	CHECK(ChooserScreenX(l) + local + 117 <= l.width - l.safeRight + (800 - 681 - 117));
	PlayLayout v = ComputePlayLayout(800);
	CHECK(ChooserScreenX(v) == 0);
	CHECK(ChooserRightButtonX(681, v) == 681);   // vanilla unchanged
}

TEST_CASE("ChooserHoverUsesChooserCoordinates") {
	// The chooser's tooltip and hand cursor hit-test seeds at chooser-local x; the chooser sits right of the camera hole,
	// so a finger on the Sunflower (screen x 156) is chooser x 99, not 156 (that showed Cherry Bomb's tooltip).
	PlayLayout l = ComputePlayLayout(1300);
	l.safeLeft = 57;
	CHECK(ChooserMouseX(156, l) == 99);
	CHECK(ChooserMouseX(156, ComputePlayLayout(800)) == 156); // vanilla unchanged
}

TEST_CASE("CrazyDaveStandsInTheScreenHeTalksOn") {
	// The board draws Crazy Dave at vanilla screen x (Board::DrawUITop). A lawn board shows the whole wide screen: he
	// stands at its left edge, by the house. An 800-wide board (Zen Garden, Tree of Wisdom) sits in the middle with
	// filled sides: he stands at its left edge, not in the filled side (the side fill covered him once the black sides were fixed).
	const PlayLayout l = ComputePlayLayout(1300);
	CHECK(CrazyDaveScreenX(true, l) == 0);
	CHECK(CrazyDaveScreenX(false, l) == 250);
	const PlayLayout v = ComputePlayLayout(800);
	CHECK(CrazyDaveScreenX(true, v) == 0);  // vanilla unchanged
	CHECK(CrazyDaveScreenX(false, v) == 0);
}
