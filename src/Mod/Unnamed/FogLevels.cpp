#include "Mod/Unnamed/FogLevels.h"

#include "Mod/Unnamed/Rules.h"

#include <string>

namespace pvzu::unnamed {

namespace {
constexpr int kFirstFogLevel = 31, kLastFogLevel = 40; // the only Adventure levels on the fog lawn
constexpr int kLawnColumns = 9;                       // MAX_GRID_SIZE_X
}

bool FullFogOn(bool theOption) {
	return theOption || RuleInt("fogLevels.fullFog", 0) != 0;
}

int FogStartColumn(int level, bool adventure, bool theFullFogOption) {
	if (FullFogOn(theFullFogOption))
		return 0;
	if (!adventure || level < kFirstFogLevel || level > kLastFogLevel)
		return kNoFogRule;
	const int aColumn = RuleInt("fogLevels.fogColumn." + std::to_string(level), kNoFogRule);
	return aColumn >= 0 && aColumn < kLawnColumns ? aColumn : kNoFogRule;
}

} // namespace pvzu::unnamed
