// PVZU: seamless loop for a song decoded to PCM (the Unnamed lawnbgm(N).mp3s, Sexy::SDLMusicInterface::LoadMusic).
// Every mod mp3 ends in a ring-out plus digital silence and starts after silence, so looping the whole file plays
// 0.1 s to 9.6 s of silence at the loop point (feedback: "it stops and starts"). The loop skips the near-silent edges and
// bakes a short equal-power crossfade into the frames before the loop end, so the mixer can jump sample-accurately.
// Evidence and per-song numbers: pvz-unnamed-pack/SOURCES-music.md "Loop point". Pure functions; tests/test_music_loop.cpp.
#pragma once

#include <cstdint>
#include <utility>

namespace pvzu {

// Sample amplitude (int16 scale) for a peak level in dBFS: round(32768 * 10^(dB/20)), at most 32767.
int PeakThreshold(double theDbfs);

// [first, end): the first frame and one past the last frame where any channel's |sample| > theThreshold.
// {0, 0} when no frame is louder.
struct SoundEdges {
	int64_t first = 0;
	int64_t end = 0;
};
SoundEdges FindSoundEdges(const int16_t* thePcm, int64_t theFrames, int theChannels, int theThreshold);

// Play from playStart; at loopEnd go on at loopStart. The crossfade frames before loopEnd hold the tail fading out
// and the frames before loopStart ([loopStart - crossfade, loopStart)) fading in (BakeLoopCrossfade), so the jump to
// loopStart continues what was fading in.
struct MusicLoop {
	int64_t playStart = 0;
	int64_t loopStart = 0;
	int64_t loopEnd = 0;
	int64_t crossfade = 0;
};
// Loop for a song of theFrames frames whose sound spans theEdges: each edge trimmed by at most theMaxTrimSec; the
// crossfade is theCrossfadeSec long but at most half the sounding part. When there is less pre-roll than the
// crossfade, the loop goes on after the crossfade (loopStart = crossfade) and the fade takes the head instead.
// No sound (silent song): the whole file loops, no crossfade.
MusicLoop ChooseMusicLoop(int64_t theFrames, SoundEdges theEdges, int theRate, double theMaxTrimSec, double theCrossfadeSec);

// Equal-power crossfade gains for frame theIndex of theLength: {out, in} = {cos, sin} of (theIndex + 0.5) / theLength
// * pi/2, so out^2 + in^2 = 1 (no level dip for uncorrelated music).
std::pair<double, double> EqualPowerGains(int64_t theIndex, int64_t theLength);

// Writes the crossfade of theLoop into interleaved PCM (frames [loopEnd - crossfade, loopEnd)), clamped to int16.
// Needs loopStart >= crossfade and loopEnd - loopStart >= crossfade (ChooseMusicLoop guarantees both).
void BakeLoopCrossfade(int16_t* thePcm, int theChannels, const MusicLoop& theLoop);

} // namespace pvzu
