// PVZU: a decorative cover over the strip of street right of the vanilla view (2026-11-22), so zombies walk
// out of it instead of popping into clear view. 2026-11-25 ("the zombies still look like they spawn, make the fog
// like a gradient 0% -> 100%"): the cover is a ramp from clear at the lawn's edge to fully opaque where the leftmost
// zombie can appear, and opaque from there to the screen edge, so a new zombie is hidden and fades in as it walks.
// Pure geometry and timing here; the board draws it (Mod/Engine/EdgeFog: the game's fog clouds, on every level since
// 2026-11-26). Presentation only: rules never read it. The seed cards and top bar: Mod/Hud.h.
#pragma once

#include "Mod/Layout.h"

namespace pvzu {

constexpr int kVanillaMinSpawnX = 780;  // Zombie::ZombieInitialize: 780 + Rand(40), other types further right
// Leftmost drawn pixel of a zombie relative to its mPosX: measured 0 for a Normal zombie (2026-11-25, 1300 x 600
// zoomed run, frame diff at the spawn frame); -20 leaves room for arms, held doors and the walk's sway.
constexpr int kZombieArtLeft = -20;
// The ramp is never steeper than this (board px from clear to opaque), even when zombies start at the lawn's edge.
constexpr int kStreetFogMinRamp = 40;

struct StreetFogRamp {
	bool active = false;    // false: no street on screen (width 800, or the view ends at the lawn's edge)
	float clearX = 0.0f;    // alpha 0 here and left of it (the lawn's right edge, board x 800)
	float opaqueX = 0.0f;   // alpha 1 here and right of it
};

// visibleRightBoardX: board x at the screen's right edge in the current view (zoom included). minSpawnX: the smallest
// mPosX a zombie can start at (kVanillaMinSpawnX plus the mod's start offset). Opaque from where that zombie's art
// begins, never closer than kStreetFogMinRamp to the lawn and never past the screen edge.
StreetFogRamp ComputeStreetFogRamp(int visibleRightBoardX, int minSpawnX);

// Cover alpha 0..1 at board x: 0 on the lawn, smoothstep up to 1 at opaqueX, 1 beyond.
float StreetFogAlpha(float boardX, const StreetFogRamp& ramp);
// Fog levels: alpha of the flat wash under the clouds. It trails StreetFogAlpha (the clouds' alpha), so the inner
// edge has the clouds' shape instead of a straight line, and is just as opaque from opaqueX on.
float StreetFogWashAlpha(float boardX, const StreetFogRamp& ramp);

// ---- 2026-11-26: "copy the style of the fog" (on every level). The cover is drawn with the game's fog clouds
// everywhere (Mod/Engine/EdgeFog). 2026-11-26 (asked and answered): "no fading, the fog only moves". When the
// camera leaves its resting place (the level-start look at the street, the zombies-won pan) the cover slides out off
// the screen's right edge; when the camera is back at rest it slides in from the right into its place, easing out like
// vanilla's fog rolling in. It is always fully opaque (the 0 -> 100% ramp moves with it).

constexpr int kEdgeFogOutTicks = 100;          // 1 s at 100 updates a second, CURVE_EASE_IN_OUT (as the camera pan)
constexpr int kEdgeFogInTicks = 120;           // 1.2 s, CURVE_EASE_OUT (as vanilla's fog roll-in)
constexpr float kEdgeFogHiddenMargin = 16.0f;  // board px past the screen edge where the cover is out of sight

struct EdgeFogState {
	bool started = false; // the first board update has placed the cover (in place if the camera is at rest)
	bool in = true;       // the current move: true into place, false out of sight
	float out = 0.0f;     // 0 in place .. 1 just past the screen's right edge
	float from = 0.0f;    // `out` when the current move began
	int ticks = 0;        // updates into the current move
	bool followRoll = false; // Fog levels: the cover also rides vanilla's fog roll-in (Board::mFogOffset)
};

struct EdgeFogInput {
	bool wanted = false;      // the cover belongs on screen: wide street showing, camera at rest, not game over/boss
	bool zombies = false;     // a live zombie that is not a level-start preview: the cover must be in place now
	bool waitForRoll = false; // Fog level intro before vanilla's fog starts to roll in: stay out to come in with it
	bool rollActive = false;  // Fog level intro while vanilla's fog rolls in (0 < mFogBlownCountDown < 200)
};

// One board update. First update: in place if wanted, else out of sight, without moving (a level's first moment, a
// resumed game). Wanted with zombies: in place at once. Otherwise it moves in or out; a move reversed half way starts
// from where the cover is and takes the matching share of the time.
EdgeFogState StepEdgeFog(EdgeFogState s, const EdgeFogInput& in);
// Board x offset of the cover (>= 0, to the right). hiddenShift: the offset that puts its clear edge just past the
// screen's right edge (EdgeFogHiddenShift). vanillaFogOffset: Board::mFogOffset, used only while followRoll (it then
// moves as one with vanilla's fog columns, which share its 80 px grid).
float EdgeFogShift(const EdgeFogState& s, float hiddenShift, float vanillaFogOffset);
float EdgeFogHiddenShift(const StreetFogRamp& ramp, int screenRightBoardX);
// Cover alpha at board x with the whole cover moved right by `shift`.
float EdgeFogCoverAlpha(float boardX, const StreetFogRamp& ramp, float shift);

struct EdgeFogRgb {
	int r = 0, g = 0, b = 0;
};
// Night look (BackgroundType 1 night, 3 fog, 5 boss night roof): vanilla's fog colouring. Day look (0 day, 2 pool,
// 4 roof): the same clouds, lighter.
bool EdgeFogNightLook(int background);
// Colour of one cloud cel: night = Board::DrawFog's (255 - look*1.5 - motion*1.5, same, 255 - look - motion); day =
// the same shimmer at a third of the darkening.
EdgeFogRgb EdgeFogCelColor(bool night, int celLook, float motion);
// The flat wash under the clouds (opaque where zombies start), in the clouds' mean tone.
EdgeFogRgb EdgeFogWashColor(bool night);

} // namespace pvzu
