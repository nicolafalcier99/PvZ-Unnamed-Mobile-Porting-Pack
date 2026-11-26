// PVZU: Unnamed Night-plant rules (the research notes (w2-night) section 1, w4-fog.md X2). Rolls are injected; boundary
// rolls 0, threshold, threshold+1 and max for every chance. Every rule is also checked with the "nightPlants" feature
// off (vanilla mode) while the pack rules are loaded: it must stay vanilla.
#include "doctest/doctest.h"
#include "Mod/GameData.h"
#include "Mod/ModPack.h"
#include "Mod/Unnamed/NightPlants.h"
#include "Mod/Unnamed/Rules.h"

using namespace pvzu;
using namespace pvzu::unnamed;

namespace {
// Same values as pvz-unnamed-pack/data/rules-night-plants.json.
nlohmann::json PackRules() {
	return nlohmann::json{
		{"nightPlants.puff.freezeRollRange", 100},
		{"nightPlants.puff.freezeChanceOffset", 15},
		{"nightPlants.puff.freezeChillCs", 750},
		{"nightPlants.iceTrap.damage", 200},
		{"nightPlants.chill.armBodyDamage", 10},
		{"nightPlants.chill.armDamageFlags", 10},
		{"nightPlants.sunshroom.growCs", 9000},
		{"nightPlants.sunshroom.bonusRollRange", 100},
		{"nightPlants.sunshroom.bonusMaxRoll", 11},
		{"nightPlants.fume.attackWidthPx", 800},
		{"nightPlants.graveBuster.refundCoin", 6},
		{"nightPlants.graveBuster.bonusRollRange", 3},
		{"nightPlants.graveBuster.bonusRoll", 1},
		{"nightPlants.graveBuster.bonusCoin", 4},
		{"nightPlants.hypno.bucketHp", 1200},
		{"nightPlants.hypno.bucketTypesMask", 603979861},
		{"nightPlants.scaredy.hp", 1500},
		{"nightPlants.scaredy.secondShotAtCounter", 25},
		{"nightPlants.doom.radiusPx", 990},
		{"nightPlants.doom.rowRange", 6},
		{"nightPlants.doom.craterCs", 24000},
	};
}

void SetFeature(bool on) {
	GameData::Get().ResetToVanilla();
	if (on) {
		ModPack p;
		p.id = "test-night";
		p.features["nightPlants"] = true;
		std::string err;
		REQUIRE_MESSAGE(GameData::Get().ApplyPack(p, &err), err);
	}
}

struct UnnamedNight {
	UnnamedNight() { SetRules(PackRules()); SetFeature(true); }
	~UnnamedNight() { SetRules(nlohmann::json::object()); SetFeature(false); }
};
// Vanilla mode but the rules still loaded: proves the feature switch alone keeps vanilla.
struct VanillaWithRules {
	VanillaWithRules() { SetRules(PackRules()); SetFeature(false); }
	~VanillaWithRules() { SetRules(nlohmann::json::object()); }
};

constexpr int kPuff = 8, kSea = 24, kFume = 10;
constexpr int kNormal = 0, kFlag = 1, kCone = 2, kPole = 3, kPail = 4, kNewspaper = 5, kDoor = 6, kFootball = 7,
	kDancer = 8, kPeaHead = 26, kWallnutHead = 27, kGatlingHead = 29;
constexpr int kSun = 4, kLargeSun = 6; // engine CoinType
}

TEST_CASE_FIXTURE(UnnamedNight, "PuffFreezesEater: awake Puff Rand(100) < 23, Sea-shroom < 39, asleep never") {
	CHECK(NightPlantsOn());
	CHECK(PuffFreezeRollRange() == 100);
	CHECK(PuffFreezesEater(kPuff, false, 0));
	CHECK(PuffFreezesEater(kPuff, false, 22));       // threshold
	CHECK_FALSE(PuffFreezesEater(kPuff, false, 23)); // threshold + 1
	CHECK_FALSE(PuffFreezesEater(kPuff, false, 99));
	CHECK(PuffFreezesEater(kSea, false, 38));
	CHECK_FALSE(PuffFreezesEater(kSea, false, 39));
	CHECK_FALSE(PuffFreezesEater(kPuff, true, 0));   // asleep (Day levels): never
	CHECK_FALSE(PuffFreezesEater(kSea, true, 0));
	CHECK_FALSE(PuffFreezesEater(kFume, false, 0));  // other plants: never
	CHECK(PuffFreezeChillCs() == 750);
}

TEST_CASE_FIXTURE(UnnamedNight, "IceTrapDamage 200 and chill arm bonus 10 (flags 0x0A) only with the arm") {
	CHECK(IceTrapDamage() == 200);
	CHECK(ChillArmBodyDamage(true) == 10);
	CHECK(ChillArmBodyDamage(false) == 0);
	CHECK(ChillArmDamageFlags() == 10u);
}

