#include "doctest/doctest.h"
#include "Mod/EndScreens.h"
#include "Mod/Screen.h"

#include <initializer_list>

using namespace pvzu;

namespace {
constexpr int kReanimImageCentre = 412; // ZombiesWon.reanim: the text image (564 px) at x 129.9 in the 800 px frame
constexpr int kMenuButtonWidth = 163;   // GameOverDialog's "Main Menu" button

// Screen x of the lose text's centre: board origin on the screen (the board widget's x) + the reanim's board x.
int TextCentreOnScreen(const PlayLayout& l) {
	const int aHouse = ComputeBoardCamera(l).house;
	return aHouse + ZombiesWonReanimBoardX(l, aHouse) + kReanimImageCentre;
}
} // namespace

TEST_CASE("EndScreensVanillaUnchanged") {
	const PlayLayout l = ComputePlayLayout(800);
	CHECK(ZombiesWonReanimBoardX(l, ComputeBoardCamera(l).house) == -220); // -BOARD_OFFSET
	CHECK(GameOverMenuButtonScreenX(l) == 635);
}

TEST_CASE("EndScreensLoseTextCentredOnWideScreens") {
	// the S26 test phone (1300), 16:9 (1067, board pans 0 px), 900 (house camera 220 but rest 100) and the widest.
	for (int w : {900, 1067, 1300, 1400}) {
		CAPTURE(w);
		const PlayLayout l = ComputePlayLayout(w);
		CHECK(TextCentreOnScreen(l) == w / 2 + (kReanimImageCentre - 400));
		// The 800 frame starts where an 800 screen centred on the wide one starts.
		CHECK(ComputeBoardCamera(l).house + ZombiesWonReanimBoardX(l, ComputeBoardCamera(l).house) == CenteredScreenX(w));
	}
}

TEST_CASE("EndScreensLoseTextOnCentredBoards") {
	// 800 px boards (Zombiquarium's aquarium...) rest centred: house camera = centred x + 220 (WideScreen Camera()).
	PlayLayout l = ComputePlayLayout(1300);
	CHECK(ZombiesWonReanimBoardX(l, CenteredScreenX(1300) + 220) == -220);
}

TEST_CASE("EndScreensMenuButtonKeepsTheRightEdge") {
	PlayLayout l = ComputePlayLayout(1300);
	CHECK(GameOverMenuButtonScreenX(l) == 1135);
	CHECK(1300 - (GameOverMenuButtonScreenX(l) + kMenuButtonWidth) == 800 - (635 + kMenuButtonWidth));
	l.safeRight = 40; // camera hole / rounded corner on the right
	CHECK(GameOverMenuButtonScreenX(l) == 1095);
}
