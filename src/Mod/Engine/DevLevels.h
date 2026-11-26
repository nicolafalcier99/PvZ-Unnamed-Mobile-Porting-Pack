// PVZU: developer flags for checking level rules on the Mac (no effect unless the flag is given).
//   -pvzu-level <n>            start adventure level n straight after loading (makes a "pvzu" profile if none)
//   -pvzu-seed <n>             seed the game RNG just before that level is made (repeatable wave picks)
//   -pvzu-log-waves <file>     append each picked wave (zombies + value budget) and rule events to <file>
//   -pvzu-quit-after-waves     exit right after the waves are logged
//   -pvzu-dev-vault            at level start: plants + one Pole Vaulter per row 0-3 (vault/bonk/grave check)
//   -pvzu-dev-rise             after a vault grave appears, raise zombies from all graves (grave re-roll check)
//   -pvzu-dev-coins <n>        set the profile's coins (stored /10) before the level (Dave's seed-slot offer)
//   -pvzu-dev-zen              with -pvzu-level: open the Zen Garden instead of the level (Dave's first-visit talk)
#pragma once

#include <string_view>

class LawnApp;
class Board;

namespace pvzu::engine {

// TitleScreen, once loading is done. True if a level was started.
bool DevStartLevel(LawnApp* theApp);
// End of Board::PickZombieWaves.
void DevLogWaves(Board* theBoard);
// End of Board::StartLevel.
void DevOnStartLevel(Board* theBoard);
// One line to the -pvzu-log-waves file (and stderr); no-op without the flag.
void DevLog(std::string_view theLine);
bool DevFlag(std::string_view theName);

} // namespace pvzu::engine
