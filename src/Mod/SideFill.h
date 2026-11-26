// PVZU: filling the sides of an 800-wide screen on a wider display (spec item 5: "sides filled with a blurred/extended
// copy of the same screen, no black bars"). The renderer (GLInterface.cpp, PvzuDrawSideFill) copies the finished
// centred screen, shrinks it to a small texture, blurs it both ways and draws it over each side, mirrored at the seam
// and graded darker/greyer so the real screen stays the focus. This file holds the pure numbers.
#pragma once

#include <vector>

namespace pvzu {

struct SideFillPart {
	int dstX = 0, dstW = 0;                    // logical side area (full height)
	float uAtDstX = 0.0f, uAtDstEnd = 0.0f;    // screen u (0..1 across the 800 screen) at dstX and at dstX + dstW
};

struct SideFillPlan {
	bool active = false;
	SideFillPart left, right;
};

// Sides of the centred 800 screen, each showing the screen's edge region mirrored at the seam, 1:1 (no stretch).
SideFillPlan PlanSideFill(int logicalWidth);

struct SideFillSize { int w = 0, h = 0; };

// Sizes of the halving steps from a w x h copy down to a width <= maxW (each step rounds up, never below 1).
std::vector<SideFillSize> SideFillShrinkChain(int w, int h, int maxW);

// Blur radius (Gaussian sigma) in logical px, and in texels of a texture `texW` wide that holds the whole 800 screen.
inline constexpr float kSideFillBlurLogicalPx = 18.0f;
float SideFillBlurSigma(int texW);

// One separable blur pass: 9 texel weights read as a centre tap and 2 linear-filtered taps each side.
struct SideFillBlurTaps {
	float offset[3] = {};
	float weight[3] = {};
};
SideFillBlurTaps SideFillBlurTapsFor(float sigma);

// Grade: brightness next to the screen, brightness further out (reached kSideFillFeatherPx from the seam), saturation.
inline constexpr float kSideFillShadeSeam = 0.85f;
inline constexpr float kSideFillShadeOuter = 0.6f;
inline constexpr float kSideFillFeatherPx = 60.0f;
inline constexpr float kSideFillSaturation = 0.7f;
float SideFillShade(float logicalPxFromSeam);

// A widget wider than 800 (the seed chooser) draws its own sides only while it covers at least half of the screen's
// height (logical px); parked below the screen (every game makes the chooser and leaves it there) it does not count.
bool ScreenWideWidgetOwnsSides(int widgetY, int widgetH, int screenH);

} // namespace pvzu
