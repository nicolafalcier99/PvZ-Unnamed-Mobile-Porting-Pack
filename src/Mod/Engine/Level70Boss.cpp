// PVZU: engine side of the Unnamed level 70 (7-10) giant Newspaper boss. Rules: Mod/Unnamed/Level70Boss.h; evidence:
// the research notes (w6-w7-and-new-content) section 3 and pvz-unnamed-pack/SOURCES-level70-boss.md (exe addresses).
#include "Mod/Engine/Level70Boss.h"
#include "Mod/Engine/ModPlants.h"

#include "Mod/CmdLine.h"
#include "Mod/Engine/DevLevels.h"
#include "Mod/GameData.h"
#include "Mod/Unnamed/Level70Boss.h"
#include "Mod/Unnamed/World7Zombies.h"

#include "LawnApp.h"
#include "Resources.h"
#include "Lawn/Board.h"
#include "Lawn/Challenge.h"
#include "Lawn/CursorObject.h"
#include "Lawn/GridItem.h"
#include "Lawn/MessageWidget.h"
#include "Lawn/Plant.h"
#include "Lawn/Projectile.h"
#include "Lawn/SeedPacket.h"
#include "Lawn/Zombie.h"
#include "Lawn/System/PlayerInfo.h"
#include "PvzpLib/Attachment.h"
#include "PvzpLib/PvzpCommon.h"
#include "PvzpLib/PvzpFoley.h"
#include "PvzpLib/PvzpStringFile.h"
#include "PvzpLib/Reanimator.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <format>
#include <string>
#include <utility>
#include <vector>
#include "Mod/Engine/ModModes.h"
#include "Mod/Engine/TestCheats.h" // PVZU Plan 9 C1: invincible plants

using namespace Sexy;
namespace u = pvzu::unnamed;
namespace b70 = pvzu::unnamed::boss70;

