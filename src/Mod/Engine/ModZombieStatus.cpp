// PVZU: zombie-side mod-plant statuses (Plan 8 part 3c); see ModZombieStatus.h.
#include "Mod/Engine/ModZombieStatus.h"

#include "Mod/Unnamed/DayPlants.h"
#include "Mod/Unnamed/ModPlantRules.h"
#include "Mod/Unnamed/ModPlantShadow.h"
#include "Mod/Unnamed/ModShots.h"

#include "Lawn/Board.h"
#include "Lawn/Zombie.h"
#include "graphics/Color.h"
#include "Common.h"

#include <format>

namespace pvzu::engine {

namespace u = pvzu::unnamed;

namespace {
constexpr int kNewspaperBossForm = 0x7f; // 0x7aa130 cmp [esi+0x134], 0x7f (only the 7-10 boss code sets it)

void Log(const Zombie* theZombie, const std::string& theText) {
	u::LogPlantAbility(theZombie->mBoard ? static_cast<int>(theZombie->mBoard->mMainCounter) : -1,
		std::format("MODZOMBIE type {} row {} x {}: {}", static_cast<int>(theZombie->mZombieType), theZombie->mRow, theZombie->mX,
			theText));
}

// 0xc10e10: the side record exists only for zombies of a wave, the cut-scene and the UI zombies have none.
bool HasSideRecord(const Zombie* theZombie) {
	return u::RepeaterPoisonRecordFor(theZombie->mFromWave);
}

std::string Hp(const Zombie* z) {
	return std::format("body {} helm {} shield {}", z->mBodyHealth, z->mHelmHealth, z->mShieldHealth);
}
} // namespace

void ModZombieApplyPoison(Zombie* theZombie, int theAddCs) {
	if (!theZombie || theAddCs <= 0 || !u::ModPlantsOn() || !HasSideRecord(theZombie))
		return;
	const int aBefore = theZombie->mPvzu.repeaterPoisonCs;
	theZombie->mPvzu.repeaterPoisonCs += theAddCs; // 0xc10c8b add [eax+8], 0x1f4
	Log(theZombie, std::format("repeaterPoison +{} cs: {} -> {}", theAddCs, aBefore, theZombie->mPvzu.repeaterPoisonCs));
}

void ModZombieAddWeakened(Zombie* theZombie, int theAddCs) {
	if (!theZombie || theAddCs == 0 || !u::ModPlantsOn())
		return;
	const int aBefore = theZombie->mPvzu.weakenCs;
	theZombie->mPvzu.weakenCs = u::ZombieWeakenAdd(aBefore, theAddCs);
	Log(theZombie, std::format("weakened +{} cs: {} -> {}", theAddCs, aBefore, theZombie->mPvzu.weakenCs));
}

int ModZombieWeakenedCs(const Zombie* theZombie) {
	return theZombie ? theZombie->mPvzu.weakenCs : 0;
}

bool ModZombieBossClass(const Zombie* theZombie) { // 0x7aa320 cmp [eax+0xcc], 0x1d4c0; jg / type 0x19 / 0x7aa130
	return u::ModPlantBossClass(static_cast<int>(theZombie->mZombieType), theZombie->mBodyMaxHealth,
		theZombie->mZombieType == ZombieType::ZOMBIE_NEWSPAPER && theZombie->mPvzu.variant == kNewspaperBossForm);
}

void ModZombieStatusTick(Zombie* theZombie) {
	u::ZombieState& s = theZombie->mPvzu;
	if ((s.repeaterPoisonCs <= 0 && s.toxicKind == 0) || !theZombie->mBoard || !u::ModPlantsOn())
		return; // 0xc10cd0: cmp [esi+4] (the board), 0
	if (theZombie->mDead || theZombie->IsDeadOrDying())
		return; // port: no more damage to a zombie that is already dying (the exe's TakeDamage returns there too)

	if (s.repeaterPoisonCs > 0 && HasSideRecord(theZombie)) { // 0xc10cdd..0xc10d0e
		const u::RepeaterPoisonStep aStep = u::RepeaterPoisonTick(s.repeaterPoisonCs);
		s.repeaterPoisonCs = aStep.counter;
		if (aStep.damage > 0) {
			theZombie->TakeDamage(aStep.damage, 0U);
			if (s.repeaterPoisonCs % 100 == 0) // one line per second of poison, and its end
				Log(theZombie, std::format("repeaterPoison tick {} left {} cs, {}", aStep.damage, s.repeaterPoisonCs, Hp(theZombie)));
		}
	}

	if (s.toxicKind != 0 && !theZombie->IsDeadOrDying()) { // 0x7a6740
		const u::ToxicPoisonStep aStep = u::ToxicPoisonTick(s.toxicKind, s.toxicTimer, ModZombieBossClass(theZombie));
		if (aStep.damage > 0) {
			if (aStep.bodyOnly) { // kind 2: mJustGotShotCounter = 25, TakeBodyDamage (helmet and shield skipped)
				if (aStep.flashCs > 0)
					theZombie->mJustGotShotCounter = aStep.flashCs;
				theZombie->TakeBodyDamage(aStep.damage, 0U);
			} else {
				theZombie->TakeDamage(aStep.damage, 0U);
			}
			Log(theZombie, std::format("toxicPoison kind {} {} {}, {}", s.toxicKind, aStep.damage, aStep.bodyOnly ? "body only" : "armour first",
				Hp(theZombie)));
		}
		s.toxicKind = aStep.kind;
		s.toxicTimer = aStep.timer;
	}
}

void ModZombieWeakenTick(Zombie* theZombie) {
	if (theZombie->mPvzu.weakenCs > 0) // 0x95a140 cmp word [esi+0x1a],0; jle; dec
	{
		theZombie->mPvzu.weakenCs = u::ZombieWeakenTick(theZombie->mPvzu.weakenCs);
		if (theZombie->mPvzu.weakenCs == 0)
			Log(theZombie, "weakened over");
	}
}

int ModZombieDamageTaken(Zombie* theZombie, int theDamage) {
	const int aWeak = theZombie->mPvzu.weakenCs;
	if (aWeak <= 0 || !u::ModPlantsOn())
		return theDamage;
	const int aRange = u::ZombieWeakenRollRange(aWeak, static_cast<int>(theZombie->mZombieType), theZombie->mPvzu.subVariant);
	const int aRoll = aRange > 0 ? Sexy::Rand(aRange) : 0; // 0xc05a92: only the football-head Newspaper rolls
	const int aDamage = u::ZombieWeakenedDamage(theDamage, aWeak, static_cast<int>(theZombie->mZombieType), theZombie->mPvzu.subVariant, aRoll);
	if (aDamage != theDamage)
		Log(theZombie, std::format("weakened hit {} -> {} ({} cs left)", theDamage, aDamage, aWeak));
	return aDamage;
}

void ModZombieTint(const Zombie* theZombie, Sexy::Color& theColor, Sexy::Color& theAdditive, bool& theEnableAdditive) {
	const u::ZombieState& s = theZombie->mPvzu;
	if ((s.weakenCs <= 0 && s.repeaterPoisonCs <= 0) || !u::ModPlantsOn())
		return;
	// The chain (hook 0x52d2bf) runs after the burned branch and the low-HP Zomboss flash (0x52d1dd: body < max / 10),
	// and before mind control and chill, so a status colour wins over those two.
	if (theZombie->mZombiePhase == ZombiePhase::PHASE_ZOMBIE_BURNED ||
		(theZombie->mZombieType == ZombieType::ZOMBIE_BOSS && theZombie->mZombiePhase != ZombiePhase::PHASE_ZOMBIE_DYING &&
			theZombie->mBodyHealth < theZombie->mBodyMaxHealth / 10))
		return;
	const u::ZombieTint aTint = u::ZombieStatusTint(HasSideRecord(theZombie), s.repeaterPoisonCs, s.weakenCs); // 0xc10d30, 0x95a365
	if (!aTint.on)
		return;
	theColor = Sexy::Color(static_cast<int>(aTint.rgb >> 16 & 0xff), static_cast<int>(aTint.rgb >> 8 & 0xff), static_cast<int>(aTint.rgb & 0xff),
		theColor.mAlpha);
	theAdditive = theColor; // 0x52d2d9: the same RGB and alpha as the additive colour, 0x52d382: additive on
	theEnableAdditive = true;
}

std::optional<bool> ModGargantuarStartsSmash(Zombie* theGargantuar) {
	if (!u::ModPlantsOn())
		return std::nullopt;
	if (theGargantuar->FindZombieTarget()) // 0x7a8262 call 0x52e840
		return true;
	if (theGargantuar->mMindControlled) // 0x7a826f cmp byte [ebx+0xb8], 0
		return false;
	return std::nullopt;
}

bool ModGargantuarSmashLands(Zombie* theGargantuar) {
	if (!u::ModPlantsOn())
		return false;
	if (Zombie* aTarget = theGargantuar->FindZombieTarget()) { // 0x7a82a2
		const int aDamage = theGargantuar->mZombieType == ZombieType::ZOMBIE_REDEYE_GARGANTUAR ? 1000 : 500; // 0x7a82bc type 0x20
		aTarget->TakeDamage(aDamage, 0U);
		Log(theGargantuar, std::format("gargantuar.smashZombie type {} {} ({})", static_cast<int>(aTarget->mZombieType), aDamage, Hp(aTarget)));
		return true;
	}
	return theGargantuar->mMindControlled; // 0x7a82d4: hypnotised, nothing to smash
}

} // namespace pvzu::engine
