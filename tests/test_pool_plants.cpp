// PVZU: Unnamed Pool-plant rules (the research notes (w3-pool) P1-P8). Rolls are injected; boundary rolls 0, threshold,
// threshold+1 and max for every chance. Every rule is also checked with the "poolPlants" feature off (vanilla mode)
// while the pack rules are loaded: it must stay vanilla. Torchwood tiers are walked through whole Torchwood rows with
// the engine's own mHitTorchwoodGridX bookkeeping, so a double conversion would show as a wrong tier.
#include "doctest/doctest.h"
#include "Mod/GameData.h"
#include "Mod/ModPack.h"
#include "Mod/Unnamed/DayPlants.h"
#include "Mod/Unnamed/PoolPlants.h"
#include "Mod/Unnamed/Rules.h"
#include "Mod/Unnamed/State.h"

using namespace pvzu;
using namespace pvzu::unnamed;

namespace {
// Same values as pvz-unnamed-pack/data/rules-pool-plants.json (barrage cut to its first and last shot of each block),
// plus the Day fire-tier keys the Torchwood tiers share (pvz-unnamed-pack/data/rules-day-plants.json).
nlohmann::json PackRules() {
	return nlohmann::json{
		{"poolPlants.squash.reachPx", 105},
		{"poolPlants.squash.reachEatingPx", 136},
		{"poolPlants.squash.surviveRoll", 100},
		{"poolPlants.squash.surviveBelow", 27},
		{"poolPlants.threepeater.roll", 3},
		{"poolPlants.threepeater.snowRoll", 1},
		{"poolPlants.threepeater.fireRoll", 2},
		{"poolPlants.tangle.drags", 3},
		{"poolPlants.tangle.anyGround", 1},
		{"poolPlants.spikeweed.anyGround", 1},
		{"poolPlants.tangle.maxTargetHp", 120000},
		{"poolPlants.jalapeno.burnsColumn", 1},
		{"poolPlants.jalapeno.columnFireDx", 40},
		{"poolPlants.jalapeno.iceCutDx", 80},
		{"poolPlants.jalapeno.iceRoll", 100},
		{"poolPlants.jalapeno.iceMax", 17},
		{"poolPlants.jalapeno.fullHp", 300},
		{"poolPlants.jalapeno.barrageConversions", 2},
		{"poolPlants.jalapeno.barrageMotion", 7},
		{"poolPlants.jalapeno.barrageRangeFlags", 1},
		{"poolPlants.jalapeno.barrageRenderOrder", 342525},
		{"poolPlants.jalapeno.barrage", nlohmann::json::array({
			nlohmann::json::array({-55, 32, -0.4, -0.3, -0.25}),
			nlohmann::json::array({-55, 32, 0.4, 0.0, -0.25}),
			nlohmann::json::array({115, 32, 0.0, -0.3, 0.2}),
			nlohmann::json::array({115, 32, 0.1, 0.525, 0.2}),
		})},
		{"poolPlants.spikeweed.stunRoll", 3},
		{"poolPlants.spikeweed.stunSkip", 1},
		{"poolPlants.spikeweed.stunCs", 30},
		{"poolPlants.torch.maxTier", 5},
		{"poolPlants.torch.tier2TintR", 40}, {"poolPlants.torch.tier2TintG", 200}, {"poolPlants.torch.tier2TintB", 240},
		{"poolPlants.torch.tier4TintR", 139}, {"poolPlants.torch.tier4TintG", 1}, {"poolPlants.torch.tier4TintB", 139},
		{"poolPlants.torch.tier5TintR", 240}, {"poolPlants.torch.tier5TintG", 60}, {"poolPlants.torch.tier5TintB", 60},
		{"poolPlants.tallnut.regenHp", 75},
		{"poolPlants.tallnut.regenTicks", 100},
		{"poolPlants.tallnut.thorns", 10},
		{"poolPlants.tallnut.thornsFlags", 0},
		{"poolPlants.tallnut.smashDamage", 2400},
		// Day keys the tiers use (one source, not duplicated in the Pool file)
		{"dayPlants.snowPea.fireTier", 3},
		{"dayPlants.fireTier.chillCs", 750},
		{"dayPlants.fireTier.chillMinTier", 3},
		{"dayPlants.fireTier.chillMaxTier", 5},
		{"dayPlants.fireTier.tier3TintR", 122}, {"dayPlants.fireTier.tier3TintG", 230}, {"dayPlants.fireTier.tier3TintB", 143},
		{"dayPlants.fireTier.damage1", 40}, {"dayPlants.fireTier.damage2", 60}, {"dayPlants.fireTier.damage3", 40},
		{"dayPlants.fireTier.damage4", 60}, {"dayPlants.fireTier.damage5", 80},
	};
}

void SetFeature(bool on) {
	GameData::Get().ResetToVanilla();
	if (on) {
		ModPack p;
		p.id = "test-pool";
		p.features["poolPlants"] = true;
		p.features["dayPlants"] = true;
		std::string err;
		REQUIRE_MESSAGE(GameData::Get().ApplyPack(p, &err), err);
	}
}

struct UnnamedPool {
	UnnamedPool() { SetRules(PackRules()); SetFeature(true); }
	~UnnamedPool() { SetRules(nlohmann::json::object()); SetFeature(false); }
};
struct VanillaWithRules {
	VanillaWithRules() { SetRules(PackRules()); SetFeature(false); }
	~VanillaWithRules() { SetRules(nlohmann::json::object()); }
};

// engine enums (same numbers as the exe)
constexpr int kSeedWallnut = 3, kSeedIceshroom = 14, kSeedThreepeater = 18, kSeedTanglekelp = 19, kSeedSpikeweed = 21,
	kSeedSeashroom = 24, kSeedLilypad = 16, kSeedTallnut = 23, kSeedSpikerock = 46, kSeedCattail = 43;
constexpr int kNormal = 0, kPail = 4, kDucky = 10, kSnorkel = 11, kZamboni = 12, kDolphin = 14, kBungee = 20,
	kGargantuar = 23, kBoss = 25, kSquashHead = 30, kRedeye = 32;
constexpr int kPhaseNormal = 0, kPhaseDying = 1, kPhaseBurned = 2, kPhaseMowered = 3;
constexpr int kProjPea = 0, kProjSnow = 1, kProjFireball = 6, kProjPuff = 4;
constexpr int kBgDay = 0, kBgNight = 1, kBgPool = 2, kBgFog = 3, kBgRoof = 4;

// One pass of a projectile through a Torchwood in column `col`, as Plant::UpdateTorchwood + Projectile::ConvertTo*
// do it (one call per Torchwood tick while the projectile overlaps it). Returns the projectile type afterwards.
struct Shot {
	int type = kProjPea;
	int hitTorchwoodGridX = -1;
	ProjectileState s;
};
void TorchwoodTick(Shot& p, int col) {
	switch (TorchwoodAction(p.type, p.s.fireTier)) {
	case TorchAction::ToFireball:
		if (FireballConvert(p.s, p.hitTorchwoodGridX, col)) {
			p.type = kProjFireball;
			p.hitTorchwoodGridX = col;
		}
		break;
	case TorchAction::ToPea:
		if (p.hitTorchwoodGridX != col) { // vanilla ConvertToPea guard
			p.type = kProjPea;
			p.hitTorchwoodGridX = col;
		}
		break;
	case TorchAction::None:
		break;
	}
}
void PassTorchwood(Shot& p, int col) {
	for (int tick = 0; tick < 4; ++tick) // the overlap lasts several ticks; every tick must be idempotent
		TorchwoodTick(p, col);
}
}

