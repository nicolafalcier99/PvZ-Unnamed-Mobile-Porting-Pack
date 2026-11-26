#include "Mod/StreetFog.h"

#include <algorithm>
#include <cmath>

namespace pvzu {

StreetFogRamp ComputeStreetFogRamp(int visibleRightBoardX, int minSpawnX) {
	StreetFogRamp r;
	if (visibleRightBoardX <= kVanillaWidth)
		return r;
	r.active = true;
	r.clearX = static_cast<float>(kVanillaWidth);
	const int aOpaque = std::max(minSpawnX + kZombieArtLeft, kVanillaWidth + kStreetFogMinRamp);
	r.opaqueX = static_cast<float>(std::min(aOpaque, visibleRightBoardX));
	return r;
}

float StreetFogAlpha(float boardX, const StreetFogRamp& ramp) {
	if (!ramp.active || boardX <= ramp.clearX)
		return 0.0f;
	if (boardX >= ramp.opaqueX)
		return 1.0f;
	const float t = (boardX - ramp.clearX) / (ramp.opaqueX - ramp.clearX);
	return t * t * (3.0f - 2.0f * t);
}

float StreetFogWashAlpha(float boardX, const StreetFogRamp& ramp) {
	const float a = StreetFogAlpha(boardX, ramp);
	return a * a;
}

namespace {
float CurveS(float u) {
	return 3.0f * u * u - 2.0f * u * u * u; // PvzpCurveS
}
float EaseInOut(float u) {
	return CurveS(CurveS(u)); // vanilla CURVE_EASE_IN_OUT (CutScene::CalcPosition, the camera's pan)
}
float EaseOut(float u) {
	return 2.0f * u - u * u; // PvzpCurveInvQuad: vanilla CURVE_EASE_OUT (the fog roll-in)
}
} // namespace

EdgeFogState StepEdgeFog(EdgeFogState s, const EdgeFogInput& in) {
	if (!s.started || (in.wanted && in.zombies)) {
		// The level's first moment (or a resumed game), or zombies that must stay hidden: no move, just be there.
		const bool aIn = in.wanted;
		s.started = true;
		s.in = aIn;
		s.out = s.from = aIn ? 0.0f : 1.0f;
		s.ticks = aIn ? kEdgeFogInTicks : kEdgeFogOutTicks;
		s.followRoll = false;
		return s;
	}
	// Fog level: back at rest before vanilla's fog starts rolling in, the cover waits out of sight and comes with it.
	const bool aWantIn = in.wanted && !(in.waitForRoll && s.out > 0.0f);
	if (aWantIn != s.in) {
		s.in = aWantIn;
		s.from = s.out;
		s.ticks = 0;
	}
	const float aDistance = s.in ? s.from : 1.0f - s.from;
	const int aDuration = std::max(1, static_cast<int>(std::ceil((s.in ? kEdgeFogInTicks : kEdgeFogOutTicks) * aDistance)));
	s.ticks = std::min(s.ticks + 1, aDuration);
	const float u = static_cast<float>(s.ticks) / static_cast<float>(aDuration);
	s.out = s.in ? (s.ticks >= aDuration ? 0.0f : s.from * (1.0f - EaseOut(u)))
				 : (s.ticks >= aDuration ? 1.0f : s.from + (1.0f - s.from) * EaseInOut(u));
	s.followRoll = s.in && in.rollActive;
	return s;
}

float EdgeFogShift(const EdgeFogState& s, float hiddenShift, float vanillaFogOffset) {
	const float aOwn = s.out * hiddenShift;
	return s.followRoll ? std::max(aOwn, vanillaFogOffset) : aOwn;
}

float EdgeFogHiddenShift(const StreetFogRamp& ramp, int screenRightBoardX) {
	return std::max(static_cast<float>(screenRightBoardX) + kEdgeFogHiddenMargin - ramp.clearX, 0.0f);
}

float EdgeFogCoverAlpha(float boardX, const StreetFogRamp& ramp, float shift) {
	return StreetFogAlpha(boardX - shift, ramp);
}

bool EdgeFogNightLook(int background) {
	return background == 1 || background == 3 || background == 5; // BACKGROUND_2_NIGHT, _4_FOG, _6_BOSS
}

EdgeFogRgb EdgeFogCelColor(bool night, int celLook, float motion) {
	if (night) // Board::DrawFog
		return {static_cast<int>(255 - celLook * 1.5f - motion * 1.5f), static_cast<int>(255 - celLook * 1.5f - motion * 1.5f),
			static_cast<int>(255 - celLook - motion)};
	const int c = static_cast<int>(255 - celLook * 0.5f - motion * 0.5f);
	return {c, c, static_cast<int>(255 - (celLook + motion) / 3.0f)};
}

EdgeFogRgb EdgeFogWashColor(bool night) {
	// Night: the clouds' mean tone under vanilla colouring (fog image ~200 grey x ~0.88, a touch of blue; as before).
	// Day: the image's own tone, lighter.
	return night ? EdgeFogRgb{196, 198, 210} : EdgeFogRgb{214, 217, 224};
}

} // namespace pvzu
