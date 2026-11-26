// PVZU: where the lose screens sit on the wide screen (2026-11-25: "When I die the text is in the left side also
// the menu is shifted"). Vanilla places both for an 800 x 600 screen:
//  - the "THE ZOMBIES ATE YOUR BRAINS!" reanim (REANIM_ZOMBIES_WON, 800 x 600 art, its image centred on x ~412) is
//    added to the board at board x -BOARD_OFFSET, which is screen x 0 because the lose pan stops the board at x 220
//    (the house camera). On a wide screen the house camera is still 220 but the screen is wider, so the text sat on
//    the left 800 px;
//  - GameOverDialog's "Main Menu" button is at screen x 635 (in place of the board's hidden top-right Menu button),
//    while the board's Menu button now keeps its distance from the right edge.
// At width 800 every function returns the vanilla value.
#pragma once

#include "Mod/Layout.h"

namespace pvzu {

// Board x for the lose reanim's origin so that its 800 x 600 frame is centred on the screen while the board widget is
// at houseCameraX (the lose pan's end). Vanilla: -220.
int ZombiesWonReanimBoardX(const PlayLayout& layout, int houseCameraX);

// Screen x of GameOverDialog's "Main Menu" button: right-anchored like the board's Menu button it replaces (inside the
// right safe inset). Vanilla: 635.
int GameOverMenuButtonScreenX(const PlayLayout& layout);

} // namespace pvzu
