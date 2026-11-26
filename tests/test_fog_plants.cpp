// PVZU: Unnamed Fog-plant rules (the research notes (w4-fog) section 2). Rolls are injected; boundary rolls 0, threshold,
// threshold+1 and max for every chance. Every rule is also checked with the "fogPlants" feature off (vanilla mode)
// while the pack rules are loaded: it must stay vanilla. The last test compares these values with the pack file.
#include "doctest/doctest.h"
#include "Mod/GameData.h"
#include "Mod/ModPack.h"
#include "Mod/Unnamed/FogPlants.h"
#include "Mod/Unnamed/PoolPlants.h"
#include "Mod/Unnamed/Rules.h"
#include "Mod/Unnamed/State.h"

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>

using namespace pvzu;
using namespace pvzu::unnamed;
namespace fs = std::filesystem;

namespace {
// Same values as pvz-unnamed-pack/data/rules-fog-plants.json (the last test compares the two).
nlohmann::json PackRules() {
	return nlohmann::json{
		{"fogPlants.plantern.fogRadius", 20},
		{"fogPlants.plantern.speedupMinRate", 100},
		{"fogPlants.plantern.speedupDivisor", 2},
		{"fogPlants.plantern.speedupAddGatlingCattail", 25},
		{"fogPlants.plantern.healTickCs", 4},
		{"fogPlants.plantern.healPerTick", 1},
		{"fogPlants.plantern.maxHpDecayBelowHalf", 1},
		{"fogPlants.cactus.secondVolleyAtCounter", 25},
		{"fogPlants.cactus.animRate", 45},
		{"fogPlants.cactus.shootingCounter", 23},
		{"fogPlants.cactus.toSpikeweed", 1},
		{"fogPlants.cactus.spikePierceMax", 64},
		{"fogPlants.blover.storm", 1},
		{"fogPlants.blover.damage", 20},
		{"fogPlants.blover.balloonChillCs", 1000},
		{"fogPlants.splitPea.fireFront", 1},
		{"fogPlants.splitPea.sweepRoll", 10},
		{"fogPlants.splitPea.sweepOnPlant", 1},
		{"fogPlants.splitPea.sweepDurationCs", 500},
		{"fogPlants.splitPea.sweepSteps", 6},
		{"fogPlants.splitPea.sweepFireStep", 3},
		{"fogPlants.splitPea.sweepFallAbove", 96},
		{"fogPlants.splitPea.sweepRiseBelow", 31},
		{"fogPlants.splitPea.sweepLaunchCounter", 30},
		{"fogPlants.splitPea.sweepPhase", -31.4},
		{"fogPlants.splitPea.sweepDiv", 20},
		{"fogPlants.splitPea.sweepSpeedDiv", 0.15},
		{"fogPlants.splitPea.sweepMotion", 7},
		{"fogPlants.splitPea.sweepRangeFlags", 1},
		{"fogPlants.starfruit.starSpeedMult", 1.5},
		{"fogPlants.starfruit.extraStarsRoll", 3},
		{"fogPlants.starfruit.returnAt", 40},
		{"fogPlants.starfruit.returnAgainAt", 120},
		{"fogPlants.starfruit.extraStars", 4},
		{"fogPlants.starfruit.spiralSpeed", 0.03},
		{"fogPlants.starfruit.spiralTurn", 0.05},
		{"fogPlants.starfruit.spiralMaxTag", 4},
		{"fogPlants.starfruit.pierceLimit", 255},
		{"fogPlants.pumpkin.explodeAtHp", 72},
		{"fogPlants.pumpkin.explodesWhenSmashed", 1},
		{"fogPlants.magnet.allRows", 1},
		{"fogPlants.magnet.unlimitedRadius", 1},
		{"fogPlants.cattail.cobRoll", 100},
		{"fogPlants.cattail.cobBelow", 3},
	};
}

void SetFeature(bool on) {
	GameData::Get().ResetToVanilla();
	if (on) {
		ModPack p;
		p.id = "test-fog";
		p.features["fogPlants"] = true;
		std::string err;
		REQUIRE_MESSAGE(GameData::Get().ApplyPack(p, &err), err);
	}
}

struct UnnamedFog {
	UnnamedFog() { SetRules(PackRules()); SetFeature(true); }
	~UnnamedFog() { SetRules(nlohmann::json::object()); SetFeature(false); }
};
struct VanillaWithRules {
	VanillaWithRules() { SetRules(PackRules()); SetFeature(false); }
	~VanillaWithRules() { SetRules(nlohmann::json::object()); }
};

// engine enums (same numbers as the exe)
constexpr int kSeedPeashooter = 0, kSeedCherry = 2, kSeedWallnut = 3, kSeedRepeater = 7, kSeedSnowPea = 5,
	kSeedSpikeweed = 21, kSeedTallnut = 23, kSeedPlantern = 25, kSeedCactus = 26, kSeedSplitPea = 28, kSeedStarfruit = 29,
	kSeedPumpkin = 30, kSeedGatling = 40, kSeedGloom = 42, kSeedCattail = 43;
constexpr int kNormal = 0, kPail = 4, kZamboni = 12, kBalloon = 16, kDigger = 17, kPogo = 18, kCatapult = 22,
	kGargantuar = 23, kImp = 24, kBoss = 25, kSquashHead = 30, kRedeye = 32;
constexpr int kProjPea = 0, kProjStar = 7, kProjSpike = 8, kProjCob = 11;
}

