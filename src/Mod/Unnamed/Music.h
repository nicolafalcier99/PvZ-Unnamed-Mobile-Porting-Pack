// PVZU: Unnamed 1.3beta2 music, as pure rules. gdi42.dll (not the exe) plays the mod's mp3s: its Music::MusicInit
// detour (exe 0x45a8f5 -> 0x100180d8) loads every "lawnbgm(N).mp3" next to the exe, and its Music::PlayMusic detour
// (exe 0x45adeb -> 0x10018800) maps MusicTune -> N with a 12-pair table (.rdata 0x103dbdf0) and plays that file in
// place of the layered vanilla tune. The exe's hook 258 (0x95c1f0) gives Adventure levels 71-80 Cerebrawl.
// Evidence: the research notes (modes-ui-shop) section 8 and pvz-unnamed-pack/SOURCES-music.md. Numbers come from the pack's
// data/rules-music.json ("music.*"); a missing key keeps the vanilla tune.
#pragma once

#include <string>

namespace pvzu::unnamed {

// N of lawnbgm(N).mp3 for a vanilla MusicTune value (1-12), or 0 = play the vanilla tune ("music.tracks", index tune-1).
int MusicTrackForTune(int theTune);
// gdi42 0x10018848: only N == 1, 4 or 5..14 are played (the mod ships no files 2 and 3).
bool ValidTrackNumber(int theN);
std::string MusicTrackFileName(int theN); // "lawnbgm(N).mp3"
// Hook 258: Adventure level in [music.cerebrawlLevelMin, music.cerebrawlLevelMax] (71..80) plays Cerebrawl.
bool CerebrawlAdventureLevel(int theLevel);

// ---- the music choice (PVZU port option, 2026-11-24: "Music: Original / Unnamed mod", Mod/Unnamed/Options.h) ----
// Original (the default): the vanilla soundtrack, mainmusic.mo3 with its drum/burst layers, as Vanilla mode plays it.
// Unnamed mod: the mod's lawnbgm(N).mp3 files (gdi42), exactly as the mod plays them.
namespace musicchoice {
constexpr int ORIGINAL = 0;
constexpr int MOD = 1;
}

// What Music::StartGameMusic (exe .text 0x45b770) looks at, read from LawnApp / Board by the engine.
struct GameTuneInput {
	bool zenOrTree = false;  // Zen Garden or Tree of Wisdom
	bool finalBoss = false;  // LawnApp::IsFinalBossLevel (Adventure 50, the Final Boss mini-game)
	bool miniGame = false;   // Wall-nut Bowling, Whack a Zombie, Little Trouble, Bungee Blitz levels, Zombies on Speed
	bool adventure = false;
	int level = 0;           // PlayerInfo level (Adventure)
	int gameMode = 0;        // LawnApp::mGameMode (mod modes 0x65..0x94)
	bool column = false;     // Column Like You See 'Em
	bool stormy = false;     // LawnApp::IsStormyNightLevel: no music
	bool puzzle = false;     // Vasebreaker or I, Zombie
	bool night = false;      // Board::StageIsNight (night, fog, night roof, mushroom garden, zombiquarium)
	bool pool = false;       // Board::StageHasPool
	bool sixRows = false;    // Board::StageHas6Rows
	bool roof = false;       // Board::StageHasRoof
};
// The tune StartGameMusic plays (a MusicTune value; -1 = none, StopAllMusic). Both choices run the exe's choice:
// vanilla StartGameMusic plus hook 258 (Adventure 71-80 Cerebrawl, rules "music.cerebrawlLevel*"). Original adds the
// vanilla tune for the Unnamed levels that have a vanilla kind but no tune of their own in the exe (port ruling,
// pvz-unnamed-pack SOURCES-music.md is the mod's evidence; this is the port's choice): the 6-5 shooter (55, mode 0x72)
// and the 7-5 star snake (65, mode 0x89) are x-5 mini-games -> Loonboon, the 7-10 boss (70, mode 0x83) -> Brainiac
// Maniac. Every other level and mod mode keeps the tune of its lawn as vanilla maps it (night roof and zombiquarium
// count as night: Moongrains, as in vanilla's Zombiquarium mini-game).
int GameTune(const GameTuneInput& theInput, int theMusicChoice);

} // namespace pvzu::unnamed
