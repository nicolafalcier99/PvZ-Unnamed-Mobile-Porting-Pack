// PVZU: Unnamed Roof-plant rules (the research notes (w5-roof) section 1, G8). Rolls are injected; boundary rolls for every
// chance. Every rule is also checked with the "roofPlants" feature off (vanilla mode) while the pack rules are loaded:
// it must stay vanilla. The last test compares these values with the pack file.
#include "doctest/doctest.h"
#include "Mod/GameData.h"
#include "Mod/ModPack.h"
#include "Mod/Unnamed/RoofPlants.h"
#include "Mod/Unnamed/Rules.h"
#include "Mod/Unnamed/State.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>

using namespace pvzu;
using namespace pvzu::unnamed;
namespace fs = std::filesystem;

namespace {
// Same values as pvz-unnamed-pack/data/rules-roof-plants.json (the last test compares the two).
nlohmann::json PackRules() {
	return nlohmann::json{
		{"roofPlants.cabbage.quickThrowRoll", 2},
		{"roofPlants.cabbage.quickThrowLaunch", 50},
		{"roofPlants.cabbage.quickThrowHpCost", 50},
		{"roofPlants.cabbage.quickThrowFlashCs", 100},
		{"roofPlants.cabbage.quickThrowFreeLevel", 55},
		{"roofPlants.cabbage.bigVelZ", -7.0},
		{"roofPlants.cabbage.bigAccZ", 0.1},
		{"roofPlants.cabbage.bigDamage", 400},
		{"roofPlants.cabbage.bigDrawScale", 3.0},
		{"roofPlants.kernel.cobAfterKernels", 15},
		{"roofPlants.kernel.butterBaseCs", 100},
		{"roofPlants.kernel.butterMinCs", 400},
		{"roofPlants.kernel.butterFlatCs", 500},
		{"roofPlants.kernel.butterStacks", 1},
		{"roofPlants.kernel.butterFixedMode", 70},
		{"roofPlants.garlic.hp", 900},
		{"roofPlants.garlic.poisonPeriodCs", 40},
		{"roofPlants.garlic.poisonDamage", 1},
		{"roofPlants.garlic.poisonRangePx", 120},
		{"roofPlants.garlic.poisonBossMinCol", 6},
		{"roofPlants.garlic.biteChillCs", 250},
		{"roofPlants.garlic.biteChillCap", 400},
		{"roofPlants.garlic.hypnoRoll", 250},
		{"roofPlants.marigold.shrink", 1},
		{"roofPlants.marigold.shrinkScale", 0.5},
		{"roofPlants.marigold.shrinkHpDivisor", 4},
		{"roofPlants.marigold.rangeBackPx", 120},
		{"roofPlants.marigold.rangeFrontPx", 80},
		{"roofPlants.marigold.maxTargetHp", 120000},
		{"roofPlants.marigold.chillAdd", 1},
		{"roofPlants.marigold.biteImmune", 1},
		{"roofPlants.melon.volleyBasePct", 10},
		{"roofPlants.melon.volleyWinterPct", 10},
		{"roofPlants.melon.volleyPerZombiePct", 4},
		{"roofPlants.melon.volleyAlwaysAt", 100},
		{"roofPlants.flowerPot.zombieShotsSkip", 1},
		{"roofPlants.flowerPot.noGraveOnPot", 1},
	};
}

void SetFeature(bool on) {
	GameData::Get().ResetToVanilla();
	if (on) {
		ModPack p;
		p.id = "test-roof";
		p.features["roofPlants"] = true;
		std::string err;
		REQUIRE_MESSAGE(GameData::Get().ApplyPack(p, &err), err);
	}
}

struct UnnamedRoof {
	UnnamedRoof() { SetRules(PackRules()); SetFeature(true); }
	~UnnamedRoof() { SetRules(nlohmann::json::object()); SetFeature(false); }
};
struct VanillaWithRules {
	VanillaWithRules() { SetRules(PackRules()); SetFeature(false); }
	~VanillaWithRules() { SetRules(nlohmann::json::object()); }
};

// engine enums (same numbers as the exe)
constexpr int kSeedPeashooter = 0, kSeedHypno = 12, kSeedScaredy = 13, kSeedStarfruit = 29, kSeedFlowerPot = 33,
	kSeedGarlic = 36, kSeedMarigold = 38, kSeedMelon = 39, kSeedWinterMelon = 44, kSeedCabbage = 32, kSeedKernel = 34;
constexpr int kNormal = 0, kFlag = 1, kCone = 2, kNewspaper = 5, kCatapult = 22, kBoss = 25;
constexpr int kProjCabbage = 2, kProjKernel = 10, kProjCob = 11, kProjButter = 12, kProjMelon = 3;
constexpr int kPhaseNormal = 0, kPhaseDying = 1, kPhaseBurned = 2, kPhaseMowered = 3, kPhaseReading = 29, kPhaseMad = 31,
	kPhaseDiggerTunnel = 32, kPhaseDiggerPause = 34, kPhaseImpThrown = 71, kPhaseBalloonFly = 73, kPhaseBalloonPop = 74;
constexpr int kStateScaredyHiding = 21;
}

