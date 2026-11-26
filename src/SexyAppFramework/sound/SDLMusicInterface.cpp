/*
 * Copyright (C) 2026 Zhou Qiankang <wszqkzqk@qq.com>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * This file is part of PvZ-Portable.
 *
 * PvZ-Portable is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PvZ-Portable is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with PvZ-Portable. If not, see <https://www.gnu.org/licenses/>.
 */

#include "SDLMusicInterface.h"
#include "SDLSoundManager.h"
#include "paklib/PakInterface.h"
#include <algorithm>
#include <vector>

using namespace Sexy;

SDLMusicInfo::SDLMusicInfo()
{
	mVolume = 0.0;
	mVolumeAdd = 0.0;
	mVolumeCap = 1.0;
	mStopOnFade = false;
}

static void ApplyMusicGain(const SDLMusicInfo& theMusic, float theGlobalVolume)
{
	MIX_SetTrackGain(theMusic.mTrack.get(), theMusic.mVolume * theGlobalVolume);
}

static bool InsertMusic(MIX_Mixer* theMixer, int theSongId, SDLMusicInfo&& theMusic, SDLMusicMap& theMap, float theGlobalVolume)
{
	theMusic.mTrack.reset(MIX_CreateTrack(theMixer));
	if (theMusic.mTrack == nullptr)
		return false;

	const bool aHasInput = theMusic.mOpenMpt != nullptr
		? MIX_SetTrackAudioStream(theMusic.mTrack.get(), theMusic.mOpenMpt->GetStream())
		: MIX_SetTrackAudio(theMusic.mTrack.get(), theMusic.mAudio.get());
	if (!aHasInput || !MIX_TagTrack(theMusic.mTrack.get(), "music"))
		return false;

	theMap.emplace(theSongId, std::move(theMusic));
	ApplyMusicGain(theMap.at(theSongId), theGlobalVolume);
	return true;
}

static bool ReadMusicFile(const std::string& theFileName, std::vector<uint8_t>& theData)
{
	std::unique_ptr<PFILE, decltype(&p_fclose)> aFile(p_fopen(theFileName.c_str(), "rb"), &p_fclose);
	if (aFile == nullptr)
		return false;

	p_fseek(aFile.get(), 0, SEEK_END);
	const size_t aSize = p_ftell(aFile.get());
	p_fseek(aFile.get(), 0, SEEK_SET);
	theData.resize(aSize);
	return p_fread(theData.data(), 1, aSize, aFile.get()) == aSize;
}

// PVZU: an mp3 decoded once to 16-bit PCM at the mixer's rate, then played as raw audio with a sample-accurate loop
// (Mod/MusicLoop.h: near-silent edges skipped, equal-power crossfade baked before the loop end). Streaming the file
// looped the whole mp3, silence included: 0.1 s to 9.6 s of silence at every loop point (pvz-unnamed-pack/SOURCES-music.md
// "Loop point"). Memory: 4 bytes a frame, 51.5 MB for the longest song (lawnbgm(9), 4:52); only one mp3 is loaded at a
// time (Mod/Engine/UnnamedMusic.cpp). MPG123 first: the only mp3 decoder in the Android build (vcpkg
// sdl3-mixer[mpg123]), so the Mac takes the same path; SDL_mixer's own choice if that decoder is missing. libmpg123
// applies the LAME/Info encoder delay and padding itself (lawnbgm(14) is the only file with that header).
namespace
{
constexpr double kLoopSilenceDbfs = -45.0; // peak level under which an edge counts as silent (ring-out end)
constexpr double kLoopMaxTrimSec = 10.0;   // at most this much is skipped at each end (lawnbgm(9) needs 8.8 s)
constexpr double kLoopCrossfadeSec = 0.030; // de-click fade at the loop point (both sides are near-silent)
}