// --- Plantern -------------------------------------------------------------------------------------------

TEST_CASE_FIXTURE(UnnamedFog, "Plantern: fog radius 20 (the whole lawn), vanilla 4") {
	CHECK(FogPlantsOn());
	CHECK(PlanternFogRadius() == 20);
}

TEST_CASE_FIXTURE(UnnamedFog, "Plantern 3x3: launch counter rate/2 - roll for rate >= 100; Gloom full; Gatling/Cattail rate/2 + 25") {
	// no Plantern near: vanilla rate - Rand(15)
	CHECK(PlanternLaunchCounter(kSeedPeashooter, 150, 0, false) == 150);
	CHECK(PlanternLaunchCounter(kSeedPeashooter, 150, 14, false) == 136);
	// Plantern near
	CHECK(PlanternLaunchCounter(kSeedPeashooter, 150, 0, true) == 75);
	CHECK(PlanternLaunchCounter(kSeedPeashooter, 150, 14, true) == 61);
	CHECK(PlanternLaunchCounter(kSeedCactus, 150, 3, true) == 72);
	CHECK(PlanternLaunchCounter(kSeedStarfruit, 250, 0, true) == 125);
	CHECK(PlanternLaunchCounter(kSeedPeashooter, 151, 0, true) == 75); // rate >>= 1
	// boundary: below 100 no bonus (fast Day Peashooters)
	CHECK(PlanternLaunchCounter(kSeedPeashooter, 100, 0, true) == 50);
	CHECK(PlanternLaunchCounter(kSeedPeashooter, 99, 0, true) == 99);
	CHECK(PlanternLaunchCounter(kSeedPeashooter, 25, 4, true) == 21);
	// exceptions
	CHECK(PlanternLaunchCounter(kSeedGloom, 200, 5, true) == 195);
	CHECK(PlanternLaunchCounter(kSeedGatling, 150, 0, true) == 100);
	CHECK(PlanternLaunchCounter(kSeedCattail, 150, 10, true) == 90);
	CHECK(PlanternLaunchCounter(kSeedGatling, 99, 0, true) == 99);
	// the cave compares the raw seed (0x6516cb..0x651721): mod plants get the plain halving, not their base's exception
	CHECK(PlanternLaunchCounter(0x7b, 300, 0, true) == 150); // Spirit-tail (Cattail base): no +25
	CHECK(PlanternLaunchCounter(0x78, 150, 0, true) == 75);  // Sniper (Gatling base): no +25
	CHECK(PlanternLaunchCounter(0x7a, 200, 5, true) == 95);  // Element-shroom (Gloom base): halved
}

TEST_CASE_FIXTURE(UnnamedFog, "Plantern heal: every 4 cs +1 HP up to max; below half HP the max drops by 1 first") {
	CHECK(PlanternHealTickCs() == 4);
	int hp = 200, maxHp = 300;
	PlanternHealStep(hp, maxHp);
	CHECK(hp == 201);
	CHECK(maxHp == 300);
	hp = 300;
	PlanternHealStep(hp, maxHp);
	CHECK(hp == 300); // full: nothing
	CHECK(maxHp == 300);
	hp = 149; // below half (150)
	PlanternHealStep(hp, maxHp);
	CHECK(maxHp == 299);
	CHECK(hp == 150);
	hp = 150; maxHp = 300; // exactly half: no decay
	PlanternHealStep(hp, maxHp);
	CHECK(maxHp == 300);
	CHECK(hp == 151);
	hp = 1; maxHp = 2; // 1 < 2 >> 1 is false: heal only
	PlanternHealStep(hp, maxHp);
	CHECK(hp == 2);
	CHECK(maxHp == 2);
}