namespace pvzu::engine {

namespace {

int gPlacingBoss = 0; // > 0 while the boss itself is placed (exe 0x800258 = 1 around 0x42a0f0, 0xc11030)

int Time(Board* theBoard) { return theBoard ? static_cast<int>(theBoard->mMainCounter) : 0; }
int Id(Zombie* z) { return z->mBoard ? static_cast<int>(z->mBoard->ZombieGetID(z)) & 0xFFFF : -1; }
bool Mad(Zombie* z) { return z->mZombiePhase == ZombiePhase::PHASE_NEWSPAPER_MAD; }
bool BossClass(Zombie* z) { return u::IsBossClass(z->mBodyMaxHealth, static_cast<int>(z->mZombieType)); }
int ProfileLevel(LawnApp* theApp) { return theApp->mPlayerInfo ? theApp->mPlayerInfo->GetLevel() : 0; }

void Log(const std::string& theLine) { DevLog(theLine); }

std::string Text(u::Boss70Text theText) {
	const std::string aKey = std::format("[{}]", u::Boss70TextKey(theText));
	return PvzpStringListExists(aKey) ? std::string(PvzpStringTranslate(aKey)) : std::string(u::Boss70TextFallback(theText));
}

// MessageWidget::SetLabel(text, 4) then +0x88 (mDuration) = 250, as the exe does for each boss line.
void Say(Board* theBoard, u::Boss70Text theText) {
	theBoard->mAdvice->SetLabel(Text(theText), MessageStyle::MESSAGE_STYLE_TUTORIAL_LATER);
	theBoard->mAdvice->mDuration = u::Boss70TextCs();
	Log(std::format("boss70 t={} say {}", Time(theBoard), u::Boss70TextKey(theText)));
}

// Challenge::IZombiePlaceZombie, returning the zombie (the exe reads it back from 0x80010c).
Zombie* Place(Board* theBoard, ZombieType theType, int theCol, int theRow) {
	Zombie* z = theBoard->AddZombieInRow(theType, theRow, 0);
	if (z == nullptr)
		return nullptr;
	if (theType == ZombieType::ZOMBIE_BUNGEE) {
		z->mTargetCol = theCol;
		z->SetRow(theRow);
		z->mPosX = theBoard->GridToPixelX(theCol, theRow);
		z->mPosY = z->GetPosYBasedOnRow(theRow);
		z->mRenderOrder = Board::MakeRenderOrder(RENDER_LAYER_GRAVE_STONE, theRow, 7);
	} else {
		z->mPosX = theBoard->GridToPixelX(theCol, theRow) - 30.0f;
	}
	return z;
}

int PlantCount(Board* theBoard) { return static_cast<int>(theBoard->mPlants.mSize); }
int ZombieCount(Board* theBoard) { return static_cast<int>(theBoard->mZombies.mSize); }

// ---- dev options ----

struct DevHp {
	int hp = 0, age = 1;
	bool done = false;
};
struct Dev {
	bool parsed = false;
	int wave = -1, timer = -1, trace = 0;
	std::vector<DevHp> hp;
	std::vector<DevHp> hits; // hp = damage
	std::vector<int> attacks;
	size_t nextAttack = 0;
	int playTicks = 0;
};
Dev& D() {
	static Dev d;
	if (d.parsed)
		return d;
	d.parsed = true;
	d.wave = CmdOptionInt("-pvzu-dev-b70-wave", -1);
	d.timer = CmdOptionInt("-pvzu-dev-b70-timer", -1);
	d.trace = CmdOptionInt("-pvzu-dev-b70-trace", 0);
	auto aSplit = [](const std::string& s) {
		std::vector<std::string> out;
		size_t a = 0;
		while (a <= s.size()) {
			const size_t c = s.find(',', a);
			out.push_back(s.substr(a, c == std::string::npos ? std::string::npos : c - a));
			if (c == std::string::npos)
				break;
			a = c + 1;
		}
		return out;
	};
	if (auto v = CmdOption("-pvzu-dev-b70-hp"))
		for (const std::string& item : aSplit(*v)) {
			DevHp h;
			if (std::sscanf(item.c_str(), "%d@%d", &h.hp, &h.age) >= 1)
				d.hp.push_back(h);
		}
	if (auto v = CmdOption("-pvzu-dev-b70-hit"))
		for (const std::string& item : aSplit(*v)) {
			DevHp h;
			if (std::sscanf(item.c_str(), "%d@%d", &h.hp, &h.age) >= 1)
				d.hits.push_back(h);
		}
	if (auto v = CmdOption("-pvzu-dev-b70-attack"))
		for (const std::string& item : aSplit(*v))
			if (!item.empty())
				d.attacks.push_back(std::atoi(item.c_str()));
	return d;
}

// ---- darkness and light (0x7aa680, 0x7aa7a0, 0x7aa8a0) ----

void SwitchLight(Board* theBoard, u::Boss70Light theLight) {
	theBoard->mBackground = static_cast<BackgroundType>(u::Boss70Background(theLight));
	theBoard->LoadBackgroundImages();
	int aAsleep = 0;
	for (Plant* p : theBoard->mPlants) {
		if (p->mDead)
			continue;
		p->SetSleeping(u::Boss70PlantSleeps(theLight, Plant::IsNocturnal(p->mSeedType))); // skips squash jumps etc.
		aAsleep += p->mIsAsleep ? 1 : 0;
	}
	Log(std::format("boss70 t={} light={} background={} plants={} asleep={}", Time(theBoard),
		theLight == u::Boss70Light::DARK ? "dark" : "day", static_cast<int>(theBoard->mBackground), PlantCount(theBoard), aAsleep));
}

// The exe writes its own seeds 0x73 / 0x5d / 0x59 (Strong Coffee Bean, Prism-shroom, Golden Sun-shroom) into packets.
// While the modPlants feature is on they are real packets; with it off the pack's stand-ins (Coffee Bean,
// Scaredy-shroom, Sun-shroom) are used. theWide is the seed with mod seeds allowed, theNarrow its stand-in.
int PacketSeed(int theWide, int theNarrow) {
	const bool aUsable = theWide < static_cast<int>(SeedType::NUM_SEED_TYPES) || IsModSeed(static_cast<SeedType>(theWide));
	return aUsable ? theWide : theNarrow;
}
constexpr int kWideSeeds = 0x80; // 0x50..0x7f: the mod seed range
int WideSeeds() { return ModPlantsOn() ? kWideSeeds : static_cast<int>(SeedType::NUM_SEED_TYPES); }

// Rewrites packet i. A card the player holds from that packet goes back first (review fix: the exe does not, and
// planting it then calls WasPlanted on the new packet, SEED_NONE after the light's removals).
void SetPacketType(Board* theBoard, int i, SeedType theSeed) {
	SeedPacket& aPacket = theBoard->mSeedBank->mSeedPackets[i];
	CursorObject* aCursor = theBoard->mCursorObject.get();
	if (aPacket.mPacketType != theSeed && aCursor != nullptr && aCursor->mCursorType == CursorType::CURSOR_TYPE_PLANT_FROM_BANK &&
		aCursor->mSeedBankIndex == i) {
		Log(std::format("boss70 t={} packet {} changes {} -> {}: the held card goes back", Time(theBoard), i,
			static_cast<int>(aPacket.mPacketType), static_cast<int>(theSeed)));
		theBoard->ClearCursor();
	}
	aPacket.mPacketType = theSeed;
}

void SetBank(Board* theBoard, u::Boss70Light theLight) {
	SeedBank* aBank = theBoard->mSeedBank.get();
	if (aBank == nullptr)
		return;
	const std::vector<int> aWide = u::Boss70Bank(theLight, aBank->mNumPackets, WideSeeds());
	const std::vector<int> aNarrow = u::Boss70Bank(theLight, aBank->mNumPackets, static_cast<int>(SeedType::NUM_SEED_TYPES));
	std::string aList;
	for (size_t i = 0; i < aWide.size() && i < aNarrow.size(); ++i) {
		const int aSeed = PacketSeed(aWide[i], aNarrow[i]);
		SetPacketType(theBoard, static_cast<int>(i), static_cast<SeedType>(aSeed)); // +0x38 (imitater type) is left as it was
		aBank->mSeedPackets[i].mRefreshTime = 0; // 0x7aa762: +0x28 = 0, +0x34 = the type
		aList += std::format("{}{}", i ? "," : "", aSeed);
	}
	Log(std::format("boss70 t={} bank={} [{}]", Time(theBoard), theLight == u::Boss70Light::DARK ? "dark" : "day", aList));
}

void Reinforce(Board* theBoard) {
	const int n = PlantCount(theBoard);
	std::string aTypes;
	for (int i = 0; i < n; ++i) {
		const int t = u::PickBoss70Reinforcement([](int r) { return Rand(r); });
		const int aRow = Rand(u::Boss70ReinforcementRows());
		Place(theBoard, static_cast<ZombieType>(t), u::Boss70ReinforcementCol(), aRow);
		aTypes += std::format("{}{}@{}", i ? "," : "", t, aRow);
	}
	Log(std::format("boss70 t={} reinforcements={} [{}]", Time(theBoard), n, aTypes));
}

void Darkness(Board* theBoard) { // 0x7aa7a0
	SwitchLight(theBoard, u::Boss70Light::DARK);
	Reinforce(theBoard);
	SetBank(theBoard, u::Boss70Light::DARK);
	Say(theBoard, u::Boss70Text::DARK_HUNT);
}

void Daylight(Board* theBoard) { // 0x7aa8a0
	SwitchLight(theBoard, u::Boss70Light::DAY);
	SetBank(theBoard, u::Boss70Light::DAY);
	int aElites = 0;
	for (Zombie* z : theBoard->mZombies)
		aElites += u::Boss70CountsAsElite(z->mPvzu.variant, BossClass(z)) ? 1 : 0;
	SeedBank* aBank = theBoard->mSeedBank.get();
	int aRemove = aBank ? u::Boss70LightRemovals(aElites, aBank->mNumPackets) : 0;
	int aNonEmpty = 0;
	if (aBank)
		for (int i = 0; i < aBank->mNumPackets; ++i)
			aNonEmpty += aBank->mSeedPackets[i].mPacketType != SeedType::SEED_NONE ? 1 : 0;
	aRemove = std::min(aRemove, aNonEmpty); // the exe re-rolls until it finds a full packet; never loop forever
	for (; aRemove > 0; --aRemove) {
		int i;
		do
			i = Rand(aBank->mNumPackets);
		while (aBank->mSeedPackets[i].mPacketType == SeedType::SEED_NONE);
		SetPacketType(theBoard, i, SeedType::SEED_NONE);
		Log(std::format("boss70 t={} light empties packet {}", Time(theBoard), i));
	}
	Log(std::format("boss70 t={} light elites={}", Time(theBoard), aElites));
	Say(theBoard, u::Boss70Text::LIGHT);
}

void MadPackets(Board* theBoard) { // 0x7ab21f: packets 0 and 1 (if not empty) become exe seed 0x59
	SeedBank* aBank = theBoard->mSeedBank.get();
	const int aSeed = PacketSeed(u::Boss70MadPacketSeed(WideSeeds()), u::Boss70MadPacketSeed(static_cast<int>(SeedType::NUM_SEED_TYPES)));
	if (aBank == nullptr || aSeed < 0)
		return;
	for (int i = 0; i < 2 && i < aBank->mNumPackets; ++i)
		if (aBank->mSeedPackets[i].mPacketType != SeedType::SEED_NONE)
			SetPacketType(theBoard, i, static_cast<SeedType>(aSeed));
}

// ---- attacks ----

void Drain(Zombie* theBoss) { // 0x7aa9d0 + 0x7a8821
	Board* aBoard = theBoss->mBoard;
	const double f = u::Boss70DrainFactor(PlantCount(aBoard));
	int aTotal = 0;
	if (PlantCount(aBoard) > 0)
		for (Plant* p : aBoard->mPlants) {
			if (p->mDead || p->mSquished ||
				(p->mSeedType == SeedType::SEED_SQUASH && (p->mState == PlantState::STATE_SQUASH_FALLING || p->mState == PlantState::STATE_SQUASH_DONE_FALLING)))
				continue;
			p->mEatenFlashCountdown = std::max(p->mEatenFlashCountdown, u::Boss70DrainFlashCs());
			const int aLoss = u::Boss70DrainHp(p->mPlantMaxHealth, f);
			aTotal += aLoss;
			p->mPlantHealth -= aLoss;
		}
	const int aHeal = u::Boss70DrainHeal(aTotal);
	theBoss->mBodyHealth = std::min(theBoss->mBodyHealth + aHeal, theBoss->mBodyMaxHealth);
	Log(std::format("boss70 t={} drain plants={} factor={:.4f} total={} heal={} hp={}", Time(aBoard), PlantCount(aBoard), f, aTotal,
		aHeal, theBoss->mBodyHealth));
}

int Eat(Zombie* theBoss) { // 0x7aac50 (rows 1, reach 80)
	Board* aBoard = theBoss->mBoard;
	int aEaten = 0;
	for (Zombie* z : aBoard->mZombies) {
		if (z == theBoss || z->mDead || BossClass(z))
			continue;
		if (!u::Boss70EatReaches(theBoss->mRow, theBoss->mX, z->mRow, z->mX, z->mZombieRect.mWidth))
			continue;
		++aEaten;
		const int aSum = z->mBodyHealth + z->mHelmHealth + z->mShieldHealth + z->mFlyingHealth;
		theBoss->mBodyHealth = std::min(theBoss->mBodyHealth + u::Boss70EatHeal(aSum), theBoss->mBodyMaxHealth);
		z->TakeDamage(aSum, 0U);
		aBoard->mApp->AddPvzpParticle(z->mPosX + 40, z->mPosY + 40, 400000, ParticleEffect::PARTICLE_MOWER_CLOUD);
	}
	theBoss->mBodyHealth = std::min(theBoss->mBodyHealth, theBoss->mBodyMaxHealth);
	aBoard->mApp->PlayFoley(FoleyType::FOLEY_ART_CHALLENGE);
	aBoard->mApp->AddPvzpParticle(theBoss->mX + theBoss->mZombieRect.mWidth / 2, theBoss->mY + theBoss->mZombieRect.mHeight / 2, 400000,
		ParticleEffect::PARTICLE_PRESENT_PICKUP);
	if (aEaten > 0) { // the board-wide freeze of the Fog ice-head Jack (exe 0x800100 / 0x800104)
		aBoard->mPvzuState.iceFreezeActive = 1;
		aBoard->mPvzuState.iceFreezeCounter += u::Boss70EatFreeze(PlantCount(aBoard), aEaten);
	}
	Log(std::format("boss70 t={} eat zombies={} hp={} freeze={}", Time(aBoard), aEaten, theBoss->mBodyHealth,
		aBoard->mPvzuState.iceFreezeCounter));
	return aEaten;
}

void Laser(Board* theBoard, int theRow) { // 0x7aadf0 + 0x7aa000
	for (int aCol = 9; aCol >= 0; --aCol)
		theBoard->mApp->AddPvzpParticle(aCol * 80.0f + 64.0f, theRow * 100.0f + 80.0f, 400000, ParticleEffect::PARTICLE_ZOMBIE_BOSS_FIREBALL);
	int aKilled = 0;
	for (Plant* p : theBoard->mPlants) {
		if (p->mDead || p->mRow != theRow || p->NotOnGround() || TestCheatPlantSurvives(p)) // PVZU Plan 9 C1: + test cheat
			continue;
		theBoard->mPlantsEaten++; // Board+0x5798
		p->Die();
		++aKilled;
	}
	Say(theBoard, u::Boss70Text::LASER);
	theBoard->mApp->PlayFoley(FoleyType::FOLEY_IGNITE);
	Log(std::format("boss70 t={} laser row={} plants={}", Time(theBoard), theRow, aKilled));
}

void Sky(Zombie* theBoss) { // mode 3
	Board* aBoard = theBoss->mBoard;
	aBoard->SpawnZombiesFromSky();
	aBoard->SpawnZombiesFromSky();
	const bool aBungees = ZombieCount(aBoard) < PlantCount(aBoard);
	if (aBungees)
		for (const auto& [aRow, aCol] : u::Boss70BungeeCells())
			Place(aBoard, ZombieType::ZOMBIE_BUNGEE, aCol, aRow);
	int aGraves = 0;
	if (Mad(theBoss)) { // 0x7aa1d0(7) + 0x412ce0
		const int aRows = aBoard->StageHas6Rows() ? 6 : 5;
		for (int aRow = aRows - 1; aRow >= 0; --aRow)
			for (int aCol = u::Boss70GraveFirstCol(); aCol <= u::Boss70GraveLastCol(); ++aCol)
				if (aBoard->GetGridItemAt(GridItemType::GRIDITEM_GRAVESTONE, aCol, aRow) == nullptr) {
					aBoard->mChallenge->GraveDangerSpawnGraveAt(aCol, aRow);
					++aGraves;
				}
		aBoard->SpawnZombiesFromGraves();
	}
	Log(std::format("boss70 t={} sky bungees={} graves={} zombies={}", Time(aBoard), aBungees ? 1 : 0, aGraves, ZombieCount(aBoard)));
}

void Frenzy(Board* theBoard) { // 0x7aa260 + 0x794707
	int n = 0;
	for (Zombie* z : theBoard->mZombies) {
		z->mChilledCounter = 0;
		z->mButteredCounter = 0;
		z->mIceTrapCounter = 0;
		z->mMindControlled = false;
		z->mBlowingAway = false;
		z->mYuckyFace = false;
		if (BossClass(z))
			continue;
		if (Reanimation* r = theBoard->mApp->ReanimationTryToGet(z->mBodyReanimID)) {
			r->mAnimRate += r->mAnimRate;
			++n;
		}
	}
	Log(std::format("boss70 t={} frenzy zombies={}", Time(theBoard), n));
}

void Column(Zombie* theBoss) { // 0x7ab13b + 0x7aab60
	Board* aBoard = theBoss->mBoard;
	const int aRoll = Rand(std::max(1, u::Boss70ColumnRollRange()));
	const ZombieType aType = static_cast<ZombieType>(u::Boss70ColumnType(aRoll));
	for (int aRow = 4; aRow >= 0; --aRow) {
		Zombie* z = Place(aBoard, aType, 8, aRow);
		if (z == nullptr)
			continue;
		z->mScaleZombie = u::Boss70ColumnScale();
		z->mZombieFade = u::Boss70ColumnFadeCs();
		const int f = u::Boss70ColumnHpFactor();
		z->mBodyHealth = z->mBodyMaxHealth = z->mBodyMaxHealth * f;
		z->mHelmHealth = z->mHelmMaxHealth = z->mHelmMaxHealth * f;
		z->mShieldHealth = z->mShieldMaxHealth = z->mShieldMaxHealth * f;
		if (aType == ZombieType::ZOMBIE_DOOR)
			if (Reanimation* r = aBoard->mApp->ReanimationTryToGet(z->mBodyReanimID)) {
				z->mHelmType = HelmType::HELMTYPE_PAIL;
				z->mHelmHealth = z->mHelmMaxHealth = u::Boss70ColumnDoorBucketHp();
				r->AssignRenderGroupToPrefix("anim_bucket", RENDER_GROUP_NORMAL);
			}
	}
	Log(std::format("boss70 t={} column type={}", Time(aBoard), static_cast<int>(aType)));
}

// The Gatling head the mad boss holds where its paper was (hook 650, 0x7aa560): REANIM_GATLINGPEA playing anim_head_idle
// at 15 fps, attached to "Zombie_paper_paper" whose own image is blanked, offset (54, 36), turned 0.2 rad, mirrored.
void AttachHead(Zombie* theBoss) {
	if (!Mad(theBoss) || theBoss->mSpecialHeadReanimID != ReanimationID::REANIMATIONID_NULL)
		return;
	Reanimation* aBody = theBoss->mApp->ReanimationTryToGet(theBoss->mBodyReanimID);
	if (aBody == nullptr)
		return;
	ReanimatorTrackInstance* aTrack = aBody->GetTrackInstanceByName("Zombie_paper_paper");
	if (aTrack == nullptr)
		return;
	aTrack->mImageOverride = IMAGE_BLANK;
	Reanimation* aHead = theBoss->mApp->AddReanimation(0.0f, 0.0f, 0, ReanimationType::REANIM_GATLINGPEA);
	aHead->PlayReanim("anim_head_idle", ReanimLoopType::REANIM_LOOP, 0, 15.0f);
	theBoss->mSpecialHeadReanimID = theBoss->mApp->ReanimationGetID(aHead);
	AttachEffect* aEffect = AttachReanim(aTrack->mAttachmentID, aHead, 0.0f, 0.0f);
	if (aEffect)
		PvzpScaleRotateTransformMatrix(aEffect->mOffset, 54.0f, 36.0f, 0.2f, -1.0f, 1.0f);
	Log(std::format("boss70 t={} boss#{} holds a Gatling head", Time(theBoss->mBoard), Id(theBoss)));
}

void PlayHead(Zombie* theBoss, const char* theAnim) {
	if (Reanimation* aHead = theBoss->mApp->ReanimationTryToGet(theBoss->mSpecialHeadReanimID))
		aHead->PlayReanim(theAnim, ReanimLoopType::REANIM_LOOP, 0, static_cast<float>(u::Boss70HeadAnimRate()));
}

void RayGunStep(Zombie* theBoss) { // 0x7ab085..0x7ab11c, mode 4
	const int t = theBoss->mPvzu.counter;
	if (!u::Boss70RayGunFires(t))
		return;
	const float aAngle = u::Boss70RayGunAngle(t);
	Projectile* p = theBoss->mBoard->AddProjectile(theBoss->mX + u::Boss70RayGunDx(), theBoss->mY + u::Boss70RayGunDy(),
		theBoss->mRenderOrder - 1, theBoss->mRow, static_cast<ProjectileType>(u::Boss70RayGunProjectile()));
	if (p == nullptr)
		return;
	p->mMotionType = ProjectileMotion::MOTION_STAR;
	p->mVelX = u::Boss70RayGunSpeedX(); // fptan: 1.0 pushed, fchs -> -1, then tan(angle)
	p->mVelY = std::tan(aAngle);
	p->mDamageRangeFlags = 0;
	Log(std::format("boss70 t={} ray-gun melon timer={} angle={:.3f} vy={:.3f} at {},{} row={}", Time(theBoss->mBoard), t, aAngle,
		p->mVelY, theBoss->mX + u::Boss70RayGunDx(), theBoss->mY + u::Boss70RayGunDy(), theBoss->mRow));
}

void BossTick(Zombie* theBoss) { // 0x7ab000
	Board* aBoard = theBoss->mBoard;
	Dev& d = D();
	AttachHead(theBoss);
	if (!u::Boss70TickRuns(theBoss->mZombieFade >= 0, theBoss->mButteredCounter > 0, theBoss->mIceTrapCounter > 0,
			theBoss->mMindControlled, theBoss->mHasHead, theBoss->mDead, static_cast<int>(theBoss->mZombiePhase)))
		return;
	if (theBoss->mPvzu.extra == static_cast<int>(u::Boss70Attack::RAY_GUN))
		RayGunStep(theBoss);
	const int t = theBoss->mPvzu.counter - 1;
	if (t >= 0) {
		theBoss->mPvzu.counter = t;
		return;
	}
	const bool aMad = Mad(theBoss);
	if (aMad)
		Column(theBoss);
	const char* aHeadAnim = "anim_head_idle";
	int aAttack = Rand(std::max(1, u::Boss70AttackRollRange(aMad)));
	if (d.nextAttack < d.attacks.size())
		aAttack = d.attacks[d.nextAttack++];
	theBoss->mPvzu.extra = aAttack;
	Log(std::format("boss70 t={} boss#{} attack={} ({}) mad={} hp={}/{} row={}", Time(aBoard), Id(theBoss), aAttack,
		u::Boss70AttackName(aAttack), aMad ? 1 : 0, theBoss->mBodyHealth, theBoss->mBodyMaxHealth, theBoss->mRow));
	switch (static_cast<u::Boss70Attack>(aAttack)) {
	case u::Boss70Attack::DARK:
		Darkness(aBoard);
		if (aMad) {
			Drain(theBoss);
			MadPackets(aBoard);
		}
		break;
	case u::Boss70Attack::LIGHT:
		Daylight(aBoard);
		if (aMad && Eat(theBoss) > 0)
			MadPackets(aBoard);
		break;
	case u::Boss70Attack::LASER:
		Laser(aBoard, theBoss->mRow);
		if (aMad) {
			const int aRows = (aBoard->mBackground == BackgroundType::BACKGROUND_3_POOL || aBoard->mBackground == BackgroundType::BACKGROUND_4_FOG) ? 5 : 4;
			int aRow;
			do
				aRow = Rand(aRows);
			while (aRow == theBoss->mRow);
			Laser(aBoard, aRow);
		}
		break;
	case u::Boss70Attack::SKY:
		Sky(theBoss);
		break;
	case u::Boss70Attack::RAY_GUN:
		aHeadAnim = "anim_shooting";
		Say(aBoard, u::Boss70Text::RAY_GUN);
		break;
	case u::Boss70Attack::FRENZY:
		Frenzy(aBoard);
		break;
	case u::Boss70Attack::REST:
		break;
	}
	if (aMad)
		PlayHead(theBoss, aHeadAnim);
	int aRow;
	do
		aRow = Rand(std::max(2, u::Boss70RowRollRange()));
	while (aRow == theBoss->mRow);
	theBoss->mRow = aRow; // +0x1c only; the vanilla row drift moves mPosY there at 1 px/cs
	int aTimer = u::Boss70NextTimer(Rand(std::max(1, u::Boss70TimerRollRange())), aMad);
	if (d.timer > 0)
		aTimer = d.timer;
	theBoss->mPvzu.counter = aTimer;
}

void SpawnBoss(Board* theBoard) { // 0x7a07a0 + 0xc11030
	const u::Boss70Spawn s = u::Boss70SpawnRules();
	++gPlacingBoss;
	Zombie* z = Place(theBoard, ZombieType::ZOMBIE_NEWSPAPER, s.col, s.row);
	--gPlacingBoss;
	if (z) {
		z->mPvzu = u::ZombieState{};
		z->mPvzu.variant = b70::kVariant;
		z->mScaleZombie = s.scale;
		z->mShieldType = ShieldType::SHIELDTYPE_NONE;
		z->mShieldHealth = 0;
		z->mShieldMaxHealth = 0;
		z->mBodyHealth = z->mBodyMaxHealth = s.hp;
		z->mPvzu.extra = 0;                // +0x128 = 0
		z->mPvzu.counter = s.firstAttackCs; // +0x130 = 750
		if (D().timer > 0)
			z->mPvzu.counter = D().timer;
		const u::Boss70Rect r = u::Boss70HitRect({z->mZombieRect.mX, z->mZombieRect.mY, z->mZombieRect.mWidth, z->mZombieRect.mHeight});
		z->mZombieRect = Rect(r.x, r.y, r.w, r.h);
		Log(std::format("boss70 t={} spawn boss#{} row={} x={:.0f} hp={} scale={:.1f} rect={},{},{},{} wave={}", Time(theBoard), Id(z), z->mRow,
			z->mPosX, z->mBodyHealth, z->mScaleZombie, r.x, r.y, r.w, r.h, theBoard->mCurrentWave));
	}
	Darkness(theBoard); // 0x7a088c: even if the boss could not be placed
}

void TraceLine(Board* theBoard, Zombie* z) {
	int aAsleep = 0;
	for (Plant* p : theBoard->mPlants)
		aAsleep += !p->mDead && p->mIsAsleep ? 1 : 0;
	std::string aBank, aRefresh;
	if (SeedBank* b = theBoard->mSeedBank.get())
		for (int i = 0; i < b->mNumPackets; ++i) {
			aBank += std::format("{}{}", i ? "," : "", static_cast<int>(b->mSeedPackets[i].mPacketType));
			aRefresh += std::format("{}{}", i ? "," : "", b->mSeedPackets[i].mRefreshTime);
		}
	const CursorObject* aCursor = theBoard->mCursorObject.get();
	const std::string aHeld = std::format(" refresh=[{}] held={}:{}", aRefresh,
		aCursor && aCursor->mCursorType == CursorType::CURSOR_TYPE_PLANT_FROM_BANK ? aCursor->mSeedBankIndex : -1,
		aCursor ? static_cast<int>(aCursor->mType) : -1);
	Log(std::format("boss70 t={} f={} state boss#{} hp={}/{} phase={} shield={}/{} row={} x={:.0f} y={:.0f} scale={:.1f} attack={} timer={} "
		"bg={} sun={} plants={} asleep={} zombies={} meter={} head={} bank=[{}] v=0x{:x} rect={},{},{},{}",
		Time(theBoard), gSexyAppBase ? static_cast<int>(gSexyAppBase->mUpdateCount) : 0, Id(z), z->mBodyHealth, z->mBodyMaxHealth,
		static_cast<int>(z->mZombiePhase), static_cast<int>(z->mShieldType), z->mShieldHealth, z->mRow, z->mPosX, z->mPosY, z->mScaleZombie, z->mPvzu.extra, z->mPvzu.counter,
		static_cast<int>(theBoard->mBackground), theBoard->mSunMoney, PlantCount(theBoard), aAsleep, ZombieCount(theBoard),
		theBoard->mProgressMeterWidth, z->mSpecialHeadReanimID != ReanimationID::REANIMATIONID_NULL ? 1 : 0, aBank, z->mPvzu.variant,
		z->mZombieRect.mX, z->mZombieRect.mY, z->mZombieRect.mWidth, z->mZombieRect.mHeight) + aHeld);
}

} // namespace

bool Level70BossOn() { return GameData::Get().Feature("level70Boss"); }

// 0x7aa100: the boss code runs when `mode == 0x83 || (mBoard && mLevel == 70)`: mod mode 0x83 counts as level 70 here
// (hook 707's phases stay Adventure-only through IsWorld7, as in the exe)
int BossLevelOf(const Board* theBoard) { return ModModeIsSecondUncle(theBoard->mApp) ? 70 : theBoard->mLevel; }

bool IsLevel70Boss(Zombie* theZombie) {
	return Level70BossOn() && theZombie->mBoard &&
		u::IsBoss70(static_cast<int>(theZombie->mZombieType), theZombie->mPvzu.variant, BossLevelOf(theZombie->mBoard));
}

bool Level70BossPlacing() { return gPlacingBoss > 0; }

void Level70BossWave(Board* theBoard) {
	if (!Level70BossOn() || !u::Boss70Triggers(BossLevelOf(theBoard), theBoard->mCurrentWave))
		return;
	SpawnBoss(theBoard);
}

void Level70BossUpdate(Board* theBoard) {
	if (!Level70BossOn() || !u::Boss70IsLevel(BossLevelOf(theBoard)))
		return;
	Dev& d = D();
	++d.playTicks;
	if (d.playTicks == 1 && d.wave > 0) {
		theBoard->mCurrentWave = std::min(d.wave - 1, theBoard->mNumWaves - 1);
		theBoard->mZombieCountDown = 20;
		theBoard->mZombieCountDownStart = 20;
		Log(std::format("boss70 dev jump to wave {} (index {})", d.wave, theBoard->mCurrentWave));
	}
	for (Zombie* z : theBoard->mZombies) {
		if (z->mDead || !IsLevel70Boss(z))
			continue;
		for (DevHp& h : d.hp)
			if (!h.done && z->mZombieAge >= h.age) {
				h.done = true;
				z->mBodyHealth = std::min(h.hp, z->mBodyMaxHealth);
				Log(std::format("boss70 t={} dev hp boss#{} = {}", Time(theBoard), Id(z), z->mBodyHealth));
			}
		for (DevHp& h : d.hits)
			if (!h.done && z->mZombieAge >= h.age) {
				h.done = true;
				Log(std::format("boss70 t={} dev hit boss#{} for {} (hp={}/{} phase={})", Time(theBoard), Id(z), h.hp, z->mBodyHealth,
					z->mBodyMaxHealth, static_cast<int>(z->mZombiePhase)));
				z->TakeDamage(h.hp, 0U); // the real path: hook 707 phases, then the vanilla shield / body damage
			}
		if (!IsLevel70Boss(z)) // a dev hit can end the boss form (phase 3)
			continue;
		BossTick(z);
		if (d.trace > 0 && theBoard->mMainCounter % d.trace == 0)
			TraceLine(theBoard, z);
	}
}

void Level70BossBeforeDamage(Zombie* z) {
	if (!IsLevel70Boss(z) || !u::IsWorld7(z->mApp->IsAdventureMode(), ProfileLevel(z->mApp), z->mBoard->mLevel))
		return; // hook 707 runs its boss branch only inside FUN_007aa4c0 (Adventure, profile level 61..70)
	const u::Boss70Phase p = u::Boss70BeforeHit(static_cast<int>(z->mZombiePhase), z->mBodyHealth, z->mBodyMaxHealth);
	if (p.giveShield && z->mShieldType == ShieldType::SHIELDTYPE_NONE) {
		Log(std::format("boss70 t={} phase1 boss#{} hp={}/{}: a 1 HP paper", Time(z->mBoard), Id(z), z->mBodyHealth, z->mBodyMaxHealth));
	}
	if (p.giveShield) {
		z->mShieldType = ShieldType::SHIELDTYPE_NEWSPAPER; // +0xd8 = 2, +0xdc = 1 (max left as it is)
		z->mShieldHealth = p.shieldHp;
	}
	if (p.phase2MaxHp > 0) {
		z->mBodyMaxHealth = p.phase2MaxHp;
		z->mBoard->mSunMoney = p.phase2Sun; // a store, not AddSunMoney
		Say(z->mBoard, u::Boss70Text::PHASE2);
		Log(std::format("boss70 t={} phase2 boss#{} hp={} max={} sun={}", Time(z->mBoard), Id(z), z->mBodyHealth, z->mBodyMaxHealth,
			z->mBoard->mSunMoney));
	}
	if (p.phase3MaxHp > 0) {
		z->mBodyMaxHealth = p.phase3MaxHp;
		z->mPvzu.variant = 0; // the boss form ends: a plain (giant) mad Newspaper
		z->mPvzu.counter = 0;
		z->mPvzu.extra = 0;
		Say(z->mBoard, u::Boss70Text::PHASE3);
		Log(std::format("boss70 t={} phase3 boss#{} hp={} max={}", Time(z->mBoard), Id(z), z->mBodyHealth, z->mBodyMaxHealth));
	}
}

bool Level70BossSkipsDamageStates(Zombie* z) { return IsLevel70Boss(z); }

bool Level70BossPinsWalk(Zombie* z) {
	if (!IsLevel70Boss(z))
		return false;
	z->mPosX = u::Boss70PinX();
	return true;
}

bool Level70BossNoPrey(Zombie* z) {
	if (!IsLevel70Boss(z))
		return false;
	if (z->mIsEating)
		z->StopEating();
	return true;
}

bool Level70BossDrawsShield(Zombie* z) { return IsLevel70Boss(z) && u::Boss70DrawsShieldGroup(); }

bool Level70BossHandsOver(Zombie* z) { return IsLevel70Boss(z) && u::Boss70HandsOverPaper(static_cast<int>(z->mZombiePhase)); }

bool Level70BossButterTick(Zombie* z) {
	if (!Level70BossOn())
		return false;
	const u::Boss70Butter b = u::Boss70ButterTick(z->mButteredCounter, z->mBodyMaxHealth, static_cast<int>(z->mZombieType),
		static_cast<int>(z->mZombiePhase));
	if (!b.cut)
		return false;
	if (b.hypnotise)
		z->mMindControlled = true;
	z->mButteredCounter = 0;
	Log(std::format("boss70 t={} long butter cut on #{}{}", Time(z->mBoard), Id(z), b.hypnotise ? " (hypnotised)" : ""));
	return true;
}

bool Level70BossMowerSkips(Zombie* z) { return Level70BossOn() && BossClass(z); }

bool Level70BossProgressMeter(Board* theBoard) {
	if (!Level70BossOn() || !u::Boss70IsLevel(BossLevelOf(theBoard)))
		return false;
	Zombie* aBoss = nullptr;
	for (Zombie* z : theBoard->mZombies)
		if (IsLevel70Boss(z)) {
			aBoss = z;
			break;
		}
	const bool aAlive = aBoss && !aBoss->mDead && aBoss->mZombiePhase != ZombiePhase::PHASE_ZOMBIE_DYING &&
		aBoss->mZombiePhase != ZombiePhase::PHASE_ZOMBIE_BURNED && aBoss->mZombiePhase != ZombiePhase::PHASE_ZOMBIE_MOWERED;
	const int w = u::Boss70ProgressWidth(aBoss != nullptr, aAlive, aBoss ? aBoss->mBodyHealth : 0, aBoss ? aBoss->mBodyMaxHealth : 0,
		theBoard->mCurrentWave);
	if (w < 0)
		return false;
	theBoard->mProgressMeterWidth = w;
	return true;
}

void Level70BossBankPlant(Board* theBoard, SeedType& theSeed, SeedType& theImitater) {
	if (!Level70BossOn() || !u::Boss70SproutRandom(static_cast<int>(theSeed), static_cast<int>(theImitater)))
		return;
	const SeedType aFrom = theSeed;
	theImitater = static_cast<SeedType>(Rand(u::Boss70SproutRollRange()));
	theSeed = static_cast<SeedType>(u::Boss70SproutSeed());
	Log(std::format("boss70 t={} sprout card ({}) plants an Imitater of {}", Time(theBoard), static_cast<int>(aFrom),
		static_cast<int>(theImitater)));
}

bool Level70BossShotCollision(Projectile* p) {
	if (!Level70BossOn() || p->mDamageRangeFlags != 0 || p->mMotionType != ProjectileMotion::MOTION_STAR ||
		p->mProjectileType != static_cast<ProjectileType>(u::Boss70RayGunProjectile()) || p->mBoard == nullptr ||
		!u::Boss70IsLevel(BossLevelOf(p->mBoard)))
		return false;
	if (Plant* aPlant = p->FindCollisionTargetPlant()) { // 0x46cfcf: the zombie-pea plant hit
		aPlant->mPlantHealth -= p->GetProjectileDef().mDamage;
		aPlant->mEatenFlashCountdown = std::max(aPlant->mEatenFlashCountdown, 25);
		p->mApp->PlayFoley(FoleyType::FOLEY_SPLAT);
		p->mApp->AddPvzpParticle(p->mPosX - 3.0f, p->mPosY + 17.0f, p->mRenderOrder + 1, ParticleEffect::PARTICLE_PEA_SPLAT);
		Log(std::format("boss70 t={} ray-gun melon hits plant {} col={} row={} hp={}", Time(p->mBoard), static_cast<int>(aPlant->mSeedType),
			aPlant->mPlantCol, aPlant->mRow, aPlant->mPlantHealth));
		p->Die();
	}
	return true; // flags 0: no zombie can be hit (EffectedByDamage(0) is false for every zombie)
}

} // namespace pvzu::engine
