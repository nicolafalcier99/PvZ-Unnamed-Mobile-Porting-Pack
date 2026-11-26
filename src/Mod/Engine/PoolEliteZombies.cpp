// PVZU: engine side of the Unnamed Pool zombies. Rules: Mod/Unnamed/PoolZombies.h; evidence: the research notes (w3-pool)
// Z1-Z5 and pvz-unnamed-pack/SOURCES-pool-zombies.md.
#include "Mod/Engine/PoolEliteZombies.h"
#include "Mod/Engine/OptionsPanel.h"

#include "Mod/Engine/EliteZombies.h"
#include "Mod/Engine/ModPlants.h" // Plan 8 part 3b: ModSeedOf (the freeze filter 0x7b1800 sees the mod seed)
#include "Mod/Engine/NightEliteZombies.h"
#include "Mod/GameData.h"
#include "Mod/Unnamed/PoolZombies.h"

#include "LawnApp.h"
#include "Resources.h"
#include "Lawn/Board.h"
#include "Lawn/Plant.h"
#include "Lawn/Zombie.h"
#include "PvzpLib/Attachment.h"
#include "PvzpLib/PvzpCommon.h"
#include "PvzpLib/Reanimator.h"

#include <cstdio>
#include <map>
#include <string>

using namespace Sexy;
namespace u = pvzu::unnamed;