// --- Cactus ---------------------------------------------------------------------------------------------

TEST_CASE_FIXTURE(UnnamedFog, "Cactus: second volley at launch counter 25, every shot anim 45 / shooting counter 23") {
	CHECK(CactusSecondVolley(25));
	CHECK_FALSE(CactusSecondVolley(24));
	CHECK_FALSE(CactusSecondVolley(26));
	CHECK_FALSE(CactusSecondVolley(0));
	float rate = 0;
	int counter = 0;
	REQUIRE(CactusShotAnim(rate, counter));
	CHECK(rate == 45.0f);
	CHECK(counter == 23);
}

TEST_CASE_FIXTURE(UnnamedFog, "Cactus: run over by Zomboni / Catapult becomes a Spikeweed; other squishers and plants vanilla") {
	CHECK(CactusBecomesSpikeweed(kSeedCactus, kZamboni));
	CHECK(CactusBecomesSpikeweed(kSeedCactus, kCatapult));
	for (int z : {kNormal, kGargantuar, kRedeye, kSquashHead, kBoss, kDigger, kPogo}) {
		CAPTURE(z);
		CHECK_FALSE(CactusBecomesSpikeweed(kSeedCactus, z));
	}
	CHECK_FALSE(CactusBecomesSpikeweed(kSeedSpikeweed, kZamboni));
	CHECK_FALSE(CactusBecomesSpikeweed(kSeedTallnut, kZamboni));
}

TEST_CASE_FIXTURE(UnnamedFog, "Cactus / Pumpkin are converted once: a squished Cactus leaves no second Spikeweed, a lit Pumpkin is no Pumpkin") {
	CHECK(CactusBecomesSpikeweed(kSeedCactus, kZamboni, false));
	CHECK_FALSE(CactusBecomesSpikeweed(kSeedCactus, kZamboni, true));  // already squished, waiting to disappear
	CHECK_FALSE(CactusBecomesSpikeweed(kSeedCactus, kCatapult, true));
	// after the first bite the plant is a Cherry Bomb: the next bite / smash is not a Pumpkin rule any more
	REQUIRE(PumpkinExplodesWhenBitten(kSeedPumpkin, 72));
	CHECK_FALSE(PumpkinExplodesWhenBitten(PumpkinExplodeSeed(), 68));
	CHECK_FALSE(PumpkinExplodesWhenSmashed(PumpkinExplodeSeed(), kGargantuar));
}

TEST_CASE_FIXTURE(UnnamedFog, "Spike pierce: a spike lives through 64 hits and dies on the 65th; other projectiles die") {
	ProjectileState s;
	for (int i = 0; i < 64; ++i) {
		CAPTURE(i);
		REQUIRE(SpikeSurvivesHit(s, kProjSpike));
	}
	CHECK(s.pierceHits == 64);
	CHECK_FALSE(SpikeSurvivesHit(s, kProjSpike));
	CHECK(s.pierceHits == 64);
	ProjectileState pea;
	CHECK_FALSE(SpikeSurvivesHit(pea, kProjPea));
	CHECK_FALSE(SpikeSurvivesHit(pea, kProjStar));
	CHECK(pea.pierceHits == 0);
}

// --- Blover ---------------------------------------------------------------------------------------------

TEST_CASE_FIXTURE(UnnamedFog, "Blover storm: 20 freeze damage to all, Imps and armless zombies blown off, balloons only chilled") {
	CHECK(BloverStorm());
	const BloverHit walker = BloverOnZombie(kNormal, false, false);
	CHECK(walker.touched);
	CHECK_FALSE(walker.blowAway);
	CHECK(walker.damage == 20);
	CHECK(walker.blowIfArmless);
	CHECK(walker.balloonChillCs == 0);
	const BloverHit imp = BloverOnZombie(kImp, false, false);
	CHECK(imp.touched);
	CHECK(imp.blowAway);
	CHECK(imp.damage == 20);
	const BloverHit balloon = BloverOnZombie(kBalloon, false, true);
	CHECK(balloon.touched);
	CHECK_FALSE(balloon.blowAway); // patched 0x466603: never blown away
	CHECK(balloon.damage == 20);
	CHECK(balloon.balloonChillCs == 1000);
	const BloverHit popped = BloverOnZombie(kBalloon, false, false); // walking after its balloon popped
	CHECK(popped.balloonChillCs == 0);
	const BloverHit hypno = BloverOnZombie(kNormal, true, false);
	CHECK_FALSE(hypno.touched);
	CHECK(hypno.damage == 0);
	CHECK_FALSE(hypno.blowAway);
	CHECK_FALSE(BloverOnZombie(kImp, true, false).blowAway);
}

