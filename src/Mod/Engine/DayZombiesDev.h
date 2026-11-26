// PVZU: desktop dev options for checking the Day zombies without clicking through the game (inert without them).
//   -pvzu-dev-level <n>              skip title and menus: profile "pvzu-dev" (made if missing), Adventure level n
//   -pvzu-dev-continue               with -pvzu-dev-level: load the level's saved game and press Continue
//   -pvzu-dev-sun <n>                sun when the level starts playing
//   -pvzu-dev-plant SEED:col[@tick][,...]  at that tick of play (default 1), plant SEED (engine name) in column col
//                                    of every row
//   -pvzu-dev-shovel col[@tick][,...]  at that tick of play, shovel the plant in column col of every row (real click path)
//   -pvzu-dev-spawn TYPE[,TYPE...]   once playing, spawn these types in turn (engine names, e.g. PAIL)
//   -pvzu-dev-spawn-every <ticks>    ticks between spawns (default 100)
//   -pvzu-dev-spawn-x <x>            put each spawn at board x, one per row (row = spawn number % 5)
//   -pvzu-dev-spawn-count <n>        how many spawns in all (default: one per listed type)
//   -pvzu-dev-quit <ticks>           quit after that many ticks of play
//   -pvzu-dev-player-level <n>       once playing, set the profile level to n (the board keeps its level; world 7 = 61..70)
//   -pvzu-dev-tree <feet>            once playing, the Tree of Wisdom is bought and <feet> tall
//   -pvzu-dev-finished <n>           once playing, the profile has finished the adventure n times
//   -pvzu-dev-almanac <TYPE>         once playing, open the Almanac on that zombie's page
#pragma once

class LawnApp;

namespace pvzu::engine {

void DayZombiesDevUpdate(LawnApp* theApp); // once per update, from LawnApp::UpdateFrames

} // namespace pvzu::engine