namespace pvzu::engine {

namespace {

int BoardTime(const Zombie* z) { return z->mBoard ? static_cast<int>(z->mBoard->mMainCounter) : 0; }
int Id(Zombie* z) { return z->mBoard ? static_cast<int>(z->mBoard->ZombieGetID(z)) & 0xFFFF : -1; }

Image* PoolImage(const std::string& theFile) {
	static const std::map<std::string, Image**> kResources = {
		{"IMAGE_BLANK", &IMAGE_BLANK},
		{"particles/ZombieFootballHead", &IMAGE_ZOMBIEFOOTBALLHEAD},
	};
	if (auto it = kResources.find(theFile); it != kResources.end())
		return *it->second;
	std::fprintf(stderr, "PVZU: pool elite image %s not known, drawing the base zombie\n", theFile.c_str());
	return nullptr;
}

void ApplyPoolArt(Zombie* theZombie, bool theAfterLoad) {
	Reanimation* aBody = theZombie->mApp->ReanimationTryToGet(theZombie->mBodyReanimID);
	if (aBody == nullptr)
		return;
	const u::PoolArt art = u::ResolvePoolArt(theZombie->mZombieType, theZombie->mPvzu.variant,
		[](const std::string& f) { return PoolImage(f) != nullptr; });
	for (const auto& img : art.images)
		aBody->SetImageOverride(img.track.c_str(), PoolImage(img.file));
	for (const auto& prefix : art.hidePrefixes)
		theZombie->ReanimShowPrefix(prefix.c_str(), RENDER_GROUP_HIDDEN);
	for (const auto& t : art.showTracks)
		theZombie->ReanimShowPrefix(t.c_str(), RENDER_GROUP_NORMAL);
	if (art.lilyPadHead && !theAfterLoad) { // helper 0x794800 (ebp = 6): the Day umbrella leaf's code with a Lily Pad
		ReanimatorTrackInstance* aTrack = aBody->GetTrackInstanceByName("anim_head1");
		Reanimation* aPad = theZombie->mApp->AddReanimation(0.0f, 0.0f, 0, ReanimationType::REANIM_LILYPAD);
		aPad->PlayReanim("anim_idle", ReanimLoopType::REANIM_LOOP, 0, 15.0f);
		theZombie->mSpecialHeadReanimID = theZombie->mApp->ReanimationGetID(aPad);
		AttachEffect* anEffect = AttachReanim(aTrack->mAttachmentID, aPad, 0.0f, 0.0f);
		aBody->mFrameBasePose = 0;
		PvzpScaleRotateTransformMatrix(anEffect->mOffset, 65.0f, -5.0f, 0.2f, -1.0f, 1.0f);
	}
}

void ApplyFrozenLook(Plant* thePlant) { // 0x795706..0x795714: anim rate 0, colour red 0 / green 160, blue kept
	Reanimation* aBody = nullptr;
	for (ReanimationID anId : {thePlant->mBodyReanimID, thePlant->mHeadReanimID, thePlant->mHeadReanimID2, thePlant->mHeadReanimID3}) {
		Reanimation* r = thePlant->mApp->ReanimationTryToGet(anId);
		if (r == nullptr)
			continue;
		if (aBody == nullptr)
			aBody = r;
		r->mAnimRate = 0.0f;
		r->mColorOverride = Color(u::ZamboniFreezeTintRed(), u::ZamboniFreezeTintGreen(), r->mColorOverride.mBlue, r->mColorOverride.mAlpha);
		r->mEnableExtraAdditiveDraw = false;
	}
	if (aBody)
		aBody->PropogateColorToAttachments();
}

bool SquareHasModPumpkin(Board* theBoard, Plant* thePlant) {
	for (Plant* p : theBoard->mPlants)
		if (!p->mDead && p->mPlantCol == thePlant->mPlantCol && p->mRow == thePlant->mRow && static_cast<int>(ModSeedOf(p)) == 0x6E)
			return true;
	return false;
}

void FreezePlants(Zombie* theZamboni) { // hook 620, 0x79561E chain
	Board* aBoard = theZamboni->mBoard;
	for (Plant* p : aBoard->mPlants) {
		if (p->mDead || p->mSquished || !p->mIsOnBoard || p->mPvzu.frozen != 0)
			continue;
		if (!u::InZamboniFreezeBox(theZamboni->mRow, theZamboni->mX, p->mRow, p->mX) ||
			u::ZamboniFreezeImmune(static_cast<int>(ModSeedOf(p)), SquareHasModPumpkin(aBoard, p))) // 0x56 / 0x55 / 0x7a by mod seed
			continue;
		p->mPvzu.frozen = 1;
		p->mBlinkCountdown = 0x7FFFFFFF;
		ApplyFrozenLook(p);
		LogZombieEvent("t=%d freeze ZAMBONI#%d form=%s row=%d x=%d -> plant seed=%d col=%d row=%d x=%d", BoardTime(theZamboni),
			Id(theZamboni), FormName(theZamboni).c_str(), theZamboni->mRow, theZamboni->mX, static_cast<int>(p->mSeedType),
			p->mPlantCol, p->mRow, p->mX);
	}
}

} // namespace

bool PoolZombiesOn() { return GameData::Get().Feature("poolZombies"); }

void PoolEliteInit(Zombie* theZombie, bool theOnBoard) {
	const ZombieType aType = theZombie->mZombieType;
	if (!PoolZombiesOn() || !theOnBoard || !u::IsPoolType(aType))
		return;
	LawnApp* anApp = theZombie->mApp;
	int aForced = 0;
	const bool aIsForced = ForcedVariantFor(aType, aForced); // a forced type rolls nothing (dev only)
	// PVZU elite-all: Snorkel / Dolphin jumps are patched code (the setting: 0x7b2000 / 0x7b2100 at 0x796967, 0x7969a9);
	// the Zomboni reads 0x800258 (the value, hook 496 0x95a40d).
	const int aOpt = aType == ZombieType::ZOMBIE_ZAMBONI ? EliteValue() : EliteOption();
	const int n = u::PoolRollRange(aType, aOpt);
	if (aType == ZombieType::ZOMBIE_ZAMBONI) { // hook 496: the state lives in +0x148, not +0x134
		int aState = u::zamboni::NORMAL;
		if (aIsForced) {
			aState = aForced;
			theZombie->mPvzu.variant = 0; // the Day code wrote the forced code there
		} else if (u::ZamboniRollAllowed(static_cast<int>(anApp->mGameMode), anApp->IsAdventureMode(),
			                                theZombie->mBoard ? theZombie->mBoard->mLevel : 0)) {
			if (u::ZamboniAlwaysOrange(aOpt)) // PVZU elite-all: no roll drawn (0x95a414)
				aState = u::zamboni::ORANGE;
			else if (n > 0)
				aState = u::RollZamboni(Rand(n));
		}
		theZombie->mPvzu.extra = aState;
	} else {
		int v = 0;
		if (aIsForced)
			v = aForced;
		else if (n > 0)
			v = aType == ZombieType::ZOMBIE_SNORKEL ? u::RollSnorkel(Rand(n), aOpt) : u::RollDolphin(Rand(n), aOpt);
		theZombie->mPvzu.variant = v;
	}
	theZombie->mBodyHealth = u::PoolBodyHp(aType, theZombie->mBodyHealth);
	ApplyPoolArt(theZombie, false);
}

void PoolAfterLoad(Zombie* theZombie) {
	if (!PoolZombiesOn() || theZombie->mDead || !u::IsPoolType(theZombie->mZombieType))
		return;
	if (theZombie->mPvzu.variant != 0)
		ApplyPoolArt(theZombie, true);
	LogZombieEvent("t=%d load pool %s#%d form=%s variant=0x%x state=%d body=%d x=%d", BoardTime(theZombie),
		GameData::Get().Vanilla().zombies[theZombie->mZombieType].name.c_str(), Id(theZombie), FormName(theZombie).c_str(),
		theZombie->mPvzu.variant, theZombie->mPvzu.extra, theZombie->mBodyHealth, theZombie->mX);
}

void PoolZombieTick(Zombie* theZombie) {
	if (!PoolZombiesOn() || theZombie->mZombieType != ZombieType::ZOMBIE_ZAMBONI || !theZombie->IsOnBoard() ||
		theZombie->IsDeadOrDying())
		return;
	const int aState = u::ZamboniStateAfterTick(theZombie->mPvzu.extra, theZombie->mBodyHealth);
	if (aState != theZombie->mPvzu.extra) {
		theZombie->mPvzu.extra = aState;
		LogZombieEvent("t=%d zamboni-red ZAMBONI#%d body=%d (hook 613)", BoardTime(theZombie), Id(theZombie), theZombie->mBodyHealth);
	}
	if (u::ZamboniFreezesPlants(theZombie->mPvzu.extra, theZombie->mMindControlled, static_cast<int>(theZombie->mZombieHeight)))
		FreezePlants(theZombie);
}

float PoolDolphinJumpVelX(Zombie* theZombie, float theVanilla) {
	if (!PoolZombiesOn())
		return theVanilla;
	const float aVelX = u::DolphinJumpVelX(theZombie->mPvzu.variant, theVanilla);
	LogZombieEvent("t=%d dolphin-jump DOLPHIN_RIDER#%d form=%s x=%d vx=%.2f", BoardTime(theZombie), Id(theZombie),
		FormName(theZombie).c_str(), theZombie->mX, aVelX);
	return aVelX;
}

float PoolZamboniVelX(Zombie* theZombie, float theVanilla) {
	return PoolZombiesOn() ? u::ZamboniVelX(theZombie->mPvzu.extra, theZombie->mBodyHealth, theVanilla) : theVanilla;
}

int PoolZamboniIceMaxX(int theVanilla) { return PoolZombiesOn() ? u::ZamboniIceMaxX(theVanilla) : theVanilla; }

int PoolBodyDamage(Zombie* theZombie, int theDamage) {
	if (!PoolZombiesOn() || theZombie->mZombieType != ZombieType::ZOMBIE_ZAMBONI || theZombie->mPvzu.extra != u::zamboni::ORANGE)
		return theDamage;
	const u::ZamboniHit aHit = u::ZamboniEliteHit(theZombie->mPvzu.extra, theDamage, theZombie->mBodyHealth);
	LogZombieEvent("t=%d zamboni-armour ZAMBONI#%d damage=%d -> %d body=%d%s", BoardTime(theZombie), Id(theZombie), theDamage,
		aHit.damage, theZombie->mBodyHealth, aHit.state != theZombie->mPvzu.extra ? " breaks: red" : "");
	theZombie->mPvzu.extra = aHit.state;
	return aHit.damage;
}

void PoolTint(Zombie* theZombie, int& theRed, int& theGreen, int& theBlue) {
	if (!PoolZombiesOn() || theZombie->mZombieType != ZombieType::ZOMBIE_ZAMBONI || theRed != 255 || theGreen != 255 || theBlue != 255)
		return;
	if (const auto aTint = u::ZamboniTint(theZombie->mPvzu.extra)) {
		theRed = aTint->r;
		theGreen = aTint->g;
		theBlue = aTint->b;
	}
}

bool PoolEatsPlants(Zombie* theZombie) {
	return !PoolZombiesOn() || u::PoolEatsPlants(theZombie->mZombieType, theZombie->mPvzu.variant);
}

bool PoolFreezeGateOn() { return PoolZombiesOn() && u::PoolFreezeGateOn(); }

bool PoolFreezeGate(Zombie* theZombie) { // Ruling: dead or dying zombies stay refused (EffectedByDamage filters them in the exe)
	return !theZombie->IsDeadOrDying() &&
		u::PoolFreezeGate(theZombie->mPvzu.variant, theZombie->mMindControlled, theZombie->mHasHead, theZombie->mBodyHealth);
}

bool PoolBossRefusesFreeze(Zombie* theZombie) {
	return PoolFreezeGateOn() && u::BossRefusesFreeze(theZombie->mZombieType, theZombie->mBodyHealth);
}

bool PoolFrozenPlantUpdate(Plant* thePlant) {
	if (thePlant->mPvzu.frozen == 0 || !PoolZombiesOn())
		return false;
	ApplyFrozenLook(thePlant); // no sun, no attack, no regen, no animation (Plant::Update skipped, study Z3a)
	if (thePlant->mPlantHealth < 0)
		thePlant->Die();
	return true;
}

void PoolPlantAfterLoad(Plant* thePlant) {
	if (thePlant->mDead || thePlant->mPvzu.frozen == 0 || !PoolZombiesOn())
		return;
	ApplyFrozenLook(thePlant);
	LogZombieEvent("t=%d load frozen plant seed=%d col=%d row=%d hp=%d", thePlant->mBoard ? static_cast<int>(thePlant->mBoard->mMainCounter) : 0,
		static_cast<int>(thePlant->mSeedType), thePlant->mPlantCol, thePlant->mRow, thePlant->mPlantHealth);
}

} // namespace pvzu::engine
