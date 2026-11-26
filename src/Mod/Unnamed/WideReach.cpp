// PVZU: Unnamed's gameplay edge (see WideReach.h).
#include "Mod/Unnamed/WideReach.h"

#include "Mod/Unnamed/Rules.h"

namespace pvzu::unnamed {

int ZombieHittableMaxX(int vanilla) { return RuleInt("wideReach.zombieHitMaxX", vanilla); }

float ProjectileMaxX(float vanilla) { return RuleFloat("wideReach.projectileMaxX", vanilla); }

const std::vector<std::string>& WideReachRuleKeys() {
	static const std::vector<std::string> keys = {"wideReach.zombieHitMaxX", "wideReach.projectileMaxX"};
	return keys;
}

} // namespace pvzu::unnamed
