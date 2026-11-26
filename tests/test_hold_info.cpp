#include "doctest/doctest.h"
#include "Mod/HoldInfo.h"

#include <string>

using namespace pvzu;

namespace {
// 10 px per code point (UTF-8 lead bytes counted), so widths are easy to reason about.
int Mono(std::string_view s) {
	int n = 0;
	for (unsigned char c : s)
		if ((c & 0xC0) != 0x80)
			++n;
	return n * 10;
}
std::string LineText(const InfoLine& l) {
	std::string s;
	for (const InfoRun& r : l.runs)
		s += r.text;
	return s;
}
} // namespace

// --- hold timing ---------------------------------------------------------------------------------------------------
TEST_CASE("HoldQuickTapPicks") {
	HoldPress h;
	h.Down(5, 100);
	CHECK(h.Active());
	h.Update(110, 5);
	CHECK_FALSE(h.PanelShown());
	CHECK(h.Up(115, 5) == HoldPress::Lift::Tap);
	CHECK_FALSE(h.Active());
}

TEST_CASE("HoldShowsPanelAfterDelayAndLiftDoesNothing") {
	HoldPress h;
	h.Down(5, 100);
	h.Update(100 + kHoldInfoDelay - 1, 5);
	CHECK_FALSE(h.PanelShown());
	h.Update(100 + kHoldInfoDelay, 5);
	CHECK(h.PanelShown());
	h.Update(400, 5);
	CHECK(h.PanelShown()); // stays while held
	CHECK(h.Up(401, 5) == HoldPress::Lift::None);
	CHECK_FALSE(h.PanelShown());
	CHECK_FALSE(h.Active());
}

TEST_CASE("HoldDelayIsFortyUpdates") {
	CHECK(kHoldInfoDelay == 40); // 0.4 s at 100 updates per second
}

TEST_CASE("HoldLiftAtDelayWithoutUpdateIsAHold") {
	// The lift arrives before the update that would have shown the panel: still a hold, never a pick.
	HoldPress h;
	h.Down(3, 0);
	CHECK(h.Up(kHoldInfoDelay, 3) == HoldPress::Lift::None);
	h.Down(3, 0);
	CHECK(h.Up(kHoldInfoDelay - 1, 3) == HoldPress::Lift::Tap);
}

TEST_CASE("HoldPanelStaysUntilLiftEvenOffThePacket") {
	// 2026-11-24: once the panel is open it stays still until the finger lifts, wherever the finger goes.
	HoldPress h;
	h.Down(5, 0);
	h.Update(50, 5);
	CHECK(h.PanelShown());
	h.Update(51, 6); // onto the next packet
	CHECK(h.PanelShown());
	CHECK(h.Target() == 5); // still the first plant's panel
	h.Update(52, kHoldNoTarget); // off every packet
	CHECK(h.PanelShown());
	CHECK(h.Up(53, 6) == HoldPress::Lift::None); // and the lift still picks nothing
	CHECK_FALSE(h.PanelShown());
}

TEST_CASE("HoldLeavingBeforeThePanelCancels") {
	HoldPress h;
	h.Down(5, 0);
	h.Update(20, 6); // slid off before the delay: no panel, ever
	h.Update(60, 5);
	CHECK_FALSE(h.PanelShown());
	CHECK(h.Up(61, 5) == HoldPress::Lift::None);
}

TEST_CASE("HoldTapThatSlidesOffPicksNothing") {
	HoldPress h;
	h.Down(5, 0);
	h.Update(5, kHoldNoTarget);
	CHECK(h.Up(8, kHoldNoTarget) == HoldPress::Lift::None);
	h.Down(5, 0);
	CHECK(h.Up(8, 7) == HoldPress::Lift::None); // lifted over another packet
}

TEST_CASE("HoldUpWithoutDownDoesNothing") {
	HoldPress h;
	CHECK(h.Up(10, 5) == HoldPress::Lift::None);
	h.Down(2, 0);
	h.Reset();
	CHECK_FALSE(h.Active());
	CHECK(h.Up(5, 2) == HoldPress::Lift::None);
}

