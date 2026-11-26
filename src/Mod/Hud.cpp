#include "Mod/Hud.h"

#include "Mod/PanelCard.h"

#include <algorithm>

namespace pvzu {

namespace {
// The card column runs from the top margin to the bottom margin.
constexpr int kColumnTop = kHudMargin;
constexpr int kColumnBottom = kLogicalHeight - kHudMargin;
constexpr int kColumnRoom = kColumnBottom - kColumnTop;
constexpr int kColumnRight = kHudMargin + kHudColumnW;
int StackHeight(int n, int h) { return n * h + (n - 1) * kCardGap; }
} // namespace

PanelMode SeedPanelMode(int count) {
	return StackHeight(std::max(count, 1), kCardH) <= kColumnRoom ? PanelMode::WideCards : PanelMode::CompactCards;
}

PanelSlot SeedPanelSlot(int index, int count) {
	PanelSlot s;
	if (SeedPanelMode(count) == PanelMode::WideCards) {
		s.x = kHudMargin;
		s.y = kColumnTop + index * (kCardH + kCardGap);
		return s;
	}
	// Compact cards two per row: a wide card's height while the rows fit, else as tall as they allow.
	const int rows = (count + 1) / 2;
	s.w = kCompactCardW;
	s.h = std::clamp((kColumnRoom - (rows - 1) * kCardGap) / rows, 1, kCardH);
	s.x = kHudMargin + (index % 2) * (kCompactCardW + kCardGap);
	s.y = kColumnTop + (index / 2) * (s.h + kCardGap);
	return s;
}

std::vector<HitRect> SeedPanelHitRects(int count) {
	std::vector<HitRect> rects;
	for (int i = 0; i < count; ++i) {
		const PanelSlot s = SeedPanelSlot(i, count);
		rects.push_back({s.x, s.y, s.w, s.h});
	}
	// Finger-sized where there is room; neighbours split the gap at its midpoint; never into the top bar.
	std::vector<HitRect> hits = ExpandHitRects(rects, kMinTouchSize);
	for (HitRect& r : hits) {
		const int x0 = std::max(r.x, 0), x1 = std::min(r.x + r.w, kColumnRight + kCardGap / 2);
		const int y0 = std::max(r.y, 0), y1 = std::min(r.y + r.h, kLogicalHeight);
		r = {x0, y0, x1 - x0, y1 - y0};
	}
	return hits;
}

PanelPoint SeedPanelArrowOrigin(const PanelSlot& slot) {
	return {slot.x + slot.w / 2 - 25, slot.y + slot.h + 10 - 80};
}

HitRect TopBarRect(HudBox box) {
	const int aSunX = kColumnRight + kCardGap;
	const int aShovelX = aSunX + kSunBoxW + kCardGap;
	const int aSpeedX = aShovelX + kShovelBoxW + kCardGap;
	switch (box) {
	case HudBox::Sun:    return {aSunX, kHudMargin, kSunBoxW, kTopBarH};
	case HudBox::Shovel: return {aShovelX, kHudMargin, kShovelBoxW, kTopBarH};
	case HudBox::Speed:  return {aSpeedX, kHudMargin, kSpeedBoxW, kTopBarH};
	}
	return {};
}

SunBoxParts SunBoxLayout(int w, int h) {
	SunBoxParts p;
	const int aSide = std::max(1, h - 2 * kHudBoxFrame + 4); // the round sun may overlap the frame's inner shadow
	p.icon = {kHudBoxFrame, (h - aSide) / 2, aSide, aSide};
	const int aPaperX = p.icon.x + p.icon.w + 4;
	p.paper = {aPaperX, kHudBoxFrame - 1, std::max(1, w - kHudBoxFrame - aPaperX), std::max(1, h - 2 * kHudBoxFrame + 2)};
	return p;
}

PxRect HudCardFrame(int index, int count) {
	const PanelSlot s = SeedPanelSlot(index, count);
	const Box f = PanelCardLayout(s.w, s.h, 0, 0).frame;
	return {s.x + f.x, s.y + f.y, f.w, f.h};
}

BracketStyle HudBracketStyle(int index, int count, const PlayLayout& layout) {
	auto box = [](HudBox b) {
		const HitRect r = TopBarRect(b);
		return PxRect{r.x, r.y, r.w, r.h};
	};
	const PxRect aTarget = index < 0 ? box(HudBox::Shovel) : HudCardFrame(index, count);
	std::vector<PxRect> aOthers;
	for (int i = 0; i < count; ++i)
		if (i != index)
			aOthers.push_back(HudCardFrame(i, count));
	for (HudBox b : {HudBox::Sun, HudBox::Shovel, HudBox::Speed})
		if (index >= 0 || b != HudBox::Shovel)
			aOthers.push_back(box(b));
	const PxRect aScreen{-layout.safeLeft, 0, layout.width, kLogicalHeight}; // HUD-local: x 0 is the left safe inset
	const BracketStyle aFull = DefaultBracketStyle(aTarget.w);
	return FitBracketStyle(aTarget.w, BracketRoom(aTarget, aOthers, aScreen, BracketOutset(aFull)));
}

bool HudFits(const PlayLayout& layout) {
	return layout.width > kVanillaWidth &&
		layout.safeLeft + kColumnRight + kCardGap <= layout.boardScreenX + kLawnLeftBoardX;
}

HudSlide HudSlideFor(int bankY, int bankHeight, int safeLeft) {
	if (bankY >= 0 || bankHeight <= 0)
		return {};
	const int aGone = std::min(-bankY, bankHeight);
	const int aCardsTravel = safeLeft + kColumnRight;
	const int aBarTravel = kHudMargin + kTopBarH;
	return {-(aGone * aCardsTravel + bankHeight / 2) / bankHeight, -(aGone * aBarTravel + bankHeight / 2) / bankHeight};
}

int CoinBankVanillaX(int labelRightX, int labelWidth, int bankWidth) {
	return std::max(0, labelRightX - labelWidth - kCoinBankLabelGap - bankWidth);
}

} // namespace pvzu
