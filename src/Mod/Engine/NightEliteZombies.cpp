// PVZU: engine side of the Unnamed Night zombies. Rules: Mod/Unnamed/NightZombies.h; evidence: the research notes (w2-night)
// section 2 and pvz-unnamed-pack/SOURCES-night-zombies.md.
#include "Mod/Engine/NightEliteZombies.h"
#include "Mod/Engine/OptionsPanel.h"
#include "Mod/Engine/ModModes.h"

#include "Mod/AssetOverlay.h"
#include "Mod/CmdLine.h"
#include "Mod/Engine/EliteZombies.h"
#include "Mod/Engine/EliteGaps.h"
#include "Mod/Engine/EngineTables.h"
#include "Mod/Engine/World7EliteZombies.h"
#include "Mod/Engine/ModesVanilla.h"
#include "Mod/GameData.h"
#include "Mod/Unnamed/DayZombies.h"
#include "Mod/Unnamed/FogZombies.h"
#include "Mod/Unnamed/RoofZombies.h"
#include "Mod/Unnamed/NightZombies.h"
#include "Mod/Unnamed/PoolZombies.h"
#include "Mod/Unnamed/ReanimCompiled32.h"

#include "LawnApp.h"
#include "Resources.h"
#include "Lawn/Board.h"
#include "Lawn/Plant.h"
#include "Lawn/Projectile.h"
#include "Lawn/Zombie.h"
#include "Lawn/System/PlayerInfo.h"
#include "PvzpLib/Definition.h"
#include "PvzpLib/Reanimator.h"
#include "SexyAppFramework/graphics/SharedImage.h"
#include "zlib.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <map>
#include <vector>

using namespace Sexy;
namespace u = pvzu::unnamed;