static bool DecodeMp3(MIX_Mixer* theMixer, const std::string& theFileName, SDLMusicInfo& theMusic)
{
	SDL_AudioSpec aMixSpec = { SDL_AUDIO_F32, 2, 44100 };
	MIX_GetMixerFormat(theMixer, &aMixSpec);
	for (const char* aDecoder : {"MPG123", static_cast<const char*>(nullptr)})
	{
		SDL_PropertiesID aProps = SDL_CreateProperties();
		if (aProps == 0)
			return false;
		if (aDecoder != nullptr)
			SDL_SetStringProperty(aProps, MIX_PROP_AUDIO_DECODER_STRING, aDecoder);
		MIX_AudioDecoder* aDec = MIX_CreateAudioDecoder(theFileName.c_str(), aProps);
		SDL_DestroyProperties(aProps);
		if (aDec == nullptr)
			continue;

		SDL_AudioSpec aSrc = {};
		MIX_GetAudioDecoderFormat(aDec, &aSrc);
		const int aChannels = aSrc.channels == 1 ? 1 : 2;
		const SDL_AudioSpec aPcmSpec = { SDL_AUDIO_S16, aChannels, aMixSpec.freq };
		const Sint64 aSrcFrames = SDL_GetNumberProperty(MIX_GetAudioDecoderProperties(aDec), MIX_PROP_METADATA_DURATION_FRAMES_NUMBER, 0);
		std::vector<int16_t> aPcm;
		if (aSrcFrames > 0 && aSrc.freq > 0)
			aPcm.reserve(static_cast<size_t>((aSrcFrames * aPcmSpec.freq / aSrc.freq + aPcmSpec.freq) * aChannels));
		int16_t aChunk[8192];
		for (;;)
		{
			const int aBytes = MIX_DecodeAudio(aDec, aChunk, sizeof(aChunk), &aPcmSpec);
			if (aBytes <= 0)
				break;
			aPcm.insert(aPcm.end(), aChunk, aChunk + aBytes / sizeof(int16_t));
		}
		MIX_DestroyAudioDecoder(aDec);
		const int64_t aFrames = static_cast<int64_t>(aPcm.size()) / aChannels;
		if (aFrames == 0)
			continue;
		aPcm.resize(static_cast<size_t>(aFrames * aChannels)); // no shrink_to_fit: that copy would double the peak memory

		const pvzu::SoundEdges anEdges = pvzu::FindSoundEdges(aPcm.data(), aFrames, aChannels, pvzu::PeakThreshold(kLoopSilenceDbfs));
		const pvzu::MusicLoop aLoop = pvzu::ChooseMusicLoop(aFrames, anEdges, aPcmSpec.freq, kLoopMaxTrimSec, kLoopCrossfadeSec);
		pvzu::BakeLoopCrossfade(aPcm.data(), aChannels, aLoop);
		MIX_Audio* anAudio = MIX_LoadRawAudioNoCopy(theMixer, aPcm.data(), aPcm.size() * sizeof(int16_t), &aPcmSpec, false);
		if (anAudio == nullptr)
			return false;
		theMusic.mPcm = std::move(aPcm); // the buffer itself moves: anAudio keeps pointing at it
		theMusic.mAudio.reset(anAudio);
		theMusic.mLoop = aLoop;
		theMusic.mHasLoop = true;
		theMusic.mPcmRate = aPcmSpec.freq;
		theMusic.mDecoder = aDecoder != nullptr ? aDecoder : "";
		return true;
	}
	return false;
}

// PVZU: MIX_PlayTrack options for a song: loops, and for a decoded mp3 its loop (start, loop start, loop end).
static SDL_PropertiesID PlayOptions(const SDLMusicInfo& theMusic, bool noLoop, Sint64 theStartFrame)
{
	SDL_PropertiesID aOptions = SDL_CreateProperties();
	if (aOptions == 0)
		return 0;
	SDL_SetNumberProperty(aOptions, MIX_PROP_PLAY_LOOPS_NUMBER, noLoop ? 0 : -1);
	if (theMusic.mHasLoop)
	{
		SDL_SetNumberProperty(aOptions, MIX_PROP_PLAY_START_FRAME_NUMBER, theStartFrame >= 0 ? theStartFrame : theMusic.mLoop.playStart);
		SDL_SetNumberProperty(aOptions, MIX_PROP_PLAY_LOOP_START_FRAME_NUMBER, theMusic.mLoop.loopStart);
		SDL_SetNumberProperty(aOptions, MIX_PROP_PLAY_MAX_FRAME_NUMBER, theMusic.mLoop.loopEnd);
	}
	return aOptions;
}

SDLMusicInterface::SDLMusicInterface(SDLSoundManager* theSoundManager)
{
	mMixer = theSoundManager->GetMixer();
	mGlobalVolume = 1.0f;
}

SDLMusicInterface::~SDLMusicInterface()
{
	UnloadAllMusic();
}