TEST_CASE_FIXTURE(UnnamedPool, "Squash: reach 105 / 136 eating; Rand(100) < 27 stays to smash again") {
	CHECK(PoolPlantsOn());
	CHECK(SquashReach(false) == 105);
	CHECK(SquashReach(true) == 136);
	CHECK(SquashSurviveRollRange() == 100);
	CHECK(SquashSurvives(0));
	CHECK(SquashSurvives(26));       // threshold
	CHECK_FALSE(SquashSurvives(27)); // threshold + 1
	CHECK_FALSE(SquashSurvives(99));
}

TEST_CASE_FIXTURE(UnnamedPool, "Threepeater: each pea Rand(3): 0 pea, 1 snow pea, 2 fire pea") {
	CHECK(ThreepeaterPeaRollRange() == 3);
	CHECK(ThreepeaterPeaKind(0) == PeaKind::Pea);
	CHECK(ThreepeaterPeaKind(1) == PeaKind::Snow);
	CHECK(ThreepeaterPeaKind(2) == PeaKind::Fire);
}

TEST_CASE_FIXTURE(UnnamedPool, "Tangle Kelp: three drags, then it dies") {
	CHECK(TangleKelpDrags() == 3);
	CHECK(TangleKelpReadyAgain(1));
	CHECK(TangleKelpReadyAgain(2));
	CHECK_FALSE(TangleKelpReadyAgain(3));
}

