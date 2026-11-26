// PVZU: engine side of the Unnamed world-7 zombies and the ZomBotany zombies in Unnamed. Rules:
// Mod/Unnamed/World7Zombies.h; evidence: the research notes (w6-w7-and-new-content) sections 2.4, 4 and
// pvz-unnamed-pack/SOURCES-world7-zombies.md.
#include "Mod/Engine/World7EliteZombies.h"
#include "Mod/Engine/CatapultShots.h"
#include "Mod/Engine/EliteGaps.h"
#include "Mod/Engine/OptionsPanel.h"

#include "Mod/Engine/EliteZombies.h"
#include "Mod/Engine/EngineTables.h"
#include "Mod/GameData.h"
#include "Mod/Unnamed/ModPlantRules.h"
#include "Mod/Unnamed/ModPlantShadow.h"
#include "Mod/Unnamed/ModShots.h"
#include "Mod/Unnamed/NightZombies.h"
#include "Mod/Unnamed/World7Zombies.h"

#include "LawnApp.h"
#include "Resources.h"
#include "Lawn/Board.h"
#include "Lawn/Plant.h"
#include "Lawn/Projectile.h"
#include "Lawn/Zombie.h"
#include "Lawn/System/PlayerInfo.h"
#include "PvzpLib/PvzpCommon.h"
#include "PvzpLib/Reanimator.h"
#include "SexyAppFramework/graphics/SharedImage.h"
#include "graphics/Color.h"

#include <algorithm>
#include <cstdio>
#include <map>

using namespace Sexy;
namespace u = pvzu::unnamed;

