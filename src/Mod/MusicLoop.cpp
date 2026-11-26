#include "Mod/MusicLoop.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <numbers>

namespace pvzu {

int PeakThreshold(double theDbfs) {
	const double anAmplitude = 32768.0 * std::pow(10.0, theDbfs / 20.0);
	return static_cast<int>(std::min(32767L, std::lround(anAmplitude)));
}

namespace {
bool Loud(const int16_t* theFrame, int theChannels, int theThreshold) {
	for (int c = 0; c < theChannels; ++c)
		if (std::abs(static_cast<int>(theFrame[c])) > theThreshold)
			return true;
	return false;
}
} // namespace

SoundEdges FindSoundEdges(const int16_t* thePcm, int64_t theFrames, int theChannels, int theThreshold) {
	int64_t aFirst = 0;
	while (aFirst < theFrames && !Loud(thePcm + aFirst * theChannels, theChannels, theThreshold))
		++aFirst;
	if (aFirst == theFrames)
		return {};
	int64_t anEnd = theFrames;
	while (anEnd > aFirst && !Loud(thePcm + (anEnd - 1) * theChannels, theChannels, theThreshold))
		--anEnd;
	return {aFirst, anEnd};
}

MusicLoop ChooseMusicLoop(int64_t theFrames, SoundEdges theEdges, int theRate, double theMaxTrimSec, double theCrossfadeSec) {
	if (theEdges.end <= theEdges.first)
		return {0, 0, theFrames, 0};
	const int64_t aMaxTrim = static_cast<int64_t>(std::llround(theMaxTrimSec * theRate));
	MusicLoop aLoop;
	aLoop.playStart = std::min(theEdges.first, aMaxTrim);
	aLoop.loopEnd = std::max(theEdges.end, theFrames - aMaxTrim);
	const int64_t aWanted = static_cast<int64_t>(std::llround(theCrossfadeSec * theRate));
	aLoop.crossfade = std::min(aWanted, (aLoop.loopEnd - aLoop.playStart) / 2);
	aLoop.loopStart = std::max(aLoop.playStart, aLoop.crossfade);
	return aLoop;
}

std::pair<double, double> EqualPowerGains(int64_t theIndex, int64_t theLength) {
	const double anAngle = (static_cast<double>(theIndex) + 0.5) / static_cast<double>(theLength) * std::numbers::pi / 2.0;
	return {std::cos(anAngle), std::sin(anAngle)};
}

void BakeLoopCrossfade(int16_t* thePcm, int theChannels, const MusicLoop& theLoop) {
	const int64_t aLength = theLoop.crossfade;
	const int64_t aTail = theLoop.loopEnd - aLength;
	const int64_t aHead = theLoop.loopStart - aLength;
	for (int64_t i = 0; i < aLength; ++i) {
		const auto [anOut, anIn] = EqualPowerGains(i, aLength);
		for (int c = 0; c < theChannels; ++c) {
			int16_t& aSample = thePcm[(aTail + i) * theChannels + c];
			const double aMix = aSample * anOut + thePcm[(aHead + i) * theChannels + c] * anIn;
			aSample = static_cast<int16_t>(std::clamp(std::lround(aMix), -32768L, 32767L));
		}
	}
}

} // namespace pvzu
