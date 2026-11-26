#include "doctest/doctest.h"
#include "Mod/TouchPlanting.h"

#include <algorithm>

using namespace pvzu;

namespace {
bool Inside(const PxRect& inner, const PxRect& outer) {
	return inner.x >= outer.x && inner.y >= outer.y && inner.x + inner.w <= outer.x + outer.w && inner.y + inner.h <= outer.y + outer.h;
}
bool Overlaps(const PxRect& a, const PxRect& b) {
	return a.x < b.x + b.w && b.x < a.x + a.w && a.y < b.y + b.h && b.y < a.y + a.h;
}
}

// --- Tiles and crosshair (Board::PixelToGridX/Y inverted) ---

TEST_CASE("TileRectDayLawn") {
	CHECK(LawnRows(LawnGrid::Day) == 5);
	CHECK(LawnTileRect(LawnGrid::Day, 0, 0) == PxRect{40, 80, 80, 100});
	CHECK(LawnTileRect(LawnGrid::Day, 3, 2) == PxRect{280, 280, 80, 100});
	CHECK(LawnTileRect(LawnGrid::Day, 8, 4) == PxRect{680, 480, 80, 100});
}

TEST_CASE("TileRectPoolLawn") {
	CHECK(LawnRows(LawnGrid::Pool) == 6);
	CHECK(LawnTileRect(LawnGrid::Pool, 0, 0) == PxRect{40, 80, 80, 85});
	CHECK(LawnTileRect(LawnGrid::Pool, 4, 5) == PxRect{360, 505, 80, 85});
}

TEST_CASE("TileRectRoofSlope") {
	// Roof: columns 0-4 are lower by (4 - col) * 20 px (PixelToGridY subtracts that before dividing by 85).
	CHECK(LawnRows(LawnGrid::Roof) == 5);
	CHECK(LawnTileRect(LawnGrid::Roof, 0, 0) == PxRect{40, 160, 80, 85});
	CHECK(LawnTileRect(LawnGrid::Roof, 4, 1) == PxRect{360, 165, 80, 85});
	CHECK(LawnTileRect(LawnGrid::Roof, 5, 1) == PxRect{440, 165, 80, 85});
	CHECK(LawnTileRect(LawnGrid::Roof, 8, 4) == PxRect{680, 420, 80, 85});
}

TEST_CASE("CrosshairDayIsOneRowBandAndOneColumnBand") {
	const Crosshair c = TileCrosshair(LawnGrid::Day, 3, 2);
	REQUIRE(c.row.size() == 1);
	REQUIRE(c.column.size() == 1);
	CHECK(c.row[0] == PxRect{40, 280, 720, 100});      // whole row, full lawn width
	CHECK(c.column[0] == PxRect{280, 80, 80, 500});    // lawn top to bottom
	CHECK(Overlaps(c.row[0], c.column[0]));            // the crossing gets both washes
}

TEST_CASE("CrosshairPoolCoversSixRows") {
	const Crosshair c = TileCrosshair(LawnGrid::Pool, 0, 5);
	REQUIRE(c.row.size() == 1);
	REQUIRE(c.column.size() == 1);
	CHECK(c.row[0] == PxRect{40, 505, 720, 85});
	CHECK(c.column[0] == PxRect{40, 80, 80, 510});
}

TEST_CASE("CrosshairRoofRowFollowsTheSlope") {
	const Crosshair c = TileCrosshair(LawnGrid::Roof, 2, 1);
	REQUIRE(c.row.size() == 9);  // one piece per column: the roof row is a staircase
	for (int col = 0; col < 9; ++col)
		CHECK(c.row[col] == LawnTileRect(LawnGrid::Roof, col, 1));
	REQUIRE(c.column.size() == 1);
	CHECK(c.column[0] == PxRect{200, 120, 80, 425});   // column 2 starts (4-2)*20 px lower
}

