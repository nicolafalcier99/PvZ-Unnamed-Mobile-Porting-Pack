// PVZU: see HoldInfo.h.
#include "Mod/Engine/HoldInfo.h"

#include "Mod/Engine/ModAlmanac.h"
#include "Mod/Engine/ModPlants.h"
#include "Mod/Engine/OptionsPanel.h"
#include "Mod/HoldInfo.h"
#include "Mod/Layout.h"
#include "Mod/Touch.h"
#include "Mod/Unnamed/ModAlmanac.h"

#include "LawnApp.h"
#include "Resources.h"
#include "Lawn/Board.h"
#include "Lawn/Plant.h"
#include "Lawn/SeedPacket.h"
#include "Lawn/ToolTipWidget.h"
#include "Lawn/Widget/SeedChooserScreen.h"
#include "PvzpLib/PvzpCommon.h"
#include "PvzpLib/PvzpStringFile.h"
#include "graphics/Font.h"
#include "graphics/Graphics.h"
#include "misc/SexyMatrix.h"
#include "widget/WidgetManager.h"

#include <cstdio>
#include <format>
#include <utility>

namespace pvzu::engine {

namespace {

struct Hold {
	HoldPress press;
	int downX = 0, downY = 0, clickCount = 1;
	bool replaying = false; // ChooserHoldUp is replaying vanilla's press: let it through
	bool logged = false;    // "panel" logged for this press
} gHold;

bool TouchPress() { return HoverEndsOnLift(LastPointer()); }

bool VanillaInput(LawnApp* theApp) { return theApp->mPlayingDemoBuffer || theApp->mRecordingDemoBuffer; }

int Frame(SeedChooserScreen* theScreen) { return static_cast<int>(theScreen->mApp->mUpdateCount); }

int SeedUnder(SeedChooserScreen* theScreen, int x, int y) {
	const SeedType aSeed = theScreen->SeedHitTest(x, y);
	return aSeed == SeedType::SEED_NONE ? kHoldNoTarget : static_cast<int>(aSeed);
}

const char* StateName(ChosenSeedState s) {
	switch (s) {
	case ChosenSeedState::SEED_FLYING_TO_BANK: return "flying-to-bank";
	case ChosenSeedState::SEED_IN_BANK: return "in-bank";
	case ChosenSeedState::SEED_FLYING_TO_CHOOSER: return "flying-to-chooser";
	case ChosenSeedState::SEED_IN_CHOOSER: return "in-chooser";
	default: return "hidden";
	}
}

// The panel's colours on the dark dialog middle (the Almanac's dark-blue-on-paper colours would not read there), and
// on the light stone of its bottom ledge.
Sexy::Color StyleColor(InfoStyle s, bool theOnStone) {
	if (theOnStone)
		return s == InfoStyle::Stat || s == InfoStyle::Metal ? Sexy::Color(150, 25, 15) : Sexy::Color(60, 40, 25);
	switch (s) {
	case InfoStyle::Keyword:
	case InfoStyle::KeyMetal: return Sexy::Color(224, 187, 98); // the dialog header's gold
	case InfoStyle::Stat:
	case InfoStyle::Metal: return Sexy::Color(255, 130, 100);
	case InfoStyle::Flavor: return Sexy::Color(205, 195, 175);
	case InfoStyle::Nocturnal: return Sexy::Color(205, 160, 255);
	case InfoStyle::Aquatic: return Sexy::Color(120, 210, 255);
	case InfoStyle::Normal: break;
	}
	return Sexy::Color(245, 240, 225);
}

Sexy::_Font* BodyFont() { return Sexy::FONT_BRIANNETOD16; }
int BodyMeasure(std::string_view s) { return BodyFont()->StringWidth(s); }

// Lines of styled runs at theScale of the body font, top-left at (x, y) in g's space.
void DrawLines(Sexy::Graphics* g, const std::vector<InfoLine>& theLines, int x, int y, float theScale, bool theOnStone = false) {
	Sexy::_Font* f = BodyFont();
	const float aLineH = f->GetLineSpacing() * theScale;
	float aY = static_cast<float>(y);
	for (const InfoLine& aLine : theLines) {
		if (aLine.gap) {
			aY += aLineH / 2;
			continue;
		}
		const float aBaseline = aY + f->GetAscent() * theScale;
		float aX = static_cast<float>(x);
		for (const InfoRun& r : aLine.runs) {
			Sexy::SexyMatrix3 m;
			// matrix drawing ignores the Graphics translation: add it (as DrawSeedPacket's scaled cost does)
			PvzpScaleTransformMatrix(m, aX + g->mTransX, aBaseline + g->mTransY, theScale, theScale);
			PvzpDrawStringMatrix(g, f, m, r.text, StyleColor(r.style, theOnStone));
			aX += f->StringWidth(r.text) * theScale;
		}
		aY += aLineH;
	}
}

std::string Translated(std::string_view s) { return std::string(PvzpStringTranslate(s)); }

int LineWidth(const InfoLine& theLine) {
	int w = 0;
	for (const InfoRun& r : theLine.runs)
		w += BodyMeasure(r.text);
	return w;
}

// Cost and recharge, as the Almanac words them: "Cost: 100", "Recharge: slow" (seconds when the recharge is not one
// of vanilla's three).
std::pair<std::string, std::string> StatsText(SeedChooserScreen* theScreen, SeedType theSeed, SeedType theImitater, SeedType theInfo) {
	std::string aCost = std::to_string(Plant::GetCost(theSeed, theImitater));
	if (theScreen->mBoard && theScreen->mBoard->PlantUsesAcceleratedPricing(theInfo))
		aCost += "+";
	const int aRefresh = Plant::GetRefreshTime(theSeed, theImitater);
	const std::string aWait = aRefresh == 750 ? Translated("[WAIT_TIME_SHORT]") : aRefresh == 3000 ? Translated("[WAIT_TIME_LONG]")
		: aRefresh == 5000 ? Translated("[WAIT_TIME_VERY_LONG]")
		: unnamed::ModAlmanacRechargeSeconds(aRefresh) + " " + ModAlmanacSecondsWord();
	return {std::format("{{KEYWORD}}{}:{{STAT}} {}", Translated("[COST]"), aCost), std::format("{{KEYWORD}}{}:{{STAT}} {}", Translated("[WAIT_TIME]"), aWait)};
}

// The plant's Almanac text (the pack's page for a mod plant): it already says what the tooltip says, in full (Mac
// check 2026-11-24: with both, Snow Pea and Mystic-Ice Shooter said it twice). The tooltip only when there is no page.
std::string BodyText(SeedType theInfo) {
	std::string aPage = IsModSeed(theInfo) ? ModAlmanacPageText(theInfo)
		: Translated(std::format("[{}_DESCRIPTION]", GetPlantDefinition(theInfo).mPlantName));
	if (!aPage.empty() && aPage.front() == '[')
		aPage.clear(); // no such string
	return "{NORMAL}" + (aPage.empty() ? Translated(Plant::GetToolTip(theInfo)) : aPage);
}

} // namespace

bool ChooserHoldDown(SeedChooserScreen* theScreen, SeedType theSeed, int x, int y, int theClickCount) {
	if (gHold.replaying || theSeed == SeedType::SEED_NONE || theClickCount <= 0 || !TouchPress() || VanillaInput(theScreen->mApp))
		return false;
	gHold.press.Down(static_cast<int>(theSeed), Frame(theScreen));
	gHold.downX = x;
	gHold.downY = y;
	gHold.clickCount = theClickCount;
	gHold.logged = false;
	std::fprintf(stderr, "PVZU hold: down seed %d (%s) frame %d\n", static_cast<int>(theSeed), StateName(theScreen->mChosenSeeds[theSeed].mSeedState),
		Frame(theScreen));
	return true;
}

void ChooserHoldUp(SeedChooserScreen* theScreen, int x, int y) {
	if (!gHold.press.Active())
		return;
	const SeedType aSeed = static_cast<SeedType>(gHold.press.Target());
	const HoldPress::Lift aLift = gHold.press.Up(Frame(theScreen), SeedUnder(theScreen, x, y));
	if (aLift == HoldPress::Lift::Tap) {
		gHold.replaying = true;
		theScreen->MouseDown(gHold.downX, gHold.downY, gHold.clickCount); // vanilla's press: pick, put back, buzz...
		gHold.replaying = false;
	}
	std::fprintf(stderr, "PVZU hold: lift seed %d frame %d %s -> %s\n", static_cast<int>(aSeed), Frame(theScreen),
		aLift == HoldPress::Lift::Tap ? "tap" : "hold (no change)", StateName(theScreen->mChosenSeeds[aSeed].mSeedState));
}

void ChooserHoldUpdate(SeedChooserScreen* theScreen) {
	if (!gHold.press.Active())
		return;
	if ((theScreen->mApp->mWidgetManager->mDownButtons & 1) == 0) { // the lift went elsewhere (a dialog took it)
		gHold.press.Reset();
		return;
	}
	gHold.press.Update(Frame(theScreen), SeedUnder(theScreen, theScreen->mLastMouseX, theScreen->mLastMouseY));
	if (gHold.press.PanelShown() && !gHold.logged) {
		gHold.logged = true;
		std::fprintf(stderr, "PVZU hold: panel seed %d frame %d\n", gHold.press.Target(), Frame(theScreen));
	}
}

bool ChooserSmallToolTipShown(SeedChooserScreen* theScreen) { return !TouchPress() || VanillaInput(theScreen->mApp); }

void ChooserHoldReset() { gHold = Hold{}; }

void DrawChooserHoldInfo(SeedChooserScreen* theScreen, Sexy::Graphics* g) {
	if (!gHold.press.PanelShown())
		return;
	const SeedType aSeed = static_cast<SeedType>(gHold.press.Target());
	const ChosenSeed& aChosen = theScreen->mChosenSeeds[aSeed];
	const SeedType aImitater = aSeed == SeedType::SEED_IMITATER ? aChosen.mImitaterType : SeedType::SEED_NONE;
	const SeedType aInfo = aImitater != SeedType::SEED_NONE ? aImitater : aSeed;

	// screen space (the chooser sits right of the left safe inset on a wide screen; on 800 it ignores the insets)
	Sexy::Graphics sg(*g);
	sg.mTransX -= theScreen->mX;
	sg.mTransY -= theScreen->mY;
	const PlayLayout& pl = CurrentLayout();
	const bool aWide = pl.width > kVanillaWidth;
	const int aSafeL = aWide ? pl.safeLeft : 0, aSafeR = aWide ? pl.safeRight : 0;

	std::string aWarning;
	if (theScreen->mToolTip->mVisible && theScreen->mToolTipSeed == static_cast<int>(aSeed) && !theScreen->mToolTip->mWarningText.empty())
		aWarning = Translated(theScreen->mToolTip->mWarningText);
	const int aLineH = BodyFont()->GetLineSpacing();
	const InfoFit aFit = FitInfoText(BodyText(aInfo), InfoBodyWidth(pl.width, aSafeL, aSafeR), InfoBodyMaxHeight(kLogicalHeight, !aWarning.empty()),
		aLineH, BodyMeasure);
	const InfoPanelLayout l = LayoutInfoPanel(pl.width, kLogicalHeight, aSafeL, aSafeR, aFit.height, !aWarning.empty());

	sg.SetColor(Sexy::Color(0, 0, 0, 120)); // dim the chooser behind it
	sg.FillRect(0, 0, pl.width, kLogicalHeight);
	PanelDrawFrame(&sg, l.panel.x, l.panel.y, l.panel.w, l.panel.h);

	Sexy::_Font* tf = Sexy::FONT_DWARVENTODCRAFT24;
	PvzpDrawString(&sg, Plant::GetNameString(aSeed, aImitater), l.title.x + l.title.w / 2, l.title.y + (l.title.h + tf->GetAscent()) / 2 - 4, tf,
		Sexy::Color(224, 187, 98), DS_ALIGN_CENTER);

	{ // the card art, as in the chooser, twice its size
		Sexy::Graphics aCard(sg);
		aCard.SetScale(kInfoPictureScale, kInfoPictureScale, static_cast<float>(l.picture.x), static_cast<float>(l.picture.y));
		DrawSeedPacket(&aCard, static_cast<float>(l.picture.x), static_cast<float>(l.picture.y), aSeed, aImitater, 0, 255, true, false);
	}

	{ // cost and recharge on the ledge's stone, centred, a wide gap between them
		const auto [aCostText, aWaitText] = StatsText(theScreen, aSeed, aImitater, aInfo);
		const int aMaxW = static_cast<int>(l.stats.w / kInfoTextScale) / 2;
		const std::vector<InfoLine> aCost = WrapInfoText(aCostText, aMaxW, BodyMeasure);
		const std::vector<InfoLine> aWait = WrapInfoText(aWaitText, aMaxW, BodyMeasure);
		if (!aCost.empty() && !aWait.empty()) {
			constexpr int kGap = 48;
			const int aCostW = static_cast<int>(LineWidth(aCost.front()) * kInfoTextScale);
			const int aWaitW = static_cast<int>(LineWidth(aWait.front()) * kInfoTextScale);
			const int aX = l.stats.x + (l.stats.w - aCostW - kGap - aWaitW) / 2;
			const int aY = l.stats.y + (l.stats.h - static_cast<int>(BodyFont()->GetLineSpacing() * kInfoTextScale)) / 2;
			DrawLines(&sg, {aCost.front()}, aX, aY, kInfoTextScale, true);
			DrawLines(&sg, {aWait.front()}, aX + aCostW + kGap, aY, kInfoTextScale, true);
		}
	}
	if (!aWarning.empty()) {
		const std::vector<InfoLine> aWarn = WrapInfoText("{STAT}" + aWarning, static_cast<int>(l.warning.w / kInfoTextScale), BodyMeasure);
		if (!aWarn.empty())
			DrawLines(&sg, {aWarn.front()}, l.warning.x, l.warning.y, kInfoTextScale);
	}
	DrawLines(&sg, aFit.lines, l.body.x, l.body.y, aFit.scale);

	static int sMeasured = -1; // Mac check: once per seed, how the text fitted
	if (sMeasured != static_cast<int>(aSeed)) {
		sMeasured = static_cast<int>(aSeed);
		std::fprintf(stderr, "PVZU hold: panel %dx%d at %d,%d; body %d lines, %dpx of %d, scale %.2f%s%s; glyph ascent %.1f px (tooltip %d)\n",
			l.panel.w, l.panel.h, l.panel.x, l.panel.y, static_cast<int>(aFit.lines.size()), aFit.height,
			InfoBodyMaxHeight(kLogicalHeight, !aWarning.empty()), aFit.scale, aFit.flavorDropped ? ", story dropped" : "",
			aFit.clipped ? ", CLIPPED" : "", BodyFont()->GetAscent() * aFit.scale, Sexy::FONT_PICO129->GetAscent());
	}
}

} // namespace pvzu::engine
