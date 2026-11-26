#include "Mod/Unnamed/Levels.h"

#include "Mod/Unnamed/Rules.h"
#include "Mod/Unnamed/VasePole.h"

#include <cstdint>

namespace pvzu::unnamed {

namespace {
// Engine enum values the rules below name (ConstEnums.h; the same numbers as the 1051 exe).
constexpr int kZombiePoleVaulter = 3;
constexpr int kZombieNewspaper = 5;
constexpr int kZombieJackInTheBox = 15;
constexpr int kPhasePoleVaulterPreVault = 11;
constexpr int kPhaseJackRunning = 15;
constexpr int kPhaseNewspaperReading = 29;
constexpr int kMaxRerolls = 1000; // a misconfigured mask cannot hang the game

bool Bit(int mask, int bit) { return bit >= 0 && bit < 31 && ((mask >> bit) & 1) != 0; }
}

int WaveBudget(int pointsBeforeMultiplier, int vanillaPointsAfter, bool adventure) {
	if (!adventure)
		return vanillaPointsAfter;
	const int aMultiplier = RuleInt("dayLevels.waveBudgetMultiplier", -1);
	return aMultiplier < 0 ? vanillaPointsAfter : pointsBeforeMultiplier * aMultiplier;
}

bool ForceFlagAt(int level, int wave, int slot) {
	const int aLevel = RuleInt("dayLevels.forcedFlag.level", -1);
	return aLevel >= 0 && level == aLevel && wave == RuleInt("dayLevels.forcedFlag.wave", -1) &&
		slot == RuleInt("dayLevels.forcedFlag.slot", -1);
}

bool AllowCheckUsesStartingLevel() {
	return RuleInt("dayLevels.allowCheckUsesStartingLevel", 1) != 0;
}

int VaultGraveColumn(int zombieX) {
	const int aDivisor = RuleInt("dayLevels.poleGrave.colDivisor", 80);
	const int aOffset = RuleInt("dayLevels.poleGrave.colOffset", -1);
	const int aX = static_cast<uint16_t>(zombieX); // exe: div bx on the low word of mX
	return (aDivisor > 0 ? aX / aDivisor : 0) + aOffset;
}

bool PlantBlocksVault(int seedType, int plantState) {
	if (seedType == RuleInt("dayLevels.poleBonk.seedTallnut", 23))
		return true;
	const int aTallnutPlus = RuleInt("dayLevels.poleBonk.seedTallnutPlus", -1);
	const int aGiantWallnut = RuleInt("dayLevels.poleBonk.seedGiantWallnut", -1);
	if (seedType >= 0 && (seedType == aTallnutPlus || seedType == aGiantWallnut))
		return true;
	const int aCactus = RuleInt("dayLevels.poleBonk.seedCactus", -1);
	return seedType >= 0 && seedType == aCactus && plantState == RuleInt("dayLevels.poleBonk.cactusState", -1);
}

VaultDecision DecideVault(const VaultInput& in) {
	VaultDecision aOut;
	const bool aUnnamed = RuleInt("dayLevels.poleGrave.enabled", 0) != 0;
	const int aAlways = RuleInt("dayLevels.poleGrave.alwaysVariant", -1);
	const bool aSkipsTallnutTest = aUnnamed && aAlways > 0 && in.variant == aAlways;
	if (!aSkipsTallnutTest && in.hasPlant && PlantBlocksVault(in.plantSeed, in.plantState)) {
		aOut.bonk = true;
		return aOut;
	}
	if (IsVasePole(in.variant)) { // 0xc036e0: the vase path; hypnosis only flips its column, +0x12c makes it once
		aOut.vase = !in.graveMade;
		return aOut;
	}
	if (!aUnnamed || in.hypnotised || in.graveMade)
		return aOut;
	const int aSkip = RuleInt("dayLevels.poleGrave.skipVariant", -1);
	const int aVase = RuleInt("dayLevels.poleGrave.vaseVariant", -1);
	if (in.variant != 0 && (in.variant == aSkip || in.variant == aVase))
		return aOut;
	aOut.grave = true;
	return aOut;
}

bool GraveRerollAccept(int roll) {
	return Bit(RuleInt("dayLevels.graveRise.rerollAcceptMask", 0), roll);
}

int GraveRiseType(int vanillaType, const std::function<int(int)>& rand) {
	const int aRange = RuleInt("dayLevels.graveRise.rerollRange", 0);
	if (aRange <= 0)
		return vanillaType;
	const int aAgainMask = RuleInt("dayLevels.graveRise.rerollAgainMask", 0);
	const int aMaxMisses = RuleInt("dayLevels.graveRise.rerollMisses", 1);
	int aMisses = 0;
	for (int i = 0; i < kMaxRerolls; i++) {
		const int aRoll = rand(aRange);
		if (Bit(aAgainMask, aRoll))
			continue;
		if (GraveRerollAccept(aRoll))
			return aRoll;
		if (++aMisses >= aMaxMisses)
			return vanillaType;
	}
	return vanillaType;
}

int GraveRisePhase(int zombieType, int vanillaPhase) {
	if (RuleInt("dayLevels.graveRise.phaseByType", 0) == 0)
		return vanillaPhase;
	switch (zombieType) {
	case kZombiePoleVaulter: return kPhasePoleVaulterPreVault;
	case kZombieNewspaper: return kPhaseNewspaperReading;
	case kZombieJackInTheBox: return kPhaseJackRunning;
	default: return vanillaPhase;
	}
}

int JackRiseFuse(int roll1000) {
	return roll1000 + RuleInt("dayLevels.graveRise.jackFuseBase", 300);
}

int JackRiseFuseRange() {
	return RuleInt("dayLevels.graveRise.jackFuseRange", 1000);
}

int AshThresholdHp(int body, int helm, int shield, bool unnamed) {
	return unnamed ? body + helm + shield : body;
}

int AshThreshold() {
	return RuleInt("dayLevels.ashThreshold", 1800);
}

bool TooToughToAsh(int body, int helm, int shield, bool unnamed) {
	const bool aTotal = unnamed && RuleInt("dayLevels.ashThresholdUsesTotalHp", 0) != 0;
	return AshThresholdHp(body, helm, shield, aTotal) >= (aTotal ? AshThreshold() : 1800);
}

} // namespace pvzu::unnamed
