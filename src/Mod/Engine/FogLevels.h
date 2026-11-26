// PVZU: engine side of the Unnamed Fog level rules (pure rules in Mod/Unnamed/FogLevels.h).
// The caller guards the rule call with GameData::Get().Feature("fogLevels"); the dev log calls are inert unless
// -pvzu-log-waves / -pvzu-dev-fog is given.
//   -pvzu-dev-fog <any>        pass Dave's talk and the seed chooser without clicks (random seeds), then log the fog
//                              state once at -pvzu-dev-fog-at (default 300 play ticks, after the fog has rolled in)
//   -pvzu-dev-fog-at <ticks>   play tick of that log line
#pragma once

class Board;

namespace pvzu::engine {

// Board::LeftFogColumn: true = a rule decides (theColumn set: full-fog flag, or the pack's per-level row).
bool FogLeftColumn(Board* theBoard, int& theColumn);
// Board render list (R8): full fog (column 0) reaches the top bar and the seed bank, so in play their item goes just
// above the fog (under the rain / storm). False without full fog: the vanilla order.
bool FullFogHudOnTop(Board* theBoard);

// End of Board::PickZombieWaves (before DevLogWaves): fog rule line to the -pvzu-log-waves file.
void DevLogFog(Board* theBoard);
// Board::Update via DevBoardTick: fog line on the first play tick (any fog-lawn level) and at -pvzu-dev-fog-at.
void FogDevTick(Board* theBoard, int thePlayTicks);

} // namespace pvzu::engine
