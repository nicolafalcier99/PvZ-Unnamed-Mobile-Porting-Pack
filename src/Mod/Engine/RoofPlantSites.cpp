// PVZU: engine side of the Unnamed Roof plants. Rules: Mod/Unnamed/RoofPlants.h; evidence: the research notes (w5-roof)
// section 1 and pvz-unnamed-pack/SOURCES-roof-plants.md.
#include "Mod/Engine/RoofPlantSites.h"

#include "Mod/CmdLine.h"
#include "Mod/Engine/ZombieGapsV1.h" // PVZU Plan 9 V1: red-eye paper vs Garlic
#include "Mod/Engine/ModPlants.h" // Plan 8 part 3b: ModSeedOf (the Marigold bite rule compares the raw seed)
#include "Mod/Unnamed/ModPlantRules.h" // ModPlantDodgesZombieShots (hook 374)
#include "Mod/Unnamed/DayPlants.h"
#include "Mod/Unnamed/RoofPlants.h"

#include "LawnApp.h"
#include "Lawn/Board.h"
#include "Lawn/Plant.h"
#include "Lawn/Projectile.h"
#include "Lawn/Zombie.h"
#include "PvzpLib/PvzpParticle.h"
#include "Resources.h"

#include <cstdio>
#include <format>

namespace u = pvzu::unnamed;