TEST_CASE_FIXTURE(UnnamedPool, "Tangle Kelp: any ground; only an imitater under it or a plant on it refuses") {
	CHECK(TangleKelpPlacement(kSeedTanglekelp, false, false) == TanglePlacement::Ok);
	CHECK(TangleKelpPlacement(kSeedTanglekelp, true, false) == TanglePlacement::NotHere);
	CHECK(TangleKelpPlacement(kSeedTanglekelp, false, true) == TanglePlacement::NotHere);
	// every other seed keeps the vanilla rules, also the other aquatic plants and Cattail (same hook, not world 3)
	for (int seed : {kSeedLilypad, kSeedSeashroom, kSeedCattail, kSeedWallnut, kSeedSpikeweed, kSeedTallnut}) {
		CAPTURE(seed);
		CHECK(TangleKelpPlacement(seed, false, false) == TanglePlacement::Vanilla);
	}
}

TEST_CASE_FIXTURE(UnnamedPool, "Tangle Kelp targets: pool as vanilla; land skips Gargantuar, Giga, HP > 120000; never Boss/Bungee") {
	CHECK_FALSE(TangleKelpSkipsTarget(kDucky, true, 270));
	CHECK_FALSE(TangleKelpSkipsTarget(kSnorkel, true, 270));
	CHECK_FALSE(TangleKelpSkipsTarget(kDolphin, true, 870));
	CHECK_FALSE(TangleKelpSkipsTarget(kGargantuar, true, 3000)); // in the pool only Boss/Bungee are skipped
	CHECK(TangleKelpSkipsTarget(kBoss, true, 40000));
	CHECK(TangleKelpSkipsTarget(kBungee, true, 450));
	// on land
	CHECK_FALSE(TangleKelpSkipsTarget(kNormal, false, 270));
	CHECK_FALSE(TangleKelpSkipsTarget(kPail, false, 270));
	CHECK_FALSE(TangleKelpSkipsTarget(kZamboni, false, 1350));
	CHECK_FALSE(TangleKelpSkipsTarget(kNormal, false, 120000)); // boundary
	CHECK(TangleKelpSkipsTarget(kNormal, false, 120001));
	CHECK(TangleKelpSkipsTarget(kGargantuar, false, 3000));
	CHECK(TangleKelpSkipsTarget(kRedeye, false, 6000));
	CHECK(TangleKelpSkipsTarget(kBoss, false, 40000));
	CHECK(TangleKelpSkipsTarget(kBungee, false, 450));
}

