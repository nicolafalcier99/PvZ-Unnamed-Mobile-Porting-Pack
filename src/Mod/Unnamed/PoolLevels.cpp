#include "Mod/Unnamed/PoolLevels.h"

#include "Mod/Unnamed/Rules.h"

#include <algorithm>
#include <string>

namespace pvzu::unnamed {

bool PoolFinalWaveSpawn(int level, PoolSpawn& out) {
	out = {};
	const std::string aBase = "poolLevels.poolSpawn." + std::to_string(level);
	const int aCount = RuleInt(aBase + ".count", 0);
	if (aCount <= 0)
		return false;
	const int aPoints = RuleInt(aBase + ".points", -1);
	if (aPoints < 0)
		return false;
	out.count = std::min(aCount, kPoolSpawnCells); // the engine's grid array holds 8 cells
	out.points = aPoints;
	return true;
}

bool PoolGravesAfterPoolSpawn() {
	return RuleInt("poolLevels.finalWave.gravesAfterPool", 0) != 0;
}

bool PoolRisersTakeHypnosis() {
	return RuleInt("poolLevels.poolSpawn.passesHypnosis", 0) != 0;
}

int PoolHypnosisRollRange() {
	return std::max(RuleInt("poolLevels.poolSpawn.hypnoRollRange", 0), 0);
}

int IceTrailTimerBelowX(int vanillaX) {
	return RuleInt("poolLevels.iceTrail.timerBelowX", vanillaX);
}

} // namespace pvzu::unnamed