// --- Cabbage-pult (1.1) ------------------------------------------------------------------------------------

TEST_CASE_FIXTURE(UnnamedRoof, "Cabbage: Rand(2) == 0 is a big quick throw: launch 50, HP -50, flash 100, 400 damage, x3 draw") {
	CHECK(RoofPlantsOn());
	CHECK(CabbageQuickRollRange() == 2);
	const CabbageQuick q = CabbageQuickThrow(0, 41);
	CHECK(q.bigShot);
	CHECK(q.plantCost);
	CHECK(q.launchCounter == 50);
	CHECK(q.hpCost == 50);
	CHECK(q.flashCs == 100);
	CHECK(q.velZ == doctest::Approx(-7.0f));
	CHECK(q.accZ == doctest::Approx(0.1f));
	const CabbageQuick no = CabbageQuickThrow(1, 41);
	CHECK_FALSE(no.bigShot);
	CHECK_FALSE(no.plantCost);
	// level 55: only the projectile part
	const CabbageQuick l55 = CabbageQuickThrow(0, 55);
	CHECK(l55.bigShot);
	CHECK_FALSE(l55.plantCost);
	CHECK(CabbageQuickThrow(0, 54).plantCost);
	// the big cabbage hits for 400 (vanilla 40 / pack) and is drawn x3; a plain one keeps the table value
	CHECK(BigCabbageDamage(kProjCabbage, true, 40) == 400);
	CHECK(BigCabbageDamage(kProjCabbage, false, 40) == 40);
	CHECK(BigCabbageDamage(kProjMelon, true, 80) == 80); // only type 2 (0xc05700)
	CHECK(BigCabbageDrawScale(kProjCabbage, true, 1.0f) == doctest::Approx(3.0f));
	CHECK(BigCabbageDrawScale(kProjCabbage, false, 1.0f) == doctest::Approx(1.0f));
}

// --- Kernel-pult (1.3) -------------------------------------------------------------------------------------

TEST_CASE_FIXTURE(UnnamedRoof, "Kernel: the 16th throw after 15 straight kernels is a Cob; butter/Cob carry the streak and reset it") {
	KernelShot s = KernelPityShot(0, kProjKernel);
	CHECK(s.projectileType == kProjKernel);
	CHECK(s.counter == 1);
	CHECK(s.carry == -1);
	s = KernelPityShot(14, kProjKernel);
	CHECK(s.counter == 15);
	CHECK(s.projectileType == kProjKernel);
	s = KernelPityShot(15, kProjKernel); // pity Cob whatever was rolled
	CHECK(s.projectileType == kProjCob);
	CHECK(s.counter == 0);
	CHECK(s.carry == 15);
	s = KernelPityShot(15, kProjButter);
	CHECK(s.projectileType == kProjCob);
	CHECK(s.carry == 15);
	s = KernelPityShot(3, kProjButter);
	CHECK(s.projectileType == kProjButter);
	CHECK(s.counter == 0);
	CHECK(s.carry == 3);
	s = KernelPityShot(0, kProjButter);
	CHECK(s.carry == 0);
}

