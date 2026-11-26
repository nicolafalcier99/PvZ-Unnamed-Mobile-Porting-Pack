// PVZU: engine side of the Unnamed Fog plants. Rules: Mod/Unnamed/FogPlants.h; evidence: the research notes (w4-fog)
// section 2 and pvz-unnamed-pack/SOURCES-fog-plants.md.
#include "Mod/Engine/FogPlantSites.h"

#include "Mod/CmdLine.h"
#include "Mod/Engine/Level65Snake.h"
#include "Mod/Engine/ModesVanilla.h"
#include "Mod/Engine/ModModes.h"
#include "Mod/Engine/ModPlants.h" // Plan 8 part 3b: ModSeedOf (the Pumpkin blasts compare the raw seed 0x1e)
#include "Mod/Unnamed/DayPlants.h"
#include "Mod/Unnamed/FogPlants.h"
#include "Mod/Unnamed/ModPlantRules.h"

#include "LawnApp.h"
#include "Lawn/Board.h"
#include "Lawn/Plant.h"
#include "Lawn/Projectile.h"
#include "Lawn/Zombie.h"
#include "PvzpLib/Reanimator.h"
#include "Resources.h"

#include <cstdio>
#include <cstdlib>
#include <format>

namespace u = pvzu::unnamed;