TEST_CASE("CrosshairOutsideGridIsEmpty") {
	CHECK(TileCrosshair(LawnGrid::Day, -1, 2).row.empty());
	CHECK(TileCrosshair(LawnGrid::Day, 9, 2).column.empty());
	CHECK(TileCrosshair(LawnGrid::Day, 3, 5).row.empty());   // day lawn has rows 0-4
	CHECK_FALSE(TileCrosshair(LawnGrid::Pool, 3, 5).row.empty());
}

// --- Corner brackets around the selected packet ---

TEST_CASE("BracketsSitOutsideThePacket") {
	const PxRect packet{100, 200, 50, 70};
	const std::vector<BracketArm> arms = CornerBrackets(packet);
	REQUIRE(arms.size() == 8);  // two arms per corner
	for (const BracketArm& a : arms) {
		CHECK_FALSE(Overlaps(a.orange, packet));   // never covers the packet
		CHECK(Inside(a.yellow, a.orange));         // the yellow line runs inside the orange arm
		CHECK(a.yellow.w * a.yellow.h < a.orange.w * a.orange.h);
	}
	// Top-left corner: the orange L's inner edge is kBracketGap px outside the packet edge.
	const BracketArm& h = arms[0];
	const BracketArm& v = arms[1];
	CHECK(h.orange.y + h.orange.h == packet.y - kBracketGap);
	CHECK(v.orange.x + v.orange.w == packet.x - kBracketGap);
	// The yellow line is on the side facing the packet.
	CHECK(h.yellow.y + h.yellow.h == h.orange.y + h.orange.h);
	CHECK(v.yellow.x + v.yellow.w == v.orange.x + v.orange.w);
}

TEST_CASE("BracketArmsAreAboutThirtyPercentLongAndSixPercentThick") {
	const PxRect packet{0, 0, 100, 140};
	const std::vector<BracketArm> arms = CornerBrackets(packet);
	const BracketArm& h = arms[0];
	const BracketArm& v = arms[1];
	CHECK(h.orange.h == 6);                                    // 6% of width
	CHECK(v.orange.w == 6);
	CHECK(h.orange.x + h.orange.w == 30);                      // reaches 30% of the width along the top edge
	CHECK(v.orange.y + v.orange.h == 42);                      // 30% of the height down the left edge
	CHECK_FALSE(Overlaps(h.orange, v.orange));                 // corner square drawn once (glow not doubled)
	// The middle of each side stays open.
	for (const BracketArm& a : arms)
		CHECK_FALSE(Overlaps(a.orange, PxRect{40, -20, 20, 180}));
}

TEST_CASE("BracketsAreMirroredOnAllFourCorners") {
	const PxRect p{10, 20, 50, 70};
	const std::vector<BracketArm> arms = CornerBrackets(p);
	// Order: TL(h,v), TR(h,v), BL(h,v), BR(h,v). Mirror of TL across the packet's centre lines.
	const PxRect tl = arms[0].orange;
	const PxRect tr = arms[2].orange;
	const PxRect bl = arms[4].orange;
	const PxRect br = arms[6].orange;
	CHECK(tr.y == tl.y);
	CHECK(tr.w == tl.w);
	CHECK((tr.x + tr.w) - (p.x + p.w) == p.x - tl.x);
	CHECK(bl.x == tl.x);
	CHECK(bl.y - (p.y + p.h) == p.y - (tl.y + tl.h));
	CHECK(br.x == tr.x);
	CHECK(br.y == bl.y);
}

TEST_CASE("BracketsOnSmallPacketStayVisible") {
	const std::vector<BracketArm> arms = CornerBrackets(PxRect{0, 0, 30, 42});
	CHECK(arms[0].orange.h >= 3);  // never thinner than 3 px
	CHECK(arms[0].yellow.h >= 1);
}

