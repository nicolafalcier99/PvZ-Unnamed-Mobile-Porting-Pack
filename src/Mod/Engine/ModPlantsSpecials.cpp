// PVZU: engine wiring of Unnamed's special mod plants (Plan 8 part 3b). See ModPlantsSpecials.h; rules
// Mod/Unnamed/ModPlantRules.h; evidence the research notes (mod-plants-behaviour) (section per seed, exe addresses).
#include "Mod/Engine/ModPlantsSpecials.h"

#include "Mod/CmdLine.h"
#include "Mod/Engine/ModPlantSites.h"
#include "Mod/Engine/ModPlants.h"
#include "Mod/Engine/ModPlantsShadow.h"
#include "Mod/Engine/ModZombieStatus.h"
#include "Mod/Unnamed/DayPlants.h"
#include "Mod/Unnamed/ModPlantRules.h"
#include "Mod/Unnamed/ModPlantSpecials.h"
#include "Mod/Unnamed/ModPlants.h"
#include "Mod/Unnamed/Rules.h"
#include "Mod/Unnamed/State.h"

#include "LawnApp.h"
#include "Lawn/Board.h"
#include "Lawn/Challenge.h"
#include "Lawn/Coin.h"
#include "Lawn/CursorObject.h"
#include "Lawn/Plant.h"
#include "Lawn/Projectile.h"
#include "Lawn/Zombie.h"
#include "PvzpLib/PvzpCommon.h"
#include "PvzpLib/PvzpFoley.h"
#include "PvzpLib/PvzpParticle.h"
#include "PvzpLib/Reanimator.h"
#include "Resources.h"

#include <algorithm>
#include <cstdio>
#include <format>
#include <string>

namespace u = pvzu::unnamed;