TEST_CASE_FIXTURE(UnnamedNight, "SunShroom: grows in 9000 cs; grown only Rand(100) <= 11 extra small sun") {
	CHECK(SunShroomGrowCs() == 9000);
	CHECK(SunShroomBonusRollRange() == 100);
	CHECK(SunShroomBonusSun(false, 0));
	CHECK(SunShroomBonusSun(false, 11));
	CHECK_FALSE(SunShroomBonusSun(false, 12));
	CHECK_FALSE(SunShroomBonusSun(false, 99));
	CHECK_FALSE(SunShroomBonusSun(true, 0)); // small Sun-shroom: never
}

TEST_CASE_FIXTURE(UnnamedNight, "FumeAttackWidth 800 (whole lane)") {
	CHECK(FumeAttackWidth() == 800);
}

TEST_CASE_FIXTURE(UnnamedNight, "GraveBuster: large sun back, then Rand(3) == 1 a 25 sun") {
	CHECK(GraveBusterRefunds());
	CHECK(GraveBusterRefundCoin() == kLargeSun);
	CHECK(GraveBusterBonusRollRange() == 3);
	CHECK_FALSE(GraveBusterBonusSun(0));
	CHECK(GraveBusterBonusSun(1));
	CHECK_FALSE(GraveBusterBonusSun(2));
	CHECK(GraveBusterBonusCoin() == kSun);
}

TEST_CASE_FIXTURE(UnnamedNight, "HypnoGivesBucket: Normal/Cone/Bucket/Door/Pea/Gatling get 1200; Flag/Newspaper/others not") {
	for (int t : {kNormal, kCone, kPail, kDoor, kPeaHead, kGatlingHead}) {
		CAPTURE(t);
		CHECK(HypnoGivesBucket(t));
	}
	for (int t : {-1, kFlag, kPole, kNewspaper, kFootball, kDancer, kWallnutHead, 31, 32, 40}) {
		CAPTURE(t);
		CHECK_FALSE(HypnoGivesBucket(t));
	}
	CHECK(HypnoBucketHp() == 1200);
}

TEST_CASE_FIXTURE(UnnamedNight, "Scaredy: 1500 HP, second spore at launch counter 25") {
	CHECK(ScaredyHp() == 1500);
	CHECK_FALSE(ScaredySecondShot(24));
	CHECK(ScaredySecondShot(25));
	CHECK_FALSE(ScaredySecondShot(26));
	CHECK_FALSE(ScaredySecondShot(0));
}

TEST_CASE_FIXTURE(UnnamedNight, "Doom: radius 990 over 6 rows, crater 24000 cs") {
	CHECK(DoomRadius() == 990);
	CHECK(DoomRowRange() == 6);
	CHECK(DoomCraterCs() == 24000);
}

TEST_CASE_FIXTURE(VanillaWithRules, "Night plants stay vanilla while the nightPlants feature is off (rules loaded)") {
	CHECK_FALSE(NightPlantsOn());
	CHECK_FALSE(PuffFreezesEater(kPuff, false, 0));
	CHECK(IceTrapDamage() == 20); // Ice-shroom must not one-shot vanilla zombies
	CHECK(ChillArmBodyDamage(true) == 0);
	CHECK(SunShroomGrowCs() == 12000);
	CHECK_FALSE(SunShroomBonusSun(false, 0));
	CHECK(FumeAttackWidth() == 340);
	CHECK_FALSE(GraveBusterRefunds());
	CHECK_FALSE(GraveBusterBonusSun(1));
	CHECK_FALSE(HypnoGivesBucket(kNormal));
	CHECK(ScaredyHp() == 300);
	CHECK_FALSE(ScaredySecondShot(25));
	CHECK(DoomRadius() == 250);
	CHECK(DoomRowRange() == 3);
	CHECK(DoomCraterCs() == 18000);
}

TEST_CASE("Night plants with the feature on but no rules are vanilla") {
	SetRules(nlohmann::json::object());
	SetFeature(true);
	CHECK_FALSE(PuffFreezesEater(kPuff, false, 0));
	CHECK(IceTrapDamage() == 20);
	CHECK(ChillArmBodyDamage(true) == 0);
	CHECK(SunShroomGrowCs() == 12000);
	CHECK_FALSE(SunShroomBonusSun(false, 0));
	CHECK(FumeAttackWidth() == 340);
	CHECK_FALSE(GraveBusterRefunds());
	CHECK_FALSE(GraveBusterBonusSun(1));
	CHECK_FALSE(HypnoGivesBucket(kNormal));
	CHECK(ScaredyHp() == 300);
	CHECK_FALSE(ScaredySecondShot(25));
	CHECK(DoomRadius() == 250);
	CHECK(DoomRowRange() == 3);
	CHECK(DoomCraterCs() == 18000);
	SetFeature(false);
}
