#include "Mod/Screen.h"

#include "Mod/Layout.h"

#include <algorithm>
#include <cmath>

namespace pvzu {

Viewport ComputeViewport(int pixelW, int pixelH, int logicalW, int logicalH) {
	if (pixelW <= 0 || pixelH <= 0 || logicalW <= 0 || logicalH <= 0)
		return {};
	Viewport v{0, 0, pixelW, pixelH};
	// Same integer math as upstream's 4:3 letterbox, with the logical aspect instead of 4:3.
	const long long wide = static_cast<long long>(pixelW) * logicalH;
	const long long tall = static_cast<long long>(pixelH) * logicalW;
	if (wide > tall) {
		v.w = static_cast<int>(static_cast<long long>(pixelH) * logicalW / logicalH);
		v.x = (pixelW - v.w) / 2;
	} else if (wide < tall) {
		v.h = static_cast<int>(static_cast<long long>(pixelW) * logicalH / logicalW);
		v.y = (pixelH - v.h) / 2;
	}
	return v;
}

WindowSize DesktopWindowSize(int logicalW, int logicalH, int maxW, int maxH) {
	if (maxW <= 0 || maxH <= 0 || logicalW <= 0 || logicalH <= 0)
		return {logicalW, logicalH};
	if (logicalW * 2 <= maxW && logicalH * 2 <= maxH)
		return {logicalW * 2, logicalH * 2};
	if (static_cast<long long>(maxW) * logicalH <= static_cast<long long>(maxH) * logicalW)
		return {maxW, static_cast<int>(std::lround(static_cast<double>(maxW) * logicalH / logicalW))};
	return {static_cast<int>(std::lround(static_cast<double>(maxH) * logicalW / logicalH)), maxH};
}

int ChooseLogicalWidth(int forcedWidth, int displayW, int displayH, bool isPhone) {
	if (forcedWidth > 0)
		return std::clamp(forcedWidth, kVanillaWidth, kMaxWidth);
	if (!isPhone)
		return kVanillaWidth;
	// The game is landscape-only; a display reported in portrait is turned.
	return ComputeLogicalWidth(std::max(displayW, displayH), std::min(displayW, displayH));
}

int CenteredScreenX(int logicalWidth) {
	return (std::max(logicalWidth, kVanillaWidth) - kVanillaWidth) / 2;
}

} // namespace pvzu