bool SDLMusicInterface::LoadMusic(int theSongId, const std::string& theFileName)
{
	std::scoped_lock anAutoCrit(mMusicMapMutex);
	if (mMixer == nullptr || mMusicMap.contains(theSongId))
		return mMixer != nullptr;

	std::string anExt;
	const size_t aDotPos = theFileName.find_last_of('.');
	if (aDotPos != std::string::npos)
		anExt = StringToLower(theFileName.substr(aDotPos + 1));

	if (anExt == "mo3")
	{
		std::vector<uint8_t> aData;
		return ReadMusicFile(theFileName, aData) && LoadMusicFromMemory(theSongId, aData.data(), aData.size(), anExt);
	}

	SDLMusicInfo aMusicInfo;
	if (anExt == "mp3") // PVZU: Unnamed's lawnbgm(N).mp3 (only Unnamed mode loads mp3)
		DecodeMp3(mMixer, theFileName, aMusicInfo);
	else
		aMusicInfo.mAudio.reset(MIX_LoadAudio(mMixer, theFileName.c_str(), false));
	if (aMusicInfo.mAudio == nullptr)
		return false;
	return InsertMusic(mMixer, theSongId, std::move(aMusicInfo), mMusicMap, mGlobalVolume);
}

bool SDLMusicInterface::LoadMusicFromMemory(int theSongId, const void* theData, size_t theSize, std::string_view theExtension)
{
	std::scoped_lock anAutoCrit(mMusicMapMutex);
	if (mMixer == nullptr || theData == nullptr || theSize == 0)
		return false;
	if (mMusicMap.contains(theSongId))
		return true;

	SDLMusicInfo aMusicInfo;
	if (StringToLower(std::string(theExtension)) == "mo3")
	{
		SDL_AudioSpec aSpec = {};
		int aSampleRate = 44100;
		if (MIX_GetMixerFormat(mMixer, &aSpec))
			aSampleRate = aSpec.freq;

		aMusicInfo.mOpenMpt = std::make_unique<OpenMptMusic>();
		if (!aMusicInfo.mOpenMpt->Load(theData, theSize, aSampleRate))
			return false;
	}
	else
	{
		aMusicInfo.mAudio.reset(MIX_LoadAudio_IO(
			mMixer, SDL_IOFromConstMem(theData, theSize), false, true));
		if (aMusicInfo.mAudio == nullptr)
			return false;
	}

	return InsertMusic(mMixer, theSongId, std::move(aMusicInfo), mMusicMap, mGlobalVolume);
}

bool SDLMusicInterface::StartMusic(SDLMusicInfo& theMusic, int theOffset, bool noLoop)
{
	MIX_LockMixer(mMixer);
	MIX_StopTrack(theMusic.mTrack.get(), 0);
	if (theMusic.mOpenMpt != nullptr)
	{
		theMusic.mOpenMpt->SetLooping(!noLoop);
		theMusic.mOpenMpt->Rewind(theOffset);
	}

	SDL_PropertiesID aOptions = PlayOptions(theMusic, noLoop, -1); // PVZU: mp3 loop points
	if (aOptions == 0)
	{
		MIX_UnlockMixer(mMixer);
		return false;
	}
	const bool aPlayed = MIX_PlayTrack(theMusic.mTrack.get(), aOptions);
	SDL_DestroyProperties(aOptions);
	MIX_UnlockMixer(mMixer);
	return aPlayed;
}

void SDLMusicInterface::PlayMusic(int theSongId, int theOffset, bool noLoop)
{
	std::scoped_lock anAutoCrit(mMusicMapMutex);
	auto anItr = mMusicMap.find(theSongId);
	if (anItr == mMusicMap.end())
		return;

	SDLMusicInfo& aMusicInfo = anItr->second;
	aMusicInfo.mVolume = aMusicInfo.mVolumeCap;
	aMusicInfo.mVolumeAdd = 0.0;
	aMusicInfo.mStopOnFade = noLoop;
	ApplyMusicGain(aMusicInfo, mGlobalVolume);
	StartMusic(aMusicInfo, theOffset, noLoop);
}

void SDLMusicInterface::StopMusic(int theSongId)
{
	std::scoped_lock anAutoCrit(mMusicMapMutex);
	auto anItr = mMusicMap.find(theSongId);
	if (anItr != mMusicMap.end())
	{
		anItr->second.mVolume = 0.0;
		MIX_StopTrack(anItr->second.mTrack.get(), 0);
	}
}

void SDLMusicInterface::PauseMusic(int theSongId)
{
	std::scoped_lock anAutoCrit(mMusicMapMutex);
	auto anItr = mMusicMap.find(theSongId);
	if (anItr != mMusicMap.end())
		MIX_PauseTrack(anItr->second.mTrack.get());
}