// --- Split Pea ------------------------------------------------------------------------------------------

TEST_CASE_FIXTURE(UnnamedFog, "Split Pea: front shots are fire peas, back shots snow peas; Rand(10) == 0 starts a sweep") {
	CHECK(SplitPeaShotKind(false) == SplitShot::Fire);
	CHECK(SplitPeaShotKind(true) == SplitShot::Snow);
	CHECK(SplitPeaSweepRollRange() == 10);
	CHECK(SplitPeaStartsSweep(0));
	CHECK_FALSE(SplitPeaStartsSweep(1));
	CHECK_FALSE(SplitPeaStartsSweep(9));
}

// V2 (C3, 1.3beta1 "[quote not included]"): PlantInitialize memsets Plant+0xC8..+0x12B to 0 (0x45dcb4), so +0xEC = 0 and
// hook 314 sweeps from the first tick; nothing writes -1 at planting (only the sweep end 0xc119bc)
TEST_CASE_FIXTURE(UnnamedFog, "Split Pea: a planted Split Pea starts sweeping at once and goes idle after 500 cs") {
	CHECK(SplitPeaInitialSweepTimer() == 0);
	PlantState s;
	s.sweepTimer = SplitPeaInitialSweepTimer();
	CHECK(SplitPeaSweeping(s));
	bool ended = false;
	for (int tick = 1; tick <= 600 && !ended; ++tick) {
		SplitPeaSweepTick(s);
		if (SplitPeaSweepFiresNow(s) && SplitPeaSweepEnds(s))
			ended = true;
	}
	CHECK(ended);
	CHECK(s.sweepTimer == -1);
	CHECK_FALSE(SplitPeaSweeping(s));
}

TEST_CASE_FIXTURE(UnnamedFog, "Split Pea sweep: idle by default; angle bounces 29..96, a shot every 6 cs at step 3, ends at 500") {
	PlantState s;
	CHECK_FALSE(SplitPeaSweeping(s)); // an idle state (-1) does not sweep; a planted Split Pea starts at 0, see below
	PlantState idle = s;
	SplitPeaSweepTick(idle);
	CHECK(idle == s); // the oscillator only runs while sweeping
	CHECK(SplitPeaSweepLaunchCounter() == 30);
	s.sweepTimer = 0;
	s.sweepAngle = 60;
	int shots = 0, minAngle = 1000, maxAngle = -1000, firstShotTick = -1;
	bool ended = false;
	for (int tick = 1; tick <= 600 && !ended; ++tick) {
		SplitPeaSweepTick(s);
		minAngle = std::min(minAngle, s.sweepAngle);
		maxAngle = std::max(maxAngle, s.sweepAngle);
		CHECK(s.sweepStep >= 0);
		CHECK(s.sweepStep <= 5);
		if (SplitPeaSweepFiresNow(s)) {
			if (firstShotTick < 0)
				firstShotTick = tick;
			if (SplitPeaSweepEnds(s))
				ended = true;
			else
				++shots;
		}
	}
	CHECK(firstShotTick == 3);
	CHECK(ended);
	CHECK_FALSE(SplitPeaSweeping(s));
	CHECK(s.sweepTimer == -1);
	CHECK(shots == 83);  // shots at ticks 3, 9, ... 495 (timer 3..495); the one at timer 501 ends the sweep
	CHECK(maxAngle == 96); // 97 is reached inside the tick and 2 taken off at once
	CHECK(minAngle == 29);  // 31 -> 32 - 2 = 30 -> 31 - 2 = 29 -> 30 (< 31: rising again)
}

TEST_CASE_FIXTURE(UnnamedFog, "Split Pea sweep shot: speed 1/0.15 px/cs, up at angle 30, forward near 62, down at 96") {
	float vx = 0, vy = 0;
	SplitPeaSweepVelocity(62, vx, vy); // (-31.4 - 62) / 20 = -4.67 rad
	CHECK(vx > 6.6f);
	CHECK(std::fabs(vy) < 0.3f);
	CHECK(std::hypot(vx, vy) == doctest::Approx(1.0 / 0.15).epsilon(0.001));
	SplitPeaSweepVelocity(30, vx, vy);
	CHECK(vy < -6.6f); // straight up (screen y grows down)
	SplitPeaSweepVelocity(96, vx, vy);
	CHECK(vy > 6.6f);  // straight down
	CHECK(SplitPeaSweepMotion() == 7);
	CHECK(SplitPeaSweepRangeFlags() == 1);
}