TEST_CASE("BracketStyleIsTheFullOneWhereThereIsRoom") {
	const BracketStyle s = DefaultBracketStyle(114); // a wide HUD card: 7 px arms
	CHECK(s.gap == kBracketGap);
	CHECK(s.thick == 7);
	CHECK(s.glow == kBracketGlow);
	CHECK(BracketOutset(s) == kBracketGap + 7 + kBracketGlow);
	CHECK(FitBracketStyle(114, 40) == s);
	CHECK(FitBracketStyle(114, BracketOutset(s)) == s);
	// The arms of the default style are the ones CornerBrackets(packet) draws.
	const PxRect p{10, 20, 114, 58};
	const auto a = CornerBrackets(p);
	const auto b = CornerBrackets(p, s);
	REQUIRE(a.size() == b.size());
	for (size_t i = 0; i < a.size(); ++i) {
		CHECK(a[i].orange == b[i].orange);
		CHECK(a[i].yellow == b[i].yellow);
	}
}

TEST_CASE("BracketStyleShrinksToTheRoom") {
	// Glow goes first, then the arms thin to 3 px, then the gap closes to 1 px, then the arms thin to 2 px.
	CHECK(FitBracketStyle(114, 10) == BracketStyle{kBracketGap, 7, 0});
	CHECK(FitBracketStyle(114, 6) == BracketStyle{kBracketGap, 3, 0});  // wide card next to the sun box
	CHECK(FitBracketStyle(54, 6) == BracketStyle{kBracketGap, 3, 0});   // compact card between its neighbours
	CHECK(FitBracketStyle(80, 4) == BracketStyle{1, 3, 0});             // shovel box at the top edge
	CHECK(FitBracketStyle(80, 3) == BracketStyle{1, 2, 0});
	for (int w : {30, 54, 80, 114})
		for (int room = 0; room <= 20; ++room) {
			const BracketStyle s = FitBracketStyle(w, room);
			CHECK(s.thick >= 2);
			CHECK(s.gap >= 1);
			CHECK(s.glow >= 0);
			CHECK(BracketOutset(s) <= std::max(room, 3)); // 3 px (1 gap + 2 arm) is the least that still shows
		}
}

TEST_CASE("BracketRoomStopsAtNeighboursAndTheScreen") {
	const PxRect screen{0, 0, 1300, 600};
	const PxRect card{61, 8, 114, 58};
	const PxRect sun{181, 4, 150, 52};   // 6 px right of the card
	const PxRect below{61, 81, 114, 58}; // 15 px below
	CHECK(BracketRoom(card, {sun, below}, screen, 13) == 6);
	CHECK(BracketRoom(card, {below}, screen, 13) == 8);   // the screen's top edge, 8 px up
	CHECK(BracketRoom(card, {}, PxRect{0, -100, 1300, 800}, 13) == 13); // capped
	CHECK(BracketRoom(card, {}, PxRect{0, -100, 1300, 800}, 5) == 5);
	// Something diagonal only counts once the grown rect reaches it in both directions.
	CHECK(BracketRoom(PxRect{61, 81, 114, 58}, {sun}, screen, 13) == 13);
}

// --- Tap-tap and drag rules ---

TEST_CASE("DownWithNothingHeldIsVanilla") {
	CHECK(OnTouchDown(kNothingHeld, {TouchTarget::Packet, 2, true}) == DownAction::Vanilla);   // picks it up
	CHECK(OnTouchDown(kNothingHeld, {TouchTarget::Packet, 2, false}) == DownAction::Vanilla);  // buzzer, red sun
	CHECK(OnTouchDown(kNothingHeld, {TouchTarget::Shovel}) == DownAction::Vanilla);
	CHECK(OnTouchDown(kNothingHeld, {TouchTarget::Lawn}) == DownAction::Vanilla);
	CHECK(OnTouchDown(kNothingHeld, {TouchTarget::Coin}) == DownAction::Vanilla);
}

TEST_CASE("TapSelectedPacketAgainDeselects") {
	CHECK(OnTouchDown(3, {TouchTarget::Packet, 3, false}) == DownAction::Deselect); // held packet reads "not ready"
	CHECK(OnTouchDown(kShovelHeld, {TouchTarget::Shovel}) == DownAction::Deselect);
}

TEST_CASE("TapAnotherReadyPacketSwitches") {
	CHECK(OnTouchDown(3, {TouchTarget::Packet, 1, true}) == DownAction::Switch);
	CHECK(OnTouchDown(3, {TouchTarget::Shovel}) == DownAction::Switch);
	CHECK(OnTouchDown(kShovelHeld, {TouchTarget::Packet, 0, true}) == DownAction::Switch);
}

