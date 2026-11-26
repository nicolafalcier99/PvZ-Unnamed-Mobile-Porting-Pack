// PVZU: Plan 8 part 1 leaves these empty (every mod plant behaves as its base plant). Plan 8 part 3 dispatches them to
// the wiring of each plant group (ModPlantsShooters.*, ModPlantsSpecials.*, ModPlantsShadow.*).
#include "Mod/Engine/ModPlantSites.h"

#include "Mod/Engine/ModPlants.h"
#include "Mod/Engine/ModPlantsSpecials.h" // PVZU Plan 8 part 3b
#include "Mod/Engine/ModPlantsShooters.h" // Plan 8 part 3a
#include "Mod/Engine/ModPlantsShadow.h" // Plan 8 part 3c

#include "Lawn/Plant.h"

namespace pvzu::engine {

SeedType ModSeed(const Plant* thePlant) {
	return thePlant && thePlant->mPvzu.modSeed != 0 ? static_cast<SeedType>(thePlant->mPvzu.modSeed) : SeedType::SEED_NONE;
}

// Each group decides for its own seeds (shooters: IsShooterModSeed; specials and shadow: their own seed checks).
void ModPlantOnPlanted(Plant* thePlant) {
	ModPlantShadowOnPlanted(thePlant); // Plan 8 part 3c: Moonflower first sun timer
	if (IsShooterModSeed(ModSeed(thePlant))) // Plan 8 part 3a: shooter plants (ModPlantsShooters.h)
		ShooterOnPlanted(thePlant);
	SpecialsOnPlanted(thePlant); // Plan 8 part 3b
}

bool ModPlantUpdate(Plant* thePlant) {
	ModPlantShadowTick(thePlant); // Plan 8 part 3c: shadow state first (hook 311 -> 0x7b06b0), Moonflower sun
	if (IsShooterModSeed(ModSeed(thePlant))) // Plan 8 part 3a
		return ShooterUpdate(thePlant);
	if (SpecialsUpdate(thePlant)) // Plan 8 part 3b
		return true;
	return false;
}

bool ModPlantFire(Plant* thePlant, Zombie* theTargetZombie, int theRow, int thePlantWeapon) {
	if (IsShooterModSeed(ModSeed(thePlant))) // Plan 8 part 3a
		return ShooterFire(thePlant, theTargetZombie, theRow, thePlantWeapon);
	if (SpecialsFire(thePlant, theTargetZombie, theRow, thePlantWeapon)) // Plan 8 part 3b
		return true;
	return false;
}

bool ModPlantDoSpecial(Plant* thePlant) {
	if (SpecialsDoSpecial(thePlant)) // Plan 8 part 3b
		return true;
	return false;
}

void ModPlantOnDie(Plant* thePlant) {
	SpecialsOnDie(thePlant); // Plan 8 part 3b
	ModPlantShadowOnDie(thePlant); // Plan 8 part 3c: log line for the shadow family
}

} // namespace pvzu::engine
