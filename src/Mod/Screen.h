// PVZU: screen fitting for the wide layout: viewport letterbox, desktop window size, logical width choice.
#pragma once

namespace pvzu {

struct Viewport { int x = 0, y = 0, w = 0, h = 0; };
struct WindowSize { int w = 0, h = 0; };

// Largest rectangle with the logical aspect, centred in a pixelW x pixelH drawable (bars only if aspects differ).
Viewport ComputeViewport(int pixelW, int pixelH, int logicalW, int logicalH);

// Desktop window for a logical size: 2x if it fits in maxW x maxH, else the largest size that fits
// (same aspect). Unknown display (0) gives the logical size.
WindowSize DesktopWindowSize(int logicalW, int logicalH, int maxW, int maxH);

// Logical width at start-up. forcedWidth > 0 (-pvzu-width) wins, clamped 800..1400. Otherwise phones follow
// the display (landscape), desktop stays 800 (vanilla).
int ChooseLogicalWidth(int forcedWidth, int displayW, int displayH, bool isPhone);

// Screen x of an 800-wide screen centred on a logicalWidth-wide screen.
int CenteredScreenX(int logicalWidth);

} // namespace pvzu
