#include "Mod/SideFill.h"

#include "Mod/Screen.h"

#include <algorithm>
#include <cmath>

namespace pvzu {

SideFillPlan PlanSideFill(int logicalWidth) {
	SideFillPlan p;
	const int aLeftW = CenteredScreenX(logicalWidth);
	const int aRightX = aLeftW + 800;
	const int aRightW = logicalWidth - aRightX;
	if (aLeftW <= 0 && aRightW <= 0)
		return p;
	p.active = true;
	p.left = {0, aLeftW, std::min(aLeftW, 800) / 800.0f, 0.0f};
	p.right = {aRightX, aRightW, 1.0f, 1.0f - std::min(aRightW, 800) / 800.0f};
	return p;
}

std::vector<SideFillSize> SideFillShrinkChain(int w, int h, int maxW) {
	std::vector<SideFillSize> out;
	while (w > maxW && w > 1) {
		w = (w + 1) / 2;
		h = std::max(1, (h + 1) / 2);
		out.push_back({w, h});
	}
	return out;
}

float SideFillBlurSigma(int texW) {
	return kSideFillBlurLogicalPx * static_cast<float>(texW) / 800.0f;
}

SideFillBlurTaps SideFillBlurTapsFor(float sigma) {
	float g[5];
	float aSum = 0.0f;
	for (int i = 0; i < 5; ++i) {
		g[i] = std::exp(-static_cast<float>(i * i) / (2.0f * sigma * sigma));
		aSum += i == 0 ? g[i] : 2.0f * g[i];
	}
	SideFillBlurTaps t;
	t.weight[0] = g[0] / aSum;
	for (int k = 1; k <= 2; ++k) {
		const float a = g[2 * k - 1], b = g[2 * k];
		t.weight[k] = (a + b) / aSum;
		t.offset[k] = static_cast<float>(2 * k - 1) + b / (a + b); // one linear read lands between the two texels
	}
	return t;
}

float SideFillShade(float logicalPxFromSeam) {
	const float t = std::clamp(logicalPxFromSeam / kSideFillFeatherPx, 0.0f, 1.0f);
	return kSideFillShadeSeam + (kSideFillShadeOuter - kSideFillShadeSeam) * t;
}

bool ScreenWideWidgetOwnsSides(int widgetY, int widgetH, int screenH) {
	const int aShown = std::min(widgetY + widgetH, screenH) - std::max(widgetY, 0);
	return screenH > 0 && 2 * aShown >= screenH;
}

} // namespace pvzu