namespace pvzu::engine {

namespace {

u::NightForm Form(const Zombie* z) { return {z->mPvzu.variant, z->mPvzu.subVariant}; }

int BoardTime(const Zombie* z) { return z->mBoard ? static_cast<int>(z->mBoard->mMainCounter) : 0; }
int Id(Zombie* z) { return z->mBoard ? static_cast<int>(z->mBoard->ZombieGetID(z)) & 0xFFFF : -1; }
int PlayerLevel(const Zombie* z) { return z->mApp->mPlayerInfo ? z->mApp->mPlayerInfo->mLevel : 0; }

// -pvzu-force-subvariant NEWSPAPER:1,FOOTBALL:0x1A
const std::map<int, int>& ForcedSubVariants() {
	static std::map<int, int> forced;
	static bool parsed = false;
	if (parsed)
		return forced;
	parsed = true;
	auto opt = CmdOption("-pvzu-force-subvariant");
	if (!opt)
		return forced;
	RegisterVanillaTables();
	const auto& zombies = GameData::Get().Vanilla().zombies;
	std::string all = *opt;
	size_t start = 0;
	while (start <= all.size()) {
		const size_t comma = all.find(',', start);
		const std::string item = all.substr(start, comma == std::string::npos ? std::string::npos : comma - start);
		auto fv = u::ParseForcedVariant(item);
		bool known = false;
		for (size_t i = 0; fv && i < zombies.size(); ++i)
			if (zombies[i].name == fv->typeName) {
				forced[static_cast<int>(i)] = fv->variant;
				known = true;
			}
		if (!known)
			std::fprintf(stderr, "PVZU: -pvzu-force-subvariant: cannot read '%s' (want TYPE:code, e.g. FOOTBALL:0x1A)\n", item.c_str());
		if (comma == std::string::npos)
			break;
		start = comma + 1;
	}
	return forced;
}

// Elite art by pack file name; engine resources first (saves keep them), else the overlay file of that name.
Image* NightImage(const std::string& theFile) {
	static const std::map<std::string, Image**> kResources = {
		{"particles/ZombieFootballHead", &IMAGE_ZOMBIEFOOTBALLHEAD},
		{"reanim/Zombie_paper_madhead", &IMAGE_REANIM_ZOMBIE_PAPER_MADHEAD},
		{"reanim/coin_gold_dollar", &IMAGE_REANIM_COIN_GOLD_DOLLAR},
		{"reanim/Tallnut_cracked2", &IMAGE_REANIM_TALLNUT_CRACKED2},
		{"images/trophy", &IMAGE_TROPHY},
		{"images/SelectorScreen_AlmanacHighlight", &IMAGE_SELECTORSCREEN_ALMANACHIGHLIGHT},
	};
	if (auto it = kResources.find(theFile); it != kResources.end() && *it->second != nullptr)
		return *it->second;
	static std::map<std::string, SharedImageRef> loaded; // kept alive for the whole run
	auto it = loaded.find(theFile);
	if (it == loaded.end()) {
		it = loaded.emplace(theFile, gSexyAppBase->GetSharedImage(theFile)).first;
		if (static_cast<Image*>(it->second) == nullptr)
			std::fprintf(stderr, "PVZU: night elite image %s not found, drawing the base zombie\n", theFile.c_str());
	}
	return static_cast<Image*>(it->second);
}

void ApplyNightArt(Zombie* theZombie) {
	Reanimation* aBody = theZombie->mApp->ReanimationTryToGet(theZombie->mBodyReanimID);
	if (aBody == nullptr)
		return;
	if (theZombie->mZombieType == ZombieType::ZOMBIE_NEWSPAPER) // Unnamed's extra tracks are off unless the form shows them
		for (const auto& t : u::UnnamedOnlyNewspaperTracks())
			if (aBody->TrackExists(t.c_str()))
				aBody->AssignRenderGroupToTrack(t.c_str(), RENDER_GROUP_HIDDEN);
	u::NightArt art = u::ResolveNightArt(theZombie->mZombieType, Form(theZombie),
		[](const std::string& f) { return NightImage(f) != nullptr; });
	for (const auto& img : art.images)
		aBody->SetImageOverride(img.track.c_str(), NightImage(img.file));
	for (const auto& prefix : art.hidePrefixes)
		theZombie->ReanimShowPrefix(prefix.c_str(), RENDER_GROUP_HIDDEN);
	for (const auto& t : art.showTracks) {
		if (aBody->TrackExists(t.c_str()))
			aBody->AssignRenderGroupToTrack(t.c_str(), RENDER_GROUP_NORMAL);
		else
			LogZombieEvent("t=%d art %s#%d track %s missing (Unnamed reanim not merged): base look", BoardTime(theZombie),
				FormName(theZombie).c_str(), Id(theZombie), t.c_str());
	}
}

bool Alive(Zombie* z) { return z && !z->mDead && !z->IsDeadOrDying(); }

} // namespace

bool NightZombiesOn() { return GameData::Get().Feature("nightZombies"); }

std::string FormName(Zombie* theZombie) {
	if (const char* aW7 = World7FormName(theZombie)) // PVZU Plan 7: imp-gargantuar, vase, black, odd-0x101
		return aW7;
	if (u::IsNightType(theZombie->mZombieType))
		return u::NightEliteName(theZombie->mZombieType, theZombie->mPvzu.variant, theZombie->mPvzu.subVariant);
	if (u::IsPoolType(theZombie->mZombieType)) // PVZU Plan 4: lotus, football-head, orange/red Zomboni
		return u::PoolEliteName(theZombie->mZombieType, theZombie->mPvzu.variant, theZombie->mPvzu.extra);
	if (u::IsFogType(theZombie->mZombieType)) // PVZU Plan 5: ice/potato head, cloud, screwdriver, chocolate, garlic
		return u::FogEliteName(theZombie->mZombieType, theZombie->mPvzu.variant);
	if (u::IsRoofType(theZombie->mZombieType)) // PVZU Plan 6: wheelbarrow, Jack head, shovel, pile, ghost, pole head, staff
		return u::RoofEliteName(theZombie->mZombieType, theZombie->mPvzu.variant);
	return u::EliteName(theZombie->mPvzu.variant);
}

void NightEliteInit(Zombie* theZombie, bool theOnBoard) {
	const ZombieType aType = theZombie->mZombieType;
	if (!NightZombiesOn() || !u::IsNightType(aType))
		return;
	if (!theOnBoard) {
		if (Reanimation* aBody = theZombie->mApp->ReanimationTryToGet(theZombie->mBodyReanimID))
			HideUnnamedOnlyTracks(aBody, aType);
		return;
	}
	LawnApp* anApp = theZombie->mApp;
	const bool iZombie = anApp->IsIZombieLevel();
	u::NightForm f;
	const auto& forcedSub = ForcedSubVariants();
	int forcedVariant = 0;
	const bool forced = ForcedVariantFor(aType, forcedVariant) || forcedSub.count(aType) > 0;
	int aIZombieFollower = -1; // PVZU modesVanilla: an I, Zombie Dancer placed with a companion (0x79682f: no roll drawn)
	const bool aIZombieCombo = aType == ZombieType::ZOMBIE_DANCER && ModesForcedDancer(aIZombieFollower);
	if (!forced && (aType == ZombieType::ZOMBIE_NEWSPAPER || aType == ZombieType::ZOMBIE_DANCER))
		FootballHelmCheck(theZombie, EliteValue()); // PVZU Plan 9 R4: I, Zombie Rand(3) / board level > 70 helmet (0xc109bf)
	if (aIZombieCombo || (aType == ZombieType::ZOMBIE_DANCER && ModModeForcesSummoner(theZombie))) { // PVZU: mod mode 0x92 (hook 100)
		f = u::NightForm{u::nightelite::RANDOM_SUMMONER, 0};
	} else if (forced) { // a forced type rolls nothing (so the run's Rand sequence differs from a natural one: dev only)
		f.variant = forcedVariant;
		if (auto it = forcedSub.find(aType); it != forcedSub.end())
			f.subVariant = it->second;
	} else if (aType == ZombieType::ZOMBIE_FOOTBALL && World7ForcedGoldFootball()) { // PVZU Plan 7: coin copy, no rolls
		f = u::NightForm{u::nightelite::GOLD_FOOTBALL, 0};
	} else if (aType == ZombieType::ZOMBIE_FOOTBALL && EliteValue() == u::eliteoption::ALL) { // PVZU elite-all: 0xc12afd
		const u::FootballAllEliteForm aAll = u::RollFootballAllElite([](int n) { return Rand(n); }, World7ProfileAbove60(theZombie));
		f = aAll.form;
		f.subVariant = World7AllEliteFootballBlack(f.subVariant, aAll.black);
	} else if (aType == ZombieType::ZOMBIE_FOOTBALL && FootballDefaultRoll(theZombie, EliteValue(), f)) { // PVZU Plan 9 R4: gold+black, grave rises
	} else if (aType == ZombieType::ZOMBIE_FOOTBALL) { // 0x9DD310: Rand(4), Rand(13), then Rand(3) unless gold
		if (const int g = EliteValue() == u::eliteoption::NONE ? 0 : u::FootballGreenRange(); g > 0) { // PVZU: elite option "none" (hook 489 reads 0x800258)
			const int greenRoll = Rand(g);
			const int goldRoll = u::FootballGoldRange() > 0 ? Rand(u::FootballGoldRange()) : 1;
			f = u::RollFootball(greenRoll, goldRoll);
			if (u::FootballDrawsExtraRoll(f) && u::FootballExtraRange() > 0) // the black-armour roll (profile level > 60)
				f.subVariant = World7FootballExtraRoll(theZombie, f.subVariant, f.variant, Rand(u::FootballExtraRange())); // PVZU Plan 7
		}
	} else if (const int aOpt = aType == ZombieType::ZOMBIE_DOOR ? EliteOption() : EliteValue(), // PVZU elite-all: the Door's
	           // PVZU Plan 9 R4: the Newspaper also rolls in I, Zombie (0xc05770 has no I, Zombie test)
	           n = u::NightRollRange(aType, iZombie && !(aType == ZombieType::ZOMBIE_NEWSPAPER && NewspaperRollsInIZombie()), aOpt); n > 0) { // jump is patched (0x7b2000/0x7b2100), hooks 502/504 read the value
		const int r = Rand(n);
		if (aType == ZombieType::ZOMBIE_NEWSPAPER) {
			const PlayerInfo* p = anApp->mPlayerInfo;
			const bool almanac = u::NewspaperAlmanacAllowed(p ? p->mLevel : 0, p ? static_cast<int>(p->mFinishedAdventure) : 0,
				anApp->IsAdventureMode(), theZombie->mBoard ? theZombie->mBoard->mLevel : 0);
			f = NewspaperForm(theZombie, u::RollNewspaper(r, almanac, aOpt), r, almanac, aOpt); // PVZU Plan 9 R4: profile > 70 (0xc05788)
			NewspaperPaperHp(theZombie);
		} else if (aType == ZombieType::ZOMBIE_DOOR) {
			f = u::RollDoor(r, aOpt);
		} else if (aType == ZombieType::ZOMBIE_DANCER) {
			f = u::RollDancer(r, iZombie, aOpt);
		}
	}
	theZombie->mPvzu.variant = f.variant;
	theZombie->mPvzu.subVariant = f.subVariant;
	theZombie->mBodyHealth = u::NightBodyHp(aType, f, theZombie->mBodyHealth);
	theZombie->mHelmHealth = u::NightHelmHp(aType, f, theZombie->mHelmHealth);
	theZombie->mShieldHealth = u::NightShieldHp(aType, f, theZombie->mShieldHealth);
	if (aType == ZombieType::ZOMBIE_DANCER) {
		theZombie->mPhaseCounter = u::DancerStartPhaseCounter(f, theZombie->mPhaseCounter);
		theZombie->mPvzu.stored = u::DancerStartStored(f);
		if (aIZombieCombo) // 0xc04610: +0x12c = [0xc046c4], the companion's zombie type
			theZombie->mPvzu.stored = aIZombieFollower;
	}
	if (aType == ZombieType::ZOMBIE_NEWSPAPER && f.variant == u::nightelite::KNIFE)
		theZombie->mHasObject = true; // 0x901CEF (exe "has object" = the knife)
	ApplyNightArt(theZombie);
	World7FootballLook(theZombie); // PVZU Plan 7: black armour HP and art
}

void NightAfterLoad(Zombie* theZombie) {
	if (!NightZombiesOn() || theZombie->mDead || !u::IsNightType(theZombie->mZombieType))
		return;
	if (theZombie->mPvzu.variant != 0 || theZombie->mPvzu.subVariant != 0 || theZombie->mZombieType == ZombieType::ZOMBIE_NEWSPAPER)
		ApplyNightArt(theZombie);
	if (theZombie->mZombiePhase == ZombiePhase::PHASE_NEWSPAPER_MAD)
		if (Reanimation* aBody = theZombie->mApp->ReanimationTryToGet(theZombie->mBodyReanimID))
			aBody->SetImageOverride("anim_head1", NewspaperMadHead(theZombie, IMAGE_REANIM_ZOMBIE_PAPER_MADHEAD));
	if (theZombie->mZombieType == ZombieType::ZOMBIE_FOOTBALL && theZombie->mHelmType != HelmType::HELMTYPE_NONE)
		if (Reanimation* aBody = theZombie->mApp->ReanimationTryToGet(theZombie->mBodyReanimID))
			if (Image* img = HelmDamageImage(theZombie, theZombie->GetHelmDamageIndex(), nullptr))
				aBody->SetImageOverride("zombie_football_helmet", img);
}

// ---- dancers ----

bool SkipsDancerUpdate(Zombie* theZombie) { return NightZombiesOn() && u::DancerSkipsDancerUpdate(Form(theZombie)); }

bool DancerSkipsSummonCounterCheck(Zombie* theZombie) {
	return NightZombiesOn() && u::DancerSkipsSummonCounterCheck(Form(theZombie));
}

bool DancerSnapsWithoutCountdown() { return NightZombiesOn() && u::DancerSnapsWithoutCountdown(); }

bool DancerReplacesFollower(Zombie* theDancer, int theSlot) {
	Board* aBoard = theDancer->mBoard;
	Zombie* aFollower = aBoard->ZombieTryToGet(theDancer->mFollowerZombieID[theSlot]);
	if (!NightZombiesOn() || !u::DancerResummonsAll())
		return aFollower == nullptr;
	// 0xC04030: a living follower is let go (walks on as a normal zombie), then the slot is summoned again
	if (aFollower && !aFollower->mDead && aFollower->mZombiePhase != ZombiePhase::PHASE_ZOMBIE_DYING &&
		aFollower->mZombiePhase != ZombiePhase::PHASE_ZOMBIE_BURNED && aFollower->mZombiePhase != ZombiePhase::PHASE_ZOMBIE_MOWERED) {
		aFollower->mRelatedZombieID = ZombieID::ZOMBIEID_NULL;
		if (aFollower->mZombieType == ZombieType::ZOMBIE_BACKUP_DANCER &&
			aFollower->mZombiePhase != ZombiePhase::PHASE_DANCER_DANCING_LEFT) {
			aFollower->mVelX = 0.0f;
			aFollower->mZombiePhase = ZombiePhase::PHASE_ZOMBIE_NORMAL;
			aFollower->PlayZombieReanim("anim_walk", ReanimLoopType::REANIM_LOOP, 0, 0.0f);
		}
		LogZombieEvent("t=%d release %s#%d follower#%d slot=%d", BoardTime(theDancer), FormName(theDancer).c_str(), Id(theDancer),
			Id(aFollower), theSlot);
	}
	theDancer->mFollowerZombieID[theSlot] = ZombieID::ZOMBIEID_NULL;
	return true;
}

std::optional<ZombieID> SummonNightFollower(Zombie* theDancer, int theRow, int thePosX) {
	if (!NightZombiesOn() || !u::SummonsRandomFollowers(Form(theDancer)))
		return std::nullopt;
	const int n = u::RandomFollowerRollRange();
	if (n <= 0)
		return std::nullopt;
	// 0xc04720: a follower type in +0x12C (an I, Zombie Dancer placed with a companion, modesVanilla) is tried first, with
	// no roll; it still goes through the same filter, and a refused one rolls as usual. -1 = random.
	int r = theDancer->mPvzu.stored;
	if (r < 0 || r >= n || !u::RandomFollowerAccepted(r, PlayerLevel(theDancer)))
		do
			r = Rand(n);
		while (!u::RandomFollowerAccepted(r, PlayerLevel(theDancer)));
	Board* aBoard = theDancer->mBoard;
	Zombie* aZombie = aBoard->AddZombie(static_cast<ZombieType>(r), theDancer->mFromWave);
	if (aZombie == nullptr)
		return ZombieID::ZOMBIEID_NULL;
	const int aCol = std::clamp(aBoard->PixelToGridX(thePosX + 40, aBoard->GridToPixelY(0, theRow)), 0, MAX_GRID_SIZE_X - 1);
	aZombie->mZombiePhase = ZombiePhase::PHASE_ZOMBIE_NORMAL;
	aZombie->RiseFromGrave(aCol, theRow); // Ruling: rises like a grave zombie (the exe makes it dance, hooks 589-593)
	aZombie->mMindControlled = theDancer->mMindControlled;
	LogZombieEvent("t=%d summon %s#%d -> %s#%d row=%d col=%d", BoardTime(theDancer), FormName(theDancer).c_str(), Id(theDancer),
		GameData::Get().Vanilla().zombies[r].name.c_str(), Id(aZombie), theRow, aCol);
	return aBoard->ZombieGetID(aZombie);
}

// ---- movement and animation ----

float NightWalkRate(Zombie* z, float theAnimRate) {
	if (!NightZombiesOn())
		return theAnimRate;
	return theAnimRate * u::NightWalkRateMultiplier(z->mZombieType, Form(z), z->mBodyHealth, z->mBodyMaxHealth, z->mHelmHealth,
		z->mHelmMaxHealth);
}

float NightAnimRate(Zombie* z, float theAnimRate) {
	if (!NightZombiesOn())
		return theAnimRate;
	return theAnimRate * u::NightAnimRateMultiplier(z->mZombieType, Form(z), z->mZombiePhase);
}

bool MoonwalkStartWalk(Zombie* z) { // 0x7A95D0
	if (!NightZombiesOn() || z->mZombieType != ZombieType::ZOMBIE_DANCER || !u::DancerSkipsDancerUpdate(Form(z)) || z->mIsEating)
		return false;
	z->mVelX = u::MoonwalkVelX();
	z->PlayZombieReanim("anim_moonwalk", ReanimLoopType::REANIM_LOOP, 0, u::MoonwalkAnimRate());
	return true;
}

void MoonwalkResume(Zombie* z) { // 0x7A9560, 0x7A9501
	if (!NightZombiesOn() || z->mZombieType != ZombieType::ZOMBIE_DANCER || !u::DancerSkipsDancerUpdate(Form(z)) || z->mIsEating)
		return;
	z->PlayZombieReanim("anim_moonwalk", ReanimLoopType::REANIM_LOOP, 0, u::MoonwalkAnimRate());
}

const char* NewspaperAnim(Zombie* z, const char* theVanillaAnim) {
	if (!NightZombiesOn() || z->mZombieType != ZombieType::ZOMBIE_NEWSPAPER || z->mPvzu.variant != u::nightelite::KNIFE || !z->mHasObject)
		return theVanillaAnim;
	const char* aKnifeAnim = u::KnifeAnim(theVanillaAnim);
	Reanimation* aBody = z->mApp->ReanimationTryToGet(z->mBodyReanimID);
	return aBody && aBody->TrackExists(aKnifeAnim) ? aKnifeAnim : theVanillaAnim;
}

Image* NewspaperMadHead(Zombie* z, Image* theVanilla) {
	if (!NightZombiesOn())
		return theVanilla;
	const std::string f = u::NewspaperMadHeadFile(Form(z));
	if (f.empty())
		return theVanilla;
	Image* img = NightImage(f);
	return img ? img : theVanilla;
}

void NightBeforeDraw(Zombie* z, Reanimation* theBodyReanim) {
	if (theBodyReanim && NightZombiesOn() && u::DrawsWhite(z->mZombieType, Form(z)))
		theBodyReanim->mFilterEffect = FilterEffect::FILTER_EFFECT_WHITE; // hook 649: reanim+0x98 = 2 at every draw
}

// ---- eating ----

std::optional<bool> NightBitesThisTick(Zombie* z, int theTicksBetweenEats) {
	if (!NightZombiesOn())
		return std::nullopt;
	return u::NightBitesThisTick(z->mZombieType, Form(z), z->mZombiePhase, z->mChilledCounter > 0, z->mZombieAge % theTicksBetweenEats);
}

int PreyChecksPerTick(Zombie* z) { return NightZombiesOn() ? u::CheckIfPreyCaughtCallsPerTick(z->mZombiePhase) : 1; }

bool SkipsNormalBite(Zombie* z) { return NightZombiesOn() && u::SkipsNormalBite(z->mZombieType, Form(z)); }

void NightChewEffect(Zombie* z, Plant* thePlant) {
	if (!NightZombiesOn())
		return;
	const int before = thePlant->mEatenFlashCountdown;
	thePlant->mEatenFlashCountdown = u::KnifeChewFlash(before, z->mZombieType, Form(z), z->mZombiePhase, z->mMindControlled,
		thePlant->mSeedType);
	if (thePlant->mEatenFlashCountdown != before && thePlant->mEatenFlashCountdown > u::kKnifePendingKill)
		LogZombieEvent("t=%d knife %s#%d marks plant col=%d row=%d flash=0x%x", BoardTime(z), FormName(z).c_str(), Id(z),
			thePlant->mPlantCol, thePlant->mRow, thePlant->mEatenFlashCountdown);
	// hook 640, 0xC12CAA: the green Football's chew after 8 s with its helmet on
	if (z->mZombieType == ZombieType::ZOMBIE_FOOTBALL && !GreenEventInAnimate() && // PVZU Plan 9 R4: hook 640 is in Animate
		u::GreenDropsHelmOnChew(Form(z), z->mHelmType != HelmType::HELMTYPE_NONE, z->mZombieAge, PlayerLevel(z))) {
		z->DropHelm(0U);
		z->mBodyHealth += u::GreenChewBodyBonus(PlayerLevel(z));
		LogZombieEvent("t=%d green-helmet-drop FOOTBALL#%d body=%d", BoardTime(z), Id(z), z->mBodyHealth);
	}
}

void PlantFlashTick(Plant* thePlant) {
	if (!NightZombiesOn() || !thePlant->mIsOnBoard || thePlant->mDead || !u::KnifeKillsNow(thePlant->mEatenFlashCountdown))
		return;
	thePlant->mEatenFlashCountdown = 0;
	LogZombieEvent("t=%d knife kills plant col=%d row=%d hp=%d", static_cast<int>(thePlant->mBoard->mMainCounter),
		thePlant->mPlantCol, thePlant->mRow, thePlant->mPlantHealth);
	thePlant->mPlantHealth = 0; // 0x901E5B: damage HP + 1; not EatPlant's death path, so Puff-shroom does not freeze
	thePlant->mApp->PlaySample(SOUND_GULP);
	thePlant->mBoard->mPlantsEaten++;
	thePlant->Die();
}

bool SkipsGarlic(Zombie* z) { return NightZombiesOn() && u::SkipsGarlic(z->mZombieType, Form(z)); }

int ButterCs(Zombie* z, int theVanilla) { return NightZombiesOn() ? u::MadButterCs(z->mZombieType, z->mZombiePhase, theVanilla) : theVanilla; }

int EatZombieDamage(Zombie* theEater, Zombie* theTarget, int theVanilla) {
	return NightZombiesOn() ? u::EatZombieDamage(theEater->mZombieType, theEater->mZombiePhase, theTarget->mZombieType, theVanilla) : theVanilla;
}

unsigned int EatZombieFlags(unsigned int theVanilla) { return NightZombiesOn() ? u::EatZombieFlags(theVanilla) : theVanilla; }

void NightZombieStep(Zombie* z) {
	if (!NightZombiesOn() || !u::GoldSquishes(z->mZombieType, Form(z)) || z->mFlatTires)
		return;
	int before = 0;
	for (Plant* p : z->mBoard->mPlants) before += (p->mDead || p->mSquished) ? 0 : 1;
	z->CheckSquish(ZombieAttackType::ATTACKTYPE_DRIVE_OVER);
	int after = 0;
	for (Plant* p : z->mBoard->mPlants) after += (p->mDead || p->mSquished) ? 0 : 1;
	if (after < before)
		LogZombieEvent("t=%d squish %s#%d row=%d x=%d", BoardTime(z), FormName(z).c_str(), Id(z), z->mRow, z->mX);
}

// ---- damage ----

bool PaperSoaksBodyDamage(Zombie* z, int theDamage) {
	if (!NightZombiesOn() || z->mShieldType == ShieldType::SHIELDTYPE_NONE ||
		!u::ReadingPaperSoaksBodyDamage(z->mZombieType, Form(z), z->mZombiePhase))
		return false;
	z->TakeShieldDamage(theDamage, 0U); // hook 702: body damage set to 0
	return true;
}

bool ShieldAlwaysFirst(Zombie* z) { return NightZombiesOn() && u::ShieldAlwaysFirst(z->mZombieType, Form(z)); }

bool SkipsShieldDamageImages(Zombie* z) { return NightZombiesOn() && u::SkipsShieldDamageImages(z->mZombieType, Form(z)); }

void AlmanacPaperHit(Zombie* z) {
	if (NightZombiesOn() && z->mShieldType == ShieldType::SHIELDTYPE_NEWSPAPER)
		z->mBodyHealth += u::AlmanacPaperBodyPerHit(z->mZombieType, Form(z));
}

Image* HelmDamageImage(Zombie* z, int theDamageIndex, Image* theVanilla) {
	if (Image* aBlack = World7HelmDamageImage(z, theDamageIndex)) // PVZU Plan 7: black armour helmet (hooks 700 / 701)
		return aBlack;
	if (!NightZombiesOn() || z->mZombieType != ZombieType::ZOMBIE_FOOTBALL || z->mPvzu.subVariant != u::nightsub::GREEN)
		return theVanilla;
	const std::string f = u::GreenHelmImage(theDamageIndex);
	if (f.empty() || u::ResolveNightArt(z->mZombieType, Form(z), [](const std::string& x) { return NightImage(x) != nullptr; }).images.empty())
		return theVanilla;
	Image* img = NightImage(f);
	return img ? img : theVanilla;
}

// ---- projectiles ----

void DoorReflect(Projectile* p, Zombie* z, int theDamage, unsigned int theDamageFlags) {
	if (!NightZombiesOn() || !u::DoorReflects(z->mShieldType, theDamageFlags, p->mProjectileType, p->mMotionType))
		return;
	z->mShieldHealth += u::DoorReflectShieldBonus(theDamage); // 0xC10DF0, then TakeDamage takes the full shot
	const u::Reflection r = u::ReflectedShot(z->IsWalkingBackwards());
	p->mMotionType = r.motion;
	p->mDamageRangeFlags = r.damageRangeFlags;
	p->mTargetZombieID = static_cast<ZombieID>(1); // exe +0x88 = 1: the next Die() (DoImpact's) is skipped once
	LogZombieEvent("t=%d reflect %s#%d projectile=%d row=%d door=%d", BoardTime(z), FormName(z).c_str(), Id(z),
		static_cast<int>(p->mProjectileType), z->mRow, z->mShieldHealth);
}

bool ProjectileSurvivesDie(Projectile* p) { // 0x956010 -> 0x651CE7
	if (!NightZombiesOn() || p->mTargetZombieID == ZombieID::ZOMBIEID_NULL ||
		(p->mMotionType != ProjectileMotion::MOTION_STRAIGHT && p->mMotionType != ProjectileMotion::MOTION_BACKWARDS))
		return false;
	p->mTargetZombieID = ZombieID::ZOMBIEID_NULL;
	return true;
}

bool ReflectedShotCollision(Projectile* p) {
	// hook 380: a straight shot with no damage-range flags (a reflected one) hits plants; hook 381: if none,
	// it can still hit a hypnotised zombie (0x955ECC, magic 0x114514)
	if (!NightZombiesOn() || p->mDamageRangeFlags != 0 || p->mMotionType != ProjectileMotion::MOTION_BACKWARDS ||
		p->mProjectileType == ProjectileType::PROJECTILE_ZOMBIE_PEA)
		return false;
	if (Plant* aPlant = p->FindCollisionTargetPlant()) {
		aPlant->mPlantHealth -= p->GetProjectileDef().mDamage;
		aPlant->mEatenFlashCountdown = std::max(aPlant->mEatenFlashCountdown, 25);
		p->mApp->PlayFoley(FoleyType::FOLEY_SPLAT);
		p->mApp->AddPvzpParticle(p->mPosX - 3.0f, p->mPosY + 17.0f, p->mRenderOrder + 1, ParticleEffect::PARTICLE_PEA_SPLAT);
		LogZombieEvent("t=%d reflected-hit plant col=%d row=%d hp=%d", static_cast<int>(p->mBoard->mMainCounter),
			aPlant->mPlantCol, aPlant->mRow, aPlant->mPlantHealth);
		if (aPlant->mPlantHealth <= 0)
			aPlant->Die();
		p->Die();
		return true;
	}
	p->mDamageRangeFlags = (1 << DamageRangeFlags::DAMAGES_ONLY_MINDCONTROLLED) | (1 << DamageRangeFlags::DAMAGES_GROUND);
	Zombie* aZombie = p->FindCollisionTarget();
	p->mDamageRangeFlags = 0;
	if (aZombie)
		p->DoImpact(aZombie);
	return true;
}

bool GoldRaisesGraveFootballs(Zombie* z) { return NightZombiesOn() && u::GoldRaisesGraveFootballs(z->mZombieType, Form(z)); }

bool DancerCountsCarnival(Zombie* theDancer) { return NightZombiesOn() && u::DancerCountsCarnival(Form(theDancer)); }

// ---- reanims ----

void MergeUnnamedReanimTracks(const std::string& theReanimFile, ReanimatorDefinition* theDef) {
	if (!NightZombiesOn() || theDef->mTracks.tracks == nullptr)
		return;
	static const char* const kMerged[] = {"reanim/Zombie_paper.reanim"};
	bool wanted = false;
	for (const char* f : kMerged)
		wanted |= strcasecmp(theReanimFile.c_str(), f) == 0;
	wanted |= MergesPlainZombieReanim() && strcasecmp(theReanimFile.c_str(), "reanim/Zombie.reanim") == 0; // PVZU Plan 9 R4: anim_football
	if (!wanted)
		return;
	auto path = OverlayResolve(DefinitionGetCompiledFilePathFromXMLFilePath(theReanimFile));
	if (!path) {
		std::fprintf(stderr, "PVZU: no Unnamed compiled reanim for %s, extra tracks off\n", theReanimFile.c_str());
		return;
	}
	std::ifstream in(*path, std::ios::binary);
	std::vector<uint8_t> file((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
	const auto usize = u::CompiledUncompressedSize(file.data(), file.size());
	if (!usize || file.size() < 8) {
		std::fprintf(stderr, "PVZU: %s is not a compiled reanim\n", path->string().c_str());
		return;
	}
	std::vector<uint8_t> raw(*usize);
	uLongf rawSize = *usize;
	if (uncompress(raw.data(), &rawSize, file.data() + 8, static_cast<uLong>(file.size() - 8)) != Z_OK || rawSize != *usize) {
		std::fprintf(stderr, "PVZU: %s: zlib failed\n", path->string().c_str());
		return;
	}
	std::string err;
	auto unnamed = u::DecodeReanim32(raw.data(), raw.size(), &err);
	if (!unnamed) {
		std::fprintf(stderr, "PVZU: %s: %s\n", path->string().c_str(), err.c_str());
		return;
	}
	std::vector<std::string> names;
	std::vector<int> counts;
	for (int i = 0; i < theDef->mTracks.count; ++i) {
		names.emplace_back(theDef->mTracks.tracks[i].mName);
		counts.push_back(theDef->mTracks.tracks[i].mTransforms.count);
	}
	auto plan = u::PlanTrackMerge(names, counts, *unnamed);
	if (!plan) {
		std::fprintf(stderr, "PVZU: %s: Unnamed tracks do not line up with GOTY's, extra tracks off\n", theReanimFile.c_str());
		return;
	}
	auto* aTracks = static_cast<ReanimatorTrack*>(DefinitionAlloc(static_cast<int>(sizeof(ReanimatorTrack) * plan->size())));
	int added = 0;
	for (size_t i = 0; i < plan->size(); ++i) {
		// every track from Unnamed's data: its GOTY frames match within 0.3 px and its new animations follow them
		const u::Reanim32Track& t = unnamed->tracks[i];
		added += (*plan)[i].fromVanilla ? 0 : 1;
		char* aName = static_cast<char*>(DefinitionAlloc(static_cast<int>(t.name.size() + 1)));
		std::memcpy(aName, t.name.c_str(), t.name.size() + 1);
		aTracks[i].mName = aName;
		aTracks[i].mTransforms.count = static_cast<int32_t>(t.transforms.size());
		aTracks[i].mTransforms.mTransforms =
			static_cast<ReanimatorTransform*>(DefinitionAlloc(static_cast<int>(sizeof(ReanimatorTransform) * t.transforms.size())));
		for (size_t k = 0; k < t.transforms.size(); ++k) {
			const u::Reanim32Transform& x = t.transforms[k];
			ReanimatorTransform& d = aTracks[i].mTransforms.mTransforms[k];
			d = ReanimatorTransform();
			d.mTransX = x.x; d.mTransY = x.y; d.mSkewX = x.kx; d.mSkewY = x.ky;
			d.mScaleX = x.sx; d.mScaleY = x.sy; d.mFrame = x.f; d.mAlpha = x.a;
			if (!x.image.empty() && !DefinitionLoadImage(&d.mImage, x.image))
				std::fprintf(stderr, "PVZU: %s track %s: image %s not found\n", theReanimFile.c_str(), t.name.c_str(), x.image.c_str());
			// fonts and text are not used by Zombie_paper's tracks: left empty
		}
	}
	theDef->mTracks.tracks = aTracks; // the old arrays stay allocated (definitions live for the whole run)
	theDef->mFPS = unnamed->fps;
	theDef->mTracks.count = static_cast<int32_t>(plan->size());
	std::fprintf(stderr, "PVZU: %s: Unnamed's definition used (%d tracks, %d of them Unnamed-only, %zu frames)\n",
		theReanimFile.c_str(), theDef->mTracks.count, added, unnamed->tracks[0].transforms.size());
}

void HideUnnamedOnlyTracks(Reanimation* theReanim, ZombieType theType) {
	if (!NightZombiesOn() || theType != ZombieType::ZOMBIE_NEWSPAPER)
		return;
	for (const auto& t : u::UnnamedOnlyNewspaperTracks())
		if (theReanim->TrackExists(t.c_str()))
			theReanim->AssignRenderGroupToTrack(t.c_str(), RENDER_GROUP_HIDDEN);
}

} // namespace pvzu::engine
