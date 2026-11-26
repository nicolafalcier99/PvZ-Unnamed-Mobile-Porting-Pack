#include "doctest/doctest.h"
#include "Mod/LawnHoldInfo.h"

#include <string>

using namespace pvzu;
namespace u = pvzu::unnamed;

namespace {
const InfoRect kPhone{0, 0, 1300, 600}; // the S26 Ultra's logical screen (19.5:9)

bool Overlaps(const InfoRect& a, const InfoRect& b) {
	return a.x < b.x + b.w && b.x < a.x + a.w && a.y < b.y + b.h && b.y < a.y + a.h;
}
// Where the finger itself sits: its pad around the touch point, the rest of it below (the phone is held from below).
InfoRect Finger(int x, int y) { return {x - kLawnInfoFingerHalfW, y - kLawnInfoFingerHalfW, 2 * kLawnInfoFingerHalfW, 600}; }

LawnInfoText English() {
	LawnInfoText t;
	t.health = "Health";
	t.armour = "Armour";
	t.shield = "Shield";
	t.balloon = "Balloon";
	t.total = "Total";
	t.pumpkin = "Pumpkin";
	t.buttered = "Buttered";
	t.slowed = "Slowed";
	t.frozen = "Frozen";
	t.weakened = "Weakened";
	t.seconds = "s";
	return t;
}
std::string Join(const std::vector<LawnInfoRow>& theRows) {
	std::string s;
	for (const LawnInfoRow& r : theRows)
		s += r.label + "=" + r.value + ";";
	return s;
}
} // namespace

// --- placement -----------------------------------------------------------------------------------------------------
TEST_CASE("LawnInfoGoesAboveTheFingerWithAFingerGap") {
	const LawnInfoPlacement p = PlaceLawnInfo(650, 400, 300, 120, kPhone);
	CHECK(p.side == LawnInfoSide::Above);
	CHECK(p.box.w == 300);
	CHECK(p.box.h == 120);
	CHECK(p.box.x == 500);                                   // centred on the finger
	CHECK(p.box.y + p.box.h == 400 - kLawnInfoFingerGap);   // a finger height between the touch and the box
	CHECK(p.box.Inside(kPhone));
	CHECK_FALSE(Overlaps(p.box, Finger(650, 400)));
}

TEST_CASE("LawnInfoAboveIsClampedInsideTheScreenEdges") {
	const LawnInfoPlacement l = PlaceLawnInfo(20, 400, 300, 120, kPhone);
	CHECK(l.side == LawnInfoSide::Above);
	CHECK(l.box.x == kLawnInfoMargin);
	CHECK(l.box.y + l.box.h == 400 - kLawnInfoFingerGap);
	const LawnInfoPlacement r = PlaceLawnInfo(1290, 400, 300, 120, kPhone);
	CHECK(r.side == LawnInfoSide::Above);
	CHECK(r.box.x + r.box.w == 1300 - kLawnInfoMargin);
	CHECK(r.box.Inside(kPhone));
}

TEST_CASE("LawnInfoRespectsTheSafeArea") {
	const InfoRect aSafe{60, 0, 1180, 600}; // camera hole and rounded corners
	const LawnInfoPlacement p = PlaceLawnInfo(70, 400, 300, 120, aSafe);
	CHECK(p.box.x == 60 + kLawnInfoMargin);
	CHECK(p.box.Inside(aSafe));
}

TEST_CASE("LawnInfoJustFitsAbove") {
	const int aY = kLawnInfoMargin + 120 + kLawnInfoFingerGap; // the box's top lands exactly on the margin
	const LawnInfoPlacement p = PlaceLawnInfo(650, aY, 300, 120, kPhone);
	CHECK(p.side == LawnInfoSide::Above);
	CHECK(p.box.y == kLawnInfoMargin);
	CHECK(PlaceLawnInfo(650, aY - 1, 300, 120, kPhone).side != LawnInfoSide::Above);
}

