#include "doctest/doctest.h"
#include "Mod/Layout.h"
#include "Mod/StreetFog.h"

#include <cmath>
#include <initializer_list>

using namespace pvzu;

namespace {
constexpr int kUnnamedMinSpawnX = kVanillaMinSpawnX + 133; // Unnamed start offset (hook 480)
constexpr int kPhoneVisibleRight = 1019;                    // 1300 x 600 at rest zoom 1.04 (measured 2026-11-25)
}

TEST_CASE("StreetFogNeverCoversTheLawn") {
	for (int right = 700; right <= 1180; right += 7)
		for (int spawn : {kVanillaMinSpawnX, kUnnamedMinSpawnX, 1000}) {
			const StreetFogRamp r = ComputeStreetFogRamp(right, spawn);
			for (float x = -400.0f; x <= 800.0f; x += 5.0f)
				CHECK(StreetFogAlpha(x, r) == 0.0f);
		}
}

TEST_CASE("NoStreetOnScreenNoFog") {
	for (int right : {600, 799, 800}) {
		const StreetFogRamp r = ComputeStreetFogRamp(right, kUnnamedMinSpawnX);
		CHECK_FALSE(r.active);
		for (float x = 700.0f; x <= 1200.0f; x += 10.0f)
			CHECK(StreetFogAlpha(x, r) == 0.0f);
	}
}

TEST_CASE("NewZombiesAreFullyHidden") {
	// 2026-11-25: zombies must not pop in. Every pixel of a zombie at its first frame (art from mPosX +
	// kZombieArtLeft) is under a fully opaque cover, at every start x the mod uses.
	for (int right = 820; right <= 1180; ++right) {
		CAPTURE(right);
		const StreetFogRamp r = ComputeStreetFogRamp(right, kUnnamedMinSpawnX);
		REQUIRE(r.active);
		for (int x = kUnnamedMinSpawnX + kZombieArtLeft; x <= right + 20; ++x)
			CHECK(StreetFogAlpha(static_cast<float>(x), r) == 1.0f);
	}
}

TEST_CASE("RampRunsFromTheLawnEdgeToTheSpawnLine") {
	const StreetFogRamp r = ComputeStreetFogRamp(kPhoneVisibleRight, kUnnamedMinSpawnX);
	CHECK(r.clearX == 800.0f);
	CHECK(r.opaqueX == static_cast<float>(kUnnamedMinSpawnX + kZombieArtLeft));
	// 0% -> 100%, rising all the way, no hard step anywhere (a gradient, not a line).
	float last = 0.0f;
	for (float x = 800.0f; x <= kPhoneVisibleRight; x += 1.0f) {
		const float a = StreetFogAlpha(x, r);
		CHECK(a >= last);
		CHECK(a - last <= 1.5f / (r.opaqueX - r.clearX) + 1e-4f);
		last = a;
	}
	CHECK(StreetFogAlpha(801.0f, r) < 0.01f);
	CHECK(StreetFogAlpha((r.clearX + r.opaqueX) / 2.0f, r) == doctest::Approx(0.5f));
}

TEST_CASE("FogWashTrailsTheClouds") {
	// Fog levels: the flat wash under the clouds rises later than the clouds, so the cover's inner edge is the clouds'
	// own ragged shape (as vanilla fog's first column), yet it is opaque at the same place.
	const StreetFogRamp r = ComputeStreetFogRamp(kPhoneVisibleRight, kUnnamedMinSpawnX);
	for (float x = 790.0f; x <= kPhoneVisibleRight; x += 1.0f) {
		CHECK(StreetFogWashAlpha(x, r) <= StreetFogAlpha(x, r));
		CHECK(StreetFogWashAlpha(x, r) >= 0.0f);
	}
	CHECK(StreetFogWashAlpha((r.clearX + r.opaqueX) / 2.0f, r) < 0.4f);
	CHECK(StreetFogWashAlpha(r.opaqueX, r) == 1.0f);
	CHECK(StreetFogWashAlpha(800.0f, r) == 0.0f);
}

