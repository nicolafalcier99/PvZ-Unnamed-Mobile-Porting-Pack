#include "Mod/OptionsPanel.h"

#include <algorithm>

namespace pvzu {

OptionsPanelLayout LayoutOptionsPanel(int theScreenW, int theScreenH, int theSafeLeft, int theSafeRight, int theRows,
	bool theWithWin, bool theWithSave) {
	OptionsPanelLayout l;
	const int aSafeX = theSafeLeft + kOptionsMargin;
	const int aSafeW = std::max(0, theScreenW - theSafeLeft - theSafeRight - 2 * kOptionsMargin);
	const int aSafeH = std::max(0, theScreenH - 2 * kOptionsMargin);
	const int aInnerW = aSafeW - kOptionsPad - kOptionsPadRight;
	auto aHeight = [](int thePerCol, int theRowH, int theGap) {
		return 2 * kOptionsPad + kOptionsTitleH + thePerCol * (theRowH + theGap) + theGap + theRowH;
	};
	auto aPerColFor = [&](int theCols) { return (theRows + theCols - 1) / theCols; };
	// PVZU R8: up to 3 columns (14 rows); the old 7-8 rows keep 2 columns and 64 + 12 px rows
	int aMaxCols = 1;
	while (aMaxCols < 3 && aInnerW >= (aMaxCols + 1) * kOptionsMinColW + aMaxCols * kOptionsGap)
		aMaxCols++;
	l.columns = std::min(2, aMaxCols);
	while (l.columns < aMaxCols && aHeight(aPerColFor(l.columns), kOptionsRowH, kOptionsGap) > aSafeH)
		l.columns++;
	int aGap = kOptionsGap, aRowH = kOptionsRowH;
	const int aPerCol = aPerColFor(l.columns);
	if (aHeight(aPerCol, aRowH, aGap) > aSafeH)
		aGap = 8;
	if (aHeight(aPerCol, aRowH, aGap) > aSafeH) // a 4:3 window: the rows give way, never below 40 px
		aRowH = std::max(40, (aSafeH - 2 * kOptionsPad - kOptionsTitleH - aPerCol * aGap - aGap) / (aPerCol + 1));
	const int aColW = std::min(kOptionsMaxColW, (aInnerW - (l.columns - 1) * kOptionsGap) / l.columns);
	const int aW = kOptionsPad + kOptionsPadRight + l.columns * aColW + (l.columns - 1) * kOptionsGap;
	const int aH = aHeight(aPerCol, aRowH, aGap);
	l.panel = {aSafeX + (aSafeW - aW) / 2, std::max(kOptionsMargin, (theScreenH - aH) / 2), aW, aH};
	l.title = {l.panel.x + kOptionsPad, l.panel.y + kOptionsPad, aW - kOptionsPad - kOptionsPadRight, kOptionsTitleH - kOptionsGap};
	const int aTop = l.panel.y + kOptionsPad + kOptionsTitleH;
	for (int i = 0; i < theRows; i++) {
		const int aCol = i / aPerCol, aRow = i % aPerCol;
		l.rows.push_back({l.panel.x + kOptionsPad + aCol * (aColW + kOptionsGap), aTop + aRow * (aRowH + aGap), aColW, aRowH});
	}
	// footer: [Win] [Save] [Load] from the left, [Done] on the right
	const int aButtons = 1 + (theWithWin ? 1 : 0) + (theWithSave ? 2 : 0);
	const int aFooterY = l.panel.y + aH - kOptionsPad - aRowH;
	const int aFooterW = std::min({kOptionsFooterW, aColW, (aW - kOptionsPad - kOptionsPadRight - (aButtons - 1) * kOptionsGap) / aButtons});
	l.done = {l.panel.x + aW - kOptionsPadRight - aFooterW, aFooterY, aFooterW, aRowH};
	int aX = l.panel.x + kOptionsPad;
	if (theWithWin) {
		l.win = {aX, aFooterY, aFooterW, aRowH};
		aX += aFooterW + kOptionsGap;
	}
	if (theWithSave) {
		l.save = {aX, aFooterY, aFooterW, aRowH};
		l.load = {aX + aFooterW + kOptionsGap, aFooterY, aFooterW, aRowH};
	}
	return l;
}

bool Inside(const HitRect& r, int x, int y) { return r.w > 0 && x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h; }

int OptionsRowAt(const OptionsPanelLayout& theLayout, int x, int y) {
	for (size_t i = 0; i < theLayout.rows.size(); i++)
		if (Inside(theLayout.rows[i], x, y))
			return static_cast<int>(i);
	return -1;
}

void LongPress::Down(int theTick) {
	mDown = true;
	mFired = false;
	mStart = theTick;
}

bool LongPress::Poll(int theTick, bool theStillOver, int theTicks) {
	if (!mDown || mFired)
		return false;
	if (!theStillOver) { // slid off the button: no long press for this touch
		mDown = false;
		return false;
	}
	if (theTicks <= 0 || theTick - mStart < theTicks)
		return false;
	mFired = true;
	return true;
}

bool LongPress::Up() {
	const bool aFired = mFired;
	mDown = false;
	mFired = false;
	return aFired;
}

} // namespace pvzu
