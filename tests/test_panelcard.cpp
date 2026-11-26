#include "doctest/doctest.h"
#include "Mod/PanelCard.h"
#include "Mod/Hud.h"

#include <cmath>
#include <cstdlib>

using namespace pvzu;

namespace {
bool Inside(const Box& inner, const Box& outer) {
	return inner.x >= outer.x && inner.y >= outer.y && inner.x + inner.w <= outer.x + outer.w &&
		inner.y + inner.h <= outer.y + outer.h;
}
int Right(const Box& b) { return b.x + b.w; }
int Bottom(const Box& b) { return b.y + b.h; }
// Slot sizes: full size, and uniformly smaller ones (the layout must hold for any card the HUD might make).
constexpr int kSizes[][2] = {{kCardW, kCardH}, {100, 59}, {74, 44}, {48, 28}};
// FONT_PICO129 cost text at the font's own size: 7 px per digit, digits 7 px tall (any width must work).
constexpr int kText2W = 14, kText3W = 21, kText4W = 28, kTextH = 7;
} // namespace

TEST_CASE("CardFrameHasTheReferenceProportions") {
	// 2026-11-23: copy the EA mobile card. Its card is 206 x 105 native (aspect 1.96); ours keeps the slot's
	// width and draws a frame of that aspect, centred in the 114 x 67 slot (the slot stays the tap area).
	const PanelCardParts p = PanelCardLayout(kCardW, kCardH, kText3W, kTextH);
	CHECK(p.frame.x == 0);
	CHECK(p.frame.w == kCardW);
	CHECK(p.frame.h == kCardFrameH);
	CHECK(std::abs(static_cast<float>(p.frame.w) / p.frame.h - kCardAspect) < 0.03f);
	CHECK(std::abs(p.frame.y - (kCardH - Bottom(p.frame))) <= 1); // centred
	CHECK(Inside(p.frame, Box{0, 0, kCardW, kCardH}));
}

TEST_CASE("CardPanelInsideOutlineAndRim") {
	const PanelCardParts p = PanelCardLayout(kCardW, kCardH, kText3W, kTextH);
	CHECK(Inside(p.panel, p.frame));
	const int aEdge = static_cast<int>(std::lround(kCardOutline + kCardRim));
	CHECK(p.panel.x - p.frame.x == aEdge);
	CHECK(p.panel.y - p.frame.y == aEdge);
	CHECK(Right(p.frame) - Right(p.panel) == aEdge);
	CHECK(Bottom(p.frame) - Bottom(p.panel) == aEdge);
}

TEST_CASE("CardCostTabIsPartOfTheRimBottomRight") {
	// The reference tab is one white shape with the rim: flush with the frame's right and bottom edges (the art keeps
	// the outline), ~45% of the width and ~42% of the height, its top-left corner rounded by about half its height.
	for (int tw : {kText2W, kText3W, kText4W}) {
		CAPTURE(tw);
		const PanelCardParts p = PanelCardLayout(kCardW, kCardH, tw, kTextH);
		CHECK(Inside(p.tab, p.frame));
		CHECK(Right(p.tab) == Right(p.frame));
		CHECK(Bottom(p.tab) == Bottom(p.frame));
		CHECK(p.tab.w >= p.frame.w * 2 / 5);
		CHECK(p.tab.w <= (tw < kText4W ? p.frame.w / 2 : p.frame.w * 3 / 5)); // 4 digits (none in vanilla) widen it
		CHECK(p.tab.h >= p.frame.h * 2 / 5);
		CHECK(p.tab.h <= p.frame.h / 2);
		CHECK(p.tabRadius >= p.tab.h * 0.4f);
		CHECK(p.tabRadius <= p.tab.h * 0.7f);
		// Text: inside the part of the tab that is inside the outline and rim, right-aligned near the right rim.
		const Box aTabInside{p.tab.x, p.tab.y, p.tab.w - p.frame.w + Right(p.panel) - p.frame.x,
			p.tab.h - p.frame.h + Bottom(p.panel) - p.frame.y};
		CHECK(Inside(p.text, aTabInside));
		CHECK(Right(aTabInside) - Right(p.text) <= 4);
		CHECK(p.text.x - p.tab.x >= p.tabRadius * 0.45f); // clear of the round corner
	}
}

