// PVZU: engine side of Plan 9 R4. Rules: Mod/Unnamed/EliteGaps.h; evidence: the research notes (r4-football) and
// pvz-unnamed-pack/SOURCES-elite-gaps.md.
#include "Mod/Engine/EliteGaps.h"

#include "Mod/Engine/EliteZombies.h"
#include "Mod/Engine/NightEliteZombies.h"
#include "Mod/Engine/NightLevels.h"
#include "Mod/Engine/World7EliteZombies.h"
#include "Mod/GameData.h"
#include "Mod/Unnamed/DayZombies.h"
#include "Mod/Unnamed/EliteGaps.h"
#include "Mod/Unnamed/Options.h"
#include "Mod/Unnamed/Rules.h"
#include "Mod/Unnamed/World7Zombies.h"

#include "LawnApp.h"
#include "Resources.h"
#include "Lawn/Board.h"
#include "Lawn/Zombie.h"
#include "Lawn/System/PlayerInfo.h"
#include "PvzpLib/Attachment.h"
#include "PvzpLib/PvzpCommon.h"
#include "PvzpLib/Reanimator.h"

using namespace Sexy;
namespace u = pvzu::unnamed;

namespace pvzu::engine {

namespace {

int BoardTime(const Zombie* z) { return z->mBoard ? static_cast<int>(z->mBoard->mMainCounter) : 0; }
int Id(Zombie* z) { return z->mBoard ? static_cast<int>(z->mBoard->ZombieGetID(z)) & 0xFFFF : -1; }
int ProfileLevel(const Zombie* z) { return z->mApp && z->mApp->mPlayerInfo ? z->mApp->mPlayerInfo->mLevel : 0; }
constexpr const char* kHelmTrack = "anim_football";

// 0xc12ce0 .. 0xc12df8: the vanilla Gatling-head attach (Zombie.cpp ZOMBIE_GATLING_HEAD) on the Football's body
bool AttachGatlingHead(Zombie* z) {
	Reanimation* aBody = z->mApp->ReanimationTryToGet(z->mBodyReanimID);
	if (aBody == nullptr || !aBody->TrackExists("anim_head1"))
		return false;
	z->ReanimShowPrefix("anim_hair", RENDER_GROUP_HIDDEN);
	z->ReanimShowPrefix("anim_head2", RENDER_GROUP_HIDDEN);
	ReanimatorTrackInstance* aTrack = aBody->GetTrackInstanceByName("anim_head1");
	aTrack->mImageOverride = IMAGE_BLANK;
	Reanimation* aHead = z->mApp->AddReanimation(0.0f, 0.0f, 0, ReanimationType::REANIM_GATLINGPEA);
	aHead->PlayReanim("anim_head_idle", ReanimLoopType::REANIM_LOOP, 0, 15.0f);
	z->mSpecialHeadReanimID = z->mApp->ReanimationGetID(aHead);
	AttachEffect* anAttach = AttachReanim(aTrack->mAttachmentID, aHead, 0.0f, 0.0f);
	aBody->mFrameBasePose = 0;
	const u::HeadOffset o = u::GatlingFootballHeadOffset();
	PvzpScaleRotateTransformMatrix(anAttach->mOffset, o.x, o.y, o.rot, o.sx, o.sy);
	z->mPhaseCounter = u::GatlingFootballStartCounter();
	z->mVariant = false;
	z->mPvzu.variant = u::gaps::GATLING_FOOTBALL;
	return true;
}

} // namespace

bool EliteGapsOn() { return GameData::Get().Feature("eliteGaps"); }

// ---- black armour ----

int BlackArmourDamageTaken(Zombie* z, int theDamage) {
	if (!EliteGapsOn() || z->mZombieType != ZombieType::ZOMBIE_FOOTBALL || z->mPvzu.subVariant != u::w7::BLACK_FOOTBALL)
		return theDamage;
	int d = u::BlackArmourDamage(theDamage, z->mIsEating);
	if (const int n = u::BlackArmourSmallHitRollRange(d); n > 0)
		d = u::BlackArmourSmallHit(d, Rand(n));
	LogZombieEvent("t=%d gaps black-armour FOOTBALL#%d hit %d -> %d eating=%d", BoardTime(z), Id(z), theDamage, d,
		z->mIsEating ? 1 : 0);
	return d;
}

bool BlackArmourRefusesFreeze(Zombie* z) {
	return EliteGapsOn() && u::BlackArmourRefusesFreeze(z->mZombieType, z->mPvzu.subVariant);
}

// ---- Football roll ----

bool FootballDefaultRoll(Zombie* z, int theEliteValue, u::NightForm& theForm) {
	if (!EliteGapsOn() || z->mZombieType != ZombieType::ZOMBIE_FOOTBALL)
		return false;
	theForm = {};
	if (theEliteValue == u::eliteoption::NONE) // 0xc12af7 -> 0x9dd321: al = 1 -> done, nothing drawn
		return true;
	const u::FootballDefaultRolls r = u::RollFootballDefault([](int n) { return Rand(n); }, World7ProfileAbove60(z),
		GraveRiseGravesOnly());
	theForm = r.form;
	if (r.blackEligible) // the black decision with its world-7 key (gold no longer excluded: 0x9dd423)
		theForm.subVariant = World7FootballExtraRoll(z, theForm.subVariant, 0, r.extraRoll);
	return true;
}

// ---- green Football, Gatling head ----

bool GreenEventInAnimate() { return EliteGapsOn() && u::GreenWalkEventOn(); }

void GreenFootballWalkTick(Zombie* z) {
	if (!EliteGapsOn() || z->mZombieType != ZombieType::ZOMBIE_FOOTBALL || z->mDead || !z->IsOnBoard())
		return;
	const bool aRefresh = u::GreenRefreshesAnimSpeed(z->mZombieType, z->mPvzu.variant, z->mPvzu.subVariant);
	const u::GreenWalkEvent e = u::GreenFootballWalkEvent(z->mZombieType, z->mPvzu.variant, z->mPvzu.subVariant,
		z->mHelmType != HelmType::HELMTYPE_NONE, z->mZombieAge, ProfileLevel(z));
	if (e.fires) {
		if (e.dropHelm)
			z->DropHelm(0U);
		const bool aHead = e.gatlingHead && AttachGatlingHead(z);
		z->mBodyHealth += e.bodyBonus;
		LogZombieEvent("t=%d gaps green-walk FOOTBALL#%d age=%d helm-drop=%d gatling=%d body=%d helm=%d", BoardTime(z), Id(z),
			z->mZombieAge, e.dropHelm ? 1 : 0, aHead ? 1 : 0, z->mBodyHealth, z->mHelmHealth);
	}
	if (aRefresh)
		z->UpdateAnimSpeed();
}

bool RunsGatlingHeadUpdate(Zombie* z, bool theVanilla) {
	if (!EliteGapsOn())
		return theVanilla;
	if (!theVanilla && !u::IsGatlingFootball(z->mZombieType, z->mPvzu.variant))
		return false;
	return !u::GatlingUpdateSkipped(z->mZombiePhase); // 0xc03f70
}

bool GatlingFootballExtraShot(Zombie* z) {
	return EliteGapsOn() && u::IsGatlingFootball(z->mZombieType, z->mPvzu.variant) && u::GatlingFootballExtraShot(z->mPhaseCounter);
}

bool HeadPeasMaySpread(Zombie* z) {
	return !(EliteGapsOn() && z->mZombieType == ZombieType::ZOMBIE_FOOTBALL && !u::GatlingFootballSpreads());
}

// ---- Newspaper ----

u::NightForm NewspaperForm(Zombie* z, const u::NightForm& theRolled, int theRoll, bool theAlmanac, int theOption) {
	if (!EliteGapsOn())
		return theRolled;
	const u::NightForm f = u::NewspaperAbove70(theRolled, theRoll, ProfileLevel(z), theAlmanac, theOption);
	if (f != theRolled)
		LogZombieEvent("t=%d gaps newspaper-above70 NEWSPAPER#%d roll=%d variant=0x%x sub=%d", BoardTime(z), Id(z), theRoll,
			f.variant, f.subVariant);
	return f;
}

void NewspaperPaperHp(Zombie* z) {
	if (!EliteGapsOn() || z->mZombieType != ZombieType::ZOMBIE_NEWSPAPER || z->mBoard == nullptr)
		return;
	z->mShieldHealth = u::NewspaperAbove70PaperHp(ProfileLevel(z), static_cast<int>(z->mBoard->mEffectCounter), z->mShieldHealth);
}

bool NewspaperRollsInIZombie() { return EliteGapsOn() && u::NewspaperRollsInIZombie(); }

// ---- football helmet, I, Zombie Normal ----

void FootballHelmCheck(Zombie* z, int theOption) {
	if (!EliteGapsOn())
		return;
	const bool iz = z->mApp->IsIZombieLevel();
	if (!u::TakesFootballHelmCheck(z->mZombieType, iz, theOption))
		return;
	const int n = u::FootballHelmRollRange(iz);
	const int roll = n > 0 ? Rand(n) : -1;
	Board* b = z->mBoard;
	if (!u::GetsFootballHelm(iz, roll, b ? b->mLevel : 0, b ? b->mCurrentWave : 0))
		return;
	z->mHelmType = HelmType::HELMTYPE_FOOTBALL; // 0xc109e1: helm type 3, 2800, show anim_football
	z->mHelmHealth = u::FootballHelmHp(z->mHelmHealth);
	Reanimation* aBody = z->mApp->ReanimationTryToGet(z->mBodyReanimID);
	const bool aShown = aBody && aBody->TrackExists(kHelmTrack);
	if (aShown)
		aBody->AssignRenderGroupToTrack(kHelmTrack, RENDER_GROUP_NORMAL);
	LogZombieEvent("t=%d gaps football-helm type=%d#%d izombie=%d helm=%d track=%d", BoardTime(z), static_cast<int>(z->mZombieType),
		Id(z), iz ? 1 : 0, z->mHelmHealth, aShown ? 1 : 0);
}

int IZombieNormalVariant(Zombie* z, int theOption) {
	if (!EliteGapsOn() || z->mZombieType != ZombieType::ZOMBIE_NORMAL || !z->mApp->IsIZombieLevel())
		return -1;
	if (theOption == u::eliteoption::NONE) // 0xc111b8
		return u::elite::NONE;
	const int n = u::IZombieNormalCoinRollRange(z->mHelmType == HelmType::HELMTYPE_FOOTBALL);
	if (n <= 0)
		return -1;
	return u::IZombieNormalIsCoin(Rand(n)) ? u::elite::SILVER_COIN : u::elite::NONE;
}

const char* FootballHelmTrack(Zombie* z) {
	if (EliteGapsOn() && z->mZombieType != ZombieType::ZOMBIE_FOOTBALL && z->mHelmType == HelmType::HELMTYPE_FOOTBALL)
		return kHelmTrack;
	return "zombie_football_helmet";
}

void HideFootballHelmTrack(Reanimation* theReanim) {
	if (EliteGapsOn() && theReanim && theReanim->TrackExists(kHelmTrack))
		theReanim->AssignRenderGroupToTrack(kHelmTrack, RENDER_GROUP_HIDDEN);
}

bool MergesPlainZombieReanim() { return EliteGapsOn() && u::HasRule("eliteGaps.helm.hp"); }

} // namespace pvzu::engine
