// PVZU: engine side of Plan 9 V1 (see ZombieGapsV1.h). Evidence: the research notes (v1-zombie-contradictions).
#include "Mod/Engine/ZombieGapsV1.h"

#include "Mod/Engine/EliteZombies.h"
#include "Mod/GameData.h"
#include "Mod/Unnamed/ZombieGapsV1.h"

#include "LawnApp.h"
#include "Lawn/Board.h"
#include "Lawn/Plant.h"
#include "Lawn/Zombie.h"

namespace pvzu::engine {

namespace u = pvzu::unnamed;

namespace {
int Time(Zombie* z) { return z->mBoard ? static_cast<int>(z->mBoard->mMainCounter) : 0; }
}

bool ZombieGapsV1On() { return GameData::Get().Feature("zombieGapsV1"); }

unsigned int V1HitFlags(Zombie* z, unsigned int theFlags) {
	if (!ZombieGapsV1On() || !u::ClearsHitFlags(z->mZombieType, z->mPvzu.subVariant, z->mZombiePhase))
		return theFlags;
	return 0U; // 0xc05900: eax (flags) = 0 for a reading red-eye
}

bool V1MoonwalkerSavesHit(Zombie* z, int theDamage) {
	if (!ZombieGapsV1On())
		return false;
	const auto aBody = u::MoonwalkerSavedBody(z->mZombieType, z->mPvzu.variant, z->mZombiePhase, z->mHasArm, theDamage,
		z->mBodyHealth, z->mBodyMaxHealth);
	if (!aBody)
		return false;
	const int aBefore = z->mBodyHealth;
	z->mBodyHealth = *aBody;    // 0xc0544e
	z->UpdateDamageStates(0U);  // 0x52a470: drops the arm (body now under 2/3)
	z->mHasArm = false;         // 0xc05462
	LogZombieEvent("t=%d v1 white-summoner saved: damage=%d body %d -> %d", Time(z), theDamage, aBefore, z->mBodyHealth);
	return true;
}

int V1BodyDamage(Zombie* z, int theDamage) {
	return ZombieGapsV1On() && u::GaspIgnoresBodyDamage(z->mZombiePhase) ? 0 : theDamage;
}

bool V1BurnBecomesHit(Zombie* z) {
	return ZombieGapsV1On() && u::BurnBecomesHit(z->mZombieType, z->mPvzu.variant, z->mZombiePhase, z->mHasArm);
}

bool V1RefusesFreeze(Zombie* z) { return ZombieGapsV1On() && u::MoonwalkerRefusesFreeze(z->mPvzu.variant, z->mZombiePhase); }

void V1DancerEating(Zombie* z) {
	if (ZombieGapsV1On() && z->mZombieType == ZombieType::ZOMBIE_DANCER &&
		u::MoonwalkerBiteRestartsSummon(z->mPvzu.variant, z->mZombiePhase))
		z->mPhaseCounter = 0; // 0xc0552a: [edi+0x68] = 0
}

bool V1GarlicRipsPaper(Zombie* z) {
	if (!ZombieGapsV1On() || !u::GarlicRipsPaper(z->mZombieType, z->mPvzu.subVariant, z->mZombiePhase) ||
		z->mShieldType == ShieldType::SHIELDTYPE_NONE)
		return false;
	z->DropShield(0U); // 0x530a00 (sets the gasp phase)
	LogZombieEvent("t=%d v1 garlic rips the red-eye's paper row=%d x=%d", Time(z), z->mRow, z->mX);
	return true;
}

bool V1KnifeBitesAnyway(Zombie* z, Plant* thePlant) {
	if (!ZombieGapsV1On() || !u::KnifeBiteConditionsOn())
		return false;
	const bool aIce = z->mBoard && z->mBoard->mPvzuState.iceFreezeActive != 0;
	return !u::KnifeSkipsBite(z->mZombiePhase, z->mHasObject, aIce, thePlant->mPvzu.frozen != 0);
}

void V1GreenFootballBite(Zombie* z, Plant* thePlant) {
	if (!ZombieGapsV1On())
		return;
	const auto aBite = u::GreenFootballBite(z->mZombieType, z->mPvzu.subVariant, thePlant->mPlantHealth,
		z->mHelmType != HelmType::HELMTYPE_NONE, z->mHelmHealth, z->mHelmMaxHealth, z->mBodyHealth, z->mBodyMaxHealth,
		z->mApp->IsIZombieLevel());
	if (!aBite)
		return;
	if (aBite->largeSun) // 0xc12f9f: AddCoin(mX, mY, 6, 2)
		z->mBoard->AddCoin(z->mX, z->mY, CoinType::COIN_LARGESUN, CoinMotion::COIN_MOTION_FROM_PLANT);
	thePlant->mPlantHealth = aBite->plantHealth;
}

bool V1VaultKeepsSpeed(Zombie* z) { return ZombieGapsV1On() && u::VaultKeepsSpeed(z->mPvzu.variant); }

void V1ZombieStep(Zombie* z) {
	if (ZombieGapsV1On() && u::PogoCrushesPlants(z->mZombieType, z->mPvzu.variant, z->mFlatTires))
		z->CheckSquish(ZombieAttackType::ATTACKTYPE_DRIVE_OVER); // 0x52ad71
}

bool V1PogoLandYucky(Zombie* thePogo, bool theFirstHit, bool thePortAnswer) {
	return ZombieGapsV1On() ? u::PogoLandMakesYucky(thePogo->mPvzu.variant, theFirstHit) : thePortAnswer;
}

float V1ZamboniVelX(Zombie* z, float theCurve) {
	(void)z;
	if (!ZombieGapsV1On())
		return theCurve;
	return u::PlainZamboniVelX().value_or(theCurve);
}

} // namespace pvzu::engine
