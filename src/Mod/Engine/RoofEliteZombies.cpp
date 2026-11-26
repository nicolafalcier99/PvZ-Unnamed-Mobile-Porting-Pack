// PVZU: engine side of the Unnamed Roof zombies. Rules: Mod/Unnamed/RoofZombies.h; evidence: the research notes (w5-roof)
// section 2 and pvz-unnamed-pack/SOURCES-roof-zombies.md.
#include "Mod/Engine/RoofEliteZombies.h"
#include "Mod/Engine/OptionsPanel.h"

#include "Mod/Engine/EliteZombies.h"
#include "Mod/Engine/EngineTables.h"
#include "Mod/Engine/World7EliteZombies.h"
#include "Mod/Engine/ZombieGapsV1.h"
#include "Mod/Unnamed/ZombieGapsV1.h"
#include "Mod/GameData.h"
#include "Mod/Unnamed/FogZombies.h"
#include "Mod/Unnamed/RoofZombies.h"

#include "LawnApp.h"
#include "Resources.h"
#include "Lawn/Board.h"
#include "Lawn/Challenge.h"
#include "Lawn/GridItem.h"
#include "Lawn/Plant.h"
#include "Lawn/Zombie.h"
#include "Lawn/System/PlayerInfo.h"
#include "PvzpLib/PvzpCommon.h"
#include "PvzpLib/Reanimator.h"
#include "SexyAppFramework/graphics/SharedImage.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <map>
#include <string>

using namespace Sexy;
namespace u = pvzu::unnamed;
namespace re = pvzu::unnamed::roofelite;

