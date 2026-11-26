// PVZU: see LawnHoldInfo.h.
#include "Mod/LawnHoldInfo.h"

#include <algorithm>
#include <format>

namespace pvzu {

namespace {
int Clamp(int v, int lo, int hi) { return hi < lo ? lo : std::clamp(v, lo, hi); }
} // namespace

LawnInfoPlacement PlaceLawnInfo(int theTouchX, int theTouchY, int theBoxW, int theBoxH, const InfoRect& theScreen) {
	const int aLeft = theScreen.x + kLawnInfoMargin, aRight = theScreen.x + theScreen.w - kLawnInfoMargin;
	const int aTop = theScreen.y + kLawnInfoMargin, aBottom = theScreen.y + theScreen.h - kLawnInfoMargin;
	LawnInfoPlacement p;
	p.box.w = theBoxW;
	p.box.h = theBoxH;
	if (const int aY = theTouchY - kLawnInfoFingerGap - theBoxH; aY >= aTop) {
		p.side = LawnInfoSide::Above;
		p.box.x = Clamp(theTouchX - theBoxW / 2, aLeft, aRight - theBoxW);
		p.box.y = aY;
		return p;
	}
	// beside the finger, toward the screen's centre (the roomier side; the hand comes from the nearer edge)
	p.side = theTouchX < theScreen.x + theScreen.w / 2 ? LawnInfoSide::Right : LawnInfoSide::Left;
	const int aX = p.side == LawnInfoSide::Right ? theTouchX + kLawnInfoSideGap : theTouchX - kLawnInfoSideGap - theBoxW;
	p.box.x = Clamp(aX, aLeft, aRight - theBoxW);
	p.box.y = Clamp(theTouchY - theBoxH / 2, aTop, aBottom - theBoxH);
	return p;
}

InfoRect LawnInfoBoxSize(int theTitleW, int theLabelColW, int theValueColW, int theRows) {
	InfoRect r;
	r.w = std::max({kLawnInfoMinW, 2 * kLawnInfoPad + theTitleW, 2 * kLawnInfoPad + theLabelColW + kLawnInfoColumnGap + theValueColW});
	r.h = 2 * kLawnInfoPad + kLawnInfoTitleH + theRows * kLawnInfoRowH;
	return r;
}

std::string LawnInfoSeconds(int theCs, const std::string& theUnit) {
	const int aTenths = (std::max(theCs, 0) + 5) / 10;
	std::string s = std::format("{}.{}", aTenths / 10, aTenths % 10);
	if (!theUnit.empty())
		s += " " + theUnit;
	return s;
}

std::vector<LawnInfoRow> PlantInfoRows(int theHp, int theMaxHp, int thePumpkinHp, int thePumpkinMax, const LawnInfoText& t) {
	std::vector<LawnInfoRow> aRows{{t.health, std::format("{}/{}", theHp, theMaxHp)}};
	if (thePumpkinMax > 0)
		aRows.push_back({t.pumpkin, std::format("{}/{}", thePumpkinHp, thePumpkinMax)});
	return aRows;
}

std::vector<LawnInfoRow> ZombieInfoRows(const unnamed::ZombieInfo& z, const LawnInfoText& t) {
	std::vector<LawnInfoRow> aRows{{t.health, std::format("{}/{}", z.body, z.bodyMax)}}; // 0x955340
	if (z.helmMax > 0)
		aRows.push_back({t.armour, std::format("{}/{}", z.helm, z.helmMax)}); // 0x95539b
	if (z.shieldMax > 0)
		aRows.push_back({t.shield, std::format("{}/{}", z.shield, z.shieldMax)}); // 0x955444
	if (z.flyingMax > 0)
		aRows.push_back({t.balloon, std::format("{}/{}", z.flying, z.flyingMax)}); // 0x9554ed
	if (aRows.size() > 1) // the exe always adds its HP sum; here only when it says more than the body row
		aRows.push_back({t.total, std::format("{}/{}", unnamed::ZombieInfoTotal(z), unnamed::ZombieInfoTotalMax(z))});
	if (z.butterCs > 0)
		aRows.push_back({t.buttered, LawnInfoSeconds(z.butterCs, t.seconds)}); // 0x95565e
	if (z.chillCs > 0)
		aRows.push_back({t.slowed, LawnInfoSeconds(z.chillCs, t.seconds)}); // 0x9556f6
	if (z.iceCs > 0)
		aRows.push_back({t.frozen, LawnInfoSeconds(z.iceCs, t.seconds)}); // 0x95578e
	if (z.weakenCs > 0)
		aRows.push_back({t.weakened, LawnInfoSeconds(z.weakenCs, t.seconds)}); // 0x95a1b0
	return aRows;
}

int LawnHoldPlantKey(uint32_t theId) { return static_cast<int>(theId & 0x3FFFFFFFu); }
int LawnHoldZombieKey(uint32_t theId) { return static_cast<int>((theId & 0x3FFFFFFFu) | 0x40000000u); }
bool LawnHoldKeyIsZombie(int theKey) { return theKey >= 0 && (theKey & 0x40000000) != 0; }

} // namespace pvzu