TEST_CASE("SpawnLineOffScreenRampsToTheEdge") {
	// A narrow strip (or a deep zoom) ends before the spawn line: zombies walk in past the screen edge, which is opaque.
	const StreetFogRamp r = ComputeStreetFogRamp(870, kUnnamedMinSpawnX);
	CHECK(r.opaqueX == 870.0f);
	CHECK(StreetFogAlpha(870.0f, r) == 1.0f);
	CHECK(StreetFogAlpha(835.0f, r) > 0.0f);
	CHECK(StreetFogAlpha(835.0f, r) < 1.0f);
}

TEST_CASE("ZombiesStartingAtTheLawnKeepTheMinimumRamp") {
	// Vanilla mode (or the 刷新点前移 option) starts zombies at 780: the cover cannot reach the lawn, so it keeps a
	// gradient of kStreetFogMinRamp px.
	const StreetFogRamp r = ComputeStreetFogRamp(kPhoneVisibleRight, kVanillaMinSpawnX);
	CHECK(r.opaqueX == static_cast<float>(800 + kStreetFogMinRamp));
	CHECK(StreetFogAlpha(800.0f + kStreetFogMinRamp, r) == 1.0f);
	const StreetFogRamp thin = ComputeStreetFogRamp(815, kVanillaMinSpawnX);
	CHECK(thin.opaqueX == 815.0f);
}

// ---- 2026-11-26: the cover looks like the game's fog on every level, and it never fades: it slides out off the
// screen's right edge when the camera leaves its resting place and slides back in when the camera is at rest.

namespace {
EdgeFogInput Want(bool wanted, bool zombies = false) {
	EdgeFogInput in;
	in.wanted = wanted;
	in.zombies = zombies;
	return in;
}
EdgeFogState StepN(EdgeFogState s, int n, const EdgeFogInput& in) {
	for (int i = 0; i < n; ++i)
		s = StepEdgeFog(s, in);
	return s;
}
// The board's first update at rest.
EdgeFogState Started() {
	return StepEdgeFog(EdgeFogState{}, Want(true));
}
// Then the camera's pan out to the street.
EdgeFogState OutOfSight() {
	return StepN(Started(), kEdgeFogOutTicks + 5, Want(false));
}
constexpr float kHidden = 235.0f; // 1019 + 16 - 800 (phone view at rest)
constexpr float kNoVanilla = 0.0f;
} // namespace

TEST_CASE("FirstMomentOfALevelCoverInPlace") {
	// The board shows at rest before the pan to the street: the cover is already there, no slide.
	const EdgeFogState s = Started();
	CHECK(s.started);
	CHECK(s.out == 0.0f);
	CHECK(EdgeFogShift(s, kHidden, kNoVanilla) == 0.0f);
	// A level whose camera starts elsewhere (scrolled to the house, final boss, game over): out of sight, no slide.
	const EdgeFogState away = StepEdgeFog(EdgeFogState{}, Want(false));
	CHECK(away.out == 1.0f);
	CHECK(EdgeFogShift(away, kHidden, kNoVanilla) == kHidden);
}

TEST_CASE("CameraLeavesTheCoverSlidesOutToTheRight") {
	// Eased in and out like the camera's own pan (CutScene::CalcPosition, CURVE_EASE_IN_OUT), about 1 s.
	CHECK(kEdgeFogOutTicks >= 80);
	CHECK(kEdgeFogOutTicks <= 120);
	EdgeFogState s = Started();
	float last = 0.0f;
	for (int t = 1; t <= kEdgeFogOutTicks; ++t) {
		s = StepEdgeFog(s, Want(false));
		const float shift = EdgeFogShift(s, kHidden, kNoVanilla);
		CHECK(shift >= last); // only ever moves right
		CHECK(shift <= kHidden);
		if (t == 5)
			CHECK(shift < kHidden * 0.01f); // starts gently
		if (t == kEdgeFogOutTicks / 2)
			CHECK(shift == doctest::Approx(kHidden * 0.5f));
		last = shift;
	}
	CHECK(s.out == 1.0f);
	CHECK(EdgeFogShift(s, kHidden, kNoVanilla) == kHidden);
	// And it stays out while the camera is away.
	s = StepN(s, 500, Want(false));
	CHECK(s.out == 1.0f);
}

