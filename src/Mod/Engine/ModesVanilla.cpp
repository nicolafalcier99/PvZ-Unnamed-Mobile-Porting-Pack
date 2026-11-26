// PVZU: engine side of the Unnamed changes to the vanilla modes (pure rules: Mod/Unnamed/ModesVanilla.h).
#include "Mod/Engine/ModesVanilla.h"

#include "Mod/CmdLine.h"
#include "Mod/Engine/DevLevels.h"
#include "Mod/Engine/Zoom.h"
#include "Mod/GameData.h"
#include "Mod/Mode.h"
#include "Mod/Unnamed/ModesVanilla.h"
#include "Mod/Unnamed/Rules.h"

#include "LawnApp.h"
#include "Lawn/Board.h"
#include "Lawn/Challenge.h"
#include "Lawn/Coin.h"
#include "Lawn/CursorObject.h"
#include "Lawn/Plant.h"
#include "Lawn/SeedPacket.h"
#include "Lawn/Zombie.h"
#include "Lawn/System/PlayerInfo.h"
#include "Lawn/System/ProfileMgr.h"
#include "Lawn/Widget/ChallengeScreen.h"
#include "Lawn/Widget/TitleScreen.h"
#include "widget/WidgetManager.h"
#include "Common.h"
#include "PvzpLib/PvzpCommon.h"

#include <format>
#include <string>
#include <vector>

