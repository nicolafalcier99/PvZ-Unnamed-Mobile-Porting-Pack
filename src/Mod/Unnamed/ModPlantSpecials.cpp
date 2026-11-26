// PVZU: see ModPlantSpecials.h.
#include "Mod/Unnamed/ModPlantSpecials.h"

#include "Mod/Unnamed/ModPlantRules.h"
#include "Mod/Unnamed/State.h"

namespace pvzu::unnamed {

namespace {
constexpr int kLastVanillaSeed = 52; // SEED_LEFTPEATER
}

int ShrapnelKindOfShot(int modShot) {
	return modShot == kModShotShrapnelShard || modShot == kModShotShrapnelCore ? modShot : 0;
}

int BossFlowerDrawSeed(const std::function<int(int)>& theRand, const std::function<bool(int)>& theUsable, int theMaxTries) {
	const int aRange = BossFlowerRollRange();
	if (aRange <= 0)
		return -1;
	for (int i = 0; i < theMaxTries; ++i) {
		const int aSeed = BossFlowerSeedFromRoll(theRand(aRange));
		if (aSeed >= 0 && theUsable(aSeed))
			return aSeed;
	}
	return -1;
}

bool PacketCoinSeedUsable(int theSeed, const std::function<bool(int)>& theModSeedHasRow) {
	if (theSeed >= 0 && theSeed <= kLastVanillaSeed)
		return true;
	return IsModSeed(theSeed) && theModSeedHasRow(theSeed);
}

} // namespace pvzu::unnamed