// --- Starfruit ------------------------------------------------------------------------------------------

TEST_CASE_FIXTURE(UnnamedFog, "Starfruit: stars x1.5 speed; Rand(3) == 0 rolls the four extra stars") {
	CHECK(StarfruitStarSpeedMult() == 1.5f);
	CHECK(StarfruitStarCurve() == kStarCurveBoomerang); // hook 286 [+0x88] = 276
	CHECK(StarfruitExtraRollRange() == 3);
	CHECK(StarfruitExtraStars(0));
	CHECK_FALSE(StarfruitExtraStars(1));
	CHECK_FALSE(StarfruitExtraStars(2));
}

TEST_CASE_FIXTURE(UnnamedFog, "Starfruit: the 1/3 roll adds four spiral stars tagged 0..3 (hook 287 0x95b0ee)") {
	CHECK(StarfruitExtraStarCount() == 4);
	CHECK(StarfruitExtraStarCurve(0) == kStarCurveSpiral0);
	CHECK(StarfruitExtraStarCurve(3) == kStarCurveSpiral0 + 3);
}

TEST_CASE_FIXTURE(UnnamedFog, "Starfruit boomerang: velocity negated at age 40 and 120 only (0x95b1ab)") {
	float vx = 5.0f, vy = -2.5f;
	CHECK_FALSE(StarCurveStep(kStarCurveBoomerang, 39, vx, vy));
	CHECK(vx == 5.0f);
	CHECK(StarCurveStep(kStarCurveBoomerang, 40, vx, vy));
	CHECK(vx == -5.0f);
	CHECK(vy == 2.5f);
	CHECK_FALSE(StarCurveStep(kStarCurveBoomerang, 41, vx, vy));
	CHECK_FALSE(StarCurveStep(kStarCurveBoomerang, 80, vx, vy));
	CHECK(StarCurveStep(kStarCurveBoomerang, 120, vx, vy));
	CHECK(vx == 5.0f);
	CHECK(vy == -2.5f);
	CHECK_FALSE(StarCurveStep(kStarCurveBoomerang, 240, vx, vy));
	CHECK_FALSE(StarCurveStep(kStarCurveNone, 40, vx, vy)); // untagged stars fly straight
	CHECK(vx == 5.0f);
}

TEST_CASE_FIXTURE(UnnamedFog, "Starfruit spiral: v = 0.03 t (cos, sin)(0.05 t + (n-1) pi/2) (0x95b1d9)") {
	const double kPi = 3.14159265358979323846;
	for (int n = 0; n <= 4; ++n) {
		for (int t : {1, 40, 126, 300}) {
			float vx = 99.0f, vy = 99.0f;
			CHECK(StarCurveStep(kStarCurveSpiral0 + n, t, vx, vy));
			const double th = t * 0.05 + (n - 1) * kPi / 2;
			CHECK(vx == doctest::Approx(0.03 * t * std::cos(th)).epsilon(1e-4));
			CHECK(vy == doctest::Approx(0.03 * t * std::sin(th)).epsilon(1e-4));
		}
	}
	// tag 0 at age 1 starts straight up-right of "k = -1": (0.03 cos(0.05 - pi/2), ...) = (+0.0015, -0.03)
	float vx = 0, vy = 0;
	CHECK(StarCurveStep(kStarCurveSpiral0, 1, vx, vy));
	CHECK(vy < -0.029f);
	CHECK(vx > 0.0f);
	// the four stars of one volley are 90 degrees apart
	float ax, ay, bx, by;
	StarCurveStep(kStarCurveSpiral0 + 1, 100, ax, ay);
	StarCurveStep(kStarCurveSpiral0 + 2, 100, bx, by);
	CHECK(ax * bx + ay * by == doctest::Approx(0.0).epsilon(1e-3));
	// +0x88 above 4 is no spiral
	vx = 1.0f;
	CHECK_FALSE(StarCurveStep(kStarCurveSpiral0 + 5, 10, vx, vy));
	CHECK(vx == 1.0f);
}

TEST_CASE_FIXTURE(UnnamedFog, "Starfruit spiral star path: an outward spiral, ~0.6 px per cs, one turn per 126 cs") {
	float x = 0, y = 0;
	for (int t = 1; t <= 252; ++t) {
		float vx = 0, vy = 0;
		StarCurveStep(kStarCurveSpiral0 + 1, t, vx, vy);
		x += vx;
		y += vy;
		if (t == 126) CHECK(std::hypot(x, y) == doctest::Approx(75.0).epsilon(0.05));
	}
	CHECK(std::hypot(x, y) == doctest::Approx(151.0).epsilon(0.05));
}

