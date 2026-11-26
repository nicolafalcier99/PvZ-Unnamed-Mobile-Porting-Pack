// PVZU: seamless loop for the Unnamed mp3s (Mod/MusicLoop.h). Evidence for the numbers: every lawnbgm(N).mp3 ends in a
// ring-out plus digital silence and starts after silence (0.1 s to 9.6 s of silence at the loop point, measured with
// mpg123 and SDL_mixer's MPG123 decoder; pvz-unnamed-pack/SOURCES-music.md "Loop point").
#include "doctest/doctest.h"
#include "Mod/MusicLoop.h"

#include <cmath>
#include <cstdint>
#include <vector>

using namespace pvzu;

namespace {
// Stereo PCM: silence, then frames [theFirst, theEnd) at theLevel, then silence.
std::vector<int16_t> Burst(int64_t theFrames, int64_t theFirst, int64_t theEnd, int16_t theLevel) {
	std::vector<int16_t> aPcm(static_cast<size_t>(theFrames * 2), 0);
	for (int64_t i = theFirst; i < theEnd; ++i)
		aPcm[i * 2] = aPcm[i * 2 + 1] = theLevel;
	return aPcm;
}
} // namespace

TEST_CASE("MusicLoopThresholdFromDbfs") {
	CHECK(PeakThreshold(0.0) == 32767);
	CHECK(PeakThreshold(-45.0) == 184); // 32768 * 10^(-45/20) = 184.3
	CHECK(PeakThreshold(-60.0) == 33);
	CHECK(PeakThreshold(-200.0) == 0);
}

TEST_CASE("MusicLoopFindsTheFramesLouderThanTheThreshold") {
	auto aPcm = Burst(1000, 120, 870, 500);
	aPcm[50 * 2 + 1] = 100;  // quieter than the threshold: still silence
	aPcm[900 * 2] = -150;    // as is a quiet negative sample
	const SoundEdges anEdges = FindSoundEdges(aPcm.data(), 1000, 2, 184);
	CHECK(anEdges.first == 120);
	CHECK(anEdges.end == 870);
}

TEST_CASE("MusicLoopEdgesSeeEitherChannelAndNegativePeaks") {
	std::vector<int16_t> aPcm(20, 0); // 10 stereo frames
	aPcm[3 * 2 + 1] = -300;           // right channel only, negative
	aPcm[7 * 2] = 200;
	const SoundEdges anEdges = FindSoundEdges(aPcm.data(), 10, 2, 184);
	CHECK(anEdges.first == 3);
	CHECK(anEdges.end == 8);
}

TEST_CASE("MusicLoopAllSilentGivesNoEdges") {
	const auto aPcm = Burst(500, 0, 0, 0);
	const SoundEdges anEdges = FindSoundEdges(aPcm.data(), 500, 2, 184);
	CHECK(anEdges.first == 0);
	CHECK(anEdges.end == 0);
}

TEST_CASE("MusicLoopTrimsSilentEdgesAndCrossfadesOverThePreRoll") {
	// 44100 Hz, sound from 0.5 s to 9 s of a 10 s file, 30 ms crossfade (1323 frames) from the pre-roll before 0.5 s.
	const MusicLoop aLoop = ChooseMusicLoop(441000, SoundEdges{22050, 396900}, 44100, 10.0, 0.030);
	CHECK(aLoop.playStart == 22050);
	CHECK(aLoop.loopStart == 22050);
	CHECK(aLoop.loopEnd == 396900);
	CHECK(aLoop.crossfade == 1323);
}

TEST_CASE("MusicLoopTrimIsBounded") {
	// 12 s of silence at each end, trims bounded to 10 s: start at 10 s, end 10 s before the end.
	const int64_t aFrames = 44100 * 60;
	const MusicLoop aLoop = ChooseMusicLoop(aFrames, SoundEdges{44100 * 12, aFrames - 44100 * 12}, 44100, 10.0, 0.030);
	CHECK(aLoop.playStart == 441000);
	CHECK(aLoop.loopEnd == aFrames - 441000);
}

TEST_CASE("MusicLoopWithoutPreRollCrossfadesIntoTheHead") {
	// Sound from frame 100: the loop restarts after the crossfade, which takes the head [0, 1323).
	const MusicLoop aLoop = ChooseMusicLoop(441000, SoundEdges{100, 441000}, 44100, 10.0, 0.030);
	CHECK(aLoop.playStart == 100);
	CHECK(aLoop.loopStart == 1323);
	CHECK(aLoop.loopEnd == 441000);
	CHECK(aLoop.crossfade == 1323);
}

