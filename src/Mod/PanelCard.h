// PVZU: inside of a seed card, copied from the EA mobile PvZ card (2026-11-23: "more modern, with the
// plants going a bit over the rectangle, they look a bit 3D"). A light rounded frame (dark hairline outline, white
// bevelled rim, pale aqua panel) with a soft shadow below; the plant drawn large, rising over the top and left rim
// and cut at the panel's bottom; the cost in a white tab that is one shape with the rim, bottom-right, round
// top-left corner, dark digits. The frame has the reference's aspect (1.96) and sits centred in the HUD slot
// (114 x 67), which stays the tap area; the slot's spare height is the room the plant rises into.
//
//        .--.
//   +---(-oo-)------------+   <- the plant rises over the top rim
//   |    \__/             |
//   |     ||       .------|
//   |    _||_     (   175 |   <- white tab, part of the rim
//   +---------------------+
//    ~~~~~~~~~~~~~~~~~~~~~      soft shadow
//
// Pure geometry in slot-local logical px; Mod/CardArt renders the frame and tab, SeedPacket.cpp draws it all.
#pragma once

#include "Mod/NineSlice.h"

namespace pvzu {

// Vanilla packet art (one 50x70 cel of seeds.png), measured: the picture window the packet's plant is placed in.
constexpr Box kPacketWindowSrc{1, 7, 48, 47};

// Reference card (EA mobile, 2340x1080 recording): 206 x 105 native px.
constexpr float kCardAspect = 1.96f;
constexpr int kCardFrameH = 58;      // a full-size frame: kCardW / kCardAspect
// Frame art at a full-size card (logical px; smaller cards scale by k).
constexpr float kCardRadius = 5.5f;  // outer corner radius
constexpr float kCardOutline = 1.0f; // dark hairline outline
constexpr float kCardRim = 3.0f;     // light rim inside the outline

struct PanelCardParts {
	bool compact = false;      // the narrow card of 9-15 slots (slot narrower than tall), see PanelCardLayout
	float k = 1.0f;            // frame.h / kCardFrameH: everything else scales with it
	Box frame;                 // the drawn card (outline outer edge), centred in the slot
	Box panel;                 // the aqua panel inside outline + rim
	Box art;                   // what the frame's art covers: the frame plus its soft shadow
	float shadowDx = 0.0f;     // card shadow offset (baked into the frame's art)
	float shadowDy = 0.0f;
	Box tab;                   // cost tab: flush with the frame's right and bottom edges; empty without a cost
	float tabRadius = 0.0f;    // its round top-left corner
	Box text;                  // where the cost text goes (its bounding box at textScale)
	float textScale = 1.0f;    // cost text scale (1 = the font's own size)
	Box plantClip;             // the plant draws only here: over the top and left rim, cut at the panel's bottom
	float plantScale = 1.0f;   // the packet's plant drawing scale (1 = vanilla packet size)
	float plantX = 0.0f;       // where a vanilla packet's origin goes for its plant to sit in the card
	float plantY = 0.0f;
	float plantFloor = 0.0f;   // a plant shorter than the card stands here (the panel's bottom; compact: the tab's top)
	float plantShadowDx = 0.0f; // the plant's own soft shadow on the panel
	float plantShadowDy = 0.0f;
};

// Card in a slot of slotW x slotH (kCardW x kCardH, or smaller: everything shrinks uniformly), cost text of
// costTextW x costTextH px at the font's own size (costTextW 0: no cost, no tab). The plant is fitted as if it
// filled the vanilla packet window; FitPanelPlant refits it to the plant's real bounds.
// A slot narrower than tall (kCompactCardW x kCardH, 9-15 slots) gets the compact card: the slot's width, a wide
// card's frame height (so k = 1: same outline, rim, shadow and rise), a lower cost tab with smaller digits, and the
// plant standing in the middle above the tab.
PanelCardParts PanelCardLayout(int slotW, int slotH, int costTextW, int costTextH);

// A plant's visible pixels in vanilla packet px, as DrawSeedPacket draws it (packet origin 0,0; 50 x 70 packet).
struct PlantBounds { float x0 = 0, y0 = 0, x1 = 0, y1 = 0; };
// Sets plantScale/plantX/plantY so the plant fills the card as in the reference: a plant's top rises a few px over
// the top rim and its lower part is cut by the panel's bottom; a wide plant is limited by the card's width and then
// stands on the panel's bottom; its middle sits left of the card's middle, clear of the cost tab.
void FitPanelPlant(PanelCardParts& p, const PlantBounds& b);

// Recharge-done flash of a HUD card (the whole card lights up, then fades; vanilla SeedPacketFlash.xml timing:
// alpha 1 -> 0 over 40% of 25 cs). Age in cs since the flash began; 0 when not flashing.
inline constexpr int kCardFlashCs = 10;
float CardFlashAlpha(int ageCs);

} // namespace pvzu