TEST_CASE("LawnInfoTopRowGoesToTheSideTowardTheScreenCentre") {
	// left half: the hand comes from the left, the box goes right of the finger
	const LawnInfoPlacement l = PlaceLawnInfo(400, 130, 300, 160, kPhone);
	CHECK(l.side == LawnInfoSide::Right);
	CHECK(l.box.x == 400 + kLawnInfoSideGap);
	CHECK(l.box.Inside(kPhone));
	CHECK_FALSE(Overlaps(l.box, Finger(400, 130)));
	CHECK(l.box.y == 130 - 80); // centred on the touch
	CHECK(PlaceLawnInfo(400, 40, 300, 160, kPhone).box.y == kLawnInfoMargin); // clamped to the top
	// right half: left of the finger
	const LawnInfoPlacement r = PlaceLawnInfo(900, 130, 300, 160, kPhone);
	CHECK(r.side == LawnInfoSide::Left);
	CHECK(r.box.x + r.box.w == 900 - kLawnInfoSideGap);
	CHECK(r.box.Inside(kPhone));
	CHECK_FALSE(Overlaps(r.box, Finger(900, 130)));
}

TEST_CASE("LawnInfoSideIsCentredOnTheTouchWhenThereIsRoom") {
	const LawnInfoPlacement p = PlaceLawnInfo(400, 250, 300, 300, kPhone); // too tall for above
	CHECK(p.side == LawnInfoSide::Right);
	CHECK(p.box.y == 250 - 150);
	CHECK(p.box.Inside(kPhone));
}

TEST_CASE("LawnInfoSideIsClampedToTheBottom") {
	const LawnInfoPlacement p = PlaceLawnInfo(400, 580, 300, 600 - 2 * kLawnInfoMargin, kPhone);
	CHECK(p.side == LawnInfoSide::Right);
	CHECK(p.box.Inside(kPhone));
	CHECK(p.box.y == kLawnInfoMargin);
}

TEST_CASE("LawnInfoSideTooWideIsClampedInsideTheScreen") {
	const InfoRect aNarrow{0, 0, 800, 600};
	const LawnInfoPlacement p = PlaceLawnInfo(380, 100, 400, 160, aNarrow); // left half; right has 800-8-436 = 356
	CHECK(p.side == LawnInfoSide::Right);
	CHECK(p.box.Inside(aNarrow));
	CHECK(p.box.x == 800 - kLawnInfoMargin - 400);
}

TEST_CASE("LawnInfoSideIsTheRoomierOne") {
	CHECK(PlaceLawnInfo(600, 100, 600, 160, kPhone).side == LawnInfoSide::Right);
	CHECK(PlaceLawnInfo(700, 100, 600, 160, kPhone).side == LawnInfoSide::Left);
	const LawnInfoPlacement v = PlaceLawnInfo(200, 100, 650, 160, kPhone);
	CHECK(v.side == LawnInfoSide::Right);
	CHECK_FALSE(Overlaps(v.box, Finger(200, 100)));
	const InfoRect aOffset{300, 0, 1000, 600}; // the centre is 800, not 650
	CHECK(PlaceLawnInfo(700, 100, 300, 160, aOffset).side == LawnInfoSide::Right);
}

TEST_CASE("LawnInfoNeverUnderTheFinger") {
	for (int y = 0; y <= 600; y += 25)
		for (int x = 0; x <= 1300; x += 50) {
			const LawnInfoPlacement p = PlaceLawnInfo(x, y, 320, 180, kPhone);
			CAPTURE(x);
			CAPTURE(y);
			CHECK(p.box.Inside(kPhone));
			if (p.side == LawnInfoSide::Above)
				CHECK(p.box.y + p.box.h <= y - kLawnInfoFingerGap);
			else
				CHECK_FALSE(Overlaps(p.box, Finger(x, y)));
		}
}

// --- box size ------------------------------------------------------------------------------------------------------
TEST_CASE("LawnInfoBoxSizeFitsTitleAndTwoColumns") {
	// title 180 px; label column 120, value column 140
	const InfoRect b = LawnInfoBoxSize(180, 120, 140, 3);
	CHECK(b.w == 2 * kLawnInfoPad + 120 + kLawnInfoColumnGap + 140);
	CHECK(b.h == 2 * kLawnInfoPad + kLawnInfoTitleH + 3 * kLawnInfoRowH);
	const InfoRect c = LawnInfoBoxSize(500, 50, 60, 1); // a long name sets the width
	CHECK(c.w == 2 * kLawnInfoPad + 500);
	CHECK(LawnInfoBoxSize(0, 0, 0, 0).w >= kLawnInfoMinW);
}