TEST_CASE("CardCostDigitsReadableAtPhoneSize") {
	// Reference digits are ~11-12 px tall at our logical size (20 native px); the vanilla packet font is drawn larger.
	const PanelCardParts p = PanelCardLayout(kCardW, kCardH, kText3W, kTextH);
	CHECK(p.text.h >= 11);
	CHECK(p.text.h <= 13);
	CHECK(p.text.h == static_cast<int>(std::lround(kTextH * p.textScale)));
	CHECK(p.text.w == static_cast<int>(std::lround(kText3W * p.textScale)));
}

TEST_CASE("CardNoCostNoTab") {
	// Conveyor/slot-machine packets draw no cost (those levels keep the top bank anyway).
	const PanelCardParts p = PanelCardLayout(kCardW, kCardH, 0, kTextH);
	CHECK(p.tab.w == 0);
	CHECK(p.tab.h == 0);
}

TEST_CASE("CardPlantClipRisesOverTheTopRim") {
	// The plant may rise over the top rim and the left rim (the "3D" look), never below the panel's bottom.
	const PanelCardParts p = PanelCardLayout(kCardW, kCardH, kText3W, kTextH);
	CHECK(Bottom(p.plantClip) == Bottom(p.panel));
	CHECK(p.plantClip.y < p.frame.y - 4);
	CHECK(p.plantClip.x < p.frame.x);
	CHECK(Right(p.plantClip) <= Right(p.frame));
}

namespace {
// Where a fitted plant's bounds land on the card (card-local px).
struct Placed { float x0, y0, x1, y1; };
Placed Place(const PanelCardParts& p, const PlantBounds& b) {
	return {p.plantX + b.x0 * p.plantScale, p.plantY + b.y0 * p.plantScale, p.plantX + b.x1 * p.plantScale,
		p.plantY + b.y1 * p.plantScale};
}
// Vanilla packet px (as DrawSeedPacket draws the plant): a peashooter-like plant fills its window; a spikeweed-like
// one is a wide strip at the bottom; a tall-nut-like one is narrow.
constexpr PlantBounds kTall{9, 9, 46, 52};
constexpr PlantBounds kFlat{3, 36, 48, 50};
constexpr PlantBounds kNarrow{14, 4, 36, 54};
} // namespace

TEST_CASE("CardPlantFitTallPlantRisesAndIsCutAtTheBottom") {
	// Reference: the head rises a few px over the top rim; the plant is the picture, its feet cut by the panel.
	PanelCardParts p = PanelCardLayout(kCardW, kCardH, kText3W, kTextH);
	FitPanelPlant(p, kTall);
	const Placed q = Place(p, kTall);
	CHECK(q.y0 < p.frame.y - 2.0f);          // rises over the top rim
	CHECK(q.y0 >= p.plantClip.y);            // ... within the clip
	CHECK(q.y1 > Bottom(p.panel) + 8.0f);    // feet cut off
	CHECK(q.y1 - q.y0 < 1.5f * (Bottom(p.panel) - q.y0)); // but most of the plant shows
	const float cx = (q.x0 + q.x1) / 2.0f;
	CHECK(cx > p.frame.w * 0.3f);
	CHECK(cx < p.frame.w * 0.5f);            // left of the middle, clear of the tab
	CHECK(q.x0 >= p.plantClip.x);
	CHECK(q.x1 - q.x0 <= 0.9f * p.frame.w);
}

