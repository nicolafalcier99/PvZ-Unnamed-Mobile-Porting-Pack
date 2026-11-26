// PVZU: the title screen ("Loading..." / "Click to start", sod roll bar) on a wide screen. The title is an 800 x 600
// widget centred on the W-wide screen (Mod/Engine/WideScreen PlaceCenteredScreen), but two of its parts were written
// for a title at x 0: its start button is a top-level widget placed in screen coordinates, and the sod roll cap is
// drawn through a matrix that skips the widget's translation. At W = 800 (title at x 0) every value is vanilla.
#pragma once

namespace pvzu {

struct TitlePlace {
	int buttonScreenX = 0;   // start button (screen x): centred on the title like vanilla's (800 - dirt) / 2
	int barLocalX = 0;       // dirt, grass and roll start (title-local x), vanilla (800 - dirt) / 2
	int mouseInsetLeft = 0;  // title mouse insets (negative = grows): a click anywhere on the screen starts, as in vanilla
	int mouseInsetRight = 0;
};

TitlePlace PlaceTitle(int screenW, int titleX, int titleW, int dirtW);

// Screen x of the sod roll cap's centre: vanilla barLocalX + 11 + rollLen, plus the title's draw translation.
float TitleRollCapScreenX(int barLocalX, float rollLen, float titleTransX);

} // namespace pvzu
