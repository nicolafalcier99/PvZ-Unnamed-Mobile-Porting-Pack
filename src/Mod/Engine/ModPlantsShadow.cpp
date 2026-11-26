// PVZU: Unnamed's shadow state and the plants built on it (Plan 8 part 3c); see ModPlantsShadow.h.
#include "Mod/Engine/ModPlantsShadow.h"

#include "Mod/CmdLine.h"
#include "Mod/Engine/ModPlantSites.h"
#include "Mod/Engine/ModPlants.h"
#include "Mod/Engine/ModZombieStatus.h"
#include "Mod/Unnamed/DayPlants.h"
#include "Mod/Unnamed/ModPlantRules.h"
#include "Mod/Unnamed/ModPlantShadow.h"
#include "Mod/Unnamed/ModPlants.h"

#include "LawnApp.h"
#include "Lawn/Board.h"
#include "Lawn/Challenge.h"
#include "Lawn/CursorObject.h"
#include "Lawn/GridItem.h"
#include "Lawn/Plant.h"
#include "Lawn/Zombie.h"
#include "PvzpLib/Reanimator.h"
#include "Lawn/System/PlayerInfo.h"
#include "Common.h"
#include "PvzpLib/PvzpCommon.h"
#include "graphics/Color.h"

#include <algorithm>
#include <format>
#include <string>
#include <vector>

