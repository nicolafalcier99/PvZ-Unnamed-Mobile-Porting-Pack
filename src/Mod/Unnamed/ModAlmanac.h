// PVZU: an in-game guide for Unnamed's 26 mod-only plants (port-only, feature "modAlmanac"; not in the exe: its Plant
// Almanac lists only the 49 vanilla plants). Pure parts: the guide's text file, the "More plants" page layout, and
// when a context hint shows on the lawn. The engine side is Mod/Engine/ModAlmanac.h.
//
// Text: the pack's strings/mod-almanac-en.txt (optional strings/mod-almanac-zh.txt for Chinese), in the game's string
// file shape ("[KEY]" line, then the text until the next key; lines starting with '#' are comments):
//   [MOD_PLANT_0x50_DESCRIPTION]  what the plant does (Almanac page; may use {KEYWORD} {STAT} {NORMAL} {SHORTLINE})
//   [MOD_PLANT_0x50_HOW_TO_USE]   one line: how to use it (Almanac page)
//   [MOD_HINT_PLANTED_0x5d]       shown once per level when the player plants that plant
//   [MOD_HINT_SHADOW_0x50]        shown instead when it is planted without enough Moonflowers for its shadow state
//   [MOD_HINT_TAP_0x7f]           shown when the player taps that plant while it cannot act (unloaded, no sun)
//   [MOD_HINT_REFUSED_0x5b]       replaces the Coffee Bean message when that plant cannot go there
//   [MOD_ALMANAC_...]             the page's own words (button labels, "How to use:", seconds)
// A missing or empty key = no text (a hint is then simply not shown). Rules: data/rules-mod-almanac.json
// ("modAlmanac.hints" 0 turns every lawn hint off; "modAlmanac.pages" 0 hides the Almanac page).
#pragma once

#include <bitset>
#include <cstdint>
#include <map>
#include <string>
#include <string_view>

namespace pvzu::unnamed {

// --- text ---------------------------------------------------------------------------------------------------------
using ModAlmanacTexts = std::map<std::string, std::string, std::less<>>;
// Parses the text file. false (and *err) on text before the first key or a malformed key line.
bool ParseModAlmanacText(std::string_view theText, ModAlmanacTexts* out, std::string* err);
// "MOD_PLANT_0x50_DESCRIPTION", "MOD_PLANT_0x50_HOW_TO_USE"
std::string ModPlantTextKey(int theSeed, std::string_view theWhat);
// "MOD_HINT_PLANTED_0x5d" (theKind: PLANTED, SHADOW, TAP, REFUSED)
std::string ModHintKey(std::string_view theKind, int theSeed);
// The recharge shown on a mod plant's page, in seconds: 1500 -> "15", 750 -> "7.5", 4250 -> "42.5".
std::string ModAlmanacRechargeSeconds(int theRefreshCs);

// --- page layout (Almanac dialog coordinates: 800 x 600, the dialog is centred on the wide screen) -----------------
struct AlmanacRect {
	int x = 0, y = 0, w = 0, h = 0;
	bool Contains(int px, int py) const { return px >= x && px < x + w && py >= y && py < y + h; }
	bool Overlaps(const AlmanacRect& o) const { return x < o.x + o.w && o.x < x + w && y < o.y + o.h && o.y < y + h; }
	bool Inside(const AlmanacRect& o) const { return x >= o.x && y >= o.y && x + w <= o.x + o.w && y + h <= o.y + o.h; }
};
inline constexpr int kModAlmanacColumns = 6, kModAlmanacRows = 5;
AlmanacRect ModAlmanacGridArea();            // the vanilla grid's frame on IMAGE_ALMANAC_PLANTBACK (covered by a panel)
AlmanacRect ModAlmanacCell(int theIndex);    // touch cell of the theIndex-th mod plant (row-major, 6 columns)
AlmanacRect ModAlmanacPacket(int theIndex);  // the packet drawn in that cell (scaled)
float ModAlmanacPacketScale();               // 1.2: packets bigger than vanilla's 50 x 70
int ModAlmanacCellAt(int x, int y, int theCount); // index of the cell under (x, y), or -1
AlmanacRect ModAlmanacButton();              // the "More plants" / "Back" button image, bottom centre
AlmanacRect ModAlmanacButtonHit();           // its touch rect (taller than the 26 px image)
AlmanacRect ModAlmanacIndexButton();         // vanilla buttons, for overlap checks
AlmanacRect ModAlmanacCloseButton();
AlmanacRect ModAlmanacCard();                // the vanilla plant card (image at 459, 86)

// --- lawn hints ---------------------------------------------------------------------------------------------------
enum class PlantedHint { None, Planted, Shadow };
// Which hint a freshly planted mod plant gets: the shadow one when it has that text and stands without enough shadow
// points (shadowPoints < threshold, threshold > 0), else the planted one when it has that text.
PlantedHint ChoosePlantedHint(bool theHasShadowText, int theShadowPoints, int theShadowThreshold, bool theHasPlantedText);
// Tapping a Loaded Cannon (0x7f) that waits for a load (cob state ARMING, 0x23).
bool CannonTapNeedsHint(int theModSeed, int theCobState);
// Tapping a Prism-shroom (0x5d) that would fire but the player has too little sun.
bool PrismTapNeedsHint(int theModSeed, bool theClickable, bool theCanPay);
// The Coffee-Bean refusal (PLANTING_NEEDS_SLEEPING) for a Magic Devourer (0x5b) or Strong Coffee Bean (0x73) in the
// cursor gets that plant's own text instead.
bool RefusalHasOwnHint(int theCursorSeed, bool theNeedsSleepingReason);
// A context hint may replace what the message bar shows only when it shows nothing or another hint (never a tutorial,
// a level message or the huge-wave banner).
bool HintMayShow(bool theMessageShown, bool theShownIsHint);

// Once per level per plant: a new level (theLevelId changes) forgets what was shown.
class HintLatch {
public:
	bool Take(std::uintptr_t theLevelId, int theSeed);
private:
	std::uintptr_t mLevel = 0;
	std::bitset<256> mShown;
};

} // namespace pvzu::unnamed