TEST_CASE("LawnInfoTextIsAtLeastTwiceTheOldTooltip") {
	CHECK(kLawnInfoTextScale * kLawnInfoBodyPt >= 2.0f * kOldToolTipPt);
	CHECK(kLawnInfoRowH >= static_cast<int>(kLawnInfoTextScale * kLawnInfoBodyPt));
}

// --- rows ----------------------------------------------------------------------------------------------------------
TEST_CASE("LawnInfoPlantRows") {
	CHECK(Join(PlantInfoRows(300, 300, 0, 0, English())) == "Health=300/300;");
	CHECK(Join(PlantInfoRows(150, 300, 3200, 4000, English())) == "Health=150/300;Pumpkin=3200/4000;");
}

TEST_CASE("LawnInfoZombieRowsPlain") {
	u::ZombieInfo z;
	z.body = z.bodyMax = 270;
	CHECK(Join(ZombieInfoRows(z, English())) == "Health=270/270;"); // no total when it equals the health
}

TEST_CASE("LawnInfoZombieRowsArmourAndTotal") {
	u::ZombieInfo z;
	z.body = z.bodyMax = 270;
	z.helmType = 2;
	z.helm = 900;
	z.helmMax = 1100;
	CHECK(Join(ZombieInfoRows(z, English())) == "Health=270/270;Armour=900/1100;Total=1170/1370;");
}

TEST_CASE("LawnInfoZombieRowsShieldBalloonAndLostHelm") {
	u::ZombieInfo z;
	z.body = 100;
	z.bodyMax = 270;
	z.shieldType = 1;
	z.shield = z.shieldMax = 1100;
	z.flying = z.flyingMax = 20;
	CHECK(Join(ZombieInfoRows(z, English())) == "Health=100/270;Shield=1100/1100;Balloon=20/20;Total=1220/1390;");
	z.shieldType = 0; // the shield fell off: the exe's total leaves it out (0x9555ce)
	CHECK(Join(ZombieInfoRows(z, English())) == "Health=100/270;Shield=1100/1100;Balloon=20/20;Total=120/1390;");
}

TEST_CASE("LawnInfoZombieRowsStatusesInSeconds") {
	u::ZombieInfo z;
	z.body = z.bodyMax = 270;
	z.butterCs = 400;
	z.chillCs = 1000;
	z.iceCs = 35;
	z.weakenCs = 380;
	CHECK(Join(ZombieInfoRows(z, English())) ==
		"Health=270/270;Buttered=4.0 s;Slowed=10.0 s;Frozen=0.4 s;Weakened=3.8 s;");
}

TEST_CASE("LawnInfoSeconds") {
	CHECK(LawnInfoSeconds(0, "s") == "0.0 s");
	CHECK(LawnInfoSeconds(5, "s") == "0.1 s"); // rounds to the nearest tenth
	CHECK(LawnInfoSeconds(1234, "s") == "12.3 s");
	CHECK(LawnInfoSeconds(1000, "") == "10.0");
}

// --- the press: stays until the lift --------------------------------------------------------------------------------
TEST_CASE("LawnHoldKeysDoNotCollide") {
	CHECK(LawnHoldPlantKey(5) != LawnHoldZombieKey(5));
	CHECK(LawnHoldPlantKey(0xFFFFFFFFu) >= 0);
	CHECK(LawnHoldZombieKey(0xFFFFFFFFu) >= 0);
	CHECK(LawnHoldPlantKey(0x00010003u) != kHoldNoTarget);
	CHECK(LawnHoldKeyIsZombie(LawnHoldZombieKey(7)));
	CHECK_FALSE(LawnHoldKeyIsZombie(LawnHoldPlantKey(7)));
}
