// PVZU: engine side of the Unnamed Roof plants (Plan 6 Task 1). Each function is called from one `// PVZU:` site in
// Plant.cpp, Projectile.cpp, Zombie.cpp or Board.cpp and does nothing unless the active pack has the feature
// "roofPlants". Rules: Mod/Unnamed/RoofPlants.h; evidence: the research notes (w5-roof) section 1 and
// pvz-unnamed-pack/SOURCES-roof-plants.md.
//
// Dev option: -pvzu-plant-test on a roof (or -pvzu-plant-test-roof anywhere) lays out every Roof plant on Flower Pots
// and keeps zombies coming, like the Day/Night/Pool/Fog plant tests (Mod/Engine/DevHarness.h); -pvzu-log-plants <file>
// logs each ability.
#pragma once

#include <string_view>

class Board;
class Plant;
class Projectile;
class Zombie;

namespace pvzu::engine {

// One -pvzu-log-plants line for a Roof ability, "<SEED> col c row r: text" like the other worlds' lines.
void RoofLogPlant(const Plant* thePlant, std::string_view theText);

// Plant::Fire end: Cabbage-pult big quick throw (hook 352 -> 0xc05660), Kernel-pult pity Cob and butter streak
// (0x797643, 0x95a780).
void RoofShotFired(Plant* thePlant, Projectile* theProjectile);
// Plant::UpdateShooting after a lobber's Fire: the Melon / Winter Melon extra volley (hook 329). theTarget = the
// zombie the normal shot aimed at (no volley without one, and it gets no second melon).
void RoofMelonVolley(Plant* thePlant, Zombie* theTarget, int thePlantWeapon);
// Plant::UpdateProductionPlant when a Marigold's timer fires: the shrink pulse, then the Marigold dies (cave 0x6517b2).
// True = handled (no coin).
bool RoofMarigoldPulse(Plant* theMarigold);

// Zombie::Update before mZombieAge++: Garlic poison aura (hook 618 -> 0x7953c3).
void RoofGarlicPoison(Zombie* theZombie);
// Zombie::EatPlant per bite, before the bite: Garlic chills and may hypnotise its eater (cave 0x651374).
void RoofGarlicBitten(Zombie* theZombie, Plant* thePlant);
// Zombie::EatPlant: a Marigold is chewed without harm (hook 680). True = no bite.
bool RoofBiteImmune(Zombie* theZombie, Plant* thePlant);

// Projectile::DoImpact around Zombie::ApplyButter: the butter shot whose streak sets the butter time (hook 720 reads
// the projectile in EDI). Zombie::ApplyButter then asks RoofButterCs for its new mButteredCounter.
struct RoofButterShot {
	explicit RoofButterShot(const Projectile* theShot);
	~RoofButterShot();
	RoofButterShot(const RoofButterShot&) = delete;
	RoofButterShot& operator=(const RoofButterShot&) = delete;
	const Projectile* mPrevious;
};
int RoofButterCs(Zombie* theZombie, int theVanilla);
// Projectile::DoImpact single-target damage (0xc05700) and Projectile::Draw scale (hook 408) of a big cabbage.
int RoofShotDamage(Projectile* theProjectile, Zombie* theZombie, int theTableDamage);
float RoofShotDrawScale(const Projectile* theProjectile, float theVanillaScale);
// Projectile::FindCollisionTargetPlant: zombie shots pass over Flower Pots, Starfruit, a hiding Scaredy (hook 374).
bool RoofZombieShotSkips(Projectile* theProjectile, Plant* thePlant);
// Board::CanAddGraveStoneAt: no grave where a Flower Pot is (hook 18).
bool RoofGraveBlockedByPot(Board* theBoard, int theGridX, int theGridY);


// Dev harness (Mod/Engine/DevHarness.cpp): true when the roof plant test runs on this board.
bool RoofPlantTestActive(Board* theBoard);
void RoofPlantTestTick(Board* theBoard, int thePlayTicks);

} // namespace pvzu::engine
