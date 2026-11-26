#include "Mod/Layout.h"

#include "Mod/Screen.h"

#include <algorithm>
#include <cmath>

namespace pvzu {

int ComputeLogicalWidth(int screenW, int screenH) {
	if (screenW <= 0 || screenH <= 0)
		return kVanillaWidth;
	const int w = static_cast<int>(std::lround(static_cast<double>(kLogicalHeight) * screenW / screenH));
	return std::clamp(w, kVanillaWidth, kMaxWidth);
}

PlayLayout ComputePlayLayout(int logicalWidth) {
	PlayLayout l;
	l.width = std::clamp(logicalWidth, kVanillaWidth, kMaxWidth);
	const int extra = l.width - kVanillaWidth;
	l.houseExtra = std::min(kBackgroundLeftSpare, extra);
	l.leftExtra = l.houseExtra;
	l.rightExtra = extra - l.leftExtra;
	l.boardScreenX = l.leftExtra;
	return l;
}

int VisibleRightBoardX(const PlayLayout& layout) {
	return kVanillaWidth + layout.rightExtra;
}

int AnchorX(int vanillaX, Anchor anchor, const PlayLayout& layout) {
	switch (anchor) {
	case Anchor::Left:   return vanillaX + layout.safeLeft;
	case Anchor::Right:  return vanillaX + (layout.width - kVanillaWidth) - layout.safeRight;
	case Anchor::Center: return vanillaX + (layout.width - kVanillaWidth) / 2;
	case Anchor::Board:  return vanillaX + layout.boardScreenX;
	}
	return vanillaX;
}

BoardCamera ComputeBoardCamera(const PlayLayout& layout) {
	constexpr int kBackgroundRightEdge = 1180; // BOARD_IMAGE_WIDTH_OFFSET: board x of the background's right edge
	BoardCamera c;
	c.rest = layout.boardScreenX;
	c.house = kBackgroundLeftSpare; // background's left edge on the screen's left edge
	c.street = layout.width - kBackgroundRightEdge;
	return c;
}

bool ShouldSmoothUpscale(int viewportH, int logicalH) {
	if (viewportH <= 0 || logicalH <= 0)
		return false;
	const double s = static_cast<double>(viewportH) / logicalH;
	return s > 1.0 + 1e-6 && std::abs(s - std::round(s)) > 0.01;
}

int ChooserScreenX(const PlayLayout& layout) {
	return layout.width > kVanillaWidth ? layout.safeLeft : 0;
}

int ChooserRightButtonX(int vanillaX, const PlayLayout& layout) {
	return vanillaX + (layout.width - kVanillaWidth) - ChooserScreenX(layout) - (layout.width > kVanillaWidth ? layout.safeRight : 0);
}

int ChooserMouseX(int screenX, const PlayLayout& layout) {
	return screenX - ChooserScreenX(layout);
}

int CrazyDaveScreenX(bool wideBoard, const PlayLayout& layout) {
	return wideBoard ? 0 : CenteredScreenX(layout.width);
}

int ScreenRightBoardX(const PlayLayout& layout) {
	return layout.width - layout.boardScreenX;
}

int BoardLocalX(int vanillaX, Anchor anchor, const PlayLayout& layout) {
	return AnchorX(vanillaX, anchor, layout) - layout.boardScreenX;
}

float CollectDrawShift(float fullShift, float startDist, float curDist) {
	if (startDist <= 0.0f)
		return fullShift;
	const float aProgress = std::clamp(1.0f - curDist / startDist, 0.0f, 1.0f);
	return fullShift * aProgress;
}

namespace {
PlayLayout gCurrent{};
}

const PlayLayout& CurrentLayout() { return gCurrent; }
void SetCurrentLayout(const PlayLayout& layout) { gCurrent = layout; }

} // namespace pvzu