TEST_CASE_FIXTURE(UnnamedPool, "Jalapeno: column on 5 lanes (6 on pool/fog), round-half-even column test, ice cut 80") {
	CHECK(JalapenoBurnsColumn());
	CHECK(JalapenoColumnLanes(kBgDay) == 5);
	CHECK(JalapenoColumnLanes(kBgNight) == 5);
	CHECK(JalapenoColumnLanes(kBgPool) == 6);
	CHECK(JalapenoColumnLanes(kBgFog) == 6);
	CHECK(JalapenoColumnLanes(kBgRoof) == 5);
	CHECK(JalapenoColumnFireDx() == 40);
	CHECK(JalapenoIceCutDx() == 80);
	// columns round(x/80)-1 .. round((x+w)/80)-1
	CHECK(JalapenoColumnHits(300, 20, 3));        // 3.75 -> 4 -1 = 3 .. 4 - 1 = 3
	CHECK_FALSE(JalapenoColumnHits(300, 20, 2));
	CHECK_FALSE(JalapenoColumnHits(300, 20, 4));
	CHECK(JalapenoColumnHits(250, 90, 2));        // 3.125 -> 2 .. 4.25 -> 3
	CHECK(JalapenoColumnHits(250, 90, 3));
	CHECK_FALSE(JalapenoColumnHits(250, 90, 4));
	CHECK(JalapenoColumnHits(200, 0, 1));         // 2.5 rounds to even 2 (x87 default), not 3
	CHECK_FALSE(JalapenoColumnHits(200, 0, 2));
	CHECK(JalapenoColumnHits(280, 0, 3));         // 3.5 rounds to even 4
	CHECK(JalapenoColumnHits(-60, 40, -2));       // left of the lawn: -0.75 -> -1 - 1
}

TEST_CASE_FIXTURE(UnnamedPool, "Jalapeno: untouched 300/300 only, Rand(100) <= 17 (18%) leaves an Ice-shroom") {
	CHECK(JalapenoIceRollRange() == 100);
	CHECK(JalapenoIceEligible(300, 300));
	CHECK_FALSE(JalapenoIceEligible(299, 300)); // bitten
	CHECK_FALSE(JalapenoIceEligible(450, 450)); // raised max HP (adventure HP scaling) refuses, as the exe
	CHECK(JalapenoLeavesIce(0));
	CHECK(JalapenoLeavesIce(17));       // threshold
	CHECK_FALSE(JalapenoLeavesIce(18)); // threshold + 1
	CHECK_FALSE(JalapenoLeavesIce(99));
	CHECK(JalapenoIceSeed() == kSeedIceshroom);
}

TEST_CASE_FIXTURE(UnnamedPool, "Jalapeno barrage: shots from the pack list, vx gets the helper's -0.25 / +0.2") {
	const auto shots = JalapenoBarrage();
	REQUIRE(shots.size() == 4);
	CHECK(shots[0].dx == -55);
	CHECK(shots[0].dy == 32);
	CHECK(shots[0].vx == -0.4f - 0.25f);
	CHECK(shots[0].vy == -0.3f);
	CHECK(shots[1].vx == 0.4f - 0.25f);
	CHECK(shots[1].vy == 0.0f);
	CHECK(shots[2].dx == 115);
	CHECK(shots[2].vx == 0.2f);
	CHECK(shots[3].vx == 0.1f + 0.2f);
	CHECK(shots[3].vy == 0.525f);
	CHECK(JalapenoBarrageConversions() == 2);
	CHECK(JalapenoBarrageMotion() == 7);
	CHECK(JalapenoBarrageRangeFlags() == 1);
	CHECK(JalapenoBarrageRenderOrder() == 342525);
	// two conversions of a fresh pea = tier 2 (blue, 60)
	Shot p;
	for (int g = 0; g < JalapenoBarrageConversions(); ++g)
		if (FireballConvert(p.s, p.hitTorchwoodGridX, g))
			p.hitTorchwoodGridX = g;
	CHECK(p.s.fireTier == 2);
	CHECK(FireTierDamage(p.s.fireTier, 40) == 60);
}

