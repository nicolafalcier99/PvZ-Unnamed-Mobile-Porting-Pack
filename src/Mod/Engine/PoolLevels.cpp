#include "Mod/Engine/PoolLevels.h"

#include "Mod/CmdLine.h"
#include "Mod/Engine/DevLevels.h"
#include "Mod/Engine/NightLevels.h"
#include "Mod/GameData.h"
#include "Mod/Mode.h"
#include "Mod/Unnamed/PoolLevels.h"

#include "LawnApp.h"
#include "Lawn/Board.h"
#include "Lawn/GridItem.h"
#include "Lawn/Zombie.h"
#include "Common.h"

#include <format>
#include <string>

namespace pvzu::engine {

namespace u = pvzu::unnamed;

namespace {
std::string Name(int theType) {
	const auto& aZombies = GameData::Get().Active().zombies;
	return theType >= 0 && theType < static_cast<int>(aZombies.size()) ? aZombies[theType].name : std::format("#{}", theType);
}
const char* ModeName() { return CurrentMode() == GameMode::Unnamed ? "unnamed" : "vanilla"; }
std::string LevelName(const Board* theBoard) {
	return std::format("{}-{}", (theBoard->mLevel - 1) / 10 + 1, (theBoard->mLevel - 1) % 10 + 1);
}
bool PoolOn() { return GameData::Get().Feature("poolLevels"); }
}

void PoolSpawnCounts(Board* theBoard, int& theCount, int& thePoints) {
	u::PoolSpawn aSpawn;
	if (!u::PoolFinalWaveSpawn(theBoard->mLevel, aSpawn))
		return;
	theCount = aSpawn.count;
	thePoints = aSpawn.points;
}

void PoolRiserBeforeRise(Zombie* theZombie) {
	if (!u::PoolRisersTakeHypnosis() || !GraveRiseHypnotises())
		return;
	theZombie->mMindControlled = true; // 0x95979A: the byte only, as the exe (no StartMindControlled sound)
	if (const int aRange = u::PoolHypnosisRollRange(); aRange > 0)
		Sexy::Rand(aRange); // 0x9597A6: drawn, result unused (keeps the exe's RNG sequence)
}

bool PoolGravesAfterPool() {
	return u::PoolGravesAfterPoolSpawn();
}

int PoolIceTimerBelowX(int theVanillaX) {
	return u::IceTrailTimerBelowX(theVanillaX);
}

void PoolRisen(Zombie* theZombie) {
	DevLog(std::format("poolRisen mode={} type={} row={} col={} hypno={}", ModeName(), Name(theZombie->mZombieType), theZombie->mRow,
		theZombie->mX / 80, theZombie->mMindControlled ? 1 : 0));
}

void PoolSpawnLog(Board* theBoard, int theCount, int thePoints) {
	DevLog(std::format("poolSpawn mode={} level={} wave={}/{} count={} points={} iceTrap={}", ModeName(), LevelName(theBoard),
		theBoard->mCurrentWave, theBoard->mNumWaves, theCount, thePoints, theBoard->mIceTrapCounter));
}

void DevLogPool(Board* theBoard) {
	if (!CmdOption("-pvzu-log-waves") || !theBoard->StageHasPool())
		return;
	const bool aOn = PoolOn();
	u::PoolSpawn aSpawn;
	const bool aRow = aOn && u::PoolFinalWaveSpawn(theBoard->mLevel, aSpawn);
	DevLog(std::format("pool mode={} level={} poolSpawn={} gravesAfterPool={} hypnoPassed={} iceTimerBelowX={}", ModeName(),
		LevelName(theBoard), aRow ? std::format("{}x{}pts", aSpawn.count, aSpawn.points) : std::string("engine"),
		aOn && PoolGravesAfterPool() ? 1 : 0, aOn && u::PoolRisersTakeHypnosis() ? 1 : 0, aOn ? PoolIceTimerBelowX(800) : 800));
}

void PoolDevTick(Board* theBoard, int thePlayTicks) {
	if (thePlayTicks == 1 && theBoard->StageHasPool())
		DevLog(std::format("play mode={} level={} startSun={} awardSeed={}", ModeName(), LevelName(theBoard), theBoard->mSunMoney,
			static_cast<int>(LawnApp::GetAwardSeedForLevel(theBoard->mLevel))));
	static const std::optional<std::string> aScenario = CmdOption("-pvzu-dev-pool");
	if (!aScenario)
		return;
	static const int aAt = CmdOptionInt("-pvzu-dev-pool-at", 50);
	static ZombieID aZamboni = ZombieID::ZOMBIEID_NULL;
	static bool aIceLogged = false;
	constexpr int kZamboniRow = 1;
	if (thePlayTicks == aAt) {
		DevLog(std::format("devPool {} mode={} level={} wave={}/{}", *aScenario, ModeName(), LevelName(theBoard), theBoard->mCurrentWave,
			theBoard->mNumWaves));
		if (*aScenario == "final") {
			theBoard->SpawnZombiesFromGraves();
		} else if (*aScenario == "final-grave") {
			if (GridItem* aGrave = theBoard->AddAGraveStone(6, 0)) {
				aGrave->mGridItemCounter = 100; // dev: fully risen, so the grave loop may use it now
				DevLog(std::format("devPool grave col={} row={}", aGrave->mGridX, aGrave->mGridY));
			}
			theBoard->SpawnZombiesFromGraves();
		} else if (*aScenario == "final-hypno") {
			GraveRiseScope aScope(ZombieType::ZOMBIE_INVALID, true, "devPool");
			theBoard->SpawnZombiesFromGraves();
		} else if (*aScenario == "zamboni") {
			Zombie* aZombie = theBoard->AddZombieInRow(ZombieType::ZOMBIE_ZAMBONI, kZamboniRow, 0);
			aZamboni = aZombie ? theBoard->ZombieGetID(aZombie) : ZombieID::ZOMBIEID_NULL;
			if (aZombie)
				DevLog(std::format("devPool zamboni start x={} row={}", aZombie->mX, kZamboniRow));
		}
		return;
	}
	if (*aScenario == "zamboni" && !aIceLogged && thePlayTicks > aAt && theBoard->mIceTimer[kZamboniRow] > 0) {
		aIceLogged = true;
		Zombie* aZombie = theBoard->ZombieTryToGet(aZamboni);
		DevLog(std::format("devPool iceTimer set row={} zamboniX={} iceMinX={} tick={}", kZamboniRow, aZombie ? aZombie->mX : -1,
			theBoard->mIceMinX[kZamboniRow], thePlayTicks));
	}
}

} // namespace pvzu::engine
