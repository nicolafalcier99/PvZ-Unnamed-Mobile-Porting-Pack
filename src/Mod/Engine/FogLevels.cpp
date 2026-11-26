#include "Mod/Engine/FogLevels.h"

#include "Mod/CmdLine.h"
#include "Mod/Engine/DevLevels.h"
#include "Mod/Engine/OptionsPanel.h"
#include "Mod/GameData.h"
#include "Mod/Mode.h"
#include "Mod/Unnamed/FogLevels.h"

#include "LawnApp.h"
#include "Lawn/Board.h"

#include <format>
#include <string>

namespace pvzu::engine {

namespace u = pvzu::unnamed;

namespace {
const char* ModeName() { return CurrentMode() == GameMode::Unnamed ? "unnamed" : "vanilla"; }
std::string LevelName(const Board* theBoard) {
	return std::format("{}-{}", (theBoard->mLevel - 1) / 10 + 1, (theBoard->mLevel - 1) % 10 + 1);
}
// World 4 in Adventure, or any stage with fog (minigames): where LeftFogColumn is defined.
bool FogWorld(Board* theBoard) {
	return (theBoard->mApp->IsAdventureMode() && theBoard->mLevel >= 31 && theBoard->mLevel <= 40) || theBoard->StageHasFog();
}
}

bool FogLeftColumn(Board* theBoard, int& theColumn) {
	const int aColumn = u::FogStartColumn(theBoard->mLevel, theBoard->mApp->IsAdventureMode(), FullFogOptionOn()); // R8 full-fog row
	if (aColumn == u::kNoFogRule)
		return false;
	theColumn = aColumn;
	return true;
}

bool FullFogHudOnTop(Board* theBoard) {
	return GameData::Get().Feature("fogLevels") && u::FullFogOn(FullFogOptionOn()) && theBoard->StageHasFog();
}

void DevLogFog(Board* theBoard) {
	if (!CmdOption("-pvzu-log-waves") || !FogWorld(theBoard))
		return;
	LawnApp* aApp = theBoard->mApp;
	const bool aOn = GameData::Get().Feature("fogLevels");
	int aRuleColumn = -1;
	const bool aRow = aOn && FogLeftColumn(theBoard, aRuleColumn);
	DevLog(std::format("fog mode={} level={} feature={} stageHasFog={} leftFogColumn={} source={} fullFog={} stormy={} vasebreaker={} conveyor={}",
		ModeName(), LevelName(theBoard), aOn ? 1 : 0, theBoard->StageHasFog() ? 1 : 0, theBoard->LeftFogColumn(),
		aRow ? "rule" : "engine", aOn && u::FullFogOn(FullFogOptionOn()) ? 1 : 0, aApp->IsStormyNightLevel() ? 1 : 0, aApp->IsScaryPotterLevel() ? 1 : 0,
		theBoard->HasConveyorBeltSeedBank() ? 1 : 0));
}

void FogDevTick(Board* theBoard, int thePlayTicks) {
	if (!FogWorld(theBoard))
		return;
	static const int aAt = CmdOptionInt("-pvzu-dev-fog-at", 300);
	if (thePlayTicks != 1 && !(CmdOption("-pvzu-dev-fog") && thePlayTicks == aAt))
		return;
	DevLog(std::format("fogPlay mode={} level={} tick={} startSun={} awardSeed={} stageHasFog={} leftFogColumn={} fogOffset={:.0f} blownCountDown={} conveyor={} vasebreaker={}",
		ModeName(), LevelName(theBoard), thePlayTicks, theBoard->mSunMoney, static_cast<int>(LawnApp::GetAwardSeedForLevel(theBoard->mLevel)),
		theBoard->StageHasFog() ? 1 : 0, theBoard->LeftFogColumn(), theBoard->mFogOffset, theBoard->mFogBlownCountDown,
		theBoard->HasConveyorBeltSeedBank() ? 1 : 0, theBoard->mApp->IsScaryPotterLevel() ? 1 : 0));
}

} // namespace pvzu::engine