TEST_CASE_FIXTURE(UnnamedFog, "Star pierce: a star lives through 254 Die calls, the 255th kills it (0x6511e3, counter from 1)") {
	CHECK(StarPierceLimit() == 255);
	ProjectileState s;
	for (int i = 0; i < 254; ++i)
		REQUIRE(StarSurvivesDie(s, StarPierceLimit()));
	CHECK(s.pierceHits == 254);
	CHECK_FALSE(StarSurvivesDie(s, StarPierceLimit()));
	CHECK(s.pierceHits == 254);
	ProjectileState red;
	for (int i = 0; i < 4; ++i) // a limit of 5 (red Dream star, cmp 5) = 4 lives
		REQUIRE(StarSurvivesDie(red, 5));
	CHECK_FALSE(StarSurvivesDie(red, 5));
	ProjectileState none;
	CHECK_FALSE(StarSurvivesDie(none, 0));
	CHECK_FALSE(StarSurvivesDie(none, 1));
}

// --- Pumpkin --------------------------------------------------------------------------------------------

TEST_CASE_FIXTURE(UnnamedFog, "Pumpkin: bitten at <= 72 HP or smashed by Gargantuar/Giga/Squash-head explodes as a Cherry Bomb") {
	CHECK(PumpkinExplodesWhenBitten(kSeedPumpkin, 72));
	CHECK(PumpkinExplodesWhenBitten(kSeedPumpkin, 1));
	CHECK_FALSE(PumpkinExplodesWhenBitten(kSeedPumpkin, 73));
	CHECK_FALSE(PumpkinExplodesWhenBitten(kSeedPumpkin, 4000));
	CHECK_FALSE(PumpkinExplodesWhenBitten(kSeedWallnut, 50));
	CHECK_FALSE(PumpkinExplodesWhenBitten(kSeedTallnut, 50));
	CHECK(PumpkinExplodesWhenSmashed(kSeedPumpkin, kGargantuar));
	CHECK(PumpkinExplodesWhenSmashed(kSeedPumpkin, kRedeye));
	CHECK(PumpkinExplodesWhenSmashed(kSeedPumpkin, kSquashHead));
	CHECK_FALSE(PumpkinExplodesWhenSmashed(kSeedPumpkin, kZamboni)); // vehicles: plain squish
	CHECK_FALSE(PumpkinExplodesWhenSmashed(kSeedPumpkin, kCatapult));
	CHECK_FALSE(PumpkinExplodesWhenSmashed(kSeedPumpkin, kBoss));
	CHECK_FALSE(PumpkinExplodesWhenSmashed(kSeedTallnut, kGargantuar));
	CHECK(PumpkinExplodeSeed() == kSeedCherry);
}

// --- Magnet-shroom, Cattail -----------------------------------------------------------------------------

TEST_CASE_FIXTURE(UnnamedFog, "Magnet-shroom: every row, no radius limit") {
	CHECK(MagnetAllRows());
	CHECK(MagnetUnlimitedRadius());
}

TEST_CASE_FIXTURE(UnnamedFog, "Cattail: Rand(100) < 3 (3 %) turns the spike into a homing Cob") {
	CHECK(CattailCobRollRange() == 100);
	CHECK(CattailFiresCob(0));
	CHECK(CattailFiresCob(2));        // threshold
	CHECK_FALSE(CattailFiresCob(3));  // threshold + 1
	CHECK_FALSE(CattailFiresCob(99));
	CHECK(CattailCobProjectile() == kProjCob);
}

// --- Torchwood fire tiers (Pool rules) with Split Pea / Starfruit / Cattail shots ---------------------------

namespace {
struct UnnamedFogPool {
	UnnamedFogPool() {
		auto rules = PackRules();
		rules["poolPlants.torch.maxTier"] = 5; // pvz-unnamed-pack/data/rules-pool-plants.json
		SetRules(rules);
		GameData::Get().ResetToVanilla();
		ModPack p;
		p.id = "test-fog-pool";
		p.features["fogPlants"] = true;
		p.features["poolPlants"] = true;
		std::string err;
		REQUIRE_MESSAGE(GameData::Get().ApplyPack(p, &err), err);
	}
	~UnnamedFogPool() { SetRules(nlohmann::json::object()); SetFeature(false); }
};
}

