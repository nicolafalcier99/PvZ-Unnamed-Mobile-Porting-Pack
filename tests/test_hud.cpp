#include "doctest/doctest.h"
#include "Mod/Hud.h"
#include "Mod/Layout.h"
#include "Mod/Touch.h"
#include "Mod/TouchPlanting.h"

#include <algorithm>
#include <vector>

using namespace pvzu;

namespace {
bool Apart(const HitRect& a, const HitRect& b) {
	return a.x + a.w <= b.x || b.x + b.w <= a.x || a.y + a.h <= b.y || b.y + b.h <= a.y;
}
HitRect R(const PanelSlot& s) { return {s.x, s.y, s.w, s.h}; }
bool Contains(const HitRect& outer, const HitRect& s) {
	return outer.x <= s.x && outer.y <= s.y && outer.x + outer.w >= s.x + s.w && outer.y + outer.h >= s.y + s.h;
}
constexpr int kMaxPackets = 16; // Unnamed allows up to 15 seed slots
constexpr int kColumnLeft = kHudMargin;
constexpr int kColumnRight = kHudMargin + kHudColumnW;
constexpr int kLawnTopY = 80; // LAWN_YMIN: the first row of tiles starts here on every lawn
PlayLayout Phone() {
	PlayLayout l = ComputePlayLayout(1300); // the S26 Ultra test phone: 1300 x 600, camera hole on the left
	l.safeLeft = 57;
	l.safeRight = 70;
	return l;
}
} // namespace

TEST_CASE("HudCardsAreLandscapeCardsOfTheReference") {
	// The reference video: cards 105 x 62 px on a 1200 px wide frame of a 2340 x 1080 screen = 114 x 67 logical.
	CHECK(kCardW == 114);
	CHECK(kCardH == 67);
	CHECK(kCardW > kCardH);
}

TEST_CASE("HudOneColumnOfCardsFromTheTop") {
	for (int n = 1; n <= kMaxPackets && SeedPanelMode(n) == PanelMode::WideCards; ++n) {
		for (int i = 0; i < n; ++i) {
			const PanelSlot s = SeedPanelSlot(i, n);
			CHECK(s.w == kCardW);
			CHECK(s.h == kCardH);
			CHECK(s.x == kColumnLeft);                          // left-aligned right of the safe inset
			CHECK(s.y == kHudMargin + i * (kCardH + kCardGap)); // from the very top, kCardGap apart
			CHECK(s.y + s.h <= kLogicalHeight - kHudMargin);
		}
	}
}

TEST_CASE("HudCardsUntilTheColumnIsFull") {
	for (int n = 1; n <= kMaxPackets; ++n) {
		const bool aFits = kHudMargin + n * kCardH + (n - 1) * kCardGap <= kLogicalHeight - kHudMargin;
		CAPTURE(n);
		CHECK((SeedPanelMode(n) == PanelMode::WideCards) == aFits);
	}
	CHECK(SeedPanelMode(8) == PanelMode::WideCards); // eight cards fit one column
	for (int n = 9; n <= 15; ++n)
		CHECK(SeedPanelMode(n) == PanelMode::CompactCards); // 9-15: compact cards, two per row
}