// --- layout --------------------------------------------------------------------------------------------------------
namespace {
void CheckLayout(int w, int h, int sl, int sr, int bodyH, bool warning) {
	CAPTURE(w); CAPTURE(sl); CAPTURE(sr); CAPTURE(bodyH); CAPTURE(warning);
	const InfoPanelLayout l = LayoutInfoPanel(w, h, sl, sr, bodyH, warning);
	const InfoRect safe{sl + kInfoMargin, kInfoMargin, w - sl - sr - 2 * kInfoMargin, h - 2 * kInfoMargin};
	CHECK(l.panel.Inside(safe));
	// centred in the safe area (to a pixel)
	const int midSafe = sl + (w - sl - sr) / 2;
	CHECK(std::abs(l.panel.x + l.panel.w / 2 - midSafe) <= 1);
	CHECK(std::abs(l.panel.y + l.panel.h / 2 - h / 2) <= 1);
	CHECK(l.content.Inside(l.panel));
	CHECK(l.content.x - l.panel.x == kInfoPad);
	CHECK(l.content.y - l.panel.y == kInfoPad);
	CHECK(l.panel.x + l.panel.w - (l.content.x + l.content.w) == kInfoPadRight);
	CHECK(l.panel.y + l.panel.h - (l.content.y + l.content.h) == kInfoLedgeH); // the ledge below the dark middle
	for (const InfoRect& r : {l.title, l.picture, l.body})
		CHECK(r.Inside(l.content));
	if (warning) {
		CHECK(l.warning.Inside(l.content));
		CHECK(l.warning.y + l.warning.h <= l.body.y);
	} else {
		CHECK(l.warning.h == 0);
	}
	CHECK(l.body.y + l.body.h + kInfoLedgeGap <= l.content.y + l.content.h);
	// the cost line on the ledge's stone
	CHECK(l.stats.Inside(l.panel));
	CHECK(l.stats.y >= l.content.y + l.content.h);
	CHECK(l.stats.y + l.stats.h <= l.panel.y + l.panel.h - 14); // above its shadow
	CHECK(l.picture.w == kInfoPictureW);
	CHECK(l.picture.h == kInfoPictureH);
	CHECK(l.body.x >= l.picture.x + l.picture.w + kInfoGap); // text right of the card art
	CHECK(l.body.w == InfoBodyWidth(w, sl, sr));
	CHECK(l.body.h >= std::min(bodyH, InfoBodyMaxHeight(h, warning)));
}
} // namespace

TEST_CASE("InfoPanelFitsPhoneAndVanillaWidths") {
	for (bool warning : {false, true}) {
		for (int bodyH : {0, 60, 200, 380, 2000}) {
			CheckLayout(1300, 600, 57, 70, bodyH, warning); // S26 Ultra (2340 x 1080), camera hole left
			CheckLayout(1300, 600, 0, 0, bodyH, warning);
			CheckLayout(800, 600, 0, 0, bodyH, warning);    // vanilla width
			CheckLayout(1400, 600, 40, 40, bodyH, warning);
		}
	}
}

TEST_CASE("InfoPanelWidthAndGrowth") {
	const InfoPanelLayout a = LayoutInfoPanel(1300, 600, 57, 70, 60, false);
	CHECK(a.panel.w == kInfoMaxW);
	const InfoPanelLayout b = LayoutInfoPanel(1300, 600, 57, 70, 300, false);
	CHECK(b.panel.h > a.panel.h); // grows with the text
	// short text: the card art sets the height
	const InfoPanelLayout c = LayoutInfoPanel(1300, 600, 57, 70, 0, false);
	CHECK(c.panel.h == kInfoPad + kInfoTitleH + kInfoPictureH + kInfoLedgeGap + kInfoLedgeH);
	// too much text: clamped to the screen
	const InfoPanelLayout d = LayoutInfoPanel(1300, 600, 57, 70, 5000, true);
	CHECK(d.panel.h == 600 - 2 * kInfoMargin);
	CHECK(d.body.h == InfoBodyMaxHeight(600, true));
	CHECK(InfoBodyMaxHeight(600, false) == InfoBodyMaxHeight(600, true) + kInfoWarningH);
	CHECK(InfoBodyWidth(1300, 57, 70) == kInfoMaxW - kInfoPad - kInfoPadRight - kInfoPictureW - kInfoGap);
	CHECK(InfoBodyWidth(800, 0, 0) == InfoBodyWidth(1300, 57, 70)); // 800 wide still has room for the full panel
}

TEST_CASE("InfoPanelOnNarrowScreenShrinks") {
	const InfoPanelLayout l = LayoutInfoPanel(640, 480, 0, 0, 100, false);
	CHECK(l.panel.w == 640 - 2 * kInfoMargin);
	CHECK(l.body.w > 100);
}

// --- text ----------------------------------------------------------------------------------------------------------
TEST_CASE("WrapBreaksAtSpaces") {
	const auto lines = WrapInfoText("aaa bbb ccc ddd", 75, Mono);
	REQUIRE(lines.size() == 2);
	CHECK(LineText(lines[0]) == "aaa bbb");
	CHECK(LineText(lines[1]) == "ccc ddd");
	for (const InfoLine& l : lines)
		CHECK(Mono(LineText(l)) <= 75);
}