TEST_CASE("CardPlantFitWideFlatPlantSitsOnThePanelBottom") {
	PanelCardParts p = PanelCardLayout(kCardW, kCardH, kText3W, kTextH);
	FitPanelPlant(p, kFlat);
	const Placed q = Place(p, kFlat);
	CHECK(q.x1 - q.x0 <= 0.9f * p.frame.w + 0.01f); // the width is what limits it
	CHECK(q.x1 - q.x0 >= 0.7f * p.frame.w);
	CHECK(q.x0 >= p.frame.x - 3.01f);   // a leaf may cross the left rim a little ...
	CHECK(q.x0 >= p.plantClip.x + 2.0f); // ... and the clip never cuts its outline (seen on the cherry bomb)
	CHECK(q.y0 > p.panel.y);                       // a short plant does not float at the top
	CHECK(q.y1 >= Bottom(p.panel));                // it stands on the panel's bottom, a little cut
	CHECK(q.y1 - Bottom(p.panel) <= 0.25f * (q.y1 - q.y0));
}

TEST_CASE("CardPlantFitNarrowPlantKeepsItsHeight") {
	PanelCardParts p = PanelCardLayout(kCardW, kCardH, kText3W, kTextH);
	FitPanelPlant(p, kNarrow);
	const Placed q = Place(p, kNarrow);
	CHECK(q.y0 < p.frame.y - 2.0f);
	CHECK(q.y0 >= p.plantClip.y);
	CHECK((q.x0 + q.x1) / 2.0f < p.tab.x);
}

TEST_CASE("CardPlantFitDefaultIsTheVanillaWindow") {
	// Without measured bounds the layout fits the vanilla packet window (where the packet draws its plant).
	const PanelCardParts a = PanelCardLayout(kCardW, kCardH, kText3W, kTextH);
	PanelCardParts b = a;
	FitPanelPlant(b, {static_cast<float>(kPacketWindowSrc.x), static_cast<float>(kPacketWindowSrc.y),
		static_cast<float>(kPacketWindowSrc.x + kPacketWindowSrc.w), static_cast<float>(kPacketWindowSrc.y + kPacketWindowSrc.h)});
	CHECK(a.plantScale == doctest::Approx(b.plantScale));
	CHECK(a.plantX == doctest::Approx(b.plantX));
	CHECK(a.plantY == doctest::Approx(b.plantY));
}

TEST_CASE("CardPlantRiseStaysClearOfTheCardAbove") {
	// The card above ends at its frame bottom: the rise reaches at most into the gap between the frames
	// (slot margins + kCardGap), leaving a pixel.
	for (const auto& s : kSizes) {
		CAPTURE(s[0]);
		const PanelCardParts p = PanelCardLayout(s[0], s[1], kText3W, kTextH);
		const int aBottomMargin = s[1] - Bottom(p.frame);
		CHECK(p.plantClip.y >= -(kCardGap + aBottomMargin) + 1);
	}
}

TEST_CASE("CardShadowsOffsetDownRight") {
	const PanelCardParts p = PanelCardLayout(kCardW, kCardH, kText3W, kTextH);
	// The card's art covers the frame plus room for its soft shadow (mostly below it).
	CHECK(Inside(p.frame, p.art));
	CHECK(Bottom(p.art) - Bottom(p.frame) >= 3);
	CHECK(p.art.x <= p.frame.x);
	CHECK(p.shadowDy > 0.0f);
	CHECK(p.shadowDx >= 0.0f);
	CHECK(p.shadowDx <= p.shadowDy);
	CHECK(p.plantShadowDx > 0.0f);
	CHECK(p.plantShadowDy > 0.0f);
	CHECK(p.plantShadowDy <= 3.0f);
}

TEST_CASE("CardShrinksUniformly") {
	for (const auto& s : kSizes) {
		const int w = s[0], h = s[1];
		CAPTURE(w);
		CAPTURE(h);
		const Box aSlot{0, 0, w, h};
		for (int tw : {kText2W, kText3W, kText4W}) {
			const PanelCardParts p = PanelCardLayout(w, h, tw, kTextH);
			CHECK(Inside(p.frame, aSlot));
			CHECK(Inside(p.panel, p.frame));
			CHECK(Inside(p.tab, p.frame));
			CHECK(Inside(p.text, p.tab));
			CHECK(p.frame.w == w);
			CHECK(std::abs(static_cast<float>(p.frame.w) / p.frame.h - kCardAspect) < 0.1f);
			CHECK(p.k == doctest::Approx(static_cast<float>(p.frame.h) / kCardFrameH));
			CHECK(p.plantScale > 0.0f);
			CHECK(Bottom(p.plantClip) == Bottom(p.panel));
		}
	}
}