TEST_CASE("TapAnotherPacketNotReadyBuzzesAndKeepsSelection") {
	CHECK(OnTouchDown(3, {TouchTarget::Packet, 1, false}) == DownAction::Buzz);
	CHECK(OnTouchDown(kShovelHeld, {TouchTarget::Packet, 1, false}) == DownAction::Buzz);
}

TEST_CASE("TouchOnLawnWhileHoldingStartsALawnPress") {
	CHECK(OnTouchDown(0, {TouchTarget::Lawn}) == DownAction::LawnPress);        // plants on lift, not now
	CHECK(OnTouchDown(kShovelHeld, {TouchTarget::Lawn}) == DownAction::LawnPress);
}

TEST_CASE("CoinsAndOffLawnTouchesStayVanillaWhileHolding") {
	CHECK(OnTouchDown(0, {TouchTarget::Coin}) == DownAction::Vanilla);   // sun is collected, seed stays held
	CHECK(OnTouchDown(0, {TouchTarget::Other}) == DownAction::Vanilla);  // vanilla returns the seed
}

TEST_CASE("LawnPressReleasesWhereTheFingerLifts") {
	TouchPress p;
	p.lawnPress = true;
	p.downX = 300;
	p.downY = 300;
	CHECK(OnTouchUp(p, 0, 300, 300, false) == UpAction::Release);
	CHECK(OnTouchUp(p, 0, 520, 180, false) == UpAction::Release);        // slid to another tile first
	CHECK(OnTouchUp(p, kShovelHeld, 300, 300, false) == UpAction::Release);
	CHECK(OnTouchUp(p, kNothingHeld, 300, 300, false) == UpAction::None); // something else cleared it
}

TEST_CASE("TapOnPacketSelectsIt") {
	TouchPress p;
	p.pickedThisPress = true;
	p.downX = 70;
	p.downY = 130;
	CHECK(OnTouchUp(p, 0, 70, 130, true) == UpAction::None);    // stays selected (mode a)
	CHECK(OnTouchUp(p, 0, 80, 135, true) == UpAction::None);    // a little wobble is still a tap
	CHECK(OnTouchUp(p, 0, 75, 140, false) == UpAction::None);   // short travel: a tap even off the packet
}

TEST_CASE("DragFromPacketReleasesOrReturns") {
	TouchPress p;
	p.pickedThisPress = true;
	p.downX = 70;
	p.downY = 130;
	CHECK(OnTouchUp(p, 0, 400, 300, false) == UpAction::ReleaseOrReturn);  // plants, or the seed goes back
	CHECK(OnTouchUp(p, 0, 70, 160, true) == UpAction::None);              // slid but let go on its own packet
	CHECK(OnTouchUp(p, 0, 70, 400, false) == UpAction::ReleaseOrReturn);  // let go elsewhere on the panel
	CHECK(OnTouchUp(p, kNothingHeld, 400, 300, false) == UpAction::None);
}

TEST_CASE("UpOfAnOrdinaryPressIsVanilla") {
	TouchPress p;  // e.g. a coin tap, or a tap that only buzzed
	CHECK(OnTouchUp(p, kNothingHeld, 10, 10, false) == UpAction::None);
	CHECK(OnTouchUp(p, 2, 400, 300, false) == UpAction::None);  // a buzz kept the old selection: nothing more
}

// --- What is drawn while holding ---

TEST_CASE("HeldImageOnlyWhileTheFingerIsDownOffTheLawn") {
	CHECK(HeldImageVisible(true, false));    // dragging from the panel towards the lawn
	CHECK_FALSE(HeldImageVisible(true, true));   // the finger hides it; the tile preview shows instead
	CHECK_FALSE(HeldImageVisible(false, false)); // finger up: nothing follows anything
	CHECK_FALSE(HeldImageVisible(false, true));
	CHECK(kHeldImageLift == 40);
}
