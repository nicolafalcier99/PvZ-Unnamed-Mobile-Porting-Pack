#include "doctest/doctest.h"
#include "Mod/Layout.h"
#include "Mod/Touch.h"

using namespace pvzu;

TEST_CASE("HitRectGrowsToMin") {
	HitRect r = ExpandHitRect({100, 200, 50, 70}, 64);
	CHECK(r.w >= 64);
	CHECK(r.h >= 70);  // already tall enough: unchanged
	CHECK(r.x + r.w / 2 == 125);  // centred on the packet
	CHECK(r.y == 200);
	HitRect big = ExpandHitRect({0, 0, 80, 90}, 64);
	CHECK(big.w == 80);  // never shrinks
	CHECK(big.h == 90);
}

TEST_CASE("HitRectsDontOverlap") {
	// Ten packets in a row (vanilla spacing 51 px) and the two-column seed panel.
	std::vector<HitRect> row;
	for (int i = 0; i < 10; ++i)
		row.push_back({79 + i * 51, 8, 50, 70});
	std::vector<HitRect> grown = ExpandHitRects(row, 64);
	REQUIRE(grown.size() == row.size());
	for (size_t i = 0; i < grown.size(); ++i) {
		CHECK(grown[i].w >= 50);
		for (size_t j = 0; j < i; ++j) {
			const HitRect& a = grown[i];
			const HitRect& b = grown[j];
			const bool apart = a.x + a.w <= b.x || b.x + b.w <= a.x || a.y + a.h <= b.y || b.y + b.h <= a.y;
			CHECK(apart);
		}
		// Each grown rect still covers its own packet.
		CHECK(grown[i].x <= row[i].x);
		CHECK(grown[i].x + grown[i].w >= row[i].x + row[i].w);
	}
	std::vector<HitRect> panel = {{15, 92, 50, 70}, {75, 92, 50, 70}, {15, 168, 50, 70}, {75, 168, 50, 70}};
	std::vector<HitRect> g2 = ExpandHitRects(panel, 64);
	CHECK(g2[0].x + g2[0].w <= g2[1].x);
	CHECK(g2[0].w >= 55);
}

TEST_CASE("SafeInsetsMoveAnchorsOnly") {
	PlayLayout l = ComputePlayLayout(1300);
	PlayLayout s = l;
	s.safeLeft = 30;
	s.safeRight = 50;
	CHECK(AnchorX(681, Anchor::Right, s) == AnchorX(681, Anchor::Right, l) - 50);  // moves left by the inset
	CHECK(AnchorX(10, Anchor::Left, s) == AnchorX(10, Anchor::Left, l) + 30);      // moves right by the inset
	CHECK(AnchorX(100, Anchor::Board, s) == AnchorX(100, Anchor::Board, l));       // the lawn does not move
	CHECK(s.boardScreenX == l.boardScreenX);
	CHECK(VisibleRightBoardX(s) == VisibleRightBoardX(l));
}

TEST_CASE("TouchLiftLeavesNothingPressed") {
	// 2026-11-22 on the S26: "when I press something it stays pressed". A lift must end with the
	// virtual mouse leaving every widget, after the release has been delivered where the finger was.
	std::vector<MouseStep> down = TouchToMouse(TouchPhase::Down, 300, 200);
	REQUIRE(down.size() == 2);
	CHECK(down[0].kind == MouseStep::Kind::Move);  // no hover needed first: the press goes where the finger is
	CHECK(down[1].kind == MouseStep::Kind::Down);
	CHECK(down[1].x == 300);
	std::vector<MouseStep> move = TouchToMouse(TouchPhase::Move, 310, 205);
	REQUIRE(move.size() == 1);
	CHECK(move[0].kind == MouseStep::Kind::Move);
	std::vector<MouseStep> up = TouchToMouse(TouchPhase::Up, 312, 207);
	REQUIRE(up.size() == 3);
	CHECK(up[0].kind == MouseStep::Kind::Move);
	CHECK(up[1].kind == MouseStep::Kind::Up);
	CHECK(up[1].x == 312);
	CHECK(up[2].kind == MouseStep::Kind::Leave);
}

TEST_CASE("HoverEndsWhenAFingerOrPenLifts") {
	// Phone bug list 2026-11-23: after a tap the hover tooltip stayed. A finger or the S Pen tip leaves nothing hovering when it
	// lifts (tooltips and highlights go); a real mouse still hovers where it is (tooltips stay, as on the PC).
	CHECK(HoverEndsOnLift(PointerSource::Finger));
	CHECK(HoverEndsOnLift(PointerSource::Pen));
	CHECK_FALSE(HoverEndsOnLift(PointerSource::Mouse));
}

TEST_CASE("FollowThePointerModesFollowAFingerOnlyWhileItHoldsALawnPress") {
	// 0x78 Sunflower Commander / 0x8b Bullet Carnival glue their object to the pointer. A mouse is followed always (the
	// exe); a finger or pen only while a press that began on bare lawn is held, so a tap on a sun or packet does not
	// teleport it.
	CHECK(FollowsPointer(PointerSource::Mouse, false));
	CHECK(FollowsPointer(PointerSource::Mouse, true));
	CHECK(FollowsPointer(PointerSource::Finger, true));
	CHECK_FALSE(FollowsPointer(PointerSource::Finger, false));
	CHECK(FollowsPointer(PointerSource::Pen, true));
	CHECK_FALSE(FollowsPointer(PointerSource::Pen, false));
}

TEST_CASE("LastPointerRemembersTheSource") {
	CHECK(LastPointer() == PointerSource::Mouse); // nothing seen yet: a mouse (desktop, demos)
	NotePointer(PointerSource::Finger);
	CHECK(LastPointer() == PointerSource::Finger);
	NotePointer(PointerSource::Mouse);
	CHECK(LastPointer() == PointerSource::Mouse);
}

TEST_CASE("DragReleaseOnLawnPlants") {
	// Drag-to-plant: a packet picked up by this press and released away from the panel plants on release.
	CHECK(DragShouldPlantOnRelease(true, 0, 0, 200, 300, 20));
	CHECK_FALSE(DragShouldPlantOnRelease(true, 0, 0, 10, 5, 20));    // a tap: keep holding, tap a tile next
	CHECK_FALSE(DragShouldPlantOnRelease(false, 0, 0, 200, 300, 20)); // not picked up by this press
}