TEST_CASE("HudCompactCardsTwoPerRowInTheColumn") {
	// 2026-11-23: up to 15 slots, the modern card at every count. 9-15: compact cards, two per row, filling the
	// same column as the wide cards (the top bar keeps its place).
	CHECK(kCompactCardW == (kHudColumnW - kCardGap) / 2);
	for (int n = 9; n <= 15; ++n) {
		REQUIRE(SeedPanelMode(n) == PanelMode::CompactCards);
		const int rows = (n + 1) / 2;
		for (int i = 0; i < n; ++i) {
			const PanelSlot s = SeedPanelSlot(i, n);
			CAPTURE(n);
			CAPTURE(i);
			CHECK(s.w == kCompactCardW);
			CHECK(s.w < s.h);                                  // narrower than tall: drawn as the compact card
			CHECK(s.h >= 44);                                  // finger-sized tap target
			CHECK(s.y == SeedPanelSlot(i - i % 2, n).y);       // two per row
			CHECK(s.x == kColumnLeft + (i % 2) * (kCompactCardW + kCardGap));
			CHECK(s.y == kHudMargin + (i / 2) * (s.h + kCardGap)); // from the top, kCardGap apart
			CHECK(s.x >= kColumnLeft);                         // right of the safe inset (HUD-local x 0)
			CHECK(s.x + s.w <= kColumnRight);                  // inside the card column
			CHECK(s.y + s.h <= kLogicalHeight - kHudMargin);
		}
		// Every card of a count has one size; as tall as a wide card, or as tall as the rows allow.
		const int h = SeedPanelSlot(0, n).h;
		const bool aCapped = h == kCardH;
		const bool aFull = rows * (h + 1) + (rows - 1) * kCardGap > kLogicalHeight - 2 * kHudMargin;
		CHECK((aCapped || aFull));
		CHECK(h <= kCardH);
	}
	CHECK(SeedPanelSlot(14, 15).y + SeedPanelSlot(14, 15).h <= kLogicalHeight - kHudMargin); // 15: 8 rows fit
}

TEST_CASE("HudCompactColumnLeftOfTheTopBar") {
	const HitRect sun = TopBarRect(HudBox::Sun);
	for (int n = 9; n <= 15; ++n)
		for (int i = 0; i < n; ++i) {
			const PanelSlot s = SeedPanelSlot(i, n);
			CHECK(s.x + s.w + kCardGap <= sun.x); // the sun, shovel and speed boxes start right of the column
		}
	const PlayLayout l = Phone();
	CHECK(HudFits(l));
	for (int i = 0; i < 15; ++i)
		CHECK(l.safeLeft + SeedPanelSlot(i, 15).x >= l.safeLeft); // never under the camera hole
}

TEST_CASE("HudSlotsNeverOverlap") {
	for (int n = 1; n <= kMaxPackets; ++n)
		for (int i = 0; i < n; ++i)
			for (int j = i + 1; j < n; ++j)
				CHECK(Apart(R(SeedPanelSlot(i, n)), R(SeedPanelSlot(j, n))));
}

TEST_CASE("HudHitRectsCoverTheirSlotsFingerSizedAndApart") {
	for (int n = 1; n <= kMaxPackets; ++n) {
		const std::vector<HitRect> hits = SeedPanelHitRects(n);
		REQUIRE(static_cast<int>(hits.size()) == n);
		for (int i = 0; i < n; ++i) {
			CAPTURE(n);
			CAPTURE(i);
			CHECK(Contains(hits[i], R(SeedPanelSlot(i, n))));
			CHECK(hits[i].x >= 0);
			CHECK(hits[i].x + hits[i].w <= kColumnRight + kCardGap / 2); // never reaches the top bar
			CHECK(hits[i].y >= 0);
			CHECK(hits[i].y + hits[i].h <= kLogicalHeight);
			for (int j = i + 1; j < n; ++j)
				CHECK(Apart(hits[i], hits[j]));
		}
	}
	for (int n = 9; n <= 15; ++n)
		for (const HitRect& r : SeedPanelHitRects(n)) { // compact cards: finger-sized both ways
			CHECK(r.w >= 56);
			CHECK(r.h >= 64);
		}
}

TEST_CASE("HudTopBarRightOfTheCardsAtTheTopEdge") {
	const HitRect sun = TopBarRect(HudBox::Sun);
	const HitRect shovel = TopBarRect(HudBox::Shovel);
	const HitRect speed = TopBarRect(HudBox::Speed);
	CHECK(sun.x == kColumnRight + kCardGap);  // right of the card column
	CHECK(shovel.x == sun.x + sun.w + kCardGap);
	CHECK(speed.x == shovel.x + shovel.w + kCardGap);
	for (const HitRect& r : {sun, shovel, speed}) {
		CHECK(r.y == kHudMargin);             // at the top edge
		CHECK(r.h == kTopBarH);
		CHECK(r.y + r.h < kLawnTopY);         // above the first row of tiles
		CHECK(r.w >= kMinTouchSize);          // finger-sized
		CHECK(r.h >= 48);
	}
	CHECK(sun.w > shovel.w);                  // the sun counter is the wide one
	CHECK(Apart(sun, shovel));
	CHECK(Apart(shovel, speed));
	for (int n = 1; n <= kMaxPackets; ++n)     // never over a card
		for (int i = 0; i < n; ++i)
			for (const HitRect& r : {sun, shovel, speed})
				CHECK(Apart(r, R(SeedPanelSlot(i, n))));
}

