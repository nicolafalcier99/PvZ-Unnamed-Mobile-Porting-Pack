// PVZU: the Unnamed options panel as a touch screen (the exe opens a 450 x 360 dialog of checkboxes with a right click
// on the Menu button or the seed chooser, hooks 80 and 457; the phone has no right click, so a long press on Menu
// opens it, a right click still does on the Mac). Pure geometry and the long-press timer; Mod/Engine/OptionsPanel
// draws it and applies the options (Mod/Unnamed/Options.h).
//
//   +--------------------------------------------------------------+
//   |                          Options                             |
//   |  +------------------------+   +------------------------+     |
//   |  | Elite zombies  Default |   | Double sun        Off  |     |
//   |  +------------------------+   +------------------------+     |
//   |   ... one big button per option, two columns ...             |
//   |  [ Win this level ] [ Save game ] [ Load game ]  [  Done  ]   |
//   +--------------------------------------------------------------+
//
// Screen coordinates (logical px). The panel stays inside the safe insets (camera hole on the left) and every button is
// at least kMinTouchSize tall.
#pragma once

#include "Mod/Touch.h"

#include <vector>

namespace pvzu {

constexpr int kOptionsMargin = 16;      // from the safe insets and the screen edges
constexpr int kOptionsPad = 34;         // inside the panel frame, left/top/bottom (the dialog art's border)
constexpr int kOptionsPadRight = 46;    // the dialog art's right border is wider
constexpr int kOptionsTitleH = 56;
constexpr int kOptionsRowH = kMinTouchSize;
constexpr int kOptionsGap = 12;         // between buttons
constexpr int kOptionsMaxColW = 380;
constexpr int kOptionsMinColW = 250;
constexpr int kOptionsFooterW = 240;    // "Done" and "Win this level"

struct OptionsPanelLayout {
	HitRect panel;
	HitRect title;
	std::vector<HitRect> rows; // one per option, column by column (first column top to bottom, then the second)
	HitRect done;
	HitRect win;               // w == 0: no "Win this level" button
	HitRect save;              // R8 存档 / 读档 (the exe dialog's buttons 0x26 / 0x27); w == 0: not shown
	HitRect load;
	int columns = 1;
};
// Columns: as few as fit the height (up to 3, R8's 14 rows take 3 on the phone); when even 3 do not fit, the gaps
// close to 8 px and then the rows shrink (a 4:3 window only, never below 40 px).
OptionsPanelLayout LayoutOptionsPanel(int theScreenW, int theScreenH, int theSafeLeft, int theSafeRight, int theRows,
	bool theWithWin, bool theWithSave = false);
// Index of the row under (x, y), -1 for none.
int OptionsRowAt(const OptionsPanelLayout& theLayout, int x, int y);
bool Inside(const HitRect& r, int x, int y);

// Long press: Down when the finger lands on the button, Poll every update with whether it is still on the button,
// Up when it lifts. Poll returns true once, when the press has been held theTicks; Up then returns true (the release
// belongs to the long press, so the button's normal tap action must not run).
class LongPress {
public:
	void Down(int theTick);
	bool Poll(int theTick, bool theStillOver, int theTicks);
	bool Up(); // true: this release ends a long press that fired
	bool Pressed() const { return mDown; }

private:
	bool mDown = false;
	bool mFired = false;
	int mStart = 0;
};

} // namespace pvzu
