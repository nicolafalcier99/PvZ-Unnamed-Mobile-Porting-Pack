// PVZU: Unnamed music in the engine (feature "unnamedMusic"). Rules: Mod/Unnamed/Music.h; evidence:
// the research notes (modes-ui-shop) section 8, pvz-unnamed-pack/SOURCES-music.md. The mp3s are the imported pack assets
// (assets/lawnbgm(N).mp3, never in git); a missing file keeps the vanilla tune.
// Dev: -pvzu-log-music logs each tune choice (file, decoder, length, volumes) and, every 300 updates, how far the
// mixer has decoded the playing song (works with SDL_AUDIO_DRIVER=dummy: the dummy device still pulls audio).
// -pvzu-music-seams <dir> writes every imported song's loop point (2 s before + 2 s after, as played) to
// <dir>/seam-lawnbgm<N>.wav at the first Unnamed tune (Mod/MusicLoop.h, SDLMusicInterface::RenderLoopSeam).
#pragma once

#include "Lawn/System/Music.h"

class LawnApp;

namespace pvzu::engine {

bool UnnamedMusicOn(); // GameData::Get().Feature("unnamedMusic")
// The options panel's "Music" row (Mod/Unnamed/Options.h, musicchoice::ORIGINAL by default; Original with the
// feature off or without the panel's feature shopUi).
int UnnamedMusicChoice();

// Music::PlayMusic site (gdi42.dll detour 0x10018800 at exe 0x45adeb): with the choice "Unnamed mod", plays
// lawnbgm(N).mp3 for theTune and returns true; or returns false and the vanilla tune code runs (feature off, choice
// Original, tune not in the table, file missing).
bool PlayUnnamedMusic(Music& theMusic, MusicTune theTune);
// True for a song id PlayUnnamedMusic uses: Music::UpdateMusicBurst skips the vanilla drum/burst layers for it.
bool IsUnnamedMusicFile(int theMusicFile);
// Music::StartGameMusic site: with the feature on, picks the tune by Mod/Unnamed/Music.h GameTune (vanilla's choice,
// hook 258's Adventure 71-80 Cerebrawl, and with Original the vanilla tunes of 55 / 65 / 70) and returns true.
bool UnnamedStartGameMusic(Music& theMusic);
// The panel's Music row changed: the playing tune restarts at once in the new choice (paused again if it was paused;
// on a lawn the tune is picked again, as the level's tune can differ), and a loaded mp3 is freed for Original.
void UnnamedMusicChoiceChanged(LawnApp* theApp);
// Music::MusicUpdate: the -pvzu-log-music position line (no game effect).
void UnnamedMusicTick(Music& theMusic);

} // namespace pvzu::engine
