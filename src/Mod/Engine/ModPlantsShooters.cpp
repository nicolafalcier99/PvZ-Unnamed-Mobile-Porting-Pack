// PVZU: Plan 8 part 3a: shooter / projectile mod plants on the engine (see ModPlantsShooters.h). Each block names the
// exe address its rule comes from (the research notes (mod-plants-behaviour)); the numbers are pack rules
// (data/rules-mod-plants.json) read through Mod/Unnamed/ModPlantRules.h.
#include "Mod/Engine/ModPlantsShooters.h"

#include "Mod/CmdLine.h"
#include "Mod/Engine/FogPlantSites.h"
#include "Mod/Engine/ModPlantSites.h"
#include "Mod/Engine/ModPlantsShadow.h"
#include "Mod/Engine/ModZombieStatus.h"
#include "Mod/Engine/NightEliteZombies.h"
#include "Mod/Unnamed/CatapultShots.h"
#include "Mod/Unnamed/DayPlants.h"
#include "Mod/Unnamed/ModPlantRules.h"
#include "Mod/Unnamed/ModPlants.h"
#include "Mod/Unnamed/ModShots.h"

#include "LawnApp.h"
#include "Resources.h"
#include "Lawn/Board.h"
#include "Lawn/Coin.h"
#include "Lawn/CursorObject.h"
#include "Lawn/GridItem.h"
#include "Lawn/Plant.h"
#include "Lawn/Projectile.h"
#include "Lawn/SeedPacket.h"
#include "Lawn/Zombie.h"
#include "PvzpLib/PvzpFoley.h"
#include "PvzpLib/PvzpParticle.h"
#include "PvzpLib/Reanimator.h"
#include "SexyAppFramework/graphics/SharedImage.h"
#include "graphics/Color.h"

#include <algorithm>
#include <cstdio>
#include <format>
#include <map>
#include <string>
#include <string_view>
#include <vector>
#include "Mod/Engine/ModModes.h"
#include "Mod/Engine/SmallRules.h" // PVZU Plan 9 R9: pity counter