namespace pvzu::engine {

namespace {
int Tick(const Board* theBoard) {
	return theBoard ? static_cast<int>(theBoard->mMainCounter) : -1;
}
void LogLine(const Board* theBoard, std::string_view theText) {
	u::LogPlantAbility(Tick(theBoard), theText);
}
}

void RoofLogPlant(const Plant* thePlant, std::string_view theText) {
	LogLine(thePlant->mBoard, std::format("{} col {} row {}: {}", GetPlantDefinition(thePlant->mSeedType).mPlantName, thePlant->mPlantCol,
		thePlant->mRow, theText));
}

// Cabbage-pult, Kernel-pult -----------------------------------------------------------------------------

void RoofShotFired(Plant* thePlant, Projectile* theProjectile) {
	if (!u::RoofPlantsOn() || !theProjectile)
		return;
	if (thePlant->mSeedType == SeedType::SEED_KERNELPULT) {
		const int aBefore = thePlant->mPvzu.counter;
		const u::KernelShot aShot = u::KernelPityShot(aBefore, static_cast<int>(theProjectile->mProjectileType));
		const bool aPity = aShot.projectileType != static_cast<int>(theProjectile->mProjectileType);
		theProjectile->mProjectileType = static_cast<ProjectileType>(aShot.projectileType); // after AddProjectile, as the exe
		thePlant->mPvzu.counter = aShot.counter;
		if (aShot.carry >= 0)
			theProjectile->mPvzu.butterStreak = aShot.carry;
		if (aPity)
			RoofLogPlant(thePlant, std::format("kernel.pityCob after {} kernels in a row (Cob, 1800 ash)", aBefore));
		else if (theProjectile->mProjectileType == ProjectileType::PROJECTILE_BUTTER)
			RoofLogPlant(thePlant, std::format("kernel.butter streak {} (kernels before it)", aShot.carry));
		return;
	}
	if (thePlant->mSeedType != SeedType::SEED_CABBAGEPULT || u::CabbageQuickRollRange() <= 0)
		return;
	const int aRoll = Sexy::Rand(u::CabbageQuickRollRange()); // 0xc0566c
	const u::CabbageQuick aQuick = u::CabbageQuickThrow(aRoll, thePlant->mBoard ? thePlant->mBoard->mLevel : 0);
	if (!aQuick.bigShot)
		return;
	const int aHp = thePlant->mPlantHealth;
	if (aQuick.plantCost) {
		thePlant->mBeghouledFlashCountdown = aQuick.flashCs;
		thePlant->mLaunchCounter = aQuick.launchCounter;
		thePlant->mPlantHealth -= aQuick.hpCost; // Plant::Update kills it once HP < 0
	}
	theProjectile->mPvzu.bigCabbage = true;
	theProjectile->mVelZ = aQuick.velZ;
	theProjectile->mAccZ = aQuick.accZ;
	RoofLogPlant(thePlant, std::format("cabbage.bigQuickThrow roll {}: launch counter {} hp {} -> {}{}", aRoll, thePlant->mLaunchCounter, aHp,
		thePlant->mPlantHealth, aQuick.plantCost ? "" : " (level 55: shot only)"));
}

// Melon-pult ------------------------------------------------------------------------------------------

void RoofMelonVolley(Plant* thePlant, Zombie* theTarget, int thePlantWeapon) {
	if (!theTarget || !u::RoofPlantsOn() || !thePlant->mBoard)
		return;
	const int aSeed = static_cast<int>(thePlant->mSeedType);
	const int aCount = static_cast<int>(thePlant->mBoard->mZombies.mSize); // Board+0xa0
	if (!u::MelonVolleyNeedsRoll(aSeed, aCount) && !u::MelonVolleyFires(aSeed, aCount, 0))
		return; // not a melon (or the rule is off): no roll
	const int aRoll = u::MelonVolleyNeedsRoll(aSeed, aCount) ? Sexy::Rand(100) : -1; // 0x79756f
	if (!u::MelonVolleyFires(aSeed, aCount, aRoll))
		return;
	int aShots = 0;
	for (Zombie* aZombie : thePlant->mBoard->mZombies) {
		if (aZombie == theTarget)
			continue;
		if (!u::MelonVolleyTarget(static_cast<int>(aZombie->mZombieType), static_cast<int>(aZombie->mZombiePhase), aZombie->mDead,
				aZombie->mMindControlled, aZombie->mRow == thePlant->mRow, aZombie->mX > thePlant->mX))
			continue;
		thePlant->Fire(aZombie, thePlant->mRow, static_cast<PlantWeapon>(thePlantWeapon));
		++aShots;
	}
	RoofLogPlant(thePlant, std::format("melon.volley roll {} with {} zombies on board: {} extra melon(s)", aRoll, aCount, aShots));
}

// Marigold --------------------------------------------------------------------------------------------

bool RoofMarigoldPulse(Plant* theMarigold) {
	if (theMarigold->mSeedType != SeedType::SEED_MARIGOLD || !u::MarigoldShrinks())
		return false;
	int aShrunk = 0;
	std::string aWho;
	for (Zombie* aZombie : theMarigold->mBoard->mZombies) {
		if (!u::MarigoldCanShrink(static_cast<int>(aZombie->mZombieType), aZombie->mPvzu.variant, aZombie->mDead, aZombie->mMindControlled,
				aZombie->mBodyMaxHealth) ||
			!u::MarigoldInRange(theMarigold->mRow, theMarigold->mX, theMarigold->mWidth, aZombie->mRow, aZombie->mX, aZombie->mZombieRect.mWidth))
			continue;
		u::ShrinkTarget t{aZombie->mScaleZombie, aZombie->mBodyHealth, aZombie->mBodyMaxHealth, aZombie->mHelmHealth, aZombie->mHelmMaxHealth,
			aZombie->mShieldHealth, aZombie->mShieldMaxHealth, aZombie->mFlyingHealth, aZombie->mFlyingMaxHealth, aZombie->mChilledCounter};
		const int aBody = t.body;
		u::MarigoldShrink(t);
		aZombie->mScaleZombie = t.scale;
		aZombie->mBodyHealth = t.body;
		aZombie->mBodyMaxHealth = t.bodyMax;
		aZombie->mHelmHealth = t.helm;
		aZombie->mHelmMaxHealth = t.helmMax;
		aZombie->mShieldHealth = t.shield;
		aZombie->mShieldMaxHealth = t.shieldMax;
		aZombie->mFlyingHealth = t.flying;
		aZombie->mChilledCounter = t.chill;
		++aShrunk;
		aWho += std::format(" [type {} row {} body {} -> {} scale {:.3f}]", static_cast<int>(aZombie->mZombieType), aZombie->mRow, aBody,
			aZombie->mBodyHealth, aZombie->mScaleZombie);
	}
	// 0x651908..0x65196a: particle 60 (gloom cloud) at the Marigold's centre, gold-dollar image, tint (150, 100, 200)
	if (PvzpParticleSystem* aParticle = theMarigold->mApp->AddPvzpParticle(theMarigold->mX + theMarigold->mWidth / 2.0f,
			theMarigold->mY + theMarigold->mHeight / 2.0f, theMarigold->mRenderOrder + 1, ParticleEffect::PARTICLE_GLOOMCLOUD)) {
		aParticle->OverrideImage(nullptr, IMAGE_REANIM_COIN_GOLD_DOLLAR);
		aParticle->OverrideColor(nullptr, Color(150, 100, 200, 255));
	}
	RoofLogPlant(theMarigold, std::format("marigold.shrinkPulse shrunk {}{}; the Marigold dies", aShrunk, aWho));
	theMarigold->Die(); // 0x651970
	return true;
}

// Garlic ----------------------------------------------------------------------------------------------

void RoofGarlicPoison(Zombie* theZombie) {
	Board* aBoard = theZombie->mBoard;
	if (!aBoard || !theZombie->IsOnBoard() || !u::GarlicPoisonTick(theZombie->mZombieAge))
		return;
	const bool aBoss = theZombie->mZombieType == ZombieType::ZOMBIE_BOSS;
	for (Plant* aPlant : aBoard->mPlants) {
		if (aPlant->mDead || aPlant->mSquished || aPlant->mSeedType != SeedType::SEED_GARLIC)
			continue;
		if (!u::GarlicPoisonReaches(aBoss, aPlant->mPlantCol, aPlant->mRow, aPlant->mX, theZombie->mRow, theZombie->mX))
			continue;
		if (!theZombie->EffectedByDamage(9U)) // ground + dog
			continue;
		const int aBody = theZombie->mBodyHealth, aHelm = theZombie->mHelmHealth, aShield = theZombie->mShieldHealth;
		if (!V1GarlicRipsPaper(theZombie)) // PVZU V1: a reading red-eye Newspaper loses its paper instead (0xc05ad0)
			theZombie->TakeDamage(u::GarlicPoisonDamage(), 1U << DamageFlags::DAMAGE_BYPASSES_SHIELD);
		if (theZombie->mZombieAge % 400 == 0) // one line per 10 poison ticks of this zombie
			RoofLogPlant(aPlant, std::format("garlic.poison zombie type {} row {} dx {}: body {} -> {} helm {} -> {} shield {} -> {}",
				static_cast<int>(theZombie->mZombieType), theZombie->mRow, theZombie->mX - aPlant->mX, aBody, theZombie->mBodyHealth, aHelm,
				theZombie->mHelmHealth, aShield, theZombie->mShieldHealth));
	}
}

void RoofGarlicBitten(Zombie* theZombie, Plant* thePlant) {
	if (thePlant->mSeedType != SeedType::SEED_GARLIC || !u::RoofPlantsOn())
		return;
	const int aChill = theZombie->mChilledCounter;
	theZombie->mChilledCounter = u::GarlicBiteChill(aChill); // the exe writes the counter only
	if (theZombie->mChilledCounter != aChill)
		RoofLogPlant(thePlant, std::format("garlic.biteChill zombie type {} chill {} -> {}", static_cast<int>(theZombie->mZombieType), aChill,
			theZombie->mChilledCounter));
	if (!u::GarlicBiteRolls(static_cast<int>(theZombie->mZombieType)))
		return;
	const int aRoll = Sexy::Rand(u::GarlicHypnoRollRange()); // 0x6513a2
	if (!u::GarlicBiteHypnotises(aRoll) || theZombie->mMindControlled)
		return;
	theZombie->StartMindControlled(); // the exe only sets +0xb8; the engine setter also detaches dancers / riders
	RoofLogPlant(thePlant, std::format("garlic.hypnotise zombie type {} roll {}", static_cast<int>(theZombie->mZombieType), aRoll));
}

bool RoofBiteImmune(Zombie* theZombie, Plant* thePlant) {
	if (thePlant->mIsAsleep || !u::MarigoldBiteImmune(static_cast<int>(ModSeedOf(thePlant)))) // seed 0x26 only, not Boss Flower
		return false;
	if (thePlant->mPvzu.extra == 0) { // one line per Marigold
		thePlant->mPvzu.extra = 1;
		RoofLogPlant(thePlant, std::format("marigold.biteImmune chewed by zombie type {}, hp stays {}", static_cast<int>(theZombie->mZombieType),
			thePlant->mPlantHealth));
	}
	return true;
}

// Butter, big cabbage, zombie shots, graves ------------------------------------------------------------------

namespace {
const Projectile* gButterShot = nullptr;
}

RoofButterShot::RoofButterShot(const Projectile* theShot) : mPrevious(gButterShot) {
	gButterShot = theShot;
}

RoofButterShot::~RoofButterShot() {
	gButterShot = mPrevious;
}

int RoofButterCs(Zombie* theZombie, int theVanilla) {
	if (!u::RoofPlantsOn())
		return theVanilla;
	const int aStreak = gButterShot ? gButterShot->mPvzu.butterStreak : 0;
	const int aCs = u::ButterCs(static_cast<int>(theZombie->mZombieType), static_cast<int>(theZombie->mZombiePhase), theZombie->mSummonCounter,
		static_cast<int>(theZombie->mApp->mGameMode), gButterShot != nullptr, aStreak, theZombie->mButteredCounter, theVanilla);
	if (gButterShot && theZombie->mBoard)
		LogLine(theZombie->mBoard, std::format("BUTTER row {}: kernel.butterTime zombie type {} streak {}: counter {} -> {}", theZombie->mRow,
			static_cast<int>(theZombie->mZombieType), aStreak, theZombie->mButteredCounter, aCs));
	return aCs;
}

int RoofShotDamage(Projectile* theProjectile, Zombie* theZombie, int theTableDamage) {
	const int aDamage = u::BigCabbageDamage(static_cast<int>(theProjectile->mProjectileType), theProjectile->mPvzu.bigCabbage, theTableDamage);
	if (aDamage != theTableDamage && theZombie)
		LogLine(theProjectile->mBoard, std::format("CABBAGE row {}: cabbage.bigHit {} damage (table {}) on zombie type {} body {}",
			theProjectile->mRow, aDamage, theTableDamage, static_cast<int>(theZombie->mZombieType), theZombie->mBodyHealth));
	return aDamage;
}

float RoofShotDrawScale(const Projectile* theProjectile, float theVanillaScale) {
	return u::BigCabbageDrawScale(static_cast<int>(theProjectile->mProjectileType), theProjectile->mPvzu.bigCabbage, theVanillaScale);
}

bool RoofZombieShotSkips(Projectile* theProjectile, Plant* thePlant) {
	// 0x46caf0: the exe's list applies to shots with [+0x74] (damage range flags) == 0: basketballs and zombie peas
	// 0xc02c30 compares the raw seed: a mod plant is skipped only as 0x58 / 0x59 / 0x6d, never as its base
	const bool aSkips = thePlant->mPvzu.modSeed != 0 ? u::ModPlantDodgesZombieShots(thePlant->mPvzu.modSeed)
	                                                 : u::ZombieShotSkipsPlant(static_cast<int>(thePlant->mSeedType), static_cast<int>(thePlant->mState));
	if (theProjectile->mDamageRangeFlags != 0 || !aSkips)
		return false;
	static const Projectile* sLogged = nullptr; // one line per shot, when it really is over the plant
	if (sLogged != theProjectile && GetRectOverlap(theProjectile->GetProjectileRect(), thePlant->GetPlantRect()) > 8) {
		sLogged = theProjectile;
		RoofLogPlant(thePlant, std::format("flowerPot.zombieShotPassesOver projectile type {} row {} x {} z {:.0f}{}",
			static_cast<int>(theProjectile->mProjectileType), theProjectile->mRow, theProjectile->mX, theProjectile->mPosZ,
			thePlant->mBoard->GetTopPlantAt(thePlant->mPlantCol, thePlant->mRow, PlantPriority::TOPPLANT_ANY) == thePlant ? " (empty pot)" : ""));
	}
	return true;
}

bool RoofGraveBlockedByPot(Board* theBoard, int theGridX, int theGridY) {
	return u::NoGraveOnFlowerPot() && theBoard->GetFlowerPotAt(theGridX, theGridY) != nullptr;
}


// Dev harness -------------------------------------------------------------------------------------------

namespace {
// A plant on the roof: a Flower Pot first where there is none.
Plant* Put(Board* theBoard, int theCol, int theRow, SeedType theSeed) {
	if (theSeed != SeedType::SEED_FLOWERPOT && !theBoard->GetFlowerPotAt(theCol, theRow))
		theBoard->AddPlant(theCol, theRow, SeedType::SEED_FLOWERPOT, SeedType::SEED_NONE);
	Plant* aTop = theBoard->GetTopPlantAt(theCol, theRow, PlantPriority::TOPPLANT_ANY);
	if (aTop && (theSeed == SeedType::SEED_FLOWERPOT || aTop->mSeedType != SeedType::SEED_FLOWERPOT))
		return nullptr;
	return theBoard->AddPlant(theCol, theRow, theSeed, SeedType::SEED_NONE);
}

Zombie* ZombieAt(Board* theBoard, ZombieType theType, int theRow, float theX) {
	Zombie* aZombie = theBoard->AddZombieInRow(theType, theRow, 0);
	if (aZombie && theX > 0.0f)
		aZombie->mPosX = theX;
	return aZombie;
}

void LayOutRoof(Board* theBoard) {
	// row 0: Cabbage-pult, two Kernel-pults (the second starts 13 kernels into a streak so the pity Cob shows soon)
	Put(theBoard, 0, 0, SeedType::SEED_CABBAGEPULT);
	Put(theBoard, 1, 0, SeedType::SEED_KERNELPULT);
	if (Plant* aKernel = Put(theBoard, 2, 0, SeedType::SEED_KERNELPULT))
		aKernel->mPvzu.counter = 13;
	// row 1: a Garlic; row 2: a Marigold (its pulse reaches rows 1..3)
	Put(theBoard, 5, 1, SeedType::SEED_GARLIC);
	Put(theBoard, 4, 2, SeedType::SEED_MARIGOLD);
	// row 3: only an empty Flower Pot, the Catapult's target (its balls pass over it)
	Put(theBoard, 6, 3, SeedType::SEED_FLOWERPOT);
	// row 4: Melon-pult and Winter Melon (their splash and chill stay out of the Garlic row)
	Put(theBoard, 0, 4, SeedType::SEED_MELONPULT);
	Put(theBoard, 1, 4, SeedType::SEED_WINTERMELON);
	std::fprintf(stderr, "PVZU dev: roof plant-test layout placed\n");
}
bool gRoofLaidOut = false;
}

bool RoofPlantTestActive(Board* theBoard) {
	return CmdFlag("-pvzu-plant-test-roof") || (CmdFlag("-pvzu-plant-test") && theBoard->StageHasRoof());
}

void RoofPlantTestTick(Board* theBoard, int thePlayTicks) {
	if (!gRoofLaidOut) {
		gRoofLaidOut = true;
		LayOutRoof(theBoard);
	}
	if (thePlayTicks == 600) { // hook 18: no grave on a Flower Pot (the roof never asks, so ask once here)
		const bool aPotGrave = theBoard->CanAddGraveStoneAt(6, 3);
		const bool aBareGrave = theBoard->CanAddGraveStoneAt(7, 3);
		u::LogPlantAbility(Tick(theBoard), std::format("FLOWERPOT dev: CanAddGraveStoneAt(6, 3) on a pot = {}, (7, 3) bare roof = {}",
			aPotGrave ? 1 : 0, aBareGrave ? 1 : 0));
		std::fprintf(stderr, "PVZU dev: roof plant-test board tick %d = app update %d\n", Tick(theBoard), theBoard->mApp->mUpdateCount);
	}
	if (CmdFlag("-pvzu-dev-roof-trace") && thePlayTicks % 50 == 0) { // Garlic row trace for this layout
		std::string aLine = std::format("tick {}:", thePlayTicks);
		for (Plant* aPlant : theBoard->mPlants)
			if (!aPlant->mDead && aPlant->mSeedType == SeedType::SEED_GARLIC)
				aLine += std::format(" garlic hp {}", aPlant->mPlantHealth);
		for (Zombie* aZombie : theBoard->mZombies)
			if (!aZombie->mDead && aZombie->mRow == 1)
				aLine += std::format(" z{} x {} hp {} eating {} yucky {} chill {} hyp {}", static_cast<int>(aZombie->mZombieType), aZombie->mX,
					aZombie->mBodyHealth, aZombie->mIsEating ? 1 : 0, aZombie->mYuckyFace ? 1 : 0, aZombie->mChilledCounter, aZombie->mMindControlled ? 1 : 0);
		std::fprintf(stderr, "PVZU dev: roof trace %s\n", aLine.c_str());
	}
	// zombies that got past everything are taken off before the house (a lost game stops the test)
	for (Zombie* aZombie : theBoard->mZombies)
		if (!aZombie->mDead && !aZombie->mMindControlled && aZombie->mPosX < 40.0f && aZombie->mZombieType != ZombieType::ZOMBIE_BOSS)
			aZombie->DieNoLoot();
	if (thePlayTicks % 450 == 0) { // targets for the pults
		theBoard->AddZombieInRow(ZombieType::ZOMBIE_TRAFFIC_CONE, 0, 0);
		theBoard->AddZombieInRow(ZombieType::ZOMBIE_NORMAL, 0, 0);
	}
	if (thePlayTicks % 700 == 0) // a group in the melon row
		for (float aX : {600.0f, 680.0f, 760.0f})
			ZombieAt(theBoard, ZombieType::ZOMBIE_NORMAL, 4, aX);
	if (thePlayTicks % 250 == 0) // Garlic eaters, close by
		ZombieAt(theBoard, thePlayTicks % 500 == 0 ? ZombieType::ZOMBIE_TRAFFIC_CONE : ZombieType::ZOMBIE_NORMAL, 1, 520.0f);
	if (thePlayTicks % 900 == 0) // Marigold area
		ZombieAt(theBoard, ZombieType::ZOMBIE_PAIL, 2, 600.0f);
	if (thePlayTicks == 1500) // a reading Newspaper for the 500 cs butter
		ZombieAt(theBoard, ZombieType::ZOMBIE_NEWSPAPER, 0, 700.0f);
	if (thePlayTicks == 2000 || thePlayTicks == 6000) // a loaded Catapult (its balls pass over the empty pot)
		ZombieAt(theBoard, ZombieType::ZOMBIE_CATAPULT, 3, 760.0f);
	if (thePlayTicks % 1500 == 0) { // refills
		Put(theBoard, 5, 1, SeedType::SEED_GARLIC);
		Put(theBoard, 6, 3, SeedType::SEED_FLOWERPOT);
		Put(theBoard, 0, 0, SeedType::SEED_CABBAGEPULT);
	}
	if (thePlayTicks % 3000 == 0)
		Put(theBoard, 4, 2, SeedType::SEED_MARIGOLD);
}

} // namespace pvzu::engine
