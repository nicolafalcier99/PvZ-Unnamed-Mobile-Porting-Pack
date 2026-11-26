// PVZU: engine side of the Unnamed Day zombies. Rules: Mod/Unnamed/DayZombies.h; evidence: the research notes (zombies).
#include "Mod/Engine/EliteZombies.h"
#include "Mod/Engine/EliteGaps.h"
#include "Mod/Engine/OptionsPanel.h"
#include "Mod/Engine/ModModes.h"

#include "Mod/CmdLine.h"
#include "Mod/Engine/EngineTables.h"
#include "Mod/Engine/FogEliteZombies.h"
#include "Mod/Engine/NightEliteZombies.h"
#include "Mod/Engine/PoolEliteZombies.h"
#include "Mod/Engine/RoofEliteZombies.h"
#include "Mod/Engine/World7EliteZombies.h"
#include "Mod/Engine/Level70Boss.h"
#include "Mod/Engine/ModPlantsShadow.h"
#include "Mod/GameData.h"
#include "Mod/Unnamed/DayZombies.h"
#include "Mod/Unnamed/NightZombies.h"
#include "Mod/Unnamed/RoofLevels.h"
#include "Mod/Unnamed/WideReach.h"

#include "LawnApp.h"
#include "Resources.h"
#include "Lawn/Board.h"
#include "Lawn/Challenge.h"
#include "Lawn/Plant.h"
#include "Lawn/Zombie.h"
#include "Lawn/System/PlayerInfo.h"
#include "PvzpLib/Attachment.h"
#include "PvzpLib/PvzpCommon.h"
#include "PvzpLib/Reanimator.h"
#include "SexyAppFramework/graphics/SharedImage.h"

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <map>
#include <string>

using namespace Sexy;
namespace u = pvzu::unnamed;

