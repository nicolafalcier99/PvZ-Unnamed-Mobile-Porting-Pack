#include "Mod/TitleLayout.h"

namespace pvzu {

TitlePlace PlaceTitle(int screenW, int titleX, int titleW, int dirtW) {
	TitlePlace p;
	p.barLocalX = (titleW - dirtW) / 2;
	p.buttonScreenX = titleX + p.barLocalX;
	p.mouseInsetLeft = -titleX;
	p.mouseInsetRight = -(screenW - titleX - titleW);
	return p;
}

float TitleRollCapScreenX(int barLocalX, float rollLen, float titleTransX) {
	return titleTransX + static_cast<float>(barLocalX) + 11.0f + rollLen;
}

} // namespace pvzu
