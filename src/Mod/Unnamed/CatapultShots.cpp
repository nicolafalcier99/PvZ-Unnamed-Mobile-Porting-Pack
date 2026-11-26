// PVZU: Plan 9 R3, Catapult projectiles 0x12 / 0x20 and the vase Catapult's volley. Exe addresses per rule in
// the research notes (r3-catapult-shots) and pvz-unnamed-pack/SOURCES-catapult-shots.md.
#include "Mod/Unnamed/CatapultShots.h"

#include "Mod/Unnamed/Rules.h"

#include <algorithm>
#include <cstdlib>

namespace pvzu::unnamed {

namespace {

int R(const char* key, int vanilla) { return RuleInt(key, vanilla); }

bool ListHas(const char* key, int value) {
	if (!HasRule(key))
		return false;
	const nlohmann::json* a = RuleArray(key);
	if (a == nullptr)
		return false;
	for (const auto& v : *a)
		if (v.is_number_integer() && v.get<int>() == value)
			return true;
	return false;
}

constexpr int kVaseCatapult = 0x70;   // World7Zombies w7::VASE_CATAPULT
constexpr int kTracks = 4;            // Zombie_catapult_basketball .. basketball4
constexpr int kSeedSquash = 17;       // SEED_SQUASH: states 5..7 = in the air (0x7aa3c3)
constexpr int kBungeeRising = 2;      // PLANT_RISING_WITH_BUNGEE (+0x134)

} // namespace

bool IsCatapultShotType(int theExeType) { return theExeType == kShotVase || theExeType == kShotBugSpray; }

// ---- volley ----

int CatapultVolleyRollRange(int theVariant, bool theMindControlled) { // 0x958ed7 / 0x910135
	if (theVariant != kVaseCatapult || theMindControlled)
		return 0;
	return R("catapultShots.volleyRoll", 0);
}

bool CatapultVolleys(int theRoll) { return theRoll < R("catapultShots.volleyBelow", 0); }   // 0x91014d `cmp 0x23`

int VolleyShotRollRange() { return R("catapultShots.volleyShotRoll", 0); }   // 0x7a1955 Rand(3)

int VolleyShotType(int theRoll) { return (theRoll & 1) ? kShotVase : kShotBugSpray; }   // `and 1; imul -0xe; add 0x20`

std::vector<int> VolleyTargets(const std::vector<VolleyCandidate>& thePlantsInRow) { // 0x9101bc..0x910211
	std::vector<int> aOut;
	const int aLast = R("catapultShots.volleyLastCol", -1);
	std::vector<bool> aUsed(aLast >= 0 ? static_cast<size_t>(aLast) + 1 : 0, false);
	for (size_t i = 0; i < thePlantsInRow.size(); ++i) {
		const VolleyCandidate& p = thePlantsInRow[i];
		if (p.modSeed != 0 ? ListHas("catapultShots.volleySkipModSeeds", p.modSeed) : ListHas("catapultShots.volleySkipSeeds", p.seed))
			continue;
		if (p.col < 0 || p.col > aLast || aUsed[static_cast<size_t>(p.col)])
			continue;
		aUsed[static_cast<size_t>(p.col)] = true;
		aOut.push_back(static_cast<int>(i));
	}
	return aOut;
}

int VolleyBallsAfter(int theBalls) { // 0x910217
	const int aMin = R("catapultShots.volleyMinBalls", 0);
	return theBalls < aMin ? aMin : theBalls;
}

// ---- ball tracks ----

int CatapultHiddenBallTracks(int theBalls) { // 0x91027a: abs(balls - 4) + 1 tracks once balls < 5
	const int aBelow = R("catapultShots.ballTracksBelow", -1);
	if (aBelow < 0)
		return -1;
	if (theBalls >= aBelow)
		return 0;
	return std::min(std::abs(theBalls - (aBelow - 1)) + 1, kTracks);
}

// ---- vase ----

bool VaseShotMakesVase(int theExeType, int theDamageRangeFlags) { // 0x7a1401 / 0x7a140b
	return theExeType == kShotVase && R("catapultShots.vaseZombieRoll", 0) > 0 &&
	       theDamageRangeFlags != R("catapultShots.vaseReflectedFlags", 1);
}

int VaseShotDamage() { return R("catapultShots.vaseDamage", 0); }

bool VaseShotKillsPlant(int theSeed, int theState, bool theSquished, bool theDead, int theBungeeState) { // 0x7aa3c3..0x7aa3f4
	if (theSeed == kSeedSquash && theState >= 5 && theState <= 7)
		return false;
	return !theSquished && !theDead && theBungeeState != kBungeeRising;
}

int VaseZombieRollRange() { return R("catapultShots.vaseZombieRoll", 0); }

bool VaseZombieAllowed(int theZombieType) { // 0x7a1426..0x7a1451
	if (theZombieType < 0 || theZombieType >= VaseZombieRollRange())
		return false;
	return !ListHas("catapultShots.vaseZombieReroll", theZombieType);
}

// ---- bug spray ----

int SprayDamage() { return R("catapultShots.sprayDamage", 0); }
int SprayKillRollRange() { return R("catapultShots.sprayKillRoll", 0); }
bool SprayKills(int theRoll) { return SprayKillRollRange() > 0 && theRoll == 0; }

int SprayLaunchRate(int theLaunchRate, int theBaseLaunchRate) { // 0x91038e..0x9103e5
	const int aStep = R("catapultShots.sprayRateStep", 0);
	if (aStep <= 0)
		return theLaunchRate;
	const int aCap = theBaseLaunchRate + R("catapultShots.sprayRateCapExtra", 0);
	return aCap >= theLaunchRate ? theLaunchRate + aStep : aCap;
}

int SprayFoley() { return R("catapultShots.sprayFoley", -1); }
int SprayParticle() { return R("catapultShots.sprayParticle", -1); }
std::pair<int, int> SprayParticleOffset() { return {R("catapultShots.sprayParticleDx", 0), R("catapultShots.sprayParticleDy", 0)}; }
int SprayParticleRenderOrder() { return R("catapultShots.sprayParticleRenderOrder", 0); }

const std::vector<std::string>& CatapultShotRuleKeys() {
	static const std::vector<std::string> keys = {
		"catapultShots.volleyRoll", "catapultShots.volleyBelow", "catapultShots.volleyShotRoll",
		"catapultShots.volleyLastCol", "catapultShots.volleyMinBalls", "catapultShots.volleySkipSeeds",
		"catapultShots.volleySkipModSeeds", "catapultShots.ballTracksBelow", "catapultShots.vaseDamage",
		"catapultShots.vaseZombieRoll", "catapultShots.vaseZombieReroll", "catapultShots.vaseReflectedFlags",
		"catapultShots.sprayDamage", "catapultShots.sprayKillRoll", "catapultShots.sprayRateStep",
		"catapultShots.sprayRateCapExtra", "catapultShots.sprayFoley", "catapultShots.sprayParticle",
		"catapultShots.sprayParticleDx", "catapultShots.sprayParticleDy", "catapultShots.sprayParticleRenderOrder",
	};
	return keys;
}

} // namespace pvzu::unnamed
