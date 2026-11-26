// PVZU: touch "hold for stats" on the lawn (phone test, 2026-11-25: "make a better view for the health and defence
// stats of plants and zombies when holding the finger on them, since it's hard to read because of text size and
// position of the box, move the box on top of the click (finger) so I can read and make the text more readable").
//
// A finger held on a plant or a zombie in play (feature "modVisuals", rule modVisuals.infoHoldTicks) shows a dark
// panel with the name and big two-column stats ("Health 300/300", "Armour 1100/1100" ...) ABOVE the finger, a finger
// height clear of the touch; in the top rows (no room above) it goes beside the finger, toward the screen's centre,
// never under it. It stays, still, until the finger lifts (as the chooser's hold panel, Mod/HoldInfo.h), and the press
// does nothing else: a plant's tap action (Cob Cannon, Prism-shroom, Element-shroom) waits for a quick lift.
// A mouse keeps the exe's small right-button tooltip (Mod/Unnamed/ModVisuals.h).
//
// Pure parts here (placement, box size, the rows); the engine side is Mod/Engine/LawnHoldInfo.h.
#pragma once

#include "Mod/HoldInfo.h"
#include "Mod/Unnamed/ModVisuals.h"

#include <cstdint>
#include <string>
#include <vector>

namespace pvzu {

// --- placement (screen coordinates, logical px) --------------------------------------------------------------------
// The S26 Ultra's 600 logical px are about 73 mm tall (8.2 px per mm): a fingertip (about 9 mm) is about 72 px.
constexpr int kLawnInfoFingerGap = 72;   // from the touch point up to the box's bottom: a finger height
constexpr int kLawnInfoFingerHalfW = 40; // half a finger's width around the touch point
constexpr int kLawnInfoSideGap = 56;     // from the touch point to the box's near edge when it goes to the side
constexpr int kLawnInfoMargin = 8;       // from the (safe) screen edges

enum class LawnInfoSide { Above, Left, Right };
struct LawnInfoPlacement {
	InfoRect box;
	LawnInfoSide side = LawnInfoSide::Above;
};
// Above the touch with kLawnInfoFingerGap, centred on it and clamped inside theScreen (the safe area). When it does
// not fit above: beside the touch on the roomier side (toward theScreen's centre), centred on the touch's height and
// clamped inside theScreen.
LawnInfoPlacement PlaceLawnInfo(int theTouchX, int theTouchY, int theBoxW, int theBoxH, const InfoRect& theScreen);

// --- box size ------------------------------------------------------------------------------------------------------
constexpr int kLawnInfoPad = 16;       // panel edge to content
constexpr int kLawnInfoTitleH = 34;    // the name (DwarvenTodcraft24, gold)
constexpr int kLawnInfoRowH = 38;      // one stats row (BrianneTod16 at 1.5 spaces 34.5 px)
constexpr int kLawnInfoColumnGap = 24; // between the label column and the value column
constexpr int kLawnInfoMinW = 160;
constexpr int kLawnInfoRadius = 10;    // rounded corners
// Rows: BrianneTod16 at 1.5 (24 pt); the exe's tooltip label is Pico129 (9 pt).
constexpr float kLawnInfoBodyPt = 16.0f;
constexpr float kLawnInfoTextScale = 1.5f;
constexpr float kOldToolTipPt = 9.0f;
// w and h of the panel for a title theTitleW px wide and theRows rows of label / value columns (x, y = 0).
InfoRect LawnInfoBoxSize(int theTitleW, int theLabelColW, int theValueColW, int theRows);

// --- the rows ------------------------------------------------------------------------------------------------------
struct LawnInfoRow {
	std::string label, value;
};
struct LawnInfoText { // short labels, from the pack's strings when it has them
	std::string health, armour, shield, balloon, total, pumpkin, buttered, slowed, frozen, weakened, seconds;
};
// "Health 150/300", then "Pumpkin 3200/4000" when a Pumpkin shields it (thePumpkinMax > 0).
std::vector<LawnInfoRow> PlantInfoRows(int theHp, int theMaxHp, int thePumpkinHp, int thePumpkinMax, const LawnInfoText& t);
// The exe tooltip's numbers (0x955340..0x95a1b0, Mod/Unnamed/ModVisuals.h) one per row: Health (body), Armour (helm),
// Shield, Balloon, Total (the exe's HP sum, only when there is more than the body), then the timed statuses in seconds.
std::vector<LawnInfoRow> ZombieInfoRows(const unnamed::ZombieInfo& z, const LawnInfoText& t);
// Centiseconds as "4.2 s" (nearest tenth).
std::string LawnInfoSeconds(int theCs, const std::string& theUnit);

// --- what the press holds --------------------------------------------------------------------------------------------
// HoldPress targets (Mod/HoldInfo.h) for a plant or a zombie id (DataArray ids), never kHoldNoTarget.
int LawnHoldPlantKey(uint32_t theId);
int LawnHoldZombieKey(uint32_t theId);
bool LawnHoldKeyIsZombie(int theKey);

} // namespace pvzu