// Compact card (9-15 slots, Mod/Hud.h): the same card in a narrow slot, two per row. Frame as tall as the wide
// card's (k = 1: same outline, rim, shadow and rise), the slot's width; cost tab bottom-right with smaller digits.
TEST_CASE("CompactCardFillsTheNarrowSlotAtFullHeight") {
	const PanelCardParts p = PanelCardLayout(kCompactCardW, kCardH, kText3W, kTextH);
	CHECK(p.compact);
	CHECK(p.frame.x == 0);
	CHECK(p.frame.w == kCompactCardW);
	CHECK(p.frame.h == kCardFrameH);
	CHECK(p.k == doctest::Approx(1.0f));
	CHECK(std::abs(p.frame.y - (kCardH - Bottom(p.frame))) <= 1); // centred like the wide card
	CHECK(Inside(p.frame, Box{0, 0, kCompactCardW, kCardH}));
	CHECK(Inside(p.panel, p.frame));
	CHECK(p.panel.x - p.frame.x == static_cast<int>(std::lround(kCardOutline + kCardRim))); // same rim
	CHECK_FALSE(PanelCardLayout(kCardW, kCardH, kText3W, kTextH).compact);
}

TEST_CASE("CompactCardCostTabBottomRightAndReadable") {
	for (int tw : {kText2W, kText3W, kText4W}) {
		CAPTURE(tw);
		const PanelCardParts p = PanelCardLayout(kCompactCardW, kCardH, tw, kTextH);
		CHECK(Inside(p.tab, p.frame));
		CHECK(Right(p.tab) == Right(p.frame));
		CHECK(Bottom(p.tab) == Bottom(p.frame));
		CHECK(p.tab.h <= p.frame.h / 3);           // leaves the card's upper two thirds to the plant
		CHECK(p.text.h >= 9);                      // 9+ logical px = 16+ device px on the S26 Ultra
		CHECK(p.text.h <= 11);
		const Box aTabInside{p.tab.x, p.tab.y, p.tab.w - p.frame.w + Right(p.panel) - p.frame.x,
			p.tab.h - p.frame.h + Bottom(p.panel) - p.frame.y};
		CHECK(Inside(p.text, aTabInside));
		CHECK(p.text.x - p.tab.x >= p.tabRadius * 0.45f);
		if (tw <= kText3W)
			CHECK(p.tab.w <= p.frame.w * 3 / 4);   // 3 digits (every vanilla cost): the left quarter stays panel
	}
}

TEST_CASE("CompactCardPlantRisesCentredAndKeepsClearOfTheCardAbove") {
	PanelCardParts p = PanelCardLayout(kCompactCardW, kCardH, kText3W, kTextH);
	CHECK(Bottom(p.plantClip) == Bottom(p.panel));
	CHECK(p.plantClip.y < p.frame.y - 4);
	CHECK(p.plantClip.y >= -(kCardGap + kCardH - Bottom(p.frame)) + 1);
	CHECK(Right(p.plantClip) <= Right(p.frame));
	for (const PlantBounds& b : {kTall, kFlat, kNarrow}) {
		FitPanelPlant(p, b);
		const Placed q = Place(p, b);
		const float cx = (q.x0 + q.x1) / 2.0f;
		CHECK(cx > p.frame.w * 0.35f);             // the card is narrow: the plant stands in the middle
		CHECK(cx < p.frame.w * 0.6f);
		CHECK(q.x1 - q.x0 <= 0.9f * p.frame.w + 0.01f);
		CHECK(q.x0 >= p.plantClip.x + 2.0f);
		CHECK(q.y0 >= p.plantClip.y);
		CHECK(q.y0 < static_cast<float>(p.tab.y)); // its head is above the cost tab
	}
	FitPanelPlant(p, kTall);
	const Placed t = Place(p, kTall);
	CHECK(t.y0 < p.frame.y - 2.0f);                // a tall plant rises over the top rim, like on the wide card
}
