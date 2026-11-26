// PVZU: Plan 8 part 3a: engine wiring of Unnamed's shooter / projectile mod plants and the shared mod projectile rules.
// Plants: Shadow Peashooter 0x50, Mystic-Ice Shooter 0x55, Primal Repeater 0x57, Dream Starfruit 0x6d, Dusk-pult 0x70,
// Butter-pult 0x72, Sniper 0x78, Element-shroom 0x7a, Spirit-tail 0x7b, Loaded Cannon 0x7f. Shared: the mod projectile
// types (Mod/Unnamed/ModShots.h: 0x16, 0x21, 0x22, 0x23 on vanilla carriers), their damage table (ModProjectileDamage),
// star kinds 1..5, "weakened" (x2 damage), Repeater poison, the Loaded Cannon burn cap.
// Rules: Mod/Unnamed/ModPlantRules.h (pure); evidence: the research notes (mod-plants-behaviour) (one section per plant).
//
// Everything here runs only for planted mod plants (ModPlantSites.cpp dispatches by mod seed) or for projectiles and
// zombies those plants touched (ProjectileState::modType / field80, all 0 otherwise; the zombie statuses "weakened" and
// Repeater poison belong to Mod/Engine/ModZombieStatus.h, which these plants call), so vanilla and ordinary Unnamed plants are unchanged. The engine sites call the functions below from one
// `// PVZU:` line each.
//
// Other mod plants can use the projectile helper AddModShot (a mod projectile); weakened time is ModZombieAddWeakened.
//
// Dev options (Mac checks):
//   -pvzu-dev-shooters <tick>   at that tick of play, plant the ten shooter plants on a fixed layout (log each)
//   -pvzu-dev-element-ice       with -pvzu-dev-shooters: the Element-shroom is toggled to ice when planted (a click)
//   -pvzu-dev-spirit-type <n>   Spirit-tail always fires exe projectile type n (after its roll; art checks, e.g. 22 prism)
//   -pvzu-dev-shadow            treat every mod plant as shadowed (until the shadow state is wired, for shadow checks)
//   -pvzu-dev-cannon <k|d>@<tick>[,...]  at that tick, load the Loaded Cannon with a Kernel-pult (k) or Doom-shroom (d)
//                               as a planted packet would (no sun); when READY it fires at the first zombie it finds
#pragma once

#include "ConstEnums.h"

class Board;
class Plant;
class Projectile;
class Zombie;
namespace Sexy {
class Color;
class Image;
}

namespace pvzu::engine {

// Mod seeds handled here (0x50 0x55 0x57 0x6d 0x70 0x72 0x78 0x7a 0x7b 0x7f).
bool IsShooterModSeed(SeedType theModSeed);

// ModPlantSites dispatch (only for IsShooterModSeed plants).
void ShooterOnPlanted(Plant* thePlant);
bool ShooterUpdate(Plant* thePlant);
bool ShooterFire(Plant* thePlant, Zombie* theTargetZombie, int theRow, int thePlantWeapon);

// Engine sites -----------------------------------------------------------------------------------------------------
// Plant::FindTargetAndFire with no target in theRow: a shadowed Dusk-pult also looks at row+1, then row-1 (hook 279).
Zombie* ShooterExtraTarget(Plant* thePlant, int theRow, int thePlantWeapon);
// Plant::FindStarFruitTarget: true = a shadowed Dream Starfruit fires at anything (hook 284).
bool ShooterStarfruitAlwaysTargets(Plant* thePlant);
// Plant::MouseDown: Element-shroom toggles fire / ice (0x7a8ef1). true = handled.
bool ShooterPlantClicked(Plant* thePlant);
// Plant::UpdateReanimColor for a mod plant: Dream Starfruit's flicker colour (0x959de0). false = keep theColor.
bool ShooterPlantColor(const Plant* thePlant, Sexy::Color& theColor);
// Board::MouseDownWithPlant: a Kernel-pult / Doom-shroom packet dropped on an arming Loaded Cannon loads it (0xc11870).
// true = handled (paid, packet used, cursor cleared).
bool ShooterCannonLoadClick(Board* theBoard, int theGridX, int theGridY);

// Projectile::DoImpact at its start: a mod shot or a star with a kind does its whole impact here. true = handled.
bool ModShotImpact(Projectile* theProjectile, Zombie* theZombie);
// Projectile::Update after the click-backoff countdown: a kind star with a return timer turns homing (hook 403).
void ModShotUpdate(Projectile* theProjectile);
// Projectile::Draw after the image is chosen: mod art (carrier image swap, tint). theTint stays white when untinted.
void ModShotLook(const Projectile* theProjectile, Sexy::Image*& theImage, float& theScale, Sexy::Color& theTint);
// Projectile::UpdateLobMotion around a cob's KillAllZombiesInRadius: a Kernel-loaded cob burns with cap 3000.
class ModCobBurnCapScope {
public:
	explicit ModCobBurnCapScope(const Projectile* theCob);
	~ModCobBurnCapScope();
	ModCobBurnCapScope(const ModCobBurnCapScope&) = delete;
	ModCobBurnCapScope& operator=(const ModCobBurnCapScope&) = delete;
private:
	bool mSet = false;
};

// Zombie::ApplyBurn under a burn cap (0xc11770): 0 = no cap (vanilla), > 0 = TakeDamage(that, 18) instead, -1 = burn.
int ModBurnCapDamage(const Zombie* theZombie);

// Helpers for other mod plants ---------------------------------------------------------------------------------------
// A projectile of exe type theExeType (0..13 or a mod type 0x14..0x23) on its carrier; nullptr for an unknown type.
Projectile* AddModShot(Board* theBoard, int theX, int theY, int theRenderOrder, int theRow, int theExeType);

// Dev harness (ModPlantsDevTick).
void ShootersDevTick(Board* theBoard, int thePlayTicks);

} // namespace pvzu::engine
