// PVZU: the street cover (Mod/StreetFog.h) is the game's own fog on every level (2026-11-26: "copy the style of
// the fog"): the cloud cels, cel looks, colour and motion of vanilla Board::DrawFog, laid on the same 80 x 85 grid out
// to the screen edge, faded in along the street ramp (clear at the lawn's edge, opaque where zombies start; the developer
// 2026-11-25). Night levels use vanilla's colouring, day levels the same clouds lighter. It does not follow the gameplay
// fog (Planterns, Blover): it stands for the street vanilla never shows. Presentation only.
#pragma once

#include "Mod/StreetFog.h"

namespace Sexy { class Graphics; }
class Board;

namespace pvzu::engine {

// Board::DrawUITop (through DrawStreetFog, inside the board zoom): fog from the ramp's clear x to theRightX (board x,
// the screen edge: nothing is drawn past it), the whole cover moved right by theShift (>= 0, the slide in and out),
// alpha EdgeFogCoverAlpha. Never faded (2026-11-26).
void DrawEdgeFog(Sexy::Graphics* g, Board* theBoard, const StreetFogRamp& theRamp, float theShift, int theRightX,
	bool theNight);

} // namespace pvzu::engine