TEST_CASE_FIXTURE(UnnamedRoof, "Butter (hook 720): T = 100 x 2^max(n,1), at least 400, stacks; 500 flat for a calm Newspaper / loaded Catapult") {
	// ButterCs(type, phase, ballsLeft, gameMode, fromProjectile, n, current, vanillaValue) -> new mButteredCounter
	CHECK(ButterCs(kNormal, kPhaseNormal, 0, 0, true, 0, 0, 400) == 400);
	CHECK(ButterCs(kNormal, kPhaseNormal, 0, 0, true, 1, 0, 400) == 400);
	CHECK(ButterCs(kNormal, kPhaseNormal, 0, 0, true, 2, 0, 400) == 400);
	CHECK(ButterCs(kNormal, kPhaseNormal, 0, 0, true, 3, 0, 400) == 800);
	CHECK(ButterCs(kNormal, kPhaseNormal, 0, 0, true, 4, 0, 400) == 1600);
	CHECK(ButterCs(kNormal, kPhaseNormal, 0, 0, true, 5, 0, 400) == 3200);
	// stacks
	CHECK(ButterCs(kNormal, kPhaseNormal, 0, 0, true, 3, 250, 400) == 1050);
	// no projectile (Spikeweed stun) or I, Zombie Endless (mode 70): fixed 400, still added
	CHECK(ButterCs(kNormal, kPhaseNormal, 0, 0, false, 5, 0, 400) == 400);
	CHECK(ButterCs(kNormal, kPhaseNormal, 0, 70, true, 5, 100, 400) == 500);
	// Newspaper not mad (reading / maddening): 500 set, not added; a mad one is a normal zombie here
	CHECK(ButterCs(kNewspaper, kPhaseReading, 0, 0, true, 5, 300, 400) == 500);
	CHECK(ButterCs(kNewspaper, kPhaseMad, 0, 0, true, 3, 0, 400) == 800);
	// Catapult with balls left: 500; out of balls: normal
	CHECK(ButterCs(kCatapult, kPhaseNormal, 20, 0, true, 4, 0, 400) == 500);
	CHECK(ButterCs(kCatapult, kPhaseNormal, 0, 0, true, 4, 0, 400) == 1600);
}

// --- Garlic (1.5) ------------------------------------------------------------------------------------------

TEST_CASE_FIXTURE(UnnamedRoof, "Garlic: 900 HP; poison 1 every 40 cs within rows +-1 and 120 px; a Boss when the Garlic is in column >= 6") {
	CHECK(GarlicHp(400) == 900);
	CHECK(GarlicPoisonPeriodCs() == 40);
	CHECK(GarlicPoisonDamage() == 1);
	CHECK(GarlicPoisonTick(0));
	CHECK(GarlicPoisonTick(80));
	CHECK_FALSE(GarlicPoisonTick(41));
	// GarlicPoisonReaches(zombieIsBoss, garlicCol, garlicRow, garlicX, zombieRow, zombieX)
	CHECK(GarlicPoisonReaches(false, 4, 2, 400, 2, 400));
	CHECK(GarlicPoisonReaches(false, 4, 2, 400, 1, 520));
	CHECK(GarlicPoisonReaches(false, 4, 2, 400, 3, 280));
	CHECK_FALSE(GarlicPoisonReaches(false, 4, 2, 400, 0, 400));
	CHECK_FALSE(GarlicPoisonReaches(false, 4, 2, 400, 4, 400));
	CHECK_FALSE(GarlicPoisonReaches(false, 4, 2, 400, 2, 521));
	CHECK_FALSE(GarlicPoisonReaches(false, 4, 2, 400, 2, 279));
	CHECK(GarlicPoisonReaches(true, 6, 0, 500, 4, 900));
	CHECK_FALSE(GarlicPoisonReaches(true, 5, 2, 400, 2, 400));
}

