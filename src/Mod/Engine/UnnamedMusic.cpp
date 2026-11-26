#include "Mod/Engine/UnnamedMusic.h"

#include "Mod/AssetOverlay.h"
#include "Mod/CmdLine.h"
#include "Mod/GameData.h"
#include "Mod/Engine/OptionsPanel.h"
#include "Mod/Unnamed/Music.h"
#include "Mod/Unnamed/Options.h"

#include "LawnApp.h"
#include "Lawn/Board.h"
#include "Lawn/System/PlayerInfo.h"
#include "PvzpLib/PvzpDebug.h"
#include "sound/MusicInterface.h"
#include "sound/SDLMusicInterface.h"

#include <set>
#include <string>

namespace pvzu::engine {

namespace u = pvzu::unnamed;

namespace {
// Song ids for lawnbgm(N).mp3: kSongIdBase + N, clear of the vanilla MusicFile ids 1-4. (gdi42 numbers them from
// MusicFile 6 in load order; the id itself is never saved or seen.)
constexpr int kSongIdBase = 100;
int gLoadedTrack = 0; // N of the one mp3 kept loaded (0 = none); a new tune unloads the previous file
std::set<int> gMissingWarned;
int gTicks = 0;

const char* TuneName(MusicTune theTune) {
	static const char* const kNames[] = {"none", "DAY_GRASSWALK", "NIGHT_MOONGRAINS", "POOL_WATERYGRAVES",
		"FOG_RIGORMORMIST", "ROOF_GRAZETHEROOF", "CHOOSE_YOUR_SEEDS", "TITLE_CRAZY_DAVE_MAIN_THEME", "ZEN_GARDEN",
		"PUZZLE_CEREBRAWL", "MINIGAME_LOONBOON", "CONVEYER", "FINAL_BOSS_BRAINIAC_MANIAC", "CREDITS_ZOMBIES_ON_YOUR_LAWN"};
	const int anIndex = theTune < 0 ? 0 : static_cast<int>(theTune);
	return anIndex < static_cast<int>(std::size(kNames)) ? kNames[anIndex] : "?";
}

bool LogMusic() { return CmdFlag("-pvzu-log-music"); }

std::string SongName(int theMusicFile) {
	if (IsUnnamedMusicFile(theMusicFile))
		return u::MusicTrackFileName(theMusicFile - kSongIdBase);
	switch (theMusicFile) {
	case MusicFile::MUSIC_FILE_MAIN_MUSIC: return "mainmusic.mo3";
	case MusicFile::MUSIC_FILE_DRUMS: return "mainmusic.mo3 (drums)";
	case MusicFile::MUSIC_FILE_CREDITS_ZOMBIES_ON_YOUR_LAWN: return "ZombiesOnYourLawn.ogg";
	default: return "none";
	}
}

// Loads lawnbgm(N).mp3 from the pack's assets as song kSongIdBase + N (unloading the previous one).
bool EnsureLoaded(Sexy::MusicInterface* theInterface, int theN) {
	if (gLoadedTrack == theN)
		return true;
	const std::string aName = u::MusicTrackFileName(theN);
	const auto aPath = OverlayResolve(aName);
	if (!aPath) {
		if (gMissingWarned.insert(theN).second)
			PvzpLogLn("PVZU: music {} not imported (pack assets); playing the vanilla tune", aName);
		return false;
	}
	if (gLoadedTrack != 0)
		theInterface->UnloadMusic(kSongIdBase + gLoadedTrack);
	gLoadedTrack = 0;
	if (!theInterface->LoadMusic(kSongIdBase + theN, aPath->string())) {
		if (gMissingWarned.insert(theN).second)
			PvzpLogLn("PVZU: music {} failed to load (no mp3 decoder?); playing the vanilla tune", aName);
		return false;
	}
	gLoadedTrack = theN;
	return true;
}
// -pvzu-music-seams <dir> (dev): every imported song's loop point as the game plays it (2 s before + 2 s after) to
// <dir>/seam-lawnbgm<N>.wav, once, at the first Unnamed tune. Works with SDL_AUDIO_DRIVER=dummy (offline mixer).
void DumpLoopSeams(Sexy::MusicInterface* theInterface) {
	static bool sDone = false;
	const auto aDir = CmdOption("-pvzu-music-seams");
	if (sDone || !aDir)
		return;
	sDone = true;
	auto* anSdl = dynamic_cast<Sexy::SDLMusicInterface*>(theInterface);
	for (int aN = 1; aN <= 14; ++aN) {
		if (!u::ValidTrackNumber(aN) || anSdl == nullptr || !EnsureLoaded(theInterface, aN))
			continue;
		const std::string aWav = *aDir + "/seam-lawnbgm" + std::to_string(aN) + ".wav";
		const bool anOk = anSdl->RenderLoopSeam(kSongIdBase + aN, aWav);
		PvzpLogLn("PVZU: music seam {} ({}) -> {} {}", u::MusicTrackFileName(aN),
			theInterface->GetSongDecoder(kSongIdBase + aN), aWav, anOk ? "written" : "FAILED");
	}
}
} // namespace

bool UnnamedMusicOn() { return GameData::Get().Feature("unnamedMusic"); }

int UnnamedMusicChoice() {
	return UnnamedMusicOn() ? CurrentOptions().Get(u::OptionId::Music) : u::musicchoice::ORIGINAL;
}

namespace {
const char* ChoiceName(int theChoice) { return theChoice == u::musicchoice::MOD ? "unnamed mod" : "original"; }
} // namespace

bool IsUnnamedMusicFile(int theMusicFile) {
	return theMusicFile > kSongIdBase && u::ValidTrackNumber(theMusicFile - kSongIdBase);
}

bool PlayUnnamedMusic(Music& theMusic, MusicTune theTune) {
	const int aChoice = UnnamedMusicChoice();
	const int aN = UnnamedMusicOn() && aChoice == u::musicchoice::MOD ? u::MusicTrackForTune(static_cast<int>(theTune)) : 0;
	if (aN != 0 && theMusic.mMusicInterface != nullptr)
		DumpLoopSeams(theMusic.mMusicInterface);
	if (aN == 0 || theMusic.mMusicInterface == nullptr || !EnsureLoaded(theMusic.mMusicInterface, aN)) {
		if (LogMusic())
			PvzpLogLn("PVZU: music tune {} {} -> vanilla {} (music choice {})", static_cast<int>(theTune), TuneName(theTune),
				theTune == MusicTune::MUSIC_TUNE_CREDITS_ZOMBIES_ON_YOUR_LAWN ? "ZombiesOnYourLawn.ogg" : "mainmusic.mo3",
				UnnamedMusicOn() ? ChoiceName(aChoice) : "none, feature off");
		gTicks = 0;
		return false;
	}

	// gdi42 0x100185a0: StopAllMusic (0x45abb0), mCurMusicFileMain = the mp3, PlayFromOffset(file, 0, 1.0 at
	// 0x65b8f8) and skip the rest of PlayMusic (no drums, no hihats). Ruling: mCurMusicTune keeps the tune (the
	// mod's StopAllMusic leaves it -1), so pause, fade-out and "same tune keeps playing" work as in vanilla.
	const int aSongId = kSongIdBase + aN;
	theMusic.StopAllMusic();
	theMusic.mCurMusicTune = theTune;
	theMusic.mCurMusicFileMain = static_cast<MusicFile>(aSongId);
	theMusic.PlayFromOffset(static_cast<MusicFile>(aSongId), 0, 1.0); // loops: vanilla PlayFromOffset, tune != credits
	if (LogMusic()) {
		Sexy::MusicInterface* anInterface = theMusic.mMusicInterface;
		PvzpLogLn("PVZU: music tune {} {} -> {} (music choice unnamed mod, decoder {}, {:.2f} s, song volume 1.00, music volume {:.2f})",
			static_cast<int>(theTune), TuneName(theTune), u::MusicTrackFileName(aN), anInterface->GetSongDecoder(aSongId),
			anInterface->GetSongSeconds(aSongId), theMusic.mApp->GetMusicVolume());
	}
	gTicks = 0;
	return true;
}

bool UnnamedStartGameMusic(Music& theMusic) {
	LawnApp* anApp = theMusic.mApp;
	if (!UnnamedMusicOn() || anApp == nullptr || anApp->mBoard == nullptr)
		return false;
	Board* aBoard = anApp->mBoard;
	u::GameTuneInput in;
	in.zenOrTree = anApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_ZEN_GARDEN || anApp->mGameMode == GameMode::GAMEMODE_TREE_OF_WISDOM;
	in.finalBoss = anApp->IsFinalBossLevel();
	in.miniGame = anApp->IsWallnutBowlingLevel() || anApp->IsWhackAZombieLevel() || anApp->IsLittleTroubleLevel() ||
		anApp->IsBungeeBlitzLevel() || anApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_SPEED;
	in.adventure = anApp->IsAdventureMode();
	in.level = in.adventure && anApp->mPlayerInfo != nullptr ? anApp->mPlayerInfo->GetLevel() : 0;
	in.gameMode = static_cast<int>(anApp->mGameMode);
	in.column = anApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_COLUMN;
	in.stormy = anApp->IsStormyNightLevel();
	in.puzzle = anApp->IsScaryPotterLevel() || anApp->IsIZombieLevel();
	in.night = aBoard->StageIsNight();
	in.pool = aBoard->StageHasPool();
	in.sixRows = aBoard->StageHas6Rows();
	in.roof = aBoard->StageHasRoof();
	const int aChoice = UnnamedMusicChoice();
	const int aTune = u::GameTune(in, aChoice);
	if (LogMusic())
		PvzpLogLn("PVZU: music game tune for mode {} level {} background {} (music choice {}): {} {}", in.gameMode,
			aBoard->mLevel, static_cast<int>(aBoard->mBackground), ChoiceName(aChoice), aTune,
			TuneName(static_cast<MusicTune>(aTune)));
	if (aTune < 0)
		theMusic.StopAllMusic(); // Stormy Night, as vanilla
	else
		theMusic.MakeSureMusicIsPlaying(static_cast<MusicTune>(aTune));
	return true;
}

void UnnamedMusicChoiceChanged(LawnApp* theApp) {
	if (theApp == nullptr || theApp->mMusic == nullptr || !UnnamedMusicOn())
		return;
	Music& aMusic = *theApp->mMusic;
	const MusicTune aTune = aMusic.mCurMusicTune;
	const bool aPaused = aMusic.mPaused;
	if (LogMusic())
		PvzpLogLn("PVZU: music choice now {} (playing tune {} {}, paused {})", ChoiceName(UnnamedMusicChoice()),
			static_cast<int>(aTune), TuneName(aTune), aPaused ? 1 : 0);
	if (aTune == MusicTune::MUSIC_TUNE_NONE || aTune == MusicTune::MUSIC_TUNE_CREDITS_ZOMBIES_ON_YOUR_LAWN)
		return; // nothing plays (or the credits song, never an mp3): the next tune uses the new choice
	aMusic.StopAllMusic();
	if (UnnamedMusicChoice() == u::musicchoice::ORIGINAL && gLoadedTrack != 0 && aMusic.mMusicInterface != nullptr) {
		aMusic.mMusicInterface->UnloadMusic(kSongIdBase + gLoadedTrack); // the decoded mp3 (tens of MB) is not needed
		gLoadedTrack = 0;
	}
	if (theApp->mBoard != nullptr && theApp->mGameScene == GameScenes::SCENE_PLAYING)
		aMusic.StartGameMusic(); // the lawn's tune (55 / 65 / 70 differ between the choices)
	else
		aMusic.PlayMusic(aTune, -1, -1); // title, chooser, shop, Zen: the same tune
	if (aPaused)
		aMusic.GameMusicPause(true);
}

void UnnamedMusicTick(Music& theMusic) {
	if (!LogMusic() || theMusic.mMusicInterface == nullptr || ++gTicks % 300 != 0)
		return;
	const int aFile = static_cast<int>(theMusic.mCurMusicFileMain);
	if (aFile == MusicFile::MUSIC_FILE_NONE)
		return;
	Sexy::MusicInterface* anInterface = theMusic.mMusicInterface;
	PvzpLogLn("PVZU: music {} ({}) decoded to {:.2f} s of {:.2f} s, playing {}", SongName(aFile),
		anInterface->GetSongDecoder(aFile), anInterface->GetSongPositionSeconds(aFile), anInterface->GetSongSeconds(aFile),
		anInterface->IsPlaying(aFile) ? "yes" : "no");
}

} // namespace pvzu::engine