namespace pvzu::engine {

namespace {

int gGoldFootballDepth = 0;   // > 0 while the silver coin adds its world-7 Football copy (exe [0x9dd48e] = 2)

constexpr int kSquashTargetZombie = 1;   // mPvzu.extra of a Squash head in the jump: exe +0xf8 (1 zombie, 2 plant)
constexpr int kSquashTargetPlant = 2;

int BoardTime(const Zombie* z) { return z->mBoard ? static_cast<int>(z->mBoard->mMainCounter) : 0; }
int Id(Zombie* z) { return z && z->mBoard ? static_cast<int>(z->mBoard->ZombieGetID(z)) & 0xFFFF : -1; }
int ProfileLevel(LawnApp* a) { return a && a->mPlayerInfo ? a->mPlayerInfo->mLevel : 0; }
int BoardLevel(LawnApp* a) { return a && a->mBoard ? a->mBoard->mLevel : 0; }
const char* TypeName(ZombieType t) {
	RegisterVanillaTables();
	const auto& zombies = GameData::Get().Vanilla().zombies;
	return t >= 0 && static_cast<size_t>(t) < zombies.size() ? zombies[t].name.c_str() : "?";
}
bool Alive(Zombie* z) { return z && !z->mDead && !z->IsDeadOrDying(); }

Image* W7Image(const std::string& theFile) {
	static const std::map<std::string, Image**> kResources = {
		{"images/bug_spray", &IMAGE_BUG_SPRAY},
	};
	if (auto it = kResources.find(theFile); it != kResources.end() && *it->second != nullptr)
		return *it->second;
	static std::map<std::string, SharedImageRef> loaded; // kept alive for the whole run
	auto it = loaded.find(theFile);
	if (it == loaded.end()) {
		it = loaded.emplace(theFile, gSexyAppBase->GetSharedImage(theFile)).first;
		if (static_cast<Image*>(it->second) == nullptr)
			std::fprintf(stderr, "PVZU: world-7 image %s not found, drawing the base zombie\n", theFile.c_str());
	}
	return static_cast<Image*>(it->second);
}

// all images or none (the form keeps its behaviour with the base look)
void ApplyArt(Zombie* theZombie, const std::vector<u::W7ArtImage>& theArt) {
	Reanimation* aBody = theZombie->mApp->ReanimationTryToGet(theZombie->mBodyReanimID);
	if (aBody == nullptr)
		return;
	for (const auto& img : theArt)
		if (W7Image(img.file) == nullptr)
			return;
	for (const auto& img : theArt)
		aBody->SetImageOverride(img.track.c_str(), W7Image(img.file));
}

void LogW7(const char* theWhat, Zombie* z) {
	LogZombieEvent("t=%d world7 %s %s#%d variant=0x%x sub=0x%x row=%d x=%.1f body=%d/%d helm=%d shield=%d profile=%d board=%d",
		BoardTime(z), theWhat, TypeName(z->mZombieType), Id(z), z->mPvzu.variant, z->mPvzu.subVariant, z->mRow, z->mPosX,
		z->mBodyHealth, z->mBodyMaxHealth, z->mHelmHealth, z->mShieldHealth, ProfileLevel(z->mApp), BoardLevel(z->mApp));
}

void GiveFlagBucket(Zombie* z) { // 0x7a1d8f..0x7a1dc2: only when the body reanim exists
	if (z == nullptr || z->mApp->ReanimationTryToGet(z->mBodyReanimID) == nullptr)
		return;
	z->mHelmType = HelmType::HELMTYPE_PAIL;
	z->mHelmHealth = z->mHelmMaxHealth = u::FlagBucketHp();
	z->ReanimShowPrefix("anim_bucket", RENDER_GROUP_NORMAL);
}

bool BossLevel(Zombie* z) { return z->mBoard && z->mBoard->mLevel == 70; }   // exe 0x7aa100: board level 0x46

} // namespace

bool World7ZombiesOn() { return GameData::Get().Feature("world7Zombies"); }

bool InWorld7(LawnApp* theApp) {
	return World7ZombiesOn() && theApp && theApp->mPlayerInfo &&
		u::IsWorld7(theApp->mGameMode == GameMode::GAMEMODE_ADVENTURE, ProfileLevel(theApp), BoardLevel(theApp));
}

bool ProfileAbove60(LawnApp* theApp) {
	return World7ZombiesOn() && theApp && theApp->mPlayerInfo && u::PlayerLevelAbove60(ProfileLevel(theApp), BoardLevel(theApp));
}

const char* World7FormName(Zombie* theZombie) {
	if (!World7ZombiesOn())
		return nullptr;
	return u::World7FormName(theZombie->mZombieType, theZombie->mPvzu.variant, theZombie->mPvzu.subVariant);
}

// ---- ZomBotany ----

void ZomBotanyInit(Zombie* theZombie) {
	const ZombieType aType = theZombie->mZombieType;
	if (!World7ZombiesOn() || !u::IsZomBotanyType(aType))
		return;
	const u::ZomBotanyArmour a = u::ZomBotanyStats(aType, theZombie->mBodyHealth, theZombie->mHelmType, theZombie->mHelmHealth);
	theZombie->mBodyHealth = a.bodyHp;
	theZombie->mHelmType = a.helm;
	theZombie->mHelmHealth = a.helmHp;
	if (a.helmPrefix)
		theZombie->ReanimShowPrefix(a.helmPrefix, RENDER_GROUP_NORMAL);
	if (a.doorHp > 0 && theZombie->mApp->ReanimationTryToGet(theZombie->mBodyReanimID)) { // hooks 509 / 511
		theZombie->mShieldType = ShieldType::SHIELDTYPE_DOOR;
		theZombie->mShieldHealth = a.doorHp;
		theZombie->AttachShield();
	}
}

void World7EliteInit(Zombie* theZombie, bool theOnBoard) {
	const ZombieType aType = theZombie->mZombieType;
	if (!World7ZombiesOn() || !u::IsZomBotanyType(aType))
		return;
	// hook 508 (0xc033a0): Rand(12) == 0 -> 0x101, every mode. Rolled here, after the Day EliteInit wrote the variant
	// (it writes 0 for every type it does not roll); nothing draws Rand in between for this type.
	if (aType == ZombieType::ZOMBIE_TALLNUT_HEAD) {
		int aForced = 0;
		if (ForcedVariantFor(aType, aForced))
			theZombie->mPvzu.variant = aForced;
		// The elite option: t = 2 - option (Rand(12) only at the default); Almanac/preview zombies are made with
		// option 1 (hook 415, 0xc10880), so they never roll.
		else if (const u::OptionRoll r = u::TallnutOddRoll(theOnBoard ? EliteValue() : u::eliteoption::NONE, u::TallnutOddRollRange()); r.range > 0)
			theZombie->mPvzu.variant = u::RollTallnutOdd(Rand(r.range));
		else if (!theOnBoard || EliteValue() != u::eliteoption::DEFAULT) // PVZU elite-all: 0xc033a8 reads the value
			theZombie->mPvzu.variant = u::RollTallnutOdd(r.fixedRoll);
	}
	if (theOnBoard && theZombie->mBoard)
		LogW7("zombotany", theZombie);
}

void World7AfterLoad(Zombie* theZombie) {
	if (!World7ZombiesOn() || theZombie->mDead)
		return;
	if (theZombie->mZombieType == ZombieType::ZOMBIE_CATAPULT && theZombie->mPvzu.variant == u::w7::VASE_CATAPULT)
		ApplyArt(theZombie, u::VaseCatapultArt());
	else if (theZombie->mZombieType == ZombieType::ZOMBIE_FOOTBALL && theZombie->mPvzu.subVariant == u::w7::BLACK_FOOTBALL)
		ApplyArt(theZombie, u::BlackFootballArt());
	else
		return;
	LogW7("load", theZombie);
}

int World7WorldNumber(Zombie* theZombie, int theBoardWorldNumber) {
	if (!World7ZombiesOn())
		return theBoardWorldNumber;
	return u::RoadSignWorldNumber(InWorld7(theZombie->mApp));   // hook 491: 0x7aa4c0, not the board level
}

// ---- Flag (hook 91) ----

void World7FlagWave(Board* theBoard, Zombie* theZombie) {
	if (theZombie == nullptr || theZombie->mZombieType != ZombieType::ZOMBIE_FLAG || !InWorld7(theBoard->mApp) ||
		!u::FlagFillsRows())
		return;
	const bool aSixRows = theBoard->mBackground == BackgroundType::BACKGROUND_3_POOL ||
		theBoard->mBackground == BackgroundType::BACKGROUND_4_FOG;
	const std::vector<int> aRows = u::FlagExtraRows(theZombie->mRow, aSixRows);
	const int aTop = std::max(aRows.empty() ? 0 : aRows.front(), theZombie->mRow);
	int aAdded = 0;
	for (int aRow = aTop; aRow >= 0; --aRow) {
		Zombie* z = theZombie;
		if (aRow != theZombie->mRow) {
			theBoard->PickRowForNewZombie(ZombieType::ZOMBIE_FLAG);   // 0x7a1d76: drawn and dropped, as the exe
			z = theBoard->AddZombieInRow(ZombieType::ZOMBIE_FLAG, aRow, theBoard->mCurrentWave);
			aAdded += z ? 1 : 0;
		}
		GiveFlagBucket(z);
		if (z)
			LogW7(z == theZombie ? "flag-bucket" : "flag-row", z);
	}
	LogZombieEvent("t=%d world7 flag-wave wave=%d flagRow=%d added=%d", static_cast<int>(theBoard->mMainCounter),
		theBoard->mCurrentWave, theZombie->mRow, aAdded);
}

// ---- Imp (hook 571) ----

void World7ImpLanded(Zombie* theZombie) {
	if (theZombie->mZombieType != ZombieType::ZOMBIE_IMP || !InWorld7(theZombie->mApp))
		return;
	const int n = u::ImpGargRollRange(EliteOption()); // PVZU: the setting patches 0x7a1f53 (0x7b2100 none, 0x7b2000 all)
	if (n <= 0)
		return;
	if (!u::ImpBecomesGarg(Rand(n), EliteOption())) {
		LogW7("imp-landed", theZombie);
		return;
	}
	theZombie->mZombieType = ZombieType::ZOMBIE_GARGANTUAR;   // keeps the Imp's reanim, attacks like a Gargantuar
	theZombie->mPvzu.variant = u::w7::IMP_GARGANTUAR;
	theZombie->mBodyMaxHealth += u::ImpGargHpBonus();
	theZombie->mBodyHealth += u::ImpGargHpBonus();
	LogW7("imp-gargantuar", theZombie);
}

// ---- Jack (hook 707) ----

bool World7JackPopsOnHit(Zombie* theZombie) {
	if (theZombie->mZombieType != ZombieType::ZOMBIE_JACK_IN_THE_BOX || !World7ZombiesOn())
		return false;
	if (!u::JackPopsWhenHit(InWorld7(theZombie->mApp), theZombie->mZombieType, theZombie->mPvzu.variant, theZombie->mBodyHealth))
		return false;
	theZombie->mPhaseCounter = 1;   // `+0x68 = 1`: UpdateZombieJackInTheBox pops it on its next tick
	LogW7("jack-pops", theZombie);
	return true;
}

// ---- Football ----

int World7FootballExtraRoll(Zombie* theZombie, int theSubVariant, int theVariant, int theRoll) {
	if (!World7ZombiesOn())
		return theSubVariant;
	const bool aGreen = theSubVariant == u::nightsub::GREEN;
	const bool aGold = theVariant == u::nightelite::GOLD_FOOTBALL;
	return u::FootballBecomesBlack(theRoll, aGreen, aGold, ProfileLevel(theZombie->mApp)) ? u::w7::BLACK_FOOTBALL : theSubVariant;
}

bool World7ProfileAbove60(Zombie* theZombie) { return u::PlayerLevelAbove60(ProfileLevel(theZombie->mApp)); }

int World7AllEliteFootballBlack(int theSubVariant, bool theBlack) {
	return theBlack && World7ZombiesOn() ? u::w7::BLACK_FOOTBALL : theSubVariant;
}

bool World7ForcedGoldFootball() { return gGoldFootballDepth > 0 && World7ZombiesOn() && u::CoinFootballIsGold(); }

World7GoldFootballScope::World7GoldFootballScope() { ++gGoldFootballDepth; }
World7GoldFootballScope::~World7GoldFootballScope() { --gGoldFootballDepth; }

void World7FootballLook(Zombie* theZombie) {
	if (!World7ZombiesOn() || theZombie->mZombieType != ZombieType::ZOMBIE_FOOTBALL)
		return;
	if (theZombie->mPvzu.subVariant != u::w7::BLACK_FOOTBALL) {
		if (theZombie->mPvzu.variant == u::nightelite::GOLD_FOOTBALL && gGoldFootballDepth > 0 && theZombie->IsOnBoard())
			LogW7("gold-copy", theZombie);
		return;
	}
	if (theZombie->mPvzu.variant == u::nightelite::GOLD_FOOTBALL) { // PVZU elite-all: 0x9dd423, gold and black
		theZombie->mBodyHealth = u::GoldBlackFootballBodyHp(theZombie->mBodyHealth);
		theZombie->mHelmHealth = u::GoldBlackFootballHelmHp(theZombie->mHelmHealth);
	} else {
		theZombie->mBodyHealth = u::BlackFootballBodyHp(theZombie->mBodyHealth);   // 0x9dd460
		theZombie->mHelmHealth = u::BlackFootballHelmHp(theZombie->mHelmHealth);
	}
	ApplyArt(theZombie, u::BlackFootballArt());
	if (theZombie->IsOnBoard())
		LogW7("black-football", theZombie);
}

Image* World7HelmDamageImage(Zombie* theZombie, int theDamageIndex) {
	if (!World7ZombiesOn() || theZombie->mZombieType != ZombieType::ZOMBIE_FOOTBALL ||
		theZombie->mPvzu.subVariant != u::w7::BLACK_FOOTBALL)
		return nullptr;
	const std::string f = u::BlackHelmImage(theDamageIndex);
	return f.empty() ? nullptr : W7Image(f);
}

// ---- Catapult ----

int World7CatapultForm(Zombie* theZombie, int theRoll, int theEliteSetting) {
	return World7ZombiesOn() ? u::CatapultWorld7Form(theRoll, ProfileLevel(theZombie->mApp), theEliteSetting) : 0;
}

void World7CatapultInit(Zombie* theZombie) {
	if (!World7ZombiesOn() || theZombie->mZombieType != ZombieType::ZOMBIE_CATAPULT ||
		theZombie->mPvzu.variant != u::w7::VASE_CATAPULT)
		return;
	theZombie->mSummonCounter = u::VaseCatapultBalls(theZombie->mSummonCounter);   // +0x114 = 10
	ApplyArt(theZombie, u::VaseCatapultArt());
	LogZombieEvent("t=%d world7 vase-catapult CATAPULT#%d balls=%d row=%d", BoardTime(theZombie), Id(theZombie),
		theZombie->mSummonCounter, theZombie->mRow);
}

void World7CatapultShot(Zombie* theZombie, Projectile* theShot) { // hook 537 (0x7a1940), right after the projectile is made
	if (!World7ZombiesOn())
		return;
	if (CatapultVolleyShot(theShot)) // PVZU Plan 9 R3: [0x701240] == 1 -> Rand(3) type, no per-shot roll
		return;
	const int v = theZombie->mPvzu.variant;
	const int n = u::CatapultShotRollRange(v, InWorld7(theZombie->mApp) || BossLevel(theZombie));
	if (n <= 0)
		return;
	const u::CatapultShot s = u::RollCatapultShot(v, CatapultDevShotRoll(v, Rand(n)), theZombie->mSummonCounter);
	if (s.exeProjectile == 9)
		return;
	theZombie->mSummonCounter = s.balls;
	LogZombieEvent("t=%d world7 catapult-shot CATAPULT#%d form=0x%x exeProjectile=0x%x balls=%d%s",
		BoardTime(theZombie), Id(theZombie), v, s.exeProjectile, s.balls,
		CatapultShotsOn() ? "" : " (shot stays a basketball)");
	CatapultTagShot(theZombie, theShot, s.exeProjectile); // PVZU Plan 9 R3: the shot becomes the vase / bug spray
}

// ---- Tree of Wisdom (hook 268) ----

int World7PlantHp(Plant* thePlant, int theHp) {
	LawnApp* anApp = thePlant->mApp;
	if (!World7ZombiesOn() || anApp == nullptr || anApp->mBoard == nullptr || anApp->mPlayerInfo == nullptr || !TreePowerOn()) // PVZU: HARDWS
		return theHp;
	const PlayerInfo* p = anApp->mPlayerInfo;
	const int aHp = u::TreeBuffedPlantHp(theHp, anApp->mGameMode == GameMode::GAMEMODE_ADVENTURE,
		static_cast<int>(p->mPurchases[StoreItem::STORE_ITEM_TREE_OF_WISDOM] & 0xFF), static_cast<int>(p->mFinishedAdventure),
		p->mLevel, static_cast<int>(p->mChallengeRecords[GameMode::GAMEMODE_TREE_OF_WISDOM - 1]));
	if (aHp != theHp)
		LogZombieEvent("t=%d world7 tree-hp seed=%d col=%d row=%d hp=%d->%d tree=%u", static_cast<int>(anApp->mBoard->mMainCounter),
			static_cast<int>(thePlant->mSeedType), thePlant->mPlantCol, thePlant->mRow, theHp, aHp,
			p->mChallengeRecords[GameMode::GAMEMODE_TREE_OF_WISDOM - 1]);
	return aHp;
}

// ---- ZomBotany behaviour ----

void HeadPeaFired(Zombie* theZombie, Projectile* theProjectile, bool theGatling) {
	if (!World7ZombiesOn() || theProjectile == nullptr)
		return;
	Board* aBoard = theZombie->mBoard;
	const bool aHypno = theZombie->mMindControlled;
	if (aHypno && u::HypnotisedHeadsFirePlayerPeas()) { // hooks 572 (0xc03f19) / 576 (0x795c91)
		theProjectile->mProjectileType = ProjectileType::PROJECTILE_PEA;
		theProjectile->mMotionType = ProjectileMotion::MOTION_STRAIGHT;
		theProjectile->mDamageRangeFlags = 1;
		if (theGatling) {
			theProjectile->mPosX = static_cast<float>(theProjectile->mX) + 94.1195297f + 5.79718065f;
			theProjectile->mX += 100;
			theProjectile->mPosY = static_cast<float>(theProjectile->mY) + 8.0f;
		}
	}
	if (!theGatling || !HeadPeasMaySpread(theZombie)) // PVZU Plan 9 R4: a Gatling-head Football never spreads (0xc12f00)
		return;
	const int n = u::GatlingSpreadRollRange(); // hook 575: 0x795ce6 / 0x795d88, per pea
	if (n <= 0 || !u::GatlingSpreads(Rand(n)))
		return;
	for (float aSign : {1.0f, -1.0f}) {
		Projectile* p = aBoard->AddProjectile(theProjectile->mX, theProjectile->mY, theZombie->mRenderOrder - 1, theZombie->mRow,
			aHypno ? ProjectileType::PROJECTILE_PEA : ProjectileType::PROJECTILE_ZOMBIE_PEA);
		p->mMotionType = ProjectileMotion::MOTION_STAR;
		p->mVelX = aHypno ? u::GatlingSpreadVx() : -u::GatlingSpreadVx();
		p->mVelY = aSign * u::GatlingSpreadVy();
		if (aHypno)
			p->mDamageRangeFlags = 1;
	}
	LogZombieEvent("t=%d world7 gatling-spread GATLING_HEAD#%d row=%d x=%d hypnotised=%d", BoardTime(theZombie), Id(theZombie),
		theZombie->mRow, theProjectile->mX, aHypno ? 1 : 0);
}

// ---- Plan 9 R5: Peashooter Zombie fire-rate ramp (0x7958fe) ----
int PeaHeadNextShot(Zombie* theZombie, int theVanilla) {
	if (theZombie == nullptr || theZombie->mZombieType != ZombieType::ZOMBIE_PEA_HEAD ||
		!GameData::Get().Feature("peaHeadRamp"))
		return theVanilla;
	const int n = u::PeaHeadRampRollRange();
	if (n <= 0)
		return theVanilla;
	const int aRoll = Rand(n); // drawn every shot, as the exe does
	const int aBefore = theZombie->mPvzu.stored;
	theZombie->mPvzu.stored = u::PeaHeadRampAfterShot(aBefore, aRoll);
	const int aNext = u::PeaHeadShotInterval(theZombie->mPvzu.stored, theVanilla);
	LogZombieEvent("t=%d world7 pea-ramp PEA_HEAD#%d row=%d roll=%d raises=%d->%d next=%d hypnotised=%d", BoardTime(theZombie),
		Id(theZombie), theZombie->mRow, aRoll, aBefore, theZombie->mPvzu.stored, aNext, theZombie->mMindControlled ? 1 : 0);
	return aNext;
}

bool World7JalapenoBurn(Zombie* theZombie) { // hook 573 (0x958130)
	if (!World7ZombiesOn() || !u::JalapenoBurnsHypnotised())
		return false;
	Board* aBoard = theZombie->mBoard;
	const int aRow = theZombie->mRow;
	if (theZombie->mMindControlled) { // Plant::BurnRow with a Jalapeno, the row's ice melts; no plant is burned
		theZombie->BurnRow(aRow);
		aBoard->mIceTimer[aRow] = u::JalapenoIceTimer();
		LogW7("jalapeno-hypnotised-burn", theZombie);
		return true;
	}
	int aBurned = 0; // 0x95817e: the hypnotised zombies of its row and the Zomboss ice ball in its row, then the plants
	for (Zombie* z : aBoard->mZombies) {
		if (z->mDead || z == theZombie)
			continue;
		if (z->mZombieType == ZombieType::ZOMBIE_BOSS && z->mBossFireBallReanimID != ReanimationID::REANIMATIONID_NULL &&
			z->mMindControlled != theZombie->mMindControlled && z->mFireballRow == aRow)
			z->BossDestroyIceballInRow();
		if ((z->mZombieType == ZombieType::ZOMBIE_BOSS || z->mRow == aRow) && z->EffectedByDamage(0xFF)) {
			z->ApplyBurn();
			++aBurned;
		}
	}
	LogZombieEvent("t=%d world7 jalapeno-burn JALAPENO_HEAD#%d row=%d hypnotisedBurned=%d", BoardTime(theZombie), Id(theZombie),
		aRow, aBurned);
	return false;
}

bool World7SquashWantsJump(Zombie* theZombie) { // hook 577 (0x958200)
	if (!World7ZombiesOn() || !u::SquashJumpsOnReach() || theZombie->mZombieType != ZombieType::ZOMBIE_SQUASH_HEAD)
		return false;
	Board* aBoard = theZombie->mBoard;
	if (Zombie* z = theZombie->FindZombieTarget(); z && z->EffectedByDamage(theZombie->mMindControlled ? 1U : 0x81U)) {
		theZombie->mPvzu.stored = static_cast<int>(aBoard->ZombieGetID(z));
		theZombie->mPvzu.extra = kSquashTargetZombie;
	} else if (Plant* p = theZombie->mMindControlled ? nullptr : theZombie->FindPlantTarget(ZombieAttackType::ATTACKTYPE_CHEW)) {
		theZombie->mPvzu.stored = static_cast<int>(aBoard->mPlants.DataArrayGetID(p));
		theZombie->mPvzu.extra = kSquashTargetPlant;
	} else {
		return false;
	}
	LogZombieEvent("t=%d world7 squash-jump SQUASH_HEAD#%d row=%d target=%s", BoardTime(theZombie), Id(theZombie), theZombie->mRow,
		theZombie->mPvzu.extra == kSquashTargetPlant ? "plant" : "zombie");
	return true;
}

int World7SquashDestX(Zombie* theZombie, int theVanillaDestX) { // hook 582 (0x9582f0): follows the target while it lives
	if (!World7ZombiesOn() || theZombie->mZombieType != ZombieType::ZOMBIE_SQUASH_HEAD || theZombie->mPvzu.extra == 0)
		return theVanillaDestX;
	Board* aBoard = theZombie->mBoard;
	if (theZombie->mPvzu.extra == kSquashTargetPlant) {
		if (Plant* p = aBoard->mPlants.DataArrayTryToGet(static_cast<unsigned int>(theZombie->mPvzu.stored)); p && !p->mDead)
			return p->mX;
	} else if (Zombie* z = aBoard->ZombieTryToGet(static_cast<ZombieID>(theZombie->mPvzu.stored)); Alive(z)) {
		return z->GetZombieRect().mX;
	}
	theZombie->mPvzu.extra = 0;   // 0x958381: target gone, the vanilla cell from now on
	theZombie->mPvzu.stored = 0;
	return theVanillaDestX;
}

int World7SquashLanded(Zombie* theZombie, int theDestX, int theVanillaCol) { // hook 584 (0x958460)
	if (!World7ZombiesOn() || theZombie->mZombieType != ZombieType::ZOMBIE_SQUASH_HEAD || u::SquashLandingZombieDamage() <= 0)
		return theVanillaCol;
	const int aCol = theZombie->mPvzu.extra == kSquashTargetPlant
		? theZombie->mBoard->PixelToGridXKeepOnBoard(theDestX, static_cast<int>(theZombie->mPosY)) : theVanillaCol;
	const Rect aRect(theDestX - 10, static_cast<int>(theZombie->mPosY), 100, 100);
	const unsigned int aFlags = theZombie->mMindControlled ? 1U : 0x81U;
	int aHit = 0;
	for (Zombie* z : theZombie->mBoard->mZombies) {
		if (z->mDead || z == theZombie || (z->mRow != theZombie->mRow && z->mZombieType != ZombieType::ZOMBIE_BOSS))
			continue;
		if (!z->EffectedByDamage(aFlags) || GetRectOverlap(aRect, z->GetZombieRect()) < 0)
			continue;
		z->TakeDamage(u::SquashLandingZombieDamage(), 0U);
		++aHit;
	}
	theZombie->mPvzu.extra = 0;
	theZombie->mPvzu.stored = 0;
	LogZombieEvent("t=%d world7 squash-lands SQUASH_HEAD#%d row=%d x=%d col=%d zombiesHit=%d", BoardTime(theZombie), Id(theZombie),
		theZombie->mRow, theDestX, aCol, aHit);
	return aCol;
}

// ---- Wall-nut Zombie self-copy (Plan 9 R6; exe 0x7971ea..0x797297) ----

void WallnutZombieCopyTick(Zombie* theZombie) {
	if (theZombie->mZombieType != ZombieType::ZOMBIE_WALLNUT_HEAD || theZombie->mBoard == nullptr || // 0xc10cd0: board
	    !GameData::Get().Feature("wallnutZombieCopy"))
		return;
	u::WallnutCopyView v;
	v.type = theZombie->mZombieType;
	v.helm = theZombie->mHelmType;
	v.fromWave = theZombie->mFromWave;
	v.x = theZombie->mX;
	v.dead = theZombie->mDead;
	v.hasHead = theZombie->mHasHead;
	v.hypnotised = theZombie->mMindControlled;
	if (!u::WallnutCopyEligible(v)) // a skipped tick changes neither field (all tests jump to 0x797302)
		return;
	// the exe's own fields: +0x12c (mBossMode) last helm HP, +0x114 (mSummonCounter) quiet ticks
	const u::WallnutCopyStep s = u::WallnutCopyTick(theZombie->mBossMode, theZombie->mSummonCounter, theZombie->mHelmHealth);
	theZombie->mBossMode = s.lastHelmHp;
	theZombie->mSummonCounter = s.quietTicks;
	if (!s.copy)
		return;
	Board* aBoard = theZombie->mBoard;
	if (aBoard->mZombies.mSize >= aBoard->mZombies.mMaxSize) // PVZU: the port never overfills the array (exe would abort)
		return;
	// 0x797265: IZombiePlaceZombie(0x1b, mX / 80, mRow) = AddZombieInRow(0x1b, row, 0) and an interim x that 0x797278
	// overwrites with the original's mPosX
	Zombie* aCopy = aBoard->AddZombieInRow(ZombieType::ZOMBIE_WALLNUT_HEAD, theZombie->mRow, u::WallnutCopyFromWave());
	if (aCopy == nullptr)
		return;
	aCopy->mPosX = theZombie->mPosX;
	const int aFreshHelm = aCopy->mHelmHealth;
	aCopy->TakeDamage(u::WallnutCopyDamage(aFreshHelm, theZombie->mHelmHealth), u::WallnutCopyDamageFlags()); // 0x797292
	LogZombieEvent("t=%d wallnut-copy WALLNUT_HEAD#%d -> #%d row=%d x=%.1f helm=%d copyHelm=%d->%d copyBody=%d wave=%d",
		BoardTime(theZombie), Id(theZombie), Id(aCopy), theZombie->mRow, theZombie->mPosX, theZombie->mHelmHealth, aFreshHelm,
		aCopy->mHelmHealth, aCopy->mBodyHealth, aCopy->mFromWave);
}

// ---- Almanac ----

int AlmanacZombieCount(int theVanilla) { return World7ZombiesOn() ? u::AlmanacZombieCount(theVanilla) : theVanilla; }

ZombieType AlmanacZombieType(int theIndex, ZombieType theVanilla) {
	const int n = AlmanacZombieCount(-1);
	return n > ZombieType::ZOMBIE_BOSS + 1 ? u::AlmanacZombieAt(theIndex, n) : theVanilla;
}

bool AlmanacZombieSlot(ZombieType theType, int& theX, int& theY, float& theScale) {
	const int n = AlmanacZombieCount(-1);
	if (n <= ZombieType::ZOMBIE_BOSS + 1)
		return false;
	for (int i = 0; i < n; ++i)
		if (u::AlmanacZombieAt(i, n) == theType) {
			const u::AlmanacSlot s = u::AlmanacZombieSlot(i, n);
			theX = s.x;
			theY = s.y;
			theScale = s.scale;
			return true;
		}
	return false;
}

float AlmanacWindowScale() {
	const int n = AlmanacZombieCount(-1);
	return n > ZombieType::ZOMBIE_BOSS + 1 ? u::AlmanacZombieSlot(0, n).scale : 1.0f;
}

bool AlmanacShowsType(ZombieType theType) {
	return theType <= ZombieType::ZOMBIE_BOSS || (AlmanacZombieCount(-1) > ZombieType::ZOMBIE_BOSS + 1 && theType < NUM_ZOMBIE_TYPES);
}

std::string AlmanacZombieNameKey(ZombieType theType, const char* theVanilla) {
	if (World7ZombiesOn())
		if (const char* k = u::ZomBotanyNameKey(theType))
			return k;
	return theVanilla;
}

// ---- Tall-nut Zombie (Plan 9 R7) ----

bool TallnutZombieOn() { return GameData::Get().Feature("tallnutZombie"); }

namespace {

void LogTallnut(const char* theWhat, Zombie* z, int theValue) {
	LogZombieEvent("t=%d tallnut %s TALLNUT_HEAD#%d variant=0x%x row=%d x=%.1f helm=%d/%d body=%d value=%d hypnotised=%d",
		BoardTime(z), theWhat, Id(z), z->mPvzu.variant, z->mRow, z->mPosX, z->mHelmHealth, z->mHelmMaxHealth, z->mBodyHealth,
		theValue, z->mMindControlled ? 1 : 0);
}

// 0xc03428..0xc0347f: AddZombieInRow(type, own row), mMindControlled copied raw, RiseFromGrave at PixelToGridX(mX),
// phase counter 50, one rise update
void TallnutOddSummon(Zombie* theZombie) {
	Board* aBoard = theZombie->mBoard;
	const int n = u::TallnutOddSummonRollRange();
	if (n <= 0)
		return;
	const ZombieType aType = u::TallnutOddSummonType(Rand(n));
	if (aType == ZombieType::ZOMBIE_INVALID)
		return;
	Zombie* aZombie = aBoard->AddZombieInRow(aType, theZombie->mRow, u::TallnutOddSummonFromWave());
	if (aZombie == nullptr)
		return;
	aZombie->mMindControlled = theZombie->mMindControlled;
	const int aCol = aBoard->PixelToGridX(theZombie->mX, theZombie->mY);
	if (aZombie->mZombiePhase == ZombiePhase::PHASE_ZOMBIE_NORMAL) { // RiseFromGrave's own precondition
		aZombie->RiseFromGrave(aCol, theZombie->mRow);
		aZombie->mPhaseCounter = u::TallnutOddRiseCounterCs();
		aZombie->UpdateZombieRiseFromGrave();
	}
	LogZombieEvent("t=%d tallnut odd-summon TALLNUT_HEAD#%d -> %s#%d row=%d col=%d hypnotised=%d", BoardTime(theZombie),
		Id(theZombie), TypeName(aType), Id(aZombie), theZombie->mRow, aCol, aZombie->mMindControlled ? 1 : 0);
}

} // namespace

void TallnutZombieTick(Zombie* theZombie) {
	if (theZombie->mZombieType != ZombieType::ZOMBIE_TALLNUT_HEAD || theZombie->mBoard == nullptr || !TallnutZombieOn())
		return;
	if (u::TallnutOddSummonsNow(theZombie->mPvzu.variant, theZombie->mApp->mGameScene == GameScenes::SCENE_PLAYING,
			theZombie->mZombieAge))
		TallnutOddSummon(theZombie);
	const u::TallnutRegen r = u::TallnutZombieRegenTick(static_cast<int>(theZombie->mHelmType), theZombie->mHelmHealth,
		theZombie->mSummonCounter, theZombie->mPvzu.stored);
	theZombie->mHelmHealth = r.helmHp;
	theZombie->mSummonCounter = r.counter;      // exe +0x114
	theZombie->mPvzu.stored = r.recordedHp;     // exe +0x12c
	if (r.healed > 0)
		LogTallnut("regen", theZombie, r.healed);
}

void TallnutZombieTint(const Zombie* theZombie, Color& theColor, Color& theAdditive, bool& theEnableAdditive) {
	if (theZombie->mZombieType != ZombieType::ZOMBIE_TALLNUT_HEAD || theZombie->mPvzu.variant != u::w7::TALLNUT_ODD ||
		!TallnutZombieOn())
		return;
	const int anAlpha = u::TallnutOddAlpha();
	if (anAlpha < 0 || theZombie->mZombiePhase == ZombiePhase::PHASE_ZOMBIE_BURNED) // burned black comes first
		return;
	// The chain checks Repeater poison and weakened grey first (0xc10d30, 0x95a365): those keep the fade alpha.
	const u::ZombieState& s = theZombie->mPvzu;
	if (u::ModPlantsOn() && u::ZombieStatusTint(u::RepeaterPoisonRecordFor(theZombie->mFromWave), s.repeaterPoisonCs, s.weakenCs).on)
		return;
	theColor = Color(128, 64, 192, anAlpha);   // 0x52d2c8: ZOMBIE_MINDCONTROLLED_COLOR (Zombie.cpp) with alpha EDI = 0xb0
	theAdditive = theColor;
	theEnableAdditive = true;                  // 0x52d382
}

} // namespace pvzu::engine