namespace pvzu::engine {

void FogLogPlant(const Plant* thePlant, std::string_view theText) {
	u::LogPlantAbility(thePlant->mBoard ? static_cast<int>(thePlant->mBoard->mMainCounter) : -1,
		std::format("{} col {} row {}: {}", GetPlantDefinition(thePlant->mSeedType).mPlantName, thePlant->mPlantCol, thePlant->mRow, theText));
}

namespace {
bool InPlanternSquare(const Plant* theA, const Plant* theB) {
	return std::abs(theA->mRow - theB->mRow) <= 1 && std::abs(theA->mPlantCol - theB->mPlantCol) <= 1;
}
}

// Plantern --------------------------------------------------------------------------------------------

int FogShooterLaunchCounter(Plant* theShooter, int theRoll) {
	const int aVanilla = theShooter->mLaunchRate - theRoll;
	if (!u::FogPlantsOn() || !theShooter->mBoard)
		return aVanilla;
	// cave 0x651670: the first live Plantern whose 3x3 holds the shooter (several do not stack)
	bool aNear = false;
	for (Plant* aPlant : theShooter->mBoard->mPlants) {
		if (!aPlant->mDead && aPlant->mSeedType == SeedType::SEED_PLANTERN && aPlant->mPvzu.modSeed == 0 && // not a Moonflower (0x19 only)
			InPlanternSquare(aPlant, theShooter)) {
			aNear = true;
			break;
		}
	}
	// raw seed (0x6516cb..0x651721): Spirit-tail / Sniper / Element-shroom get the halving, not their base's +25 / no-bonus
	const int aCounter = u::PlanternLaunchCounter(static_cast<int>(ModSeedOf(theShooter)), theShooter->mLaunchRate, theRoll, aNear);
	if (aCounter != aVanilla)
		FogLogPlant(theShooter, std::format("plantern.speedUp launch counter {} (vanilla {}, rate {})", aCounter, aVanilla, theShooter->mLaunchRate));
	return aCounter;
}

void FogPlanternHeal(Plant* thePlantern) {
	if (thePlantern->mSeedType != SeedType::SEED_PLANTERN || thePlantern->mPvzu.modSeed != 0 || !u::FogPlantsOn() || u::PlanternHealTickCs() <= 0 ||
		!thePlantern->mBoard || thePlantern->mStateCountdown >= 1)
		return;
	int aHealed = 0;
	for (Plant* aPlant : thePlantern->mBoard->mPlants) {
		if (aPlant->mDead || !InPlanternSquare(aPlant, thePlantern))
			continue;
		const int aHp = aPlant->mPlantHealth, aMax = aPlant->mPlantMaxHealth;
		u::PlanternHealStep(aPlant->mPlantHealth, aPlant->mPlantMaxHealth);
		if (aPlant->mPlantHealth != aHp || aPlant->mPlantMaxHealth != aMax) {
			++aHealed;
			// one line per 25 heal ticks of this Plantern (a heal runs 25 times a second)
			if (thePlantern->mPvzu.counter % 25 == 0)
				FogLogPlant(thePlantern, std::format("plantern.heal {} col {} row {}: hp {} -> {} max {} -> {}",
					GetPlantDefinition(aPlant->mSeedType).mPlantName, aPlant->mPlantCol, aPlant->mRow, aHp, aPlant->mPlantHealth, aMax, aPlant->mPlantMaxHealth));
		}
	}
	if (aHealed > 0)
		thePlantern->mPvzu.counter += 1;
	thePlantern->mStateCountdown = u::PlanternHealTickCs();
}

// Cactus ----------------------------------------------------------------------------------------------

void FogCactusSecondVolley(Plant* theCactus) {
	if (theCactus->mSeedType != SeedType::SEED_CACTUS || !u::CactusSecondVolley(theCactus->mLaunchCounter))
		return;
	// hook 290 (0x7a8850): the same weapon mapping as the first shot
	bool aFired = false;
	if (theCactus->mState == PlantState::STATE_CACTUS_HIGH)
		aFired = theCactus->FindTargetAndFire(theCactus->mRow, PlantWeapon::WEAPON_PRIMARY);
	else if (theCactus->mState == PlantState::STATE_CACTUS_LOW)
		aFired = theCactus->FindTargetAndFire(theCactus->mRow, PlantWeapon::WEAPON_SECONDARY);
	if (aFired)
		FogLogPlant(theCactus, std::format("cactus.secondVolley {}", theCactus->mState == PlantState::STATE_CACTUS_HIGH ? "high" : "low"));
}

void FogCactusShotAnim(Plant* theCactus) {
	float aRate = 0.0f;
	int aCounter = 0;
	if (theCactus->mSeedType != SeedType::SEED_CACTUS || !u::CactusShotAnim(aRate, aCounter))
		return;
	if (Reanimation* aBody = theCactus->mApp->ReanimationTryToGet(theCactus->mBodyReanimID))
		aBody->mAnimRate = aRate;
	theCactus->mShootingCounter = aCounter;
}

// Split Pea, Cattail --------------------------------------------------------------------------------------

void FogSplitPeaShooting(Plant* theSplitPea) {
	if (theSplitPea->mSeedType != SeedType::SEED_SPLITPEA || !u::FogPlantsOn() || !u::SplitPeaSweeping(theSplitPea->mPvzu))
		return;
	u::PlantState& s = theSplitPea->mPvzu;
	u::SplitPeaSweepTick(s); // hook 314
	theSplitPea->mLaunchCounter = u::SplitPeaSweepLaunchCounter(); // hook 325: normal fire held back
	if (!u::SplitPeaSweepFiresNow(s))
		return;
	const int aTimer = s.sweepTimer;
	if (u::SplitPeaSweepEnds(s)) { // hook 278
		FogLogPlant(theSplitPea, std::format("splitPea.sweepEnd after {} cs", aTimer));
		return;
	}
	theSplitPea->Fire(nullptr, theSplitPea->mRow, PlantWeapon::WEAPON_PRIMARY);
}

void FogShotFired(Plant* thePlant, Projectile* theProjectile, int thePlantWeapon) {
	if (!u::FogPlantsOn())
		return;
	if (thePlant->mSeedType == SeedType::SEED_SPLITPEA) {
		u::PlantState& s = thePlant->mPvzu;
		const bool aBack = theProjectile->mX < thePlant->mX; // hook 352: [proj+8] < [plant+8]
		if (!aBack && thePlantWeapon == static_cast<int>(PlantWeapon::WEAPON_PRIMARY)) {
			if (!u::SplitPeaSweeping(s) && u::SplitPeaSweepRollRange() > 0) { // hook 326: the front head fired while idle
				const int aRoll = Sexy::Rand(u::SplitPeaSweepRollRange());
				if (u::SplitPeaStartsSweep(aRoll)) {
					s.sweepTimer = 0;
					FogLogPlant(thePlant, std::format("splitPea.sweepStart roll {} angle {}", aRoll, s.sweepAngle));
				}
			}
			if (u::SplitPeaSweeping(s)) { // hook 354
				float aVelX = 0.0f, aVelY = 0.0f;
				u::SplitPeaSweepVelocity(s.sweepAngle, aVelX, aVelY);
				theProjectile->mMotionType = static_cast<ProjectileMotion>(u::SplitPeaSweepMotion());
				theProjectile->mDamageRangeFlags = u::SplitPeaSweepRangeFlags();
				theProjectile->mVelX = aVelX;
				theProjectile->mVelY = aVelY;
				if (s.sweepTimer % 60 == 3) // every 10th sweep shot
					FogLogPlant(thePlant, std::format("splitPea.sweepShot timer {} angle {} v ({:.3f}, {:.3f})", s.sweepTimer, s.sweepAngle, aVelX, aVelY));
			}
		}
		switch (u::SplitPeaShotKind(aBack)) {
		case u::SplitShot::Snow:
			theProjectile->mProjectileType = ProjectileType::PROJECTILE_SNOWPEA;
			break;
		case u::SplitShot::Fire:
			theProjectile->ConvertToFireball(-2); // not a Torchwood column (as the Threepeater fire pea)
			break;
		case u::SplitShot::Pea:
			break;
		}
		return;
	}
	if (thePlant->mSeedType == SeedType::SEED_CATTAIL && u::CattailCobRollRange() > 0) {
		const int aRoll = Sexy::Rand(u::CattailCobRollRange());
		if (u::CattailFiresCob(aRoll)) {
			theProjectile->mProjectileType = static_cast<ProjectileType>(u::CattailCobProjectile()); // keeps the homing motion
			FogLogPlant(thePlant, std::format("cattail.cob roll {} (< 3) homing Cob", aRoll));
		}
	}
}

// Starfruit -------------------------------------------------------------------------------------------

void FogStarfruitStar(Projectile* theStar) {
	if (!u::FogPlantsOn())
		return;
	const float aMult = u::StarfruitStarSpeedMult(); // hook 286: Starfruit stars have +0x80 == 0
	theStar->mVelX *= aMult;
	theStar->mVelY *= aMult;
	theStar->mPvzu.starCurve = u::StarfruitStarCurve(); // [+0x88] = 276: the boomerang tag
}

void FogMagnetPull(Plant* theMagnet, Zombie* theZombie) {
	if (!u::FogPlantsOn() || (!u::MagnetAllRows() && !u::MagnetUnlimitedRadius()))
		return;
	FogLogPlant(theMagnet, std::format("magnet.pull zombie type {} row {} (row diff {}) x {}", static_cast<int>(theZombie->mZombieType),
		theZombie->mRow, theZombie->mRow - theMagnet->mRow, theZombie->mX));
}

void FogStarfruitVolleyDone(Plant* theStarfruit) {
	if (!u::FogPlantsOn())
		return;
	std::string aExtra;
	if (ModesStarfruitNoExtraStars(theStarfruit->mApp)) // PVZU modesVanilla: I, Zombie Endless skips roll and stars (0x95b790)
		aExtra = ", I, Zombie Endless: no extra-star roll, no extra stars";
	else if (u::StarfruitExtraRollRange() > 0) {
		const int aRoll = Sexy::Rand(u::StarfruitExtraRollRange()); // hook 287: Rand(3) after the five stars
		if (u::StarfruitExtraStars(aRoll)) {
			Board* aBoard = theStarfruit->mBoard;
			const int aCount = u::StarfruitExtraStarCount();
			for (int i = 0; i < aCount; ++i) { // 0x95b0f0: x + 25, y + 25, render order + 1, row, type 7; motion 7, v = 0
				Projectile* aStar = aBoard->AddProjectile(theStarfruit->mX + 25, theStarfruit->mY + 25, theStarfruit->mRenderOrder + 1,
					theStarfruit->mRow, ProjectileType::PROJECTILE_STAR);
				aStar->mDamageRangeFlags = theStarfruit->GetDamageRangeFlags(PlantWeapon::WEAPON_PRIMARY);
				aStar->mMotionType = ProjectileMotion::MOTION_STAR;
				aStar->mVelX = 0.0f;
				aStar->mVelY = 0.0f;
				aStar->mPvzu.starCurve = u::StarfruitExtraStarCurve(i); // +0x88 = i
			}
			aExtra = std::format(", extra-star roll {} hit: {} spiral stars", aRoll, aCount);
		} else
			aExtra = std::format(", extra-star roll {}", aRoll);
	}
	FogLogPlant(theStarfruit, std::format("starfruit.volley speed x{} (back star vx {:.3f}){}", u::StarfruitStarSpeedMult(), -3.33f * u::StarfruitStarSpeedMult(), aExtra));
}

namespace {
Projectile* gCollisionShot = nullptr; // the exe's EBP inside Projectile::CheckForCollision

void LogStar(const Projectile* theStar, std::string_view theText) {
	u::LogPlantAbility(static_cast<int>(theStar->mBoard->mMainCounter),
		std::format("STAR row {} x {} y {} age {}: {}", theStar->mRow, theStar->mX, theStar->mY, theStar->mProjectileAge, theText));
}

bool PlainStar(const Projectile* theStar) { // a type-7 star that is not a Shrapnel shard / core or another mod shot
	return theStar->mProjectileType == ProjectileType::PROJECTILE_STAR && theStar->mPvzu.modShot == 0 && theStar->mPvzu.modType == 0;
}
}

void FogStarCurveTick(Projectile* theStar) {
	if (theStar->mPvzu.starCurve == u::kStarCurveNone || !PlainStar(theStar) || theStar->mMotionType != ProjectileMotion::MOTION_STAR ||
		theStar->mPvzu.field80 != 0 || !u::FogPlantsOn())
		return;
	if (SnakeOn(theStar->mBoard) || ModModeSkipsStarCurves(theStar->mApp)) // 0x95b700: 0x89, 0x8b, Adventure level 65
		return;
	const float aVx = theStar->mVelX, aVy = theStar->mVelY;
	if (!u::StarCurveStep(theStar->mPvzu.starCurve, theStar->mProjectileAge, theStar->mVelX, theStar->mVelY))
		return;
	if (theStar->mPvzu.starCurve == u::kStarCurveBoomerang)
		LogStar(theStar, std::format("starfruit.boomerang turns: v ({:.2f},{:.2f}) -> ({:.2f},{:.2f})", aVx, aVy, theStar->mVelX, theStar->mVelY));
	else if (theStar->mProjectileAge % 50 == 0)
		LogStar(theStar, std::format("starfruit.spiral tag {} at ({:.0f},{:.0f}) v ({:.2f},{:.2f})", theStar->mPvzu.starCurve - u::kStarCurveSpiral0,
			theStar->mPosX, theStar->mPosY, theStar->mVelX, theStar->mVelY));
}

StarCollisionScope::StarCollisionScope(Projectile* theProjectile) : mPrevious(gCollisionShot) { gCollisionShot = theProjectile; }
StarCollisionScope::~StarCollisionScope() { gCollisionShot = mPrevious; }

bool FogStarSurvivesDie(Projectile* theProjectile) {
	if (gCollisionShot != theProjectile || !PlainStar(theProjectile)) // ebp != esi: level end, high ground, clean-ups kill
		return false;
	const int aKind = theProjectile->mPvzu.field80;
	bool aLives = false;
	const int aFate = u::StarKindHitFate(ProjectileType::PROJECTILE_STAR, aKind, theProjectile->mPvzu.pierceHits); // 2..5
	if (aFate == u::kStarHitSurvives) {
		theProjectile->mPvzu.pierceHits++;
		aLives = true;
	} else if (aFate == u::kStarHitDies)
		aLives = false;
	else if (SnakeOn(theProjectile->mBoard) && SnakeShotSurvivesHit(theProjectile))
		aLives = true; // hook 401 sets the counter back to 1 every tick: a snake star never dies here
	else
		aLives = u::StarSurvivesDie(theProjectile->mPvzu, u::StarPierceLimit()); // kind <= 0, 1, > 5: 254 lives
	if (!aLives)
		return false;
	if (aKind > 0 && theProjectile->mMotionType == ProjectileMotion::MOTION_HOMING) { // 0xc00f10: back to free flight
		theProjectile->mMotionType = ProjectileMotion::MOTION_STAR;
		theProjectile->mTargetZombieID = ZombieID::ZOMBIEID_NULL;
	}
	const int aHits = theProjectile->mPvzu.pierceHits;
	if (aHits == 1 || aHits == 100 || aHits == 254)
		LogStar(theProjectile, std::format("star.pierce kind {} lived through Die {} (limit {})", aKind, aHits,
			aKind == 2 || aKind == 4 ? "kind" : std::to_string(u::StarPierceLimit())));
	return true;
}

// Blover ----------------------------------------------------------------------------------------------

bool FogBloverStorm(Plant* theBlover) {
	if (!u::BloverStorm())
		return false;
	Board* aBoard = theBlover->mBoard;
	int aHit = 0, aBlown = 0, aBalloons = 0;
	for (Zombie* aZombie : aBoard->mZombies) {
		if (aZombie->mDead || aZombie->IsDeadOrDying())
			continue;
		const u::BloverHit aEffect = u::BloverOnZombie(static_cast<int>(aZombie->mZombieType), aZombie->mMindControlled,
			aZombie->mZombiePhase == ZombiePhase::PHASE_BALLOON_FLYING);
		if (!aEffect.touched)
			continue;
		++aHit;
		bool aBlow = aEffect.blowAway;
		aZombie->TakeDamage(aEffect.damage, 1U << DamageFlags::DAMAGE_FREEZE);
		if (aEffect.blowIfArmless && !aZombie->mHasArm)
			aBlow = true;
		if (aBlow) {
			aZombie->mBlowingAway = true;
			++aBlown;
		}
		if (aEffect.balloonChillCs > 0) {
			aZombie->mChilledCounter = aEffect.balloonChillCs;
			++aBalloons;
		}
	}
	theBlover->mApp->PlaySample(SOUND_BLOVER);
	aBoard->mFogBlownCountDown = 4000; // vanilla (hook 342 only changes it in mod mode 0x7c)
	FogLogPlant(theBlover, std::format("blover.storm hit {} blownAway {} balloonsChilled {} (no balloon blown)", aHit, aBlown, aBalloons));
	return true;
}

// Squish, bite, spikes ----------------------------------------------------------------------------------

bool FogSquish(Zombie* theZombie, Plant* thePlant) {
	if (!u::FogPlantsOn())
		return false;
	const int aZombieType = static_cast<int>(theZombie->mZombieType);
	if (u::PumpkinExplodesWhenSmashed(static_cast<int>(ModSeedOf(thePlant)), aZombieType)) { // not a Holy-fire Pumpkin (0x6e)
		// cave 0x6519fd + hook 800: becomes a Cherry Bomb, HP -1, DoSpecial at once (it dies there), +0x54 = 1
		const int aHpBefore = theZombie->mBodyHealth;
		thePlant->mSeedType = static_cast<SeedType>(u::PumpkinExplodeSeed());
		thePlant->mPlantHealth = -1;
		thePlant->DoSpecial();
		thePlant->mStateCountdown = 1;
		FogLogPlant(thePlant, std::format("pumpkin.smashed by zombie type {}: exploded as a Cherry Bomb, smasher body {} -> {}{}, plant dead {}",
			aZombieType, aHpBefore, theZombie->mBodyHealth, theZombie->IsDeadOrDying() ? " (dying)" : "", thePlant->mDead ? 1 : 0));
		return true;
	}
	if (u::CactusBecomesSpikeweed(static_cast<int>(thePlant->mSeedType), aZombieType, thePlant->mSquished)) {
		// cave 0x6519c0: the Cactus is hidden, a Spikeweed is added on its square, then the vanilla squish
		thePlant->mVisible = false;
		Plant* aSpikeweed = thePlant->mBoard->AddPlant(thePlant->mPlantCol, thePlant->mRow, SeedType::SEED_SPIKEWEED, SeedType::SEED_NONE);
		FogLogPlant(thePlant, std::format("cactus.runOver by zombie type {}: Spikeweed {}", aZombieType, aSpikeweed ? "added" : "not added"));
	}
	return false;
}

bool FogPumpkinBitten(Zombie* theZombie, Plant* thePlant) {
	if (!u::FogPlantsOn() || !u::PumpkinExplodesWhenBitten(static_cast<int>(ModSeedOf(thePlant)), thePlant->mPlantHealth)) // not 0x6e
		return false;
	// cave 0x6513b6: seed = Cherry Bomb, mDoSpecialCountdown = 1, mStateCountdown = 1 -> it explodes on its next update.
	// Ruling: no bite this tick, so the Pumpkin can never be eaten to 0 (and removed) before it explodes.
	FogLogPlant(thePlant, std::format("pumpkin.bitten at hp {} by zombie type {}: explodes as a Cherry Bomb next tick",
		thePlant->mPlantHealth, static_cast<int>(theZombie->mZombieType)));
	thePlant->mSeedType = static_cast<SeedType>(u::PumpkinExplodeSeed());
	thePlant->mDoSpecialCountdown = 1;
	thePlant->mStateCountdown = 1;
	return true;
}

bool FogShotSurvivesHit(Projectile* theProjectile) {
	if (!u::FogPlantsOn())
		return false;
	// hook 806 (stars: FogStarSurvivesDie). A Cattail spike (homing) is left out: in the exe its [+0x88] is its target, so the cave above takes
	// its first hit and clears the target, and a homing shot without a target never hits again (study 2.9, ruling).
	if (theProjectile->mMotionType == ProjectileMotion::MOTION_HOMING ||
		!u::SpikeSurvivesHit(theProjectile->mPvzu, static_cast<int>(theProjectile->mProjectileType)))
		return false;
	const int aHits = theProjectile->mPvzu.pierceHits;
	if (aHits == 1 || aHits == 64)
		u::LogPlantAbility(static_cast<int>(theProjectile->mBoard->mMainCounter),
			std::format("SPIKE row {} x {}: spike.pierce hit {} (dies after 64)", theProjectile->mRow, theProjectile->mX, aHits));
	return true;
}

// Dev harness ------------------------------------------------------------------------------------------

namespace {
Plant* Put(Board* theBoard, int theCol, int theRow, SeedType theSeed) {
	if (theBoard->GetTopPlantAt(theCol, theRow, PlantPriority::TOPPLANT_ANY))
		return nullptr;
	return theBoard->AddPlant(theCol, theRow, theSeed, SeedType::SEED_NONE);
}

// A Pumpkin over a Wall-nut, so chewers bite the Pumpkin first.
void PutPumpkin(Board* theBoard, int theCol, int theRow, int theHp) {
	Put(theBoard, theCol, theRow, SeedType::SEED_WALLNUT);
	for (Plant* aPlant : theBoard->mPlants)
		if (!aPlant->mDead && aPlant->mSeedType == SeedType::SEED_PUMPKINSHELL && aPlant->mPlantCol == theCol && aPlant->mRow == theRow)
			return;
	Plant* aPumpkin = theBoard->AddPlant(theCol, theRow, SeedType::SEED_PUMPKINSHELL, SeedType::SEED_NONE);
	if (aPumpkin && theHp > 0)
		aPumpkin->mPlantHealth = theHp;
}

Zombie* ZombieAt(Board* theBoard, ZombieType theType, int theRow, float theX) {
	Zombie* aZombie = theBoard->AddZombieInRow(theType, theRow, 0);
	if (aZombie && theX > 0.0f)
		aZombie->mPosX = theX;
	return aZombie;
}

void LogFog(Board* theBoard) {
	int aFogged = 0, aCells = 0;
	for (int x = theBoard->LeftFogColumn(); x < MAX_GRID_SIZE_X; ++x)
		for (int y = 0; y < MAX_GRID_SIZE_Y; ++y) {
			++aCells;
			if (theBoard->mGridCelFog[x][y] > 0)
				++aFogged;
		}
	u::LogPlantAbility(static_cast<int>(theBoard->mMainCounter), std::format("PLANTERN dev: fog left column {}, fogged cells {} of {} (radius {})",
		theBoard->LeftFogColumn(), aFogged, aCells, u::PlanternFogRadius()));
}

void LayOutFog(Board* theBoard) {
	// row 0: Peashooter + Cactus in a Plantern's 3x3 (speed-up, heal; the Peashooter starts below half HP)
	if (Plant* aHurt = Put(theBoard, 0, 0, SeedType::SEED_PEASHOOTER))
		aHurt->mPlantHealth = 100;
	Put(theBoard, 1, 0, SeedType::SEED_PLANTERN);
	Put(theBoard, 2, 0, SeedType::SEED_CACTUS);
	// row 1: a lone Cactus for the Zomboni and the Catapult
	Put(theBoard, 5, 1, SeedType::SEED_CACTUS);
	// row 2 (water): Sea-shrooms for Ducky Tubes; row 3 (water): a Cattail on a Lily Pad
	Put(theBoard, 6, 2, SeedType::SEED_SEASHROOM);
	// row 4: a Split Pea
	Put(theBoard, 3, 4, SeedType::SEED_SPLITPEA);
	// row 5: Magnet-shroom (pail helmets five rows up), Starfruit, a weak Pumpkin for chewers
	Put(theBoard, 0, 5, SeedType::SEED_MAGNETSHROOM);
	Put(theBoard, 2, 5, SeedType::SEED_STARFRUIT);
	PutPumpkin(theBoard, 6, 5, 200);
	std::fprintf(stderr, "PVZU dev: fog plant-test layout placed\n");
}
bool gFogLaidOut = false;
}

bool FogPlantTestActive(Board* theBoard) {
	return CmdFlag("-pvzu-plant-test-fog") || (CmdFlag("-pvzu-plant-test") && theBoard->StageHasFog());
}

void FogPlantTestTick(Board* theBoard, int thePlayTicks) {
	if (!gFogLaidOut) {
		gFogLaidOut = true;
		LayOutFog(theBoard);
	}
	if (thePlayTicks == 600 || thePlayTicks == 3000)
		LogFog(theBoard);
	if (CmdFlag("-pvzu-dev-fog-trace") && thePlayTicks % 100 == 0) { // Sea-shroom / Gargantuar trace for this layout
		std::string aLine = std::format("tick {}:", thePlayTicks);
		for (Plant* aPlant : theBoard->mPlants)
			if (!aPlant->mDead && (aPlant->mSeedType == SeedType::SEED_SEASHROOM || aPlant->mSeedType == SeedType::SEED_PUMPKINSHELL))
				aLine += std::format(" {} hp {} asleep {}", GetPlantDefinition(aPlant->mSeedType).mPlantName, aPlant->mPlantHealth, aPlant->mIsAsleep ? 1 : 0);
		for (Zombie* aZombie : theBoard->mZombies)
			if (!aZombie->mDead && (aZombie->mZombieType == ZombieType::ZOMBIE_DUCKY_TUBE || aZombie->mZombieType == ZombieType::ZOMBIE_GARGANTUAR))
				aLine += std::format(" z{} row {} x {} hp {} eating {} phase {}", static_cast<int>(aZombie->mZombieType), aZombie->mRow, aZombie->mX,
					aZombie->mBodyHealth, aZombie->mIsEating ? 1 : 0, static_cast<int>(aZombie->mZombiePhase));
		std::fprintf(stderr, "PVZU dev: fog trace %s\n", aLine.c_str());
	}
	// the tough Sea-shroom eaters are taken off once past it, so they never reach the house (game over stops the test)
	for (Zombie* aZombie : theBoard->mZombies)
		if (!aZombie->mDead && aZombie->mZombieType == ZombieType::ZOMBIE_DUCKY_TUBE && aZombie->mRow == 2 && aZombie->mPosX < 380.0f)
			aZombie->DieNoLoot();
	// a bitten Pumpkin is a Cherry Bomb for one tick (no other Cherry Bomb in this layout): log when it is gone
	static int aLitSince = -1;
	bool aLit = false;
	for (Plant* aPlant : theBoard->mPlants)
		if (!aPlant->mDead && aPlant->mSeedType == SeedType::SEED_CHERRYBOMB)
			aLit = true;
	if (aLit && aLitSince < 0)
		aLitSince = thePlayTicks;
	else if (!aLit && aLitSince >= 0) {
		u::LogPlantAbility(static_cast<int>(theBoard->mMainCounter), std::format("PUMPKIN dev: the lit Pumpkin exploded after {} tick(s)",
			thePlayTicks - aLitSince));
		aLitSince = -1;
	}
	if (thePlayTicks % 450 == 0) // Plantern row, Split Pea row
	{
		theBoard->AddZombieInRow(ZombieType::ZOMBIE_PAIL, 0, 0);
		theBoard->AddZombieInRow(ZombieType::ZOMBIE_NORMAL, 4, 0);
	}
	if (thePlayTicks % 600 == 0) // Sea-shroom eaters (just right of it, so their bite rect meets it), Pumpkin chewers, Starfruit targets
	{
		if (Plant* aSea = theBoard->GetTopPlantAt(6, 2, PlantPriority::TOPPLANT_ANY))
			if (Zombie* aDucky = ZombieAt(theBoard, ZombieType::ZOMBIE_DUCKY_TUBE, 2, static_cast<float>(aSea->mX) + 90.0f))
				aDucky->mBodyHealth = aDucky->mBodyMaxHealth = 4000; // lives long enough to eat it (stray shots cross row 2)
		theBoard->AddZombieInRow(ZombieType::ZOMBIE_NORMAL, 5, 0);
	}
	if (thePlayTicks == 5000) { // a Cattail on a Lily Pad (late: its homing spikes would kill the Sea-shroom eaters)
		Put(theBoard, 0, 3, SeedType::SEED_LILYPAD);
		theBoard->AddPlant(0, 3, SeedType::SEED_CATTAIL, SeedType::SEED_NONE);
	}
	if (thePlayTicks == 700) // behind the Split Pea: its back head shoots snow peas
		ZombieAt(theBoard, ZombieType::ZOMBIE_NORMAL, 4, 140.0f);
	if (thePlayTicks == 1200) // the Zomboni reaches the lone Cactus quickly
		ZombieAt(theBoard, ZombieType::ZOMBIE_ZAMBONI, 1, 560.0f);
	if (thePlayTicks == 1900) { // Blover storm at 2000: an Imp, a balloon, a wounded walker
		ZombieAt(theBoard, ZombieType::ZOMBIE_IMP, 4, 700.0f);
		ZombieAt(theBoard, ZombieType::ZOMBIE_BALLOON, 3, 700.0f);
	}
	if (thePlayTicks == 2000 || thePlayTicks == 6000)
		if (Plant* aBlover = Put(theBoard, 8, 0, SeedType::SEED_BLOVER))
			FogLogPlant(aBlover, "dev.planted");
	if (thePlayTicks == 3500) { // a new lone Cactus, then a Catapult right on it
		Put(theBoard, 5, 1, SeedType::SEED_CACTUS);
		if (Plant* aCactus = theBoard->GetTopPlantAt(5, 1, PlantPriority::TOPPLANT_ANY))
			ZombieAt(theBoard, ZombieType::ZOMBIE_CATAPULT, 1, static_cast<float>(aCactus->mX) + 10.0f);
	}
	if (thePlayTicks == 4500) { // a full Pumpkin and a Gargantuar that smashes it
		PutPumpkin(theBoard, 6, 5, 0);
		ZombieAt(theBoard, ZombieType::ZOMBIE_GARGANTUAR, 5, 640.0f);
	}
	if (thePlayTicks % 1100 == 0) { // refills
		Put(theBoard, 6, 2, SeedType::SEED_SEASHROOM);
		PutPumpkin(theBoard, 6, 5, 200);
		Put(theBoard, 2, 0, SeedType::SEED_CACTUS);
		if (Plant* aHurt = Put(theBoard, 0, 0, SeedType::SEED_PEASHOOTER))
			aHurt->mPlantHealth = 100;
	}
}

} // namespace pvzu::engine
