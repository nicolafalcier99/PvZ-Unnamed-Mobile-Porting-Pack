#include "Mod/Unnamed/VasePole.h"

#include "Mod/Unnamed/DayZombies.h"
#include "Mod/Unnamed/Rules.h"

#include <cstdint>

namespace pvzu::unnamed {

namespace {
int R(const char* key, int vanilla) { return RuleInt(key, vanilla); }
constexpr int kMaxRerolls = 1000; // the exe re-rolls without a cap; a scripted rand could loop forever
constexpr int kPlantRisingWithBungee = 2;
}

bool VasePoleOn() { return R("vasePole.enabled", 0) != 0; }

bool IsVasePole(int variant) { return variant == elite::VASE_POLE && VasePoleOn(); }

int VasePoleVaultColumn(int zombieX, bool hypnotised) {
	const int aDiv = R("vasePole.colDivisor", 80);
	const int aCol = (static_cast<uint16_t>(zombieX) / (aDiv > 0 ? aDiv : 80)) & 0xff; // div bx; xor ah, ah
	return aCol + (hypnotised ? 1 : -1);
}

int VasePoleLandColumn(int pixelGridX, bool hypnotised) { return pixelGridX + (hypnotised ? -1 : 1); }

bool VasePoleMakesOwnVase(bool hypnotised) { return !hypnotised; }

bool VasePolePlantLeft(bool dead, bool squished, int bungeeState) {
	return !dead && !squished && bungeeState != kPlantRisingWithBungee;
}

bool VasePoleZombieAccepted(int zombieType) {
	if (zombieType < 0 || zombieType > 32) return false; // ZOMBIE_REDEYE_GARGANTUAR = 32
	const int aMask = R("vasePole.zombieRejectMask", 0);
	return zombieType >= 31 || ((aMask >> zombieType) & 1) == 0;
}

int VasePoleRollZombie(const std::function<int(int)>& rand) {
	const int aRange = R("vasePole.zombieRollRange", 33);
	for (int i = 0; i < kMaxRerolls; i++) {
		const int aType = rand(aRange);
		if (VasePoleZombieAccepted(aType))
			return aType;
	}
	return 0; // ZOMBIE_NORMAL
}

unsigned VasePoleCatchFlags(bool poleHypnotised) {
	return static_cast<unsigned>(poleHypnotised ? R("vasePole.catchFlagsHypnotised", 1) : R("vasePole.catchFlags", 0x81));
}

bool VasePoleRectsOverlap(const VaseRect& a, const VaseRect& b) {
	return a.x + a.w > b.x && b.x + b.w > a.x && a.y + a.h > b.y && b.y + b.h > a.y;
}

bool VasePoleCatches(const VaseCatchInput& in) {
	return !in.self && !in.bossClass && in.sameRow && in.overlap && in.effected;
}

bool VasePoleCaughtPotHypnotised(bool caughtHypnotised) { return !caughtHypnotised; }

int VasePolePotState() { return R("vasePole.potState", 5); }

bool VasePolePotSetsSide() { return R("vasePole.potSetsSide", 0) != 0; }

bool VasePoleAwardAllowed(bool vasebreakerLevel) {
	return vasebreakerLevel || R("vasePole.awardOutsideVasebreaker", 1) != 0;
}

const std::vector<std::string>& VasePoleRuleKeys() {
	static const std::vector<std::string> keys = {
		"vasePole.enabled", "vasePole.colDivisor", "vasePole.zombieRollRange", "vasePole.zombieRejectMask",
		"vasePole.catchFlags", "vasePole.catchFlagsHypnotised", "vasePole.potState", "vasePole.potSetsSide",
		"vasePole.awardOutsideVasebreaker",
	};
	return keys;
}

} // namespace pvzu::unnamed