TEST_CASE("CameraBackAtRestTheCoverSlidesInFromTheRight") {
	// Eased out like vanilla's fog rolling in (Board::UpdateGame, CURVE_EASE_OUT = 2u - u^2), about 1.2 s.
	CHECK(kEdgeFogInTicks >= 100);
	CHECK(kEdgeFogInTicks <= 150);
	EdgeFogState s = OutOfSight();
	float last = kHidden;
	for (int t = 1; t <= kEdgeFogInTicks; ++t) {
		s = StepEdgeFog(s, Want(true));
		const float shift = EdgeFogShift(s, kHidden, kNoVanilla);
		CHECK(shift <= last); // only ever moves left
		CHECK(shift >= 0.0f);
		if (t == kEdgeFogInTicks / 2)
			CHECK(shift == doctest::Approx(kHidden * 0.25f)); // (1 - u)^2 left to go
		last = shift;
	}
	CHECK(s.out == 0.0f);
	CHECK(EdgeFogShift(s, kHidden, kNoVanilla) == 0.0f);
	// Ease out: the first step moves furthest.
	const EdgeFogState a = StepEdgeFog(OutOfSight(), Want(true));
	CHECK(kHidden - EdgeFogShift(a, kHidden, kNoVanilla) > 3.0f);
}

TEST_CASE("AReversedMoveCarriesOnFromWhereTheCoverIs") {
	EdgeFogState s = StepN(OutOfSight(), kEdgeFogInTicks / 2, Want(true));
	const float mid = s.out;
	REQUIRE(mid > 0.1f);
	REQUIRE(mid < 0.9f);
	s = StepEdgeFog(s, Want(false)); // the camera leaves again half way
	CHECK(s.out >= mid);
	CHECK(s.out - mid < 0.01f); // no jump
	s = StepN(s, kEdgeFogOutTicks, Want(false));
	CHECK(s.out == 1.0f);
	EdgeFogState t = StepN(Started(), kEdgeFogOutTicks / 2, Want(false));
	const float half = t.out;
	t = StepEdgeFog(t, Want(true));
	CHECK(t.out <= half);
	CHECK(half - t.out < 0.05f);
	t = StepN(t, kEdgeFogInTicks, Want(true));
	CHECK(t.out == 0.0f);
}

TEST_CASE("ZombiesOnTheBoardSnapTheCoverIn") {
	// Mid slide-in a zombie appears (a mode that places zombies, a very early wave): in place at once.
	EdgeFogState s = StepN(OutOfSight(), 5, Want(true));
	REQUIRE(s.out > 0.5f);
	s = StepEdgeFog(s, Want(true, true));
	CHECK(s.out == 0.0f);
	CHECK(EdgeFogShift(s, kHidden, kNoVanilla) == 0.0f);
	// Out of sight while the camera is at rest (Fog level waiting for vanilla's roll): zombies bring it in at once.
	EdgeFogInput wait = Want(true, true);
	wait.waitForRoll = true;
	CHECK(StepEdgeFog(OutOfSight(), wait).out == 0.0f);
	// A resumed game with zombies: in place from the first update.
	CHECK(StepEdgeFog(EdgeFogState{}, Want(true, true)).out == 0.0f);
	// Game over / the final boss with zombies on the board: the cover still goes (by sliding).
	const EdgeFogState g = StepEdgeFog(Started(), Want(false, true));
	CHECK(g.out > 0.0f);
	CHECK(g.out < 0.01f);
}