TEST_CASE_FIXTURE(UnnamedRoof, "Garlic bite: chill +250 while <= 400; Rand(250) == 0 hypnotises, never a Flag or Newspaper") {
	CHECK(GarlicBiteChill(0) == 250);
	CHECK(GarlicBiteChill(400) == 650);
	CHECK(GarlicBiteChill(401) == 401);
	CHECK(GarlicHypnoRollRange() == 250);
	CHECK(GarlicBiteRolls(kNormal));
	CHECK(GarlicBiteRolls(kCone));
	CHECK_FALSE(GarlicBiteRolls(kFlag));
	CHECK_FALSE(GarlicBiteRolls(kNewspaper));
	CHECK(GarlicBiteHypnotises(0));
	CHECK_FALSE(GarlicBiteHypnotises(1));
	CHECK_FALSE(GarlicBiteHypnotises(249));
}

// --- Marigold (1.7) ----------------------------------------------------------------------------------------

TEST_CASE_FIXTURE(UnnamedRoof, "Marigold: no coins, one shrink pulse then it dies; bites do nothing") {
	CHECK(MarigoldShrinks());
	CHECK(MarigoldBiteImmune(kSeedMarigold));
	CHECK_FALSE(MarigoldBiteImmune(kSeedPeashooter));
	// eligibility: not dead, not hypnotised, max HP <= 120000, not the Boss, not the Newspaper 0x7f boss form
	CHECK(MarigoldCanShrink(kNormal, 0, false, false, 270));
	CHECK_FALSE(MarigoldCanShrink(kNormal, 0, true, false, 270));
	CHECK_FALSE(MarigoldCanShrink(kNormal, 0, false, true, 270));
	CHECK(MarigoldCanShrink(kNormal, 0, false, false, 120000));
	CHECK_FALSE(MarigoldCanShrink(kNormal, 0, false, false, 120001));
	CHECK_FALSE(MarigoldCanShrink(kBoss, 0, false, false, 50000));
	CHECK_FALSE(MarigoldCanShrink(kNewspaper, 0x7f, false, false, 270));
	CHECK(MarigoldCanShrink(kNewspaper, 0, false, false, 270));
	// area: rows +-1, x in [mx - 120 - zombieW, mx + 80 + marigoldW]
	// MarigoldInRange(marigoldRow, marigoldX, marigoldW, zombieRow, zombieX, zombieW)
	CHECK(MarigoldInRange(2, 400, 80, 2, 400, 42));
	CHECK(MarigoldInRange(2, 400, 80, 1, 560, 42));
	CHECK_FALSE(MarigoldInRange(2, 400, 80, 2, 561, 42));
	CHECK(MarigoldInRange(2, 400, 80, 3, 238, 42));
	CHECK_FALSE(MarigoldInRange(2, 400, 80, 3, 237, 42));
	CHECK_FALSE(MarigoldInRange(2, 400, 80, 0, 400, 42));
	// effect: scale x0.5, body/helm/shield and max /4 (toward 0), flying HP /4 (not its max), chill +1
	ShrinkTarget t{1.0f, 270, 270, 1100, 1100, 1100, 1100, 21, 20, 0};
	MarigoldShrink(t);
	CHECK(t.scale == doctest::Approx(0.5f));
	CHECK(t.body == 67);
	CHECK(t.bodyMax == 67);
	CHECK(t.helm == 275);
	CHECK(t.helmMax == 275);
	CHECK(t.shield == 275);
	CHECK(t.shieldMax == 275);
	CHECK(t.flying == 5);
	CHECK(t.flyingMax == 20);
	CHECK(t.chill == 1);
	MarigoldShrink(t); // no "already shrunk" guard
	CHECK(t.scale == doctest::Approx(0.25f));
	CHECK(t.body == 16);
	ShrinkTarget neg{1.0f, -3, 7, 0, 0, 0, 0, 0, 0, 0};
	MarigoldShrink(neg);
	CHECK(neg.body == 0); // toward 0, like the exe's cdq/and/add/sar
}

// --- Melon-pult (1.8) --------------------------------------------------------------------------------------

