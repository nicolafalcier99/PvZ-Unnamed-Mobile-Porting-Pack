// PVZU: Plan 9 R9 small rules, engine side (rules: Mod/Unnamed/SmallRules.h; evidence pvz-unnamed-pack/SOURCES-small-rules.md,
// the research notes (r9-small-rules)).
//   * Plain stars: x2 table damage outside I, Zombie (0xc10025, key modPlants.projectileDamage.starDouble) and the pity
//     hypnotise (feature smallRules): star counter +1 per star hit, then a plain star on a zombie that is not boss class
//     hypnotises it at a full counter or on Rand(14370) == 0.
//   * (The vanilla Coffee Bean's heal, hook 362, is ModPlantsSpecials.h CoffeeBeanDies; the dev scene below shows it.)
// Dev: -pvzu-dev-pity-counter <n> (start value of the star counter), -pvzu-dev-small <tick> (a Starfruit lane and a
// Coffee Bean on a hurt Sun-shroom; lines in -pvzu-log-plants).
#pragma once

class Board;
class Plant;
class Projectile;
class Zombie;

namespace pvzu::engine {

// Projectile::DoImpact, non-splash branch, a zombie hit, before TakeDamage (hooks 396 / 397). Returns the damage.
int SmallRulesShotHit(Projectile* theShot, Zombie* theZombie, int theDamage);
// A kind star (Dream Starfruit, Spirit-tail; ModShotImpact) still counts for the pity counter (0xc10000).
void SmallRulesCountKindStar(Projectile* theShot);
// Board tick (DevBoardTick): the -pvzu-dev-small scene.
void SmallRulesDevTick(Board* theBoard, int thePlayTicks);

int SmallRulesPityCounter(); // dev / log: the star counter (exe 0xc10033)

} // namespace pvzu::engine
