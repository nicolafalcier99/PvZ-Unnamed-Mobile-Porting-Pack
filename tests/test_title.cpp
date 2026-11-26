#include "doctest/doctest.h"
#include "Mod/TitleSkip.h"
#include "Mod/TitleLayout.h"

using namespace pvzu;

TEST_CASE("TitleSkipsOnPhonesOnly") {
	CHECK(ShouldSkipTitleScreen(/*mobile=*/true, /*demo=*/false, /*flag=*/false));
	CHECK_FALSE(ShouldSkipTitleScreen(false, false, false));  // desktop: vanilla title (recorded demos click it)
	CHECK(ShouldSkipTitleScreen(false, false, true));         // desktop opt-in: -pvzu-skip-title
	CHECK_FALSE(ShouldSkipTitleScreen(true, true, false));    // demo playback never auto-skips
	CHECK_FALSE(ShouldSkipTitleScreen(false, true, true));
}

TEST_CASE("TitleLoadBarCentredOnScreen") {
	// PC 800: the vanilla numbers (bar and button at (800 - 321) / 2 = 239, the title covers the screen).
	TitlePlace pc = PlaceTitle(/*screenW=*/800, /*titleX=*/0, /*titleW=*/800, /*dirtW=*/321);
	CHECK(pc.buttonScreenX == 239);
	CHECK(pc.barLocalX == 239);
	CHECK(pc.mouseInsetLeft == 0);
	CHECK(pc.mouseInsetRight == 0);

	// Phone 1300: the 800 title is centred at x 250. The "Loading..." button is a top-level widget (screen
	// coordinates) and must move with it; the bar keeps its place on the title. Both centre on 650 - 1, the
	// same 1 px rounding as vanilla's 400 - 1.
	TitlePlace wide = PlaceTitle(1300, 250, 800, 321);
	CHECK(wide.buttonScreenX == 489);
	CHECK(wide.barLocalX == 239);
	CHECK(wide.buttonScreenX + 321 / 2 == 1300 / 2 - 1);
	// Vanilla: a click anywhere on the screen starts the game; the side fill counts too.
	CHECK(wide.mouseInsetLeft == -250);
	CHECK(wide.mouseInsetRight == -250);

	// Odd extra width: the title rounds left like CenteredScreenX, the right side gets the extra pixel.
	TitlePlace odd = PlaceTitle(1301, 250, 800, 321);
	CHECK(odd.buttonScreenX == 489);
	CHECK(odd.mouseInsetLeft == -250);
	CHECK(odd.mouseInsetRight == -251);
}

TEST_CASE("TitleSodRollCapFollowsTheTitle") {
	// The roll cap is drawn through a matrix in screen coordinates (no widget translation), 11 px into the bar.
	CHECK(TitleRollCapScreenX(/*barLocalX=*/239, /*rollLen=*/0.0f, /*titleTransX=*/0.0f) == doctest::Approx(250.0f));
	CHECK(TitleRollCapScreenX(239, 100.0f, 0.0f) == doctest::Approx(350.0f)); // PC: vanilla x
	CHECK(TitleRollCapScreenX(239, 100.0f, 250.0f) == doctest::Approx(600.0f)); // wide: moves with the title
}