TEST_CASE_FIXTURE(UnnamedPool, "Jalapeno barrage: a bad entry is skipped, not a crash") {
	auto rules = PackRules();
	rules["poolPlants.jalapeno.barrage"] = nlohmann::json::array({nlohmann::json::array({1, 2}), "x",
		nlohmann::json::array({-55, 32, -0.4, -0.3, -0.25})});
	SetRules(rules);
	CHECK(JalapenoBarrage().size() == 1);
	rules["poolPlants.jalapeno.barrage"] = 5;
	SetRules(rules);
	CHECK(JalapenoBarrage().empty());
}

TEST_CASE_FIXTURE(UnnamedPool, "Spikeweed / Spikerock: eligible hit, then Rand(3) != 1 (2/3) butters for 30 cs") {
	CHECK(SpikeweedStunEligible(kSeedSpikeweed, true, 0, kPhaseNormal));
	CHECK(SpikeweedStunEligible(kSeedSpikerock, true, 0, kPhaseNormal));
	CHECK_FALSE(SpikeweedStunEligible(kSeedTallnut, true, 0, kPhaseNormal));
	CHECK_FALSE(SpikeweedStunEligible(kSeedSpikeweed, false, 0, kPhaseNormal)); // no head
	CHECK_FALSE(SpikeweedStunEligible(kSeedSpikeweed, true, 1, kPhaseNormal));  // already buttered
	for (int ph : {kPhaseDying, kPhaseBurned, kPhaseMowered}) {
		CAPTURE(ph);
		CHECK_FALSE(SpikeweedStunEligible(kSeedSpikeweed, true, 0, ph));
	}
	CHECK(SpikeweedStunEligible(kSeedSpikeweed, true, 0, 4));
	CHECK(SpikeweedStunRollRange() == 3);
	CHECK(SpikeweedStuns(0));
	CHECK_FALSE(SpikeweedStuns(1));
	CHECK(SpikeweedStuns(2));
	CHECK(SpikeweedStunCs() == 30);
}

TEST_CASE_FIXTURE(UnnamedPool, "Torchwood: pea path fire (1) -> blue (2) -> red (5), red passes untouched") {
	Shot p;
	PassTorchwood(p, 3);
	CHECK(p.type == kProjFireball);
	CHECK(p.s.fireTier == 1);
	PassTorchwood(p, 4);
	CHECK(p.s.fireTier == 2);
	PassTorchwood(p, 5);
	CHECK(p.s.fireTier == 5);
	CHECK(TorchwoodAction(kProjFireball, 5) == TorchAction::None);
	PassTorchwood(p, 6);
	CHECK(p.s.fireTier == 5);
	CHECK(FireTierDamage(1, 40) == 40);
	CHECK(FireTierDamage(2, 40) == 60);
	CHECK(FireTierDamage(5, 40) == 80);
	CHECK_FALSE(FireTierChills(1));
	CHECK_FALSE(FireTierChills(2));
	CHECK(FireTierChills(5));
}

TEST_CASE_FIXTURE(UnnamedPool, "Torchwood: snow pea thaws and turns green (3) at the SAME Torchwood, then purple (4), red (5)") {
	Shot p;
	p.type = kProjSnow;
	PassTorchwood(p, 2);
	CHECK(p.type == kProjFireball);
	CHECK(p.s.fireTier == 3);
	CHECK(p.s.converted);
	PassTorchwood(p, 3);
	CHECK(p.s.fireTier == 4);
	PassTorchwood(p, 4);
	CHECK(p.s.fireTier == 5);
	PassTorchwood(p, 5);
	CHECK(p.s.fireTier == 5);
	CHECK(FireTierChills(3));
	CHECK(FireTierChills(4));
	CHECK(FireTierChillCs() == 750);
}