TEST_CASE("WrapKeepsStylesAndJoinsFlavorLines") {
	const auto lines = WrapInfoText("{SHORTLINE}\n{KEYWORD}Damage:{STAT} normal\n{SHORTLINE}\n{FLAVOR}one\ntwo", 500, Mono);
	REQUIRE(lines.size() == 3);
	REQUIRE(lines[0].runs.size() == 2);
	CHECK(lines[0].runs[0] == InfoRun{InfoStyle::Keyword, "Damage:"});
	CHECK(lines[0].runs[1] == InfoRun{InfoStyle::Stat, " normal"});
	CHECK(lines[1].gap);
	CHECK(LineText(lines[2]) == "one two"); // FLAVOR: the newline is a space
	CHECK(lines[2].runs[0].style == InfoStyle::Flavor);
}

TEST_CASE("WrapNewlinesAndBlankLines") {
	const auto lines = WrapInfoText("one\n\ntwo\n", 500, Mono);
	REQUIRE(lines.size() == 3);
	CHECK(LineText(lines[0]) == "one");
	CHECK(lines[1].runs.empty());
	CHECK_FALSE(lines[1].gap);
	CHECK(LineText(lines[2]) == "two");
}

TEST_CASE("WrapCutsLongWordsAndCjk") {
	const auto a = WrapInfoText("abcdefghij", 40, Mono);
	REQUIRE(a.size() == 3);
	CHECK(LineText(a[0]) == "abcd");
	CHECK(LineText(a[2]) == "ij");
	// Chinese has no spaces: breaks between characters
	const auto b = WrapInfoText("\xE8\xB1\x8C\xE8\xB1\x86\xE5\xB0\x84\xE6\x89\x8B\xE5\x8F\x91\xE5\xB0\x84", 40, Mono); // 6 characters
	REQUIRE(b.size() == 2);
	CHECK(Mono(LineText(b[0])) == 40);
	CHECK(Mono(LineText(b[1])) == 20);
}

TEST_CASE("WrapUnknownCodesAreDropped") {
	const auto lines = WrapInfoText("a{WHATEVER}b {NORMAL}c", 500, Mono);
	REQUIRE(lines.size() == 1);
	CHECK(LineText(lines[0]) == "ab c");
	CHECK(lines[0].runs.back().style == InfoStyle::Normal);
}

TEST_CASE("InfoTextHeightCountsGapsAsHalf") {
	const auto lines = WrapInfoText("a\n{SHORTLINE}\nb", 500, Mono);
	REQUIRE(lines.size() == 3);
	CHECK(InfoTextHeight(lines, 20) == 50);
}

TEST_CASE("DropFlavorKeepsTheStats") {
	CHECK(DropFlavor("{KEYWORD}Damage:{STAT} normal\n{SHORTLINE}\n{FLAVOR}story") == "{KEYWORD}Damage:{STAT} normal\n{SHORTLINE}\n");
	CHECK(DropFlavor("no story") == "no story");
}

TEST_CASE("FitPrefersFullTextAtFullSize") {
	const std::string text = "word word word\n{FLAVOR}story story story story";
	const InfoFit f = FitInfoText(text, 600, 400, 20, Mono);
	CHECK(f.scale == kInfoTextScale);
	CHECK_FALSE(f.flavorDropped);
	CHECK_FALSE(f.clipped);
	CHECK(f.height <= 400);
	for (const InfoLine& l : f.lines)
		CHECK(Mono(LineText(l)) * f.scale <= 600.0f);
}

TEST_CASE("FitDropsTheStoryBeforeShrinking") {
	std::string text = "stat line\n{SHORTLINE}\n{FLAVOR}";
	for (int i = 0; i < 40; ++i)
		text += "story ";
	// 1 line + gap at 24 px = 36 px; the story adds many lines
	const InfoFit f = FitInfoText(text, 600, 60, 20, Mono);
	CHECK(f.flavorDropped);
	CHECK(f.scale == kInfoTextScale);
	CHECK_FALSE(f.clipped);
	CHECK(f.height <= 60);
}

TEST_CASE("FitShrinksThenCuts") {
	// 3 lines: 3 * 20 * 1.2 = 72 px > 70; at 1.1: 66 px fits
	const InfoFit a = FitInfoText("a\nb\nc", 600, 70, 20, Mono);
	CHECK(a.scale == kInfoTextMinScale);
	CHECK_FALSE(a.clipped);
	CHECK(a.height <= 70);
	const InfoFit b = FitInfoText("a\nb\nc\nd\ne", 600, 50, 20, Mono);
	CHECK(b.clipped);
	CHECK(b.scale == kInfoTextMinScale);
	CHECK(b.lines.size() == 2);
	CHECK(b.height <= 50);
}
