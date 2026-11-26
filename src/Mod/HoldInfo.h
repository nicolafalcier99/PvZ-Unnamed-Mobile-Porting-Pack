// PVZU: touch "hold for info" in the seed chooser (phone test, 2026-11-24: "the holding feature on the selecting
// section is a bit tricky since the text is small and hard to read; maybe a better version will be to have the text in
// the center of the screen"). A finger held on a seed packet (an available seed or a card already in the bank) shows a
// big info panel in the middle of the screen: the card art, the name, cost and recharge, and the plant's full text.
// A quick tap still picks the seed (or puts a picked card back), as before; a hold never changes the selection.
// Chooser only: in play a held card is for planting (a design ruling, same day). A mouse keeps the small tooltip.
//
// Pure parts here (hold timing, panel layout, styled text wrapping); the engine side is Mod/Engine/HoldInfo.h.
#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace pvzu {

// --- hold timing ---------------------------------------------------------------------------------------------------
constexpr int kHoldInfoDelay = 40; // updates (100 per second): the panel shows after 0.4 s on the same packet
constexpr int kHoldNoTarget = -1;

// One finger press on a seed packet. The press picks nothing by itself: its lift decides.
class HoldPress {
public:
	enum class Lift {
		None, // nothing happens: a hold (the panel was due or shown), or the finger left the packet
		Tap,  // a quick tap that stayed on its packet: pick the seed (or put the picked card back), as before
	};
	void Down(int theTarget, int theFrame);
	// Each update while the finger is down: what is under it now. Leaving the packet before the panel shows ends the
	// press for good (no panel; the lift does nothing). Once the panel shows it stays, wherever the finger goes, until
	// the lift (which then does nothing).
	void Update(int theFrame, int theTargetUnder);
	Lift Up(int theFrame, int theTargetUnder); // ends the press
	void Reset();
	bool Active() const { return mTarget != kHoldNoTarget; }
	int Target() const { return mTarget; }
	bool PanelShown() const { return Active() && !mLeft && mShown; }

private:
	int mTarget = kHoldNoTarget;
	int mDownFrame = 0;
	bool mShown = false;
	bool mLeft = false;
};

// --- panel layout (screen coordinates, logical px) -----------------------------------------------------------------
struct InfoRect {
	int x = 0, y = 0, w = 0, h = 0;
	bool operator==(const InfoRect&) const = default;
	bool Inside(const InfoRect& o) const { return x >= o.x && y >= o.y && x + w <= o.x + o.w && y + h <= o.y + o.h; }
};
constexpr int kInfoMargin = 16;       // from the safe insets and the screen edges
// The dialog art (LawnDialog's tiles, drawn by the options panel's frame): its dark middle starts about 30 px in at the
// top and left and 44 px at the right; at the bottom a 106 px stone ledge (where vanilla dialogs put their buttons).
constexpr int kInfoPad = 34;          // left and top: from the panel's edge to the content (as the options panel)
constexpr int kInfoPadRight = 58;     // the right border is wider (44 px), and wrapped text keeps clear of it
constexpr int kInfoLedgeH = 106;      // the bottom ledge: the cost and recharge line sits on it
constexpr int kInfoLedgeGap = 10;     // the last text line's distance from the ledge
constexpr int kInfoMaxW = 720;
constexpr int kInfoTitleH = 46;       // the name (DwarvenTodcraft24)
constexpr int kInfoPictureScale = 2;  // the card art at twice the packet's 50 x 70
constexpr int kInfoPictureW = 50 * kInfoPictureScale;
constexpr int kInfoPictureH = 70 * kInfoPictureScale;
constexpr int kInfoGap = 22;          // between the card art and the text column
constexpr int kInfoStatsH = 32;       // cost and recharge line, centred on the ledge's stone
constexpr int kInfoStatsLift = 8;     // the ledge's stone sits a little above its middle (its lower edge is shadow)
constexpr int kInfoWarningH = 30;     // "not recommended for this level" and the like (only when there is one)

struct InfoPanelLayout {
	InfoRect panel;
	InfoRect content; // the dark middle: title, card art, warning and body text
	InfoRect title, picture, warning, body;
	InfoRect stats;   // on the ledge
};
// The text column's width, and the most body text that fits on the screen.
int InfoBodyWidth(int theScreenW, int theSafeLeft, int theSafeRight);
int InfoBodyMaxHeight(int theScreenH, bool theWarning);
// The panel for theBodyH px of body text (clamped to the screen), centred inside the safe insets.
InfoPanelLayout LayoutInfoPanel(int theScreenW, int theScreenH, int theSafeLeft, int theSafeRight, int theBodyH, bool theWarning);

// --- styled text (the game's string codes: {KEYWORD} {STAT} {NORMAL} {FLAVOR} {SHORTLINE} ...) -----------------------
enum class InfoStyle { Normal, Flavor, Keyword, Nocturnal, Aquatic, Stat, Metal, KeyMetal };
struct InfoRun {
	InfoStyle style = InfoStyle::Normal;
	std::string text;
	bool operator==(const InfoRun&) const = default;
};
struct InfoLine {
	std::vector<InfoRun> runs; // empty: a blank line
	bool gap = false;          // a {SHORTLINE}: a half-height gap, no text
};
using InfoMeasure = std::function<int(std::string_view)>; // px width of a string at the font's own size

// Wraps theText to theMaxWidth (the measure's px): breaks at spaces and between CJK characters, a word too long for a
// line is cut. '\n' starts a new line, except inside {FLAVOR} (it joins lines into one paragraph, as the Almanac).
// Gaps and blank lines at the start and end are dropped.
std::vector<InfoLine> WrapInfoText(std::string_view theText, int theMaxWidth, const InfoMeasure& theMeasure);
int InfoTextHeight(const std::vector<InfoLine>& theLines, int theLineH); // a gap counts half a line
// The text up to its {FLAVOR} part (the Almanac's story line), or all of it.
std::string DropFlavor(std::string_view theText);

// Body text drawn at this scale of BrianneTod16 (16 pt -> 19 pt; the small tooltip's Pico129 is 9 pt), never below
// the minimum (17.6 pt).
constexpr float kInfoTextScale = 1.2f;
constexpr float kInfoTextMinScale = 1.1f;
struct InfoFit {
	std::vector<InfoLine> lines;
	float scale = kInfoTextScale;
	bool flavorDropped = false;
	bool clipped = false; // even the shortest version at the smallest size was too tall: the last lines are cut
	int height = 0;       // px on screen
};
// The largest readable version that fits theMaxHeight: the whole text at full size, else without the story line, else
// smaller (down to the minimum), else cut. theLineH: the font's line height at its own size.
InfoFit FitInfoText(std::string_view theText, int theWidth, int theMaxHeight, int theLineH, const InfoMeasure& theMeasure);

} // namespace pvzu