void SDLMusicInterface::ResumeMusic(int theSongId)
{
	std::scoped_lock anAutoCrit(mMusicMapMutex);
	auto anItr = mMusicMap.find(theSongId);
	if (anItr != mMusicMap.end())
		MIX_ResumeTrack(anItr->second.mTrack.get());
}

void SDLMusicInterface::StopAllMusic()
{
	std::scoped_lock anAutoCrit(mMusicMapMutex);
	for (auto& [aId, aMusicInfo] : mMusicMap)
	{
		aMusicInfo.mVolume = 0.0;
		MIX_StopTrack(aMusicInfo.mTrack.get(), 0);
	}
}

void SDLMusicInterface::UnloadMusic(int theSongId)
{
	std::scoped_lock anAutoCrit(mMusicMapMutex);
	StopMusic(theSongId);
	mMusicMap.erase(theSongId);
}

void SDLMusicInterface::UnloadAllMusic()
{
	std::scoped_lock anAutoCrit(mMusicMapMutex);
	StopAllMusic();
	mMusicMap.clear();
}

void SDLMusicInterface::PauseAllMusic()
{
	std::scoped_lock anAutoCrit(mMusicMapMutex);
	for (auto& [aId, aMusicInfo] : mMusicMap)
		if (MIX_TrackPlaying(aMusicInfo.mTrack.get()))
			MIX_PauseTrack(aMusicInfo.mTrack.get());
}

void SDLMusicInterface::ResumeAllMusic()
{
	std::scoped_lock anAutoCrit(mMusicMapMutex);
	for (auto& [aId, aMusicInfo] : mMusicMap)
		if (MIX_TrackPaused(aMusicInfo.mTrack.get()))
			MIX_ResumeTrack(aMusicInfo.mTrack.get());
}

void SDLMusicInterface::FadeIn(int theSongId, int theOffset, double theSpeed, bool noLoop)
{
	std::scoped_lock anAutoCrit(mMusicMapMutex);
	auto anItr = mMusicMap.find(theSongId);
	if (anItr == mMusicMap.end())
		return;

	SDLMusicInfo& aMusicInfo = anItr->second;
	aMusicInfo.mVolumeAdd = theSpeed;
	aMusicInfo.mStopOnFade = noLoop;
	StartMusic(aMusicInfo, theOffset, noLoop);
}

void SDLMusicInterface::FadeOut(int theSongId, bool stopSong, double theSpeed)
{
	std::scoped_lock anAutoCrit(mMusicMapMutex);
	auto anItr = mMusicMap.find(theSongId);
	if (anItr != mMusicMap.end())
	{
		SDLMusicInfo& aMusicInfo = anItr->second;
		if (aMusicInfo.mVolume != 0.0)
			aMusicInfo.mVolumeAdd = -theSpeed;
		aMusicInfo.mStopOnFade = stopSong;
	}
}

void SDLMusicInterface::FadeOutAll(bool stopSong, double theSpeed)
{
	std::scoped_lock anAutoCrit(mMusicMapMutex);
	for (auto& [aId, aMusicInfo] : mMusicMap)
	{
		aMusicInfo.mVolumeAdd = -theSpeed;
		aMusicInfo.mStopOnFade = stopSong;
	}
}

void SDLMusicInterface::SetSongVolume(int theSongId, double theVolume)
{
	std::scoped_lock anAutoCrit(mMusicMapMutex);
	auto anItr = mMusicMap.find(theSongId);
	if (anItr != mMusicMap.end())
	{
		anItr->second.mVolume = theVolume;
		ApplyMusicGain(anItr->second, mGlobalVolume);
	}
}

void SDLMusicInterface::SetSongMaxVolume(int theSongId, double theMaxVolume)
{
	std::scoped_lock anAutoCrit(mMusicMapMutex);
	auto anItr = mMusicMap.find(theSongId);
	if (anItr != mMusicMap.end())
	{
		SDLMusicInfo& aMusicInfo = anItr->second;
		aMusicInfo.mVolumeCap = theMaxVolume;
		aMusicInfo.mVolume = std::min(aMusicInfo.mVolume, theMaxVolume);
		ApplyMusicGain(aMusicInfo, mGlobalVolume);
	}
}

bool SDLMusicInterface::IsPlaying(int theSongId)
{
	std::scoped_lock anAutoCrit(mMusicMapMutex);
	auto anItr = mMusicMap.find(theSongId);
	return anItr != mMusicMap.end() &&
		(MIX_TrackPlaying(anItr->second.mTrack.get()) || MIX_TrackPaused(anItr->second.mTrack.get()));
}