TEST_CASE_FIXTURE(UnnamedPool, "Torchwood with Threepeater snow / fire peas and Snow Pea fire shots: no double conversion") {
	// Threepeater fire pea: ConvertToFireball(not -1, not a column) on a fresh pea -> tier 1
	Shot fire;
	REQUIRE(FireballConvert(fire.s, fire.hitTorchwoodGridX, -2));
	fire.hitTorchwoodGridX = -2;
	fire.type = kProjFireball;
	CHECK(fire.s.fireTier == 1);
	PassTorchwood(fire, 3);
	CHECK(fire.s.fireTier == 2); // one step per Torchwood, not two
	PassTorchwood(fire, 4);
	CHECK(fire.s.fireTier == 5);
	// Threepeater snow pea: same as a Snow Pea's pea
	Shot snow;
	snow.type = kProjSnow;
	PassTorchwood(snow, 3);
	CHECK(snow.s.fireTier == 3);
	// Snow Pea 34% fire shot: the exe marks tier -1 then converts -> 3 (the Day "ice-fire ball")
	Shot ice;
	ice.s.fireTier = SnowPeaFireShotStartTier();
	CHECK(ice.s.fireTier == -1);
	REQUIRE(FireballConvert(ice.s, ice.hitTorchwoodGridX, -2));
	ice.hitTorchwoodGridX = -2;
	ice.type = kProjFireball;
	CHECK(ice.s.fireTier == 3);
	PassTorchwood(ice, 3);
	CHECK(ice.s.fireTier == 4);
	PassTorchwood(ice, 4);
	CHECK(ice.s.fireTier == 5);
	// other projectiles never convert
	CHECK(TorchwoodAction(kProjPuff, 0) == TorchAction::None);
}

TEST_CASE_FIXTURE(UnnamedPool, "Fire tier tints: 1 vanilla look, 2 blue, 3 green (Day key), 4 purple, 5 red") {
	int r = -1, g = -1, b = -1;
	CHECK_FALSE(FireTierLook(1, r, g, b));
	REQUIRE(FireTierLook(2, r, g, b));
	CHECK((r == 40 && g == 200 && b == 240));
	REQUIRE(FireTierLook(3, r, g, b));
	CHECK((r == 122 && g == 230 && b == 143));
	REQUIRE(FireTierLook(4, r, g, b));
	CHECK((r == 139 && g == 1 && b == 139));
	REQUIRE(FireTierLook(5, r, g, b));
	CHECK((r == 240 && g == 60 && b == 60));
	CHECK_FALSE(FireTierLook(0, r, g, b));
	CHECK_FALSE(FireTierLook(6, r, g, b));
}

TEST_CASE_FIXTURE(UnnamedPool, "Tall-nut: +75 HP after 100 unchanged ticks (capped), any change restarts; not in Zen Garden") {
	PlantState s;
	int hp = 5000;
	hp = TallnutRegenTick(s, hp, 8000, false); // first tick only records the HP
	CHECK(hp == 5000);
	for (int i = 0; i < 99; ++i)
		hp = TallnutRegenTick(s, hp, 8000, false);
	CHECK(hp == 5000);
	CHECK(s.counter == 99);
	hp = TallnutRegenTick(s, hp, 8000, false); // 100th unchanged tick
	CHECK(hp == 5075);
	CHECK(s.counter == 0);
	CHECK(s.extra == 5075);
	hp = TallnutRegenTick(s, 5000, 8000, false); // bitten: restarts
	CHECK(s.counter == 0);
	for (int i = 0; i < 100; ++i)
		hp = TallnutRegenTick(s, hp, 8000, false);
	CHECK(hp == 5075);
	PlantState top;
	int full = 7990;
	for (int i = 0; i < 101; ++i)
		full = TallnutRegenTick(top, full, 8000, false);
	CHECK(full == 8000); // cap
	PlantState zen;
	int z = 5000;
	for (int i = 0; i < 300; ++i)
		z = TallnutRegenTick(zen, z, 8000, true);
	CHECK(z == 5000);
}