TEST_CASE_FIXTURE(UnnamedRoof, "Melon volley: Rand(100) < 10 + 10 (Winter) + 4 per zombie on the board; always at 25 zombies") {
	CHECK(MelonVolleyNeedsRoll(kSeedMelon, 0));
	CHECK(MelonVolleyFires(kSeedMelon, 0, 9));
	CHECK_FALSE(MelonVolleyFires(kSeedMelon, 0, 10));
	CHECK(MelonVolleyFires(kSeedMelon, 3, 21));
	CHECK_FALSE(MelonVolleyFires(kSeedMelon, 3, 22));
	CHECK(MelonVolleyFires(kSeedWinterMelon, 0, 19));
	CHECK_FALSE(MelonVolleyFires(kSeedWinterMelon, 0, 20));
	CHECK(MelonVolleyFires(kSeedMelon, 22, 97));
	CHECK_FALSE(MelonVolleyFires(kSeedMelon, 22, 98));
	CHECK(MelonVolleyNeedsRoll(kSeedMelon, 24));
	CHECK_FALSE(MelonVolleyNeedsRoll(kSeedMelon, 25)); // 4n >= 100: no roll, always
	CHECK(MelonVolleyFires(kSeedMelon, 25, 99));
	CHECK_FALSE(MelonVolleyNeedsRoll(kSeedCabbage, 0));
	CHECK_FALSE(MelonVolleyFires(kSeedCabbage, 30, 0));
}

TEST_CASE_FIXTURE(UnnamedRoof, "Melon volley targets: every zombie in the row ahead, never the Boss, Imps in flight, diggers underground, balloons") {
	// MelonVolleyTarget(type, phase, dead, hypnotised, sameRow, ahead)
	CHECK(MelonVolleyTarget(kNormal, kPhaseNormal, false, false, true, true));
	CHECK_FALSE(MelonVolleyTarget(kNormal, kPhaseNormal, true, false, true, true));
	CHECK_FALSE(MelonVolleyTarget(kNormal, kPhaseNormal, false, true, true, true));
	CHECK_FALSE(MelonVolleyTarget(kNormal, kPhaseNormal, false, false, false, true));
	CHECK_FALSE(MelonVolleyTarget(kNormal, kPhaseNormal, false, false, true, false));
	CHECK_FALSE(MelonVolleyTarget(kBoss, kPhaseNormal, false, false, true, true));
	for (int phase : {kPhaseDying, kPhaseBurned, kPhaseMowered, kPhaseDiggerTunnel, kPhaseDiggerPause, kPhaseImpThrown, kPhaseBalloonFly, kPhaseBalloonPop})
		CHECK_FALSE(MelonVolleyTarget(kNormal, phase, false, false, true, true));
	CHECK(MelonVolleyTarget(kNormal, 33, false, false, true, true)); // digger rising: not excluded
}

// --- Flower Pot (1.2) --------------------------------------------------------------------------------------

TEST_CASE_FIXTURE(UnnamedRoof, "Zombie shots pass over Flower Pots, Starfruit and a hiding Scaredy-shroom; no grave on a pot") {
	CHECK(ZombieShotSkipsPlant(kSeedFlowerPot, 0));
	CHECK(ZombieShotSkipsPlant(kSeedStarfruit, 0));
	CHECK(ZombieShotSkipsPlant(kSeedScaredy, kStateScaredyHiding));
	CHECK_FALSE(ZombieShotSkipsPlant(kSeedScaredy, 0));
	CHECK_FALSE(ZombieShotSkipsPlant(kSeedPeashooter, 0));
	CHECK(NoGraveOnFlowerPot());
}

// --- vanilla ------------------------------------------------------------------------------------------------

