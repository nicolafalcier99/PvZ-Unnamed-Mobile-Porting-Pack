// PVZU: engine side of the seed chooser's hold-for-info panel (rules and layout in Mod/HoldInfo.h).
//
// A finger (or S Pen) press on a seed packet in the chooser no longer picks on the press: the pick waits for the lift.
// A quick tap (lifted on the same packet before kHoldInfoDelay) replays vanilla's press there, so it picks the seed or
// puts a picked card back exactly as before. A hold shows the big panel in the middle of the screen until the finger
// lifts or leaves the packet, and its lift changes nothing. Touch presses also hide the small hover tooltip (its words
// are in the panel). A mouse, demo playback and recording keep vanilla's behaviour byte for byte. Chooser only: play's
// HUD cards are untouched (2026-11-24: a held card in play is for planting).
//
// Mac check: "-pvzu-input" with fdown / fup on a packet; stderr prints "PVZU hold: ..." for each press.
#pragma once

#include "ConstEnums.h"

class SeedChooserScreen;
namespace Sexy {
class Graphics;
}

namespace pvzu::engine {

// SeedChooserScreen::MouseDown at its seed test: true = a touch press on a packet, the pick waits for the lift.
bool ChooserHoldDown(SeedChooserScreen* theScreen, SeedType theSeed, int x, int y, int theClickCount);
// SeedChooserScreen::MouseUp: a quick tap replays the press (vanilla's pick or put-back); a hold does nothing.
void ChooserHoldUp(SeedChooserScreen* theScreen, int x, int y);
// SeedChooserScreen::Update: shows the panel after the delay, hides it when the finger leaves the packet.
void ChooserHoldUpdate(SeedChooserScreen* theScreen);
// SeedChooserScreen::Draw: false = do not draw the small tooltip (the last press was a finger or pen).
bool ChooserSmallToolTipShown(SeedChooserScreen* theScreen);
// SeedChooserScreen::Draw, last: the panel (screen space, over everything the chooser draws).
void DrawChooserHoldInfo(SeedChooserScreen* theScreen, Sexy::Graphics* g);
// A new chooser starts with no press.
void ChooserHoldReset();

} // namespace pvzu::engine
