// PVZU: Plan 9 R9 small rules, engine side. See Mod/Engine/SmallRules.h.
#include "Mod/Engine/SmallRules.h"

#include "Mod/CmdLine.h"
#include "Mod/Engine/ModZombieStatus.h"
#include "Mod/Unnamed/DayPlants.h"
#include "Mod/Unnamed/ModPlantRules.h"
#include "Mod/Unnamed/SmallRules.h"
#include "Mod/Unnamed/State.h"

#include "LawnApp.h"
#include "Lawn/Board.h"
#include "Lawn/Plant.h"
#include "Lawn/Projectile.h"
#include "Lawn/Zombie.h"

#include <algorithm>
#include <format>
#include <string>

namespace u = pvzu::unnamed;

namespace pvzu::engine {

namespace {
// The exe's global star counter 0xc10033: process-wide, starts at 0, never saved (a new run starts at 0 again).
int gPityStars = -1;

int& PityStars() {
	if (gPityStars < 0)
		gPityStars = std::max(0, CmdOptionInt("-pvzu-dev-pity-counter", 0));
	return gPityStars;
}

int Tick(const Board* theBoard) {
	return theBoard ? static_cast<int>(theBoard->mMainCounter) : -1;
}

void CountStar(Projectile* theShot) { // 0xc10000: add [0xc10033], 1 (type 7, not I, Zombie)
	if (u::PityCountsStar(static_cast<int>(theShot->mProjectileType), theShot->mApp->IsIZombieLevel()))
		PityStars()++;
}
}

int SmallRulesPityCounter() {
	return PityStars();
}

int SmallRulesShotHit(Projectile* theShot, Zombie* theZombie, int theDamage) {
	if (theShot->mProjectileType != ProjectileType::PROJECTILE_STAR || theShot->mPvzu.modType != 0 || theShot->mPvzu.field80 != 0)
		return theDamage; // kind stars and mod shots: ModShotImpact (Mod/Engine/ModPlantsShooters.cpp)
	// 0xc10025: a star's table damage x2 outside I, Zombie (key modPlants.projectileDamage.starDouble)
	const int aDamage = u::ModProjectileDamage(static_cast<int>(theShot->mProjectileType), theDamage, 0, theShot->mApp->IsIZombieLevel());
	static bool sLoggedDamage = false;
	if (!sLoggedDamage && aDamage != theDamage) {
		sLoggedDamage = true;
		u::LogPlantAbility(Tick(theShot->mBoard), std::format("STAR row {}: plain star damage {} (table {}, x2 at 0xc10025)", theShot->mRow,
			aDamage, theDamage));
	}
	CountStar(theShot);
	const bool aCore = theShot->mPvzu.modShot == u::kModShotShrapnelCore; // exe +0x84 == 2 (0x79ef88)
	if (!theZombie || !u::PityCandidate(static_cast<int>(theShot->mProjectileType), 0, aCore, ModZombieBossClass(theZombie)))
		return aDamage;
	int& aCounter = PityStars();
	const int aRoll = u::PityNeedsRoll(aCounter) ? Sexy::Rand(u::PityRollRange()) : -1; // 0xc10083
	if (!u::PityHypnotises(aCounter, aRoll))
		return aDamage;
	const int aBefore = aCounter;
	aCounter = 0; // 0xc10091
	const bool aWas = theZombie->mMindControlled;
	if (!aWas)
		theZombie->StartMindControlled(); // the exe only sets +0xb8; the engine setter also detaches dancers (Garlic does the same)
	u::LogPlantAbility(Tick(theShot->mBoard), std::format("STAR row {}: pity.hypnotise zombie type {} x {} counter {} roll {}{}", theShot->mRow,
		static_cast<int>(theZombie->mZombieType), theZombie->mX, aBefore, aRoll, aWas ? " (already hypnotised)" : ""));
	return aDamage;
}

void SmallRulesCountKindStar(Projectile* theShot) {
	if (theShot->mProjectileType == ProjectileType::PROJECTILE_STAR && theShot->mPvzu.modType == 0)
		CountStar(theShot);
}

// ---- dev scene -------------------------------------------------------------------------------------------------
void SmallRulesDevTick(Board* theBoard, int thePlayTicks) {
	static const int aAt = CmdOptionInt("-pvzu-dev-small", -1);
	if (aAt < 0 || thePlayTicks < aAt)
		return;
	const int t = thePlayTicks - aAt;
	theBoard->mSunMoney = std::max(theBoard->mSunMoney, 2000);
	auto Walker = [&](ZombieType theType, int theRow, float theX) {
		if (Zombie* z = theBoard->AddZombieInRow(theType, theRow, 0)) {
			z->mPosX = theX;
			z->mX = static_cast<int>(theX);
		}
	};
	if (t == 0) {
		// rows 0 / 4: Starfruits (their diagonal stars cross rows 1 and 3); walkers in rows 0, 1, 3, 4 every 300 ticks
		for (int aCol = 0; aCol < 3; aCol++) {
			theBoard->AddPlant(aCol, 0, SeedType::SEED_STARFRUIT, SeedType::SEED_NONE);
			theBoard->AddPlant(aCol, 4, SeedType::SEED_STARFRUIT, SeedType::SEED_NONE);
		}
		// row 2: a vanilla Coffee Bean on a hurt sleeping Sun-shroom (a day lawn: mushrooms sleep)
		if (Plant* p = theBoard->AddPlant(3, 2, SeedType::SEED_SUNSHROOM, SeedType::SEED_NONE)) {
			p->mPlantHealth = 100;
			u::LogPlantAbility(Tick(theBoard), std::format("small dev: Sun-shroom col 3 row 2 hp {} / {} asleep {}", p->mPlantHealth,
				p->mPlantMaxHealth, p->mIsAsleep ? 1 : 0));
		}
		theBoard->AddPlant(3, 2, SeedType::SEED_INSTANT_COFFEE, SeedType::SEED_NONE);
		u::LogPlantAbility(Tick(theBoard), std::format("small dev: scene placed, pity counter {}", PityStars()));
	}
	if (t % 300 == 0)
		for (int aRow : {0, 1, 3, 4})
			Walker(ZombieType::ZOMBIE_NORMAL, aRow, 700.0f);
	if (t == 400)
		for (Plant* p : theBoard->mPlants)
			if (!p->mDead && p->mPlantCol == 3 && p->mRow == 2)
				u::LogPlantAbility(Tick(theBoard), std::format("small dev: col 3 row 2 {} hp {} / {} asleep {}",
					GetPlantDefinition(p->mSeedType).mPlantName, p->mPlantHealth, p->mPlantMaxHealth, p->mIsAsleep ? 1 : 0));
	if (t > 0 && t % 500 == 0) {
		int aHypno = 0;
		for (Zombie* z : theBoard->mZombies)
			aHypno += !z->mDead && z->mMindControlled ? 1 : 0;
		std::string aWhere;
		for (Zombie* z : theBoard->mZombies)
			if (!z->mDead)
				aWhere += std::format(" r{}x{}{}", z->mRow, z->mX, z->mMindControlled ? "H" : "");
		u::LogPlantAbility(Tick(theBoard), std::format("small dev: t {} pity counter {} hypnotised zombies on the lawn {}:{}", t, PityStars(),
			aHypno, aWhere));
	}
}

} // namespace pvzu::engine