TEST_CASE_FIXTURE(UnnamedPool, "Tall-nut: 10 thorn damage per chew; Gargantuar/Giga/Squash-head smash -2400 (dies on the 4th)") {
	CHECK(TallnutThornsDamage(kSeedTallnut) == 10);
	CHECK(TallnutThornsDamage(kSeedWallnut) == 0);
	CHECK(TallnutThornsFlags() == 0u);
	CHECK(TallnutSmashed(kSeedTallnut, kGargantuar));
	CHECK(TallnutSmashed(kSeedTallnut, kRedeye));
	CHECK(TallnutSmashed(kSeedTallnut, kSquashHead));
	CHECK_FALSE(TallnutSmashed(kSeedTallnut, kZamboni)); // drive-over still squishes (vanilla)
	CHECK_FALSE(TallnutSmashed(kSeedWallnut, kGargantuar));
	CHECK_FALSE(TallnutSmashed(0x67, kGargantuar)); // cave 0x651a45 compares raw 0x17: not the Charmed Tall-nut
	CHECK(TallnutSmashDamage() == 2400);
	int hp = 8000;
	for (int i = 0; i < 3; ++i)
		hp -= TallnutSmashDamage();
	CHECK(hp == 800);
	CHECK(hp - TallnutSmashDamage() < 0); // Plant::Update kills a plant below 0 HP
}

TEST_CASE("ProjectileState: converted flag round-trips; 4-byte blobs of older saves read converted = false") {
	ProjectileState s;
	s.fireTier = 4;
	s.converted = true;
	auto blob = Encode(s);
	CHECK(blob.size() == 44); // 8 in Plan 4; Plan 5 pierce + spare hit, Plan 6 Roof fields, Plan 8 part 3b/3a mod shot fields
	ProjectileState back;
	REQUIRE(Decode(blob.data(), blob.size(), back));
	CHECK(back == s);
	ProjectileState old;
	REQUIRE(Decode(blob.data(), 4, old));
	CHECK(old.fireTier == 4);
	CHECK_FALSE(old.converted);
}

TEST_CASE_FIXTURE(VanillaWithRules, "Pool plants stay vanilla while the poolPlants feature is off (rules loaded)") {
	CHECK_FALSE(PoolPlantsOn());
	CHECK(SquashReach(false) == 70);
	CHECK(SquashReach(true) == 110);
	CHECK_FALSE(SquashSurvives(0));
	CHECK(ThreepeaterPeaRollRange() == 0);
	CHECK(ThreepeaterPeaKind(1) == PeaKind::Pea);
	CHECK(ThreepeaterPeaKind(2) == PeaKind::Pea);
	CHECK(TangleKelpDrags() == 1);
	CHECK_FALSE(TangleKelpReadyAgain(1));
	CHECK(TangleKelpPlacement(kSeedTanglekelp, false, false) == TanglePlacement::Vanilla);
	CHECK(TangleKelpSkipsTarget(kNormal, false, 270));      // vanilla: never on land
	CHECK_FALSE(TangleKelpSkipsTarget(kDucky, true, 270));  // vanilla: any pool zombie
	CHECK_FALSE(TangleKelpSkipsTarget(kGargantuar, true, 3000));
	CHECK_FALSE(JalapenoBurnsColumn());
	CHECK_FALSE(JalapenoIceEligible(300, 300));
	CHECK_FALSE(JalapenoLeavesIce(0));
	CHECK(JalapenoBarrage().empty());
	CHECK_FALSE(SpikeweedStunEligible(kSeedSpikeweed, true, 0, kPhaseNormal));
	CHECK_FALSE(SpikeweedStuns(0));
	CHECK(TorchwoodAction(kProjPea, 0) == TorchAction::ToFireball);
	CHECK(TorchwoodAction(kProjSnow, 0) == TorchAction::ToPea);
	CHECK(TorchwoodAction(kProjFireball, 1) == TorchAction::None);
	// vanilla Torchwood: a snow pea thaws at the first Torchwood and burns at the next one, no tiers
	Shot p;
	p.type = kProjSnow;
	PassTorchwood(p, 2);
	CHECK(p.type == kProjPea);
	CHECK(p.s.fireTier == 0);
	PassTorchwood(p, 3);
	CHECK(p.type == kProjFireball);
	CHECK(p.s.fireTier == 0);
	CHECK_FALSE(p.s.converted);
	CHECK(SnowPeaFireShotStartTier() == 3); // Day-only behaviour: the tier is set directly
	int r, g, b;
	CHECK_FALSE(FireTierLook(2, r, g, b));
	PlantState s;
	int hp = 5000;
	for (int i = 0; i < 300; ++i)
		hp = TallnutRegenTick(s, hp, 8000, false);
	CHECK(hp == 5000);
	CHECK(TallnutThornsDamage(kSeedTallnut) == 0);
	CHECK_FALSE(TallnutSmashed(kSeedTallnut, kGargantuar));
}