TEST_CASE("HudTopBarClearOfTheMenuButton") {
	const PlayLayout l = Phone();
	const int aMenuLeft = AnchorX(681, Anchor::Right, l); // vanilla Menu button at x 681, kept at the right edge
	const HitRect speed = TopBarRect(HudBox::Speed);
	CHECK(l.safeLeft + speed.x + speed.w + 40 <= aMenuLeft);
}

TEST_CASE("HudFitsTheS26ClearOfTheMowers") {
	const PlayLayout l = Phone();
	CHECK(HudFits(l));
	// The card column's right edge (screen) stays left of the mowers (board x kLawnLeftBoardX).
	CHECK(l.safeLeft + kColumnRight + kCardGap <= l.boardScreenX + kLawnLeftBoardX);
	PlayLayout noHole = ComputePlayLayout(1300);
	CHECK(HudFits(noHole));
}

TEST_CASE("HudNeedsRoomLeftOfTheLawn") {
	CHECK_FALSE(HudFits(ComputePlayLayout(800)));  // vanilla width: vanilla top bank
	CHECK_FALSE(HudFits(ComputePlayLayout(900)));  // 100 px of house: the cards would cover the mowers
	for (int w = 800; w <= 1400; ++w) {
		for (int safe : {0, 30, 57, 90}) {
			PlayLayout l = ComputePlayLayout(w);
			l.safeLeft = safe;
			const bool aRoom = l.safeLeft + kColumnRight + kCardGap <= l.boardScreenX + kLawnLeftBoardX;
			CAPTURE(w);
			CAPTURE(safe);
			CHECK(HudFits(l) == aRoom);
		}
	}
}

TEST_CASE("HudSlidesOutWithTheVanillaBank") {
	// Crazy Dave and the level intro hide vanilla's bank by moving it up (bank y 0 .. -87); the cards slide off
	// the left edge and the top bar off the top edge with the same progress.
	const int H = 87, safe = 57;
	const HudSlide in = HudSlideFor(0, H, safe);
	CHECK(in.cardsX == 0);
	CHECK(in.barY == 0);
	CHECK(HudSlideFor(5, H, safe).cardsX == 0);
	const HudSlide out = HudSlideFor(-H, H, safe);
	CHECK(out.cardsX <= -(safe + kColumnRight)); // fully off the left edge (camera-hole inset included)
	CHECK(out.barY <= -(kHudMargin + kTopBarH)); // fully off the top
	CHECK(HudSlideFor(-H - 40, H, safe).cardsX == out.cardsX);
	int lastX = 1, lastY = 1;
	for (int y = 0; y >= -H; --y) {                 // monotonic, together
		const HudSlide s = HudSlideFor(y, H, safe);
		CHECK(s.cardsX <= lastX);
		CHECK(s.barY <= lastY);
		lastX = s.cardsX;
		lastY = s.barY;
	}
	const HudSlide half = HudSlideFor(-H / 2, H, safe);
	CHECK(half.cardsX < 0);
	CHECK(half.cardsX > out.cardsX);
	CHECK(half.barY < 0);
	CHECK(half.barY > out.barY);
	CHECK(HudSlideFor(-10, 0, safe).cardsX == 0);   // no bank art height: never divides by zero
}

TEST_CASE("HudArrowCentredUnderTheSlot") {
	for (int n : {3, 8, 10}) {
		const PanelSlot s = SeedPanelSlot(1, n);
		const PanelPoint o = SeedPanelArrowOrigin(s);
		CHECK(o.x + 25 == s.x + s.w / 2);   // the vanilla arrow particle sits at a packet's (25, 80)
		CHECK(o.y + 80 == s.y + s.h + 10);
	}
}