namespace pvzu::engine {

namespace u = pvzu::unnamed;

namespace {

const char* RunMode() { return CurrentMode() == pvzu::GameMode::Unnamed ? "unnamed" : "vanilla"; }

std::vector<std::string> ZombieNames() {
	std::vector<std::string> aNames;
	for (const auto& z : GameData::Get().Active().zombies)
		aNames.push_back(z.name);
	return aNames;
}
std::string ZombieName(int theType) {
	const auto& aZombies = GameData::Get().Active().zombies;
	return theType >= 0 && theType < static_cast<int>(aZombies.size()) ? aZombies[theType].name : std::format("#{}", theType);
}
std::string SeedName(int theSeed) {
	const auto& aPlants = GameData::Get().Active().plants;
	if (theSeed >= 0 && theSeed < static_cast<int>(aPlants.size()))
		return aPlants[theSeed].name;
	if (theSeed >= SeedType::SEED_ZOMBIE_NORMAL && theSeed <= SeedType::SEED_ZOMBIE_IMP)
		return "CARD_" + ZombieName(u::IZombieCardZombie(theSeed));
	return std::format("#{}", theSeed);
}

// ---- I, Zombie state (the exe keeps these in globals: 0xc05bdb toggle, packet+0x40 companion) ----
bool gFootballNewspaper = false; // hook 79 / 190: never reset in the exe (kept for the app run)
Zombie* gBungeeCargo = nullptr;  // hook 193: the zombie made before the Bungee
Zombie* gPlacedBungee = nullptr;
int gForcedDancerFollower = -1;  // hook 193: 0x7968eb = 1 while the Dancer is placed, 0xc046c4 = its follower
int gPressPacket = -1;           // long press on the phone
uint gPressStart = 0;

// The companion is kept where the exe keeps it: SeedPacket+0x40 (mSlotMachiningNextSeed, unused in I, Zombie and saved
// with the packet), as a ZombieType; SEED_NONE (-1) = none.
int Companion(const SeedPacket& thePacket) { return static_cast<int>(thePacket.mSlotMachiningNextSeed); }
void SetCompanion(SeedPacket& thePacket, int theZombieType) {
	thePacket.mSlotMachiningNextSeed = static_cast<SeedType>(theZombieType);
}

SeedPacket* FindPacket(Board* theBoard, SeedType theSeed) {
	if (!theBoard || !theBoard->mSeedBank)
		return nullptr;
	for (int i = 0; i < theBoard->mSeedBank->mNumPackets; i++)
		if (theBoard->mSeedBank->mSeedPackets[i].mPacketType == theSeed)
			return &theBoard->mSeedBank->mSeedPackets[i];
	return nullptr;
}

bool gInCost = false;
// The card price without any combination: the patched jump table (Pole 125); a Dancer in a Dancer is its table 350.
int BaseCardCost(SeedType theSeed) {
	gInCost = true;
	const int aVanilla = Plant::GetCost(theSeed, SeedType::SEED_NONE);
	gInCost = false;
	return u::IZombieCardCost(theSeed, aVanilla);
}

void ApplyChallengeSlot(ChallengeDefinition& theDef, const u::ChallengeSlot& theSlot) {
	theDef.mPage = static_cast<ChallengePage>(theSlot.page);
	theDef.mRow = theSlot.row;
	theDef.mCol = theSlot.col;
	theDef.mChallengeIconIndex = theSlot.icon;
}

} // namespace

bool ModesVanillaOn() { return GameData::Get().Feature("modesVanilla"); }

const ChallengeDefinition* ModesChallengeDefinition(int theChallengeIndex) {
	if (!ModesVanillaOn() || theChallengeIndex < 0 || theChallengeIndex >= NUM_CHALLENGE_MODES)
		return nullptr;
	u::ChallengeSlot aSlot;
	if (!u::ChallengeSlotFor(gChallengeDefs[theChallengeIndex].mChallengeMode, aSlot))
		return nullptr;
	static ChallengeDefinition sTable[NUM_CHALLENGE_MODES];
	sTable[theChallengeIndex] = gChallengeDefs[theChallengeIndex];
	ApplyChallengeSlot(sTable[theChallengeIndex], aSlot);
	return &sTable[theChallengeIndex];
}

void ModesAllowList(Challenge* theChallenge) {
	Board* aBoard = theChallenge->mBoard;
	const int aMode = theChallenge->mApp->mGameMode;
	std::string aErr;
	if (ModesVanillaOn()) {
		const u::AllowEdit aEdit = u::AllowListEdit(aMode, ZombieNames(), &aErr);
		u::ApplyAllowEdit(aEdit, aBoard->mZombieAllowed, NUM_ZOMBIE_TYPES);
	}
	if (!aErr.empty())
		std::fprintf(stderr, "PVZU: modesVanilla.allow.%d: %s\n", aMode, aErr.c_str());
	std::string aList;
	for (int t = 0; t < NUM_ZOMBIE_TYPES; t++)
		if (aBoard->mZombieAllowed[t])
			aList += (aList.empty() ? "" : ",") + ZombieName(t);
	DevLog(std::format("modesAllowed mode={} gameMode={} feature={} list={}", RunMode(), aMode, ModesVanillaOn() ? 1 : 0, aList));
}

bool ModesArtIgnoresPicture(SeedType theSeedType) { return ModesVanillaOn() && u::ArtIgnoresPicture(theSeedType); }

int ModesNumWaves(LawnApp* theApp, int theVanilla) {
	if (!ModesVanillaOn() || theApp == nullptr)
		return theVanilla;
	const int aWaves = u::ModeNumWaves(theApp->mGameMode, theVanilla);
	if (aWaves != theVanilla)
		DevLog(std::format("modesWaves mode={} vanilla={} unnamed={}", static_cast<int>(theApp->mGameMode), theVanilla, aWaves));
	return aWaves;
}

void ModesConveyorPicks(LawnApp* theApp, PvzpWeightedArray* thePicks, int& theCount) {
	std::vector<std::pair<int, int>> aPicks;
	if (!ModesVanillaOn() || theApp == nullptr || !u::ConveyorPicks(theApp->mGameMode, aPicks) || aPicks.size() > 20)
		return;
	theCount = static_cast<int>(aPicks.size());
	for (int i = 0; i < theCount; i++) {
		thePicks[i].mItem = aPicks[i].first;
		thePicks[i].mWeight = aPicks[i].second;
	}
}

bool ModesWhackBank(Board* theBoard) {
	if (!ModesVanillaOn() || theBoard->mApp->IsAdventureMode())
		return false;
	const std::vector<int> aBank = u::WhackBank();
	if (aBank.empty() || static_cast<int>(aBank.size()) > theBoard->mSeedBank->mNumPackets)
		return false;
	for (size_t i = 0; i < aBank.size(); i++)
		theBoard->mSeedBank->mSeedPackets[i].SetPacketType(static_cast<SeedType>(aBank[i]));
	DevLog(std::format("modesWhack bank packets={}", aBank.size()));
	return true;
}

int ModesWhackSlots(Board* theBoard, int theVanilla) {
	if (!ModesVanillaOn() || theBoard->mApp->IsAdventureMode())
		return theVanilla;
	const int aSize = static_cast<int>(u::WhackBank().size());
	return aSize > 0 ? aSize : theVanilla;
}

int ModesWhackGroupSize(int theVanilla) { return ModesVanillaOn() ? u::WhackGroupSize(theVanilla) : theVanilla; }
int ModesWhackGraveTarget(int theVanilla) { return ModesVanillaOn() ? u::WhackGraveTarget(theVanilla) : theVanilla; }

int ModesGraveDangerGraves() { return ModesVanillaOn() ? u::GraveDangerGravesPerWave(1) : 1; }

bool ModesSeedBanned(LawnApp* theApp, SeedType theSeedType) {
	return ModesVanillaOn() && u::SeedBannedInMode(theApp->mGameMode, theSeedType);
}

bool ModesBowlingRicochet(Plant* theNut, int thePosX) {
	if (!ModesVanillaOn())
		return false;
	const u::Ricochet r = u::BowlingRicochet(theNut->mLaunchCounter);
	Board* aBoard = theNut->mBoard;
	if (r.kind == u::RicochetKind::Zombie) {
		// 0xc110d0: AddZombieInRow(type, row) (the exe's third argument is whatever EBX holds: the hit x), then
		// mPosX = the nut's mX and mMindControlled = 1 (+0xb8), no StartMindControlled sound.
		Zombie* aZombie = aBoard->AddZombieInRow(static_cast<ZombieType>(r.value), theNut->mRow, thePosX);
		if (aZombie) {
			aZombie->mPosX = static_cast<float>(theNut->mX);
			aZombie->mMindControlled = true;
		}
		DevLog(std::format("modesBowling count={} row={} reward=zombie {} hypnotised x={}", theNut->mLaunchCounter, theNut->mRow,
			ZombieName(r.value), theNut->mX));
		return true;
	}
	if (r.kind == u::RicochetKind::SeedPacket) {
		Coin* aCoin = aBoard->AddCoin(theNut->mX, theNut->mY, CoinType::COIN_USABLE_SEED_PACKET, CoinMotion::COIN_MOTION_COIN);
		if (aCoin)
			aCoin->mUsableSeedType = static_cast<SeedType>(r.value);
		DevLog(std::format("modesBowling count={} row={} reward=packet {}", theNut->mLaunchCounter, theNut->mRow, SeedName(r.value)));
		return true;
	}
	DevLog(std::format("modesBowling count={} row={} reward=vanilla", theNut->mLaunchCounter, theNut->mRow));
	return false;
}

bool ModesScaryPotDirectSun(Challenge* theChallenge, int theSunCount) {
	if (!ModesVanillaOn())
		return false;
	const int aSun = u::ScaryPotDirectSun(theSunCount);
	if (aSun <= 0)
		return false;
	theChallenge->mBoard->mSunMoney += aSun; // 0xc01f31: add [board+0x5560], no cap
	DevLog(std::format("modesPot sunCount={} sunAdded={}", theSunCount, aSun));
	return true;
}

bool ModesIZombieCost(SeedType theSeedType, int& theCost) {
	if (gInCost || !ModesVanillaOn() || theSeedType < SeedType::SEED_ZOMBIE_NORMAL || theSeedType > SeedType::SEED_ZOMBIE_IMP)
		return false;
	const int aBase = BaseCardCost(theSeedType);
	theCost = aBase;
	if (u::IZombieIsCarrier(theSeedType)) {
		const SeedPacket* aPacket = FindPacket(gLawnApp ? gLawnApp->mBoard : nullptr, theSeedType);
		const int aCompanion = aPacket ? Companion(*aPacket) : -1;
		const int aCard = aCompanion >= 0 ? u::IZombieZombieCard(aCompanion) : -1;
		theCost = u::IZombieCarrierCost(theSeedType, aCompanion, aBase, aCard >= 0 ? BaseCardCost(static_cast<SeedType>(aCard)) : 0);
	}
	return true;
}

ZombieType ModesIZombieCardZombie(SeedType theSeedType, ZombieType theVanilla) {
	if (!ModesVanillaOn() || theSeedType != SeedType::SEED_ZOMBIE_FOOTBALL)
		return theVanilla;
	return static_cast<ZombieType>(u::IZombieFootballZombie(gFootballNewspaper, theVanilla));
}

void ModesIZombieCarrierClick(Board* theBoard, const HitResult& theHit) {
	if (!ModesVanillaOn() || theHit.mObjectType != GameObjectType::OBJECT_TYPE_SEEDPACKET || !theHit.mObject)
		return;
	SeedPacket* aPacket = static_cast<SeedPacket*>(theHit.mObject);
	const SeedType aHeld = theBoard->mCursorObject->mType;
	const int aCompanion = u::IZombieCompanionFor(aPacket->mPacketType, aHeld);
	if (aCompanion < 0)
		return;
	SetCompanion(*aPacket, aCompanion);
	int aCost = 0;
	ModesIZombieCost(aPacket->mPacketType, aCost);
	DevLog(std::format("modesIZ carrier={} held={} companion={} cost={}", SeedName(aPacket->mPacketType), SeedName(aHeld),
		ZombieName(aCompanion), aCost));
}

bool ModesIZombiePacketClick(Board* theBoard, SeedPacket* thePacket, int theClickCount) {
	(void)theBoard;
	if (!ModesVanillaOn() || theClickCount > 0 || !thePacket)
		return false;
	const SeedType aSeed = thePacket->mPacketType;
	if (u::IZombieRightClickToggles(aSeed)) { // 0xc05b6a: xor [0xc05bdb], 1
		gFootballNewspaper = !gFootballNewspaper;
		DevLog(std::format("modesIZ football toggled newspaper={}", gFootballNewspaper ? 1 : 0));
		return true;
	}
	if (u::IZombieIsCarrier(aSeed)) { // 0xc045d0 / 0x8f037b: companion -1, price back to the base
		SetCompanion(*thePacket, -1);
		DevLog(std::format("modesIZ carrier={} companion cleared", SeedName(aSeed)));
		return true;
	}
	return false;
}

void ModesIZombiePressDown(Board* theBoard, const HitResult& theHit) {
	gPressPacket = -1;
	if (!ModesVanillaOn() || !theBoard->mApp->IsIZombieLevel() || theHit.mObjectType != GameObjectType::OBJECT_TYPE_SEEDPACKET)
		return;
	SeedPacket* aPacket = static_cast<SeedPacket*>(theHit.mObject);
	if (!u::IZombieRightClickToggles(aPacket->mPacketType) && !u::IZombieIsCarrier(aPacket->mPacketType))
		return;
	gPressPacket = aPacket->mIndex;
	gPressStart = theBoard->mApp->mUpdateCount;
}

void ModesIZombiePressUp(Board* theBoard, int x, int y) {
	const int aIndex = gPressPacket;
	gPressPacket = -1;
	if (aIndex < 0 || !ModesVanillaOn() || !theBoard->mSeedBank || aIndex >= theBoard->mSeedBank->mNumPackets)
		return;
	const uint aHeldFor = theBoard->mApp->mUpdateCount - gPressStart;
	if (aHeldFor < static_cast<uint>(u::RuleInt("modesVanilla.iz.longPressTicks", 50)))
		return;
	SeedPacket& aPacket = theBoard->mSeedBank->mSeedPackets[aIndex];
	int aX = x, aY = y;
	LocalToBoard(theBoard, aX, aY); // as Board::MouseDown does before its hit test
	HitResult aHit{};
	if (!theBoard->MouseHitTest(aX, aY, &aHit) || aHit.mObject != &aPacket) // the finger slid off the card: a normal pick-up
		return;
	if (theBoard->mCursorObject->mCursorType == CursorType::CURSOR_TYPE_PLANT_FROM_BANK && theBoard->mCursorObject->mSeedBankIndex == aIndex)
		theBoard->RefreshSeedPacketFromCursor(); // the press picked it up; a long press is the right click instead
	DevLog(std::format("modesIZ long press card={} ticks={}", SeedName(aPacket.mPacketType), aHeldFor));
	ModesIZombiePacketClick(theBoard, &aPacket, -1);
}

void ModesIZombieBeforePlace(Board* theBoard, ZombieType theZombieType) {
	gBungeeCargo = nullptr;
	gPlacedBungee = nullptr;
	gForcedDancerFollower = -1;
	if (!ModesVanillaOn())
		return;
	const int aIndex = theBoard->mCursorObject->mSeedBankIndex;
	if (aIndex < 0 || aIndex >= theBoard->mSeedBank->mNumPackets)
		return;
	const int aCompanion = Companion(theBoard->mSeedBank->mSeedPackets[aIndex]);
	if (aCompanion < 0)
		return;
	if (theZombieType == ZombieType::ZOMBIE_BUNGEE) {
		gBungeeCargo = theBoard->AddZombieInRow(static_cast<ZombieType>(aCompanion), 0, 0); // 0x8f0520, before the Bungee
		DevLog(std::format("modesIZ bungee cargo={} made={}", ZombieName(aCompanion), gBungeeCargo ? 1 : 0));
	} else if (theZombieType == ZombieType::ZOMBIE_DANCER) {
		gForcedDancerFollower = aCompanion;
	}
}

void ModesIZombiePlaced(Zombie* thePlaced) {
	if (gBungeeCargo && thePlaced && thePlaced->mZombieType == ZombieType::ZOMBIE_BUNGEE)
		gPlacedBungee = thePlaced;
}

void ModesIZombieAfterPlace() {
	if (gBungeeCargo && gPlacedBungee) { // 0x8f0557: BungeeDropZombie(cargo, the Bungee's target column, its row)
		gPlacedBungee->BungeeDropZombie(gBungeeCargo, gPlacedBungee->mTargetCol, gPlacedBungee->mRow);
		DevLog(std::format("modesIZ bungee drops {} col={} row={}", ZombieName(gBungeeCargo->mZombieType), gPlacedBungee->mTargetCol,
			gPlacedBungee->mRow));
	}
	gBungeeCargo = nullptr;
	gPlacedBungee = nullptr;
	gForcedDancerFollower = -1;
}

bool ModesForcedDancer(int& theFollowerType) {
	if (gForcedDancerFollower < 0)
		return false;
	theFollowerType = gForcedDancerFollower;
	DevLog(std::format("modesIZ dancer forced random summoner follower={}", ZombieName(gForcedDancerFollower)));
	return true;
}

bool ModesStarfruitNoExtraStars(LawnApp* theApp) {
	return ModesVanillaOn() && u::StarfruitNoExtraStars(theApp->mGameMode);
}

bool ModesGloveCanPickUp(Plant* thePlant) {
	if (!ModesVanillaOn() || u::GloveCanPickUp(thePlant->mSeedType, thePlant->mState))
		return true;
	DevLog(std::format("modesZen glove leaves {} state={}", SeedName(thePlant->mSeedType), static_cast<int>(thePlant->mState)));
	return false;
}

// ---- Dev ----

bool ModesDevStart(LawnApp* theApp) {
	const int aMode = CmdOptionInt("-pvzu-dev-mode", -1);
	const int aPage = CmdOptionInt("-pvzu-dev-challenge-page", -1);
	if (aMode < 0 && aPage < 0)
		return false;
	if (theApp->mPlayerInfo == nullptr)
		theApp->mPlayerInfo = theApp->mProfileMgr->AddProfile("pvzu");
	if (theApp->mPlayerInfo == nullptr)
		return false;
	if (CmdOption("-pvzu-seed")) {
		const int aSeedValue = CmdOptionInt("-pvzu-seed", 0);
		Sexy::SRand(static_cast<ulong>(aSeedValue));
		theApp->mAppRandSeed = aSeedValue;
	}
	if (aPage >= 0) {
		// A profile that has seen everything: adventure finished, every Normal/Hard survival won (Endless unlocked).
		PlayerInfo* p = theApp->mPlayerInfo;
		if (p->mFinishedAdventure < 1)
			p->mFinishedAdventure = 1;
		p->mHasUnlockedMinigames = 1;
		p->mHasUnlockedPuzzleMode = 1;
		p->mHasUnlockedSurvivalMode = 1;
		for (int i = 0; i < 10; i++)
			p->mChallengeRecords[i] = 99;
		theApp->mWidgetManager->RemoveWidget(theApp->mTitleScreen.get());
		theApp->SafeDeleteWidget(theApp->mTitleScreen.release());
		theApp->ShowChallengeScreen(static_cast<ChallengePage>(std::clamp(aPage, 0, MAX_CHALLANGE_PAGES - 1)));
		std::string aLine;
		for (int i = 0; i < NUM_CHALLENGE_MODES; i++) {
			const ChallengeDefinition& d = GetChallengeDefinition(i);
			if (d.mPage == aPage)
				aLine += std::format(" {}@r{}c{}i{}", static_cast<int>(d.mChallengeMode), d.mRow, d.mCol, d.mChallengeIconIndex);
		}
		DevLog(std::format("modesChallenge mode={} page={} entries:{}", RunMode(), aPage, aLine));
		return true;
	}
	DevLog(std::format("start mode={} gameMode={} seed={}", RunMode(), aMode, CmdOption("-pvzu-seed").value_or("-")));
	theApp->FastLoad(static_cast<::GameMode>(aMode));
	return true;
}

namespace {
Point LawnCell(Board* theBoard, int theCol, int theRow) {
	return Point(theBoard->GridToPixelX(theCol, theRow) + 40, theBoard->GridToPixelY(theCol, theRow) + 40);
}

void PlaceFromCard(Board* theBoard, SeedType theCard, int theCol, int theRow) {
	SeedPacket* aPacket = FindPacket(theBoard, theCard);
	if (!aPacket) {
		DevLog(std::format("modesIZ dev: no {} card", SeedName(theCard)));
		return;
	}
	aPacket->mActive = true;
	aPacket->mRefreshing = false;
	theBoard->ClearCursor();
	theBoard->mCursorObject->mType = theCard;
	theBoard->mCursorObject->mImitaterType = SeedType::SEED_NONE;
	theBoard->mCursorObject->mSeedBankIndex = aPacket->mIndex;
	theBoard->mCursorObject->mCursorType = CursorType::CURSOR_TYPE_PLANT_FROM_BANK;
	const int aSunBefore = theBoard->mSunMoney;
	const Point p = LawnCell(theBoard, theCol, theRow);
	theBoard->mChallenge->IZombieMouseDownWithZombie(p.mX, p.mY, 1);
	DevLog(std::format("modesIZ dev placed card={} col={} row={} paid={}", SeedName(theCard), theCol, theRow, aSunBefore - theBoard->mSunMoney));
}

void HoldAndClick(Board* theBoard, SeedType theHeld, SeedType theCarrier) {
	SeedPacket* aCarrier = FindPacket(theBoard, theCarrier);
	if (!aCarrier)
		return;
	theBoard->mCursorObject->mType = theHeld;
	theBoard->mCursorObject->mCursorType = CursorType::CURSOR_TYPE_PLANT_FROM_BANK;
	HitResult aHit{aCarrier, GameObjectType::OBJECT_TYPE_SEEDPACKET};
	ModesIZombieCarrierClick(theBoard, aHit);
	theBoard->ClearCursor();
}

void LogZombies(Board* theBoard, const char* theWhat) {
	for (Zombie* z : theBoard->mZombies) {
		if (z->mDead)
			continue;
		DevLog(std::format("modesIZ {} zombie={} row={} x={:.0f} variant=0x{:x} stored={} mind={} height={}", theWhat,
			ZombieName(z->mZombieType), z->mRow, z->mPosX, z->mPvzu.variant, z->mPvzu.stored, z->mMindControlled ? 1 : 0,
			static_cast<int>(z->mZombieHeight)));
	}
}
} // namespace

void ModesDevTick(Board* theBoard, int thePlayTicks) {
	static const std::optional<std::string> aScenario = CmdOption("-pvzu-dev-modes");
	LawnApp* aApp = theBoard->mApp;
	if (thePlayTicks == 1)
		DevLog(std::format("modesPlay mode={} gameMode={} feature={} scenario={}", RunMode(), static_cast<int>(aApp->mGameMode),
			ModesVanillaOn() ? 1 : 0, aScenario.value_or("-")));
	if (thePlayTicks == 1 && theBoard->mSeedBank && CmdOption("-pvzu-log-waves")) {
		std::string aBank;
		for (int i = 0; i < theBoard->mSeedBank->mNumPackets; i++)
			aBank += (i ? "," : "") + SeedName(theBoard->mSeedBank->mSeedPackets[i].mPacketType);
		DevLog(std::format("modesBan mode={} gameMode={} graveBusterBanned={} seedBank={}", RunMode(), static_cast<int>(aApp->mGameMode),
			ModesSeedBanned(aApp, SeedType::SEED_GRAVEBUSTER) ? 1 : 0, aBank));
	}
	if (thePlayTicks % 1000 == 0 && CmdOption("-pvzu-log-waves")) {
		int aGraves = 0;
		for (GridItem* g : theBoard->mGridItems)
			if (!g->mDead && g->mGridItemType == GridItemType::GRIDITEM_GRAVESTONE)
				aGraves++;
		DevLog(std::format("modesTick mode={} tick={} wave={}/{} graves={} zombies={} sun={}", RunMode(), thePlayTicks,
			theBoard->mCurrentWave, theBoard->mNumWaves, aGraves, theBoard->mZombies.mSize, theBoard->mSunMoney));
	}
	if (!aScenario)
		return;
	if (*aScenario == "clear" && thePlayTicks % 60 == 0) { // keep the lawn alive so the waves (and graves) go on
		for (Zombie* z : theBoard->mZombies)
			if (!z->mDead && !z->mMindControlled && z->mZombieType != ZombieType::ZOMBIE_BOSS && z->mPosX < 700.0f)
				z->DieNoLoot();
		if (theBoard->mZombieCountDown > 300) // dev only: the next wave comes within 3 s
			theBoard->mZombieCountDown = 300;
	} else if (*aScenario == "bowling") {
		if (thePlayTicks % 400 == 0) // a column of walkers in every row
			for (int aRow = 0; aRow < 5; aRow++)
				for (int i = 0; i < 3; i++) {
					Zombie* z = theBoard->AddZombieInRow(ZombieType::ZOMBIE_NORMAL, aRow, 0);
					if (z)
						z->mPosX = 520.0f + 70.0f * i;
				}
		if (thePlayTicks % 150 == 0) // a Wall-nut rolled from column 0, rows in turn
			theBoard->AddPlant(0, (thePlayTicks / 150) % 5, SeedType::SEED_WALLNUT, SeedType::SEED_NONE);
	} else if (*aScenario == "iz" && thePlayTicks == 100) {
		theBoard->mSunMoney = 5000;
		SeedPacket* aFootball = FindPacket(theBoard, SeedType::SEED_ZOMBIE_FOOTBALL);
		if (aFootball && !ModesIZombiePacketClick(theBoard, aFootball, -1)) // the right click on the Football card
			DevLog("modesIZ dev right click on the Football card: vanilla (not handled)");
		HoldAndClick(theBoard, SeedType::SEED_ZOMBIE_TRAFFIC_CONE, SeedType::SEED_ZOMBIE_DANCER);
		HoldAndClick(theBoard, SeedType::SEED_ZOMBIE_PAIL, SeedType::SEED_ZOMBIE_BUNGEE);
		for (SeedType s : {SeedType::SEED_ZOMBIE_POLEVAULTER, SeedType::SEED_ZOMBIE_DANCER, SeedType::SEED_ZOMBIE_BUNGEE}) {
			int c = Plant::GetCost(s, SeedType::SEED_NONE);
			DevLog(std::format("modesIZ dev cost card={} cost={}", SeedName(s), c));
		}
		PlaceFromCard(theBoard, SeedType::SEED_ZOMBIE_FOOTBALL, 8, 0);
		PlaceFromCard(theBoard, SeedType::SEED_ZOMBIE_DANCER, 8, 2);
		PlaceFromCard(theBoard, SeedType::SEED_ZOMBIE_BUNGEE, 3, 4);
		LogZombies(theBoard, "after");
	} else if (*aScenario == "iz" && thePlayTicks > 100 && thePlayTicks <= 1200 && thePlayTicks % 100 == 0) {
		LogZombies(theBoard, std::format("t{}", thePlayTicks).c_str());
	} else if (*aScenario == "art" && thePlayTicks == 20) {
		int aCells = 0, aBlocked = 0;
		for (int aCol = 0; aCol < MAX_GRID_SIZE_X; aCol++)
			for (int aRow = 0; aRow < MAX_GRID_SIZE_Y; aRow++) {
				const SeedType aArt = theBoard->mChallenge->GetArtChallengeSeed(aCol, aRow);
				if (aArt == SeedType::SEED_NONE || aArt == SeedType::SEED_GRAVEBUSTER)
					continue;
				aCells++;
				if (theBoard->mChallenge->CanPlantAt(aCol, aRow, SeedType::SEED_GRAVEBUSTER) != PlantingReason::PLANTING_OK)
					aBlocked++;
			}
		DevLog(std::format("modesArt mode={} gameMode={} seed=GRAVEBUSTER artCells={} notOnArt={}", RunMode(),
			static_cast<int>(aApp->mGameMode), aCells, aBlocked));
	}
}

} // namespace pvzu::engine
