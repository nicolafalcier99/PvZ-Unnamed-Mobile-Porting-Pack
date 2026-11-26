#include "Mod/Unnamed/RoofLevels.h"

#include "Mod/Unnamed/DayZombies.h"
#include "Mod/Unnamed/Rules.h"

#include <algorithm>
#include <string>

namespace pvzu::unnamed {

namespace {
constexpr int kMaxRerolls = 1000; // a broken roll source cannot hang the game
}

bool RoofFinalWaveSkyDrop(int level, SkyDrop& out) {
	out = {};
	const std::string aBase = "roofLevels.skyDrop." + std::to_string(level);
	const int aCount = RuleInt(aBase + ".count", 0);
	if (aCount <= 0)
		return false;
	const int aPoints = RuleInt(aBase + ".points", -1);
	if (aPoints < 0)
		return false;
	out.count = std::min(aCount, kSkyDropCells); // the engine's bungee drop grid holds 25 cells
	out.points = aPoints;
	return true;
}

int SkyDropType(const std::function<int()>& vanillaPick, const std::function<int(int)>& rand) {
	const int aVanilla = vanillaPick(); // 0x412CA3: drawn before the hook, kept only as the fallback
	const int aRange = SkyDropTypeRollRange(); // dayZombies.skyDrop.typeRollRange (25)
	if (aRange <= 0)
		return aVanilla;
	for (int i = 0; i < kMaxRerolls; i++) {
		const int aRoll = rand(aRange);
		if (SkyDropTypeAccepted(aRoll)) // 0..7, 15, 21..24 (hook 85, 0x795C27)
			return aRoll;
	}
	return aVanilla;
}

} // namespace pvzu::unnamed