TEST_CASE_FIXTURE(UnnamedFogPool, "Split Pea fire pea (front, sweep too) steps one Torchwood tier per Torchwood; stars and Cobs never convert") {
	// Plant::Fire: the front shot is converted once, ConvertToFireball(-2) (not a Torchwood column) -> tier 1
	REQUIRE(SplitPeaShotKind(false) == SplitShot::Fire);
	ProjectileState s;
	int hitGridX = -1;
	REQUIRE(FireballConvert(s, hitGridX, -2));
	hitGridX = -2;
	CHECK(s.fireTier == 1);
	// then Torchwoods in columns 3 and 4, each overlapped for several ticks (Plant::UpdateTorchwood)
	for (int col : {3, 4}) {
		for (int tick = 0; tick < 4; ++tick) {
			if (TorchwoodAction(6, s.fireTier) == TorchAction::ToFireball && FireballConvert(s, hitGridX, col))
				hitGridX = col;
		}
	}
	CHECK(s.fireTier == 5); // 1 -> 2 -> 5, one step per Torchwood
	// the back snow pea thaws at a Torchwood like any snow pea
	CHECK(SplitPeaShotKind(true) == SplitShot::Snow);
	CHECK(TorchwoodAction(1, 0) == TorchAction::ToPea);
	// Starfruit stars and the Cattail Cob are not Torchwood shots
	CHECK(TorchwoodAction(kProjStar, 0) == TorchAction::None);
	CHECK(TorchwoodAction(kProjCob, 0) == TorchAction::None);
	CHECK(TorchwoodAction(kProjSpike, 0) == TorchAction::None);
}

// --- Save state, vanilla --------------------------------------------------------------------------------

TEST_CASE("PlantState sweep fields round-trip; 12-byte blobs of older saves read an idle Split Pea") {
	PlantState s;
	s.counter = 3;
	s.frozen = 1;
	s.sweepAngle = 77;
	s.sweepDir = 1;
	s.sweepStep = 4;
	s.sweepTimer = 321;
	auto blob = Encode(s);
	CHECK(blob.size() >= 28); // PVZU Plan 8: later fields follow (modSeed)
	PlantState back;
	REQUIRE(Decode(blob.data(), blob.size(), back));
	CHECK(back == s);
	PlantState old;
	REQUIRE(Decode(blob.data(), 12, old));
	CHECK(old.counter == 3);
	CHECK(old.frozen == 1);
	CHECK(old.sweepTimer == -1);
	CHECK(old.sweepAngle == 0);
}

TEST_CASE("ProjectileState pierce hits round-trip; 8-byte blobs read 0") {
	ProjectileState s;
	s.fireTier = 2;
	s.converted = true;
	s.pierceHits = 17;
	s.starCurve = kStarCurveSpiral0 + 3;
	auto blob = Encode(s);
	CHECK(blob.size() == 44); // Plan 6: big cabbage + butter streak; Plan 8 part 3b: mod shot + damage; 3a: modType, field80, cobAmmo
	ProjectileState back;
	REQUIRE(Decode(blob.data(), blob.size(), back));
	CHECK(back == s);
	ProjectileState old;
	REQUIRE(Decode(blob.data(), 8, old));
	CHECK(old.converted);
	CHECK(old.pierceHits == 0);
	CHECK(old.starCurve == kStarCurveNone);
}