TEST_CASE_FIXTURE(VanillaWithRules, "Roof plants in vanilla mode (feature off, rules loaded) are vanilla") {
	CHECK_FALSE(RoofPlantsOn());
	CHECK(CabbageQuickRollRange() == 0);
	CHECK_FALSE(CabbageQuickThrow(0, 41).bigShot);
	CHECK(BigCabbageDamage(kProjCabbage, true, 40) == 40);
	CHECK(BigCabbageDrawScale(kProjCabbage, true, 1.0f) == 1.0f);
	KernelShot s = KernelPityShot(15, kProjKernel);
	CHECK(s.projectileType == kProjKernel);
	CHECK(s.counter == 15);
	CHECK(s.carry == -1);
	CHECK(ButterCs(kNormal, kPhaseNormal, 0, 0, true, 5, 300, 400) == 400);
	CHECK(ButterCs(kNewspaper, kPhaseMad, 0, 0, true, 0, 0, 500) == 500); // keeps whatever the caller passed
	CHECK(GarlicHp(400) == 400);
	CHECK(GarlicPoisonPeriodCs() == 0);
	CHECK_FALSE(GarlicPoisonTick(0));
	CHECK(GarlicBiteChill(0) == 0);
	CHECK(GarlicHypnoRollRange() == 0);
	CHECK_FALSE(GarlicBiteRolls(kNormal));
	CHECK_FALSE(GarlicBiteHypnotises(0));
	CHECK_FALSE(MarigoldShrinks());
	CHECK_FALSE(MarigoldBiteImmune(kSeedMarigold));
	CHECK_FALSE(MelonVolleyNeedsRoll(kSeedMelon, 0));
	CHECK_FALSE(MelonVolleyFires(kSeedMelon, 30, 0));
	CHECK_FALSE(ZombieShotSkipsPlant(kSeedFlowerPot, 0));
	CHECK_FALSE(NoGraveOnFlowerPot());
	(void)kSeedHypno; (void)kSeedGarlic; (void)kSeedKernel;
}

TEST_CASE("Roof plants with the feature on but no rules are vanilla") {
	SetRules(nlohmann::json::object());
	SetFeature(true);
	CHECK(CabbageQuickRollRange() == 0);
	CHECK_FALSE(CabbageQuickThrow(0, 41).bigShot);
	CHECK(KernelPityShot(15, kProjKernel).projectileType == kProjKernel);
	CHECK(ButterCs(kNormal, kPhaseNormal, 0, 0, true, 5, 300, 400) == 400);
	CHECK(GarlicHp(400) == 400);
	CHECK(GarlicPoisonPeriodCs() == 0);
	CHECK(GarlicBiteChill(0) == 0);
	CHECK_FALSE(GarlicBiteHypnotises(0));
	CHECK_FALSE(MarigoldShrinks());
	CHECK_FALSE(MelonVolleyFires(kSeedMelon, 30, 0));
	CHECK_FALSE(ZombieShotSkipsPlant(kSeedFlowerPot, 0));
	SetFeature(false);
}

TEST_CASE("ProjectileState keeps the Roof fields through a save; older blobs read none") {
	ProjectileState s;
	s.fireTier = 2;
	s.bigCabbage = true;
	s.butterStreak = 7;
	const auto b = Encode(s);
	ProjectileState out;
	REQUIRE(Decode(b.data(), b.size(), out));
	CHECK(out == s);
	ProjectileState old;
	old.fireTier = 1;
	old.pierceHits = 3;
	auto ob = Encode(old);
	ob.resize(16); // a Plan 5 blob
	REQUIRE(Decode(ob.data(), ob.size(), out));
	CHECK(out.pierceHits == 3);
	CHECK_FALSE(out.bigCabbage);
	CHECK(out.butterStreak == 0);
}

TEST_CASE("Roof plant pack file says the same as these test values") {
	const char* home = std::getenv("HOME");
	fs::path file = std::getenv("PVZU_PACK_DIR") ? fs::path(std::getenv("PVZU_PACK_DIR"))
	                                              : fs::path(home ? home : "") / "pvz-unnamed-pack";
	file /= "data/rules-roof-plants.json";
	if (!fs::exists(file)) { MESSAGE("pack not here, skipped: " << file.string()); return; }
	std::ifstream in(file);
	nlohmann::json pack = nlohmann::json::parse(in, nullptr, false);
	REQUIRE(pack.is_object());
	CHECK(pack == PackRules());
}
