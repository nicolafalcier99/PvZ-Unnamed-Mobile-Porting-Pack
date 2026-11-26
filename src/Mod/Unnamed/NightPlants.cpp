#include "Mod/Unnamed/NightPlants.h"

#include "Mod/GameData.h"
#include "Mod/Unnamed/Rules.h"

namespace pvzu::unnamed {

namespace {
constexpr int kSeedPuffshroom = 8, kSeedSeashroom = 24; // engine SeedType (same as the exe)

// Rule value while the feature is on, else the vanilla value.
int Night(const char* key, int vanilla) {
	return NightPlantsOn() ? RuleInt(key, vanilla) : vanilla;
}
}

bool NightPlantsOn() {
	return GameData::Get().Feature("nightPlants");
}

// Puff-shroom / Sea-shroom ------------------------------------------------------------------------

int PuffFreezeRollRange() {
	return Night("nightPlants.puff.freezeRollRange", 100);
}

bool PuffFreezesEater(int seedType, bool asleep, int roll) {
	if (asleep || (seedType != kSeedPuffshroom && seedType != kSeedSeashroom))
		return false;
	const int offset = Night("nightPlants.puff.freezeChanceOffset", -1000); // vanilla: never
	return roll < seedType + offset;
}

int PuffFreezeChillCs() {
	return Night("nightPlants.puff.freezeChillCs", 0);
}

// HitIceTrap and ApplyChill -----------------------------------------------------------------------

int IceTrapDamage() {
	return Night("nightPlants.iceTrap.damage", 20);
}

int ChillArmBodyDamage(bool hasArm) {
	return hasArm ? Night("nightPlants.chill.armBodyDamage", 0) : 0;
}

unsigned ChillArmDamageFlags() {
	return static_cast<unsigned>(Night("nightPlants.chill.armDamageFlags", 10));
}

// Sun-shroom --------------------------------------------------------------------------------------

int SunShroomGrowCs() {
	return Night("nightPlants.sunshroom.growCs", 12000);
}

int SunShroomBonusRollRange() {
	return Night("nightPlants.sunshroom.bonusRollRange", 100);
}

bool SunShroomBonusSun(bool small, int roll) {
	return !small && roll <= Night("nightPlants.sunshroom.bonusMaxRoll", -1);
}

// Fume-shroom -------------------------------------------------------------------------------------

int FumeAttackWidth() {
	return Night("nightPlants.fume.attackWidthPx", 340);
}

// Grave Buster ------------------------------------------------------------------------------------

bool GraveBusterRefunds() {
	return GraveBusterRefundCoin() > 0;
}

int GraveBusterRefundCoin() {
	return Night("nightPlants.graveBuster.refundCoin", 0); // 0 = COIN_NONE: no refund
}

int GraveBusterBonusRollRange() {
	return Night("nightPlants.graveBuster.bonusRollRange", 3);
}

bool GraveBusterBonusSun(int roll) {
	return GraveBusterRefunds() && GraveBusterBonusCoin() > 0 && roll == Night("nightPlants.graveBuster.bonusRoll", -1);
}

int GraveBusterBonusCoin() {
	return Night("nightPlants.graveBuster.bonusCoin", 0);
}

// Hypno-shroom ------------------------------------------------------------------------------------

bool HypnoGivesBucket(int zombieType) {
	if (zombieType < 0 || zombieType > 30)
		return false;
	const unsigned mask = static_cast<unsigned>(Night("nightPlants.hypno.bucketTypesMask", 0));
	return HypnoBucketHp() > 0 && ((mask >> zombieType) & 1u) != 0;
}

int HypnoBucketHp() {
	return Night("nightPlants.hypno.bucketHp", 0);
}

// Scaredy-shroom ----------------------------------------------------------------------------------

int ScaredyHp() {
	return Night("nightPlants.scaredy.hp", 300);
}

bool ScaredySecondShot(int launchCounter) {
	return launchCounter == Night("nightPlants.scaredy.secondShotAtCounter", -1000); // vanilla: never
}

// Doom-shroom -------------------------------------------------------------------------------------

int DoomRadius() {
	return Night("nightPlants.doom.radiusPx", 250);
}

int DoomRowRange() {
	return Night("nightPlants.doom.rowRange", 3);
}

int DoomCraterCs() {
	return Night("nightPlants.doom.craterCs", 18000);
}

} // namespace pvzu::unnamed