TEST_CASE("HudCoinBankLeftOfTheLevelLabel") {
	// Vanilla draws "Level x-y" right-aligned at x 780 (593 beside the progress meter); the coin bank goes just left of it.
	CHECK(CoinBankVanillaX(780, 70, 128) == 780 - 70 - kCoinBankLabelGap - 128);
	CHECK(CoinBankVanillaX(593, 70, 128) + 128 + kCoinBankLabelGap == 593 - 70);
	CHECK(CoinBankVanillaX(593, 900, 128) >= 0);   // a very long label never pushes it off the screen
}

TEST_CASE("HudSunBoxIconAndPaperInsideTheFrame") {
	const HitRect box = TopBarRect(HudBox::Sun);
	const SunBoxParts p = SunBoxLayout(box.w, box.h);
	for (const HitRect& r : {p.icon, p.paper}) {
		CHECK(r.x >= kHudBoxFrame);
		CHECK(r.y >= kHudBoxFrame - 2);
		CHECK(r.x + r.w <= box.w - kHudBoxFrame);
		CHECK(r.y + r.h <= box.h - kHudBoxFrame + 2);
		CHECK(r.w > 0);
		CHECK(r.h > 0);
	}
	CHECK(p.icon.x + p.icon.w <= p.paper.x);     // sun icon on the left, the paper with the number right of it
	CHECK(p.icon.w == p.icon.h);                 // round sun keeps its shape
	CHECK(p.paper.w >= 2 * p.icon.w);            // room for four digits
}

TEST_CASE("HudSelectionBracketsClearTheOtherHudItems") {
	// Phone bug list 2026-11-23: the corner brackets of a selected card ran into the sun box (first card) and over the next
	// compact card, and off the top of the screen. Every card of every slot count, and the shovel box: the brackets
	// with their glow stay off the other cards, the top-bar boxes and the screen's edges.
	for (int safe : {0, 57}) {
		PlayLayout l = Phone();
		l.safeLeft = safe;
		const PxRect screen{-safe, 0, l.width, kLogicalHeight}; // HUD-local
		for (int n = 1; n < kMaxPackets; ++n) {
			std::vector<PxRect> items;
			for (int i = 0; i < n; ++i)
				items.push_back(HudCardFrame(i, n));
			for (HudBox b : {HudBox::Sun, HudBox::Shovel, HudBox::Speed}) {
				const HitRect r = TopBarRect(b);
				items.push_back({r.x, r.y, r.w, r.h});
			}
			for (int i = -1; i < n; ++i) { // -1: the shovel box
				const int self = i < 0 ? n + 1 : i;
				const BracketStyle s = HudBracketStyle(i, n, l);
				CAPTURE(safe);
				CAPTURE(n);
				CAPTURE(i);
				for (const BracketArm& a : CornerBrackets(items[self], s)) {
					const PxRect g{a.orange.x - s.glow, a.orange.y - s.glow, a.orange.w + 2 * s.glow, a.orange.h + 2 * s.glow};
					CHECK(g.x >= screen.x);
					CHECK(g.y >= screen.y);
					CHECK(g.x + g.w <= screen.x + screen.w);
					for (size_t k = 0; k < items.size(); ++k)
						if (static_cast<int>(k) != self)
							CHECK(Apart({g.x, g.y, g.w, g.h}, {items[k].x, items[k].y, items[k].w, items[k].h}));
				}
			}
		}
	}
}

TEST_CASE("HudSelectionBracketsKeepTheirLookWhereThereIsRoom") {
	// A wide card in the middle of the column keeps the full brackets (7 px arms, gap, glow).
	CHECK(HudBracketStyle(2, 6, Phone()) == DefaultBracketStyle(HudCardFrame(2, 6).w));
	// The card frame is the drawn card, inside its slot (Mod/PanelCard.h).
	const PanelSlot s = SeedPanelSlot(2, 6);
	const PxRect f = HudCardFrame(2, 6);
	CHECK(f.x >= s.x);
	CHECK(f.y >= s.y);
	CHECK(f.x + f.w <= s.x + s.w);
	CHECK(f.y + f.h <= s.y + s.h);
}
