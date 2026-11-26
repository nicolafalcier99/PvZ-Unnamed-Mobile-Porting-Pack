#include "Mod/Unnamed/NightLevels.h"

#include "Mod/Unnamed/Rules.h"

#include <string>

namespace pvzu::unnamed {

namespace {
constexpr int kMaxAllowExtra = 64; // a misconfigured count cannot make the allow check slow
std::string Key(std::string_view prefix, int n, std::string_view suffix) {
	return std::string(prefix) + std::to_string(n) + std::string(suffix);
}
}

bool NightStartGraves(int level, GraveLayout& out) {
	out = {};
	// Only levels with a ".firstCol" key have a row; ask for that key first so absent levels warn once, not per column.
	const std::string aBase = Key("nightLevels.startGraves.", level, "");
	const int aFirst = RuleInt(aBase + ".firstCol", -1);
	if (aFirst < 0 || aFirst >= static_cast<int>(out.size()))
		return false;
	for (int aCol = aFirst; aCol < static_cast<int>(out.size()); aCol++)
		out[aCol] = RuleInt(Key(aBase + ".c", aCol, ""), 0);
	return GraveTotal(out) > 0;
}

int GraveTotal(const GraveLayout& layout) {
	int aTotal = 0;
	for (int aCount : layout)
		aTotal += aCount;
	return aTotal;
}

bool NightAllowExtra(int zombieType, int level) {
	const int aCount = RuleInt("nightLevels.allowExtra.count", 0);
	for (int i = 0; i < aCount && i < kMaxAllowExtra; i++) {
		if (RuleInt(Key("nightLevels.allowExtra.", i, ".level"), -1) == level &&
			RuleInt(Key("nightLevels.allowExtra.", i, ".type"), -1) == zombieType)
			return true;
	}
	return false;
}

bool GoldFootballRaisesGraves(int zombieType, int variant, int fromWave) {
	const int aVariant = RuleInt("nightLevels.goldFootball.variant", 0);
	return aVariant != 0 && variant == aVariant && zombieType == RuleInt("nightLevels.goldFootball.type", -1) &&
		fromWave >= 0;
}

int GoldFootballGraveType() {
	return RuleInt("nightLevels.goldFootball.graveType", -1);
}

CarnivalStep CarnivalOnBackupSummoned(int summonerType, int summonerVariant, int counter) {
	const int aEvery = RuleInt("nightLevels.carnival.everyBackups", 0);
	if (aEvery <= 0 || summonerType != RuleInt("nightLevels.carnival.summonerType", -1) ||
		summonerVariant == RuleInt("nightLevels.carnival.skipVariant", -1))
		return {counter, false};
	const int aNext = counter + 1;
	if (aNext < aEvery) // 0x7966C0: cmp [ebx+0x12c],0xc; jl
		return {aNext, false};
	return {0, true};
}

bool CarnivalPassesHypnosis() {
	return RuleInt("nightLevels.carnival.passesHypnosis", 0) != 0;
}

} // namespace pvzu::unnamed