TEST_CASE("Pool plants with the feature on but no rules are vanilla") {
	SetRules(nlohmann::json::object());
	SetFeature(true);
	CHECK(SquashReach(false) == 70);
	CHECK(SquashReach(true) == 110);
	CHECK_FALSE(SquashSurvives(0));
	CHECK(ThreepeaterPeaRollRange() == 0);
	CHECK(TangleKelpDrags() == 1);
	CHECK(TangleKelpPlacement(kSeedTanglekelp, false, false) == TanglePlacement::Vanilla);
	CHECK(TangleKelpSkipsTarget(kNormal, false, 270));
	CHECK_FALSE(JalapenoBurnsColumn());
	CHECK_FALSE(JalapenoIceEligible(300, 300));
	CHECK(JalapenoBarrage().empty());
	CHECK_FALSE(SpikeweedStuns(0));
	CHECK(TorchwoodAction(kProjFireball, 1) == TorchAction::None);
	PlantState s;
	int hp = 5000;
	for (int i = 0; i < 300; ++i)
		hp = TallnutRegenTick(s, hp, 8000, false);
	CHECK(hp == 5000);
	CHECK(TallnutThornsDamage(kSeedTallnut) == 0);
	CHECK_FALSE(TallnutSmashed(kSeedTallnut, kGargantuar));
	SetFeature(false);
}

// V2 (C24): Board::CanPlantAt 0x40e230 `cmp esi,0x15` + 2 NOPs (the vanilla `je` to the needs-ground test is gone),
// hook 49 0x95a800 on a roof square: Spikeweed with no pot -> on, with a pot -> refused (eax 5, PLANTING_NEEDS_POT)
TEST_CASE_FIXTURE(UnnamedPool, "Spikeweed: no needs-ground test; on the roof only without a pot") {
	CHECK_FALSE(SpikeweedNeedsGround(kSeedSpikeweed));
	CHECK(SpikeweedNeedsGround(kSeedSpikerock));
	CHECK(SpikeweedRoofPot(kSeedSpikeweed, false) == RoofPot::Ok);
	CHECK(SpikeweedRoofPot(kSeedSpikeweed, true) == RoofPot::Refused);
	CHECK(SpikeweedRoofPot(kSeedSpikerock, false) == RoofPot::Vanilla);
	CHECK(SpikeweedRoofPot(kSeedWallnut, true) == RoofPot::Vanilla);
}

TEST_CASE_FIXTURE(VanillaWithRules, "Spikeweed: vanilla needs ground and a pot on the roof") {
	CHECK(SpikeweedNeedsGround(kSeedSpikeweed));
	CHECK(SpikeweedNeedsGround(kSeedSpikerock));
	CHECK_FALSE(SpikeweedNeedsGround(kSeedWallnut));
	CHECK(SpikeweedRoofPot(kSeedSpikeweed, false) == RoofPot::Vanilla);
}
