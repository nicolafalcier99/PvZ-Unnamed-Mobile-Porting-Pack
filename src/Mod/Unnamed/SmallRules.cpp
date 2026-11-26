// PVZU: Plan 9 R9 small rules. Evidence per function: pvz-unnamed-pack/SOURCES-small-rules.md.
#include "Mod/Unnamed/SmallRules.h"

#include "Mod/GameData.h"
#include "Mod/Unnamed/Rules.h"

namespace pvzu::unnamed {

namespace {
constexpr int kProjStar = 7; // the exe's PROJECTILE_STAR (engine value is the same)

int SR(const char* key) { return SmallRulesOn() ? RuleInt(key, 0) : 0; }
}

bool SmallRulesOn() {
	return GameData::Get().Feature("smallRules");
}

int PityRollRange() { return SR("smallRules.pity.rollRange"); }

int PityCounterAt() { return SR("smallRules.pity.counterAt"); }

bool PityCountsStar(int projectileType, bool iZombieLevel) { // 0xc10000: cmp ebx, 7; call 0x4537d0
	return PityRollRange() > 0 && projectileType == kProjStar && !iZombieLevel;
}

bool PityCandidate(int projectileType, int field80, bool shrapnelCore, bool zombieBossClass) {
	// 0xc0571e stars only; 0x79ef88 +0x84 == 2 skips; 0xc10050 jg kinds / jne negative; 0x7aa320 boss class
	return PityRollRange() > 0 && projectileType == kProjStar && field80 == 0 && !shrapnelCore && !zombieBossClass;
}

bool PityNeedsRoll(int counter) {
	const int at = PityCounterAt();
	return PityRollRange() > 0 && (at <= 0 || counter < at); // cmp [0xc10033], 0x3822; jge (no Rand)
}

bool PityHypnotises(int counter, int roll) {
	if (PityRollRange() <= 0)
		return false;
	const int at = PityCounterAt();
	return (at > 0 && counter >= at) || roll == 0;
}

} // namespace pvzu::unnamed

namespace pvzu::unnamed {

bool FlagsHitFlying(unsigned theFlags) {
	const int aFlags = SR("smallRules.flyingHitByFlags"); // 0xc11cb8 `cmp bl, 0xd`
	return aFlags != 0 && static_cast<int>(theFlags & 0xffu) == aFlags;
}

} // namespace pvzu::unnamed