TEST_CASE("TheCoverIsNeverFaded") {
	// Wherever the cover is, it is the resting ramp moved right: fully opaque from its moved opaque line on.
	const StreetFogRamp r = ComputeStreetFogRamp(kPhoneVisibleRight, kUnnamedMinSpawnX);
	const float hidden = EdgeFogHiddenShift(r, kPhoneVisibleRight);
	CHECK(hidden == static_cast<float>(kPhoneVisibleRight) + kEdgeFogHiddenMargin - r.clearX);
	EdgeFogState s = Started();
	auto check = [&](const EdgeFogState& st) {
		const float shift = EdgeFogShift(st, hidden, kNoVanilla);
		for (float x = r.opaqueX + shift; x <= kPhoneVisibleRight + 20; x += 3.0f)
			CHECK(EdgeFogCoverAlpha(x, r, shift) == 1.0f);
		for (float x = 600.0f; x <= kPhoneVisibleRight + 20; x += 3.0f)
			CHECK(EdgeFogCoverAlpha(x, r, shift) == StreetFogAlpha(x - shift, r));
	};
	for (int t = 0; t < kEdgeFogOutTicks + 5; t += 4) {
		check(s);
		s = StepN(s, 4, Want(false));
	}
	for (int t = 0; t < kEdgeFogInTicks + 5; t += 4) {
		check(s);
		s = StepN(s, 4, Want(true));
	}
	// Out of sight: nothing of it is left on the screen.
	const float shift = EdgeFogShift(OutOfSight(), hidden, kNoVanilla);
	for (float x = 600.0f; x <= kPhoneVisibleRight + kEdgeFogHiddenMargin; x += 1.0f)
		CHECK(EdgeFogCoverAlpha(x, r, shift) == 0.0f);
	// Moving only ever takes cover off the lawn: never over x < 800.
	for (float sh = 0.0f; sh <= hidden; sh += 5.0f)
		for (float x = 600.0f; x <= 800.0f; x += 5.0f)
			CHECK(EdgeFogCoverAlpha(x, r, sh) == 0.0f);
}

TEST_CASE("NewZombiesStayHiddenAtEveryMomentOfTheStart") {
	// Whenever zombies first exist during the slide in, from that update on every pixel right of the spawn line is
	// fully covered, even during vanilla's roll on a Fog level.
	const StreetFogRamp r = ComputeStreetFogRamp(kPhoneVisibleRight, kUnnamedMinSpawnX);
	const float hidden = EdgeFogHiddenShift(r, kPhoneVisibleRight);
	EdgeFogInput rollZ = Want(true, true);
	rollZ.rollActive = true;
	for (int k = 0; k <= kEdgeFogInTicks + 5; k += 4) {
		CAPTURE(k);
		EdgeFogState s = StepN(OutOfSight(), k, Want(true));
		for (int t = 0; t < 40; ++t) {
			s = StepEdgeFog(s, rollZ);
			const float shift = EdgeFogShift(s, hidden, 600.0f);
			for (int x = kUnnamedMinSpawnX + kZombieArtLeft; x <= kPhoneVisibleRight + 20; x += 3)
				CHECK(EdgeFogCoverAlpha(static_cast<float>(x), r, shift) == 1.0f);
		}
	}
}

