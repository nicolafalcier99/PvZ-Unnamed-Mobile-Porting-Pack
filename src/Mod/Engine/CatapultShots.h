// PVZU: Plan 9 R3, engine side of Unnamed's Catapult projectiles 0x12 (vase) and 0x20 (bug spray) and the vase
// Catapult's volley. Each function is called from one `// PVZU:` site in Zombie.cpp / Projectile.cpp or from the
// world-7 shot roll (World7EliteZombies.cpp), and does nothing (or returns the vanilla answer) unless the active pack
// has the feature "catapultShots". Rules: Mod/Unnamed/CatapultShots.h; evidence: the research notes (r3-catapult-shots)
// and pvz-unnamed-pack/SOURCES-catapult-shots.md.
//
// The shots are engine basketballs with ProjectileState::modType = 0x12 / 0x20 (Mod/Unnamed/ModShots.h), so flight,
// umbrella and collision stay vanilla; their plant hit is CatapultShotHitsPlant, their art ModShotLook.
//
// Dev option (with -pvzu-force-variant CATAPULT:0x70 / CATAPULT:0x19, -pvzu-dev-player-level 65, -pvzu-dev-spawn ...):
//   -pvzu-dev-catapult-shot volley|spray|vase   force the rolls: volley = every vase-Catapult fire event is a volley;
//                                               spray = never a volley, every single shot of a vase Catapult is 0x20;
//                                               vase = every pole-head shot in world 7 is 0x12, and volley shots too
// Log lines carry the word "catapultShots" (with -pvzu-log-zombies <file>).
#pragma once

class Plant;
class Projectile;
class Zombie;

namespace pvzu::engine {

bool CatapultShotsOn();

// UpdateZombieCatapult at the fire event, before the vanilla FindCatapultTarget + ZombieCatapultFire: true = the vase
// Catapult threw its volley (hook 547 / 548), skip the vanilla shot.
bool CatapultVolley(Zombie* theCatapult);
// ZombieCatapultFire, through World7CatapultShot: during a volley the shot's type is Rand(3) (hook 537 with
// [0x701240] = 1). true = typed here, skip the world-7 per-shot roll.
bool CatapultVolleyShot(Projectile* theShot);
// World7CatapultShot: the rolled exe type (0x12 / 0x20) becomes the shot's mod type.
void CatapultTagShot(Zombie* theCatapult, Projectile* theShot, int theExeType);
// the world-7 shot roll with -pvzu-dev-catapult-shot (theRoll otherwise)
int CatapultDevShotRoll(int theVariant, int theRoll);

// UpdateZombieCatapult after the loop-end mSummonCounter-- and the vanilla track hides: the mod's count (hook 549).
void CatapultBallTracks(Zombie* theCatapult);

// Projectile::UpdateLobMotion, a basketball-carried shot hitting a plant without an umbrella: the HP the caller takes
// off. Vase: the cell's plants die and a vase with a random zombie is left (returns 300 for the dead plant); bug spray:
// 100 damage, then 1 in 20 the plant dies, else it shoots slower (returns 0, the damage is done here).
int CatapultShotHitsPlant(Projectile* theShot, Plant* thePlant, int theVanillaDamage);

} // namespace pvzu::engine
