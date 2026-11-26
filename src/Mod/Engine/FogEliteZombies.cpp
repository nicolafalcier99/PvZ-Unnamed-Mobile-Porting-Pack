// PVZU: engine side of the Unnamed Fog zombies. Rules: Mod/Unnamed/FogZombies.h; evidence: the research notes (w4-fog)
// section 3 and pvz-unnamed-pack/SOURCES-fog-zombies.md.
#include "Mod/Engine/FogEliteZombies.h"
#include "Mod/Engine/ZombieGapsV1.h"
#include "Mod/Engine/OptionsPanel.h"

#include "Mod/Engine/EliteZombies.h"
#include "Mod/Engine/EngineTables.h"
#include "Mod/Engine/NightEliteZombies.h"
#include "Mod/Engine/ModPlants.h" // ModSeedOf: hooks 654 / 676 compare the raw seed
#include "Mod/GameData.h"
#include "Mod/Unnamed/FogZombies.h"

#include "LawnApp.h"
#include "Resources.h"
#include "Lawn/Board.h"
#include "Lawn/Challenge.h"
#include "Lawn/Plant.h"
#include "Lawn/SeedPacket.h"
#include "Lawn/Zombie.h"
#include "PvzpLib/Attachment.h"
#include "PvzpLib/PvzpCommon.h"
#include "PvzpLib/PvzpParticle.h"
#include "PvzpLib/Reanimator.h"
#include "SexyAppFramework/graphics/SharedImage.h"

#include <algorithm>
#include <cstdio>
#include <map>
#include <string>
#include <vector>

using namespace Sexy;
namespace u = pvzu::unnamed;