void SDLMusicInterface::SetVolume(double theVolume)
{
	std::scoped_lock anAutoCrit(mMusicMapMutex);
	mGlobalVolume = static_cast<float>(theVolume);
	for (auto& [aId, aMusicInfo] : mMusicMap)
		ApplyMusicGain(aMusicInfo, mGlobalVolume);
}

void SDLMusicInterface::SetMusicAmplify(int, double)
{
}

void SDLMusicInterface::Update()
{
	std::scoped_lock anAutoCrit(mMusicMapMutex);
	for (auto& [aId, aMusicInfo] : mMusicMap)
	{
		if (aMusicInfo.mVolumeAdd == 0.0)
			continue;

		aMusicInfo.mVolume += aMusicInfo.mVolumeAdd;
		if (aMusicInfo.mVolume > aMusicInfo.mVolumeCap)
		{
			aMusicInfo.mVolume = aMusicInfo.mVolumeCap;
			aMusicInfo.mVolumeAdd = 0.0;
		}
		else if (aMusicInfo.mVolume < 0.0)
		{
			aMusicInfo.mVolume = 0.0;
			aMusicInfo.mVolumeAdd = 0.0;
			if (aMusicInfo.mStopOnFade)
				MIX_StopTrack(aMusicInfo.mTrack.get(), 0);
		}
		ApplyMusicGain(aMusicInfo, mGlobalVolume);
	}
}

int SDLMusicInterface::GetMusicOrder(int theSongId)
{
	std::scoped_lock anAutoCrit(mMusicMapMutex);
	auto anItr = mMusicMap.find(theSongId);
	if (anItr == mMusicMap.end() || anItr->second.mOpenMpt == nullptr)
		return -1;

	MIX_LockMixer(mMixer);
	const int aPosition = anItr->second.mOpenMpt->GetOrder();
	MIX_UnlockMixer(mMixer);
	return aPosition;
}

bool SDLMusicInterface::JumpToOrder(int theSongId, int thePackedOrder)
{
	std::scoped_lock anAutoCrit(mMusicMapMutex);
	auto anItr = mMusicMap.find(theSongId);
	if (anItr == mMusicMap.end() || anItr->second.mOpenMpt == nullptr)
		return false;

	MIX_LockMixer(mMixer);
	anItr->second.mOpenMpt->JumpToOrder(thePackedOrder);
	MIX_UnlockMixer(mMixer);
	return true;
}

bool SDLMusicInterface::SetChannelVolume(int theSongId, int theChannel, double theVolume)
{
	std::scoped_lock anAutoCrit(mMusicMapMutex);
	auto anItr = mMusicMap.find(theSongId);
	if (anItr == mMusicMap.end() || anItr->second.mOpenMpt == nullptr)
		return false;

	MIX_LockMixer(mMixer);
	anItr->second.mOpenMpt->SetChannelVolume(theChannel, theVolume);
	MIX_UnlockMixer(mMixer);
	return true;
}

// PVZU: -pvzu-log-music (what a loaded song is and how far the mixer has decoded it)
double SDLMusicInterface::GetSongSeconds(int theSongId)
{
	std::scoped_lock anAutoCrit(mMusicMapMutex);
	auto anItr = mMusicMap.find(theSongId);
	if (anItr == mMusicMap.end() || anItr->second.mAudio == nullptr)
		return -1.0;
	const Sint64 aFrames = MIX_GetAudioDuration(anItr->second.mAudio.get());
	return aFrames < 0 ? -1.0 : MIX_AudioFramesToMS(anItr->second.mAudio.get(), aFrames) / 1000.0;
}

double SDLMusicInterface::GetSongPositionSeconds(int theSongId)
{
	std::scoped_lock anAutoCrit(mMusicMapMutex);
	auto anItr = mMusicMap.find(theSongId);
	if (anItr == mMusicMap.end() || anItr->second.mTrack == nullptr)
		return -1.0;
	const Sint64 aFrames = MIX_GetTrackPlaybackPosition(anItr->second.mTrack.get());
	return aFrames < 0 ? -1.0 : MIX_TrackFramesToMS(anItr->second.mTrack.get(), aFrames) / 1000.0;
}