namespace pvzu::engine {

namespace {

static_assert(static_cast<int>(ZombiePhase::PHASE_ZOMBIE_DYING) == 1 && static_cast<int>(ZombiePhase::PHASE_ZOMBIE_MOWERED) == 3,
	"Rewind Grass skips exe zombie phases 1..3");
static_assert(static_cast<int>(ZombiePhase::PHASE_DANCER_DANCING_IN) == 40, "Rewind Grass: exe Dancer phase 40");
static_assert(static_cast<int>(PlantState::STATE_CHOMPER_BITING) == 10 && static_cast<int>(PlantState::STATE_CHOMPER_BITING_MISSED) == 12,
	"Fire-dragon Grass uses exe plant states 10 / 12");

int Seed(const Plant* thePlant) {
	return static_cast<int>(ModSeed(thePlant));
}

int Tick(const Board* theBoard) {
	return theBoard ? static_cast<int>(theBoard->mMainCounter) : -1;
}

std::string NameOf(int theSeed) {
	if (const u::ModPlant* p = u::FindModPlant(theSeed))
		return p->nameEn;
	if (theSeed >= 0 && theSeed < static_cast<int>(SeedType::NUM_SEED_TYPES))
		return GetPlantDefinition(static_cast<SeedType>(theSeed)).mPlantName;
	return "?";
}

void Log(const Plant* thePlant, std::string_view theText) {
	const std::string aLine = std::format("{} 0x{:02x} col {} row {}: {}", NameOf(Seed(thePlant)), Seed(thePlant), thePlant->mPlantCol,
		thePlant->mRow, theText);
	u::LogPlantAbility(Tick(thePlant->mBoard), aLine);
}

// A plant's rules are on: feature "modPlants" and its key in rules-mod-plants.json (else the base plant runs).
bool RuleOn(const char* theKey) {
	return u::ModPlantsOn() && u::RuleInt(theKey, 0) > 0;
}

bool DevShadow() {
	static const bool aFlag = CmdFlag("-pvzu-dev-specials-shadow");
	return aFlag;
}

bool Shadowed(const Plant* thePlant) {
	return DevShadow() || ModPlantShadowed(thePlant);
}

bool UsableCoinSeed(int theSeed) {
	return u::PacketCoinSeedUsable(theSeed, [](int s) { return IsModSeed(static_cast<SeedType>(s)); });
}

// A coin from a mod plant; a seed-packet coin carries theSeed (Coin+0x68). nullptr when nothing was dropped.
Coin* DropCoin(Plant* thePlant, int theX, int theY, int theCoinType, int theMotion, int theSeed) {
	if (theCoinType <= 0)
		return nullptr;
	const bool aPacket = theCoinType == static_cast<int>(CoinType::COIN_USABLE_SEED_PACKET);
	if (aPacket && !UsableCoinSeed(theSeed))
		return nullptr;
	Coin* aCoin = thePlant->mBoard->AddCoin(theX, theY, static_cast<CoinType>(theCoinType), static_cast<CoinMotion>(theMotion));
	if (aCoin && aPacket)
		aCoin->mUsableSeedType = static_cast<SeedType>(theSeed);
	return aCoin;
}

std::string CoinName(int theCoinType, int theSeed) {
	switch (theCoinType) {
	case CoinType::COIN_SUN: return "sun25";
	case CoinType::COIN_SMALLSUN: return "sun15";
	case CoinType::COIN_LARGESUN: return "sun50";
	case CoinType::COIN_USABLE_SEED_PACKET: return std::format("packet 0x{:02x} {}", theSeed, NameOf(theSeed));
	default: return std::format("coin {}", theCoinType);
	}
}

// Vanilla UpdateProductionPlant's gates (the exe's sun helper 0x7b0200 repeats them).
bool ProductionGatesOpen(Plant* thePlant) {
	LawnApp* anApp = thePlant->mApp;
	if (!thePlant->IsInPlay() || anApp->IsIZombieLevel() || anApp->mGameMode == GameMode::GAMEMODE_UPSELL ||
		anApp->mGameMode == GameMode::GAMEMODE_INTRO)
		return false;
	return !thePlant->mBoard->HasLevelAwardDropped();
}

bool LastStandWaits(Plant* thePlant) {
	return thePlant->mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_LAST_STAND &&
		thePlant->mBoard->mChallenge->mChallengeState != ChallengeState::STATECHALLENGE_LAST_STAND_ONSLAUGHT;
}

// The production timer: -1 per tick, the glow when <= 100 (vanilla). True when it ran out.
bool ProductionTimerRunsOut(Plant* thePlant) {
	thePlant->mLaunchCounter--;
	if (thePlant->mLaunchCounter <= 100)
		thePlant->mEatenFlashCountdown = std::max(thePlant->mEatenFlashCountdown, std::clamp(100 - thePlant->mLaunchCounter, 0, 100));
	return thePlant->mLaunchCounter <= 0;
}

void PlayBodyTrack(Plant* thePlant, const char* theTrack, const char* theFallback, ReanimLoopType theLoop, int theBlend, float theRate) {
	Reanimation* aBody = thePlant->mApp->ReanimationTryToGet(thePlant->mBodyReanimID);
	if (aBody == nullptr)
		return;
	const char* aTrack = aBody->TrackExists(theTrack) ? theTrack : theFallback;
	if (aTrack && aBody->TrackExists(aTrack))
		thePlant->PlayBodyReanim(aTrack, theLoop, theBlend, theRate);
}

// --- 0x51 Primal Sunflower (0x7b0200, 0xc131f0) ----------------------------------------------------------------
bool UpdatePrimalSunflower(Plant* thePlant) {
	if (!u::ModSunPlantProduces(Seed(thePlant)))
		return false;
	if (!ProductionGatesOpen(thePlant) || LastStandWaits(thePlant) || !ProductionTimerRunsOut(thePlant))
		return true;
	const int aTimerRange = u::ModSunPlantTimerRollRange();
	thePlant->mLaunchCounter = u::ModSunPlantNextTimer(thePlant->mLaunchRate, aTimerRange > 0 ? Sexy::Rand(aTimerRange) : 0);
	thePlant->mApp->PlayFoley(FoleyType::FOLEY_SPAWN_SUN);
	const bool aShadow = Shadowed(thePlant);
	const int aPacketRange = u::PrimalSunflowerPacketRollRange(aShadow);
	const int aRoll = aPacketRange > 0 ? Sexy::Rand(aPacketRange) : -1;
	const u::ModSunDrops d = u::PrimalSunflowerProduce(aShadow, aRoll);
	DropCoin(thePlant, thePlant->mX, thePlant->mY, d.first.coinType, d.first.motion, d.first.usableSeed);
	DropCoin(thePlant, thePlant->mX, thePlant->mY, d.second.coinType, d.second.motion, -1);
	Log(thePlant, std::format("primalSunflower.produce shadow {} roll {}: {} + {}, next in {} cs", aShadow ? 1 : 0, aRoll,
		CoinName(d.first.coinType, d.first.usableSeed), CoinName(d.second.coinType, -1), thePlant->mLaunchCounter));
	return true;
}

// --- 0x76 Boss Flower (hooks 310 / 291 Marigold timer 0x45fa01, hook 292 drop 0xc000b0) -------------------------
bool UpdateBossFlower(Plant* thePlant) {
	if (u::BossFlowerRollRange() <= 0)
		return false;
	if (!ProductionGatesOpen(thePlant))
		return true;
	Board* aBoard = thePlant->mBoard;
	if (aBoard->mCurrentWave == aBoard->mNumWaves) { // Marigold ending: 6000 more cs, then no more drops
		if (thePlant->mState != PlantState::STATE_MARIGOLD_ENDING) {
			thePlant->mState = PlantState::STATE_MARIGOLD_ENDING;
			thePlant->mStateCountdown = 6000;
		} else if (thePlant->mStateCountdown <= 0) {
			return true;
		}
	}
	if (LastStandWaits(thePlant) || !ProductionTimerRunsOut(thePlant))
		return true;
	thePlant->mLaunchCounter = RandRangeInt(thePlant->mLaunchRate - 150, thePlant->mLaunchRate); // no Marigold cap 120 (0x651779)
	thePlant->mApp->PlayFoley(FoleyType::FOLEY_SPAWN_SUN);
	Coin* aCoin = aBoard->AddCoin(thePlant->mX, thePlant->mY, static_cast<CoinType>(u::BossFlowerCoinType()),
		static_cast<CoinMotion>(u::BossFlowerCoinMotion()));
	const int aSeed = u::BossFlowerDrawSeed([](int n) { return Sexy::Rand(n); }, UsableCoinSeed);
	if (aCoin && aSeed >= 0)
		aCoin->mUsableSeedType = static_cast<SeedType>(aSeed);
	else if (aCoin)
		aCoin->Die();
	Log(thePlant, std::format("bossFlower.drop {}, next in {} cs", aSeed >= 0 ? CoinName(CoinType::COIN_USABLE_SEED_PACKET, aSeed) : "nothing",
		thePlant->mLaunchCounter));
	return true;
}

// --- 0x52 Shrapnel Cherry (DoSpecial 0x794000) --------------------------------------------------------------
bool DoSpecialShrapnelCherry(Plant* thePlant) {
	const std::vector<u::ShrapnelShard> aShards = u::ShrapnelCherryShards();
	if (aShards.empty())
		return false;
	for (const u::ShrapnelShard& s : aShards) {
		Projectile* aShot = thePlant->mBoard->AddProjectile(thePlant->mX, thePlant->mY, 0x539fd, thePlant->mRow, ProjectileType::PROJECTILE_STAR);
		if (aShot == nullptr)
			continue;
		aShot->mMotionType = ProjectileMotion::MOTION_STAR;
		aShot->mVelX = s.velX;
		aShot->mVelY = s.velY;
		aShot->mDamageRangeFlags = 1; // +0x74 = 1 (ground)
		aShot->mPvzu.modShot = s.kind == 2 ? u::kModShotShrapnelCore : u::kModShotShrapnelShard;
	}
	Log(thePlant, std::format("shrapnelCherry.blast {} stars (1 core) + the Cherry Bomb blast", aShards.size()));
	return false; // jmp 0x466713: the vanilla Cherry Bomb blast follows
}

// --- 0x53 Bowling Wall-nut (hooks 307, 265, 370) --------------------------------------------------------------
bool BowlingRolls(const Plant* thePlant) {
	return Seed(thePlant) == u::kModSeedBowlingWallnut && u::BowlingWallnutRolls(thePlant->mApp->IsWallnutBowlingLevel());
}

void PlantedBowlingWallnut(Plant* thePlant) {
	if (!BowlingRolls(thePlant))
		return;
	Reanimation* aBody = thePlant->mApp->ReanimationTryToGet(thePlant->mBodyReanimID);
	if (aBody && aBody->TrackExists("_ground"))
		aBody->SetFramesForLayer("_ground"); // hook 265: the bowling layer; no RandF 12..18 rate (only seeds 3 / 0x31)
	Log(thePlant, "bowlingWallnut.rolls on this level, not eaten");
}

// --- 0x56 Fire-dragon Grass (0x7b0400, 0xc026c0, 0xc055b0, 0xc060b0, 0xc061c0) ----------------------------------
bool FireDragonTarget(const Plant* thePlant, Zombie* theZombie) {
	return u::FireDragonInRange(static_cast<int>(theZombie->mZombieType), theZombie->mMindControlled,
		theZombie->EffectedByDamage(static_cast<unsigned int>(u::FireDragonDamageFlags())), theZombie->mRow, theZombie->mX, thePlant->mRow,
		thePlant->mX, thePlant->mPlantCol);
}

void FireDragonFlame(Plant* thePlant) {
	thePlant->mApp->PlayFoley(FoleyType::FOLEY_IGNITE); // sound 0x26
	const bool aShadow = Shadowed(thePlant);
	int aBudget = u::FireDragonFlameBudget(); // 0xc060fb: 1800 per flame
	int aHit = 0, aTotal = 0;
	for (Zombie* aZombie : thePlant->mBoard->mZombies) {
		if (aZombie->mDead || !FireDragonTarget(thePlant, aZombie))
			continue;
		const u::FireDragonHit h = u::FireDragonSpend(u::FireDragonHitDamage(aZombie->mBodyMaxHealth), aBudget, aShadow);
		aBudget = h.budgetLeft;
		aZombie->TakeDamage(h.damage, 0U);
		if (u::FireDragonClearsChillAndFreeze()) { // 0x7b054c
			if (aZombie->mIceTrapCounter > 0)
				aZombie->RemoveIceTrap();
			if (aZombie->mChilledCounter > 0) {
				aZombie->mChilledCounter = 0;
				aZombie->UpdateAnimSpeed();
			}
		}
		if (h.poisonAdd > 0)
			ModZombieAddWeakened(aZombie, h.poisonAdd); // 0xc060e9 add word [zombie+0x1a], 400 (Mod/Engine/ModZombieStatus.h)
		aHit++;
		aTotal += h.damage;
	}
	Log(thePlant, std::format("fireDragon.flame shadow {}: {} zombies hit, {} damage, budget left {}", aShadow ? 1 : 0, aHit, aTotal, aBudget));
}

bool UpdateFireDragonGrass(Plant* thePlant) {
	const int aAttackCs = u::FireDragonAttackCs();
	if (aAttackCs <= 0)
		return false;
	if (thePlant->mState == PlantState::STATE_CHOMPER_BITING) {
		if (thePlant->mStateCountdown == u::FireDragonHitAtCountdown())
			FireDragonFlame(thePlant);
		if (thePlant->mStateCountdown == 0)
			thePlant->mState = PlantState::STATE_CHOMPER_BITING_MISSED; // 0x7b043a: state 12
	} else if (thePlant->mState == PlantState::STATE_CHOMPER_BITING_MISSED) {
		Reanimation* aBody = thePlant->mApp->ReanimationTryToGet(thePlant->mBodyReanimID);
		if (aBody == nullptr || aBody->mLoopCount > 0) { // 0x7b05c8: back to idle
			PlayBodyTrack(thePlant, "anim_idle", nullptr, ReanimLoopType::REANIM_LOOP, 10, 0.0f);
			thePlant->mState = PlantState::STATE_READY;
		}
	} else if (thePlant->mState == PlantState::STATE_READY) {
		for (Zombie* aZombie : thePlant->mBoard->mZombies) {
			if (aZombie->mDead || !FireDragonTarget(thePlant, aZombie))
				continue;
			// 0x7b0576: anim_shooting once at 24 (a Chomper reanim has anim_bite), state 10, countdown 70
			PlayBodyTrack(thePlant, "anim_shooting", "anim_bite", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 0, 24.0f);
			thePlant->mState = PlantState::STATE_CHOMPER_BITING;
			thePlant->mStateCountdown = aAttackCs;
			Log(thePlant, std::format("fireDragon.attack zombie type {} row {} x {}, flame in {} cs", static_cast<int>(aZombie->mZombieType),
				aZombie->mRow, aZombie->mX, aAttackCs - u::FireDragonHitAtCountdown()));
			break;
		}
	}
	return true;
}

// --- 0x59 Golden Sun-shroom (0x7b0ce0; its timer is the exe's +0xbc = mBeghouledFlashCountdown) -----------------
bool UpdateGoldenSunShroom(Plant* thePlant) {
	if (!RuleOn("modPlants.goldenSunShroom.burstCs"))
		return false;
	const u::GoldenSunShroomStep s = u::GoldenSunShroomUpdate(static_cast<int>(thePlant->mApp->mGameMode), thePlant->IsOnBoard(),
		thePlant->NotOnGround(), static_cast<int>(thePlant->mState), thePlant->mBeghouledFlashCountdown);
	if (s.state >= 0)
		thePlant->mState = static_cast<PlantState>(s.state);
	if (s.disappearCs >= 0)
		thePlant->mDisappearCountdown = s.disappearCs;
	if (s.timerCs >= 0) {
		thePlant->mBeghouledFlashCountdown = s.timerCs;
		Log(thePlant, std::format("goldenSunShroom.burst: 3 large suns in {} cs, then gone", s.timerCs));
	}
	if (s.dropSun) {
		thePlant->mBoard->AddCoin(thePlant->mX, thePlant->mY, static_cast<CoinType>(s.coinType), static_cast<CoinMotion>(s.coinMotion));
		thePlant->mApp->PlayFoley(static_cast<FoleyType>(0x44)); // 0x453630 esi 0x44
		Log(thePlant, std::format("goldenSunShroom.sun {} motion {} at timer {}", CoinName(s.coinType, -1), s.coinMotion,
			thePlant->mBeghouledFlashCountdown));
	}
	return true;
}

// --- 0x5b Magic Devourer (0x7b07c0) -----------------------------------------------------------------------------
Plant* DevourerVictim(Plant* theDevourer) {
	Board* aBoard = theDevourer->mBoard;
	PlantsOnLawn aOn{};
	aBoard->GetPlantsOnLawn(theDevourer->mPlantCol, theDevourer->mRow, &aOn);
	if (aOn.mNormalPlant && aOn.mNormalPlant != theDevourer && !aOn.mNormalPlant->mDead)
		return aOn.mNormalPlant;
	// The Devourer itself took the normal slot (no flying-slot rule for 0x5b on this build): the other normal plant.
	for (Plant* p : aBoard->mPlants)
		if (!p->mDead && p != theDevourer && p->mPlantCol == theDevourer->mPlantCol && p->mRow == theDevourer->mRow &&
			p != aOn.mPumpkinPlant && p != aOn.mFlyingPlant && p != aOn.mUnderPlant)
			return p;
	return nullptr;
}

bool UpdateMagicDevourer(Plant* thePlant) {
	if (u::MagicDevourerCardCoinType() <= 0)
		return false;
	thePlant->UpdateGraveBuster(); // called first by 0x7b07c0
	if (thePlant->mDead)
		return true;
	thePlant->mStateCountdown = u::MagicDevourerCountdownTick(static_cast<int>(thePlant->mState), thePlant->mStateCountdown);
	if (!u::MagicDevourerSwallowsNow(static_cast<int>(thePlant->mState), thePlant->mStateCountdown, thePlant->mPlantCol, thePlant->mRow,
			thePlant->mApp->IsWallnutBowlingLevel()))
		return true;
	Plant* aVictim = DevourerVictim(thePlant);
	if (aVictim == nullptr) {
		Log(thePlant, "magicDevourer.swallow: no plant under it");
		return true;
	}
	const int aSeed = static_cast<int>(ModSeedOf(aVictim));
	Coin* aCoin = DropCoin(thePlant, aVictim->mX, aVictim->mY, u::MagicDevourerCardCoinType(), u::MagicDevourerCardCoinMotion(), aSeed);
	Log(thePlant, std::format("magicDevourer.swallow {} -> {}", NameOf(aSeed), aCoin ? CoinName(CoinType::COIN_USABLE_SEED_PACKET, aSeed) : "no packet"));
	aVictim->Die();
	return true;
}

// --- 0x5d Prism-shroom (hook 339 0x7a6014, hook 349 0x7a5279, 0x7978e6) -----------------------------------------
bool PrismFire(Plant* thePlant, int theRow) {
	if (u::PrismShroomProjectileType() < 0)
		return false;
	Board* aBoard = thePlant->mBoard;
	int aX = thePlant->mX + 29, aY = thePlant->mY + 21; // Scaredy-shroom's offsets (hook 351 compares seed - 0x50)
	if (aBoard->GetFlowerPotAt(thePlant->mPlantCol, thePlant->mRow))
		aY -= 5;
	thePlant->mApp->PlayFoley(FoleyType::FOLEY_THROW);
	thePlant->mApp->PlayFoley(FoleyType::FOLEY_PUFF);
	thePlant->mApp->AddPvzpParticle(aX + 27, aY + 13, Board::MakeRenderOrder(RenderLayer::RENDER_LAYER_LAWN_MOWER, thePlant->mRow, 1),
		ParticleEffect::PARTICLE_PUFFSHROOM_MUZZLE);
	Projectile* aShot = aBoard->AddProjectile(aX, aY, thePlant->mRenderOrder - 1, theRow, ProjectileType::PROJECTILE_PUFF);
	if (aShot == nullptr)
		return true;
	aShot->mDamageRangeFlags = thePlant->GetDamageRangeFlags(PlantWeapon::WEAPON_PRIMARY);
	// the exe's type 0x16 with its damage in +0x80, the same mod shot as Spirit-tail's beam (ModShotImpact / ModShotLook
	// in ModPlantsShooters.cpp: damage 0x7b08fb, image 0x7a0e5e); the Puff carrier keeps Scaredy's motion
	aShot->mPvzu.modType = u::PrismShroomProjectileType();
	aShot->mPvzu.field80 = u::PrismShroomShotDamage(aBoard->mSunMoney); // read when the shot is made, after the 50 was paid
	Log(thePlant, std::format("prismShroom.fire damage {} (sun {})", aShot->mPvzu.field80, aBoard->mSunMoney));
	return true;
}

bool ClickPrismShroom(Plant* thePlant) {
	Board* aBoard = thePlant->mBoard;
	if (aBoard == nullptr || !u::PrismShroomClickable(thePlant->mIsAsleep, thePlant->mShootingCounter)) {
		Log(thePlant, std::format("prismShroom.click ignored: asleep {} shooting counter {}", thePlant->mIsAsleep ? 1 : 0, thePlant->mShootingCounter));
		return false;
	}
	const bool aFree = aBoard->HasConveyorBeltSeedBank() || thePlant->mApp->mEasyPlantingCheat; // 0x41be50 / FREE cheat
	const u::PrismClick c = u::PrismShroomClick(thePlant->mIsAsleep, thePlant->mShootingCounter, aBoard->mSunMoney,
		aBoard->CanTakeSunMoney(u::PrismShroomShotCost()), aFree);
	if (c.buzzer) {
		thePlant->mApp->PlaySample(Sexy::SOUND_BUZZER);
		aBoard->mOutOfMoneyCounter = c.sunFlashCs;
		Log(thePlant, std::format("prismShroom.click: not enough sun ({}), buzzer", aBoard->mSunMoney));
		return true;
	}
	if (!c.fires)
		return false;
	const int aSunBefore = aBoard->mSunMoney;
	aBoard->mSunMoney = c.sun;
	thePlant->mShootingCounter = c.shootingCounter;
	PlayBodyTrack(thePlant, "anim_shooting", nullptr, ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 20, 35.0f);
	Log(thePlant, std::format("prismShroom.click: paid {} sun ({} -> {}), shot in {} cs", aSunBefore - c.sun, aSunBefore, c.sun, c.shootingCounter));
	return true;
}

// --- 0x6b Rewind Grass (0x7b0a00) -------------------------------------------------------------------------------
void RewindZombies(Plant* thePlant) {
	int aHealed = 0, aSentBack = 0;
	for (Zombie* z : thePlant->mBoard->mZombies) {
		if (z->mDead || !u::RewindGrassAffects(ModZombieBossClass(z), static_cast<int>(z->mZombiePhase), z->mX, z->mHasHead))
			continue;
		u::ZombieHpSet hp{z->mBodyHealth, z->mBodyMaxHealth, z->mHelmHealth, z->mHelmMaxHealth, z->mShieldHealth, z->mShieldMaxHealth,
			z->mFlyingHealth, z->mFlyingMaxHealth};
		u::RewindGrassHeal(hp); // 0x7946a7: each HP up to its max
		z->mBodyHealth = hp.body;
		z->mHelmHealth = hp.helm;
		z->mShieldHealth = hp.shield;
		z->mFlyingHealth = hp.flying;
		aHealed++;
		if (u::RewindGrassSendsBack(static_cast<int>(z->mZombieType), z->mMindControlled, z->mBlowingAway)) {
			if (u::RewindGrassResetsPhaseCounter(static_cast<int>(z->mZombiePhase)))
				z->mPhaseCounter = 0;
			z->mPosX = u::RewindGrassBackX(); // 0xc054f1: 800.0
			z->mX = static_cast<int>(z->mPosX);
			aSentBack++;
		}
	}
	Log(thePlant, std::format("rewindGrass.rewind: {} zombies healed, {} sent back to x {}", aHealed, aSentBack, u::RewindGrassBackX()));
}

bool UpdateRewindGrass(Plant* thePlant) {
	const int aDelay = u::RewindGrassBlowDelayCs();
	if (aDelay <= 0)
		return false;
	Reanimation* aBody = thePlant->mApp->ReanimationTryToGet(thePlant->mBodyReanimID);
	if (thePlant->mState == PlantState::STATE_NOTREADY && thePlant->mDoSpecialCountdown == 0 && aBody &&
		aBody->mLoopType != ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD) { // 0x7b0a47: arm the blow
		thePlant->mDoSpecialCountdown = aDelay;
		PlayBodyTrack(thePlant, "anim_blow", nullptr, ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 0, u::RewindGrassBlowAnimRate());
		return true;
	}
	if (thePlant->mDoSpecialCountdown <= 0 || --thePlant->mDoSpecialCountdown > 0 || thePlant->mState == PlantState::STATE_DOINGSPECIAL)
		return true;
	// 0x7b0a92: back to anim_loop, state 2 (one shot), foley 0xf, every zombie visited. The vanilla DOINGSPECIAL fade
	// at the top of UpdateAbilities then removes it 200 cs later, like a Blover.
	PlayBodyTrack(thePlant, "anim_loop", nullptr, ReanimLoopType::REANIM_LOOP, 0, 0.0f);
	thePlant->mState = PlantState::STATE_DOINGSPECIAL;
	thePlant->mApp->PlayFoley(static_cast<FoleyType>(0xf));
	RewindZombies(thePlant);
	return true;
}

// --- 0x73 Strong Coffee Bean (0x7b0b90; heal: Plant::Die hook 362 0xc05db2) -----------------------------------
bool UpdateStrongCoffee(Plant* thePlant) {
	if (u::StrongCoffeeWakeCs() <= 0)
		return false;
	if (!u::StrongCoffeeTriggers(thePlant->mDoSpecialCountdown, thePlant->IsOnBoard())) {
		if (thePlant->mDoSpecialCountdown > 1)
			thePlant->mDoSpecialCountdown--; // the vanilla countdown; the special itself never runs (it fires at 1)
		return true;
	}
	int aWoken = 0;
	for (Plant* p : thePlant->mBoard->mPlants) {
		if (p->mDead || !u::StrongCoffeeWakes(thePlant->mRow, thePlant->mPlantCol, p->mRow, p->mPlantCol, p->NotOnGround()))
			continue;
		if (p->mIsAsleep)
			aWoken++;
		p->mWakeUpCounter = u::StrongCoffeeWakeCs(); // 0x7b0c05: awake plants too
	}
	thePlant->mState = PlantState::STATE_DOINGSPECIAL;
	PlayBodyTrack(thePlant, "anim_crumble", nullptr, ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 0, 0.0f);
	thePlant->mApp->PlayFoley(FoleyType::FOLEY_COFFEE); // sound 0x45
	thePlant->mApp->AddPvzpParticle(thePlant->mX + thePlant->mWidth / 2, thePlant->mY + thePlant->mHeight / 2, thePlant->mRenderOrder + 1,
		static_cast<ParticleEffect>(0x3c));
	Log(thePlant, std::format("strongCoffee.wake 3x3: {} sleeping plants wake in {} cs", aWoken, u::StrongCoffeeWakeCs()));
	thePlant->Die();
	return true;
}

// theExeSeed: 0x73 Strong Coffee or 0x23 the vanilla Coffee Bean (hook 362 -> 0xc11ce0 takes both, no mode gate).
void CoffeeDiesHeal(Plant* thePlant, int theExeSeed) {
	if (thePlant->mState != PlantState::STATE_DOINGSPECIAL || !thePlant->IsOnBoard())
		return;
	PlantsOnLawn aOn{};
	thePlant->mBoard->GetPlantsOnLawn(thePlant->mPlantCol, thePlant->mRow, &aOn);
	Plant* aTop = nullptr; // 0x467a76: flying, normal, pumpkin, under (the dying Coffee itself left out)
	for (Plant* p : {aOn.mFlyingPlant, aOn.mNormalPlant, aOn.mPumpkinPlant, aOn.mUnderPlant})
		if (aTop == nullptr && p && p != thePlant && !p->mDead)
			aTop = p;
	if (aTop == nullptr)
		return;
	const u::CoffeeHeal h = u::CoffeeBeanDeathHeal(theExeSeed, static_cast<int>(thePlant->mState), aTop->mPlantHealth,
		aTop->mPlantMaxHealth);
	if (h.hp == aTop->mPlantHealth && h.maxHp == aTop->mPlantMaxHealth)
		return;
	const int aBefore = aTop->mPlantHealth;
	aTop->mPlantHealth = h.hp;
	aTop->mPlantMaxHealth = h.maxHp;
	thePlant->mApp->AddPvzpParticle(aTop->mX + 40, aTop->mY + 40, aTop->mRenderOrder + 1, static_cast<ParticleEffect>(0x46));
	const std::string aText = std::format("{}.heal {} hp {} -> {} (max {})", theExeSeed == u::kModSeedStrongCoffeeBean ? "strongCoffee" :
		"coffeeBean", NameOf(static_cast<int>(ModSeedOf(aTop))), aBefore, h.hp, h.maxHp);
	if (theExeSeed == u::kModSeedStrongCoffeeBean)
		Log(thePlant, aText);
	else // a vanilla plant has no mod seed for Log's name
		u::LogPlantAbility(Tick(thePlant->mBoard), std::format("COFFEE_BEAN 0x23 col {} row {}: {}", thePlant->mPlantCol, thePlant->mRow, aText));
}

void StrongCoffeeDies(Plant* thePlant) {
	CoffeeDiesHeal(thePlant, u::kModSeedStrongCoffeeBean);
}

// --- 0x75 Leaf Shield (0x7b0da0) --------------------------------------------------------------------------------
bool UpdateLeafShield(Plant* thePlant) {
	if (!RuleOn("modPlants.leafShield.bandPx"))
		return false;
	if (!thePlant->IsOnBoard() || thePlant->NotOnGround())
		return true;
	for (Projectile* aShot : thePlant->mBoard->mProjectiles) {
		if (aShot->mDead || !u::LeafShieldReflects(aShot->mDamageRangeFlags, aShot->mRow, aShot->mX, thePlant->mRow, thePlant->mX, thePlant->mWidth))
			continue;
		const u::LeafShieldBounce b = u::LeafShieldBounceOf(static_cast<int>(aShot->mMotionType));
		const int aDamage = u::ModProjectileDamage(aShot->mPvzu.modType != 0 ? aShot->mPvzu.modType : static_cast<int>(aShot->mProjectileType), aShot->GetProjectileDef().mDamage, // PVZU R3: a vase costs 300 >> 3 + 2
			aShot->mPvzu.field80, thePlant->mApp->IsIZombieLevel());
		const int aCost = u::LeafShieldHpCost(aDamage);
		aShot->mDamageRangeFlags = b.damageRangeFlags;
		aShot->mMotionType = static_cast<ProjectileMotion>(b.motionType);
		if (b.negateVelX)
			aShot->mVelX = -aShot->mVelX;
		if (aShot->mPvzu.modShot == u::kModShotNone)
			aShot->mPvzu.modShot = u::kModShotLeafReflected;
		thePlant->mPlantHealth -= aCost;
		thePlant->mEatenFlashCountdown = b.flashCs;
		thePlant->mApp->PlayFoley(static_cast<FoleyType>(0x30)); // sound 0x30
		thePlant->mApp->AddPvzpParticle(aShot->mPosX, aShot->mPosY, aShot->mRenderOrder + 1, ParticleEffect::PARTICLE_POW); // 0xf
		Log(thePlant, std::format("leafShield.reflect shot type {} row {}: now hits zombies, leaf hp -{} -> {}",
			static_cast<int>(aShot->mProjectileType), aShot->mRow, aCost, thePlant->mPlantHealth));
	}
	return true;
}

// --- 0x5c Hallucination-shroom (EatPlant 0x52fd38 -> 0x79b030 -> 0x7a65eb) -------------------------------------
Zombie* HallucinationMorph(Zombie* theZombie, Plant* thePlant) {
	const u::HallucinationImp m = u::HallucinationImpMorph(theZombie->mBodyHealth);
	Board* aBoard = theZombie->mBoard;
	Zombie* anImp = aBoard->AddZombieInRow(static_cast<ZombieType>(m.zombieType), theZombie->mRow, theZombie->mFromWave);
	if (anImp == nullptr)
		return theZombie;
	// Same zombie in the exe (type, HP, rects and reanim rewritten in place): here a new Imp takes its place, its
	// position, side, chill and Unnamed state (the elite form +0x134 is not reset either).
	anImp->mPosX = theZombie->mPosX;
	anImp->mX = static_cast<int>(anImp->mPosX);
	anImp->mBodyMaxHealth = m.bodyMaxHp;
	anImp->mBodyHealth = m.bodyHp;
	anImp->mHelmType = HelmType::HELMTYPE_NONE;
	anImp->mHelmHealth = anImp->mHelmMaxHealth = 0;
	anImp->mShieldType = ShieldType::SHIELDTYPE_NONE;
	anImp->mShieldHealth = anImp->mShieldMaxHealth = 0;
	anImp->mFlyingHealth = anImp->mFlyingMaxHealth = 0;
	anImp->mPvzu = theZombie->mPvzu;
	anImp->mChilledCounter = theZombie->mChilledCounter;
	if (theZombie->mMindControlled)
		anImp->StartMindControlled();
	anImp->UpdateAnimSpeed();
	theZombie->mApp->PlayFoley(static_cast<FoleyType>(8));
	theZombie->mApp->AddPvzpParticle(theZombie->mPosX + 60.0f, theZombie->mPosY + 40.0f, theZombie->mRenderOrder + 1,
		static_cast<ParticleEffect>(m.particle));
	Log(thePlant, std::format("hallucination.imp zombie type {} body {} -> Imp body {}/{}{}", static_cast<int>(theZombie->mZombieType),
		theZombie->mBodyHealth, anImp->mBodyHealth, anImp->mBodyMaxHealth, theZombie->mMindControlled ? " (still hypnotised)" : ""));
	theZombie->mPvzu = {}; // no death effects of its old form: it did not die
	theZombie->DieNoLoot();
	return anImp;
}

} // namespace

// --- dispatch ------------------------------------------------------------------------------------------------------

void SpecialsOnPlanted(Plant* thePlant) {
	const int aSeed = Seed(thePlant);
	switch (aSeed) {
	case u::kModSeedPrimalSunflower:
	case u::kModSeedBossFlower:
		if (const PlantDefinition* aDef = ModPlantDefinition(static_cast<SeedType>(aSeed)); aDef && aDef->mLaunchRate > 0) {
			thePlant->mLaunchRate = aDef->mLaunchRate; // the table's rate (Boss Flower 2400; the Marigold base has 2500)
			thePlant->mLaunchCounter = RandRangeInt(0, thePlant->mLaunchRate); // hook 265: mod seeds Rand(rate + 1)
			Log(thePlant, std::format("planted: launch rate {}, first drop in {} cs", thePlant->mLaunchRate, thePlant->mLaunchCounter));
		}
		break;
	case u::kModSeedBowlingWallnut:
		PlantedBowlingWallnut(thePlant);
		break;
	case u::kModSeedGoldenSunShroom:
	case u::kModSeedHallucinationShroom:
	case u::kModSeedPrismShroom:
		if (thePlant->mIsAsleep && u::ModPlantsOn()) { // 0x7a4770: the day-sleep list holds vanilla mushrooms only
			thePlant->SetSleeping(false);
			Log(thePlant, "planted awake (mod mushrooms do not sleep by day)");
		}
		break;
	default:
		break;
	}
}

bool SpecialsUpdate(Plant* thePlant) {
	switch (Seed(thePlant)) {
	case u::kModSeedPrimalSunflower: return UpdatePrimalSunflower(thePlant);
	case u::kModSeedBowlingWallnut: {
		if (!BowlingRolls(thePlant))
			return false;
		const PlantState aBefore = thePlant->mState;
		const int aRowBefore = thePlant->mRow;
		thePlant->UpdateBowling(); // hook 307: vanilla bowling (1800 / 900 / 400 hits, bounce, gone past x 800)
		if (thePlant->mState != aBefore || thePlant->mRow != aRowBefore || thePlant->mDead) {
			PlantsOnLawn aFirst; // does its planting square still list it (hook 41 flying slot)?
			thePlant->mBoard->GetPlantsOnLawn(thePlant->mPlantCol, thePlant->mStartRow, &aFirst);
			const bool aHeld = aFirst.mFlyingPlant == thePlant || aFirst.mNormalPlant == thePlant;
			Log(thePlant, std::format("bowlingWallnut.{} state {} -> {} (28 up / 29 down) x {} y {} row {} -> {}; first square {},{} {}",
				thePlant->mDead ? "gone" : "roll", static_cast<int>(aBefore), static_cast<int>(thePlant->mState), thePlant->mX, thePlant->mY,
				aRowBefore, thePlant->mRow, thePlant->mPlantCol, thePlant->mStartRow, aHeld ? "held" : "free"));
		}
		return true;
	}
	case u::kModSeedFireDragonGrass: return UpdateFireDragonGrass(thePlant);
	case u::kModSeedGoldenSunShroom: return UpdateGoldenSunShroom(thePlant);
	case u::kModSeedMagicDevourer: return UpdateMagicDevourer(thePlant);
	case u::kModSeedPrismShroom: return RuleOn("modPlants.prismShroom.clickToFire"); // never hides or fires by itself (0x7a5400)
	case u::kModSeedRewindGrass: return UpdateRewindGrass(thePlant);
	case u::kModSeedStrongCoffeeBean: return UpdateStrongCoffee(thePlant);
	case u::kModSeedLeafShield: return UpdateLeafShield(thePlant);
	case u::kModSeedBossFlower: return UpdateBossFlower(thePlant);
	default: return false;
	}
}

bool SpecialsFire(Plant* thePlant, [[maybe_unused]] Zombie* theTargetZombie, int theRow, [[maybe_unused]] int thePlantWeapon) {
	if (Seed(thePlant) == u::kModSeedPrismShroom)
		return PrismFire(thePlant, theRow);
	return false;
}

bool SpecialsDoSpecial(Plant* thePlant) {
	switch (Seed(thePlant)) {
	case u::kModSeedShrapnelCherry: return DoSpecialShrapnelCherry(thePlant);
	case u::kModSeedRewindGrass: return u::RewindGrassBlowDelayCs() > 0; // 0x794000 has no 0x6b case: no Blover blow
	default: return false;
	}
}

void SpecialsOnDie(Plant* thePlant) {
	if (Seed(thePlant) == u::kModSeedStrongCoffeeBean)
		StrongCoffeeDies(thePlant);
}

void CoffeeBeanDies(Plant* thePlant) {
	if (u::ModPlantsOn() && thePlant->mPvzu.modSeed == 0 && thePlant->mSeedType == SeedType::SEED_INSTANT_COFFEE)
		CoffeeDiesHeal(thePlant, static_cast<int>(SeedType::SEED_INSTANT_COFFEE)); // 0xc11ce0: cmp [ebp+0x24], 0x23
}

bool ModPlantClicked(Plant* thePlant) {
	return Seed(thePlant) == u::kModSeedPrismShroom && ClickPrismShroom(thePlant);
}

bool ModPlantShowsFinger(Plant* thePlant) {
	return Seed(thePlant) == u::kModSeedPrismShroom && u::PrismShroomClickable(thePlant->mIsAsleep, thePlant->mShootingCounter);
}

bool ModPlantBowlingRect(Plant* thePlant) {
	return BowlingRolls(thePlant);
}

int BowlingRowY(Plant* thePlant) {
	Board* aBoard = thePlant->mBoard;
	if (aBoard->StageHasRoof()) // GridToPixelY(col, row) at every column's plant x, smooth in between
		return aBoard->GridToPixelY(MAX_GRID_SIZE_X - 1, thePlant->mRow) + u::BowlingRoofSlopeY(thePlant->mX);
	return aBoard->GridToPixelY(0, thePlant->mRow); // flat lawns: vanilla
}

int BowlingLastRow(Board* theBoard) { return u::BowlingLastRow(theBoard->StageHas6Rows()); }

bool ModPlantUntouchable(const Plant* thePlant) {
	return u::RewindGrassUntouchable(Seed(thePlant));
}

std::optional<bool> ModCanTargetPlant(const Plant* thePlant) {
	const int aSeed = Seed(thePlant);
	if (u::RewindGrassUntouchable(aSeed))
		return false;
	if (aSeed == u::kModSeedBowlingWallnut && !u::BowlingWallnutEdible()) // 0x959423
		return false;
	return std::nullopt;
}

bool ModPlantEatenAsPlain(const Plant* thePlant) {
	return Seed(thePlant) == u::kModSeedHallucinationShroom && RuleOn("modPlants.hallucination.impMaxHp");
}

Zombie* ModPlantEatenToDeath(Zombie* theZombie, Plant* thePlant) {
	if (!u::HallucinationTurnsEaterToImp(Seed(thePlant), thePlant->mPlantHealth, ModZombieBossClass(theZombie)))
		return theZombie;
	return HallucinationMorph(theZombie, thePlant);
}

bool ModPlantGivesUmbrellaCover(const Plant* thePlant) {
	return Seed(thePlant) != u::kModSeedLeafShield; // no vanilla Umbrella behaviour for 0x75 (cover is keyed on seed 37)
}

bool SpecialShotUpdate(Projectile* theShot) {
	if (theShot->mPvzu.modShot != u::kModShotShrapnelCore || !u::ShrapnelCoreReplants(2, theShot->mProjectileAge))
		return false;
	// 0x79f067: a Cherry Bomb at column x / 80 of the core's row, moved onto the core; the core dies
	const int aCol = std::clamp(u::ShrapnelCoreReplantColumn(theShot->mX), 0, MAX_GRID_SIZE_X - 1);
	Plant* aCherry = theShot->mBoard->AddPlant(aCol, theShot->mRow, SeedType::SEED_CHERRYBOMB, SeedType::SEED_NONE);
	if (aCherry) {
		aCherry->mX = theShot->mX;
		aCherry->mY = theShot->mY;
	}
	u::LogPlantAbility(Tick(theShot->mBoard), std::format("Shrapnel Cherry core col {} row {}: shrapnelCherry.replant Cherry Bomb at age {} cs ({})",
		aCol, theShot->mRow, theShot->mProjectileAge, aCherry ? "planted" : "not planted"));
	theShot->Die();
	return true;
}

bool ModShotHitsZombies(const Projectile* theShot) {
	return theShot->mPvzu.modShot == u::kModShotLeafReflected && theShot->mDamageRangeFlags != 0;
}

int SpecialShotImpact(Projectile* theShot, Zombie* theZombie, int theDamage) {
	if (const int aKind = u::ShrapnelKindOfShot(theShot->mPvzu.modShot); aKind != 0 && theZombie) {
		const int anExtra = u::ShrapnelShardHitDamage(aKind);
		if (anExtra > 0) { // 0x79ef88: TakeDamage(250), particle 4, sound 0x37, board shake, then the star hit
			theZombie->TakeDamage(anExtra, 0U);
			theShot->mApp->AddPvzpParticle(theShot->mPosX + 15.0f, theShot->mPosY + 15.0f, theShot->mRenderOrder + 1, ParticleEffect::PARTICLE_POWIE);
			theShot->mApp->PlayFoley(static_cast<FoleyType>(0x37));
			theShot->mBoard->ShakeBoard(3, -4);
			u::LogPlantAbility(Tick(theShot->mBoard), std::format("Shrapnel Cherry {} row {}: shrapnelCherry.hit zombie type {}: {} + {}",
				aKind == 2 ? "core" : "shard", theShot->mRow, static_cast<int>(theZombie->mZombieType), anExtra, theDamage));
		}
		return theDamage;
	}
	if (theShot->mPvzu.modShot == u::kModShotLeafReflected && theZombie)
		u::LogPlantAbility(Tick(theShot->mBoard), std::format("Leaf Shield shot row {}: leafShield.reflectedHit zombie type {} for {}", theShot->mRow,
			static_cast<int>(theZombie->mZombieType), theDamage));
	return theDamage;
}

bool ModPacketStartsRecharging(SeedType theSeedType, SeedType theImitaterType) {
	const int aStart = u::GoldenSunShroomPacketStartCounter(static_cast<int>(theSeedType), static_cast<int>(theImitaterType));
	if (aStart >= 0)
		u::LogPlantAbility(-1, std::format("Golden Sun-shroom packet: goldenSunShroom.packetStartsRecharging ({} cs)", aStart));
	return aStart >= 0;
}

// --- dev ----------------------------------------------------------------------------------------------------------

namespace {
Plant* DevPut(Board* theBoard, int theCol, int theRow, int theSeed) {
	Plant* p = theBoard->AddPlant(theCol, theRow, static_cast<SeedType>(theSeed), SeedType::SEED_NONE);
	u::LogPlantAbility(Tick(theBoard), std::format("specials dev: put {} 0x{:02x} at col {} row {}", NameOf(theSeed), theSeed, theCol, theRow));
	return p;
}

void DevZombie(Board* theBoard, ZombieType theType, int theRow, float theX) {
	if (Zombie* z = theBoard->AddZombieInRow(theType, theRow, 0)) {
		z->mPosX = theX;
		z->mX = static_cast<int>(theX);
	}
}

// The real click path below the SDL layer: Coin::MouseDown picks the packet up, Board::MouseDown plants it.
void DevPlantPacketCoins(Board* theBoard) {
	if (theBoard->mCursorObject->mCursorType != CursorType::CURSOR_TYPE_NORMAL)
		return;
	for (Coin* aCoin : theBoard->mCoins) {
		if (aCoin->mDead || aCoin->mIsBeingCollected || aCoin->mType != CoinType::COIN_USABLE_SEED_PACKET || aCoin->mDisappearCounter < 150)
			continue;
		const SeedType aSeed = aCoin->mUsableSeedType;
		aCoin->MouseDown(static_cast<int>(aCoin->mPosX) + 20, static_cast<int>(aCoin->mPosY) + 20, 1);
		if (theBoard->mCursorObject->mCursorType != CursorType::CURSOR_TYPE_PLANT_FROM_USABLE_COIN)
			return;
		const SeedType aPlanting = theBoard->GetSeedTypeInCursor();
		for (int aCol = 2; aCol <= 4; ++aCol) {
			for (int aRow = 0; aRow < 5; ++aRow) {
				if (theBoard->GetTopPlantAt(aCol, aRow, PlantPriority::TOPPLANT_ANY) || theBoard->CanPlantAt(aCol, aRow, aPlanting) != PlantingReason::PLANTING_OK)
					continue;
				const int aBefore = theBoard->mPlants.mSize;
				theBoard->MouseDown(theBoard->GridToPixelX(aCol, aRow) + 40, theBoard->GridToPixelY(aCol, aRow) + 40, 1);
				Plant* aPlanted = theBoard->GetTopPlantAt(aCol, aRow, PlantPriority::TOPPLANT_ANY);
				u::LogPlantAbility(Tick(theBoard), std::format("specials dev: packet coin 0x{:02x} {} picked up and planted at col {} row {}: {} (plant seed 0x{:02x}, mod {})",
					static_cast<int>(aSeed), NameOf(static_cast<int>(aSeed)), aCol, aRow, theBoard->mPlants.mSize > aBefore ? "planted" : "NOT planted",
					aPlanted ? static_cast<int>(ModSeedOf(aPlanted)) : -1, aPlanted && aPlanted->mPvzu.modSeed ? "yes" : "no"));
				return;
			}
		}
		theBoard->RefreshSeedPacketFromCursor();
		return;
	}
}
} // namespace

namespace {
void DevLogZombies(Board* theBoard, const char* theWhen) {
	for (Zombie* z : theBoard->mZombies)
		if (!z->mDead)
			u::LogPlantAbility(Tick(theBoard), std::format("specials dev: {} zombie type {} row {} x {} hp body {}/{} helm {}/{} shield {}/{}", theWhen,
				static_cast<int>(z->mZombieType), z->mRow, z->mX, z->mBodyHealth, z->mBodyMaxHealth, z->mHelmHealth, z->mHelmMaxHealth,
				z->mShieldHealth, z->mShieldMaxHealth));
}

void DevLogPlantsAt(Board* theBoard, int theCol, int theRow, const char* theWhy) {
	std::string aLine;
	for (Plant* p : theBoard->mPlants)
		if (!p->mDead && p->mPlantCol == theCol && p->mRow == theRow)
			aLine += std::format(" {} hp {} frozen {}", NameOf(static_cast<int>(ModSeedOf(p))), p->mPlantHealth, p->mPvzu.frozen);
	u::LogPlantAbility(Tick(theBoard), std::format("specials dev: {} col {} row {}:{}", theWhy, theCol, theRow, aLine.empty() ? " nothing" : aLine));
}
} // namespace

void SpecialsDevTick(Board* theBoard, int thePlayTicks) {
	static const int aAt = CmdOptionInt("-pvzu-dev-specials", -1);
	if (aAt < 0 || thePlayTicks < aAt || !ModPlantsOn())
		return;
	const int t = thePlayTicks - aAt;
	if (t < 1500)
		theBoard->mSunMoney = std::max(theBoard->mSunMoney, 2000);
	else if (t == 1500) {
		theBoard->mSunMoney = 20; // the next Prism-shroom click buzzes
		u::LogPlantAbility(Tick(theBoard), "specials dev: sun set to 20");
	}
	if (t == 0) {
		// row 0: production, Devourer on a Peashooter, Hallucination-shroom in front of walkers
		if (Plant* p = DevPut(theBoard, 0, 0, u::kModSeedPrimalSunflower))
			p->mLaunchCounter = 200; // dev: first drops soon
		if (Plant* p = DevPut(theBoard, 1, 0, u::kModSeedBossFlower))
			p->mLaunchCounter = 300;
		DevPut(theBoard, 3, 0, SeedType::SEED_PEASHOOTER);
		DevPut(theBoard, 3, 0, u::kModSeedMagicDevourer);
		if (Plant* p = DevPut(theBoard, 7, 0, u::kModSeedHallucinationShroom))
			p->mPlantHealth = 20; // dev: the first bites finish it
		DevZombie(theBoard, ZombieType::ZOMBIE_PAIL, 0, 720.0f);
		// row 1: Prism-shroom (clicked through -pvzu-input), Leaf Shield against a Peashooter zombie
		if (Plant* p = DevPut(theBoard, 0, 1, u::kModSeedPrismShroom))
			u::LogPlantAbility(Tick(theBoard), std::format("specials dev: Prism-shroom click point x {} y {} (logical), app frame {}",
				theBoard->mX + p->mX + 40, theBoard->mY + p->mY + 40, theBoard->mApp->mUpdateCount));
		DevPut(theBoard, 4, 1, u::kModSeedLeafShield);
		DevZombie(theBoard, ZombieType::ZOMBIE_PEA_HEAD, 1, 760.0f);
		DevZombie(theBoard, ZombieType::ZOMBIE_NORMAL, 1, 900.0f);
		// row 2: Golden Sun-shroom, Fire-dragon Grass
		DevPut(theBoard, 0, 2, u::kModSeedGoldenSunShroom);
		DevPut(theBoard, 2, 2, u::kModSeedFireDragonGrass);
		DevZombie(theBoard, ZombieType::ZOMBIE_TRAFFIC_CONE, 2, 500.0f);
		DevZombie(theBoard, ZombieType::ZOMBIE_NORMAL, 3, 520.0f);
		// row 3: Strong Coffee on a hurt sleeping Sun-shroom, sleeping Puff-shrooms around it
		DevPut(theBoard, 0, 3, SeedType::SEED_PUFFSHROOM);
		if (Plant* p = DevPut(theBoard, 1, 3, SeedType::SEED_SUNSHROOM))
			p->mPlantHealth = 100;
		DevPut(theBoard, 1, 4, SeedType::SEED_PUFFSHROOM);
		DevPut(theBoard, 1, 3, u::kModSeedStrongCoffeeBean);
		// row 4: Bowling Wall-nut; a Holy-fire Pumpkin and (control) a Pumpkin, each on a Peashooter at 80 HP
		DevPut(theBoard, 6, 4, SeedType::SEED_PEASHOOTER);
		if (Plant* p = DevPut(theBoard, 6, 4, u::kModSeedHolyFirePumpkin))
			p->mPlantHealth = 80;
		DevPut(theBoard, 7, 4, SeedType::SEED_PEASHOOTER);
		if (Plant* p = DevPut(theBoard, 7, 4, SeedType::SEED_PUMPKINSHELL))
			p->mPlantHealth = 80;
		DevZombie(theBoard, ZombieType::ZOMBIE_NORMAL, 4, 650.0f);
	}
	if (t == 600) { // Shrapnel Cherry among the walkers of rows 1..3
		DevPut(theBoard, 5, 2, u::kModSeedShrapnelCherry);
		DevZombie(theBoard, ZombieType::ZOMBIE_NORMAL, 1, 470.0f);
		DevZombie(theBoard, ZombieType::ZOMBIE_NORMAL, 3, 470.0f);
	}
	if (t == 900) { // Rewind Grass once the walkers are hurt and close
		DevLogZombies(theBoard, "before rewind");
		DevPut(theBoard, 8, 0, u::kModSeedRewindGrass); // row 0: out of the Zomboni's freeze box (rows 2..4)
	}
	if (t == 960)
		DevLogZombies(theBoard, "after rewind");
	if (t == 1000) { // walkers for both Pumpkins, a Zomboni next to them (its freeze box spans rows 3..5)
		DevZombie(theBoard, ZombieType::ZOMBIE_NORMAL, 4, 700.0f);
		DevZombie(theBoard, ZombieType::ZOMBIE_ZAMBONI, 3, 760.0f);
	}
	if (t == 1100 || t == 1300)
		DevLogPlantsAt(theBoard, 8, 0, "Rewind Grass square");
	if (t == 300) { // Bowling Wall-nut with a walker in its row
		DevPut(theBoard, 0, 4, u::kModSeedBowlingWallnut);
		DevZombie(theBoard, ZombieType::ZOMBIE_TRAFFIC_CONE, 4, 450.0f);
	}
	for (Plant* p : theBoard->mPlants) // dev: drops every 150 cs or sooner, to see many of them
		if (!p->mDead && (Seed(p) == u::kModSeedPrimalSunflower || Seed(p) == u::kModSeedBossFlower) && p->mLaunchCounter > 150)
			p->mLaunchCounter = 150;
	if (t == 1800 || t == 2400) {
		DevLogPlantsAt(theBoard, 6, 4, "Holy-fire square");
		DevLogPlantsAt(theBoard, 7, 4, "Pumpkin square");
	}
	if (t > 0 && t % 100 == 0) {
		DevPlantPacketCoins(theBoard);
		if (t % 400 == 0)
			DevZombie(theBoard, ZombieType::ZOMBIE_NORMAL, 0, 760.0f); // more eaters for the Hallucination-shroom row
	}
}

} // namespace pvzu::engine