namespace pvzu::engine {

namespace {

int BoardTime(const Zombie* z) { return z->mBoard ? static_cast<int>(z->mBoard->mMainCounter) : 0; }
int BoardTime(const Plant* p) { return p->mBoard ? static_cast<int>(p->mBoard->mMainCounter) : 0; }
int Frame() { return gSexyAppBase ? static_cast<int>(gSexyAppBase->mUpdateCount) : 0; }
int Id(Zombie* z) { return z->mBoard ? static_cast<int>(z->mBoard->ZombieGetID(z)) & 0xFFFF : -1; }
const char* Name(Zombie* z) { return u::FogEliteName(z->mZombieType, z->mPvzu.variant); }
bool Is(const Zombie* z, ZombieType t) { return z->mZombieType == t; }
const char* TypeName(ZombieType t) {
	RegisterVanillaTables();
	const auto& zombies = GameData::Get().Vanilla().zombies;
	return t >= 0 && static_cast<size_t>(t) < zombies.size() ? zombies[t].name.c_str() : "?";
}

// Elite art by pack file name: all are engine resources (loaded with the game), so the asset overlay serves Unnamed's
// file of that name when the pack has one (e.g. images/Chocolate -> pvz-unnamed-pack/assets/images/chocolate.png).
Image* FogImage(const std::string& theFile) {
	static const std::map<std::string, Image**> kResources = {
		{"IMAGE_BLANK", &IMAGE_BLANK},
		{"particles/Zombie_boss_iceball_particles", &IMAGE_ZOMBIE_BOSS_ICEBALL_PARTICLES},
		{"particles/Zomboss_particles", &IMAGE_ZOMBOSS_PARTICLES},
		{"images/Chocolate", &IMAGE_CHOCOLATE},
		{"reanim/Garlic_body3", &IMAGE_REANIM_GARLIC_BODY3},
	};
	if (auto it = kResources.find(theFile); it != kResources.end() && *it->second != nullptr)
		return *it->second;
	static std::map<std::string, SharedImageRef> loaded; // kept alive for the whole run
	auto it = loaded.find(theFile);
	if (it == loaded.end()) {
		it = loaded.emplace(theFile, gSexyAppBase->GetSharedImage(theFile)).first;
		if (static_cast<Image*>(it->second) == nullptr)
			std::fprintf(stderr, "PVZU: fog elite image %s not found, drawing the base zombie\n", theFile.c_str());
	}
	return static_cast<Image*>(it->second);
}

void ApplyFogArt(Zombie* theZombie, bool theAfterLoad) {
	Reanimation* aBody = theZombie->mApp->ReanimationTryToGet(theZombie->mBodyReanimID);
	if (aBody == nullptr)
		return;
	const u::FogArt art = u::ResolveFogArt(theZombie->mZombieType, theZombie->mPvzu.variant,
		[](const std::string& f) { return FogImage(f) != nullptr; });
	for (const auto& img : art.images)
		aBody->SetImageOverride(img.track.c_str(), FogImage(img.file));
	for (const auto& prefix : art.hidePrefixes)
		theZombie->ReanimShowPrefix(prefix.c_str(), RENDER_GROUP_HIDDEN);
	if (art.headReanim != ReanimationType::REANIM_NONE && !theAfterLoad) { // helper 0x794800, as the umbrella leaf
		ReanimatorTrackInstance* aTrack = aBody->GetTrackInstanceByName("anim_head1");
		Reanimation* aHead = theZombie->mApp->AddReanimation(0.0f, 0.0f, 0, art.headReanim);
		std::vector<std::string> aTracks; // the exe plays "anim_head_idle"; a reanim without it plays its track 0
		for (int i = 0; i < aHead->mDefinition->mTracks.count; ++i)
			aTracks.emplace_back(aHead->mDefinition->mTracks.tracks[i].mName);
		const std::string aAnim = u::FogHeadAnimTrack(art.headAnim, aTracks);
		aHead->PlayReanim(aAnim.c_str(), ReanimLoopType::REANIM_LOOP, 0, 15.0f);
		theZombie->mSpecialHeadReanimID = theZombie->mApp->ReanimationGetID(aHead);
		AttachEffect* anEffect = AttachReanim(aTrack->mAttachmentID, aHead, 0.0f, 0.0f);
		aBody->mFrameBasePose = 0;
		PvzpScaleRotateTransformMatrix(anEffect->mOffset, 65.0f, -5.0f, 0.2f, -1.0f, 1.0f);
	}
}

bool Alive(Zombie* z) { return z && !z->mDead && !z->IsDeadOrDying(); }

void IceAllZombies(Zombie* theJack) { // Plant::IceZombies (hypnotised ice head, hook 561)
	Board* aBoard = theJack->mBoard;
	for (Zombie* z : aBoard->mZombies)
		if (!z->mDead && z != theJack)
			z->HitIceTrap();
	aBoard->mIceTrapCounter = 300;
	if (PvzpParticleSystem* aSparkly = theJack->mApp->ParticleTryToGet(aBoard->mPoolSparklyParticleID))
		aSparkly->mDontUpdate = false;
	if (Zombie* aBoss = aBoard->GetBossZombie())
		aBoss->BossDestroyFireball();
}

int SetPackets(Board* theBoard, bool theFullCooldown) { // 0x7a8530 (potato head) / hook 561 (hypnotised)
	SeedBank* aBank = theBoard->mSeedBank.get();
	if (aBank == nullptr)
		return 0;
	int n = 0;
	for (int i = 0; i < std::min(u::PotatoPacketCount(), static_cast<int>(aBank->mNumPackets)); ++i) {
		SeedPacket& p = aBank->mSeedPackets[i];
		if (p.mPacketType == SeedType::SEED_NONE) // Ruling: an empty slot has no refresh time (the exe reads table -1)
			continue;
		if (theFullCooldown) {
			p.mRefreshTime = Plant::GetRefreshTime(p.mPacketType, p.mImitaterType);
			p.mActive = false;
			p.mRefreshing = true;
		} else {
			p.mRefreshTime = 0;
		}
		++n;
	}
	return n;
}

} // namespace

bool FogZombiesOn() { return GameData::Get().Feature("fogZombies"); }

void FogEliteInit(Zombie* theZombie, bool theOnBoard) {
	const ZombieType aType = theZombie->mZombieType;
	if (!FogZombiesOn() || !theOnBoard || !u::IsFogType(aType))
		return;
	LawnApp* anApp = theZombie->mApp;
	int aForced = 0;
	const bool aIsForced = ForcedVariantFor(aType, aForced); // a forced type rolls nothing for its form (dev only)
	int v = aIsForced ? aForced : 0;
	const int aSetting = EliteOption(); // PVZU elite-all: Jack, Balloon, Digger are patched code (0x7b2000 / 0x7b2100)
	if (aType == ZombieType::ZOMBIE_JACK_IN_THE_BOX) { // hook 500
		if (const int n = u::JackRollRange(aSetting); !aIsForced && n > 0 && u::JackRollAllowed(anApp->IsScaryPotterLevel()))
			v = u::RollJack(Rand(n));
	} else if (aType == ZombieType::ZOMBIE_BALLOON) { // hook 503
		if (const int n = u::BalloonRollRange(aSetting); !aIsForced && n > 0 && u::BalloonDiggerRollAllowed(anApp->IsIZombieLevel()))
			v = u::RollBalloon(Rand(n), aSetting);
		if (v == u::fogelite::CLOUD)
			RandRangeFloat(0.23f, 0.30f); // the exe writes it to the body reanim's +0x34 (no visible effect, study U8)
	} else if (aType == ZombieType::ZOMBIE_DIGGER) { // hook 490
		if (const int n = u::DiggerRollRange(aSetting); !aIsForced && n > 0 && u::BalloonDiggerRollAllowed(anApp->IsIZombieLevel()))
			v = u::RollDigger(Rand(n), aSetting);
		theZombie->mPvzu.subVariant = 0; // exe +0x14C: surfacing x, 0 = vanilla x < 10
		if (v == u::fogelite::SCREWDRIVER && u::DiggerSurfaceRollRange() > 0)
			theZombie->mPvzu.subVariant = u::DiggerSurfaceX(Rand(u::DiggerSurfaceRollRange()));
	} else if (aType == ZombieType::ZOMBIE_POGO) { // hook 501
		if (const int n = u::PogoRollRange(EliteValue()); !aIsForced && n > 0) // PVZU elite-all: 0x796c08 reads the value
			v = u::RollPogo(Rand(n));
		if (v == u::fogelite::GARLIC && u::PogoGarlicVisibleRollRange() > 0)
			theZombie->mVisible = u::PogoGarlicVisible(Rand(u::PogoGarlicVisibleRollRange()));
	}
	theZombie->mPvzu.variant = v;
	theZombie->mBodyHealth = u::FogBodyHp(aType, v, theZombie->mBodyHealth);
	theZombie->mHelmHealth = u::FogHelmHp(aType, theZombie->mHelmHealth);
	theZombie->mFlyingHealth = u::FogFlyingHp(aType, theZombie->mFlyingHealth);
	if (v != 0)
		ApplyFogArt(theZombie, false);
	LogZombieEvent("t=%d f=%d fog form %s#%d form=%s row=%d vx=%.2f surfaceX=%d visible=%d body=%d helm=%d flying=%d",
		BoardTime(theZombie), Frame(), TypeName(aType), Id(theZombie), Name(theZombie),
		theZombie->mRow, theZombie->mVelX, theZombie->mPvzu.subVariant, theZombie->mVisible ? 1 : 0, theZombie->mBodyHealth,
		theZombie->mHelmHealth, theZombie->mFlyingHealth);
}

void FogAfterLoad(Zombie* theZombie) {
	if (!FogZombiesOn() || theZombie->mDead || !u::IsFogType(theZombie->mZombieType) || theZombie->mPvzu.variant == 0)
		return;
	ApplyFogArt(theZombie, true);
	LogZombieEvent("t=%d f=%d fog load %s#%d form=%s variant=0x%x surfaceX=%d visible=%d body=%d helm=%d flying=%d x=%d",
		BoardTime(theZombie), Frame(), TypeName(theZombie->mZombieType), Id(theZombie),
		Name(theZombie), theZombie->mPvzu.variant, theZombie->mPvzu.subVariant, theZombie->mVisible ? 1 : 0,
		theZombie->mBodyHealth, theZombie->mHelmHealth, theZombie->mFlyingHealth, theZombie->mX);
}

// ---- Balloon ----

bool FogPickSpeed(Zombie* theZombie) {
	if (!FogZombiesOn())
		return false;
	const auto aVelX = u::BalloonVelX(theZombie->mZombieType, theZombie->mPvzu.variant,
		theZombie->mZombiePhase == ZombiePhase::PHASE_BALLOON_WALKING);
	if (!aVelX)
		return false;
	theZombie->mVelX = *aVelX;
	return true;
}

bool FogCloudLands(Zombie* theZombie) {
	if (!FogZombiesOn() || !Is(theZombie, ZombieType::ZOMBIE_BALLOON) ||
		!u::CloudKeepsFlying(theZombie->mPvzu.variant, theZombie->mZombiePhase == ZombiePhase::PHASE_BALLOON_FLYING))
		return false;
	if (theZombie->mZombieHeight != ZombieHeight::HEIGHT_FALLING && theZombie->mAltitude > 0.0f)
		LogZombieEvent("t=%d f=%d fog cloud-sinks BALLOON#%d row=%d x=%d body=%d (no pop, hook 551)", BoardTime(theZombie), Frame(),
			Id(theZombie), theZombie->mRow, theZombie->mX, theZombie->mBodyHealth);
	theZombie->mZombieHeight = ZombieHeight::HEIGHT_FALLING;
	return true;
}

bool FogFlyerEats(Zombie* theZombie) {
	return FogZombiesOn() && Is(theZombie, ZombieType::ZOMBIE_BALLOON) && u::FlyersEat();
}

bool FogMowerSkips(Zombie* theZombie) {
	if (!FogZombiesOn() || !u::MowerSkips(theZombie->mZombieType, theZombie->mFlyingHealth))
		return false;
	if (theZombie->mX < 50 && theZombie->mPvzu.counter == 0) { // one line, once it is over the mowers
		theZombie->mPvzu.counter = 1;
		LogZombieEvent("t=%d f=%d fog mower-skip BALLOON#%d form=%s row=%d x=%d flying=%d", BoardTime(theZombie), Frame(),
			Id(theZombie), Name(theZombie), theZombie->mRow, theZombie->mX, theZombie->mFlyingHealth);
	}
	return true;
}

std::optional<bool> FogCanTargetPlant(Zombie* theZombie, Plant* thePlant, ZombieAttackType theAttackType) {
	if (!FogZombiesOn())
		return std::nullopt;
	const int aSeed = static_cast<int>(ModSeedOf(thePlant)); // hooks 654 / 676 compare the raw seed: a mod plant is not its base
	if (theZombie->mZombiePhase == ZombiePhase::PHASE_DIGGER_TUNNELING && Is(theZombie, ZombieType::ZOMBIE_DIGGER)) {
		const auto aTarget = u::TunnelingDiggerTargets(aSeed);
		const int aPlantKey = static_cast<int>(theZombie->mBoard->mPlants.DataArrayGetID(thePlant)) + 1;
		if (aTarget && *aTarget && theZombie->mPvzu.counter != aPlantKey) { // one line per plant
			theZombie->mPvzu.counter = aPlantKey;
			LogZombieEvent("t=%d f=%d fog dig-bite DIGGER#%d form=%s -> plant seed=%d col=%d row=%d hp=%d (hook 654)",
				BoardTime(theZombie), Frame(), Id(theZombie), Name(theZombie), aSeed, thePlant->mPlantCol, thePlant->mRow,
				thePlant->mPlantHealth);
		}
		return aTarget;
	}
	if (theZombie->mZombiePhase == ZombiePhase::PHASE_BALLOON_FLYING && theAttackType == ZombieAttackType::ATTACKTYPE_CHEW &&
		u::FlyingBalloonSkipsPlant(aSeed))
		return false;
	return std::nullopt;
}

// ---- Digger ----

float FogDiggerSurfaceX(Zombie* theZombie, float theVanilla) {
	if (!FogZombiesOn() || !Is(theZombie, ZombieType::ZOMBIE_DIGGER))
		return theVanilla;
	const int aSurfaceX = theZombie->mPvzu.variant == u::fogelite::SCREWDRIVER ? theZombie->mPvzu.subVariant : 0;
	return u::DiggerSurfaceBelowX(aSurfaceX, theVanilla);
}

ZombiePhase FogDiggerRisePhase(Zombie* theZombie, ZombiePhase theVanilla) {
	if (!FogZombiesOn())
		return theVanilla;
	const bool aNoAxe = u::DiggerRisesWithoutAxe(theZombie->mPvzu.variant);
	LogZombieEvent("t=%d f=%d fog digger-surface DIGGER#%d form=%s row=%d x=%.1f surfaceX=%d phase=%s", BoardTime(theZombie), Frame(),
		Id(theZombie), Name(theZombie), theZombie->mRow, theZombie->mPosX, theZombie->mPvzu.subVariant,
		aNoAxe ? "rise-without-axe" : "rising");
	return aNoAxe ? ZombiePhase::PHASE_DIGGER_RISE_WITHOUT_AXE : theVanilla;
}

// ---- Jack-in-the-Box ----

bool FogJackExplodes(Zombie* theZombie) {
	if (!FogZombiesOn() || !Is(theZombie, ZombieType::ZOMBIE_JACK_IN_THE_BOX))
		return false;
	LawnApp* anApp = theZombie->mApp;
	Board* aBoard = theZombie->mBoard;
	const u::JackBlast aBlast = u::JackExplosion(theZombie->mPvzu.variant, theZombie->mMindControlled, anApp->IsIZombieLevel());
	if (aBlast == u::JackBlast::VANILLA)
		return false;
	const int aPosX = theZombie->mX + theZombie->mWidth / 2;
	const int aPosY = theZombie->mY + theZombie->mHeight / 2;
	switch (aBlast) {
	case u::JackBlast::FREEZE_PLANTS: { // 0x796aff..0x796b46
		aBoard->mPvzuState.iceFreezeActive = 1;
		aBoard->mPvzuState.iceFreezeCounter = u::IceFreezeCounter(static_cast<int>(aBoard->mPlants.mSize));
		anApp->AddPvzpParticle(aPosX, aPosY, static_cast<int>(RenderLayer::RENDER_LAYER_TOP), ParticleEffect::PARTICLE_ICE_TRAP);
		LogZombieEvent("t=%d f=%d fog jack-ice JACK_IN_THE_BOX#%d row=%d x=%d plants=%u freezeCounter=%d (no blast)",
			BoardTime(theZombie), Frame(), Id(theZombie), theZombie->mRow, theZombie->mX, aBoard->mPlants.mSize,
			aBoard->mPvzuState.iceFreezeCounter);
		break;
	}
	case u::JackBlast::PACKETS_COOL: {
		const int n = SetPackets(aBoard, true);
		LogZombieEvent("t=%d f=%d fog jack-potato JACK_IN_THE_BOX#%d row=%d x=%d packets=%d to full cooldown (no blast)",
			BoardTime(theZombie), Frame(), Id(theZombie), theZombie->mRow, theZombie->mX, n);
		break;
	}
	case u::JackBlast::ICE_ZOMBIES:
		IceAllZombies(theZombie);
		anApp->AddPvzpParticle(aPosX, aPosY, static_cast<int>(RenderLayer::RENDER_LAYER_TOP), ParticleEffect::PARTICLE_ICE_TRAP);
		LogZombieEvent("t=%d f=%d fog jack-ice-hypnotised JACK_IN_THE_BOX#%d ices every zombie", BoardTime(theZombie), Frame(), Id(theZombie));
		break;
	case u::JackBlast::PACKETS_READY: {
		const int n = SetPackets(aBoard, false);
		LogZombieEvent("t=%d f=%d fog jack-potato-hypnotised JACK_IN_THE_BOX#%d packets=%d ready", BoardTime(theZombie), Frame(), Id(theZombie), n);
		break;
	}
	case u::JackBlast::VANILLA: break;
	}
	aBoard->ShakeBoard(4, -6);
	theZombie->DieNoLoot();
	if (anApp->IsScaryPotterLevel())
		aBoard->mChallenge->ScaryPotterJackExplode(aPosX, aPosY);
	return true;
}

bool FogIceFreezeSkips(Plant* thePlant) {
	Board* aBoard = thePlant->mBoard;
	if (aBoard == nullptr || aBoard->mPvzuState.iceFreezeActive == 0 || !thePlant->IsOnBoard() || !FogZombiesOn())
		return false;
	u::IceFreeze f{aBoard->mPvzuState.iceFreezeActive, aBoard->mPvzuState.iceFreezeCounter};
	const bool aSkip = u::IceFreezeStep(f);
	aBoard->mPvzuState.iceFreezeActive = f.active;
	aBoard->mPvzuState.iceFreezeCounter = f.counter;
	if (f.active == 0)
		LogZombieEvent("t=%d f=%d fog ice-freeze ends (plants=%u)", BoardTime(thePlant), Frame(), aBoard->mPlants.mSize);
	return aSkip;
}

void FogBoardAfterLoad(Board* theBoard) {
	if (!FogZombiesOn() || theBoard->mPvzuState.IsDefault())
		return;
	LogZombieEvent("t=%d f=%d fog load board ice-freeze active=%d counter=%d plants=%u", static_cast<int>(theBoard->mMainCounter),
		Frame(), theBoard->mPvzuState.iceFreezeActive, theBoard->mPvzuState.iceFreezeCounter, theBoard->mPlants.mSize);
}

// ---- Pogo ----

bool FogPogoBonks(Zombie* theZombie, Plant* thePlant) {
	if (!FogZombiesOn())
		return thePlant->mSeedType == SeedType::SEED_TALLNUT;
	const bool aBonk = u::PogoBonksOn(static_cast<int>(thePlant->mSeedType));
	if (aBonk)
		LogZombieEvent("t=%d f=%d fog pogo-bonk POGO#%d form=%s on seed=%d", BoardTime(theZombie), Frame(), Id(theZombie), Name(theZombie),
			static_cast<int>(thePlant->mSeedType));
	return aBonk;
}

bool FogPogoLandsOnZombies(Zombie* theZombie) { // hook 531 0x959840
	if (!FogZombiesOn() || !u::PogoLandsOnZombies())
		return false;
	Rect aRect = theZombie->GetZombieAttackRect();
	aRect.mX -= u::PogoLandRectGrowPx();
	aRect.mWidth += u::PogoLandRectGrowPx();
	int aHits = 0;
	for (Zombie* z : theZombie->mBoard->mZombies) {
		// Ruling: same row and not flying, as FindZombieTarget (the cave's own filters were not read in full)
		if (z == theZombie || !Alive(z) || z->mMindControlled == theZombie->mMindControlled || z->mRow != theZombie->mRow ||
			z->IsFlying() || z->mZombieHeight == ZombieHeight::HEIGHT_GETTING_BUNGEE_DROPPED)
			continue;
		if (GetRectOverlap(aRect, z->GetZombieRect()) <= 0)
			continue;
		const int aDamage = u::PogoLandDamage(theZombie->mPvzu.variant, aHits == 0);
		z->TakeDamage(aDamage, 0U);
		if (V1PogoLandYucky(theZombie, aHits == 0, theZombie->mPvzu.variant == u::fogelite::GARLIC)) // PVZU V1: not the first (0x95991c)
			z->mYuckyFace = true;
		LogZombieEvent("t=%d f=%d fog pogo-land POGO#%d form=%s -> zombie#%d damage=%d", BoardTime(theZombie), Frame(), Id(theZombie),
			Name(theZombie), Id(z), aDamage);
		++aHits;
	}
	return aHits > 0;
}

bool FogPogoBounceEnd(Zombie* theZombie) { // patch 0x5256eb + hook 533
	if (!FogZombiesOn())
		return false;
	std::optional<int> aDx;
	if (theZombie->IsOnBoard()) {
		if (Plant* aPlant = theZombie->FindPlantTarget(ZombieAttackType::ATTACKTYPE_VAULT))
			aDx = theZombie->mX - aPlant->mX;
		else if (u::PogoTargetsZombies())
			if (Zombie* aZombie = theZombie->FindZombieTarget())
				aDx = theZombie->mX - aZombie->mX;
	}
	const int v = theZombie->mPvzu.variant;
	if (u::PogoTurnsYucky(v)) { // 0x959960: garlic changes lane at every bounce
		theZombie->mYuckyFace = true;
		theZombie->mYuckyFaceCounter = u::PogoYuckyCounter();
	}
	const u::PogoNext aNext = u::PogoBounceEnd(v, aDx, theZombie->mZombiePhase == ZombiePhase::PHASE_POGO_HIGH_BOUNCE_1);
	theZombie->mPhaseCounter = 80; // POGO_BOUNCE_TIME
	switch (aNext.move) {
	case u::PogoMove::BOUNCE:
		theZombie->mZombiePhase = ZombiePhase::PHASE_POGO_BOUNCING;
		theZombie->PickRandomSpeed();
		break;
	case u::PogoMove::HIGH_BOUNCE:
		theZombie->mZombiePhase = ZombiePhase::PHASE_POGO_HIGH_BOUNCE_1;
		theZombie->mVelX = 0.0f;
		break;
	case u::PogoMove::FORWARD:
		theZombie->mZombiePhase = ZombiePhase::PHASE_POGO_FORWARD_BOUNCE_2;
		theZombie->mVelX = aNext.velX;
		break;
	}
	if (v != 0 || aNext.move == u::PogoMove::FORWARD)
		LogZombieEvent("t=%d f=%d fog pogo-bounce POGO#%d form=%s target=%s move=%s vx=%.2f row=%d x=%d yucky=%d", BoardTime(theZombie), Frame(),
			Id(theZombie), Name(theZombie), aDx ? "yes" : "no",
			aNext.move == u::PogoMove::FORWARD ? "forward" : aNext.move == u::PogoMove::HIGH_BOUNCE ? "high" : "bounce",
			theZombie->mVelX, theZombie->mRow, theZombie->mX, theZombie->mYuckyFace ? 1 : 0);
	return true;
}

bool FogDrawsHiddenShadow(Zombie* theZombie) {
	return !theZombie->mVisible && FogZombiesOn() && Is(theZombie, ZombieType::ZOMBIE_POGO) &&
		theZombie->mPvzu.variant == u::fogelite::GARLIC;
}

} // namespace pvzu::engine
