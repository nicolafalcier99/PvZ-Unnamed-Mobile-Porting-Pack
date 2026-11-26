// PVZU: dev-only harness for Mac checks (never used unless its -pvzu-* options are given).
//   -pvzu-level <n>         skip the menus: start Adventure level n (makes a profile "pvzu" if none)
//   -pvzu-plant-test        at level start, plant one test layout of every Day plant and keep zombies coming;
//                           on a Night level (or with -pvzu-plant-test-night on any level) the Night-plant layout
//                           on a Pool level (or with -pvzu-plant-test-pool on any level) the Pool-plant layout
//                           on a Fog level (or with -pvzu-plant-test-fog on any level) the Fog-plant layout
//                           on a Roof level (or with -pvzu-plant-test-roof on any level) the Roof-plant layout
//   -pvzu-speed <n>         run the game n times faster (SexyAppBase::mUpdateMultiplier)
//   -pvzu-quit-after <cs>   exit after n board ticks of play
#pragma once

class LawnApp;
class Board;

namespace pvzu::engine {

// From TitleScreen when loading finished. Returns false (does nothing) without -pvzu-level.
// DevStartLevel: see Mod/Engine/DevLevels.h
// From Board::Update every unpaused tick.
void DevBoardTick(Board* theBoard);

} // namespace pvzu::engine