std::string SDLMusicInterface::GetSongDecoder(int theSongId)
{
	std::scoped_lock anAutoCrit(mMusicMapMutex);
	auto anItr = mMusicMap.find(theSongId);
	if (anItr == mMusicMap.end())
		return {};
	if (anItr->second.mOpenMpt != nullptr)
		return "libopenmpt";
	const SDLMusicInfo& aMusic = anItr->second;
	std::string aName = aMusic.mDecoder.empty() ? "auto" : aMusic.mDecoder;
	if (aMusic.mHasLoop)
	{
		const double aRate = aMusic.mPcmRate;
		char aLine[160];
		SDL_snprintf(aLine, sizeof(aLine), " -> PCM %d Hz, %.1f MB, loop %.3f-%.3f s, crossfade %.0f ms", aMusic.mPcmRate,
			aMusic.mPcm.size() * sizeof(int16_t) / 1e6, aMusic.mLoop.loopStart / aRate, aMusic.mLoop.loopEnd / aRate,
			aMusic.mLoop.crossfade * 1000.0 / aRate);
		aName += aLine;
	}
	return aName;
}

// PVZU: -pvzu-music-seams (dev): the loop point as the game plays it, rendered offline to a float WAV.
bool SDLMusicInterface::RenderLoopSeam(int theSongId, const std::string& theWavPath)
{
	std::scoped_lock anAutoCrit(mMusicMapMutex);
	auto anItr = mMusicMap.find(theSongId);
	if (anItr == mMusicMap.end() || !anItr->second.mHasLoop)
		return false;
	const SDLMusicInfo& aMusic = anItr->second;
	const int aRate = aMusic.mPcmRate;
	const int aChannels = 2;
	const SDL_AudioSpec anOutSpec = { SDL_AUDIO_F32, aChannels, aRate };
	MIX_Mixer* anOffline = MIX_CreateMixer(&anOutSpec);
	if (anOffline == nullptr)
		return false;
	SDL_AudioSpec aPcmSpec = {};
	MIX_GetAudioFormat(aMusic.mAudio.get(), &aPcmSpec);
	MIX_Audio* anAudio = MIX_LoadRawAudioNoCopy(anOffline, aMusic.mPcm.data(), aMusic.mPcm.size() * sizeof(int16_t), &aPcmSpec, false);
	MIX_Track* aTrack = anAudio != nullptr ? MIX_CreateTrack(anOffline) : nullptr;
	bool anOk = aTrack != nullptr && MIX_SetTrackAudio(aTrack, anAudio);
	const Sint64 aBefore = std::min<Sint64>(2 * aRate, aMusic.mLoop.loopEnd - aMusic.mLoop.playStart);
	const SDL_PropertiesID anOptions = anOk ? PlayOptions(aMusic, false, aMusic.mLoop.loopEnd - aBefore) : 0;
	anOk = anOptions != 0 && MIX_PlayTrack(aTrack, anOptions);
	if (anOptions != 0)
		SDL_DestroyProperties(anOptions);
	std::vector<float> aBuf(static_cast<size_t>((aBefore + 2 * aRate) * aChannels));
	if (anOk)
		anOk = MIX_Generate(anOffline, aBuf.data(), static_cast<int>(aBuf.size() * sizeof(float))) > 0;
	if (aTrack != nullptr)
		MIX_DestroyTrack(aTrack);
	if (anAudio != nullptr)
		MIX_DestroyAudio(anAudio);
	MIX_DestroyMixer(anOffline);
	if (!anOk)
		return false;

	SDL_IOStream* anIo = SDL_IOFromFile(theWavPath.c_str(), "wb");
	if (anIo == nullptr)
		return false;
	const Uint32 aBytes = static_cast<Uint32>(aBuf.size() * sizeof(float));
	bool aWrote = SDL_WriteIO(anIo, "RIFF", 4) == 4 && SDL_WriteU32LE(anIo, 36 + aBytes) && SDL_WriteIO(anIo, "WAVEfmt ", 8) == 8 &&
		SDL_WriteU32LE(anIo, 16) && SDL_WriteU16LE(anIo, 3) && SDL_WriteU16LE(anIo, aChannels) && SDL_WriteU32LE(anIo, aRate) &&
		SDL_WriteU32LE(anIo, aRate * aChannels * 4) && SDL_WriteU16LE(anIo, aChannels * 4) && SDL_WriteU16LE(anIo, 32) &&
		SDL_WriteIO(anIo, "data", 4) == 4 && SDL_WriteU32LE(anIo, aBytes) && SDL_WriteIO(anIo, aBuf.data(), aBytes) == aBytes;
	return SDL_CloseIO(anIo) && aWrote;
}
