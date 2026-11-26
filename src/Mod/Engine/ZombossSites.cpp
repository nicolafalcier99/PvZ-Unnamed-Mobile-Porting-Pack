// PVZU: engine side of the Unnamed Dr. Zomboss (default settings). Rules: Mod/Unnamed/Zomboss.h; evidence:
// the research notes (w5-roof) section 4.1 and pvz-unnamed-pack/SOURCES-zomboss.md.
#include "Mod/Engine/ZombossSites.h"

#include "Mod/CmdLine.h"
#include "Mod/Engine/EliteZombies.h"
#include "Mod/GameData.h"
#include "Mod/Unnamed/Zomboss.h"

#include "LawnApp.h"
#include "Resources.h"
#include "Lawn/Board.h"
#include "Lawn/Challenge.h"
#include "Lawn/GridItem.h"
#include "Lawn/Plant.h"
#include "Lawn/SeedPacket.h"
#include "Lawn/Zombie.h"
#include "PvzpLib/Reanimator.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

using namespace Sexy;
namespace u = pvzu::unnamed;

namespace pvzu::engine {

namespace {

int gBungeeScopeDepth = 0; // > 0 while BossBungeeSpawn adds its bungees (exe flag 0xC04476 == 1)

int Time(Zombie* z) { return z->mBoard ? static_cast<int>(z->mBoard->mMainCounter) : 0; }
int Id(Zombie* z) { return z->mBoard ? static_cast<int>(z->mBoard->ZombieGetID(z)) & 0xFFFF : -1; }

bool RoofZombiesOwnTheBungee() { return GameData::Get().Feature("roofZombies"); }

// Hypnotised zombies (or, for a hypnotised boss, the normal ones) the crush can hit: EffectedByDamage(0x8d / 0xd).
template <class InArea>
int CrushZombies(Zombie* theBoss, InArea theInArea, bool theDamage) {
	int n = 0;
	const unsigned aFlags = u::BossCrushDamageFlags(theBoss->mMindControlled);
	for (Zombie* z : theBoss->mBoard->mZombies) {
		if (z->mDead || z == theBoss)
			continue;
		const Rect r = z->GetZombieRect();
		if (!theInArea(z->mRow, r.mX, r.mWidth) || !z->EffectedByDamage(aFlags))
			continue;
		++n;
		if (theDamage)
			z->TakeDamage(u::BossCrushDamage(), 1U);
	}
	return n;
}

template <class InArea>
int OpenPots(Board* theBoard, InArea theInArea, bool theOpen) {
	int n = 0;
	for (GridItem* g : theBoard->mGridItems) {
		if (g->mDead || g->mGridItemType != GridItemType::GRIDITEM_SCARY_POT || !theInArea(g->mGridX, g->mGridY))
			continue;
		++n;
		if (theOpen)
			theBoard->mChallenge->ScaryPotterOpenPot(g);
	}
	return n;
}

// ---- dev options ----

struct DevAttack {
	std::string name;
	int tick = 0;
	bool done = false;
};

struct Dev {
	bool parsed = false;
	int hp = -1;
	int age = -1;
	bool yetiKilled = false;
	std::vector<DevAttack> attacks;
	int hypnoX = -1;
	int hypnoTick = -1;
	bool hypnoDone = false;
	bool setupDone = false;
	int ticks = 0;
};

Dev& D() {
	static Dev d;
	if (d.parsed)
		return d;
	d.parsed = true;
	d.hp = CmdOptionInt("-pvzu-dev-boss-hp", -1);
	d.age = CmdOptionInt("-pvzu-dev-boss-age", -1);
	d.yetiKilled = CmdOption("-pvzu-dev-boss-yeti-killed").has_value();
	std::string all = CmdOption("-pvzu-dev-boss-attack").value_or("");
	size_t start = 0;
	while (!all.empty() && start <= all.size()) {
		const size_t comma = all.find(',', start);
		const std::string item = all.substr(start, comma == std::string::npos ? std::string::npos : comma - start);
		const size_t at = item.find('@');
		DevAttack a;
		a.name = item.substr(0, at);
		a.tick = at == std::string::npos ? 1 : std::atoi(item.c_str() + at + 1);
		if (a.name == "stomp" || a.name == "bungee" || a.name == "summon" || a.name == "head" || a.name == "rv")
			d.attacks.push_back(a);
		else
			std::fprintf(stderr, "PVZU: -pvzu-dev-boss-attack: cannot read '%s' (want stomp|bungee|summon|head|rv@tick)\n",
				item.c_str());
		if (comma == std::string::npos)
			break;
		start = comma + 1;
	}
	if (auto h = CmdOption("-pvzu-dev-boss-hypno")) {
		const size_t at = h->find('@');
		d.hypnoX = std::atoi(h->c_str());
		d.hypnoTick = at == std::string::npos ? 1 : std::atoi(h->c_str() + at + 1);
	}
	return d;
}

} // namespace

bool ZombossOn() { return GameData::Get().Feature("zomboss"); }

int ZombossStartHp(Zombie* theBoss, int theVanilla) {
	if (!ZombossOn())
		return theVanilla;
	const int hp = u::BossStartHp(theBoss->mApp->IsAdventureMode(), theVanilla);
	LogZombieEvent("t=%d boss start hp=%d (vanilla %d) adventure=%d", Time(theBoss), hp, theVanilla,
		theBoss->mApp->IsAdventureMode() ? 1 : 0);
	return hp;
}

void ZombossDevTick(Zombie* theBoss) {
	Dev& d = D();
	if (d.hp < 0 && d.age < 0 && !d.yetiKilled && d.attacks.empty() && d.hypnoX < 0)
		return;
	if (theBoss->mApp->mGameScene != GameScenes::SCENE_PLAYING)
		return;
	Board* aBoard = theBoss->mBoard;
	++d.ticks;
	if (!d.setupDone) {
		d.setupDone = true;
		if (d.hp > 0)
			theBoss->mBodyHealth = d.hp;
		if (d.age >= 0)
			theBoss->mZombieAge = d.age;
		if (d.yetiKilled)
			aBoard->mKilledYeti = true;
		LogZombieEvent("t=%d boss dev hp=%d max=%d age=%d killedYeti=%d", Time(theBoss), theBoss->mBodyHealth,
			theBoss->mBodyMaxHealth, theBoss->mZombieAge, aBoard->mKilledYeti ? 1 : 0);
	}
	if (d.hypnoX >= 0 && !d.hypnoDone && d.ticks >= d.hypnoTick) {
		d.hypnoDone = true;
		for (int row = 0; row < 5; ++row)
			if (Zombie* z = aBoard->AddZombieInRow(ZombieType::ZOMBIE_NORMAL, row, std::max(0, aBoard->mCurrentWave))) {
				z->mPosX = static_cast<float>(d.hypnoX);
				z->mX = d.hypnoX;
				z->StartMindControlled();
				LogZombieEvent("t=%d boss dev hypnotised NORMAL#%d row=%d x=%d hp=%d", Time(theBoss), Id(z), row, d.hypnoX,
					z->mBodyHealth);
			}
	}
	if (theBoss->mZombiePhase != ZombiePhase::PHASE_BOSS_IDLE)
		return;
	for (DevAttack& a : d.attacks) {
		if (a.done || d.ticks < a.tick)
			continue;
		a.done = true;
		LogZombieEvent("t=%d boss dev attack %s", Time(theBoss), a.name.c_str());
		theBoss->mPhaseCounter = 0;
		if (a.name == "stomp") theBoss->mBossStompCounter = 0;
		else if (a.name == "bungee") theBoss->mBossBungeeCounter = 0;
		else if (a.name == "summon") theBoss->mSummonCounter = 0;
		else if (a.name == "head") theBoss->mBossHeadCounter = 0;
		else if (a.name == "rv") theBoss->BossRVAttack();
		break; // one attack per idle
	}
}

std::optional<ZombieType> ZombossLateSpawnType(Zombie* theBoss) {
	if (!ZombossOn() || u::BossLateSpawnRollRange() <= 0)
		return std::nullopt;
	const bool aYeti = theBoss->mBoard->mKilledYeti;
	const ZombieType t = u::PickBossLateSpawn(theBoss->mTargetRow, aYeti, [](int n) { return Rand(n); });
	LogZombieEvent("t=%d boss late-spawn type=%d row=%d age=%d killedYeti=%d", Time(theBoss), static_cast<int>(t),
		theBoss->mTargetRow, theBoss->mZombieAge, aYeti ? 1 : 0);
	return t;
}

void ZombossSpawned(Zombie* theBoss, Zombie* theZombie) {
	if (!ZombossOn() || theZombie == nullptr)
		return;
	if (u::BossCopiesHypnosis())
		theZombie->mMindControlled = theBoss->mMindControlled;
	LogZombieEvent("t=%d boss spawned type=%d #%d row=%d x=%.0f variant=0x%x hypno=%d bossAge=%d", Time(theBoss),
		static_cast<int>(theZombie->mZombieType), Id(theZombie), theZombie->mRow, theZombie->mPosX, theZombie->mPvzu.variant,
		theZombie->mMindControlled ? 1 : 0, theBoss->mZombieAge);
}

void ZombossEmptyStomp(Zombie* theBoss) {
	if (!ZombossOn() || !u::BossEmptyStompHeals())
		return;
	const int before = theBoss->mBodyHealth;
	theBoss->mBodyHealth = u::BossHpAfterEmptyStomp(before);
	LogZombieEvent("t=%d boss empty-stomp heal hp=%d -> %d max=%d", Time(theBoss), before, theBoss->mBodyHealth,
		theBoss->mBodyMaxHealth);
}

std::optional<bool> ZombossCanStompRow(Zombie* theBoss, int theRow) { // hook 738, 0x957e50 -> 0x959b80 -> 0x957ed6
	if (!ZombossOn())
		return std::nullopt;
	if (u::BossCrushesHypnotised() &&
		CrushZombies(theBoss, [&](int row, int x, int w) { return u::StompHitsZombie(row, x, w, theRow); }, false) > 0)
		return true;
	if (u::BossCrushOpensPots() &&
		OpenPots(theBoss->mBoard, [&](int gx, int gy) { return u::StompOpensPot(gx, gy, theRow); }, false) > 0)
		return true;
	if (theBoss->mMindControlled)
		return false; // a hypnotised boss never stomps plants
	return std::nullopt;
}

bool ZombossStompContact(Zombie* theBoss) { // hook 739, 0x957ef0 -> 0x959be0 -> 0x957f7d
	if (!ZombossOn())
		return false;
	const int aRow = theBoss->mTargetRow;
	int hit = 0, pots = 0;
	if (u::BossCrushesHypnotised())
		hit = CrushZombies(theBoss, [&](int row, int x, int w) { return u::StompHitsZombie(row, x, w, aRow); }, true);
	if (u::BossCrushOpensPots())
		pots = OpenPots(theBoss->mBoard, [&](int gx, int gy) { return u::StompOpensPot(gx, gy, aRow); }, true);
	LogZombieEvent("t=%d boss stomp row=%d zombiesHit=%d potsOpened=%d hp=%d", Time(theBoss), aRow, hit, pots,
		theBoss->mBodyHealth);
	return theBoss->mMindControlled;
}

bool ZombossRVLanding(Zombie* theBoss) { // hook 731 0xc02b20 -> 0x957bf0 -> 0x959c50 -> 0x957c89; hook 732
	if (!ZombossOn())
		return false;
	const int aRow = theBoss->mTargetRow, aCol = theBoss->mTargetCol;
	for (Plant* p : theBoss->mBoard->mPlants) { // the first Umbrella Leaf near the landing square takes the RV
		if (p->mDead || p->mSquished || p->mSeedType != SeedType::SEED_UMBRELLA || p->mPvzu.modSeed != 0 /* raw 0x25: not Leaf Shield */ ||
			p->mOnBungeeState == PlantOnBungeeState::RISING_WITH_BUNGEE || !u::UmbrellaBlocksRv(p->mRow, p->mPlantCol, aRow, aCol))
			continue;
		p->mPlantHealth -= u::UmbrellaRvDamage();
		p->DoSpecial();
		LogZombieEvent("t=%d boss rv row=%d col=%d blocked by umbrella col=%d row=%d hp=%d", Time(theBoss), aRow, aCol,
			p->mPlantCol, p->mRow, p->mPlantHealth);
		return true;
	}
	int hit = 0, pots = 0;
	if (u::BossCrushesHypnotised())
		hit = CrushZombies(theBoss, [&](int row, int x, int w) { return u::RvHitsZombie(row, x, w, aRow, aCol); }, true);
	if (u::BossCrushOpensPots())
		pots = OpenPots(theBoss->mBoard, [&](int gx, int gy) { return u::RvOpensPot(gx, gy, aRow, aCol); }, true);
	LogZombieEvent("t=%d boss rv row=%d col=%d zombiesHit=%d potsOpened=%d", Time(theBoss), aRow, aCol, hit, pots);
	return theBoss->mMindControlled;
}

bool ZombossPickBall(Zombie* theBoss) { // hook 745 0x957a50
	if (!ZombossOn())
		return false;
	std::vector<u::BossPacket> aBank;
	if (SeedBank* b = theBoss->mBoard->mSeedBank.get())
		for (int i = 0; i < b->mNumPackets; ++i)
			aBank.push_back({static_cast<int>(b->mSeedPackets[i].mPacketType), static_cast<int>(b->mSeedPackets[i].mImitaterType)});
	const u::BossBall aBall = u::PickBossBall(aBank);
	int aJalapenos = 0, aIceshrooms = 0; // for the log line only
	for (const u::BossPacket& p : aBank) {
		const int t = p.type == SeedType::SEED_IMITATER ? p.imitaterType : p.type;
		aJalapenos += t == SeedType::SEED_JALAPENO ? 1 : 0;
		aIceshrooms += t == SeedType::SEED_ICESHROOM ? 1 : 0;
	}
	if (aBall != u::BossBall::VANILLA_ROLL)
		theBoss->mIsFireBall = aBall == u::BossBall::FIRE;
	LogZombieEvent("t=%d boss spit row=%d pick=%s packets=%d jalapenos=%d iceshrooms=%d", Time(theBoss), theBoss->mFireballRow,
		aBall == u::BossBall::VANILLA_ROLL ? "roll" : theBoss->mIsFireBall ? "fire" : "ice", static_cast<int>(aBank.size()),
		aJalapenos, aIceshrooms);
	return aBall != u::BossBall::VANILLA_ROLL;
}

ZombossBungeeScope::ZombossBungeeScope() { ++gBungeeScopeDepth; }
ZombossBungeeScope::~ZombossBungeeScope() { --gBungeeScopeDepth; }

bool ZombossBungeeSpawning() { return gBungeeScopeDepth > 0 && ZombossOn() && u::BossBungeeJackForced(); }

void ZombossBungeeSpawned(Zombie* theBoss, Zombie* theBungee) { // hooks 741 (0x957dc5), 743 (0x957d70)
	if (!ZombossOn() || theBungee == nullptr)
		return;
	if (u::BossCopiesHypnosis())
		theBungee->mMindControlled = theBoss->mMindControlled;
	theBungee->mFollowerZombieID[0] = theBoss->mBoard->ZombieGetID(theBoss); // exe bungee+0xF4 = the boss's id
	// Without the Roof zombies' Bungee elite (feature roofZombies) the boss rolls its own: Rand(12) < 3 -> Jack head.
	if (!RoofZombiesOwnTheBungee() && theBungee->mPvzu.variant == 0 && u::BossBungeeEliteRollRange() > 0) {
		theBungee->mPvzu.variant = u::BossBungeeVariant(Rand(u::BossBungeeEliteRollRange()));
		if (theBungee->mPvzu.variant == u::zomboss::JACK_HEAD_BUNGEE)
			if (Reanimation* aBody = theBungee->mApp->ReanimationTryToGet(theBungee->mBodyReanimID))
				aBody->SetImageOverride("anim_head1", IMAGE_REANIM_ZOMBIE_JACKBOX_BOX);
	}
	LogZombieEvent("t=%d boss bungee #%d col=%d variant=0x%x hypno=%d", Time(theBoss), Id(theBungee), theBungee->mTargetCol,
		theBungee->mPvzu.variant, theBungee->mMindControlled ? 1 : 0);
}

int ZombossBungeeGrabCs(Zombie* theBungee, int theVanilla) {
	if (!ZombossOn() || RoofZombiesOwnTheBungee() || theBungee->mPvzu.variant != u::zomboss::JACK_HEAD_BUNGEE)
		return theVanilla;
	return u::JackBungeeGrabCs(theVanilla);
}

void ZombossBungeeLifted(Zombie* theBungee, Plant* thePlant) { // study 2.1 0x17 (hooks 527, 529 0x797051)
	if (!ZombossOn() || RoofZombiesOwnTheBungee() || thePlant == nullptr ||
		theBungee->mPvzu.variant != u::zomboss::JACK_HEAD_BUNGEE || !theBungee->IsOnBoard())
		return;
	Board* aBoard = theBungee->mBoard;
	Zombie* aJack = aBoard->AddZombieInRow(ZombieType::ZOMBIE_JACK_IN_THE_BOX, thePlant->mRow, theBungee->mFromWave);
	if (aJack == nullptr)
		return;
	aJack->mPosX = static_cast<float>(aBoard->GridToPixelX(thePlant->mPlantCol, thePlant->mRow) + u::JackBungeeJackDx());
	aJack->mX = static_cast<int>(aJack->mPosX);
	aJack->mPhaseCounter = 0; // pops at once
	LogZombieEvent("t=%d boss jack-bungee #%d lifted plant seed=%d col=%d row=%d -> JACK#%d x=%.0f variant=0x%x",
		Time(theBungee), Id(theBungee), static_cast<int>(thePlant->mSeedType), thePlant->mPlantCol, thePlant->mRow, Id(aJack),
		aJack->mPosX, aJack->mPvzu.variant);
}

bool ZombossBreakFireballs(Board* theBoard) { // hook 340 0x957810
	if (!ZombossOn() || !u::EveryBossBallsBreak())
		return false;
	for (Zombie* z : theBoard->mZombies)
		if (!z->mDead && z->mZombieType == ZombieType::ZOMBIE_BOSS && u::BossBallBreaks(z->mMindControlled)) {
			const bool had = z->mBossFireBallReanimID != ReanimationID::REANIMATIONID_NULL && z->mIsFireBall;
			z->BossDestroyFireball();
			if (had)
				LogZombieEvent("t=%d boss #%d fireball broken by ice", Time(z), Id(z));
		}
	return true;
}

bool ZombossBreakIceballs(Board* theBoard, int theRow) { // hook 341 0x9577c0
	if (!ZombossOn() || !u::EveryBossBallsBreak())
		return false;
	for (Zombie* z : theBoard->mZombies)
		if (!z->mDead && z->mZombieType == ZombieType::ZOMBIE_BOSS && u::BossBallBreaks(z->mMindControlled) &&
			z->mFireballRow == theRow) {
			const bool had = z->mBossFireBallReanimID != ReanimationID::REANIMATIONID_NULL && !z->mIsFireBall;
			z->BossDestroyIceballInRow();
			if (had)
				LogZombieEvent("t=%d boss #%d iceball broken by fire row=%d", Time(z), Id(z), theRow);
		}
	return true;
}

bool ZombossRefusesFreeze(Zombie* theZombie) { // hook 713 0xc10ae0
	return theZombie->mZombieType == ZombieType::ZOMBIE_BOSS && ZombossOn() && u::ZombossRefusesFreeze(theZombie->mBodyHealth);
}

} // namespace pvzu::engine