namespace pvzu::engine {

namespace u = pvzu::unnamed;

namespace {

enum : int {
	kShadowPea = u::kModSeedShadowPeashooter, kMysticIce = u::kModSeedMysticIceShooter,
	kPrimalRepeater = u::kModSeedPrimalRepeater, kDreamStar = u::kModSeedDreamStarfruit, kDuskPult = u::kModSeedDuskPult,
	kButterPult = u::kModSeedButterPult, kSniper = u::kModSeedSniper, kElement = u::kModSeedElementShroom,
	kSpiritTail = u::kModSeedSpiritTail, kCannon = u::kModSeedLoadedCannon,
};

int Mod(const Plant* thePlant) { return static_cast<int>(ModSeed(thePlant)); }

bool DevShadow() {
	static const bool aFlag = CmdFlag("-pvzu-dev-shadow");
	return aFlag;
}

bool Shadowed(const Plant* thePlant) { return DevShadow() || ModPlantShadowed(thePlant); }

std::string PlantName(int theModSeed) {
	const u::ModPlant* p = u::FindModPlant(theModSeed);
	return p ? p->nameEn : std::format("0x{:02x}", theModSeed);
}

void Log(const Plant* thePlant, const std::string& theWhat) {
	u::LogPlantAbility(static_cast<int>(thePlant->mBoard->mMainCounter), std::format("MOD {} (0x{:02x}) col {} row {}: {}",
		PlantName(Mod(thePlant)), Mod(thePlant), thePlant->mPlantCol, thePlant->mRow, theWhat));
}

void LogShot(const Projectile* theShot, const std::string& theWhat) {
	u::LogPlantAbility(static_cast<int>(theShot->mBoard->mMainCounter), std::format("MODSHOT type 0x{:02x} row {} x {}: {}",
		theShot->mPvzu.modType ? theShot->mPvzu.modType : static_cast<int>(theShot->mProjectileType), theShot->mRow,
		theShot->mX, theWhat));
}

int Roll(int theRange) { return theRange > 0 ? Sexy::Rand(theRange) : 0; }

int PotDy(Plant* thePlant) { return thePlant->mBoard->GetFlowerPotAt(thePlant->mPlantCol, thePlant->mRow) ? -5 : 0; }

// Plant::Fire's pult aim (vanilla, copied: a mod pult fires through its own Fire).
void Lob(Projectile* theShot, Plant* thePlant, Zombie* theTarget, int theOriginX, int theOriginY) {
	float aRangeX, aRangeY;
	if (theTarget) {
		Rect aZombieRect = theTarget->GetZombieRect();
		aRangeX = theTarget->ZombieTargetLeadX(50.0f) - theOriginX - 30.0f;
		aRangeY = aZombieRect.mY - theOriginY;
		if (theTarget->mZombiePhase == ZombiePhase::PHASE_DOLPHIN_RIDING)
			aRangeX -= 60.0f;
		if (theTarget->mZombieType == ZombieType::ZOMBIE_POGO && theTarget->mHasObject)
			aRangeX -= 60.0f;
		if (theTarget->mZombiePhase == ZombiePhase::PHASE_SNORKEL_WALKING_IN_POOL)
			aRangeX -= 40.0f;
		if (theTarget->mZombieType == ZombieType::ZOMBIE_BOSS)
			aRangeY = thePlant->mBoard->GridToPixelY(8, thePlant->mRow) - theOriginY;
	} else {
		aRangeX = 700.0f - theOriginX;
		aRangeY = 0.0f;
	}
	if (aRangeX < 40.0f)
		aRangeX = 40.0f;
	theShot->mMotionType = ProjectileMotion::MOTION_LOBBED;
	theShot->mVelX = aRangeX / 120.0f;
	theShot->mVelY = 0.0f;
	theShot->mVelZ = aRangeY / 120.0f - 7.0f;
	theShot->mAccZ = 0.115f;
}

int gBurnCap = 0; // 0xc117c0: the burn cap of the blast in progress (0 = vanilla 1800)

// ---- Shadow Peashooter 0x50 (UpdateShooting hook 327 -> 0x7b0000) ----------------------------------------------
bool FireShadowPea(Plant* thePlant, int theRow) {
	const bool aShadow = Shadowed(thePlant);
	const u::ShadowPeashooterShot s = u::ShadowPeashooterFire(kShadowPea, aShadow, theRow);
	if (!s.handled)
		return false;
	Board* aBoard = thePlant->mBoard;
	if (!s.beam) { // 0x7b00e0: a snow pea from (x + 66, y + 10)
		Projectile* aShot = AddModShot(aBoard, thePlant->mX + s.originDx,
			thePlant->mY + s.originDy + (aBoard->GetFlowerPotAt(thePlant->mPlantCol, thePlant->mRow) ? s.potDy : 0),
			thePlant->mRenderOrder - 1, theRow, s.projectileType);
		if (aShot)
			aShot->mDamageRangeFlags = thePlant->GetDamageRangeFlags(PlantWeapon::WEAPON_PRIMARY);
		thePlant->mApp->PlayFoley(FoleyType::FOLEY_THROW);
		Log(thePlant, std::format("shadowPeashooter.pea type {}", s.projectileType));
		return true;
	}
	// 0x7b0005..0x7b00db: fume clouds along the row, then DoRowAreaDamage with 30 / flags 6 (hook 274 0x7a915c)
	const int aRenderOrder = Board::MakeRenderOrder(RenderLayer::RENDER_LAYER_PARTICLE, thePlant->mRow, 0);
	for (int i = 0; i < s.beamPuffs; ++i)
		thePlant->mApp->AddPvzpParticle(thePlant->mX + 66 + i * s.beamPuffStepPx, thePlant->mY + 31, aRenderOrder,
			ParticleEffect::PARTICLE_FUMECLOUD);
	const int aRangeFlags = thePlant->GetDamageRangeFlags(PlantWeapon::WEAPON_PRIMARY);
	const Rect aAttackRect = thePlant->GetPlantAttackRect(PlantWeapon::WEAPON_PRIMARY);
	int aHits = 0;
	for (Zombie* aZombie : aBoard->mZombies) {
		if (aZombie->mDead)
			continue;
		const int aDiffY = aZombie->mZombieType == ZombieType::ZOMBIE_BOSS ? 0 : aZombie->mRow - thePlant->mRow;
		if (aDiffY != 0 || aZombie->mOnHighGround != thePlant->IsOnHighGround() || !aZombie->EffectedByDamage(aRangeFlags))
			continue;
		if (GetRectOverlap(aAttackRect, aZombie->GetZombieRect()) <= 0)
			continue;
		aZombie->TakeDamage(s.beamDamage, static_cast<unsigned int>(s.beamDamageFlags));
		thePlant->mApp->PlayFoley(FoleyType::FOLEY_SPLAT);
		++aHits;
	}
	thePlant->mApp->PlayFoley(FoleyType::FOLEY_FUME);
	Log(thePlant, std::format("shadowPeashooter.beam damage {} flags {} hits {} puffs {}", s.beamDamage, s.beamDamageFlags,
		aHits, s.beamPuffs));
	return true;
}

// ---- Mystic-Ice Shooter 0x55 (UpdateAbilities 0x7a54a0 -> 0xc04760) ------------------------------------------
bool UpdateMysticIce(Plant* thePlant) {
	Board* aBoard = thePlant->mBoard;
	const int aRange = u::MysticIceVelRollRange();
	if (aRange <= 0)
		return false;
	const u::MysticIceShot s = u::MysticIceTickShot(thePlant->mX, thePlant->mY, Sexy::Rand(aRange));
	if (s.fires) {
		if (Projectile* aShot = AddModShot(aBoard, s.x, s.y, 0, thePlant->mRow, s.projectileType)) {
			aShot->mMotionType = static_cast<ProjectileMotion>(s.motion);
			aShot->mVelX = s.velX;
			aShot->mVelY = s.velY;
			aShot->mDamageRangeFlags = s.damageRangeFlags;
		}
		if (thePlant->mLaunchRate % 100 == 0)
			Log(thePlant, std::format("mysticIce.snowPea vx {:.2f} vy {:.3f} life {}", s.velX, s.velY, thePlant->mLaunchRate));
	}
	int aConverted = 0; // 0xc047b7..0xc047e1: every plain pea on the board becomes a snow pea
	for (Projectile* aShot : aBoard->mProjectiles) {
		if (aShot->mPvzu.modType == 0 &&
			u::MysticIceConvertsProjectile(static_cast<int>(aShot->mProjectileType), aShot->mHitTorchwoodGridX, aShot->mDead)) {
			aShot->mProjectileType = static_cast<ProjectileType>(u::MysticIceConvertedType());
			++aConverted;
		}
	}
	if (aConverted > 0)
		Log(thePlant, std::format("mysticIce.convertsPeas {}", aConverted));
	const u::MysticIceLife aLife = u::MysticIceLifeTick(thePlant->mLaunchRate); // 0xc047e7
	thePlant->mLaunchRate = aLife.launchRate;
	if (!aLife.dies)
		return true;
	if (aLife.deathParticle >= 0)
		thePlant->mApp->AddPvzpParticle(thePlant->mX + 40, thePlant->mY + 40,
			Board::MakeRenderOrder(RenderLayer::RENDER_LAYER_TOP, 0, 0), static_cast<ParticleEffect>(aLife.deathParticle));
	struct Spot { int col, row; };
	std::vector<Spot> aUpgrades; // 0xc04823..0xc04864: live Peashooters (seed 0, not mod plants) become Snow Peas
	const int aTo = u::MysticIceUpgradeSeed(SeedType::SEED_PEASHOOTER);
	for (Plant* aPlant : aBoard->mPlants)
		if (!aPlant->mDead && aPlant != thePlant && aPlant->mPvzu.modSeed == 0 && aTo >= 0 &&
			u::MysticIceUpgradeSeed(static_cast<int>(aPlant->mSeedType)) == aTo) {
			aUpgrades.push_back({aPlant->mPlantCol, aPlant->mRow});
			aPlant->Die();
		}
	for (const Spot& s : aUpgrades)
		aBoard->AddPlant(s.col, s.row, static_cast<SeedType>(aTo), SeedType::SEED_NONE);
	Log(thePlant, std::format("mysticIce.dies particle {} peashootersUpgraded {}", aLife.deathParticle, aUpgrades.size()));
	thePlant->Die();
	return true;
}

// ---- Primal Repeater 0x57 (Fire hook 349 -> pea, hook 352 -> 0xc10ba0 roll per pea) ----------------------------
bool FirePrimalRepeater(Plant* thePlant, int theRow) {
	int aOffsetX, aOffsetY;
	thePlant->GetPeaHeadOffset(aOffsetX, aOffsetY); // Repeater's muzzle (the exe compares seed - 0x50, hook 351)
	const int aX = thePlant->mX + aOffsetX + 24, aY = thePlant->mY + aOffsetY - 33 + PotDy(thePlant);
	const int aRoll = Roll(u::PrimalRepeaterRollRange());
	const int aType = u::PrimalRepeaterShotType(aRoll, ProjectileType::PROJECTILE_PEA);
	thePlant->mApp->PlayFoley(FoleyType::FOLEY_THROW);
	Projectile* aShot = AddModShot(thePlant->mBoard, aX, aY, thePlant->mRenderOrder - 1, theRow, aType);
	if (aShot)
		aShot->mDamageRangeFlags = thePlant->GetDamageRangeFlags(PlantWeapon::WEAPON_PRIMARY);
	if (aType != ProjectileType::PROJECTILE_PEA)
		Log(thePlant, std::format("primalRepeater.{} roll {}", aType == u::kShotKnockback ? "knockbackPea" : "poisonPea", aRoll));
	return true;
}

// ---- Dream Starfruit 0x6d (0x959d60 -> vanilla StarFruitFire, then 0x959e60 / 0xc00e80 per star) -------------
bool FireDreamStarfruit(Plant* thePlant) {
	Board* aBoard = thePlant->mBoard;
	thePlant->mApp->PlayFoley(FoleyType::FOLEY_THROW);
	const int aKind = u::DreamStarfruitStarKind(thePlant->mPvzu.starColour);
	const int aReturn = u::DreamStarfruitReturnTimer(Shadowed(thePlant));
	const float aAngleX = cos(DEG_TO_RAD(30.0f)) * 3.33f, aAngleY = sin(DEG_TO_RAD(30.0f)) * 3.33f;
	const float aVel[5][2] = {{-3.33f, 0.0f}, {0.0f, 3.33f}, {0.0f, -3.33f}, {aAngleX, aAngleY}, {aAngleX, -aAngleY}};
	for (const auto& v : aVel) { // vanilla StarFruitFire's five stars; no extra-star roll (hook 287 checks raw 0x1d)
		Projectile* aStar = aBoard->AddProjectile(thePlant->mX + 25, thePlant->mY + 25, thePlant->mRenderOrder - 1, thePlant->mRow,
			ProjectileType::PROJECTILE_STAR);
		aStar->mDamageRangeFlags = thePlant->GetDamageRangeFlags(PlantWeapon::WEAPON_PRIMARY);
		aStar->mMotionType = ProjectileMotion::MOTION_STAR;
		aStar->mVelX = v[0];
		aStar->mVelY = v[1];
		FogStarfruitStar(aStar);   // hook 286 inside StarFruitFire: x1.5 speed (and +0x88, cleared below)
		aStar->mPvzu.starCurve = u::kStarCurveNone; // 0x959e6d: +0x88 = 0 (and +0x80 = kind: no curve either way)
		aStar->mPvzu.field80 = aKind;
		if (aReturn > 0)
			aStar->mClickBackoffCounter = aReturn; // 0xc00e9a: +0x64 = 30 in shadow state
	}
	Log(thePlant, std::format("dreamStarfruit.stars kind {} colour 0x{:06x}{}", aKind, u::StarKindColour(aKind),
		aReturn > 0 ? std::format(" homing after {} cs", aReturn) : std::string()));
	return true;
}

// ---- Dusk-pult 0x70 (Fire 0x7a528e type 2 -> 0x91040b type 0x21; shadow 0x7a50b0) -----------------------------
Projectile* DuskLob(Plant* thePlant, Zombie* theTarget, int theRow, int theWeapon) {
	const int aX = thePlant->mX + 5, aY = thePlant->mY - 12 + PotDy(thePlant); // Cabbage-pult's hand
	Projectile* aShot = AddModShot(thePlant->mBoard, aX, aY, thePlant->mRenderOrder - 1, theRow,
		u::DuskPultProjectileType(kDuskPult, ProjectileType::PROJECTILE_CABBAGE));
	if (aShot == nullptr)
		return nullptr;
	aShot->mDamageRangeFlags = thePlant->GetDamageRangeFlags(static_cast<PlantWeapon>(theWeapon));
	Lob(aShot, thePlant, theTarget, aX, aY);
	return aShot;
}

bool FireDuskPult(Plant* thePlant, Zombie* theTarget, int theRow, int theWeapon) {
	thePlant->mApp->PlayFoley(FoleyType::FOLEY_THROW);
	DuskLob(thePlant, theTarget, theRow, theWeapon);
	const u::DuskPultVolley v = u::DuskPultExtraRows(kDuskPult, Shadowed(thePlant));
	std::string aExtra;
	Board* aBoard = thePlant->mBoard;
	for (int aSide : {-1, 1}) { // 0x7a50c8 row - 1 (moved up 80, raised 80), 0x7a5105 row + 1 (not moved)
		if ((aSide < 0 && !v.above) || (aSide > 0 && !v.below))
			continue;
		const int aRow = theRow + aSide;
		if (!aBoard->RowCanHaveZombies(aRow))
			continue;
		Zombie* aZombie = thePlant->FindTargetZombie(aRow, static_cast<PlantWeapon>(theWeapon));
		if (aZombie == nullptr)
			continue;
		if (Projectile* aShot = DuskLob(thePlant, aZombie, aRow, theWeapon); aShot && aSide < 0) {
			aShot->mPosY += v.aboveDy;
			aShot->mPosZ += v.aboveDz;
		}
		aExtra += std::format(" +row {}", aRow);
	}
	Log(thePlant, std::format("duskPult.shard 0x21{}", aExtra));
	return true;
}

// ---- Butter-pult 0x72 (Fire hook 349 0x7a529b -> butter; FindTargetAndFire hook 283 butter pose) ---------------
void ButterPose(Plant* thePlant) {
	if (Reanimation* aBody = thePlant->mApp->ReanimationTryToGet(thePlant->mBodyReanimID)) {
		aBody->AssignRenderGroupToPrefix("Cornpult_butter", RENDER_GROUP_NORMAL);
		aBody->AssignRenderGroupToPrefix("Cornpult_kernal", RENDER_GROUP_HIDDEN);
	}
}

bool FireButterPult(Plant* thePlant, Zombie* theTarget, int theRow, int theWeapon) {
	const int aX = thePlant->mX + 12, aY = thePlant->mY - 56 + PotDy(thePlant); // Kernel-pult's butter hand
	thePlant->mApp->PlayFoley(FoleyType::FOLEY_THROW);
	Projectile* aShot = AddModShot(thePlant->mBoard, aX, aY, thePlant->mRenderOrder - 1, theRow,
		u::ButterPultProjectile(ProjectileType::PROJECTILE_KERNEL));
	if (aShot == nullptr)
		return true;
	aShot->mDamageRangeFlags = u::ButterPultDamageRangeFlags(thePlant->GetDamageRangeFlags(static_cast<PlantWeapon>(theWeapon)));
	aShot->mPvzu.butterStreak = u::ButterPultButterStreak(); // no Kernel streak carry (0xc10bc7 tests seed 0x22)
	Lob(aShot, thePlant, theTarget, aX, aY);
	Log(thePlant, std::format("butterPult.butter flags {}", aShot->mDamageRangeFlags));
	return true;
}

// ---- Sniper 0x78 (UpdateAbilities 0x7a5400 -> 0x7b1300; pick 0x7b1200) -----------------------------------------
u::SniperCandidate Candidate(Zombie* z) {
	u::SniperCandidate c;
	c.x = z->mX;
	c.zombieType = static_cast<int>(z->mZombieType);
	c.variant = z->mPvzu.variant;
	c.bodyHp = z->mBodyHealth;
	c.bodyMaxHp = z->mBodyMaxHealth;
	c.helmType = static_cast<int>(z->mHelmType);
	c.helmHp = z->mHelmHealth;
	c.shieldType = static_cast<int>(z->mShieldType);
	c.shieldHp = z->mShieldHealth;
	c.flyingHp = z->mFlyingHealth;
	c.flyingMaxHp = z->mFlyingMaxHealth;
	c.hasHead = z->mHasHead;
	c.effectedByDamage = z->EffectedByDamage(1U);
	return c;
}

Zombie* SniperPick(Plant* thePlant) {
	std::vector<Zombie*> aZombies;
	std::vector<u::SniperCandidate> aCandidates;
	for (Zombie* aZombie : thePlant->mBoard->mZombies) {
		if (aZombie->mDead || aZombie->IsDeadOrDying())
			continue;
		aZombies.push_back(aZombie);
		aCandidates.push_back(Candidate(aZombie));
	}
	const int aBest = u::SniperPickTarget(aCandidates, thePlant->mX + thePlant->mWidth);
	return aBest >= 0 ? aZombies[aBest] : nullptr;
}

bool UpdateSniper(Plant* thePlant) {
	Board* aBoard = thePlant->mBoard;
	thePlant->mStateCountdown = u::SniperCountdownAfterTick(thePlant->mStateCountdown); // 0x7b1314: 0 -> 500
	const int aRightX = thePlant->mX + thePlant->mWidth;
	Zombie* aTarget = aBoard->ZombieTryToGet(thePlant->mTargetZombieID);
	if (aTarget && (aTarget->IsDeadOrDying() || !u::SniperKeepsTarget(Candidate(aTarget), aRightX)))
		aTarget = nullptr; // 0x7b135e..0x7b13dd: drop it and pick again this tick
	if (aTarget == nullptr) {
		aTarget = SniperPick(thePlant);
		thePlant->mTargetZombieID = aTarget ? aBoard->ZombieGetID(aTarget) : ZombieID::ZOMBIEID_NULL;
	}
	if (aTarget == nullptr)
		return true;
	const u::SniperTick t = u::SniperTimer(thePlant->mStateCountdown);
	if (t.foley > 0) // phone report 2026-11-23: the exe's only shot effects are these sounds (warning 0x52, shot 0x37)
		thePlant->mApp->PlayFoley(static_cast<FoleyType>(t.foley));
	if (t.warn)
		Log(thePlant, std::format("sniper.aim zombie type {} row {} x {} hp {}", static_cast<int>(aTarget->mZombieType),
			aTarget->mRow, aTarget->mX, u::SniperHpSum(Candidate(aTarget))));
	if (t.fire) {
		const int aBefore = u::SniperHpSum(Candidate(aTarget));
		aTarget->TakeDamage(t.damage, 0U); // 0x7b13b3: TakeDamage(500, 0)
		aTarget->AddAttachedParticle(40, 40, ParticleEffect::PARTICLE_PEA_SPLAT); // port addition: a mark where it hits
		Log(thePlant, std::format("sniper.shot {} damage zombie type {} row {} hp {} -> {}", t.damage,
			static_cast<int>(aTarget->mZombieType), aTarget->mRow, aBefore, u::SniperHpSum(Candidate(aTarget))));
		if (!aTarget->mHasHead || aTarget->IsDeadOrDying()) { // after the shot: keep it while it has its head
			aTarget = SniperPick(thePlant);
			thePlant->mTargetZombieID = aTarget ? aBoard->ZombieGetID(aTarget) : ZombieID::ZOMBIEID_NULL;
		}
	}
	return true;
}

// ---- Element-shroom 0x7a (UpdateAbilities 0x7b1450; pulses 0x7b1700 / 0x7a9070) ---------------------------------
int ElementState(const Plant* thePlant) {
	return u::ElementShroomState(thePlant->mPvzu.elementBits, thePlant->mPvzu.elementCount);
}

void UpdateElement(Plant* thePlant) {
	u::PlantState& s = thePlant->mPvzu;
	const u::ElementTick t = u::ElementShroomTick(thePlant->mStateCountdown, s.elementCount);
	if (t.resetCountdownTo >= 0)
		thePlant->mStateCountdown = t.resetCountdownTo;
	if (t.gain) {
		const u::ElementGain g = u::ElementShroomGainElement(s.elementBits, s.elementMode, s.elementCount);
		s.elementBits = g.bits;
		s.elementCount = g.count;
		Log(thePlant, std::format("elementShroom.gain {} bits {}{}{} state {} count {}", s.elementMode ? "ice" : "fire",
			(s.elementBits >> 2) & 1, (s.elementBits >> 1) & 1, s.elementBits & 1, ElementState(thePlant), s.elementCount));
	}
}

bool FireElementPulse(Plant* thePlant) {
	if (!u::ElementShroomPulseAt(thePlant->mShootingCounter)) // 3 pulses (126 / 98 / 70), not Gloom's 4th at 42
		return true;
	Board* aBoard = thePlant->mBoard;
	const int aState = ElementState(thePlant);
	const int aReach = u::ElementShroomRowReach(aState);
	const u::ElementRect aOwn = u::ElementShroomAttackRect(aState, thePlant->mX, thePlant->mY);
	const Rect aRect = aOwn.own ? Rect(aOwn.x, aOwn.y, aOwn.w, aOwn.h) : thePlant->GetPlantAttackRect(PlantWeapon::WEAPON_PRIMARY);
	const int aRangeFlags = thePlant->GetDamageRangeFlags(PlantWeapon::WEAPON_PRIMARY);
	int aHits = 0, aFrozen = 0, aPushed = 0, aStripped = 0;
	for (Zombie* aZombie : aBoard->mZombies) {
		if (aZombie->mDead)
			continue;
		const int aDiffY = aZombie->mZombieType == ZombieType::ZOMBIE_BOSS ? 0 : aZombie->mRow - thePlant->mRow;
		if (aDiffY < -aReach || aDiffY > aReach || aZombie->mOnHighGround != thePlant->IsOnHighGround() ||
			!aZombie->EffectedByDamage(aRangeFlags) || GetRectOverlap(aRect, aZombie->GetZombieRect()) <= 0)
			continue;
		const bool aBoss = ModZombieBossClass(aZombie);
		const int aIceRoll = Roll(u::ElementShroomIceRollRange(aState));
		const u::ElementHit h = u::ElementShroomHit(aState, 20, 2, aIceRoll, static_cast<int>(aZombie->mZombieType), aZombie->mX, aBoss);
		if (h.thaw) { // state 1: thaws (0x532350, chill 0)
			if (aZombie->mIceTrapCounter > 0)
				aZombie->RemoveIceTrap();
			if (aZombie->mChilledCounter > 0) {
				aZombie->mChilledCounter = 0;
				aZombie->UpdateAnimSpeed();
			}
		}
		if (h.freeze) {
			aZombie->HitIceTrap();
			aZombie->mIceTrapCounter = h.iceTrapCs;
			++aFrozen;
		}
		if (h.pushPx != 0) {
			aZombie->mPosX += static_cast<float>(h.pushPx);
			++aPushed;
		}
		if (h.damage > 0 && !ModModeRowDamage(thePlant, aZombie, h.damage, static_cast<unsigned int>(h.damageFlags))) // mod mode 0x8e (hook 275)
			aZombie->TakeDamage(h.damage, static_cast<unsigned int>(h.damageFlags));
		if (h.stripRollRange > 0 && !aZombie->IsDeadOrDying()) {
			const int aStrip = u::ElementShroomFireStrip(Sexy::Rand(h.stripRollRange), aZombie->mHelmType != HelmType::HELMTYPE_NONE,
				aZombie->mShieldType != ShieldType::SHIELDTYPE_NONE);
			if (aStrip == u::kElementStripHelm)
				aZombie->DropHelm(0U);
			else if (aStrip == u::kElementStripShield)
				aZombie->DropShield(0U);
			aStripped += aStrip != u::kElementStripNone;
		}
		thePlant->mApp->PlayFoley(FoleyType::FOLEY_SPLAT);
		++aHits;
	}
	const u::ElementHeal aHeal = u::ElementShroomPulseHeal(aState, thePlant->mPlantHealth, thePlant->mPlantMaxHealth);
	thePlant->mPlantHealth = aHeal.hp;
	thePlant->mPlantMaxHealth = aHeal.maxHp;
	Log(thePlant, std::format("elementShroom.pulse state {} rows +-{} hits {} frozen {} pushed {} stripped {} hp {}/{}", aState,
		aReach, aHits, aFrozen, aPushed, aStripped, aHeal.hp, aHeal.maxHp));
	return true;
}

// ---- Spirit-tail 0x7b (Fire 0xc106b0 roll; 0x467381 -> 0xc10600 homing + extras) -------------------------------
bool FireSpiritTail(Plant* thePlant, Zombie* theTarget, int theRow, int theWeapon) {
	const int aRange = u::SpiritTailRollRange();
	int aType = -1, aRoll = 0;
	for (int i = 0; i < 4096 && aType < 0 && aRange > 0; ++i) // re-rolled until accepted (15 of 36)
		aType = u::SpiritTailProjectileType(aRoll = Sexy::Rand(aRange));
	if (aType < 0)
		aType = ProjectileType::PROJECTILE_SPIKE;
	if (static const int aDevType = CmdOptionInt("-pvzu-dev-spirit-type", -1); aDevType >= 0) // dev: force one type (art checks)
		aType = aDevType;
	const int aX = thePlant->mX + 20, aY = thePlant->mY - 3 + PotDy(thePlant); // Cattail's
	thePlant->mApp->PlayFoley(FoleyType::FOLEY_THROW);
	Projectile* aShot = AddModShot(thePlant->mBoard, aX, aY, thePlant->mRenderOrder - 1, theRow, aType);
	if (aShot == nullptr)
		return true;
	aShot->mDamageRangeFlags = thePlant->GetDamageRangeFlags(static_cast<PlantWeapon>(theWeapon));
	if (u::SpiritTailMotion() == ProjectileMotion::MOTION_HOMING) {
		aShot->mVelX = 2.0f;
		aShot->mMotionType = ProjectileMotion::MOTION_HOMING;
		aShot->mTargetZombieID = thePlant->mBoard->ZombieGetID(theTarget);
	}
	const int aExtraRoll = Roll(u::SpiritTailExtraRollRange(aType));
	const int aField80 = u::SpiritTailProjectileField80(aType, aExtraRoll);
	if (aType == ProjectileType::PROJECTILE_CABBAGE)
		aShot->mPvzu.bigCabbage = aField80 != 0; // the Roof big cabbage (+0x80 = 1)
	else if (aField80 != 0)
		aShot->mPvzu.field80 = aField80;       // star kind 0..5, prism damage 475
	Log(thePlant, std::format("spiritTail.shot type 0x{:02x} (roll {}) field80 {}", aType, aRoll, aField80));
	return true;
}

// ---- Loaded Cannon 0x7f (0xc04b53 -> 0x461069; Fire 0xc04b80 / hook 352 -> 0xc117e0 ammo) ----------------------
bool FireLoadedCannon(Plant* thePlant, int theRow) {
	const int aX = thePlant->mX - 44, aY = thePlant->mY - 184; // vanilla Cob Cannon Fire
	thePlant->mApp->PlayFoley(FoleyType::FOLEY_THROW);
	Projectile* aCob = thePlant->mBoard->AddProjectile(aX, aY, thePlant->mRenderOrder - 1, theRow, ProjectileType::PROJECTILE_COBBIG);
	aCob->mVelX = 0.001f;
	aCob->mDamageRangeFlags = thePlant->GetDamageRangeFlags(PlantWeapon::WEAPON_PRIMARY);
	aCob->mMotionType = ProjectileMotion::MOTION_LOBBED;
	aCob->mVelY = 0.0f;
	aCob->mAccZ = 0.0f;
	aCob->mVelZ = -8.0f;
	aCob->mCobTargetX = thePlant->mTargetX - 40;
	aCob->mCobTargetRow = thePlant->mBoard->PixelToGridYKeepOnBoard(thePlant->mTargetX, thePlant->mTargetY);
	aCob->mPvzu.cobAmmo = thePlant->mPvzu.cannonAmmo;
	Log(thePlant, std::format("loadedCannon.cob ammo 0x{:02x} burnCap {}", thePlant->mPvzu.cannonAmmo,
		u::LoadedCannonCobBurnCap(thePlant->mPvzu.cannonAmmo)));
	return true;
}

// Applies a load (Kernel-pult / Doom-shroom) to an arming Loaded Cannon (0xc11870). false = not loadable.
bool LoadCannon(Plant* theCannon, SeedType theSeed) {
	const u::CannonLoad aLoad = u::LoadedCannonLoad(static_cast<int>(theCannon->mState), static_cast<int>(theSeed));
	if (aLoad == u::CannonLoad::None)
		return false;
	if (aLoad == u::CannonLoad::Kernel) { // 0xc11810: ammo 0x22, LOADING (the vanilla LOADING -> READY follows)
		theCannon->mPvzu.cannonAmmo = static_cast<int>(theSeed);
		theCannon->mState = PlantState::STATE_COBCANNON_LOADING;
		theCannon->PlayBodyReanim("anim_charge", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 20, 12.0f);
		Log(theCannon, "loadedCannon.load kernel (ammo 0x22, next cob burn cap 3000)");
		return true;
	}
	// Doom: the cannon becomes a Doom-shroom and blows at once with burn cap 3600; hook 345: no crater, graves removed
	Board* aBoard = theCannon->mBoard;
	const int aCol = theCannon->mPlantCol, aRow = theCannon->mRow;
	theCannon->mPvzu.cannonAmmo = static_cast<int>(theSeed);
	theCannon->mSeedType = SeedType::SEED_DOOMSHROOM;
	Log(theCannon, std::format("loadedCannon.load doom (burn cap {})", u::LoadedCannonDoomBurnCap()));
	gBurnCap = u::LoadedCannonDoomBurnCap();
	theCannon->DoSpecial();
	gBurnCap = 0;
	if (u::LoadedCannonDoomClearsGraves()) {
		int aGraves = 0;
		for (GridItem* aItem : aBoard->mGridItems) {
			if (aItem->mGridItemType == GridItemType::GRIDITEM_CRATER && aItem->mGridX == aCol && aItem->mGridY == aRow)
				aItem->GridItemDie();
			else if (aItem->mGridItemType == GridItemType::GRIDITEM_GRAVESTONE) {
				aItem->GridItemDie();
				++aGraves;
			}
		}
		u::LogPlantAbility(static_cast<int>(aBoard->mMainCounter), std::format("MOD Loaded Cannon (0x7f) col {} row {}: "
			"loadedCannon.doom no crater, {} graves removed", aCol, aRow, aGraves));
	}
	return true;
}

Plant* CannonAt(Board* theBoard, int theGridX, int theGridY) {
	for (Plant* aPlant : theBoard->mPlants)
		if (!aPlant->mDead && Mod(aPlant) == kCannon && aPlant->mRow == theGridY &&
			(aPlant->mPlantCol == theGridX || aPlant->mPlantCol + 1 == theGridX))
			return aPlant;
	return nullptr;
}

// ---- Mod shot impact helpers -------------------------------------------------------------------------------------
void Splat(Projectile* theShot, Zombie* theZombie) {
	float aX = theShot->mPosX + 12.0f, aY = theShot->mPosY + 12.0f;
	const float aLastX = theShot->mPosX - theShot->mVelX, aLastY = theShot->mPosY + theShot->mPosZ - theShot->mVelY - theShot->mVelZ;
	ParticleEffect aEffect = ParticleEffect::PARTICLE_NONE;
	switch (theShot->mProjectileType) {
	case ProjectileType::PROJECTILE_PEA: aX -= 15.0f; aEffect = ParticleEffect::PARTICLE_PEA_SPLAT; break;
	case ProjectileType::PROJECTILE_SNOWPEA: aX -= 15.0f; aEffect = ParticleEffect::PARTICLE_SNOWPEA_SPLAT; break;
	case ProjectileType::PROJECTILE_STAR: aEffect = ParticleEffect::PARTICLE_STAR_SPLAT; break;
	case ProjectileType::PROJECTILE_PUFF: aX -= 20.0f; aEffect = ParticleEffect::PARTICLE_PUFF_SPLAT; break;
	case ProjectileType::PROJECTILE_CABBAGE:
		aX = aLastX - 38.0f;
		aY = aLastY + 23.0f;
		aEffect = ParticleEffect::PARTICLE_CABBAGE_SPLAT;
		break;
	default: break;
	}
	if (aEffect == ParticleEffect::PARTICLE_NONE)
		return;
	if (theZombie) {
		float aPosX = aX + 52.0f - theZombie->mX, aPosY = aY - theZombie->mY;
		if (theShot->mPosX > theZombie->mX + 40 && theShot->mMotionType != ProjectileMotion::MOTION_LOBBED)
			aPosX -= 60.0f;
		theZombie->AddAttachedParticle(aPosX, std::clamp(aPosY, 20.0f, 100.0f), aEffect);
	} else {
		theShot->mApp->AddPvzpParticle(aX, aY, theShot->mRenderOrder + 1, aEffect);
	}
}

void StarKindEffects(Projectile* theShot, Zombie* theZombie, int theKind) {
	if (theZombie->IsDeadOrDying())
		return;
	u::StarZombie z;
	z.type = static_cast<int>(theZombie->mZombieType);
	z.variant = theZombie->mPvzu.variant;
	z.subVariant = theZombie->mPvzu.subVariant;
	z.bodyHp = theZombie->mBodyHealth;
	z.scale = theZombie->mScaleZombie;
	z.canBeFrozen = theZombie->CanBeFrozen();
	z.iZombie = theShot->mApp->IsIZombieLevel();
	const int aRange = u::StarKindImpactRollRange(theKind, z);
	const int aRoll = Roll(aRange);
	const u::StarImpact s = u::StarKindImpact(theKind, z, aRoll);
	std::string aWhat;
	if (s.freeze) { theZombie->HitIceTrap(); aWhat += " freeze"; }
	if (s.chill) { theZombie->ApplyChill(false); aWhat += " chill"; }
	if (s.dropHelm && theZombie->mHelmType != HelmType::HELMTYPE_NONE) { theZombie->DropHelm(0U); aWhat += " helmOff"; }
	if (s.hypnotise) { // 0x95a07a: weaken cleared, cold removed, hypnotised
		theZombie->mPvzu.weakenCs = 0;
		theZombie->RemoveColdEffects();
		theZombie->StartMindControlled();
		aWhat += " hypnotised";
	}
	if (s.blowAway) { theZombie->mBlowingAway = true; aWhat += " blownAway"; }
	if (s.weakenAddCs > 0) { ModZombieAddWeakened(theZombie, s.weakenAddCs); aWhat += std::format(" weakened {} cs", theZombie->mPvzu.weakenCs); }
	if (!aWhat.empty() || aRange > 0)
		LogShot(theShot, std::format("starKind {} roll {}/{}:{}", theKind, aRoll, aRange, aWhat.empty() ? " nothing" : aWhat));
}

void DuskSplash(Projectile* theShot) {
	Board* aBoard = theShot->mBoard;
	if (PvzpParticleSystem* aPuff = theShot->mApp->AddPvzpParticle(theShot->mX, theShot->mY + 32, theShot->mRenderOrder + 1,
		ParticleEffect::PARTICLE_FIREBALL_DEATH))
		aPuff->OverrideColor(nullptr, Sexy::Color(0x55, 0, 0x88)); // 0x910457: particle 76 coloured
	const u::DuskSplash s = u::DuskShardSplash(u::kShotDusk);
	const Rect aRect = theShot->GetProjectileRect();
	int aHits = 0;
	for (Zombie* aZombie : aBoard->mZombies) {
		if (aZombie->mDead || !aZombie->EffectedByDamage(1U))
			continue;
		const Rect aZombieRect = aZombie->GetZombieRect();
		if (!u::DuskShardSplashReaches(u::kShotDusk, theShot->mRow, aRect.mX, aRect.mWidth, aZombie->mRow, aZombieRect.mX,
			aZombieRect.mWidth, aZombie->mMindControlled))
			continue;
		aZombie->TakeDamage(s.damage, static_cast<unsigned int>(s.damageFlags)); // 0xc06150..0xc06167
		aZombie->mPosX += s.pushPx;
		ModZombieAddWeakened(aZombie, s.weakenCs); // 0xc0615c add word [zombie+0x1a] (Mod/Engine/ModZombieStatus.h)
		++aHits;
	}
	LogShot(theShot, std::format("duskShard.splash damage {} push {:.0f} weaken {} cs hits {}", s.damage, s.pushPx, s.weakenCs, aHits));
}

} // namespace

// ==== Dispatch ======================================================================================================

bool IsShooterModSeed(SeedType theModSeed) {
	switch (static_cast<int>(theModSeed)) {
	case kShadowPea: case kMysticIce: case kPrimalRepeater: case kDreamStar: case kDuskPult:
	case kButterPult: case kSniper: case kElement: case kSpiritTail: case kCannon:
		return true;
	default:
		return false;
	}
}

void ShooterOnPlanted(Plant* thePlant) {
	const int aMod = Mod(thePlant);
	if (const u::ModPlant* p = u::FindModPlant(aMod); p && p->shooter && p->launchRate > 0)
		thePlant->mLaunchRate = p->launchRate; // the mod table's launch rate (Mystic-Ice: its 500-tick life)
	if (aMod == kButterPult)
		ButterPose(thePlant);
	if (aMod == kCannon)
		thePlant->mPvzu.cannonAmmo = 0;
	if (aMod == kElement && thePlant->mIsAsleep) // 0x7a4770: the day-sleep list holds vanilla mushrooms only
		thePlant->SetSleeping(false);
}

bool ShooterUpdate(Plant* thePlant) {
	switch (Mod(thePlant)) {
	case kMysticIce: return UpdateMysticIce(thePlant);
	case kSniper: return UpdateSniper(thePlant);
	case kElement: UpdateElement(thePlant); return false;
	case kButterPult: ButterPose(thePlant); return false;
	case kDreamStar: // 0x959de0: the colour is re-rolled while (counter & 0xc) == 0 (the exe rolls in Draw; here in Update)
		if (const int r = u::DreamStarfruitColourRollRange(static_cast<int>(thePlant->mBoard->mMainCounter)); r > 0)
			thePlant->mPvzu.starColour = Sexy::Rand(r);
		return false;
	case kCannon: // 0xc04b53: ARMING never ends by itself; it waits for a load
		return !u::LoadedCannonArmsByItself() && thePlant->mState == PlantState::STATE_COBCANNON_ARMING;
	default:
		return false;
	}
}

bool ShooterFire(Plant* thePlant, Zombie* theTargetZombie, int theRow, int thePlantWeapon) {
	switch (Mod(thePlant)) {
	case kShadowPea: return FireShadowPea(thePlant, theRow);
	case kPrimalRepeater: return FirePrimalRepeater(thePlant, theRow);
	case kDreamStar: return FireDreamStarfruit(thePlant);
	case kDuskPult: return FireDuskPult(thePlant, theTargetZombie, theRow, thePlantWeapon);
	case kButterPult: return FireButterPult(thePlant, theTargetZombie, theRow, thePlantWeapon);
	case kElement: return FireElementPulse(thePlant);
	case kSpiritTail: return FireSpiritTail(thePlant, theTargetZombie, theRow, thePlantWeapon);
	case kCannon: return FireLoadedCannon(thePlant, theRow);
	case kMysticIce: case kSniper: return true; // never fire (0xc04760 / 0x7b1300 replace the shooter)
	default: return false;
	}
}

// ==== Engine sites ==================================================================================================

Zombie* ShooterExtraTarget(Plant* thePlant, int theRow, int thePlantWeapon) {
	if (Mod(thePlant) != kDuskPult || !u::DuskPultLooksInAdjacentRows(kDuskPult, Shadowed(thePlant)))
		return nullptr;
	for (int aRow : {theRow + 1, theRow - 1}) // hook 279 0x7a5150: row + 1 first
		if (thePlant->mBoard->RowCanHaveZombies(aRow))
			if (Zombie* aZombie = thePlant->FindTargetZombie(aRow, static_cast<PlantWeapon>(thePlantWeapon)))
				return aZombie;
	return nullptr;
}

bool ShooterStarfruitAlwaysTargets(Plant* thePlant) {
	return Mod(thePlant) == kDreamStar && u::DreamStarfruitAlwaysTargets(Shadowed(thePlant));
}

bool ShooterPlantClicked(Plant* thePlant) {
	if (Mod(thePlant) != kElement || !thePlant->IsInPlay())
		return false;
	thePlant->mPvzu.elementMode = u::ElementShroomToggleMode(thePlant->mPvzu.elementMode); // 0x7a8ef1: +0xcd ^= 1
	Log(thePlant, std::format("elementShroom.toggle -> {}", thePlant->mPvzu.elementMode ? "ice" : "fire"));
	return true;
}

bool ShooterPlantColor(const Plant* thePlant, Sexy::Color& theColor) {
	const int aMod = Mod(thePlant);
	if (aMod == kDreamStar) { // 0x959de0: drawn in its current star colour
		if (const uint32_t c = u::StarKindColour(u::DreamStarfruitStarKind(thePlant->mPvzu.starColour)); c != 0) {
			theColor = Sexy::Color((c >> 16) & 0xff, (c >> 8) & 0xff, c & 0xff, 255);
			return true;
		}
	}
	if (aMod == kElement && thePlant->mPvzu.elementMode == 1 && u::ModPlantsOn()) { // 0x7a8f53: ice chosen
		theColor = Sexy::Color(10, 100, 240, 255);
		return true;
	}
	return false;
}

bool ShooterCannonLoadClick(Board* theBoard, int theGridX, int theGridY) {
	CursorObject* aCursor = theBoard->mCursorObject.get();
	const bool aFromBank = aCursor->mCursorType == CursorType::CURSOR_TYPE_PLANT_FROM_BANK;
	const bool aFromCoin = aCursor->mCursorType == CursorType::CURSOR_TYPE_PLANT_FROM_USABLE_COIN;
	if (!aFromBank && !aFromCoin)
		return false;
	const SeedType aSeed = aCursor->mType;
	Plant* aCannon = CannonAt(theBoard, theGridX, theGridY);
	if (aCannon == nullptr ||
		u::LoadedCannonLoad(static_cast<int>(aCannon->mState), static_cast<int>(aSeed)) == u::CannonLoad::None)
		return false;
	if (aFromBank && !theBoard->mApp->mEasyPlantingCheat && !theBoard->HasConveyorBeltSeedBank() &&
		!theBoard->TakeSunMoney(theBoard->GetCurrentPlantCost(aSeed, SeedType::SEED_NONE)))
		return true; // not enough sun: TakeSunMoney flashed the counter
	const u::CannonLoad aLoad = u::LoadedCannonLoad(static_cast<int>(aCannon->mState), static_cast<int>(aSeed));
	LoadCannon(aCannon, aSeed);
	if (aFromBank) {
		SeedPacket& aPacket = theBoard->mSeedBank->mSeedPackets[aCursor->mSeedBankIndex];
		aPacket.WasPlanted();
		aPacket.mRefreshCounter = -u::LoadedCannonPacketPenaltyCs(aLoad); // 0xc04a71: recharge 6000 cs longer
	} else if (Coin* aCoin = theBoard->mCoins.DataArrayTryToGet(static_cast<unsigned int>(aCursor->mCoinID))) {
		aCursor->mCoinID = CoinID::COINID_NULL;
		aCoin->Die();
	}
	theBoard->ClearCursor();
	return true;
}

bool ModShotImpact(Projectile* theShot, Zombie* theZombie) {
	const int aKind = theShot->mProjectileType == ProjectileType::PROJECTILE_STAR ? theShot->mPvzu.field80 : 0;
	if (theShot->mPvzu.modType == 0 && aKind == 0)
		return false;
	const int aExeType = theShot->mPvzu.modType != 0 ? theShot->mPvzu.modType : static_cast<int>(theShot->mProjectileType);
	theShot->PlayImpactSound(theZombie);
	if (theZombie) {
		SmallRulesCountKindStar(theShot); // PVZU Plan 9 R9: a kind star counts for the pity counter too (0xc10000)
		unsigned int aFlags = theShot->GetDamageFlags(theZombie);
		int aDamage = u::ModProjectileDamage(aExeType, theShot->GetProjectileDef().mDamage, theShot->mPvzu.field80,
			theShot->mApp->IsIZombieLevel()); // 0x7b0890
		aDamage += u::StarKindDamageBonus(aKind); // 0x959cf0
		DoorReflect(theShot, theZombie, aDamage, aFlags);
		theZombie->TakeDamage(aDamage, aFlags);
		if (const int aPush = u::PrimalKnockbackPx(aExeType); aPush != 0) { // 0xc10bf0: +30 px, POW
			theZombie->mPosX += static_cast<float>(aPush);
			theShot->mApp->AddPvzpParticle(theZombie->mX + 40, theZombie->mY + 40, theShot->mRenderOrder + 1, ParticleEffect::PARTICLE_POW);
			LogShot(theShot, std::format("knockbackPea {} damage, zombie pushed +{} px to x {:.0f}", aDamage, aPush, theZombie->mPosX));
		}
		if (const int aPoison = u::PrimalPoisonAddCs(aExeType); aPoison > 0) { // 0xc10c71 (Mod/Engine/ModZombieStatus.h)
			ModZombieApplyPoison(theZombie, aPoison);
			LogShot(theShot, std::format("poisonPea {} damage, poison {} cs", aDamage, theZombie->mPvzu.repeaterPoisonCs));
		}
		if (aKind != 0)
			StarKindEffects(theShot, theZombie, aKind);
		else if (aExeType == u::kShotPrism)
			LogShot(theShot, std::format("prism {} damage", aDamage));
	}
	if (aExeType == u::kShotDusk)
		DuskSplash(theShot);
	Splat(theShot, theZombie);
	// kind stars: Die decides (0x959f30, FogStarSurvivesDie): red / grey live through 4 / 2 hits, purple / green die,
	// blue like an ordinary star (254)
	theShot->Die();
	return true;
}

void ModShotUpdate(Projectile* theShot) {
	if (theShot->mProjectileType != ProjectileType::PROJECTILE_STAR || theShot->mPvzu.field80 == 0 ||
		!u::StarKindTurnsHoming(ProjectileType::PROJECTILE_STAR, static_cast<int>(theShot->mMotionType), theShot->mPvzu.field80,
			theShot->mClickBackoffCounter))
		return;
	// hook 403 0xc00ec0: motion 9; its target here = the nearest zombie it can hit (the exe's pick was not decoded)
	Zombie* aBest = nullptr;
	float aBestDist = 0.0f;
	for (Zombie* aZombie : theShot->mBoard->mZombies) {
		if (aZombie->mDead || aZombie->IsDeadOrDying() || aZombie->mMindControlled ||
			!aZombie->EffectedByDamage(static_cast<unsigned int>(theShot->mDamageRangeFlags)))
			continue;
		const Rect r = aZombie->GetZombieRect();
		const float aDist = Distance2D(theShot->mPosX, theShot->mPosY, r.mX + r.mWidth / 2, r.mY + r.mHeight / 2);
		if (aBest == nullptr || aDist < aBestDist) {
			aBest = aZombie;
			aBestDist = aDist;
		}
	}
	theShot->mMotionType = ProjectileMotion::MOTION_HOMING;
	theShot->mTargetZombieID = aBest ? theShot->mBoard->ZombieGetID(aBest) : ZombieID::ZOMBIEID_NULL;
	LogShot(theShot, std::format("starKind {} turns homing{}", theShot->mPvzu.field80,
		aBest ? std::format(" at zombie type {} row {}", static_cast<int>(aBest->mZombieType), aBest->mRow) : std::string(" (no target)")));
}

namespace {
// The mod's own shot image (images/... in the pack, through the asset overlay), loaded once; nullptr when missing.
Sexy::Image* ModShotOwnImage(std::string_view theFile) {
	static std::map<std::string, Sexy::SharedImageRef, std::less<>> aLoaded; // kept alive for the whole run
	auto it = aLoaded.find(theFile);
	if (it == aLoaded.end()) {
		it = aLoaded.emplace(std::string(theFile), gSexyAppBase->GetSharedImage(std::string(theFile))).first;
		if (static_cast<Sexy::Image*>(it->second) == nullptr)
			std::fprintf(stderr, "PVZU: mod shot image %s not found, drawing the tinted stand-in\n", it->first.c_str());
	}
	return static_cast<Sexy::Image*>(it->second);
}
} // namespace

void ModShotLook(const Projectile* theShot, Sexy::Image*& theImage, float& theScale, Sexy::Color& theTint) {
	const int aKind = theShot->mProjectileType == ProjectileType::PROJECTILE_STAR ? theShot->mPvzu.field80 : 0;
	if (theShot->mPvzu.modType == 0 && aKind == 0)
		return;
	if (theShot->mPvzu.modType == u::kShotPrism && Sexy::IMAGE_POOLSPARKLY) { // 0x7a0e4a [0x6a75b8]
		theImage = Sexy::IMAGE_POOLSPARKLY;
		theScale = 1.0f;
	}
	// PVZU Plan 9 R3: Catapult vase 0x12 = IMAGE_SCARY_POT cel 0 (0x7a0df5 [0x6a7ac0]), bug spray 0x20 = IMAGE_BUG_SPRAY
	// (0x7a0e6d [0x6a744c]); both vanilla resources with their cel layout, scale 1, white
	if (Sexy::Image* aPot = theShot->mPvzu.modType == u::kShotVase ? Sexy::IMAGE_SCARY_POT :
	                        theShot->mPvzu.modType == u::kShotBugSpray ? Sexy::IMAGE_BUG_SPRAY : nullptr) {
		theImage = aPot;
		theScale = 1.0f;
		return;
	}
	if (const std::string_view aFile = u::ModShotImageFile(theShot->mPvzu.modType); !aFile.empty()) {
		if (Sexy::Image* aOwn = ModShotOwnImage(aFile)) { // hook 405 0xc10b50: own image, scale 1, white (0x959e90)
			theImage = aOwn;
			theScale = 1.0f;
			return;
		}
	}
	if (const uint32_t c = u::ModShotTint(theShot->mPvzu.modType != 0 ? theShot->mPvzu.modType : 7, aKind); c != 0)
		theTint = Sexy::Color((c >> 16) & 0xff, (c >> 8) & 0xff, c & 0xff, 255);
}

ModCobBurnCapScope::ModCobBurnCapScope(const Projectile* theCob) {
	if (theCob->mPvzu.cobAmmo == 0)
		return;
	if (const int aCap = u::LoadedCannonCobBurnCap(theCob->mPvzu.cobAmmo); aCap > 0) { // 0xc1179f: 0x22 -> 3000
		gBurnCap = aCap;
		mSet = true;
		LogShot(theCob, std::format("loadedCannon.cob lands, burn cap {}", aCap));
	}
}

ModCobBurnCapScope::~ModCobBurnCapScope() {
	if (mSet)
		gBurnCap = 0; // hook 156 0xc118c0
}

int ModBurnCapDamage(const Zombie* theZombie) {
	if (gBurnCap <= 0 || theZombie->mZombieType == ZombieType::ZOMBIE_BOSS)
		return 0;
	const int aSum = theZombie->mBodyHealth + (theZombie->mHelmType != HelmType::HELMTYPE_NONE ? theZombie->mHelmHealth : 0) +
	                 (theZombie->mShieldType != ShieldType::SHIELDTYPE_NONE ? theZombie->mShieldHealth : 0);
	const int aDamage = u::BurnCapDamage(aSum, gBurnCap); // 0xc11770
	return aDamage > 0 ? aDamage : -1;
}

Projectile* AddModShot(Board* theBoard, int theX, int theY, int theRenderOrder, int theRow, int theExeType) {
	const int aCarrier = u::ModShotCarrier(theExeType);
	if (aCarrier < 0)
		return nullptr;
	Projectile* aShot = theBoard->AddProjectile(theX, theY, theRenderOrder, theRow, static_cast<ProjectileType>(aCarrier));
	if (u::IsModShotType(theExeType))
		aShot->mPvzu.modType = theExeType;
	return aShot;
}

// ==== Dev harness ===================================================================================================

namespace {
struct DevCannon { bool doom; int tick; bool done; };
std::vector<DevCannon> ParseDevCannon() {
	std::vector<DevCannon> v;
	const std::string s = CmdOption("-pvzu-dev-cannon").value_or("");
	size_t aPos = 0;
	while (aPos < s.size()) {
		size_t aEnd = s.find(',', aPos);
		const std::string aItem = s.substr(aPos, aEnd == std::string::npos ? std::string::npos : aEnd - aPos);
		if (const size_t aAt = aItem.find('@'); aAt != std::string::npos && aAt > 0)
			v.push_back({aItem[0] == 'd', std::atoi(aItem.c_str() + aAt + 1), false});
		if (aEnd == std::string::npos)
			break;
		aPos = aEnd + 1;
	}
	return v;
}
} // namespace

void ShootersDevTick(Board* theBoard, int thePlayTicks) {
	static const int aPlantAt = CmdOptionInt("-pvzu-dev-shooters", -1);
	static std::vector<DevCannon> aLoads = ParseDevCannon();
	static bool aFireArmed = false;
	if (thePlayTicks == aPlantAt) {
		struct Put { int seed, col, row; };
		const Put aLayout[] = {
			{kShadowPea, 0, 0}, {kPrimalRepeater, 2, 0},
			{kMysticIce, 0, 1}, {SeedType::SEED_PEASHOOTER, 2, 1}, {kDreamStar, 4, 1},
			{kDuskPult, 0, 2}, {kButterPult, 2, 2},
			{kSniper, 0, 3}, {kElement, 3, 3},
			{kSpiritTail, 0, 4}, {kCannon, 2, 4},
		};
		for (const Put& p : aLayout) {
			if (theBoard->GetTopPlantAt(p.col, p.row, PlantPriority::TOPPLANT_ANY))
				continue;
			Plant* aPlant = theBoard->AddPlant(p.col, p.row, static_cast<SeedType>(p.seed), SeedType::SEED_NONE);
			if (aPlant && p.seed == kElement && CmdFlag("-pvzu-dev-element-ice"))
				ShooterPlantClicked(aPlant); // as a click on it: ice
			u::LogPlantAbility(static_cast<int>(theBoard->mMainCounter), std::format("DEV shooters: 0x{:02x} {} at col {} row {} "
				"launchRate {}", p.seed, p.seed >= 0x50 ? PlantName(p.seed) : std::string("Peashooter"), p.col, p.row,
				aPlant ? aPlant->mLaunchRate : -1));
		}
	}
	for (DevCannon& d : aLoads) {
		if (d.done || thePlayTicks < d.tick)
			continue;
		for (Plant* aPlant : theBoard->mPlants) {
			if (aPlant->mDead || Mod(aPlant) != kCannon || aPlant->mState != PlantState::STATE_COBCANNON_ARMING)
				continue;
			d.done = true;
			LoadCannon(aPlant, d.doom ? SeedType::SEED_DOOMSHROOM : SeedType::SEED_KERNELPULT);
			aFireArmed = !d.doom;
			break;
		}
	}
	if (aFireArmed) { // fire the loaded cob at the first zombie on the lawn once it is READY
		for (Plant* aPlant : theBoard->mPlants) {
			if (aPlant->mDead || Mod(aPlant) != kCannon || aPlant->mState != PlantState::STATE_COBCANNON_READY)
				continue;
			for (Zombie* aZombie : theBoard->mZombies) {
				if (aZombie->mDead || aZombie->IsDeadOrDying() || aZombie->mX > 750)
					continue;
				aPlant->CobCannonFire(aZombie->mX + 60, aZombie->mY + 70);
				Log(aPlant, std::format("DEV cannon fires at zombie type {} row {} x {}", static_cast<int>(aZombie->mZombieType),
					aZombie->mRow, aZombie->mX));
				aFireArmed = false;
				break;
			}
			break;
		}
	}
}

} // namespace pvzu::engine
