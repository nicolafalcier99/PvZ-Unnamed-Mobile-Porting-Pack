// PVZU: Plan 9 L3 visual details, pure rules. See ModVisuals.h; evidence the research notes (l3-visual-details).
#include "Mod/Unnamed/ModVisuals.h"

#include "Mod/GameData.h"
#include "Mod/Unnamed/ModPlantRules.h"
#include "Mod/Unnamed/Rules.h"

#include <cmath>
#include <cstdlib>
#include <format>
#include <vector>

namespace pvzu::unnamed {

namespace {
constexpr int kSeedPeashooter = 0, kSeedSniper = 0x78, kSeedElement = 0x7a;
constexpr int kScenePlaying = 3; // the exe's mGameScene == 3
int R(const char* theKey) { return RuleInt(theKey, 0); }
} // namespace

bool ModVisualsOn() { return GameData::Get().Feature("modVisuals"); }

bool InfoToolTipShown(int theGameScene, int theGameMode, int theDownButtons, int theHeldTicks) {
	if (!ModVisualsOn() || R("modVisuals.infoTooltip") == 0)
		return false;
	if (theGameScene != kScenePlaying || theGameMode == R("modVisuals.infoSkipMode")) // 0x955299, 0x9552a6
		return false;
	if (const int aButtons = R("modVisuals.infoButtons"); aButtons != 0 && theDownButtons == aButtons) // 0x9552b9 == 2
		return true;
	const int aHold = R("modVisuals.infoHoldTicks"); // port: the phone has no right button
	return aHold > 0 && theDownButtons == 1 && theHeldTicks >= aHold;
}

int ZombieInfoTotal(const ZombieInfo& z) {
	int aTotal = z.body; // 0x9555ce..0x9555f2
	if (z.helmType > 0)
		aTotal += z.helm;
	if (z.shieldType > 0)
		aTotal += z.shield;
	return aTotal + z.flying;
}

int ZombieInfoTotalMax(const ZombieInfo& z) { return z.bodyMax + z.helmMax + z.shieldMax + z.flyingMax; }

std::string ZombieInfoLabel(const ZombieInfo& z, const InfoLabels& l) {
	std::vector<std::string> aFields;
	aFields.push_back(std::format("{}/{}", z.body, z.bodyMax)); // 0x955340
	if (z.helmMax > 0)
		aFields.push_back(std::format("{}{}/{}", l.helm, z.helm, z.helmMax)); // 0x95539b
	if (z.shieldMax > 0)
		aFields.push_back(std::format("{}{}/{}", l.shield, z.shield, z.shieldMax)); // 0x955444
	if (z.flyingMax > 0)
		aFields.push_back(std::format("{}{}/{}", l.balloon, z.flying, z.flyingMax)); // 0x9554ed
	aFields.push_back(std::format("{}{}/{}", l.hp, ZombieInfoTotal(z), ZombieInfoTotalMax(z)));
	if (z.butterCs > 0)
		aFields.push_back(std::format("{}{}{}", l.butter, z.butterCs, l.cs)); // 0x95565e
	if (z.chillCs > 0)
		aFields.push_back(std::format("{}{}{}", l.chill, z.chillCs, l.cs)); // 0x9556f6
	if (z.iceCs > 0)
		aFields.push_back(std::format("{}{}{}", l.frozen, z.iceCs, l.cs)); // 0x95578e
	if (z.weakenCs > 0)
		aFields.push_back(std::format("{}{}{}", l.weakened, z.weakenCs, l.cs)); // 0x95a1b0
	// 0x9558c0: after field n (1-based) a ' ' when n is odd, a '\n' when even; the last separator is cut (0x95582b)
	std::string aText;
	for (size_t i = 0; i < aFields.size(); i++) {
		if (i > 0)
			aText += (i % 2 == 1) ? ' ' : '\n';
		aText += aFields[i];
	}
	return aText;
}

std::string PlantInfoLabel(int theHp, int theMaxHp, const std::string& theHealthLabel) {
	return std::format("{}{}/{}", theHealthLabel, theHp, theMaxHp);
}

bool PeashooterLayersDrawn(int theRawSeed) {
	return theRawSeed == kSeedPeashooter && ModVisualsOn() && R("modVisuals.peashooterLayers") != 0; // 0xc10faa
}

bool SniperAims(int theSeed, int theState, int theTargetId, bool theAliveOnBoard) {
	return theSeed == kSeedSniper && theState != 0 && theTargetId != 0 && theAliveOnBoard && ModPlantsOn() &&
		R("modVisuals.sniperAim") != 0; // 0x7a7ea9..0x7a7ed5
}

float SniperHeadAngle(int thePlantX, int thePlantY, int theZombieX, int theZombieY) {
	// 0x7a7ef0: fild z.y; fisubr p.y; fild z.x; fisub p.x; fabs; fpatan
	return std::atan2(static_cast<float>(thePlantY - theZombieY), static_cast<float>(std::abs(theZombieX - thePlantX)));
}

AimMatrix SniperHeadMatrix(float theAngle) {
	// 0x7aa500: rotation about (40, 40): t = p - R p
	const float c = std::cos(theAngle), s = std::sin(theAngle);
	AimMatrix m;
	m.m00 = c;
	m.m01 = s;
	m.m10 = -s;
	m.m11 = c;
	m.m02 = 40.0f * (1.0f - c - s);
	m.m12 = 40.0f * (1.0f - c + s);
	return m;
}

bool SniperMarkDrawn() { return ModPlantsOn() && R("modVisuals.sniperMark") != 0; }
bool PacketPlantCelsUsed() { return !ModVisualsOn() || RuleInt("modVisuals.packetPlantCels", 1) != 0; } // 0x4873e0

int SniperMarkOffsetX() { return 20; }
int SniperMarkOffsetY() { return 0; }

bool ElementIconsDrawn(int theSeed, int theElementCount, bool theSquished) {
	return theSeed == kSeedElement && !theSquished && theElementCount > 0 && ModPlantsOn() &&
		R("modVisuals.elementIcons") != 0; // 0x7a8e29, 0x7a8e33, 0x7b15a1
}

std::array<ElementIcon, 3> ElementShroomIcons(int theBits, int theWidth, int theHeight) {
	(void)theWidth;
	std::array<ElementIcon, 3> a{};
	static constexpr int kBit[3] = {4, 2, 1}; // 0x7b1606 / 0x7b1632 / 0x7b1661
	for (int i = 0; i < 3; i++) {
		ElementIcon& e = a[i];
		e.ice = (theBits & kBit[i]) != 0;
		e.x = 3 + 20 * i;           // edx = x + 3, + 0x14 / + 0x28
		e.y = theHeight - 25;       // ecx = y + height - 0x19
		if (!e.ice) {               // the fire pea frame: 20 left, 25 up
			e.x -= 20;
			e.y -= 25;
		}
	}
	return a;
}

int ElementDigitX(int theWidth) { return theWidth / 2 - 6; } // 0x7b1556 sar 1; sub 6
int ElementDigitY() { return 25; }                          // 0x7b154f add 0x19

} // namespace pvzu::unnamed
