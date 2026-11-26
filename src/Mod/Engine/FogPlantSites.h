// PVZU: engine side of the Unnamed Fog plants (Plan 5 Task 1). Each function is called from one `// PVZU:` site in
// Plant.cpp, Projectile.cpp, Zombie.cpp or Board.cpp and does nothing unless the active pack has the feature
// "fogPlants". Rules: Mod/Unnamed/FogPlants.h; evidence: the research notes (w4-fog) section 2 and
// pvz-unnamed-pack/SOURCES-fog-plants.md.
//
// Dev option: -pvzu-plant-test on a fog lawn (or -pvzu-plant-test-fog anywhere) lays out every Fog plant and keeps
// zombies coming, like the Day/Night/Pool plant tests (Mod/Engine/DevHarness.h); -pvzu-log-plants <file> logs each ability.
#pragma once

#include <string_view>

class Board;
class Plant;
class Projectile;
class Zombie;

namespace pvzu::engine {

// One -pvzu-log-plants line for a Fog ability, "<SEED> col c row r: text" like the Day/Night/Pool lines.
void FogLogPlant(const Plant* thePlant, std::string_view theText);

// Plant::UpdateShooter when the launch counter ran out: vanilla `mLaunchRate - theRoll`, halved next to a Plantern.
int FogShooterLaunchCounter(Plant* theShooter, int theRoll);
// Plant::UpdateAbilities before UpdateShooting: a Plantern heals its 3x3 every 4 cs (hook 306).
void FogPlanternHeal(Plant* thePlantern);
// Plant::UpdateShooter at launch counter 25: the Cactus' second volley (hook 290).
void FogCactusSecondVolley(Plant* theCactus);
// Plant::FindTargetAndFire after the anim branches: every Cactus shot plays at 45 with shooting counter 23 (hook 282).
void FogCactusShotAnim(Plant* theCactus);

// Plant::UpdateShooting, first line: the Split Pea sweep (hooks 314, 325). Holds the launch counter while sweeping
// and fires the front head every 6 cs.
void FogSplitPeaShooting(Plant* theSplitPea);
// Plant::Fire end, Split Pea / Cattail shots: fire / snow peas, the sweep roll and sweep velocity, the 3 % Cob.
void FogShotFired(Plant* thePlant, Projectile* theProjectile, int thePlantWeapon);
// Plant::StarFruitFire: each star x1.5 and the boomerang tag (hook 286); after the five, the 1/3 roll and its four spiral
// stars (hook 287; none in I, Zombie Endless). Study: the research notes (r1-starfruit).
void FogStarfruitStar(Projectile* theStar);
void FogStarfruitVolleyDone(Plant* theStarfruit);
// Projectile::Update right after mProjectileAge++ (hook 400 -> 0x95b170): the boomerang turns and the spiral velocity.
// Skipped in 7-5, Glutton Star (0x89) and Bullet Carnival (0x8b) (0x95b700).
void FogStarCurveTick(Projectile* theStar);
// Projectile::CheckForCollision holds one of these for its whole body: Die() calls made inside it (its impact, its field
// edges) are the exe's "ebp == esi" calls that a star can live through (cave 0x6511e3).
class StarCollisionScope {
public:
	explicit StarCollisionScope(Projectile* theProjectile);
	~StarCollisionScope();
	StarCollisionScope(const StarCollisionScope&) = delete;
	StarCollisionScope& operator=(const StarCollisionScope&) = delete;
private:
	Projectile* mPrevious;
};
// Projectile::Die, first lines: true = a star inside its own CheckForCollision lives on (254 times for an ordinary or blue
// star, 4 red, 2 grey, never purple / green; always for a 7-5 / Glutton Star snake star).
bool FogStarSurvivesDie(Projectile* theProjectile);
// Plant::UpdateMagnetShroom when it pulls: one log line (the whole-lawn reach itself is two tests at the site).
void FogMagnetPull(Plant* theMagnet, Zombie* theZombie);

// Plant::BlowAwayFliers, first line: the Unnamed storm. True = handled (skip the vanilla body).
bool FogBloverStorm(Plant* theBlover);

// Zombie::SquishAllInSquare per plant: a Pumpkin smashed by Gargantuar / Giga / Squash-head explodes (true = handled,
// skip the vanilla squish); a Cactus run over by Zomboni / Catapult leaves a Spikeweed (false: vanilla squish follows).
bool FogSquish(Zombie* theZombie, Plant* thePlant);
// Zombie::EatPlant before the bite: a Pumpkin at <= 72 HP turns into a Cherry Bomb that goes off next tick. True = no bite.
bool FogPumpkinBitten(Zombie* theZombie, Plant* thePlant);

// Projectile::DoImpact before Die(): a Cactus spike lives through up to 64 hits (hook 806). True = keep flying. (Stars:
// FogStarSurvivesDie; the Die cave 0x651ce7 spare hit runs for motion 0 / 6 only, never for a star.)
bool FogShotSurvivesHit(Projectile* theProjectile);

// Dev harness (Mod/Engine/DevHarness.cpp): true when the fog plant test runs on this board.
bool FogPlantTestActive(Board* theBoard);
void FogPlantTestTick(Board* theBoard, int thePlayTicks);

} // namespace pvzu::engine
