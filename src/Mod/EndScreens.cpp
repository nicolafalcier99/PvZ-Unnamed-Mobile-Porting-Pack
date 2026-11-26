#include "Mod/EndScreens.h"

#include "Mod/Screen.h"

namespace pvzu {

int ZombiesWonReanimBoardX(const PlayLayout& layout, int houseCameraX) {
	return CenteredScreenX(layout.width) - houseCameraX;
}

int GameOverMenuButtonScreenX(const PlayLayout& layout) {
	constexpr int kVanillaMenuButtonX = 635; // GameOverDialog: "Main Menu" over the board's top-right Menu button
	return AnchorX(kVanillaMenuButtonX, Anchor::Right, layout);
}

} // namespace pvzu