namespace pvzu::engine {

namespace {
bool gModModeNoElites = false; // PVZU: mod mode 0x6d writes elite variant -1 (0x7a3081): every world's roll forced to 0
}


namespace {

int gSkyDropDepth = 0;     // > 0 while Board::SpawnZombiesFromSky adds zombies (exe flag 0xC036CD == 1)
bool gCoinCopyPending = false; // the next Normal is a silver-coin copy (exe flag 0xC111F4 == 2)

FILE* ZombieLog() {
	static bool opened = false;
	static FILE* file = nullptr;
	if (!opened) {
		opened = true;
		if (auto path = CmdOption("-pvzu-log-zombies"); path && !path->empty()) {
			file = std::fopen(path->c_str(), "w");
			if (!file)
				std::fprintf(stderr, "PVZU: cannot write zombie log %s\n", path->c_str());
		}
	}
	return file;
}

const char* TypeName(ZombieType theType) {
	RegisterVanillaTables();
	const auto& zombies = GameData::Get().Vanilla().zombies;
	return theType >= 0 && static_cast<size_t>(theType) < zombies.size() ? zombies[theType].name.c_str() : "?";
}

// -pvzu-force-variant PAIL:4,NORMAL:0x105
const std::map<int, int>& ForcedVariants() {
	static std::map<int, int> forced;
	static bool parsed = false;
	if (parsed)
		return forced;
	parsed = true;
	auto opt = CmdOption("-pvzu-force-variant");
	if (!opt)
		return forced;
	RegisterVanillaTables();
	const auto& zombies = GameData::Get().Vanilla().zombies;
	std::string all = *opt;
	size_t start = 0;
	while (start <= all.size()) {
		size_t comma = all.find(',', start);
		std::string item = all.substr(start, comma == std::string::npos ? std::string::npos : comma - start);
		auto fv = u::ParseForcedVariant(item);
		bool known = false;
		for (size_t i = 0; fv && i < zombies.size(); ++i)
			if (zombies[i].name == fv->typeName) {
				forced[static_cast<int>(i)] = fv->variant;
				known = true;
			}
		if (!known)
			std::fprintf(stderr, "PVZU: -pvzu-force-variant: cannot read '%s' (want TYPE:variant, e.g. PAIL:4)\n",
				item.c_str());
		if (comma == std::string::npos)
			break;
		start = comma + 1;
	}
	return forced;
}

int WorldNumber(Zombie* theZombie) {
	LawnApp* app = theZombie->mApp;
	if (!app->IsAdventureMode() || theZombie->mBoard == nullptr)
		return 0;
	return (theZombie->mBoard->mLevel - 1) / 10 + 1;
}

int ZombieNumber(Zombie* theZombie) {
	return theZombie->mBoard ? static_cast<int>(theZombie->mBoard->ZombieGetID(theZombie)) & 0xFFFF : -1; // array index
}

int BoardTime(Zombie* theZombie) {
	return theZombie->mBoard ? static_cast<int>(theZombie->mBoard->mMainCounter) : 0;
}

// Elite art by pack file name. Files that are engine resources use the resource (so saves keep them and strips such
// as the 4-cell seaweed keep their cells); the overlay serves the Unnamed file under the same name either way.
Image* EliteImage(const std::string& theFile) {
	static const std::map<std::string, Image**> kResources = {
		{"IMAGE_BLANK", &IMAGE_BLANK},
		{"images/ZombieNoteSmall", &IMAGE_ZOMBIE_NOTE_SMALL},
		{"reanim/Coin_silver_dollar", &IMAGE_REANIM_COIN_SILVER_DOLLAR},
		{"images/Phonograph", &IMAGE_PHONOGRAPH},
		{"particles/ZombieDancerHead", &IMAGE_ZOMBIEDANCERHEAD},
		{"reanim/Pumpkin_damage1", &IMAGE_REANIM_PUMPKIN_DAMAGE1},
		{"images/Zen_GardenGlove", &IMAGE_ZEN_GARDENGLOVE},
		{"reanim/Zombie_gargantuar_duckxing", &IMAGE_REANIM_ZOMBIE_GARGANTUAR_DUCKXING},
		{"particles/Zombie_seaweed", &IMAGE_ZOMBIE_SEAWEED},
	};
	if (auto it = kResources.find(theFile); it != kResources.end())
		return *it->second;
	static std::map<std::string, SharedImageRef> loaded; // kept alive for the whole run
	auto it = loaded.find(theFile);
	if (it == loaded.end()) {
		it = loaded.emplace(theFile, gSexyAppBase->GetSharedImage(theFile)).first;
		if (static_cast<Image*>(it->second) == nullptr)
			std::fprintf(stderr, "PVZU: elite image %s not found, drawing the base zombie\n", theFile.c_str());
	}
	return static_cast<Image*>(it->second);
}

void ApplyEliteArt(Zombie* theZombie, bool theAfterLoad) {
	Reanimation* aBody = theZombie->mApp->ReanimationTryToGet(theZombie->mBodyReanimID);
	if (aBody == nullptr)
		return;
	u::EliteArt art = u::ResolveEliteArt(theZombie->mZombieType, theZombie->mPvzu.variant,
		[](const std::string& f) { return EliteImage(f) != nullptr; });
	if (art.empty())
		return;
	for (const auto& img : art.images)
		aBody->SetImageOverride(img.track.c_str(), EliteImage(img.file));
	for (const auto& prefix : art.hidePrefixes)
		theZombie->ReanimShowPrefix(prefix.c_str(), RENDER_GROUP_HIDDEN);
	if (art.umbrellaLeaf && !theAfterLoad) { // study 5.3: like the vanilla Pea-head zombie's head (Zombie.cpp, ZOMBIE_PEA_HEAD)
		ReanimatorTrackInstance* aTrack = aBody->GetTrackInstanceByName("anim_head1");
		Reanimation* aLeaf = theZombie->mApp->AddReanimation(0.0f, 0.0f, 0, ReanimationType::REANIM_UMBRELLALEAF);
		aLeaf->PlayReanim("anim_idle", ReanimLoopType::REANIM_LOOP, 0, 15.0f);
		theZombie->mSpecialHeadReanimID = theZombie->mApp->ReanimationGetID(aLeaf);
		AttachEffect* anEffect = AttachReanim(aTrack->mAttachmentID, aLeaf, 0.0f, 0.0f);
		aBody->mFrameBasePose = 0;
		PvzpScaleRotateTransformMatrix(anEffect->mOffset, 65.0f, -5.0f, 0.2f, -1.0f, 1.0f);
	}
}

bool Alive(Zombie* z) { return !z->mDead && !z->IsDeadOrDying(); }

void AuraPulse(Zombie* theSource) {
	Board* aBoard = theSource->mBoard;
	int touched = 0, restored = 0;
	for (Zombie* z : aBoard->mZombies) {
		if (!Alive(z) || !z->mHasHead || !z->IsOnBoard() || z->mMindControlled != theSource->mMindControlled)
			continue;
		if (!u::InAura(theSource->mRow, theSource->mX, z->mRow, z->mX) ||
			u::AuraSkips(z->mZombieType, z->mBodyMaxHealth, z->mPvzu.variant))
			continue;
		if (theSource->mPvzu.variant == u::elite::PHONOGRAPH) { // 0x7946A0: every pool back to its max
			restored += std::max(0, z->mBodyMaxHealth - z->mBodyHealth) + std::max(0, z->mHelmMaxHealth - z->mHelmHealth) +
				std::max(0, z->mShieldMaxHealth - z->mShieldHealth) + std::max(0, z->mFlyingMaxHealth - z->mFlyingHealth);
			z->mBodyHealth = std::max(z->mBodyHealth, z->mBodyMaxHealth);
			z->mHelmHealth = std::max(z->mHelmHealth, z->mHelmMaxHealth);
			z->mShieldHealth = std::max(z->mShieldHealth, z->mShieldMaxHealth);
			z->mFlyingHealth = std::max(z->mFlyingHealth, z->mFlyingMaxHealth);
			++touched;
		} else if (u::PumpkinGivesBucketTo(z->mZombieType, z->mHelmType)) { // 0x79474F
			z->mHelmType = HelmType::HELMTYPE_PAIL;
			z->mHelmHealth = z->mHelmMaxHealth = u::PumpkinBucketHp();
			z->ReanimShowPrefix("anim_bucket", RENDER_GROUP_NORMAL);
			restored += z->mHelmHealth;
			++touched;
		}
	}
	LogZombieEvent("t=%d aura %s#%d row=%d x=%d touched=%d hpGiven=%d", BoardTime(theSource),
		u::EliteName(theSource->mPvzu.variant), ZombieNumber(theSource), theSource->mRow, theSource->mX, touched, restored);
}

} // namespace

bool DayZombiesOn() { return GameData::Get().Feature("dayZombies"); }

void LogZombieEvent(const char* theFormat, ...) {
	FILE* f = ZombieLog();
	if (!f)
		return;
	va_list args;
	va_start(args, theFormat);
	std::vfprintf(f, theFormat, args);
	va_end(args);
	std::fputc('\n', f);
	std::fflush(f);
}

float StartOffsetPx(ZombieType theType, int theFromWave) {
	if (!DayZombiesOn() || theFromWave == Zombie::ZOMBIE_WAVE_UI)
		return 0.0f;
	return u::ForwardStartOffset(u::StartOffsetPx(theType), SpawnForwardOn()); // R8 刷新点前移 (hook 480)
}

namespace {
// R8 刷新点前移 (hooks 492, 495, 497, 512): forward = the Unnamed base - the wide lawn's start offset.
int ForwardBase(int theBase) {
	return u::ForwardStartBaseX(theBase, static_cast<int>(u::StartOffsetPx(ZombieType::ZOMBIE_NORMAL)), SpawnForwardOn());
}
} // namespace

int PoleStartBaseX(int theVanillaBase) { return DayZombiesOn() ? ForwardBase(u::PoleStartBaseX(theVanillaBase)) : theVanillaBase; }

int ZombieStartBaseX(ZombieType theType, int theVanillaBase) {
	return DayZombiesOn() ? ForwardBase(u::ZombieStartBaseX(theType, theVanillaBase)) : theVanillaBase;
}

int ZombieHittableMaxX(int theVanilla) { return DayZombiesOn() ? u::ZombieHittableMaxX(theVanilla) : theVanilla; }

float ProjectileMaxX(float theVanilla) { return DayZombiesOn() ? u::ProjectileMaxX(theVanilla) : theVanilla; }

void EliteInit(Zombie* theZombie) {
	if (Level70BossPlacing()) // the 7-10 boss is placed with elite rolls off (exe 0x800258 = 1, 0xc11030)
		return;
	const ZombieType aType = theZombie->mZombieType;
	const bool onBoard = theZombie->IsOnBoard() && theZombie->mFromWave != Zombie::ZOMBIE_WAVE_WINNER;
	gModModeNoElites = ModModeNoElites(theZombie); // PVZU: mod mode 0x6d, no elites
	if (DayZombiesOn() && onBoard) {
		int v = u::elite::NONE;
		if (aType == ZombieType::ZOMBIE_NORMAL && !gModModeNoElites)
			FootballHelmCheck(theZombie, EliteValue()); // PVZU Plan 9 R4: I, Zombie Normal, Rand(3) football helmet (0xc10998)
		const auto& forced = ForcedVariants();
		if (gModModeNoElites)
			v = u::elite::NONE;
		else if (auto it = forced.find(aType); it != forced.end())
			v = it->second;
		else if (gCoinCopyPending && aType == ZombieType::ZOMBIE_NORMAL)
			v = u::elite::SILVER_COIN;
		// PVZU elite-all: Normal, Cone, Pole read exe 0x800258 (the value); the Bucket's Rand(9) is patched (the setting)
		else if (const int aOpt = aType == ZombieType::ZOMBIE_PAIL ? EliteOption() : EliteValue();
		         u::NormalForcedCoin(aType, aOpt, theZombie->mApp->IsIZombieLevel()))
			v = u::elite::SILVER_COIN; // 0xc111c7: I, Zombie with option 2, no roll
		else if (const int aIz = IZombieNormalVariant(theZombie, aOpt); aIz >= 0) // PVZU Plan 9 R4: Rand(7) / Rand(30) + 1 (0xc111af)
			v = aIz;
		else if (const int n = u::EliteRollRange(aType, gSkyDropDepth > 0, aOpt); n > 0) // PVZU: elite option "none" skips the roll
			v = u::RollEliteVariant(aType, Rand(n), World7WorldNumber(theZombie, WorldNumber(theZombie)), gSkyDropDepth > 0, aOpt); // PVZU Plan 7: road sign gate = profile level 61..70
		theZombie->mPvzu.variant = v;
		theZombie->mBodyHealth = u::ZombieStartBodyHp(aType, v, theZombie->mBodyHealth);
		if (const int extra = u::EliteAttackRectExtraPx(v)) {
			theZombie->mZombieAttackRect.mX -= extra;
			theZombie->mZombieAttackRect.mWidth += extra;
		}
		ApplyEliteArt(theZombie, false);
	}
	NightEliteInit(theZombie, onBoard); // PVZU Plan 3: Newspaper, Screen Door, Football, Dancer forms (feature nightZombies)
	PoolEliteInit(theZombie, onBoard);  // PVZU Plan 4: Snorkel, Dolphin Rider, Zomboni forms (feature poolZombies)
	FogEliteInit(theZombie, onBoard);   // PVZU Plan 5: Jack, Balloon, Digger, Pogo forms (feature fogZombies)
	RoofEliteInit(theZombie, onBoard);  // PVZU Plan 6: Bungee, Ladder, Catapult, Gargantuar forms (feature roofZombies)
	World7EliteInit(theZombie, onBoard); // PVZU Plan 7: Tall-nut head 1-in-12 (feature world7Zombies)
	gModModeNoElites = false;
	if (onBoard && theZombie->mBoard)
		LogZombieEvent("t=%d spawn %s#%d variant=%s(0x%x) sub=0x%x row=%d x=%.1f body=%d helm=%d shield=%d wave=%d sky=%d",
			BoardTime(theZombie), TypeName(aType), ZombieNumber(theZombie), FormName(theZombie).c_str(),
			theZombie->mPvzu.variant, theZombie->mPvzu.subVariant, theZombie->mRow, theZombie->mPosX, theZombie->mBodyHealth,
			theZombie->mHelmHealth, theZombie->mShieldHealth, theZombie->mFromWave, gSkyDropDepth > 0 ? 1 : 0);
}

void EliteUpdate(Zombie* theZombie) {
	if (!DayZombiesOn() || theZombie->mDead || !theZombie->IsOnBoard())
		return;
	Board* aBoard = theZombie->mBoard;
	LawnApp* anApp = theZombie->mApp;
	// study 3: the Flag's "Horn of Dark Night", 1..3 sun every 40 cs
	if (u::FlagDrainsThisTick(theZombie->mZombieType, theZombie->mZombieAge) && !theZombie->mMindControlled &&
		theZombie->mHasHead && Alive(theZombie) &&
		u::FlagDrainAllowedIn(anApp->mGameMode, anApp->IsIZombieLevel(), anApp->IsAdventureMode(), aBoard->mLevel)) {
		if (const int n = u::FlagSunDrainRollRange(); n > 0) {
			const int take = u::FlagSunDrain(Rand(n));
			aBoard->mSunMoney = std::max(0, aBoard->mSunMoney - take);
			LogZombieEvent("t=%d drain FLAG#%d took=%d sun=%d", BoardTime(theZombie), ZombieNumber(theZombie), take,
				aBoard->mSunMoney);
		}
	}
	// study 4.2 / 5.2: phonograph heal and pumpkin buckets, every 650 cs while it has its head
	if (u::HasAura(theZombie->mPvzu.variant) && theZombie->mHasHead && Alive(theZombie) &&
		u::AuraTick(theZombie->mPvzu.counter))
		AuraPulse(theZombie);
}

float EliteAnimRate(Zombie* theZombie, float theAnimRate) {
	if (!DayZombiesOn())
		return theAnimRate;
	return theAnimRate * u::EliteAnimRateMultiplier(theZombie->mZombieType, theZombie->mPvzu.variant);
}

bool BitesThisTick(Zombie* theZombie, int theTicksBetweenEats) {
	if (auto aNight = NightBitesThisTick(theZombie, theTicksBetweenEats)) // mad Newspaper, green Football (hook 672)
		return *aNight;
	if (auto aPile = RoofPileBitesThisTick(theZombie)) // PVZU Plan 9 V1: ladder pile 3 of 4 / every tick (0x79c300)
		return *aPile;
	if (!DayZombiesOn())
		return theZombie->mZombieAge % theTicksBetweenEats == 0;
	return u::BitesThisTick(theZombie->mZombieType, theZombie->mZombieAge, theTicksBetweenEats);
}

bool EatsPlants(Zombie* theZombie) { // PVZU Plan 4: the lotus Snorkel never bites plants either
	return (!DayZombiesOn() || u::EliteEatsPlants(theZombie->mPvzu.variant)) && PoolEatsPlants(theZombie) &&
		RoofEatsPlants(theZombie); // PVZU Plan 6: the ladder pile never bites while it carries a ladder
}

bool ShowsYuckyFace(Zombie* theZombie) { return !DayZombiesOn() || u::EliteShowsYuckyFace(theZombie->mPvzu.variant); }

void CoinStep(Zombie* theZombie) {
	if (!DayZombiesOn() || !u::CoinSquishes(theZombie->mZombieType, theZombie->mPvzu.variant, theZombie->mHasArm,
		theZombie->mHasHead))
		return;
	auto livePlants = [&] {
		int n = 0;
		for (Plant* p : theZombie->mBoard->mPlants)
			n += (p->mDead || p->mSquished) ? 0 : 1;
		return n;
	};
	const bool logging = ZombieLog() != nullptr;
	const int before = logging ? livePlants() : 0;
	theZombie->CheckSquish(ZombieAttackType::ATTACKTYPE_DRIVE_OVER);
	if (logging && livePlants() < before)
		LogZombieEvent("t=%d squish silver-coin#%d row=%d x=%d", BoardTime(theZombie), ZombieNumber(theZombie),
			theZombie->mRow, theZombie->mX);
}

bool EliteEatPlant(Zombie* theZombie, Plant* thePlant) {
	if (!DayZombiesOn() || !u::CoinCopiesOnBite(theZombie->mZombieType, theZombie->mPvzu.variant, thePlant->mPlantHealth))
		return false;
	Board* aBoard = theZombie->mBoard;
	LawnApp* anApp = theZombie->mApp;
	ZombieType aCopyType = ZombieType::ZOMBIE_NORMAL; // hook 686
	if (anApp->mPlayerInfo && u::CoinCopyNeedsFootballRoll(anApp->mPlayerInfo->mLevel) &&
		u::CoinCopyFootballRollRange() > 0 && u::CoinCopyIsFootball(Rand(u::CoinCopyFootballRollRange())))
		aCopyType = ZombieType::ZOMBIE_FOOTBALL;
	gCoinCopyPending = aCopyType == ZombieType::ZOMBIE_NORMAL;
	Zombie* aCopy = nullptr;
	{
		World7GoldFootballScope aGold; // PVZU Plan 7: the Football copy is a gold Football ([0x9dd48e] = 2)
		aCopy = aBoard->AddZombieInRow(aCopyType, theZombie->mRow, theZombie->mFromWave);
	}
	gCoinCopyPending = false;
	if (aCopy)
		aCopy->mPosX = theZombie->mPosX;
	LogZombieEvent("t=%d copy silver-coin#%d -> %s#%d row=%d x=%.1f plantHp=%d", BoardTime(theZombie),
		ZombieNumber(theZombie), TypeName(aCopyType), aCopy ? ZombieNumber(aCopy) : -1, theZombie->mRow, theZombie->mPosX,
		thePlant->mPlantHealth);
	// The plant dies at once; the vanilla "plant eaten" steps run once (see SOURCES-day-zombies.md, ruling).
	anApp->PlaySample(SOUND_GULP);
	aBoard->mPlantsEaten++;
	thePlant->Die();
	aBoard->mChallenge->ZombieAtePlant(thePlant);
	return true;
}

// ---- glove pole leap (Plan 9 V1: hook 361 -> 0x95adf0, 0x95af00; hook 557 -> 0x95ad80) ----

namespace {
u::PlantDeathCause gPlantDeathCause = u::PlantDeathCause::OTHER;
}

PlantDeathCauseScope::PlantDeathCauseScope(u::PlantDeathCause theCause) : mPrev(gPlantDeathCause) { gPlantDeathCause = theCause; }
PlantDeathCauseScope::~PlantDeathCauseScope() { gPlantDeathCause = mPrev; }

void GlovePlantDied(Plant* thePlant) {
	if (!DayZombiesOn() || !u::PlantDeathWakesGlove(gPlantDeathCause) || !thePlant->IsOnBoard())
		return;
	PlantDeathCauseScope aOther(u::PlantDeathCause::OTHER); // deaths inside are named again
	Board* aBoard = thePlant->mBoard;
	for (Zombie* aZombie : aBoard->mZombies) {
		u::GloveLeapCheck c;
		c.type = aZombie->mZombieType;
		c.variant = aZombie->mPvzu.variant;
		c.phase = aZombie->mZombiePhase;
		c.deadOrDying = aZombie->IsDeadOrDying();
		c.zombieRow = aZombie->mRow;
		c.zombieX = aZombie->mX;
		c.plantRow = thePlant->mRow;
		c.plantX = thePlant->mX;
		if (!u::GloveLeapsFor(c))
			continue;
		const u::GloveLeap aLeap = u::GloveLeapAt(thePlant->mX, aZombie->mZombiePhase);
		const float aOldX = aZombie->mPosX;
		aZombie->mPosX = aLeap.posX; // 0x95ae7e, before the bite (mX is still the old spot, so the bite finds its plant)
		Plant* aBitten = nullptr;
		if (aZombie->mIsEating) { // 0x95af00
			if (!aZombie->FindZombieTarget()) {
				aBitten = aZombie->FindPlantTarget(ZombieAttackType::ATTACKTYPE_CHEW);
				if (aBitten) { // 0x950000(plant, 800, 1); a kill there wakes the gloves again (exe EBX = the damage)
					aBitten->mPlantHealth -= ModPlantBitten(aZombie, aBitten, aLeap.bite);
					aBitten->mRecentlyEatenCountdown = 50;
					if (aBitten->mPlantHealth <= 0) {
						PlantDeathCauseScope aBite(u::PlantDeathCause::LEAP_BITE);
						aBitten->Die();
					}
				}
			}
			aZombie->StopEating();
		}
		// 0x95ae8e: a nested leap (from the bite's kill) has already set phase 12, so this one reads "no pole"
		const u::GloveLeap aMode = u::GloveLeapAt(thePlant->mX, aZombie->mZombiePhase);
		aZombie->mPvzu.extra = aMode.jumpMode;
		aZombie->mZombiePhase = ZombiePhase::PHASE_POLEVAULTER_IN_VAULT;
		aZombie->PlayZombieReanim("anim_jump", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 20, 24.0f);
		aZombie->mVelX = aMode.velX;
		LogZombieEvent("t=%d glove-leap glove#%d row=%d x=%.1f->%.1f plant=%d@%d mode=%d bitten=%d", BoardTime(aZombie),
			ZombieNumber(aZombie), aZombie->mRow, aOldX, aZombie->mPosX, static_cast<int>(thePlant->mSeedType), thePlant->mX,
			aMode.jumpMode, aBitten ? aBitten->mPlantHealth : -1);
	}
}

bool GloveLandingKeepsPole(Zombie* theZombie) {
	if (!DayZombiesOn() || theZombie->mZombieType != ZombieType::ZOMBIE_POLEVAULTER ||
		!u::GloveLandingKeepsPole(theZombie->mPvzu.variant, theZombie->mPvzu.extra))
		return false;
	theZombie->mPvzu.extra = 0;
	theZombie->mX = static_cast<int>(theZombie->mPosX);
	theZombie->mZombiePhase = ZombiePhase::PHASE_POLEVAULTER_PRE_VAULT; // attack rect kept (the glove's +112)
	theZombie->StartWalkAnim(0);
	LogZombieEvent("t=%d glove-land glove#%d row=%d x=%.1f keeps pole", BoardTime(theZombie), ZombieNumber(theZombie),
		theZombie->mRow, theZombie->mPosX);
	return true;
}

void EliteOnDeath(Zombie* theZombie) {
	if (!DayZombiesOn() || !u::DeathEffectAllowed(theZombie->mZombieHeight, theZombie->mFromWave) || !theZombie->mBoard)
		return;
	const int v = theZombie->mPvzu.variant;
	if (u::SummonsDancersOnDeath(v)) { // study 4.3 (hook 691)
		theZombie->mHasHead = true;
		theZombie->SummonBackupDancers();
		theZombie->mHasHead = false;
		int summoned = 0;
		for (ZombieID id : theZombie->mFollowerZombieID)
			summoned += theZombie->mBoard->ZombieTryToGet(id) ? 1 : 0;
		LogZombieEvent("t=%d death dancer-head#%d row=%d x=%d dancers=%d", BoardTime(theZombie), ZombieNumber(theZombie),
			theZombie->mRow, theZombie->mX, summoned);
	} else if (u::SkyDropsOnDeath(v)) { // study 5.3
		LogZombieEvent("t=%d death umbrella-leaf#%d row=%d x=%d skydrop", BoardTime(theZombie), ZombieNumber(theZombie),
			theZombie->mRow, theZombie->mX);
		theZombie->mBoard->SpawnZombiesFromSky();
	}
}

ZombieType SkyDropZombieType(Board* theBoard) {
	auto aVanillaPick = [theBoard] { return static_cast<int>(theBoard->PickGraveRisingZombieType()); };
	if (!DayZombiesOn())
		return static_cast<ZombieType>(aVanillaPick());
	// Hook 85: the exe draws the vanilla pick first (0x412CA3), then re-rolls Rand(25) to one of 13 types.
	const ZombieType aType = static_cast<ZombieType>(u::SkyDropType(aVanillaPick, [](int n) { return Rand(n); }));
	LogZombieEvent("t=%d skydrop pick %s", static_cast<int>(theBoard->mMainCounter), TypeName(aType));
	return aType;
}

bool ForcedVariantFor(ZombieType theType, int& theVariant) {
	if (gModModeNoElites) {
		theVariant = 0;
		return true;
	}
	const auto& forced = ForcedVariants();
	auto it = forced.find(theType);
	if (it == forced.end())
		return false;
	theVariant = it->second;
	return true;
}

SkyDropScope::SkyDropScope() { ++gSkyDropDepth; }
SkyDropScope::~SkyDropScope() { --gSkyDropDepth; }

void EliteAfterLoad(Zombie* theZombie) {
	if (theZombie->mDead)
		return;
	NightAfterLoad(theZombie); // PVZU Plan 3 (feature nightZombies)
	PoolAfterLoad(theZombie);  // PVZU Plan 4 (feature poolZombies)
	FogAfterLoad(theZombie);   // PVZU Plan 5 (feature fogZombies)
	RoofAfterLoad(theZombie);  // PVZU Plan 6 (feature roofZombies)
	World7AfterLoad(theZombie); // PVZU Plan 7 (feature world7Zombies)
	if (!DayZombiesOn())
		return;
	if (theZombie->mPvzu.variant != u::elite::NONE && !u::IsNightType(theZombie->mZombieType))
		ApplyEliteArt(theZombie, true);
	LogZombieEvent("t=%d load %s#%d variant=%s(0x%x) sub=0x%x counter=%d stored=%d body=%d helm=%d shield=%d x=%.1f phase=%d",
		BoardTime(theZombie), TypeName(theZombie->mZombieType), ZombieNumber(theZombie), FormName(theZombie).c_str(),
		theZombie->mPvzu.variant, theZombie->mPvzu.subVariant, theZombie->mPvzu.counter, theZombie->mPvzu.stored,
		theZombie->mBodyHealth, theZombie->mHelmHealth, theZombie->mShieldHealth, theZombie->mPosX,
		static_cast<int>(theZombie->mZombiePhase));
}

} // namespace pvzu::engine