TEST_CASE("FogLevelsTheCoverRollsInWithVanillasFog") {
	// Board::UpdateGame in the intro: mFogOffset = CURVE_EASE_OUT from 1065 - LeftFogColumn * 80 to 0 while
	// CutScene counts mFogBlownCountDown 200 -> 0. Both now move left out of the street, so the cover rides it.
	EdgeFogInput wait = Want(true);
	wait.waitForRoll = true;
	// The level's first moment (vanilla's fog is blown off to the right, countdown 200): the cover is in place.
	EdgeFogState s = StepEdgeFog(EdgeFogState{}, wait);
	CHECK(s.out == 0.0f);
	CHECK(EdgeFogShift(s, kHidden, 665.0f) == 0.0f);
	s = StepN(s, 3, wait);
	CHECK(s.out == 0.0f);
	// Back at rest before vanilla starts its roll: it waits out of sight.
	s = StepN(OutOfSight(), 50, wait);
	CHECK(s.out == 1.0f);
	CHECK(EdgeFogShift(s, kHidden, 665.0f) == kHidden);
	// The roll starts: the cover comes in with it, at vanilla's offset (same 80 px grid, so the clouds coincide).
	EdgeFogInput roll = Want(true);
	roll.rollActive = true;
	const float maxOffset = 1065.0f - 5 * 80.0f;
	for (int c = 199; c > 0; --c) {
		s = StepEdgeFog(s, roll);
		const float u = (200 - c) / 200.0f;
		const float vanilla = maxOffset * (1.0f - (2.0f * u - u * u));
		CAPTURE(c);
		CHECK(s.followRoll);
		CHECK(EdgeFogShift(s, kHidden, vanilla) == doctest::Approx(vanilla).epsilon(0.001));
	}
	// The level starts (CutScene: countdown and offset 0): in place.
	s = StepEdgeFog(s, Want(true));
	CHECK_FALSE(s.followRoll);
	CHECK(EdgeFogShift(s, kHidden, 0.0f) == 0.0f);
	CHECK(s.out == 0.0f);
	// During play vanilla's offset (Blover) never moves the cover.
	CHECK(EdgeFogShift(s, kHidden, 500.0f) == 0.0f);
	// Zombies during the roll: in place at once, vanilla's offset ignored.
	EdgeFogInput rollZ = roll;
	rollZ.zombies = true;
	const EdgeFogState z = StepEdgeFog(StepN(OutOfSight(), 10, roll), rollZ);
	CHECK_FALSE(z.followRoll);
	CHECK(EdgeFogShift(z, kHidden, 400.0f) == 0.0f);
	// Camera away during a roll (not wanted): out, vanilla's offset ignored.
	EdgeFogInput away = roll;
	away.wanted = false;
	CHECK_FALSE(StepEdgeFog(Started(), away).followRoll);
}

TEST_CASE("NightLevelsUseTheGamesFogColourDayLevelsALighterOne") {
	// BackgroundType: 0 day, 1 night, 2 pool, 3 fog, 4 roof, 5 boss (night roof).
	CHECK_FALSE(EdgeFogNightLook(0));
	CHECK(EdgeFogNightLook(1));
	CHECK_FALSE(EdgeFogNightLook(2));
	CHECK(EdgeFogNightLook(3));
	CHECK_FALSE(EdgeFogNightLook(4));
	CHECK(EdgeFogNightLook(5));
	for (int look = 0; look < 20; ++look)
		for (float motion = 1.0f; motion <= 25.0f; motion += 2.0f) {
			// Night: exactly Board::DrawFog's colouring.
			const EdgeFogRgb n = EdgeFogCelColor(true, look, motion);
			CHECK(n.r == static_cast<int>(255 - look * 1.5f - motion * 1.5f));
			CHECK(n.g == n.r);
			CHECK(n.b == static_cast<int>(255 - look - motion));
			// Day: the same shimmer, lighter.
			const EdgeFogRgb d = EdgeFogCelColor(false, look, motion);
			CHECK(d.r > n.r);
			CHECK(d.b >= n.b);
			CHECK(d.r <= 255);
			CHECK(d.b <= 255);
		}
	const EdgeFogRgb wn = EdgeFogWashColor(true), wd = EdgeFogWashColor(false);
	CHECK(wn.r + wn.g + wn.b < wd.r + wd.g + wd.b);
}