namespace pvzu::engine {

bool ZombossBungeeSpawning(); // Mod/Engine/ZombossSites.h (Plan 6 Task 4): exe flag 0xc04476, boss bungees being made

namespace {

int gPendingPoleHit = -1;   // the Gargantuar's vanilla aPoleHit of the ZombieInitialize running now (hook 513)
// The global "elite zombies" option (exe 0x800258, study G2; Mod/Engine/OptionsPanel.h): the Bungee (0x796ec0 /
// 0x796eb4), Ladder (0x796d2c, 0x95aa1f) and Catapult (0x796e16, 0x910002) rolls are code the setting patches
// (0x7b2000 / 0x7b2100); 0x95aa15, 0x91009b and hook 513 read the value, which a throwing staff Giga sets to "all"
// for the zombie it throws (hook 565, EliteValueAllScope).

int BoardTime(const Zombie* z) { return z->mBoard ? static_cast<int>(z->mBoard->mMainCounter) : 0; }
int Frame() { return gSexyAppBase ? static_cast<int>(gSexyAppBase->mUpdateCount) : 0; }
int Id(Zombie* z) { return z && z->mBoard ? static_cast<int>(z->mBoard->ZombieGetID(z)) & 0xFFFF : -1; }
const char* Name(Zombie* z) { return u::RoofEliteName(z->mZombieType, z->mPvzu.variant); }
bool Is(const Zombie* z, ZombieType t) { return z->mZombieType == t; }
bool IsGarg(const Zombie* z) { return Is(z, ZombieType::ZOMBIE_GARGANTUAR) || Is(z, ZombieType::ZOMBIE_REDEYE_GARGANTUAR); }
bool HasLadder(const Zombie* z) { return z->mShieldType == ShieldType::SHIELDTYPE_LADDER; }
const char* TypeName(ZombieType t) {
	RegisterVanillaTables();
	const auto& zombies = GameData::Get().Vanilla().zombies;
	return t >= 0 && static_cast<size_t>(t) < zombies.size() ? zombies[t].name.c_str() : "?";
}

// Elite art by pack file name. The engine resources are used when loaded (so the asset overlay serves Unnamed's file of
// the same name); the staff is Unnamed's own file (mod resources.xml line 385), loaded by name from the pack.
Image* RoofImage(const std::string& theFile) {
	static const std::map<std::string, Image**> kResources = {
		{"images/Zen_WheelBarrow", &IMAGE_ZEN_WHEELBARROW},
		{"reanim/Zombie_jackbox_box", &IMAGE_REANIM_ZOMBIE_JACKBOX_BOX},
		{"images/Shovel_hi_res", &IMAGE_SHOVEL_HI_RES},
		{"particles/ZombiePoleVaulterHead", &IMAGE_ZOMBIEPOLEVAULTERHEAD},
		{"reanim/Zombie_ladder_1", &IMAGE_REANIM_ZOMBIE_LADDER_1},
	};
	if (auto it = kResources.find(theFile); it != kResources.end() && *it->second != nullptr)
		return *it->second;
	static std::map<std::string, SharedImageRef> loaded; // kept alive for the whole run
	auto it = loaded.find(theFile);
	if (it == loaded.end()) {
		it = loaded.emplace(theFile, gSexyAppBase->GetSharedImage(theFile)).first;
		if (static_cast<Image*>(it->second) == nullptr)
			std::fprintf(stderr, "PVZU: roof elite image %s not found, drawing the base zombie\n", theFile.c_str());
	}
	return static_cast<Image*>(it->second);
}

void ApplyRoofArt(Zombie* theZombie) {
	Reanimation* aBody = theZombie->mApp->ReanimationTryToGet(theZombie->mBodyReanimID);
	if (aBody == nullptr)
		return;
	const auto art = u::ResolveRoofArt(theZombie->mZombieType, theZombie->mPvzu.variant,
		[](const std::string& f) { return RoofImage(f) != nullptr; });
	for (const auto& img : art)
		aBody->SetImageOverride(img.track.c_str(), RoofImage(img.file));
}

void LogForm(const char* theWhat, Zombie* theZombie) {
	LogZombieEvent("t=%d f=%d roof %s %s#%d form=%s variant=0x%x row=%d x=%.1f body=%d shield=%d spares=%d balls=%d",
		BoardTime(theZombie), Frame(), theWhat, TypeName(theZombie->mZombieType), Id(theZombie), Name(theZombie),
		theZombie->mPvzu.variant, theZombie->mRow, theZombie->mPosX, theZombie->mBodyHealth, theZombie->mShieldHealth,
		theZombie->mPvzu.counter, theZombie->mSummonCounter);
}

} // namespace

bool RoofZombiesOn() { return GameData::Get().Feature("roofZombies"); }

void RoofGargPoleRoll(int thePoleHit) { gPendingPoleHit = thePoleHit; }

void RoofEliteInit(Zombie* theZombie, bool theOnBoard) {
	const ZombieType aType = theZombie->mZombieType;
	const int aPoleHit = gPendingPoleHit;
	gPendingPoleHit = -1;
	if (!RoofZombiesOn() || !theOnBoard || !u::IsRoofType(aType))
		return;
	LawnApp* anApp = theZombie->mApp;
	Board* aBoard = theZombie->mBoard;
	int aForced = 0;
	const bool aIsForced = ForcedVariantFor(aType, aForced); // a forced type rolls nothing for its form (dev only)
	int v = aIsForced ? aForced : 0;
	const int aSetting = EliteOption(), aValue = EliteValue(); // PVZU elite-all
	if (aType == ZombieType::ZOMBIE_BUNGEE) { // hook 488
		if (const int n = u::BungeeRollRange(aSetting); !aIsForced && n > 0)
			v = u::RollBungee(Rand(n), anApp->IsIZombieLevel() || ZombossBungeeSpawning());
	} else if (aType == ZombieType::ZOMBIE_LADDER) { // hook 487
		if (const int n = u::LadderShovelRollRange(aSetting); !aIsForced && n > 0) {
			v = u::RollLadderShovel(Rand(n), aSetting);
			if (const int n2 = u::LadderSecondRollRange(aValue, aSetting); v == 0 && n2 > 0)
				v = u::RollLadderSecond(Rand(n2), aBoard ? static_cast<int>(aBoard->mPlants.mSize) : 0);
		}
		if (v == re::LADDER_PILE) // 0xc02cc0: Board+0x5888[id] = 5 (11 from PlayerInfo level 60)
			theZombie->mPvzu.counter = u::PileSpares(anApp->mPlayerInfo ? anApp->mPlayerInfo->mLevel : 0);
		theZombie->mShieldHealth = u::RoofShieldHp(aType, v, theZombie->mShieldHealth);
	} else if (aType == ZombieType::ZOMBIE_CATAPULT) { // hook 498
		if (const int n = u::CatapultRollRange(aSetting); !aIsForced && n > 0) {
			const int r = Rand(n);
			v = World7CatapultForm(theZombie, r, aSetting); // PVZU Plan 7: 50..59 (83..99 "all") with profile level > 60 -> vase 0x70
			if (v == 0)
				v = u::RollCatapult(r, aValue, aSetting);
		}
	} else if (!aIsForced && aPoleHit >= 0 && u::GargStaffHit(aPoleHit, aValue)) { // hook 513
		v = re::STAFF;
	}
	theZombie->mPvzu.variant = v;
	theZombie->mBodyHealth = u::RoofBodyHp(aType, v, theZombie->mBodyHealth);
	if (v != 0)
		ApplyRoofArt(theZombie);
	World7CatapultInit(theZombie); // PVZU Plan 7: the vase catapult's 10 balls and art
	LogForm("form", theZombie);
}

void RoofAfterLoad(Zombie* theZombie) {
	if (!RoofZombiesOn() || theZombie->mDead || !u::IsRoofType(theZombie->mZombieType) || theZombie->mPvzu.variant == 0)
		return;
	ApplyRoofArt(theZombie);
	LogForm("load", theZombie);
}

// Stand-in until Plan 6 Task 4 (Mod/Engine/ZombossSites.cpp) is merged: its strong definition replaces this one.

// ---- Bungee ----

int RoofBungeeBottomCounter(Zombie* theZombie, int theVanilla) {
	if (!RoofZombiesOn() || !Is(theZombie, ZombieType::ZOMBIE_BUNGEE))
		return theVanilla;
	const int aCounter = u::BungeeBottomCounter(theZombie->mPvzu.variant, theVanilla);
	if (theZombie->mPvzu.variant != 0)
		LogZombieEvent("t=%d f=%d roof bungee-bottom BUNGEE#%d form=%s col=%d row=%d counter=%d", BoardTime(theZombie), Frame(),
			Id(theZombie), Name(theZombie), theZombie->mTargetCol, theZombie->mRow, aCounter);
	return aCounter;
}

bool RoofBungeeStealsZombie(Zombie* theZombie) { // hook 522 -> 0x959000 / 0xc02530 / 0x959023
	if (!RoofZombiesOn() || !u::BungeeCarriesZombies())
		return false;
	Board* aBoard = theZombie->mBoard;
	for (Zombie* z : aBoard->mZombies) {
		if (z == theZombie || z->mRow != theZombie->mRow)
			continue;
		if (!u::BungeeCanCarry(theZombie->mMindControlled, z->mZombieType, z->mBodyMaxHealth, z->mMindControlled,
			z->mDead || z->IsDeadOrDying()))
			continue;
		if (!z->EffectedByDamage(theZombie->mMindControlled ? 1U : 0x81U))
			continue;
		const Rect aRect = z->GetZombieRect();
		if (!u::ZombieCoversColumn(aRect.mX, aRect.mWidth, theZombie->mTargetCol))
			continue;
		theZombie->mRelatedZombieID = aBoard->ZombieGetID(z);
		z->mZombieHeight = ZombieHeight::HEIGHT_GETTING_BUNGEE_DROPPED;   // carried (height 9)
		LogZombieEvent("t=%d f=%d roof bungee-takes-zombie BUNGEE#%d -> %s#%d hypnotised=%d col=%d row=%d", BoardTime(theZombie),
			Frame(), Id(theZombie), TypeName(z->mZombieType), Id(z), z->mMindControlled ? 1 : 0, theZombie->mTargetCol, theZombie->mRow);
		return true;
	}
	return false;
}

bool RoofPlantAlreadyGrabbed(Plant* thePlant) {
	return RoofZombiesOn() && thePlant->mOnBungeeState != PlantOnBungeeState::NOT_ON_BUNGEE;
}

bool RoofBungeeLiftsZombie(Zombie* theZombie) { // hook 524 -> 0x959100: foley 0x44, no plant
	if (!RoofZombiesOn() || theZombie->mRelatedZombieID == ZombieID::ZOMBIEID_NULL)
		return false;
	if (theZombie->mBoard->ZombieTryToGet(theZombie->mRelatedZombieID) == nullptr)
		return false;
	theZombie->mApp->PlayFoley(FoleyType::FOLEY_FLOOP);
	return true;
}

void RoofBungeeLiftedPlant(Zombie* theZombie, Plant* thePlant) { // hook 529 -> 0x797051
	if (!RoofZombiesOn() || !u::JackBungeeLeavesJack(theZombie->mPvzu.variant) || !theZombie->IsOnBoard())
		return;
	Board* aBoard = theZombie->mBoard;
	Zombie* aJack = aBoard->AddZombieInRow(ZombieType::ZOMBIE_JACK_IN_THE_BOX, thePlant->mRow, theZombie->mFromWave);
	if (aJack == nullptr)
		return;
	aJack->mPosX = u::JackBungeeJackX(static_cast<float>(aBoard->GridToPixelX(thePlant->mPlantCol, thePlant->mRow)));
	aJack->mX = static_cast<int>(aJack->mPosX);
	aJack->mPhaseCounter = 0;   // pops at once (Almanac "a big surprise")
	LogZombieEvent("t=%d f=%d roof jack-bungee-drops BUNGEE#%d -> JACK_IN_THE_BOX#%d form=%s col=%d row=%d x=%.1f",
		BoardTime(theZombie), Frame(), Id(theZombie), Id(aJack), u::FogEliteName(aJack->mZombieType, aJack->mPvzu.variant),
		thePlant->mPlantCol, thePlant->mRow, aJack->mPosX);
}

bool RoofBlocksPlanting(Board* theBoard, int theGridX, int theGridY) { // hook 46
	if (!RoofZombiesOn())
		return false;
	for (Zombie* z : theBoard->mZombies) {
		if (z->mDead || !z->mHasHead || z->mMindControlled || !Is(z, ZombieType::ZOMBIE_BUNGEE) ||
			z->mZombiePhase != ZombiePhase::PHASE_BUNGEE_AT_BOTTOM || z->mPvzu.variant != re::WHEELBARROW)
			continue;
		if (u::WheelbarrowBlocksCell(z->mRow, z->mTargetCol, theGridX, theGridY))
			return true;
	}
	return false;
}

// ---- Ladder ----

bool RoofLadderTargetsAnyPlant(Zombie* theZombie) {
	return RoofZombiesOn() && Is(theZombie, ZombieType::ZOMBIE_LADDER) && u::LadderTargetsAnyPlant(theZombie->mPvzu.variant);
}

std::optional<bool> RoofCanTargetPlant(Zombie* theZombie, Plant* thePlant, ZombieAttackType theAttackType) {
	if (!RoofZombiesOn() || !Is(theZombie, ZombieType::ZOMBIE_LADDER) || theZombie->mPvzu.variant != re::LADDER_PILE)
		return std::nullopt;
	if (ZombieGapsV1On() && u::PileTargetRuleOnlyWhileCarrying() && // PVZU V1: 0xc02c90 sits in the vanilla phase 76/77 block
		theZombie->mZombiePhase != ZombiePhase::PHASE_LADDER_CARRYING && theZombie->mZombiePhase != ZombiePhase::PHASE_LADDER_PLACING)
		return std::nullopt;
	const bool aLadder = theZombie->mBoard->GetLadderAt(thePlant->mPlantCol, thePlant->mRow) != nullptr;
	return u::PileCanTarget(theZombie->mPvzu.variant, static_cast<int>(theAttackType), aLadder);
}

bool RoofLadderPlaces(Zombie* theZombie, Plant* thePlant) { // hook 600 -> 0x796d8f
	if (!RoofZombiesOn() || !Is(theZombie, ZombieType::ZOMBIE_LADDER) || !u::ShovelDigsUpPlant(theZombie->mPvzu.variant))
		return false;
	theZombie->mApp->PlayFoley(FoleyType::FOLEY_USE_SHOVEL); // foley 8
	LogZombieEvent("t=%d f=%d roof shovel-digs LADDER#%d -> plant seed=%d col=%d row=%d hp=%d", BoardTime(theZombie), Frame(),
		Id(theZombie), static_cast<int>(thePlant->mSeedType), thePlant->mPlantCol, thePlant->mRow, thePlant->mPlantHealth);
	thePlant->Die();
	return true;
}

void RoofLadderPlaced(Zombie* theZombie, Plant* thePlant) { // hook 600 -> 0x95ac80
	if (!RoofZombiesOn() || !Is(theZombie, ZombieType::ZOMBIE_LADDER))
		return;
	const bool aBungee = u::PileCallsBungee(theZombie->mPvzu.variant);
	if (aBungee)
		theZombie->mBoard->mChallenge->IZombiePlaceZombie(ZombieType::ZOMBIE_BUNGEE, thePlant->mPlantCol, thePlant->mRow);
	if (theZombie->mPvzu.variant != 0)
		LogZombieEvent("t=%d f=%d roof ladder-placed LADDER#%d form=%s col=%d row=%d bungee=%d keepsLadder=%d", BoardTime(theZombie),
			Frame(), Id(theZombie), Name(theZombie), thePlant->mPlantCol, thePlant->mRow, aBungee ? 1 : 0,
			u::KeepsShieldOnLadder(theZombie->mPvzu.variant) ? 1 : 0);
}

bool RoofKeepsShieldOnLadder(Zombie* theZombie) {
	return RoofZombiesOn() && Is(theZombie, ZombieType::ZOMBIE_LADDER) && u::KeepsShieldOnLadder(theZombie->mPvzu.variant);
}

bool RoofReattachLadder(Zombie* theZombie) { // hook 722 -> 0x95ab40
	if (!RoofZombiesOn() || !Is(theZombie, ZombieType::ZOMBIE_LADDER) || theZombie->mPvzu.variant != re::LADDER_PILE ||
		!HasLadder(theZombie) || theZombie->mPvzu.counter <= 0)
		return false;
	Reanimation* aBody = theZombie->mApp->ReanimationTryToGet(theZombie->mBodyReanimID);
	if (aBody == nullptr)
		return false;
	theZombie->mPvzu.counter--;
	theZombie->mShieldHealth = theZombie->mShieldMaxHealth = u::PileLadderHp();
	theZombie->AttachShield();
	aBody->SetImageOverride("Zombie_ladder_1", RoofImage("reanim/Zombie_ladder_1"));
	LogForm("new-ladder", theZombie);
	return true;
}

bool RoofKeepsLadderShield(Zombie* theZombie) { // hook 694 -> 0x95ac10
	return RoofZombiesOn() && Is(theZombie, ZombieType::ZOMBIE_LADDER) && theZombie->mPvzu.variant == re::LADDER_PILE &&
		HasLadder(theZombie) && theZombie->mShieldHealth > 0;
}

void RoofBodyDamageHitsLadder(Zombie* theZombie, int theDamage) { // hook 702 -> 0xc02d0f
	if (!RoofZombiesOn() || !Is(theZombie, ZombieType::ZOMBIE_LADDER))
		return;
	if (const int d = u::PileBodyToShield(theZombie->mPvzu.variant, HasLadder(theZombie), theDamage); d > 0)
		theZombie->TakeShieldDamage(d, 0U);
}

int RoofOverflowIntoLadders(Zombie* theZombie, int theRemaining, unsigned int theDamageFlags) { // hook 710 -> 0x95ac40
	if (!RoofZombiesOn() || !Is(theZombie, ZombieType::ZOMBIE_LADDER) || !u::PileOverflowsIntoNextLadder(theZombie->mPvzu.variant))
		return theRemaining;
	for (int aGuard = 0; theRemaining > 0 && HasLadder(theZombie) && aGuard < 64; ++aGuard)
		theRemaining = theZombie->TakeShieldDamage(theRemaining, theDamageFlags);
	return theRemaining;
}

std::optional<bool> RoofPileBitesThisTick(Zombie* theZombie) {
	if (!RoofZombiesOn() || !ZombieGapsV1On() || !Is(theZombie, ZombieType::ZOMBIE_LADDER))
		return std::nullopt;
	return u::PileBitesThisTick(theZombie->mPvzu.variant, theZombie->mShieldType == ShieldType::SHIELDTYPE_LADDER,
		theZombie->mZombieAge, theZombie->mChilledCounter > 0);
}

bool RoofEatsPlants(Zombie* theZombie) {
	if (ZombieGapsV1On() && u::PileTargetRuleOnlyWhileCarrying()) // PVZU V1: no "no bites with a ladder" rule in the exe
		return true;
	return !RoofZombiesOn() || !Is(theZombie, ZombieType::ZOMBIE_LADDER) || u::PileBites(theZombie->mPvzu.variant, HasLadder(theZombie));
}

float RoofAnimRate(Zombie* theZombie, float theAnimRate) {
	if (!RoofZombiesOn() || !Is(theZombie, ZombieType::ZOMBIE_LADDER))
		return theAnimRate;
	return theAnimRate * u::LadderAnimRateMultiplier(theZombie->mPvzu.variant, HasLadder(theZombie), theZombie->mIsEating);
}

void RoofZombieTick(Zombie* theZombie) { // hook 619 -> 0x95c2a0
	if (!RoofZombiesOn() || !Is(theZombie, ZombieType::ZOMBIE_LADDER) || theZombie->mPvzu.variant != re::GHOST_LADDER ||
		theZombie->mZombieAge != u::GhostTeleportAge() || theZombie->mDead || !theZombie->IsOnBoard() ||
		theZombie->mZombieHeight != ZombieHeight::HEIGHT_ZOMBIE_NORMAL || theZombie->mMindControlled)
		return;
	Board* aBoard = theZombie->mBoard;
	int aMaxCol = -1;
	for (Plant* p : aBoard->mPlants)
		if (!p->mDead && p->mRow == theZombie->mRow && aBoard->GetLadderAt(p->mPlantCol, p->mRow) == nullptr)
			aMaxCol = std::max(aMaxCol, p->mPlantCol);
	const float aFrom = theZombie->mPosX;
	theZombie->mPosX = u::GhostTeleportX(aMaxCol);
	theZombie->mX = static_cast<int>(std::lround(theZombie->mPosX));
	LogZombieEvent("t=%d f=%d roof ghost-teleport LADDER#%d row=%d lastFreePlantCol=%d x=%.1f->%.1f", BoardTime(theZombie), Frame(),
		Id(theZombie), theZombie->mRow, aMaxCol, aFrom, theZombie->mPosX);
}

int RoofBiteDamage(Zombie* theZombie, Plant* thePlant, int theVanilla) {
	if (!RoofZombiesOn() || !Is(theZombie, ZombieType::ZOMBIE_LADDER))
		return theVanilla;
	const int aDamage = u::GhostBiteDamage(theZombie->mPvzu.variant, thePlant->mPlantMaxHealth, thePlant->mPlantHealth, theVanilla);
	if (aDamage != theVanilla && aDamage >= thePlant->mPlantHealth) // one line per one-bite kill
		LogZombieEvent("t=%d f=%d roof ghost-one-bite LADDER#%d -> plant seed=%d col=%d row=%d hp=%d max=%d", BoardTime(theZombie),
			Frame(), Id(theZombie), static_cast<int>(thePlant->mSeedType), thePlant->mPlantCol, thePlant->mRow, thePlant->mPlantHealth,
			thePlant->mPlantMaxHealth);
	return aDamage;
}

void RoofTint(Zombie* theZombie, int& theAlpha) {
	if (RoofZombiesOn() && Is(theZombie, ZombieType::ZOMBIE_LADDER))
		theAlpha = std::min(theAlpha, u::GhostAlpha(theZombie->mPvzu.variant));
}

bool RoofSkipsShieldDamageImages(Zombie* theZombie) {
	return RoofZombiesOn() && Is(theZombie, ZombieType::ZOMBIE_LADDER) && u::SkipsShieldDamageImages(theZombie->mPvzu.variant);
}

// ---- Catapult ----

bool RoofCatapultPrefers(Zombie* theZombie, int theCandidateCol, int theCurrentCol) {
	if (!RoofZombiesOn())
		return theCandidateCol < theCurrentCol;
	return u::CatapultPrefers(theZombie->mPvzu.variant, theCandidateCol, theCurrentCol);
}

int RoofCatapultFaith(Zombie* theZombie, int theDamage) { // hook 705 -> 0xc00000
	if (!RoofZombiesOn() || !Is(theZombie, ZombieType::ZOMBIE_CATAPULT))
		return theDamage;
	const u::FaithHit aHit = u::CatapultFaith(theZombie->mBodyHealth, theDamage, theZombie->mSummonCounter,
		theZombie->mApp->IsWallnutBowlingLevel());
	if (aHit.bodyHp != theZombie->mBodyHealth || aHit.damage != theDamage)
		LogZombieEvent("t=%d f=%d roof faith-lock CATAPULT#%d form=%s balls=%d body=%d->%d damage=%d->%d", BoardTime(theZombie),
			Frame(), Id(theZombie), Name(theZombie), theZombie->mSummonCounter, theZombie->mBodyHealth, aHit.bodyHp, theDamage, aHit.damage);
	theZombie->mBodyHealth = aHit.bodyHp;
	return aHit.damage;
}

bool RoofCatapultTooTough(Zombie* theZombie) { return RoofCatapultLocked(theZombie); }

bool RoofCatapultLocked(const Zombie* theZombie) {
	return RoofZombiesOn() && Is(theZombie, ZombieType::ZOMBIE_CATAPULT) && u::CatapultTooToughToAsh(theZombie->mSummonCounter);
}

// ---- Gargantuar / Giga ----

bool RoofGargThrowsAnywhere() { return RoofZombiesOn() && u::GargThrowsAtAnyDistance(); }

float RoofGargThrowDistance(float theComputed, float theMinimum) {
	return RoofZombiesOn() ? u::GargThrowDistance(theComputed, theMinimum) : theComputed;
}

Zombie* RoofGargAddThrown(Zombie* theGarg, bool theHeldImp) { // hooks 565 / 566
	Board* aBoard = theGarg->mBoard;
	if (!RoofZombiesOn() || !IsGarg(theGarg) || theGarg->mPvzu.variant != re::STAFF || u::StaffThrowRollRange() <= 0)
		return aBoard->AddZombie(ZombieType::ZOMBIE_IMP, theGarg->mFromWave);
	// PVZU V1 (0xc11500): the throw that still holds the Imp throws it; every later one is a plain spawn in the row
	// PickRowForNewZombie(ZOMBIE_IMP) picks, at the type's own start x
	const bool aV1 = ZombieGapsV1On() && u::StaffSummonsWithoutThrow(theGarg->mPvzu.variant, false);
	if (aV1 && theHeldImp)
		return aBoard->AddZombie(ZombieType::ZOMBIE_IMP, theGarg->mFromWave);
	int r = 0;
	do
		r = Rand(u::StaffThrowRollRange());
	while (!u::StaffThrowTypeAccepted(r));
	const ZombieType aType = static_cast<ZombieType>(r);
	const bool aGiga = Is(theGarg, ZombieType::ZOMBIE_REDEYE_GARGANTUAR);
	Zombie* aThrown = nullptr;
	const int aRow = aV1 ? aBoard->PickRowForNewZombie(ZombieType::ZOMBIE_IMP) : -1;
	if (aGiga) {
		EliteValueAllScope aAll; // exe 0x800258 = 2 around the call (0xc1152c), for every world's direct readers
		aThrown = aV1 ? aBoard->AddZombieInRow(aType, aRow, theGarg->mFromWave) : aBoard->AddZombie(aType, theGarg->mFromWave);
	} else {
		aThrown = aV1 ? aBoard->AddZombieInRow(aType, aRow, theGarg->mFromWave) : aBoard->AddZombie(aType, theGarg->mFromWave);
	}
	LogZombieEvent("t=%d f=%d roof staff-throws %s#%d -> %s#%d forcedElite=%d boardZombies=%u", BoardTime(theGarg), Frame(),
		TypeName(theGarg->mZombieType), Id(theGarg), TypeName(aType), Id(aThrown), aGiga ? 1 : 0, aBoard->mZombies.mSize);
	return aThrown;
}

bool RoofThrownLandsAtOnce(Zombie* theGarg, Zombie* theThrown, bool theHeldImp) {
	if (RoofZombiesOn() && theThrown && IsGarg(theGarg) && ZombieGapsV1On() && // PVZU V1: a summon is not thrown (0xc114bc)
		u::StaffSummonsWithoutThrow(theGarg->mPvzu.variant, theHeldImp))
		return true;
	if (!RoofZombiesOn() || theThrown == nullptr || Is(theThrown, ZombieType::ZOMBIE_IMP))
		return false;
	if (Is(theThrown, ZombieType::ZOMBIE_BUNGEE)) // Ruling: a thrown Bungee keeps its own sky target
		return true;
	// where the Imp would land: start mPosX - 133, throw distance = the minimum (40 lawn, -140 roof)
	Board* aBoard = theGarg->mBoard;
	float aDistance = theGarg->mPosX - 360.0f;
	float aMinimum = 40.0f;
	if (aBoard->StageHasRoof()) {
		aDistance -= 180.0f;
		aMinimum = -140.0f;
	}
	aDistance = std::max(RoofGargThrowDistance(aDistance, aMinimum), aMinimum);
	theThrown->SetRow(theGarg->mRow);
	// the Imp's flight (UpdateZombieImp), stepped here without drawing: velX 3 (-3 hypnotised), gravity 0.05,
	// altitude 88, the roof slope folded into the altitude as aDiffY
	const float aVelX = theGarg->mMindControlled ? -3.0f : 3.0f;
	constexpr float kGravity = 0.05f; // THOWN_ZOMBIE_GRAVITY
	theThrown->mPosX = theGarg->mMindControlled ? theGarg->mPosX + theGarg->mWidth : theGarg->mPosX - 133.0f;
	theThrown->mPosY = theThrown->GetPosYBasedOnRow(theGarg->mRow);
	float aAltitude = 88.0f;
	float aVelZ = 0.5f * (aDistance / aVelX) * kGravity;
	for (int aStep = 0; aStep < 2000 && aAltitude > 0.0f; ++aStep) {
		aVelZ -= kGravity;
		aAltitude += aVelZ;
		theThrown->mPosX -= aVelX;
		const float aDiffY = theThrown->GetPosYBasedOnRow(theGarg->mRow) - theThrown->mPosY;
		theThrown->mPosY += aDiffY;
		aAltitude += aDiffY;
	}
	theThrown->mX = static_cast<int>(theThrown->mPosX);
	theThrown->mRenderOrder = theGarg->mRenderOrder + 1;
	theThrown->mChilledCounter = theGarg->mChilledCounter;
	if (theGarg->mMindControlled) // as the hypnotised Gargantuar's Imp (DO_FIX_BUGS block)
		theThrown->StartMindControlled();
	LogZombieEvent("t=%d f=%d roof thrown-lands %s#%d row=%d x=%.1f", BoardTime(theGarg), Frame(), TypeName(theThrown->mZombieType),
		Id(theThrown), theThrown->mRow, theThrown->mPosX);
	return true;
}

ZombiePhase RoofGargAfterThrowPhase(Zombie* theGarg, ZombiePhase theVanilla) { // hook 568 -> 0xc11700
	if (!RoofZombiesOn() || !IsGarg(theGarg) || theGarg->mPvzu.variant != re::STAFF ||
		!u::StaffKeepsThrowing(static_cast<int>(theGarg->mBoard->mZombies.mSize), theGarg->mMindControlled))
		return theVanilla;
	return ZombiePhase::PHASE_GARGANTUAR_THROWING;
}

// ---- thrown Imp ----

bool RoofThrownImpSkipsHouse(Zombie* theZombie) {
	return RoofZombiesOn() && Is(theZombie, ZombieType::ZOMBIE_IMP) && u::ThrownImpSkipsHouse(theZombie->mZombiePhase);
}

} // namespace pvzu::engine