TEST_CASE_FIXTURE(VanillaWithRules, "Fog plants stay vanilla while the fogPlants feature is off (rules loaded)") {
	CHECK_FALSE(FogPlantsOn());
	CHECK(PlanternFogRadius() == 4);
	CHECK(PlanternLaunchCounter(kSeedPeashooter, 150, 7, true) == 143);
	CHECK(PlanternHealTickCs() == 0);
	int hp = 100, maxHp = 300;
	PlanternHealStep(hp, maxHp);
	CHECK(hp == 100);
	CHECK(maxHp == 300);
	CHECK_FALSE(CactusSecondVolley(25));
	float rate = 0;
	int counter = 0;
	CHECK_FALSE(CactusShotAnim(rate, counter));
	CHECK_FALSE(CactusBecomesSpikeweed(kSeedCactus, kZamboni));
	ProjectileState spike;
	CHECK_FALSE(SpikeSurvivesHit(spike, kProjSpike));
	CHECK_FALSE(BloverStorm());
	CHECK(SplitPeaShotKind(false) == SplitShot::Pea);
	CHECK(SplitPeaShotKind(true) == SplitShot::Pea);
	CHECK(SplitPeaSweepRollRange() == 0);
	CHECK_FALSE(SplitPeaStartsSweep(0));
	PlantState s;
	s.sweepTimer = 0; // even a sweep state left in memory does nothing
	PlantState before = s;
	SplitPeaSweepTick(s);
	CHECK(s == before);
	CHECK_FALSE(SplitPeaSweeping(s));
	CHECK_FALSE(SplitPeaSweepFiresNow(s));
	CHECK(StarfruitStarSpeedMult() == 1.0f);
	CHECK(StarfruitStarCurve() == kStarCurveNone);
	CHECK(StarfruitExtraStarCount() == 0);
	float vx = 3.0f, vy = 1.0f; // even a tag left in a save does nothing
	CHECK_FALSE(StarCurveStep(kStarCurveBoomerang, 40, vx, vy));
	CHECK_FALSE(StarCurveStep(kStarCurveSpiral0, 40, vx, vy));
	CHECK(vx == 3.0f);
	CHECK(StarPierceLimit() == 0);
	ProjectileState star;
	CHECK_FALSE(StarSurvivesDie(star, StarPierceLimit()));
	CHECK(StarfruitExtraRollRange() == 0);
	CHECK_FALSE(StarfruitExtraStars(0));
	CHECK_FALSE(PumpkinExplodesWhenBitten(kSeedPumpkin, 72));
	CHECK_FALSE(PumpkinExplodesWhenSmashed(kSeedPumpkin, kGargantuar));
	CHECK_FALSE(MagnetAllRows());
	CHECK_FALSE(MagnetUnlimitedRadius());
	CHECK(CattailCobRollRange() == 0);
	CHECK_FALSE(CattailFiresCob(0));
}

TEST_CASE("Fog plants with the feature on but no rules are vanilla") {
	SetRules(nlohmann::json::object());
	SetFeature(true);
	CHECK(PlanternFogRadius() == 4);
	CHECK(PlanternLaunchCounter(kSeedPeashooter, 150, 7, true) == 143);
	CHECK(PlanternHealTickCs() == 0);
	CHECK_FALSE(CactusSecondVolley(25));
	float rate = 0;
	int counter = 0;
	CHECK_FALSE(CactusShotAnim(rate, counter));
	CHECK_FALSE(CactusBecomesSpikeweed(kSeedCactus, kZamboni));
	ProjectileState spike;
	CHECK_FALSE(SpikeSurvivesHit(spike, kProjSpike));
	CHECK_FALSE(BloverStorm());
	CHECK(SplitPeaShotKind(false) == SplitShot::Pea);
	CHECK_FALSE(SplitPeaStartsSweep(0));
	CHECK(StarfruitStarSpeedMult() == 1.0f);
	CHECK_FALSE(StarfruitExtraStars(0));
	CHECK(StarfruitStarCurve() == kStarCurveNone);
	CHECK(StarfruitExtraStarCount() == 0);
	float vx = 3.0f, vy = 1.0f;
	CHECK_FALSE(StarCurveStep(kStarCurveBoomerang, 0, vx, vy));
	CHECK_FALSE(StarCurveStep(kStarCurveSpiral0, 7, vx, vy));
	CHECK(StarPierceLimit() == 0);
	CHECK_FALSE(PumpkinExplodesWhenBitten(kSeedPumpkin, 72));
	CHECK_FALSE(PumpkinExplodesWhenSmashed(kSeedPumpkin, kGargantuar));
	CHECK_FALSE(MagnetAllRows());
	CHECK_FALSE(CattailFiresCob(0));
	SetFeature(false);
}

TEST_CASE("Fog plant pack file says the same as these test values") {
	const char* home = std::getenv("HOME");
	fs::path file = std::getenv("PVZU_PACK_DIR") ? fs::path(std::getenv("PVZU_PACK_DIR"))
	                                              : fs::path(home ? home : "") / "pvz-unnamed-pack";
	file /= "data/rules-fog-plants.json";
	if (!fs::exists(file)) { MESSAGE("pack not here, skipped: " << file.string()); return; }
	std::ifstream in(file);
	nlohmann::json pack = nlohmann::json::parse(in, nullptr, false);
	REQUIRE(pack.is_object());
	CHECK(pack == PackRules());
}

TEST_CASE_FIXTURE(VanillaWithRules, "Split Pea: with the feature off (or without the sweepOnPlant key) a new Split Pea starts idle") {
	CHECK(SplitPeaInitialSweepTimer() == -1);
	auto aRules = PackRules();
	aRules.erase("fogPlants.splitPea.sweepOnPlant");
	SetRules(aRules);
	SetFeature(true);
	CHECK(SplitPeaInitialSweepTimer() == -1);
	SetFeature(false);
}