namespace pvzu::engine {

namespace u = pvzu::unnamed;

namespace {
constexpr int kMoonflower = u::kModSeedMoonflower, kToxic = u::kModSeedToxicShadowShroom,
	kCharmed = u::kModSeedCharmedTallnut;

std::string Hex(int theSeed) { return std::format("0x{:02x}", theSeed); }

void Log(const Plant* thePlant, const std::string& theText) {
	u::LogPlantAbility(thePlant->mBoard ? static_cast<int>(thePlant->mBoard->mMainCounter) : -1,
		std::format("MODPLANT {} col {} row {}: {}", Hex(ModSeedOf(thePlant)), thePlant->mPlantCol, thePlant->mRow, theText));
}

void LogBoard(const Board* theBoard, const std::string& theText) {
	u::LogPlantAbility(theBoard ? static_cast<int>(theBoard->mMainCounter) : -1, "MODPLANT " + theText);
}

std::string ZombieHp(const Zombie* z) {
	return std::format("body {}/{} helm {} shield {}", z->mBodyHealth, z->mBodyMaxHealth, z->mHelmHealth, z->mShieldHealth);
}

// A hypnotised zombie rising at the plant's square (0x950108..0x95014e, 0xc00fb8..0xc00ff0): AddZombieInRow with the
// current wave, raw mMindControlled = 1 (no StartMindControlled), RiseFromGrave, phase counter 50, one rise update.
Zombie* SummonCharmed(Plant* thePlant, ZombieType theType) {
	Board* aBoard = thePlant->mBoard;
	Zombie* aZombie = aBoard->AddZombieInRow(theType, thePlant->mRow, aBoard->mCurrentWave);
	if (aZombie == nullptr)
		return nullptr;
	aZombie->mMindControlled = true;
	return aZombie;
}

void RiseAtPlant(Zombie* theZombie, Plant* thePlant) {
	theZombie->RiseFromGrave(thePlant->mPlantCol, thePlant->mRow);
	theZombie->mPhaseCounter = u::CharmedTallnutRiseCounterCs();
	theZombie->UpdateZombieRiseFromGrave();
}

void CharmedEnterShadow(Plant* thePlant) {
	const u::CharmedTallnutGarg aGarg = u::CharmedTallnutEnterShadow(thePlant->mPlantHealth, thePlant->mPlantMaxHealth);
	if (!aGarg.summon) {
		Log(thePlant, std::format("charmedTallnut.enterShadow hp {}/{}: no Gargantuar (under 75%)", thePlant->mPlantHealth,
			thePlant->mPlantMaxHealth));
		return;
	}
	Zombie* aGargantuar = SummonCharmed(thePlant, ZombieType::ZOMBIE_GARGANTUAR);
	if (aGargantuar) {
		aGargantuar->mBodyHealth = aGarg.gargBodyHp; // 0xc00fc8: body and max = plant HP / 2
		aGargantuar->mBodyMaxHealth = aGarg.gargBodyHp;
		RiseAtPlant(aGargantuar, thePlant);
	}
	const int aBefore = thePlant->mPlantHealth;
	thePlant->mPlantHealth = aGarg.plantHpAfter; // 0xc00ff6: - 0.4 x max
	Log(thePlant, std::format("charmedTallnut.enterShadow hypnotised Gargantuar body {}, plant hp {} -> {}", aGarg.gargBodyHp, aBefore,
		thePlant->mPlantHealth));
}

void MoonflowerSun(Plant* thePlant) {
	LawnApp* aApp = thePlant->mApp;
	Board* aBoard = thePlant->mBoard;
	// 0x7b0200: the vanilla UpdateProductionPlant gates, plus the mod's mode list
	if (!u::ModSunRunsInMode(static_cast<int>(aApp->mGameMode)) || !thePlant->IsInPlay() || aApp->IsIZombieLevel() ||
		aBoard->HasLevelAwardDropped())
		return;
	if (aApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_LAST_STAND &&
		aBoard->mChallenge->mChallengeState != ChallengeState::STATECHALLENGE_LAST_STAND_ONSLAUGHT)
		return;
	thePlant->mLaunchCounter--;
	if (thePlant->mLaunchCounter <= 100) // the vanilla glow before a drop
		thePlant->mEatenFlashCountdown = std::max(thePlant->mEatenFlashCountdown,
			PvzpAnimateCurve(100, 0, thePlant->mLaunchCounter, 0, 100, PvzpCurves::CURVE_LINEAR));
	if (thePlant->mLaunchCounter > 0)
		return;
	const int aRange = u::ModSunPlantTimerRollRange();
	thePlant->mLaunchCounter = u::ModSunPlantNextTimer(thePlant->mLaunchRate, aRange > 0 ? Sexy::Rand(aRange) : 0); // 0x7b02af
	aApp->PlayFoley(FoleyType::FOLEY_SPAWN_SUN);
	const bool aShadow = ModPlantShadowed(thePlant);
	const u::ModSunDrops aDrops = u::MoonflowerProduce(aShadow); // 0x7b0305: Small sun, or a Sun in shadow; motion COIN
	if (aDrops.first.coinType > 0)
		aBoard->AddCoin(thePlant->mX, thePlant->mY, static_cast<CoinType>(aDrops.first.coinType), static_cast<CoinMotion>(aDrops.first.motion));
	Log(thePlant, std::format("moonflower.sun coin {} ({}), next in {} cs", aDrops.first.coinType, aShadow ? "shadow" : "plain",
		thePlant->mLaunchCounter));
}
} // namespace

bool ModPlantShadowed(const Plant* thePlant) {
	return thePlant && thePlant->mPvzu.modSeed != 0 && thePlant->mPvzu.shadow != 0 && u::IsShadowPlant(thePlant->mPvzu.modSeed);
}

void ModPlantShadowTick(Plant* thePlant) {
	const int aSeed = thePlant->mPvzu.modSeed;
	Board* aBoard = thePlant->mBoard;
	if (aBoard == nullptr || !u::IsShadowPlant(aSeed))
		return;
	// 0x7b06d7..0x7b075b: points from every plant on the board, then the threshold
	static std::vector<u::ShadowLawnPlant> sLawn;
	sLawn.clear();
	for (Plant* aPlant : aBoard->mPlants) {
		if (aPlant->mDead)
			continue;
		u::ShadowLawnPlant q;
		q.seed = static_cast<int>(ModSeedOf(aPlant));
		q.row = aPlant->mRow;
		q.col = aPlant->mPlantCol;
		q.shadow = aPlant->mPvzu.shadow != 0;
		q.squished = aPlant->mSquished;
		q.self = aPlant == thePlant;
		sLawn.push_back(q);
	}
	const int aPoints = u::ShadowPointsOnLawn(aSeed, thePlant->mRow, thePlant->mPlantCol, sLawn);
	const u::ShadowChange aStep = u::ShadowStateStep(aSeed, thePlant->mPvzu.shadow != 0, static_cast<int>(thePlant->mApp->mGameMode), aPoints);
	thePlant->mPvzu.shadow = aStep.shadow ? 1 : 0;
	if (aStep.entered || aStep.left)
		Log(thePlant, std::format("shadow {} (points {}, threshold {})", aStep.entered ? "on" : "off", aPoints, u::ShadowThreshold(aSeed)));
	if (aStep.entered && aSeed == kCharmed) // 0x7b0766 -> 0xc00f80
		CharmedEnterShadow(thePlant);
	if (aSeed == kMoonflower && u::ModSunPlantProduces(aSeed)) // 0x7a5414 -> 0x7b0200
		MoonflowerSun(thePlant);
}

void ModPlantShadowOnPlanted(Plant* thePlant) {
	const int aSeed = thePlant->mPvzu.modSeed;
	if (aSeed != kMoonflower)
		return;
	const int aRange = u::ModSunFirstTimerRollRange(aSeed, thePlant->mLaunchRate);
	if (aRange <= 0)
		return;
	thePlant->mLaunchCounter = Sexy::Rand(aRange); // 0x7a4770: Rand(rate + 1)
	Log(thePlant, std::format("moonflower.firstSun in {} cs", thePlant->mLaunchCounter));
}

void ModPlantShadowOnDie(Plant* thePlant) {
	if (u::IsShadowPlant(thePlant->mPvzu.modSeed))
		Log(thePlant, std::format("dies hp {}/{} shadow {} squished {}", thePlant->mPlantHealth, thePlant->mPlantMaxHealth,
			thePlant->mPvzu.shadow, thePlant->mSquished ? 1 : 0));
}

bool ModPlantShadowColor(const Plant* thePlant, Sexy::Color& theColor) {
	if (!ModPlantShadowed(thePlant))
		return false;
	const u::ShadowTint aTint = u::ShadowPlantTint(thePlant->mPvzu.modSeed, true);
	if (!aTint.apply)
		return false;
	// Dream Starfruit: only 1 draw in 6 (exe Rand(6) in Draw). Port: picked from the board clock and the plant, so the
	// game's random numbers (and replays) do not depend on how often the screen is drawn.
	if (const int aFlicker = u::ShadowTintFlickerRange(thePlant->mPvzu.modSeed); aFlicker > 0) {
		const uint32_t aHash = (static_cast<uint32_t>(thePlant->mBoard ? thePlant->mBoard->mMainCounter : 0) * 2654435761u) ^
			(static_cast<uint32_t>(thePlant->mPlantCol * 7 + thePlant->mRow) * 40503u);
		if ((aHash >> 16) % static_cast<uint32_t>(aFlicker) != 0)
			return false;
	}
	theColor = Sexy::Color(aTint.r, aTint.g, aTint.b, aTint.a);
	return true;
}

int ModPlantBitten(Zombie* theEater, Plant* thePlant, int theDamage) {
	if (thePlant->mPvzu.modSeed != kCharmed || !u::ModPlantsOn())
		return theDamage;
	const bool aShadow = ModPlantShadowed(thePlant);
	const int aDamage = u::CharmedTallnutBiteDamage(theDamage, aShadow); // 0xc01040: x1.5 before the counter
	const u::CharmedTallnutBite aBite = u::CharmedTallnutOnBite(thePlant->mPvzu.charmedBitten, aDamage, thePlant->mPlantHealth, true);
	thePlant->mPvzu.charmedBitten = aBite.counter;
	if (aBite.summon) {
		const int aRange = u::CharmedTallnutSummonRollRange();
		const int aType = u::CharmedTallnutSummonType(aRange > 0 ? Sexy::Rand(aRange) : -1); // 0x9500f0 Rand(3)
		if (aType >= 0) {
			if (Zombie* aZombie = SummonCharmed(thePlant, static_cast<ZombieType>(aType))) {
				aZombie->mBodyHealth = u::CharmedTallnutSummonBodyHp(aZombie->mBodyHealth); // 0x950121: x1.35, max unchanged
				RiseAtPlant(aZombie, thePlant);
				Log(thePlant, std::format("charmedTallnut.summon type {} {} (bitten left {}, plant hp {}, eater type {})", aType,
					ZombieHp(aZombie), thePlant->mPvzu.charmedBitten, thePlant->mPlantHealth - aDamage,
					theEater ? static_cast<int>(theEater->mZombieType) : -1)); // PVZU V2: null eater = 8-2's drain
			}
		}
	}
	return aDamage;
}

void ShadowPlantEatenToDeath(Zombie* theEater, Plant* thePlant) {
	if (thePlant->mPvzu.modSeed != kToxic)
		return;
	const bool aShadow = ModPlantShadowed(thePlant);
	const u::ToxicPoisonSet aSet = u::ToxicShadowShroomEaten(kToxic, aShadow, theEater->mPvzu.toxicKind); // 0x7a65b0
	if (!aSet.apply)
		return;
	theEater->mPvzu.toxicKind = aSet.kind;
	theEater->mPvzu.toxicTimer = aSet.timer;
	Log(thePlant, std::format("toxicShadowShroom.eaten: zombie type {} poisoned kind {} ({}) for {} cs, {}", static_cast<int>(theEater->mZombieType),
		aSet.kind, aShadow ? "shadow, body only" : "normal", aSet.timer, ZombieHp(theEater)));
}

void ModPlantChewed(Zombie* theEater, Plant* thePlant) {
	if (thePlant->mPvzu.modSeed != kCharmed || !ModPlantShadowed(thePlant))
		return;
	const int aRange = u::CharmedTallnutBiteBackRollRange();
	const bool aMad = theEater->mZombiePhase == ZombiePhase::PHASE_NEWSPAPER_MAD && theEater->mHasObject;
	const int aRoll = aRange > 0 && !aMad ? Sexy::Rand(aRange) : 1; // 0xc021e9 Rand(200); the mad Newspaper check comes first
	const u::CharmedTallnutBiteBack aBack = u::CharmedTallnutOnEaten(true, static_cast<int>(theEater->mZombiePhase), theEater->mHasObject,
		theEater->mPvzu.variant, aRoll);
	if (aBack.damageToEater > 0)
		theEater->TakeDamage(aBack.damageToEater, 0U);
	if (aBack.charmEater && !theEater->mMindControlled) {
		theEater->mMindControlled = true; // raw write, as the exe
		Log(thePlant, std::format("charmedTallnut.biteBack charms zombie type {}", static_cast<int>(theEater->mZombieType)));
	}
	if (aBack.damageToEater > 0 && theEater->mBodyHealth % 200 < 8) // a line now and then, not per chew
		Log(thePlant, std::format("charmedTallnut.biteBack {} to zombie type {}: {}", aBack.damageToEater, static_cast<int>(theEater->mZombieType),
			ZombieHp(theEater)));
}

bool ModPlantRegenTick(Plant* thePlant) {
	if (thePlant->mPvzu.modSeed != kCharmed || !u::ModPlantsOn())
		return false;
	const int aBefore = thePlant->mPlantHealth;
	const u::NutRegen aRegen = u::CharmedTallnutRegenTick(ModPlantShadowed(thePlant), static_cast<int>(thePlant->mApp->mGameMode),
		thePlant->mPlantHealth, thePlant->mPlantMaxHealth, thePlant->mPvzu.extra, thePlant->mPvzu.counter);
	thePlant->mPlantHealth = aRegen.hp;
	thePlant->mPvzu.counter = aRegen.counter; // the exe's +0x58 count and +0xd0 last HP (as the Pool Tall-nut keeps them)
	thePlant->mPvzu.extra = aRegen.lastHp;
	if (thePlant->mPlantHealth != aBefore)
		Log(thePlant, std::format("charmedTallnut.regen {} -> {}", aBefore, thePlant->mPlantHealth));
	return true;
}

// Placement ---------------------------------------------------------------------------------------------------------

namespace {
// The seed whose rules apply: the mod seed in the cursor when it plants as thePlantingSeed, else thePlantingSeed.
int CursorSeed(const Board* theBoard, SeedType thePlantingSeed) {
	const CursorObject* aCursor = theBoard->mCursorObject.get();
	if (aCursor && (aCursor->mCursorType == CursorType::CURSOR_TYPE_PLANT_FROM_BANK ||
			aCursor->mCursorType == CursorType::CURSOR_TYPE_PLANT_FROM_USABLE_COIN) &&
		u::IsModSeedRange(static_cast<int>(aCursor->mType)) && ModPlantingSeed(aCursor->mType) == thePlantingSeed)
		return static_cast<int>(aCursor->mType);
	return static_cast<int>(thePlantingSeed);
}
bool gInPlacement = false; // the Loaded Cannon asks CanPlantAt about its right square
} // namespace

std::optional<PlantingReason> ModPlantCanPlantAt(Board* theBoard, int theGridX, int theGridY, SeedType thePlantingSeed) {
	if (gInPlacement || !ModPlantsOn())
		return std::nullopt;
	const int aSeed = CursorSeed(theBoard, thePlantingSeed);
	PlantsOnLawn aOnLawn;
	theBoard->GetPlantsOnLawn(theGridX, theGridY, &aOnLawn);
	Plant* aNormal = aOnLawn.mNormalPlant;

	// hook 47 + patch 0x40e137: Magic Devourer and Strong Coffee Bean go on any normal plant like a Coffee Bean, awake or
	// not (the vanilla Coffee Bean keeps its own rule here: port ruling, non-mod plants unchanged)
	if (aSeed == u::kModSeedMagicDevourer || aSeed == u::kModSeedStrongCoffeeBean) {
		const int aReason = u::CoffeeLikePlantingReason(aSeed, aOnLawn.mFlyingPlant != nullptr, aNormal != nullptr,
			aNormal && aNormal->mIsAsleep, aNormal ? aNormal->mWakeUpCounter : 0,
			aNormal && aNormal->mOnBungeeState == PlantOnBungeeState::GETTING_GRABBED_BY_BUNGEE);
		if (aReason >= 0)
			return static_cast<PlantingReason>(aReason);
	}

	// 0x957620: Wall-nut first aid compares the exact seeds, so a Charmed Tall-nut and a Tall-nut never replace each other
	if (aNormal && (thePlantingSeed == SeedType::SEED_TALLNUT || thePlantingSeed == SeedType::SEED_WALLNUT) &&
		!u::ModNutFirstAidAllowed(aSeed, static_cast<int>(ModSeedOf(aNormal))))
		return PlantingReason::PLANTING_NOT_HERE;

	// Loaded Cannon covers this square and the next (hook 40): port, both must take a plant
	if (aSeed == u::kModSeedLoadedCannon && u::LoadedCannonNeedsTwoSquares()) {
		gInPlacement = true;
		const bool aRightFree = theGridX + 1 < MAX_GRID_SIZE_X &&
			theBoard->CanPlantAt(theGridX + 1, theGridY, thePlantingSeed) == PlantingReason::PLANTING_OK;
		gInPlacement = false;
		if (!u::LoadedCannonFits(theGridX, MAX_GRID_SIZE_X, aRightFree))
			return PlantingReason::PLANTING_NOT_HERE;
	}
	return std::nullopt;
}

bool ModPlantFlyingSlot(const Plant* thePlant) {
	return thePlant->mPvzu.modSeed != 0 && u::ModPlantLawnSlot(thePlant->mPvzu.modSeed) == 3; // hook 41: 0x5b, 0x53, 0x6b
}

bool ModPlantLeftItsSquare(const Plant* thePlant) {
	if (thePlant->mPvzu.modSeed != u::kModSeedBowlingWallnut || thePlant->mBoard == nullptr || !ModPlantsOn())
		return false;
	const int aCentreCol = thePlant->mBoard->PixelToGridXKeepOnBoard(thePlant->mX + thePlant->mWidth / 2, thePlant->mY);
	return !u::BowlingNutInItsSquare(thePlant->mPlantCol, thePlant->mStartRow, thePlant->mRow, aCentreCol);
}

// Dev scenarios -----------------------------------------------------------------------------------------------------

namespace {
Plant* Put(Board* theBoard, int theCol, int theRow, int theSeed) {
	if (GridItem* aGrave = theBoard->GetGraveStoneAt(theCol, theRow))
		aGrave->GridItemDie();
	if (theBoard->GetTopPlantAt(theCol, theRow, PlantPriority::TOPPLANT_ANY))
		return nullptr;
	return theBoard->AddPlant(theCol, theRow, static_cast<SeedType>(theSeed), SeedType::SEED_NONE);
}

Zombie* Walker(Board* theBoard, ZombieType theType, int theRow, float theX) {
	Zombie* aZombie = theBoard->AddZombieInRow(theType, theRow, 0);
	if (aZombie && theX > 0.0f) {
		aZombie->mPosX = theX;
		aZombie->mX = static_cast<int>(theX);
	}
	return aZombie;
}

void DumpState(Board* theBoard, const char* theWhen) {
	for (Plant* aPlant : theBoard->mPlants)
		if (!aPlant->mDead && (aPlant->mPvzu.modSeed != 0 || aPlant->mSeedType == SeedType::SEED_PEASHOOTER))
		{
			const Reanimation* aBody = aPlant->mApp->ReanimationTryToGet(aPlant->mBodyReanimID);
			Log(aPlant, std::format("dump {} shadow {} bitten {} hp {}/{} launchRate {} colour {}", theWhen, aPlant->mPvzu.shadow,
				aPlant->mPvzu.charmedBitten, aPlant->mPlantHealth, aPlant->mPlantMaxHealth, aPlant->mLaunchRate,
				aBody ? std::format("{},{},{}", aBody->mColorOverride.mRed, aBody->mColorOverride.mGreen, aBody->mColorOverride.mBlue) : "-"));
		}
	for (Zombie* aZombie : theBoard->mZombies)
		if (!aZombie->mDead && (aZombie->mPvzu.toxicKind || aZombie->mPvzu.repeaterPoisonCs || aZombie->mPvzu.weakenCs))
			LogBoard(theBoard, std::format("dump {} zombie type {} row {} toxic kind {} timer {} poison {} weakened {} {} hypno {}", theWhen,
				static_cast<int>(aZombie->mZombieType), aZombie->mRow, aZombie->mPvzu.toxicKind, aZombie->mPvzu.toxicTimer,
				aZombie->mPvzu.repeaterPoisonCs, aZombie->mPvzu.weakenCs, ZombieHp(aZombie), aZombie->mMindControlled ? 1 : 0));
}

const char* ReasonName(PlantingReason theReason) {
	switch (theReason) {
	case PlantingReason::PLANTING_OK: return "OK";
	case PlantingReason::PLANTING_NOT_HERE: return "NOT_HERE";
	case PlantingReason::PLANTING_ONLY_ON_GRAVES: return "ONLY_ON_GRAVES";
	case PlantingReason::PLANTING_NEEDS_SLEEPING: return "NEEDS_SLEEPING";
	case PlantingReason::PLANTING_NOT_ON_GRAVE: return "NOT_ON_GRAVE";
	case PlantingReason::PLANTING_NEEDS_UPGRADE: return "NEEDS_UPGRADE";
	default: return "other";
	}
}

// CanPlantAt as the player's cursor holding theCursorSeed would ask it.
void AskPlacement(Board* theBoard, int theCursorSeed, int theCol, int theRow, const char* theWhat) {
	CursorObject* aCursor = theBoard->mCursorObject.get();
	const CursorType aOldType = aCursor->mCursorType;
	const SeedType aOldSeed = aCursor->mType;
	aCursor->mCursorType = CursorType::CURSOR_TYPE_PLANT_FROM_BANK;
	aCursor->mType = static_cast<SeedType>(theCursorSeed);
	const PlantingReason aReason = theBoard->CanPlantAt(theCol, theRow, theBoard->GetSeedTypeInCursor());
	aCursor->mCursorType = aOldType;
	aCursor->mType = aOldSeed;
	LogBoard(theBoard, std::format("place {} at col {} row {} ({}): {}", Hex(theCursorSeed), theCol, theRow, theWhat, ReasonName(aReason)));
}

void LayOut(Board* theBoard, const std::string& theScenario) {
	if (theScenario == "shadow") {
		// row 0: Moonflower + Shadow Peashooter (1 point: on) and a lone Dusk-pult (0: off)
		Put(theBoard, 2, 0, kMoonflower);
		Put(theBoard, 3, 0, u::kModSeedShadowPeashooter);
		Put(theBoard, 7, 0, u::kModSeedDuskPult);
		// rows 2-3: Moonflower M2 (2,2) around Toxic (1,2), Primal Sunflower (3,2), Dusk-pult (2,3), Fire-dragon (3,3)
		// = 4 points: shadowed, so it gives its neighbours 2; Fire-dragon (2 < 3) stays off until tick 600 brings a
		// Moonflower to (4,4) (3 points)
		Put(theBoard, 2, 2, kMoonflower);
		Put(theBoard, 1, 2, kToxic);
		Put(theBoard, 3, 2, u::kModSeedPrimalSunflower);
		Put(theBoard, 2, 3, u::kModSeedDuskPult);
		Put(theBoard, 3, 3, u::kModSeedFireDragonGrass);
		// row 4 / col 0: Dream Starfruit (0,4) with one plain Moonflower (1 point < 3)
		Put(theBoard, 0, 4, u::kModSeedDreamStarfruit);
		Put(theBoard, 0, 3, kMoonflower);
		// row 4: Charmed Tall-nut next to one Moonflower (1 point < 2); a second Moonflower comes at tick 300
		Put(theBoard, 6, 4, kCharmed);
		Put(theBoard, 5, 4, kMoonflower);
	} else if (theScenario == "toxic") {
		// row 1: a plain Toxic Shadow-shroom; row 3: one next to a Moonflower (shadowed). Bucketheads eat both.
		Put(theBoard, 4, 1, kToxic);
		Put(theBoard, 4, 3, kToxic);
		Put(theBoard, 3, 3, kMoonflower);
		Walker(theBoard, ZombieType::ZOMBIE_PAIL, 1, 520.0f);
		Walker(theBoard, ZombieType::ZOMBIE_PAIL, 3, 520.0f);
	} else if (theScenario == "charmed") {
		// a Charmed Tall-nut eaten by a pile of zombies; at tick 2500 two Moonflowers put it in the shadow state
		Put(theBoard, 5, 2, kCharmed);
	} else if (theScenario == "charmed-shadow") {
		// a Charmed Tall-nut under 75% HP in the shadow state from the start (no Gargantuar): bites x1.5, bite-back
		if (Plant* p = Put(theBoard, 5, 2, kCharmed))
			p->mPlantHealth = 5000;
		Put(theBoard, 4, 1, kMoonflower);
		Put(theBoard, 6, 3, kMoonflower);
	} else if (theScenario == "status") {
		// row 1: Repeater poison on a Buckethead (two hits = 1000 cs); row 3: a weakened Conehead in front of a Peashooter
		// (its peas hit for 40); row 4: an unweakened Conehead in front of a Peashooter
		Put(theBoard, 0, 3, SeedType::SEED_PEASHOOTER);
		Put(theBoard, 0, 4, SeedType::SEED_PEASHOOTER);
		if (Zombie* z = Walker(theBoard, ZombieType::ZOMBIE_PAIL, 1, 600.0f)) {
			ModZombieApplyPoison(z, u::PrimalPoisonAddCs(0x23));
			ModZombieApplyPoison(z, u::PrimalPoisonAddCs(0x23));
		}
		if (Zombie* z = Walker(theBoard, ZombieType::ZOMBIE_TRAFFIC_CONE, 3, 600.0f))
			ModZombieAddWeakened(z, 2000);
		Walker(theBoard, ZombieType::ZOMBIE_TRAFFIC_CONE, 4, 600.0f);
	} else if (theScenario == "base") {
		// base-seed rules that compare the raw seed in the exe must skip mod plants: rows 0 / 1 a Peashooter and a Shadow
		// Peashooter (speed-up 0xc05d10), row 2 a Spirit-tail beside a Plantern (no Cattail +25, 0x6516cb), Bucketheads
		// in rows 0..2 keep them shooting; the dump logs each launch rate
		Put(theBoard, 0, 0, SeedType::SEED_PEASHOOTER);
		Put(theBoard, 0, 1, u::kModSeedShadowPeashooter);
		Put(theBoard, 1, 2, SeedType::SEED_PLANTERN);
		Put(theBoard, 0, 2, u::kModSeedSpiritTail);
		for (int aRow = 0; aRow < 3; ++aRow)
			if (Zombie* z = Walker(theBoard, ZombieType::ZOMBIE_PAIL, aRow, 1100.0f))
				z->mBodyHealth = z->mBodyMaxHealth = 100000; // a target for the whole run
	} else if (theScenario == "place") {
		theBoard->mApp->mPlayerInfo->mPurchases[StoreItem::STORE_ITEM_FIRSTAID] = 1;
		Put(theBoard, 1, 0, SeedType::SEED_PEASHOOTER);
		Put(theBoard, 2, 0, SeedType::SEED_PUFFSHROOM);
		if (!theBoard->GetGraveStoneAt(4, 0))
			theBoard->AddAGraveStone(4, 0);
		if (Plant* p = Put(theBoard, 1, 1, kCharmed))
			p->mPlantHealth = 1000;
		if (Plant* p = Put(theBoard, 2, 1, SeedType::SEED_TALLNUT))
			p->mPlantHealth = 1000;
		Put(theBoard, 5, 2, SeedType::SEED_PEASHOOTER);
		Put(theBoard, 0, 3, u::kModSeedBowlingWallnut);
		Put(theBoard, 2, 3, u::kModSeedRewindGrass);
	}
	LogBoard(theBoard, std::format("dev shadow scenario {} laid out", theScenario));
}

void Feed(Board* theBoard, const std::string& theScenario, int theTick) {
	if (theScenario == "shadow" && theTick == 300)
		Put(theBoard, 7, 4, kMoonflower); // the Charmed Tall-nut (6,4) now has 2 points: shadow, Gargantuar
	if (theScenario == "shadow" && theTick == 600)
		Put(theBoard, 4, 4, kMoonflower); // Fire-dragon (3,3): 2 (shadowed M2) + 1 = 3
	if (theScenario == "toxic" && theTick == 1500)
		Walker(theBoard, ZombieType::ZOMBIE_NORMAL, 1, 0.0f); // walks in later: nothing poisons it
	if (theScenario == "charmed-shadow" && theTick % 120 == 1 && theTick < 6000)
		Walker(theBoard, ZombieType::ZOMBIE_NORMAL, 2, 560.0f);
	if (theScenario == "charmed") {
		if (theTick % 120 == 1 && theTick < 6000) // a steady stream of eaters in the nut's row
			Walker(theBoard, ZombieType::ZOMBIE_NORMAL, 2, 560.0f);
		if (theTick == 2500) {
			Put(theBoard, 4, 1, kMoonflower);
			Put(theBoard, 6, 3, kMoonflower);
		}
	}
	if (theScenario == "place" && theTick == 5) {
		AskPlacement(theBoard, u::kModSeedMagicDevourer, 1, 0, "on an awake Peashooter");
		AskPlacement(theBoard, u::kModSeedMagicDevourer, 6, 0, "on an empty square");
		AskPlacement(theBoard, u::kModSeedMagicDevourer, 4, 0, "on a grave, no plant");
		AskPlacement(theBoard, SeedType::SEED_GRAVEBUSTER, 4, 0, "vanilla Grave Buster on the grave");
		AskPlacement(theBoard, u::kModSeedStrongCoffeeBean, 1, 0, "on an awake Peashooter");
		AskPlacement(theBoard, u::kModSeedStrongCoffeeBean, 2, 0, "on a Puff-shroom");
		AskPlacement(theBoard, SeedType::SEED_INSTANT_COFFEE, 1, 0, "vanilla Coffee Bean on an awake Peashooter");
		AskPlacement(theBoard, kCharmed, 1, 1, "Charmed on a hurt Charmed Tall-nut (first aid)");
		AskPlacement(theBoard, kCharmed, 2, 1, "Charmed on a hurt vanilla Tall-nut");
		AskPlacement(theBoard, SeedType::SEED_TALLNUT, 1, 1, "vanilla Tall-nut on a hurt Charmed Tall-nut");
		AskPlacement(theBoard, SeedType::SEED_TALLNUT, 2, 1, "vanilla Tall-nut on a hurt vanilla Tall-nut");
		AskPlacement(theBoard, u::kModSeedLoadedCannon, 4, 2, "Loaded Cannon, right square has a Peashooter");
		AskPlacement(theBoard, u::kModSeedLoadedCannon, 2, 2, "Loaded Cannon, both squares free");
		AskPlacement(theBoard, u::kModSeedLoadedCannon, 8, 2, "Loaded Cannon, last column");
		AskPlacement(theBoard, SeedType::SEED_PEASHOOTER, 0, 3, "Peashooter on a Bowling Wall-nut's square (flying slot)");
		AskPlacement(theBoard, SeedType::SEED_PEASHOOTER, 2, 3, "Peashooter on a Rewind Grass's square (flying slot)");
		AskPlacement(theBoard, u::kModSeedMagicDevourer, 1, 0, "again");
		PlantsOnLawn aOnLawn;
		theBoard->GetPlantsOnLawn(0, 3, &aOnLawn);
		LogBoard(theBoard, std::format("layers col 0 row 3: flying {} normal {}", aOnLawn.mFlyingPlant ? Hex(ModSeedOf(aOnLawn.mFlyingPlant)) : "-",
			aOnLawn.mNormalPlant ? Hex(ModSeedOf(aOnLawn.mNormalPlant)) : "-"));
		// plant for real: a Magic Devourer on the Peashooter, a Loaded Cannon on (2,2)
		CursorObject* aCursor = theBoard->mCursorObject.get();
		aCursor->mCursorType = CursorType::CURSOR_TYPE_PLANT_FROM_BANK;
		aCursor->mType = static_cast<SeedType>(u::kModSeedMagicDevourer);
		if (theBoard->CanPlantAt(1, 0, theBoard->GetSeedTypeInCursor()) == PlantingReason::PLANTING_OK)
			theBoard->AddPlant(1, 0, static_cast<SeedType>(u::kModSeedMagicDevourer), SeedType::SEED_NONE);
		aCursor->mType = static_cast<SeedType>(u::kModSeedLoadedCannon);
		if (theBoard->CanPlantAt(2, 2, theBoard->GetSeedTypeInCursor()) == PlantingReason::PLANTING_OK)
			theBoard->AddPlant(2, 2, static_cast<SeedType>(u::kModSeedLoadedCannon), SeedType::SEED_NONE);
		aCursor->mCursorType = CursorType::CURSOR_TYPE_NORMAL;
		aCursor->mType = SeedType::SEED_NONE;
		theBoard->GetPlantsOnLawn(1, 0, &aOnLawn);
		LogBoard(theBoard, std::format("layers col 1 row 0: flying {} normal {}", aOnLawn.mFlyingPlant ? Hex(ModSeedOf(aOnLawn.mFlyingPlant)) : "-",
			aOnLawn.mNormalPlant ? Hex(ModSeedOf(aOnLawn.mNormalPlant)) : "-"));
		theBoard->GetPlantsOnLawn(3, 2, &aOnLawn);
		LogBoard(theBoard, std::format("layers col 3 row 2 (the cannon's right half): normal {}",
			aOnLawn.mNormalPlant ? Hex(ModSeedOf(aOnLawn.mNormalPlant)) : "-"));
		AskPlacement(theBoard, SeedType::SEED_PEASHOOTER, 3, 2, "Peashooter on the cannon's right half");
	}
}
} // namespace

void ModPlantsShadowDevTick(Board* theBoard, int thePlayTicks) {
	static const std::optional<std::string> aScenario = CmdOption("-pvzu-dev-shadow-test");
	static const int aDumpAt = CmdOptionInt("-pvzu-dev-shadow-dump", -1);
	if (aScenario && !aScenario->empty()) {
		if (thePlayTicks == 1) {
			theBoard->mSunMoney = 5000;
			LayOut(theBoard, *aScenario);
		}
		Feed(theBoard, *aScenario, thePlayTicks);
	}
	if (thePlayTicks == aDumpAt)
		DumpState(theBoard, aScenario ? "run" : "after load");
}

} // namespace pvzu::engine