TEST_CASE("MusicLoopSilentOrTinySongsLoopWhole") {
	const MusicLoop aSilent = ChooseMusicLoop(1000, SoundEdges{0, 0}, 44100, 10.0, 0.030);
	CHECK(aSilent.playStart == 0);
	CHECK(aSilent.loopStart == 0);
	CHECK(aSilent.loopEnd == 1000);
	CHECK(aSilent.crossfade == 0);
	// 100 frames of sound: the crossfade is at most half the loop.
	const MusicLoop aTiny = ChooseMusicLoop(1000, SoundEdges{400, 500}, 44100, 10.0, 0.030);
	CHECK(aTiny.playStart == 400);
	CHECK(aTiny.loopEnd == 500);
	CHECK(aTiny.crossfade == 50);
	CHECK(aTiny.loopStart == 400);
}

TEST_CASE("MusicLoopEqualPowerGainsKeepThePower") {
	const auto [aOut0, aIn0] = EqualPowerGains(0, 100);
	CHECK(aOut0 > 0.99);
	CHECK(aIn0 < 0.02);
	const auto [aOutN, aInN] = EqualPowerGains(99, 100);
	CHECK(aOutN < 0.02);
	CHECK(aInN > 0.99);
	for (int i = 0; i < 100; ++i) {
		const auto [aOut, aIn] = EqualPowerGains(i, 100);
		CHECK(aOut * aOut + aIn * aIn == doctest::Approx(1.0));
	}
	const auto [aOutMid, aInMid] = EqualPowerGains(50, 101); // centre: both -3 dB
	CHECK(aOutMid == doctest::Approx(std::sqrt(0.5)));
	CHECK(aInMid == doctest::Approx(std::sqrt(0.5)));
}

TEST_CASE("MusicLoopBakedSeamIsContinuous") {
	// A sine that does not line up at the cut: tail [loopEnd - C, loopEnd) gets the pre-roll [loopStart - C, loopStart)
	// faded in, so the frame before loopEnd is (almost) the frame before loopStart and the jump has no step.
	const int64_t aFrames = 4000;
	std::vector<int16_t> aPcm(aFrames * 2);
	for (int64_t i = 0; i < aFrames; ++i)
		aPcm[i * 2] = aPcm[i * 2 + 1] = static_cast<int16_t>(std::lround(10000.0 * std::sin(i * 0.05)));
	const MusicLoop aLoop{500, 500, 3300, 200};
	const std::vector<int16_t> anOriginal = aPcm;
	BakeLoopCrossfade(aPcm.data(), 2, aLoop);
	// Outside the crossfade nothing changes.
	for (int64_t i = 0; i < aLoop.loopEnd - aLoop.crossfade; ++i)
		REQUIRE(aPcm[i * 2] == anOriginal[i * 2]);
	for (int64_t i = aLoop.loopEnd; i < aFrames; ++i)
		REQUIRE(aPcm[i * 2] == anOriginal[i * 2]);
	// The seam: last baked frame -> loopStart is as smooth as the sine itself (step <= 10000 * 0.05 = 500, plus the
	// 0.4 % of tail still in the last frame).
	const int aStep = std::abs(aPcm[(aLoop.loopEnd - 1) * 2] - anOriginal[aLoop.loopStart * 2]);
	CHECK(aStep <= 560);
	// Unbaked the same jump is a click.
	CHECK(std::abs(anOriginal[(aLoop.loopEnd - 1) * 2] - anOriginal[aLoop.loopStart * 2]) > 2000);
	// The first baked frame still is (almost) the tail.
	CHECK(std::abs(aPcm[(aLoop.loopEnd - aLoop.crossfade) * 2] - anOriginal[(aLoop.loopEnd - aLoop.crossfade) * 2]) <= 560);
}

TEST_CASE("MusicLoopBakeClampsAndSkipsEmptyCrossfade") {
	std::vector<int16_t> aPcm(2000, 30000); // 1000 frames, loud, in phase: equal-power sum overshoots and clamps
	BakeLoopCrossfade(aPcm.data(), 2, MusicLoop{0, 400, 1000, 200});
	CHECK(aPcm[(900) * 2] == 32767);
	std::vector<int16_t> aSame(2000, 7);
	BakeLoopCrossfade(aSame.data(), 2, MusicLoop{0, 0, 1000, 0});
	CHECK(aSame == std::vector<int16_t>(2000, 7));
}
