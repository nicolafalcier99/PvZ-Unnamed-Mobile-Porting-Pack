#include "Mod/PanelCard.h"

#include "Mod/Hud.h"

#include <algorithm>
#include <cmath>

namespace pvzu {

namespace {
// Reference measurements (EA mobile card, in our logical px at a full-size card).
constexpr float kDigitH = 13.0f;      // cost digit height (reference: 21-24 native px)
constexpr float kTabW = 0.45f;        // tab width, of the frame's
constexpr float kTabH = 0.42f;        // tab height, of the frame's
constexpr float kTabRound = 0.55f;    // its top-left radius, of its height
constexpr float kTextRightPad = 1.5f; // digits to the panel's right edge
constexpr float kShadowDx = 0.5f;     // card shadow (blurred, baked into the frame art)
constexpr float kShadowDy = 2.0f;
constexpr float kShadowBlur = 2.5f;
// Plant fit (FitPanelPlant), from the reference: heads rise ~3-8 px over the top rim; the plant's lower part is cut
// by the panel (the peashooter shows down to its leaves); a wide plant spans ~85% of the card.
constexpr float kPlantTop = 5.0f;     // the plant's top, above the frame
constexpr float kPlantCut = 1.4f;     // the plant's height, of what shows of it (top to the panel's bottom)
constexpr float kPlantMaxW = 0.85f;   // its widest, of the frame's width
constexpr float kPlantFlatCut = 0.15f; // a plant shorter than the card: how much of it the panel's bottom cuts
constexpr float kPlantCentreX = 0.40f; // its middle, of the frame's width
constexpr float kPlantLeft = 3.0f;    // how far a fitted plant may cross the left rim
constexpr float kPlantRise = 12.0f;   // how far the plant may rise above the frame (clip)
constexpr float kPlantSide = 6.0f;    // ... and reach left of it (clip: room for the outline's soft edge)
constexpr float kPlantShadow = 1.5f;  // the plant's shadow offset (down-right)
// Compact card (Hud.h kCompactCardW x kCardH): two per row, so the tab is lower and its digits smaller, and the plant
// stands in the middle on the tab's top (a few px behind it) instead of beside it.
constexpr float kCompactDigitH = 10.0f; // 10 logical px = 18 device px on the S26 Ultra
constexpr float kCompactTabH = 0.30f;
constexpr float kCompactTabRound = 0.45f; // a little tighter than the wide tab's: 3 digits keep the left quarter free
constexpr float kCompactPlantCentreX = 0.47f;
constexpr float kCompactFloorSink = 4.0f; // how far a short plant stands behind the tab's top
int Px(float v) { return static_cast<int>(std::lround(v)); }
int Bottom(const Box& b) { return b.y + b.h; }
int Right(const Box& b) { return b.x + b.w; }
} // namespace

PanelCardParts PanelCardLayout(int slotW, int slotH, int costTextW, int costTextH) {
	PanelCardParts p;
	// Frame: the slot's width at the reference's aspect (or the slot's height, if that is the tighter one), centred.
	p.compact = slotW < slotH;
	int fw = std::max(1, slotW), fh = std::max(1, Px(slotW / kCardAspect));
	if (p.compact) {
		fh = std::clamp(Px(static_cast<float>(slotH) * kCardFrameH / kCardH), 1, std::max(1, slotH));
	} else if (fh > slotH) {
		fh = std::max(1, slotH);
		fw = std::clamp(Px(fh * kCardAspect), 1, std::max(1, slotW));
	}
	p.frame = {0, (slotH - fh) / 2, fw, fh};
	p.k = static_cast<float>(fh) / kCardFrameH;
	const float k = p.k;

	const int aEdge = std::max(1, Px((kCardOutline + kCardRim) * k));
	p.panel = {p.frame.x + aEdge, p.frame.y + aEdge, std::max(1, fw - 2 * aEdge), std::max(1, fh - 2 * aEdge)};

	p.shadowDx = kShadowDx * k;
	p.shadowDy = kShadowDy * k;
	const int aShadowRight = static_cast<int>(std::ceil(p.shadowDx + kShadowBlur * k));
	const int aShadowBelow = static_cast<int>(std::ceil(p.shadowDy + kShadowBlur * k));
	p.art = {p.frame.x - 1, p.frame.y - 1, fw + 1 + aShadowRight, fh + 1 + aShadowBelow};

	if (costTextW > 0 && costTextH > 0) {
		p.textScale = (p.compact ? kCompactDigitH : kDigitH) * k / costTextH;
		const int tw = std::max(1, Px(costTextW * p.textScale)), th = std::max(1, Px(costTextH * p.textScale));
		const int aTabH = std::clamp(Px(fh * (p.compact ? kCompactTabH : kTabH)), 1, fh);
		p.tabRadius = aTabH * (p.compact ? kCompactTabRound : kTabRound);
		const int aRightPad = Right(p.frame) - Right(p.panel) + Px(kTextRightPad * k);
		const int aLeftPad = static_cast<int>(std::ceil(p.tabRadius * 0.5f));
		const int aTabW = std::clamp(std::max(Px(fw * kTabW), tw + aRightPad + aLeftPad), 1, fw);
		p.tab = {Right(p.frame) - aTabW, Bottom(p.frame) - aTabH, aTabW, aTabH};
		const int aInsideH = Bottom(p.panel) - p.tab.y; // the tab's height inside the outline and rim
		p.text = {Right(p.panel) - Px(kTextRightPad * k) - tw, p.tab.y + std::max(0, (aInsideH - th) / 2), tw, th};
	}

	p.plantFloor = static_cast<float>(Bottom(p.panel));
	if (p.compact && p.tab.h > 0)
		p.plantFloor = std::min(p.plantFloor, p.tab.y + kCompactFloorSink * k);
	p.plantShadowDx = kPlantShadow * k;
	p.plantShadowDy = kPlantShadow * k;
	// It rises over the top rim, at most into the gap above (stopping a px short of the card above's frame), reaches
	// a little left of the frame and is cut at the panel's bottom.
	const int aRoomAbove = p.frame.y + kCardGap + (slotH - Bottom(p.frame)) - 1;
	const int aRise = std::min(Px(kPlantRise * k), aRoomAbove);
	const int aSide = Px(kPlantSide * k);
	p.plantClip = {p.frame.x - aSide, p.frame.y - aRise, fw + aSide, Bottom(p.panel) - (p.frame.y - aRise)};
	FitPanelPlant(p, {static_cast<float>(kPacketWindowSrc.x), static_cast<float>(kPacketWindowSrc.y),
		static_cast<float>(kPacketWindowSrc.x + kPacketWindowSrc.w), static_cast<float>(kPacketWindowSrc.y + kPacketWindowSrc.h)});
	return p;
}

void FitPanelPlant(PanelCardParts& p, const PlantBounds& b) {
	const float bw = std::max(1.0f, b.x1 - b.x0), bh = std::max(1.0f, b.y1 - b.y0);
	const float aTop = std::max(static_cast<float>(p.plantClip.y), p.frame.y - kPlantTop * p.k);
	const float aShown = p.plantFloor - aTop;
	// As tall as kPlantCut times what shows of it, unless that makes it wider than kPlantMaxW of the card.
	const float s = std::min(kPlantCut * aShown / bh, kPlantMaxW * p.frame.w / bw);
	const float aW = bw * s, aH = bh * s;
	// Top at aTop; a plant too short for that stands on the panel's bottom instead (kPlantFlatCut of it cut).
	// Compact: most plants are as wide as the narrow card allows; one at least as tall as the room above the tab still
	// rises over the top rim (its feet go behind the tab), only a shorter one stands on the tab.
	const bool aStands = p.compact ? aH < aShown : true;
	const float aY0 = aStands ? std::max(aTop, p.plantFloor + kPlantFlatCut * aH - aH) : aTop;
	const float aCentreX = p.compact ? kCompactPlantCentreX : kPlantCentreX;
	const float aX0 = std::max(p.frame.x - kPlantLeft * p.k, p.frame.x + p.frame.w * aCentreX - aW / 2.0f);
	p.plantScale = s;
	p.plantX = aX0 - b.x0 * s;
	p.plantY = aY0 - b.y0 * s;
}

} // namespace pvzu

namespace pvzu {

float CardFlashAlpha(int ageCs) {
	if (ageCs < 0 || ageCs >= kCardFlashCs)
		return 0.0f;
	return 1.0f - static_cast<float>(ageCs) / kCardFlashCs;
}

} // namespace pvzu
