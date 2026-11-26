// PVZU: Unnamed mod-only plant rules (the research notes (mod-plants-behaviour)). Rolls are injected; boundary rolls for
// every chance. Every rule is also checked with the "modPlants" feature off (vanilla mode) while the pack rules are
// loaded: it must have no effect. The last test compares these values with the pack file.
#include "doctest/doctest.h"
#include "Mod/GameData.h"
#include "Mod/ModPack.h"
#include "Mod/Unnamed/ModPlantRules.h"
#include "Mod/Unnamed/Rules.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <vector>

using namespace pvzu;
using namespace pvzu::unnamed;
namespace fs = std::filesystem;

namespace {
// Same values as pvz-unnamed-pack/data/rules-mod-plants.json (the last test compares the two).
const char* kPackRulesJson = R"json(
{
	"modPlants.shadow.on": 1,
	"modPlants.shadow.skipModeA": 43,
	"modPlants.shadow.skipModeB": 50,
	"modPlants.shadow.rangeCells": 1,
	"modPlants.shadow.moonflowerPoint": 1,
	"modPlants.shadow.shadowedMoonflowerBonus": 1,
	"modPlants.shadow.thresholdDefault": 1,
	"modPlants.shadow.thresholdMoonflower": 3,
	"modPlants.shadow.thresholdFireDragon": 3,
	"modPlants.shadow.thresholdDreamStarfruit": 3,
	"modPlants.shadow.thresholdCharmedTallnut": 2,
	"modPlants.shadow.tintR": 80,
	"modPlants.shadow.tintG": 80,
	"modPlants.shadow.tintB": 240,
	"modPlants.shadow.tintA": 255,
	"modPlants.shadowPeashooter.on": 1,
	"modPlants.shadowPeashooter.projectileType": 1,
	"modPlants.shadowPeashooter.originDx": 66,
	"modPlants.shadowPeashooter.originDy": 10,
	"modPlants.shadowPeashooter.potDy": -5,
	"modPlants.shadowPeashooter.beamDamage": 30,
	"modPlants.shadowPeashooter.beamDamageFlags": 6,
	"modPlants.shadowPeashooter.beamPuffBase": 7,
	"modPlants.shadowPeashooter.beamPuffStepPx": 80,
	"modPlants.modSun.on": 1,
	"modPlants.modSun.timerJitter": 150,
	"modPlants.primalSunflower.sunCoin": 4,
	"modPlants.primalSunflower.shadowSunCoin": 6,
	"modPlants.primalSunflower.mainMotion": 2,
	"modPlants.primalSunflower.extraCoin": 5,
	"modPlants.primalSunflower.extraMotion": 3,
	"modPlants.primalSunflower.packetRoll": 25,
	"modPlants.primalSunflower.packetBelow": 5,
	"modPlants.primalSunflower.packetCoin": 16,
	"modPlants.primalSunflower.packetSeeds": [
		1,
		9,
		41,
		81,
		89
	],
	"modPlants.moonflower.sunCoin": 5,
	"modPlants.moonflower.shadowSunCoin": 4,
	"modPlants.moonflower.coinMotion": 3,
	"modPlants.toxicShadowShroom.normalKind": 1,
	"modPlants.toxicShadowShroom.shadowKind": 2,
	"modPlants.toxicShadowShroom.poisonTimer": 524287,
	"modPlants.toxicShadowShroom.poisonPeriod": 64,
	"modPlants.toxicShadowShroom.poisonDamage": 20,
	"modPlants.toxicShadowShroom.shadowFlashCs": 25,
	"modPlants.dodgeZombieShots.on": 1,
	"modPlants.repeaterPoison.damagePerTick": 1,
	"modPlants.duskPult.projectileType": 33,
	"modPlants.duskPult.shadowRows": 1,
	"modPlants.duskPult.aboveDy": -80.0,
	"modPlants.duskPult.aboveDz": 80.0,
	"modPlants.duskPult.splashRows": 1,
	"modPlants.duskPult.splashBackPx": 152,
	"modPlants.duskPult.splashWidenPx": 256,
	"modPlants.duskPult.splashDamage": 13,
	"modPlants.duskPult.splashFlags": 1,
	"modPlants.duskPult.splashPushPx": 5.0,
	"modPlants.duskPult.splashWeakenCs": 200,
	"modPlants.elementShroom.firstElementCs": 200,
	"modPlants.elementShroom.elementPeriodCs": 800,
	"modPlants.elementShroom.shootingCounter": 200,
	"modPlants.elementShroom.pulseCounters": [
		126,
		98,
		70
	],
	"modPlants.elementShroom.puffCounters": [
		136,
		108,
		80
	],
	"modPlants.elementShroom.pulseDamage": 16,
	"modPlants.elementShroom.wideDamage": 12,
	"modPlants.elementShroom.fireDamage": 56,
	"modPlants.elementShroom.wideRowReach": 2,
	"modPlants.elementShroom.wideRectHalf": 160,
	"modPlants.elementShroom.wideRectSize": 400,
	"modPlants.elementShroom.fireStripRoll": 5,
	"modPlants.elementShroom.iceRoll": 18,
	"modPlants.elementShroom.iceDamageFlags": 6,
	"modPlants.elementShroom.iceFreezeCs": 150,
	"modPlants.elementShroom.icePushPx": 20,
	"modPlants.elementShroom.icePushMaxX": 800,
	"modPlants.elementShroom.healPerPulse": 30,
	"modPlants.elementShroom.healMaxCap": 3000,
	"modPlants.elementShroom.noPlanternBoost": 1,
	"modPlants.prismShroom.clickToFire": 1,
	"modPlants.prismShroom.shotCost": 50,
	"modPlants.prismShroom.shootingCounter": 25,
	"modPlants.prismShroom.outOfSunFlashCs": 70,
	"modPlants.prismShroom.projectileType": 22,
	"modPlants.prismShroom.damageBase": 300,
	"modPlants.prismShroom.damageSunCap": 1000,
	"modPlants.prismShroom.damageSunDivisor": 4,
	"modPlants.dreamStarfruit.colours": 5,
	"modPlants.dreamStarfruit.colourRerollMask": 12,
	"modPlants.dreamStarfruit.palette": [
		3383753,
		16711680,
		12583167,
		7833753,
		65280
	],
	"modPlants.dreamStarfruit.damageBonus": [
		15,
		20,
		15,
		25,
		15
	],
	"modPlants.dreamStarfruit.shadowReturnCs": 30,
	"modPlants.dreamStarfruit.shadowAlwaysTargets": 1,
	"modPlants.dreamStarfruit.redPierceHits": 5,
	"modPlants.dreamStarfruit.greyPierceHits": 3,
	"modPlants.dreamStarfruit.blueFreezeRoll": 7,
	"modPlants.dreamStarfruit.redHelmRoll": 7,
	"modPlants.dreamStarfruit.redHelmMaxHp": 120000,
	"modPlants.dreamStarfruit.purpleHypnoRoll": 10,
	"modPlants.dreamStarfruit.purpleHypnoRollIZombie": 20,
	"modPlants.dreamStarfruit.greenBlowRoll": 7,
	"modPlants.dreamStarfruit.greenBlowRollIZombie": 21,
	"modPlants.dreamStarfruit.effectMaxHp": 15000,
	"modPlants.dreamStarfruit.greyWeakenCs": 400,
	"modPlants.dreamStarfruit.weakenDamageMult": 2,
	"modPlants.dreamStarfruit.weakenFootballRoll": 2,
	"modPlants.spiritTail.roll": 36,
	"modPlants.spiritTail.projectileTypes": [
		0,
		1,
		2,
		3,
		4,
		5,
		7,
		8,
		10,
		11,
		12,
		22,
		33,
		34,
		35
	],
	"modPlants.spiritTail.bigCabbageRoll": 2,
	"modPlants.spiritTail.starKindRoll": 6,
	"modPlants.spiritTail.prismDamage": 475,
	"modPlants.spiritTail.motion": 9,
	"modPlants.mysticIce.shotOffsetX": 69,
	"modPlants.mysticIce.shotOffsetY": 10,
	"modPlants.mysticIce.shotType": 1,
	"modPlants.mysticIce.shotMotion": 7,
	"modPlants.mysticIce.shotRangeFlags": 1,
	"modPlants.mysticIce.shotVelX": 3.33,
	"modPlants.mysticIce.velYRoll": 2000,
	"modPlants.mysticIce.velYDivisor": 650,
	"modPlants.mysticIce.velYOffset": 1.53846,
	"modPlants.mysticIce.convertPeas": 1,
	"modPlants.mysticIce.convertTo": 1,
	"modPlants.mysticIce.lifeIsLaunchRate": 1,
	"modPlants.mysticIce.deathParticle": 71,
	"modPlants.mysticIce.upgradeFrom": 0,
	"modPlants.mysticIce.upgradeTo": 5,
	"modPlants.zomboniImmune.elementAndMysticIce": 1,
	"modPlants.sniper.periodCs": 500,
	"modPlants.sniper.warnAtCs": 120,
	"modPlants.sniper.fireAtCs": 5,
	"modPlants.sniper.damage": 500,
	"modPlants.sniper.warnFoley": 82,
	"modPlants.sniper.fireFoley": 55,
	"modPlants.sniper.bossMaxHp": 120000,
	"modPlants.primalRepeater.shotRoll": 10,
	"modPlants.primalRepeater.secondPea": 1,
	"modPlants.primalRepeater.knockbackDamage": 40,
	"modPlants.primalRepeater.poisonDamage": 15,
	"modPlants.primalRepeater.knockbackPx": 30,
	"modPlants.primalRepeater.poisonAddCs": 500,
	"modPlants.shrapnelCherry.shards": 1,
	"modPlants.shrapnelCherry.straightSpeed": 4.0,
	"modPlants.shrapnelCherry.diagonalSpeed": 2.8,
	"modPlants.shrapnelCherry.shardHitDamage": 250,
	"modPlants.shrapnelCherry.coreReplantAgeCs": 1400,
	"modPlants.shrapnelCherry.replantColumnPx": 80,
	"modPlants.shrapnelCherry.squishExplodes": 1,
	"modPlants.loadedCannon.selfArms": 0,
	"modPlants.loadedCannon.loadable": 1,
	"modPlants.loadedCannon.packetPenaltyCs": 6000,
	"modPlants.loadedCannon.kernelBurnCap": 3000,
	"modPlants.loadedCannon.doomBurnCap": 3600,
	"modPlants.loadedCannon.doomClearsGraves": 1,
	"modPlants.butterPult.alwaysButter": 1,
	"modPlants.butterPult.damageRangeFlags": 13,
	"modPlants.bowlingWallnut.rollsAnywhere": 1,
	"modPlants.bowlingWallnut.notEaten": 1,
	"modPlants.bowlingWallnut.fixedAnimRate": 1,
	"modPlants.initAsBase": 1,
	"modPlants.bossClassMinMaxHp": 120000,
	"modPlants.rewindGrass.blowDelayCs": 50,
	"modPlants.rewindGrass.blowAnimRate": 20.0,
	"modPlants.rewindGrass.maxX": 800,
	"modPlants.rewindGrass.backX": 800.0,
	"modPlants.rewindGrass.untouchable": 1,
	"modPlants.charmedTallnut.firstSummonHp": 1500,
	"modPlants.charmedTallnut.summonStepHp": 1200,
	"modPlants.charmedTallnut.summonTypes": [
		5,
		7,
		23
	],
	"modPlants.charmedTallnut.summonBodyHpMul": 1.35,
	"modPlants.charmedTallnut.riseCounterCs": 50,
	"modPlants.charmedTallnut.shadowBiteMul": 1.5,
	"modPlants.charmedTallnut.shadowThreshold": 2,
	"modPlants.charmedTallnut.gargMinHpFrac": 0.75,
	"modPlants.charmedTallnut.gargPlantCostFrac": 0.4,
	"modPlants.charmedTallnut.regenDelayCs": 100,
	"modPlants.charmedTallnut.regenHp": 75,
	"modPlants.charmedTallnut.biteBackDamage": 8,
	"modPlants.charmedTallnut.biteBackCharmRoll": 200,
	"modPlants.charmedTallnut.instantCharmVariant": 49,
	"modPlants.charmedTallnut.actsAsTallnut": 1,
	"modPlants.hallucination.impMaxHp": 270,
	"modPlants.magicDevourer.fastCountdownFrom": 10,
	"modPlants.magicDevourer.swallowAtCountdown": 5,
	"modPlants.magicDevourer.maxCol": 8,
	"modPlants.magicDevourer.maxRow": 5,
	"modPlants.magicDevourer.cardCoinType": 16,
	"modPlants.magicDevourer.cardCoinMotion": 2,
	"modPlants.holyFirePumpkin.actsAsPumpkin": 1,
	"modPlants.holyFirePumpkin.freezeGuard": 1,
	"modPlants.fireDragon.rowReach": 1,
	"modPlants.fireDragon.rangeBackPx": 40,
	"modPlants.fireDragon.rangeFrontPx": 240,
	"modPlants.fireDragon.bossMinCol": 7,
	"modPlants.fireDragon.damageFlags": 9,
	"modPlants.fireDragon.attackCs": 70,
	"modPlants.fireDragon.hitAtCountdown": 12,
	"modPlants.fireDragon.damageBase": 100,
	"modPlants.fireDragon.damageMaxHpDivisor": 15,
	"modPlants.fireDragon.damageCap": 600,
	"modPlants.fireDragon.flameBudget": 1800,
	"modPlants.fireDragon.shadowPoison": 400,
	"modPlants.fireDragon.clearsChill": 1,
	"modPlants.fireDragon.zomboniFreezeImmune": 1,
	"modPlants.goldenSunShroom.burstCs": 150,
	"modPlants.goldenSunShroom.dropAtCs": [
		125,
		75,
		25
	],
	"modPlants.goldenSunShroom.coinType": 6,
	"modPlants.goldenSunShroom.coinMotion": 2,
	"modPlants.goldenSunShroom.coinMotionAlt": 3,
	"modPlants.goldenSunShroom.coinMotionCoinAtCs": 75,
	"modPlants.goldenSunShroom.offMode": 31,
	"modPlants.goldenSunShroom.packetStartCounter": 3500,
	"modPlants.strongCoffee.triggerCountdown": 1,
	"modPlants.strongCoffee.wakeRadius": 1,
	"modPlants.strongCoffee.wakeCs": 100,
	"modPlants.strongCoffee.anyPlantOk": 1,
	"modPlants.coffeeHeal.tenths": 3,
	"modPlants.coffeeHeal.cap": 300,
	"modPlants.leafShield.rowReach": 1,
	"modPlants.leafShield.bandInsidePx": 30,
	"modPlants.leafShield.bandPx": 40,
	"modPlants.leafShield.hpCostShift": 3,
	"modPlants.leafShield.hpCostBase": 2,
	"modPlants.leafShield.reflectedFlags": 1,
	"modPlants.leafShield.flashCs": 25,
	"modPlants.projectileDamage.starDouble": 1,
	"modPlants.projectileDamage.byType": [
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		40,
		50,
		40,
		25,
		300,
		40,
		20,
		30,
		-1,
		20,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		100,
		40,
		40,
		15
	],
	"modPlants.bossFlower.rollRange": 128,
	"modPlants.bossFlower.coinType": 16,
	"modPlants.bossFlower.coinMotion": 2,
	"modPlants.bossFlower.seedTable": [
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		1,
		1,
		1,
		0,
		1,
		1,
		1,
		1,
		1,
		0,
		1,
		1,
		1,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		0,
		1,
		0,
		1,
		0,
		1,
		1,
		0,
		1,
		0,
		1,
		1,
		0,
		1,
		1,
		0,
		1,
		0,
		1,
		1,
		0,
		0,
		0,
		0
	]
}
)json";

nlohmann::json PackRules() {
	return nlohmann::json::parse(kPackRulesJson);
}

void SetModPlantsFeature(bool on) {
	GameData::Get().ResetToVanilla();
	if (on) {
		ModPack p;
		p.id = "test-mod-plants";
		p.features["modPlants"] = true;
		std::string err;
		REQUIRE_MESSAGE(GameData::Get().ApplyPack(p, &err), err);
	}
}

struct ModPlantsOnFixture {
	ModPlantsOnFixture() { SetRules(PackRules()); SetModPlantsFeature(true); }
	~ModPlantsOnFixture() { SetRules(nlohmann::json::object()); SetModPlantsFeature(false); }
};
struct VanillaWithRulesFixture {
	VanillaWithRulesFixture() { SetRules(PackRules()); SetModPlantsFeature(false); }
	~VanillaWithRulesFixture() { SetRules(nlohmann::json::object()); }
};
} // namespace

TEST_CASE_FIXTURE(ModPlantsOnFixture, "Mod plants: feature switch and seed range") {
	CHECK(ModPlantsOn());
	CHECK(IsModSeed(0x50));
	CHECK(IsModSeed(0x7f));
	CHECK_FALSE(IsModSeed(0x4f));
	CHECK_FALSE(IsModSeed(0x80));
}

TEST_CASE_FIXTURE(VanillaWithRulesFixture, "Mod plants: off in vanilla mode") {
	CHECK_FALSE(ModPlantsOn());
}

// --- Group A ------------------------------------------------------------------------------------------------

TEST_CASE_FIXTURE(ModPlantsOnFixture, "Shadow state: list, 3x3 Moonflower points, thresholds 3/3/3/2/1, tint") {
	CHECK(IsShadowPlant(0x50));
	CHECK(IsShadowPlant(0x51));
	CHECK(IsShadowPlant(0x56));
	CHECK(IsShadowPlant(0x58));
	CHECK(IsShadowPlant(0x67));
	CHECK(IsShadowPlant(0x69));
	CHECK(IsShadowPlant(0x6d));
	CHECK(IsShadowPlant(0x70));
	CHECK_FALSE(IsShadowPlant(0x7a)); // Element-shroom reuses +0xcc for its element, not shadow
	CHECK_FALSE(IsShadowPlant(0));
	CHECK(ShadowUpdateRunsInMode(0));
	CHECK_FALSE(ShadowUpdateRunsInMode(0x2b));
	CHECK_FALSE(ShadowUpdateRunsInMode(0x32));
	// a Shadow Peashooter counts Moonflowers only, +1 more for a shadowed one
	CHECK(ShadowNeighbourPoints(0x50, 0x69, false, false, false, 1, -1) == 1);
	CHECK(ShadowNeighbourPoints(0x50, 0x69, true, false, false, 0, 1) == 2);
	CHECK(ShadowNeighbourPoints(0x50, 0x70, true, false, false, 0, 1) == 0);
	CHECK(ShadowNeighbourPoints(0x50, 0x69, false, false, false, 2, 0) == 0); // outside the 3x3
	CHECK(ShadowNeighbourPoints(0x50, 0x69, false, true, false, 0, 1) == 0);  // squished
	CHECK(ShadowNeighbourPoints(0x50, 0x00, false, false, false, 0, 1) == 0); // vanilla plant
	// a Moonflower counts every shadow neighbour once
	CHECK(ShadowNeighbourPoints(0x69, 0x50, false, false, false, -1, 1) == 1);
	CHECK(ShadowNeighbourPoints(0x69, 0x69, true, false, false, 0, 1) == 1);
	CHECK(ShadowNeighbourPoints(0x69, 0x69, false, false, true, 0, 0) == 0); // itself
	CHECK(ShadowThreshold(0x69) == 3);
	CHECK(ShadowThreshold(0x56) == 3);
	CHECK(ShadowThreshold(0x6d) == 3);
	CHECK(ShadowThreshold(0x67) == 2);
	CHECK(ShadowThreshold(0x50) == 1);
	CHECK(ShadowThreshold(0x00) == 0);
	CHECK(ShadowStateFromPoints(0x50, 1));
	CHECK_FALSE(ShadowStateFromPoints(0x50, 0));
	CHECK_FALSE(ShadowStateFromPoints(0x69, 2));
	CHECK(ShadowStateFromPoints(0x69, 3));
	CHECK_FALSE(ShadowStateFromPoints(0x67, 1));
	CHECK(ShadowStateFromPoints(0x67, 2));
	const ShadowTint t = ShadowPlantTint(0x50, true);
	CHECK(t.apply);
	CHECK(t.r == 80);
	CHECK(t.g == 80);
	CHECK(t.b == 240);
	CHECK(t.a == 255);
	CHECK_FALSE(ShadowPlantTint(0x50, false).apply);
}

TEST_CASE_FIXTURE(ModPlantsOnFixture, "Mod projectile damage table 0x7b0890, weakened x2") {
	CHECK(ModProjectileDamage(0, 20, 0, false) == 20); // vanilla types untouched
	CHECK(ModProjectileDamage(0x21, 0, 0, false) == 40);
	CHECK(ModProjectileDamage(0x22, 0, 0, false) == 40);
	CHECK(ModProjectileDamage(0x23, 0, 0, false) == 15);
	CHECK(ModProjectileDamage(0x12, 0, 0, false) == 300);
	CHECK(ModProjectileDamage(0x20, 0, 0, false) == 100);
	CHECK(ModProjectileDamage(0x16, 0, 425, false) == 425);
	CHECK(ZombieWeakenedDamage(30, 1, 0, 0, 0) == 60);
	CHECK(ZombieWeakenedDamage(30, 0, 0, 0, 0) == 30);
	CHECK(ZombieWeakenTick(200) == 199);
	CHECK(ZombieWeakenTick(0) == 0);
}

TEST_CASE_FIXTURE(ModPlantsOnFixture, "Shadow Peashooter: snow pea normally, 30-damage chilling beam in shadow") {
	ShadowPeashooterShot s = ShadowPeashooterFire(0x50, false, 2);
	CHECK(s.handled);
	CHECK_FALSE(s.beam);
	CHECK(s.projectileType == 1);
	CHECK(s.originDx == 66);
	CHECK(s.originDy == 10);
	CHECK(s.potDy == -5);
	s = ShadowPeashooterFire(0x50, true, 2);
	CHECK(s.beam);
	CHECK(s.beamDamage == 30);
	CHECK(s.beamDamageFlags == 6); // hits shield and body + freeze
	CHECK(s.beamPuffs == 6);       // 1 + (7 - row 2)
	CHECK(s.beamPuffStepPx == 80);
	CHECK(ShadowPeashooterFire(0x50, true, 9).beamPuffs == 1);
	CHECK_FALSE(ShadowPeashooterFire(0, true, 2).handled);
}

TEST_CASE_FIXTURE(ModPlantsOnFixture, "Primal Sunflower: 25+15, shadow 50+15 with Rand(25) < 5 a sun-plant card") {
	CHECK(ModSunPlantProduces(0x51));
	CHECK(ModSunPlantProduces(0x69));
	CHECK_FALSE(ModSunPlantProduces(1));
	CHECK(ModSunPlantTimerRollRange() == 151);
	CHECK(ModSunPlantNextTimer(2500, 0) == 2350);
	CHECK(ModSunPlantNextTimer(2500, 150) == 2500);
	CHECK(PrimalSunflowerPacketRollRange(false) == 0);
	CHECK(PrimalSunflowerPacketRollRange(true) == 25);
	ModSunDrops d = PrimalSunflowerProduce(false, 0);
	CHECK(d.first.coinType == 4);
	CHECK(d.first.motion == 2);
	CHECK(d.second.coinType == 5);
	CHECK(d.second.motion == 3);
	d = PrimalSunflowerProduce(true, 5); // first roll without a card
	CHECK(d.first.coinType == 6);
	CHECK(d.first.usableSeed == -1);
	CHECK(d.second.coinType == 5);
	d = PrimalSunflowerProduce(true, 0);
	CHECK(d.first.coinType == 16);
	CHECK(d.first.usableSeed == 1);
	CHECK(PrimalSunflowerProduce(true, 1).first.usableSeed == 9);
	CHECK(PrimalSunflowerProduce(true, 2).first.usableSeed == 0x29);
	CHECK(PrimalSunflowerProduce(true, 3).first.usableSeed == 0x51);
	CHECK(PrimalSunflowerProduce(true, 4).first.usableSeed == 0x59); // last roll with a card
	CHECK(PrimalSunflowerProduce(true, 24).first.coinType == 6);
}

TEST_CASE_FIXTURE(ModPlantsOnFixture, "Moonflower: small sun, a normal sun in shadow, motion COIN") {
	ModSunDrops d = MoonflowerProduce(false);
	CHECK(d.first.coinType == 5);
	CHECK(d.first.motion == 3);
	CHECK(d.second.coinType == 0);
	CHECK(MoonflowerProduce(true).first.coinType == 4);
}

TEST_CASE_FIXTURE(ModPlantsOnFixture, "Toxic Shadow-shroom: eaten -> poison 20 per 64 cs, shadow ignores armour") {
	ToxicPoisonSet p = ToxicShadowShroomEaten(0x58, false, 0);
	CHECK(p.apply);
	CHECK(p.kind == 1);
	CHECK(p.timer == 0x7ffff);
	CHECK(ToxicShadowShroomEaten(0x58, true, 1).kind == 2);  // upgrade
	CHECK_FALSE(ToxicShadowShroomEaten(0x58, false, 2).apply); // kind 2 is kept
	CHECK_FALSE(ToxicShadowShroomEaten(8, true, 0).apply);
	ToxicPoisonStep s = ToxicPoisonTick(1, 0x7ffff, false); // not on a 64 boundary
	CHECK(s.damage == 0);
	CHECK(s.timer == 0x7fffe);
	s = ToxicPoisonTick(1, 0x7ffc0, false); // first hit, 63 cs after the bite
	CHECK(s.damage == 20);
	CHECK_FALSE(s.bodyOnly);
	CHECK(s.flashCs == 0);
	s = ToxicPoisonTick(2, 64, false);
	CHECK(s.damage == 20);
	CHECK(s.bodyOnly);
	CHECK(s.flashCs == 25);
	s = ToxicPoisonTick(2, 1, false); // runs out
	CHECK(s.kind == 0);
	CHECK(s.timer == 0);
	CHECK(ToxicPoisonTick(1, 64, true).damage == 0); // boss-class zombies are immune
	CHECK(ToxicPoisonTick(3, 64, false).damage == 0);
	CHECK(ModPlantDodgesZombieShots(0x58));
	CHECK(ModPlantDodgesZombieShots(0x59));
	CHECK(ModPlantDodgesZombieShots(0x6d));
	CHECK_FALSE(ModPlantDodgesZombieShots(0x50));
}

TEST_CASE_FIXTURE(ModPlantsOnFixture, "Repeater poison tick: 1 damage per cs (the +500 cs per 0x23 hit is PrimalPoisonAddCs)") {
	RepeaterPoisonStep s = RepeaterPoisonTick(500);
	CHECK(s.damage == 1);
	CHECK(s.counter == 499);
	CHECK(RepeaterPoisonTick(0).damage == 0);
}

TEST_CASE_FIXTURE(ModPlantsOnFixture, "Dusk-pult: shot 0x21 (40 + 3-row splash 13, push 5, weaken 200), shadow 3 rows") {
	CHECK(DuskPultProjectileType(0x70, 2) == 0x21);
	CHECK(DuskPultProjectileType(0x20, 2) == 2);
	CHECK_FALSE(DuskPultLooksInAdjacentRows(0x70, false));
	CHECK(DuskPultLooksInAdjacentRows(0x70, true));
	DuskPultVolley v = DuskPultExtraRows(0x70, true);
	CHECK(v.above);
	CHECK(v.below);
	CHECK(v.aboveDy == doctest::Approx(-80.0f));
	CHECK(v.aboveDz == doctest::Approx(80.0f));
	CHECK_FALSE(DuskPultExtraRows(0x70, false).above);
	// splash: rows +-1, rect x - 152 .. x + w + 104
	CHECK(DuskShardSplashReaches(0x21, 2, 400, 40, 1, 300, 20, false));
	CHECK(DuskShardSplashReaches(0x21, 2, 400, 40, 3, 500, 20, false));
	CHECK_FALSE(DuskShardSplashReaches(0x21, 2, 400, 40, 4, 400, 20, false)); // two rows away
	CHECK_FALSE(DuskShardSplashReaches(0x21, 2, 400, 40, 2, 228, 20, false)); // ends at 248 = x - 152
	CHECK(DuskShardSplashReaches(0x21, 2, 400, 40, 2, 229, 20, false));
	CHECK_FALSE(DuskShardSplashReaches(0x21, 2, 400, 40, 2, 544, 20, false)); // starts at x - 152 + 296
	CHECK(DuskShardSplashReaches(0x21, 2, 400, 40, 2, 543, 20, false));
	CHECK_FALSE(DuskShardSplashReaches(0x21, 2, 400, 40, 2, 400, 20, true)); // hypnotised
	CHECK_FALSE(DuskShardSplashReaches(2, 2, 400, 40, 2, 400, 20, false));
	const DuskSplash s = DuskShardSplash(0x21);
	CHECK(s.damage == 13);
	CHECK(s.damageFlags == 1);
	CHECK(s.pushPx == doctest::Approx(5.0f));
	CHECK(s.weakenCs == 200);
	CHECK(DuskShardSplash(2).damage == 0);
}

TEST_CASE_FIXTURE(VanillaWithRulesFixture, "Group A rules do nothing with the modPlants feature off") {
	CHECK_FALSE(IsShadowPlant(0x50));
	CHECK_FALSE(ShadowUpdateRunsInMode(0));
	CHECK(ShadowNeighbourPoints(0x50, 0x69, true, false, false, 0, 1) == 0);
	CHECK(ShadowThreshold(0x69) == 0);
	CHECK_FALSE(ShadowStateFromPoints(0x50, 5));
	CHECK_FALSE(ShadowPlantTint(0x50, true).apply);
	CHECK(ModProjectileDamage(0x21, 7, 0, false) == 7);
	CHECK(ZombieWeakenedDamage(30, 100, 0, 0, 0) == 30);
	CHECK_FALSE(ShadowPeashooterFire(0x50, true, 2).handled);
	CHECK_FALSE(ModSunPlantProduces(0x51));
	CHECK(ModSunPlantTimerRollRange() == 0);
	CHECK(PrimalSunflowerPacketRollRange(true) == 0);
	CHECK(PrimalSunflowerProduce(true, 0).first.coinType == 0);
	CHECK(MoonflowerProduce(true).first.coinType == 0);
	CHECK_FALSE(ToxicShadowShroomEaten(0x58, true, 0).apply);
	CHECK(ToxicPoisonTick(1, 64, false).damage == 0);
	CHECK_FALSE(ModPlantDodgesZombieShots(0x58));
	CHECK(RepeaterPoisonTick(500).damage == 0);
	CHECK(DuskPultProjectileType(0x70, 2) == 2);
	CHECK_FALSE(DuskPultExtraRows(0x70, true).above);
	CHECK_FALSE(DuskShardSplashReaches(0x21, 2, 400, 40, 2, 400, 20, false));
	CHECK(DuskShardSplash(0x21).damage == 0);
}

// ---- fork B: Element-shroom ---------------------------------------------------------------------------------

TEST_CASE_FIXTURE(ModPlantsOnFixture, "Element-shroom: state from the last three elements (0 fire, 1 ice)") {
	CHECK(ElementShroomState(0, 0) == kElementNone);
	CHECK(ElementShroomState(7, 0) == kElementNone); // nothing gained yet
	CHECK(ElementShroomState(0, 1) == kElementFire3);
	CHECK(ElementShroomState(7, 3) == kElementIce3);
	for (int b : {1, 2, 4})
		CHECK(ElementShroomState(b, 5) == kElementFire2Ice1);
	for (int b : {3, 5, 6})
		CHECK(ElementShroomState(b, 5) == kElementIce2Fire1);
}

TEST_CASE_FIXTURE(ModPlantsOnFixture, "Element-shroom: first gain fills three slots, later gains shift one in") {
	ElementGain g = ElementShroomGainElement(0, 1, 0);
	CHECK(g.bits == 7);
	CHECK(g.count == 1);
	g = ElementShroomGainElement(0, 0, 0);
	CHECK(g.bits == 0);
	CHECK(g.count == 1);
	g = ElementShroomGainElement(7, 0, 1); // ice ice ice + fire
	CHECK(g.bits == 6);
	CHECK(g.count == 2);
	g = ElementShroomGainElement(6, 0, 2);
	CHECK(g.bits == 4);
	g = ElementShroomGainElement(4, 1, 3);
	CHECK(g.bits == 1);
	g = ElementShroomGainElement(1, 1, 0x7fff); // count saturates
	CHECK(g.count == 0x7fff);
	CHECK(g.bits == 3);
}

TEST_CASE_FIXTURE(ModPlantsOnFixture, "Element-shroom: timer 200 then 800, gain on 1, toggle, digit") {
	ElementTick t = ElementShroomTick(0, 0);
	CHECK(t.resetCountdownTo == 200);
	CHECK_FALSE(t.gain);
	t = ElementShroomTick(0, 4);
	CHECK(t.resetCountdownTo == 800);
	t = ElementShroomTick(1, 0);
	CHECK(t.gain);
	CHECK(t.resetCountdownTo == -1);
	t = ElementShroomTick(2, 0);
	CHECK_FALSE(t.gain);
	CHECK(ElementShroomToggleMode(0) == 1);
	CHECK(ElementShroomToggleMode(1) == 0);
	CHECK(ElementShroomCountdownDigit(800) == 9);
	CHECK(ElementShroomCountdownDigit(899) == 9);
	CHECK(ElementShroomCountdownDigit(900) == 0);
	CHECK(ElementShroomCountdownDigit(1) == 1);
	CHECK(ElementShroomShootingCounter() == 200);
}

TEST_CASE_FIXTURE(ModPlantsOnFixture, "Element-shroom: three pulses, wide state reach and rect") {
	for (int c : {126, 98, 70})
		CHECK(ElementShroomPulseAt(c));
	for (int c : {136, 108, 80})
		CHECK(ElementShroomPuffAt(c));
	CHECK_FALSE(ElementShroomPulseAt(52));
	CHECK_FALSE(ElementShroomPulseAt(136));
	CHECK(ElementShroomRowReach(kElementFire2Ice1) == 2);
	CHECK(ElementShroomRowReach(kElementIce3) == 1);
	const ElementRect r = ElementShroomAttackRect(kElementFire2Ice1, 300, 200);
	CHECK(r.own);
	CHECK(r.x == 140);
	CHECK(r.y == 40);
	CHECK(r.w == 400);
	CHECK(r.h == 400);
	CHECK_FALSE(ElementShroomAttackRect(kElementFire3, 300, 200).own);
}

TEST_CASE_FIXTURE(ModPlantsOnFixture, "Element-shroom: hit damage per state, fire strip, ice freeze and push") {
	CHECK(ElementShroomHit(kElementNone, 20, 2, 0, 0, 500, false).damage == 16);
	CHECK(ElementShroomHit(kElementIce2Fire1, 20, 2, 0, 0, 500, false).damage == 16);
	CHECK(ElementShroomHit(kElementFire2Ice1, 20, 2, 0, 0, 500, false).damage == 12);
	ElementHit f = ElementShroomHit(kElementFire3, 20, 2, 0, 0, 500, false);
	CHECK(f.damage == 56);
	CHECK(f.thaw);
	CHECK(f.stripRollRange == 5);
	CHECK(ElementShroomHit(kElementFire3, 20, 2, 0, 0, 500, true).stripRollRange == 0); // boss class
	CHECK(ElementShroomFireStrip(0, true, true) == kElementStripHelm);
	CHECK(ElementShroomFireStrip(0, false, true) == kElementStripShield);
	CHECK(ElementShroomFireStrip(0, false, false) == kElementStripNone);
	CHECK(ElementShroomFireStrip(1, true, true) == kElementStripNone);
	CHECK(ElementShroomFireStrip(4, true, true) == kElementStripNone);
	CHECK(ElementShroomIceRollRange(kElementIce3) == 18);
	CHECK(ElementShroomIceRollRange(kElementFire3) == 0);
	ElementHit i0 = ElementShroomHit(kElementIce3, 20, 2, 0, 0, 500, false);
	CHECK(i0.freeze);
	CHECK(i0.damage == 0);
	CHECK(i0.iceTrapCs == 150);
	CHECK(i0.damageFlags == 6);
	ElementHit i1 = ElementShroomHit(kElementIce3, 20, 2, 1, 0, 500, false);
	CHECK(i1.pushPx == 20);
	CHECK(i1.damage == 16);
	CHECK(ElementShroomHit(kElementIce3, 20, 2, 1, 0, 800, false).pushPx == 0); // x >= 800
	CHECK(ElementShroomHit(kElementIce3, 20, 2, 1, 20, 500, false).pushPx == 0); // Bungee
	CHECK(ElementShroomHit(kElementIce3, 20, 2, 1, 25, 500, false).pushPx == 0); // Zomboss
	ElementHit i17 = ElementShroomHit(kElementIce3, 20, 2, 17, 0, 500, false);
	CHECK_FALSE(i17.freeze);
	CHECK(i17.pushPx == 0);
	CHECK(i17.damage == 16);
	CHECK(ElementShroomHit(kElementFire3, 20, 2, 0, 0, 500, false).damageFlags == 2);
}

TEST_CASE_FIXTURE(ModPlantsOnFixture, "Element-shroom: two-ice state heals 30 per pulse up to 3000 max") {
	ElementHeal h = ElementShroomPulseHeal(kElementIce2Fire1, 300, 300);
	CHECK(h.maxHp == 330);
	CHECK(h.hp == 330);
	h = ElementShroomPulseHeal(kElementIce2Fire1, 100, 2990);
	CHECK(h.maxHp == 3000);
	CHECK(h.hp == 130);
	h = ElementShroomPulseHeal(kElementIce2Fire1, 2995, 3000);
	CHECK(h.hp == 3000);
	h = ElementShroomPulseHeal(kElementIce3, 100, 300);
	CHECK(h.hp == 100);
	CHECK(h.maxHp == 300);
	CHECK(ElementShroomIgnoresPlantern());
}

// ---- fork B: Prism-shroom -----------------------------------------------------------------------------------

TEST_CASE_FIXTURE(ModPlantsOnFixture, "Prism-shroom: click pays 50 sun and fires; no sun buzzes") {
	CHECK(PrismShroomClickable(false, 0));
	CHECK_FALSE(PrismShroomClickable(true, 0));
	CHECK_FALSE(PrismShroomClickable(false, 3));
	PrismClick c = PrismShroomClick(false, 0, 120, true, false);
	CHECK(c.fires);
	CHECK(c.sun == 70);
	CHECK(c.shootingCounter == 25);
	CHECK_FALSE(c.buzzer);
	c = PrismShroomClick(false, 0, 20, false, false);
	CHECK_FALSE(c.fires);
	CHECK(c.buzzer);
	CHECK(c.sunFlashCs == 70);
	CHECK(c.sun == 20);
	c = PrismShroomClick(false, 0, 20, false, true); // conveyor / FREE: fires, sun floors at 0
	CHECK(c.fires);
	CHECK(c.sun == 0);
	c = PrismShroomClick(true, 0, 500, true, false); // asleep
	CHECK_FALSE(c.fires);
	CHECK_FALSE(c.buzzer);
	CHECK(PrismShroomProjectileType() == 0x16);
}

TEST_CASE_FIXTURE(ModPlantsOnFixture, "Prism-shroom: damage 300 + min(sun, 1000) / 4") {
	CHECK(PrismShroomShotDamage(0) == 300);
	CHECK(PrismShroomShotDamage(3) == 300);
	CHECK(PrismShroomShotDamage(400) == 400);
	CHECK(PrismShroomShotDamage(1000) == 550);
	CHECK(PrismShroomShotDamage(9990) == 550);
}

// ---- fork B: Dream Starfruit --------------------------------------------------------------------------------

TEST_CASE_FIXTURE(ModPlantsOnFixture, "Dream Starfruit: colour reroll, star kind, shadow return and target") {
	CHECK(DreamStarfruitColourRollRange(0) == 5);
	CHECK(DreamStarfruitColourRollRange(3) == 5);
	CHECK(DreamStarfruitColourRollRange(16) == 5);
	CHECK(DreamStarfruitColourRollRange(4) == 0);
	CHECK(DreamStarfruitColourRollRange(8) == 0);
	CHECK(DreamStarfruitStarKind(0) == 1);
	CHECK(DreamStarfruitStarKind(4) == 5);
	CHECK(DreamStarfruitReturnTimer(true) == 30);
	CHECK(DreamStarfruitReturnTimer(false) == 0);
	CHECK(DreamStarfruitAlwaysTargets(true));
	CHECK_FALSE(DreamStarfruitAlwaysTargets(false));
	CHECK(StarKindTurnsHoming(7, 7, 3, 1));
	CHECK_FALSE(StarKindTurnsHoming(7, 7, 0, 1));
	CHECK_FALSE(StarKindTurnsHoming(7, 7, 3, 2));
	CHECK_FALSE(StarKindTurnsHoming(7, 9, 3, 1));
	CHECK_FALSE(StarKindTurnsHoming(0, 7, 3, 1));
}

TEST_CASE_FIXTURE(ModPlantsOnFixture, "Dream Starfruit: star kind colour, damage bonus and pierce") {
	CHECK(StarKindColour(1) == 0x33a1c9u);
	CHECK(StarKindColour(2) == 0xff0000u);
	CHECK(StarKindColour(3) == 0xc000ffu);
	CHECK(StarKindColour(4) == 0x778899u);
	CHECK(StarKindColour(5) == 0x00ff00u);
	CHECK(StarKindColour(0) == 0u);
	CHECK(StarKindColour(6) == 0u);
	const int bonus[] = {0, 15, 20, 15, 25, 15, 0};
	for (int k = 0; k <= 6; ++k)
		CHECK(StarKindDamageBonus(k) == bonus[k]);
	CHECK(StarKindHitFate(7, 1, 0) == kStarHitVanilla);
	CHECK(StarKindHitFate(7, 0, 0) == kStarHitVanilla);
	// the exe's counter (+0x18) starts at 1: cmp 5 lets a red star live through 4 hits, cmp 3 a grey one through 2
	CHECK(StarKindHitFate(7, 2, 3) == kStarHitSurvives);
	CHECK(StarKindHitFate(7, 2, 4) == kStarHitDies);
	CHECK(StarKindHitFate(7, 4, 1) == kStarHitSurvives);
	CHECK(StarKindHitFate(7, 4, 2) == kStarHitDies);
	CHECK(StarKindHitFate(7, 3, 0) == kStarHitDies);
	CHECK(StarKindHitFate(7, 5, 0) == kStarHitDies);
	CHECK(StarKindHitFate(2, 2, 0) == kStarHitVanilla); // pierce only for stars
}

TEST_CASE_FIXTURE(ModPlantsOnFixture, "Dream Starfruit: star kind effects on hit with boundary rolls") {
	StarZombie z;
	z.type = 0;
	z.bodyHp = 270;
	z.canBeFrozen = true;
	// blue: Rand(7) == 0 freezes
	CHECK(StarKindImpactRollRange(1, z) == 7);
	CHECK(StarKindImpact(1, z, 0).freeze);
	CHECK_FALSE(StarKindImpact(1, z, 1).freeze);
	CHECK_FALSE(StarKindImpact(1, z, 6).freeze);
	StarZombie noFreeze = z;
	noFreeze.canBeFrozen = false;
	CHECK(StarKindImpactRollRange(1, noFreeze) == 0);
	CHECK_FALSE(StarKindImpact(1, noFreeze, 0).freeze);
	StarZombie paper = z;
	paper.type = 5;
	paper.subVariant = 1;
	CHECK(StarKindImpact(1, paper, 0).chill);
	CHECK_FALSE(StarKindImpact(1, paper, 0).freeze);
	StarZombie boss = z;
	boss.type = 25;
	CHECK_FALSE(StarKindImpact(1, boss, 0).freeze);
	// red: Rand(7) == 0 drops the helm up to 120000 HP
	CHECK(StarKindImpactRollRange(2, z) == 7);
	CHECK(StarKindImpact(2, z, 0).dropHelm);
	CHECK_FALSE(StarKindImpact(2, z, 6).dropHelm);
	StarZombie big = z;
	big.bodyHp = 120001;
	CHECK_FALSE(StarKindImpact(2, big, 0).dropHelm);
	big.bodyHp = 120000;
	CHECK(StarKindImpact(2, big, 0).dropHelm);
	CHECK(StarKindImpactRollRange(2, paper) == 0);
	// purple: Rand(10) (20 in I, Zombie) hypnotises a small zombie
	CHECK(StarKindImpactRollRange(3, z) == 10);
	CHECK(StarKindImpact(3, z, 0).hypnotise);
	CHECK_FALSE(StarKindImpact(3, z, 9).hypnotise);
	StarZombie iz = z;
	iz.iZombie = true;
	CHECK(StarKindImpactRollRange(3, iz) == 20);
	StarZombie tough = z;
	tough.bodyHp = 15000;
	CHECK(StarKindImpactRollRange(3, tough) == 0);
	StarZombie scaled = z;
	scaled.scale = 1.5f;
	CHECK(StarKindImpactRollRange(3, scaled) == 0);
	StarZombie bossPaper = z;
	bossPaper.type = 5;
	bossPaper.variant = 0x7f;
	CHECK(StarKindImpactRollRange(3, bossPaper) == 0);
	CHECK(StarKindImpactRollRange(3, boss) == 0);
	CHECK(StarKindImpactRollRange(3, paper) == 10); // football Newspaper: purple applies
	// grey: +400 weaken, no roll
	CHECK(StarKindImpactRollRange(4, z) == 0);
	CHECK(StarKindImpact(4, z, 3).weakenAddCs == 400);
	CHECK(StarKindImpact(4, paper, 0).weakenAddCs == 400);
	// green: Rand(7) (21 in I, Zombie) blows it away
	CHECK(StarKindImpactRollRange(5, z) == 7);
	CHECK(StarKindImpactRollRange(5, iz) == 21);
	CHECK(StarKindImpact(5, z, 0).blowAway);
	CHECK_FALSE(StarKindImpact(5, z, 6).blowAway);
	CHECK(StarKindImpactRollRange(5, paper) == 0);
	CHECK(StarKindImpactRollRange(5, boss) == 0);
	CHECK(StarKindImpactRollRange(0, z) == 0);
	CHECK(StarKindImpactRollRange(6, z) == 0);
}

TEST_CASE_FIXTURE(ModPlantsOnFixture, "Dream Starfruit: weakened zombie takes double damage, counts down") {
	CHECK(ZombieWeakenedDamage(20, 400, 0, 0, 0) == 40);
	CHECK(ZombieWeakenedDamage(20, 0, 0, 0, 0) == 20);
	CHECK(ZombieWeakenRollRange(400, 5, 1) == 2);
	CHECK(ZombieWeakenRollRange(0, 5, 1) == 0);
	CHECK(ZombieWeakenRollRange(400, 0, 0) == 0);
	CHECK(ZombieWeakenedDamage(20, 400, 5, 1, 0) == 20);
	CHECK(ZombieWeakenedDamage(20, 400, 5, 1, 1) == 5);
	CHECK(ZombieWeakenTick(400) == 399);
	CHECK(ZombieWeakenTick(0) == 0);
}

// ---- fork B: Spirit-tail ------------------------------------------------------------------------------------

TEST_CASE_FIXTURE(ModPlantsOnFixture, "Spirit-tail: Rand(36) re-rolled until one of 15 projectile types") {
	CHECK(SpiritTailRollRange() == 36);
	int accepted = 0;
	for (int r = 0; r < 36; ++r) {
		const int t = SpiritTailProjectileType(r);
		if (t >= 0) {
			CHECK(t == r);
			++accepted;
		}
	}
	CHECK(accepted == 15);
	for (int r : {6, 9, 13, 21, 23, 32})
		CHECK(SpiritTailProjectileType(r) == -1);
	for (int r : {0, 5, 7, 12, 22, 33, 35})
		CHECK(SpiritTailProjectileType(r) == r);
	CHECK(SpiritTailMotion() == 9);
}

TEST_CASE_FIXTURE(ModPlantsOnFixture, "Spirit-tail: big cabbage on Rand(2) == 0, star kind Rand(6), prism 475") {
	CHECK(SpiritTailExtraRollRange(2) == 2);
	CHECK(SpiritTailExtraRollRange(7) == 6);
	CHECK(SpiritTailExtraRollRange(0) == 0);
	CHECK(SpiritTailProjectileField80(2, 0) == 1);
	CHECK(SpiritTailProjectileField80(2, 1) == 0);
	CHECK(SpiritTailProjectileField80(7, 0) == 0);
	CHECK(SpiritTailProjectileField80(7, 5) == 5);
	CHECK(SpiritTailProjectileField80(0x16, 0) == 475);
	CHECK(SpiritTailProjectileField80(0, 0) == 0);
}

// ---- fork B: Mystic-Ice Shooter -----------------------------------------------------------------------------

TEST_CASE_FIXTURE(ModPlantsOnFixture, "Mystic-Ice Shooter: one snow pea per tick in a fan") {
	CHECK(MysticIceVelRollRange() == 2000);
	MysticIceShot s = MysticIceTickShot(100, 200, 0);
	CHECK(s.fires);
	CHECK(s.x == 169);
	CHECK(s.y == 210);
	CHECK(s.projectileType == 1);
	CHECK(s.motion == 7);
	CHECK(s.damageRangeFlags == 1);
	CHECK(s.velX == doctest::Approx(3.33f));
	CHECK(s.velY == doctest::Approx(-1.53846f));
	CHECK(MysticIceTickShot(100, 200, 1999).velY == doctest::Approx(1.5369246f).epsilon(1e-4));
	CHECK(MysticIceTickShot(100, 200, 1000).velY == doctest::Approx(0.0000015f).epsilon(1e-3));
}

TEST_CASE_FIXTURE(ModPlantsOnFixture, "Mystic-Ice Shooter: peas become snow peas, 500-tick life, Peashooters upgrade") {
	CHECK(MysticIceConvertsProjectile(0, -1, false));
	CHECK_FALSE(MysticIceConvertsProjectile(0, 3, false)); // already passed a Torchwood
	CHECK_FALSE(MysticIceConvertsProjectile(0, -1, true));
	CHECK_FALSE(MysticIceConvertsProjectile(6, -1, false));
	CHECK(MysticIceConvertedType() == 1);
	MysticIceLife l = MysticIceLifeTick(500);
	CHECK(l.launchRate == 499);
	CHECK_FALSE(l.dies);
	l = MysticIceLifeTick(1);
	CHECK(l.launchRate == 0);
	CHECK(l.dies);
	CHECK(l.deathParticle == 71);
	CHECK(MysticIceUpgradeSeed(0) == 5);
	CHECK(MysticIceUpgradeSeed(7) == -1);
	CHECK(ModPlantZomboniFreezeImmuneB(0x7a));
	CHECK(ModPlantZomboniFreezeImmuneB(0x55));
	CHECK_FALSE(ModPlantZomboniFreezeImmuneB(0x50));
}

// ---- fork B: feature off ------------------------------------------------------------------------------------

TEST_CASE_FIXTURE(VanillaWithRulesFixture, "Element/Prism/Dream Starfruit/Spirit-tail/Mystic-Ice: no effect while modPlants is off") {
	CHECK(ElementShroomState(0, 3) == kElementNone);
	CHECK(ElementShroomGainElement(0, 1, 0).bits == 0);
	CHECK(ElementShroomGainElement(0, 1, 0).count == 0);
	CHECK(ElementShroomTick(0, 0).resetCountdownTo == -1);
	CHECK_FALSE(ElementShroomTick(1, 0).gain);
	CHECK(ElementShroomToggleMode(0) == 0);
	CHECK(ElementShroomCountdownDigit(500) == 0);
	CHECK(ElementShroomShootingCounter() == 0);
	CHECK_FALSE(ElementShroomPulseAt(126));
	CHECK_FALSE(ElementShroomPuffAt(136));
	CHECK(ElementShroomRowReach(kElementFire2Ice1) == 1);
	CHECK_FALSE(ElementShroomAttackRect(kElementFire2Ice1, 0, 0).own);
	CHECK(ElementShroomIceRollRange(kElementIce3) == 0);
	CHECK(ElementShroomHit(kElementFire3, 20, 2, 0, 0, 0, false).damage == 20);
	CHECK(ElementShroomHit(kElementIce3, 20, 2, 0, 0, 0, false).damageFlags == 2);
	CHECK(ElementShroomFireStrip(0, true, true) == kElementStripNone);
	CHECK(ElementShroomPulseHeal(kElementIce2Fire1, 1, 1).hp == 1);
	CHECK_FALSE(ElementShroomIgnoresPlantern());
	CHECK_FALSE(PrismShroomClickable(false, 0));
	CHECK_FALSE(PrismShroomClick(false, 0, 500, true, false).fires);
	CHECK(PrismShroomShotCost() == 0);
	CHECK(PrismShroomProjectileType() == -1);
	CHECK(PrismShroomShotDamage(1000) == 0);
	CHECK(DreamStarfruitColourRollRange(0) == 0);
	CHECK(DreamStarfruitStarKind(2) == 0);
	CHECK(DreamStarfruitReturnTimer(true) == 0);
	CHECK_FALSE(DreamStarfruitAlwaysTargets(true));
	CHECK_FALSE(StarKindTurnsHoming(7, 7, 3, 1));
	CHECK(StarKindColour(1) == 0u);
	CHECK(StarKindDamageBonus(2) == 0);
	CHECK(StarKindHitFate(7, 2, 0) == kStarHitVanilla);
	StarZombie z;
	z.canBeFrozen = true;
	CHECK(StarKindImpactRollRange(1, z) == 0);
	CHECK(StarKindImpact(4, z, 0).weakenAddCs == 0);
	CHECK(ZombieWeakenRollRange(400, 5, 1) == 0);
	CHECK(ZombieWeakenedDamage(20, 400, 0, 0, 0) == 20);
	CHECK(ZombieWeakenTick(400) == 400);
	CHECK(SpiritTailRollRange() == 0);
	CHECK(SpiritTailProjectileType(6) == 0);
	CHECK(SpiritTailExtraRollRange(2) == 0);
	CHECK(SpiritTailProjectileField80(0x16, 0) == 0);
	CHECK(SpiritTailMotion() == -1);
	CHECK(MysticIceVelRollRange() == 0);
	CHECK_FALSE(MysticIceTickShot(0, 0, 0).fires);
	CHECK_FALSE(MysticIceConvertsProjectile(0, -1, false));
	CHECK(MysticIceConvertedType() == 0);
	CHECK_FALSE(MysticIceLifeTick(1).dies);
	CHECK(MysticIceLifeTick(1).launchRate == 1);
	CHECK(MysticIceUpgradeSeed(0) == -1);
	CHECK_FALSE(ModPlantZomboniFreezeImmuneB(0x7a));
}

// --- fork C: Sniper, Primal Repeater, Shrapnel Cherry, Loaded Cannon, Butter-pult, Bowling Wall-nut ---------
namespace {
SniperCandidate Zed(int x, int body, int helmType = 0, int helm = 0) {
	SniperCandidate z;
	z.x = x;
	z.bodyHp = z.bodyMaxHp = body;
	z.helmType = helmType;
	z.helmHp = helm;
	return z;
}
}

TEST_CASE_FIXTURE(ModPlantsOnFixture, "Sniper: HP sum counts only the armour pieces that exist") {
	SniperCandidate z = Zed(500, 270, 2, 370);
	z.shieldType = 1; z.shieldHp = 1100;
	z.flyingMaxHp = 20; z.flyingHp = 20;
	CHECK(SniperHpSum(z) == 270 + 370 + 1100 + 20);
	z.helmType = 0; // helm HP left over but no helm
	CHECK(SniperHpSum(z) == 270 + 1100 + 20);
}

TEST_CASE_FIXTURE(ModPlantsOnFixture, "Sniper: picks the highest HP right of it, any row; skips bosses, headless, untouchable") {
	std::vector<SniperCandidate> v{Zed(400, 270), Zed(600, 270, 3, 1100), Zed(700, 640)};
	CHECK(SniperPickTarget(v, 300) == 1);
	v[1].x = 299; // left of the sniper's right edge
	CHECK(SniperPickTarget(v, 300) == 2);
	v[1].x = 300; // exactly at the edge: pickable (jl skip)
	CHECK(SniperPickTarget(v, 300) == 1);
	CHECK_FALSE(SniperKeepsTarget(v[1], 300)); // but kept only while strictly right of it
	v[1].hasHead = false;
	CHECK(SniperPickTarget(v, 300) == 2);
	v[2].effectedByDamage = false;
	CHECK(SniperPickTarget(v, 300) == 0);
	std::vector<SniperCandidate> boss{Zed(500, 150000), Zed(500, 270)};
	CHECK(SniperPickTarget(boss, 100) == 1); // bodyMax 150000 > 120000
	boss[0].bodyHp = boss[0].bodyMaxHp = 1000;
	boss[0].zombieType = 5; boss[0].variant = 0x7f; // 7-10 Newspaper boss form
	CHECK(SniperPickTarget(boss, 100) == 1);
	boss[0].zombieType = 25; boss[0].variant = 0; // Zomboss
	CHECK(SniperPickTarget(boss, 100) == 1);
	std::vector<SniperCandidate> tie{Zed(500, 300), Zed(600, 300)};
	CHECK(SniperPickTarget(tie, 100) == 1); // a tie goes to the later zombie
	CHECK(SniperPickTarget({}, 100) == -1);
}

TEST_CASE_FIXTURE(ModPlantsOnFixture, "Sniper: 500 cs cycle, warning at 120, 500 damage at 5") {
	CHECK(SniperCountdownAfterTick(0) == 500);
	CHECK(SniperCountdownAfterTick(-3) == 500);
	CHECK(SniperCountdownAfterTick(1) == 1);
	CHECK(SniperTimer(120).warn);
	CHECK_FALSE(SniperTimer(120).fire);
	CHECK_FALSE(SniperTimer(121).warn);
	const SniperTick shot = SniperTimer(5);
	CHECK(shot.fire);
	CHECK(shot.damage == 500);
	// phone report 2026-11-23 ("no shot at all"): the exe has no projectile, only sounds: PlayFoley(0x52) at the warning
	// (0x7b13a0 mov esi,0x52; call 0x453630) and PlayFoley(0x37) with the damage (0x7b13c2 mov esi,0x37)
	CHECK(SniperTimer(120).foley == 0x52);
	CHECK(shot.foley == 0x37);
	CHECK(SniperTimer(6).foley == 0);
	CHECK_FALSE(SniperTimer(4).fire);
	CHECK_FALSE(SniperTimer(6).fire);
}

TEST_CASE_FIXTURE(ModPlantsOnFixture, "Primal Repeater: Rand(10) 0 = knock-back pea, 1 = poison pea, 2..9 plain") {
	CHECK(PrimalRepeaterRollRange() == 10);
	CHECK(PrimalRepeaterShotType(0, 0) == 0x22);
	CHECK(PrimalRepeaterShotType(1, 0) == 0x23);
	CHECK(PrimalRepeaterShotType(2, 0) == 0);
	CHECK(PrimalRepeaterShotType(9, 0) == 0);
	CHECK(PrimalRepeaterSecondPea());
	CHECK(PrimalKnockbackPx(0x22) == 30);
	CHECK(PrimalKnockbackPx(0x23) == 0);
	CHECK(PrimalPoisonAddCs(0x23) == 500);
	CHECK(PrimalPoisonAddCs(0x22) == 0);
}

TEST_CASE_FIXTURE(ModPlantsOnFixture, "Shrapnel Cherry: 1 resting core + 6 shards (up, down, 4 diagonals), 250 per hit, core re-plants at 1400") {
	const auto s = ShrapnelCherryShards();
	REQUIRE(s.size() == 7);
	CHECK(s[0].kind == 2);
	CHECK(s[0].velX == 0.0f);
	CHECK(s[0].velY == 0.0f);
	CHECK(s[1].velY == doctest::Approx(-4.0f));
	CHECK(s[2].velY == doctest::Approx(4.0f));
	CHECK(s[3].velX == doctest::Approx(2.8f));
	CHECK(s[4].velY == doctest::Approx(-2.8f));
	CHECK(s[5].velX == doctest::Approx(2.8f));
	CHECK(s[5].velY == doctest::Approx(-2.8f));
	CHECK(s[6].velX == doctest::Approx(-2.8f));
	for (size_t i = 1; i < s.size(); i++)
		CHECK(s[i].kind == 1);
	CHECK(ShrapnelShardHitDamage(1) == 250);
	CHECK(ShrapnelShardHitDamage(2) == 250);
	CHECK(ShrapnelShardHitDamage(0) == 0); // a Starfruit star
	CHECK(ShrapnelCoreReplants(2, 1400));
	CHECK_FALSE(ShrapnelCoreReplants(2, 1399));
	CHECK_FALSE(ShrapnelCoreReplants(1, 1400));
	CHECK(ShrapnelCoreReplantColumn(479) == 5);
	CHECK(ShrapnelCoreReplantColumn(480) == 6);
	CHECK(ShrapnelCherryExplodesWhenSquished(0x52));
	CHECK(ShrapnelCherryExplodesWhenSquished(2));
	CHECK_FALSE(ShrapnelCherryExplodesWhenSquished(3));
}

TEST_CASE_FIXTURE(ModPlantsOnFixture, "Loaded Cannon: never re-arms alone; a Kernel load gives 3000 cobs, a Doom load blows at 3600") {
	CHECK_FALSE(LoadedCannonArmsByItself());
	CHECK(LoadedCannonLoad(0x23, 0x22) == CannonLoad::Kernel);
	CHECK(LoadedCannonLoad(0x23, 0xf) == CannonLoad::Doom);
	CHECK(LoadedCannonLoad(0x23, 0) == CannonLoad::None);
	CHECK(LoadedCannonLoad(0x25, 0x22) == CannonLoad::None); // only while ARMING
	CHECK(LoadedCannonPacketPenaltyCs(CannonLoad::Kernel) == 6000);
	CHECK(LoadedCannonPacketPenaltyCs(CannonLoad::None) == 0);
	CHECK(LoadedCannonCobBurnCap(0x22) == 3000);
	CHECK(LoadedCannonCobBurnCap(-1) == 0);
	CHECK(LoadedCannonDoomBurnCap() == 3600);
	CHECK(LoadedCannonDoomClearsGraves());
	CHECK(BurnCapDamage(3000, 3000) == 3000);
	CHECK(BurnCapDamage(2999, 3000) == -1);
	CHECK(BurnCapDamage(1800, 0) == 1800); // no cap = vanilla 1800
	CHECK(BurnCapDamage(1799, 0) == -1);
}

TEST_CASE_FIXTURE(ModPlantsOnFixture, "Butter-pult: every lob is butter (streak 0 -> 400 cs), flags 13") {
	CHECK(ButterPultProjectile(10) == 12);
	CHECK(ButterPultButterStreak() == 0);
	CHECK(ButterPultDamageRangeFlags(5) == 13);
}

TEST_CASE_FIXTURE(ModPlantsOnFixture, "Bowling Wall-nut: rolls on any level, is not eaten, fixed anim rate") {
	CHECK(BowlingWallnutRolls(false));
	CHECK_FALSE(BowlingWallnutEdible());
	CHECK_FALSE(BowlingWallnutRandomAnimRate());
}

TEST_CASE("Bowling geometry on any lawn: roof slope, last row, the square it holds") {
	// The roof's ground under x: GridToPixelY's (5 - col) * 20 at each column's plant x (col * 80 + 40), 0 from x 440.
	for (int aCol = 0; aCol < 9; ++aCol)
		CHECK(BowlingRoofSlopeY(aCol * 80 + 40) == (aCol < 5 ? (5 - aCol) * 20 : 0));
	CHECK(BowlingRoofSlopeY(41) == 99);
	CHECK(BowlingRoofSlopeY(439) == 0);
	CHECK(BowlingRoofSlopeY(1100) == 0);
	CHECK(BowlingRoofSlopeY(-40) == 120); // left of the lawn: the slope goes on
	// Rows: 5-row lawns bounce at 4 (vanilla), 6-row pool lawns at 5.
	CHECK(BowlingLastRow(false) == 4);
	CHECK(BowlingLastRow(true) == 5);
	// The nut holds its planting square only while its middle is still in it, on its planting row.
	CHECK(BowlingNutInItsSquare(2, 3, 3, 2));
	CHECK_FALSE(BowlingNutInItsSquare(2, 3, 3, 3));
	CHECK_FALSE(BowlingNutInItsSquare(2, 3, 4, 2));
}

TEST_CASE_FIXTURE(VanillaWithRulesFixture, "Fork C plants: feature off = no effect") {
	std::vector<SniperCandidate> v{Zed(600, 270)};
	CHECK(SniperPickTarget(v, 100) == -1);
	CHECK_FALSE(SniperKeepsTarget(v[0], 100));
	CHECK(SniperCountdownAfterTick(0) == 0);
	CHECK_FALSE(SniperTimer(5).fire);
	CHECK_FALSE(SniperTimer(120).warn);
	CHECK(PrimalRepeaterRollRange() == 0);
	CHECK(PrimalRepeaterShotType(0, 0) == 0);
	CHECK_FALSE(PrimalRepeaterSecondPea());
	CHECK(PrimalKnockbackPx(0x22) == 0);
	CHECK(PrimalPoisonAddCs(0x23) == 0);
	CHECK(ShrapnelCherryShards().empty());
	CHECK(ShrapnelShardHitDamage(1) == 0);
	CHECK_FALSE(ShrapnelCoreReplants(2, 1400));
	CHECK_FALSE(ShrapnelCherryExplodesWhenSquished(0x52));
	CHECK(ShrapnelCherryExplodesWhenSquished(2)); // the day-world Cherry rule is not this feature's
	CHECK(LoadedCannonArmsByItself());
	CHECK(LoadedCannonLoad(0x23, 0x22) == CannonLoad::None);
	CHECK(LoadedCannonPacketPenaltyCs(CannonLoad::Kernel) == 0);
	CHECK(LoadedCannonCobBurnCap(0x22) == 0);
	CHECK(LoadedCannonDoomBurnCap() == 0);
	CHECK_FALSE(LoadedCannonDoomClearsGraves());
	CHECK(ButterPultProjectile(10) == 10);
	CHECK(ButterPultDamageRangeFlags(5) == 5);
	CHECK_FALSE(BowlingWallnutRolls(false));
	CHECK(BowlingWallnutRolls(true));
	CHECK(BowlingWallnutEdible());
	CHECK(BowlingWallnutRandomAnimRate());
}

// --- fork D: Rewind Grass, Charmed Tall-nut, Hallucination-shroom, Magic Devourer, Holy-fire Pumpkin ---------
namespace {
constexpr int kD_Normal = 0, kD_Newspaper = 5, kD_Football = 7, kD_Dancer = 8, kD_Bungee = 20, kD_Garg = 23,
	kD_Imp = 24, kD_Boss = 25;
}

TEST_CASE_FIXTURE(ModPlantsOnFixture, "ModPlantInitSeed / slots / coffee rule: base init, flying slot, no sleep test") {
	CHECK(ModPlantInitSeed(0x67) == 0x17);
	CHECK(ModPlantInitSeed(0x6e) == 0x1e);
	CHECK(ModPlantInitSeed(0x5b) == 0x0b);
	CHECK(ModPlantInitSeed(0x17) == 0x17);
	CHECK(ModPlantLawnSlot(0x6b) == 3);
	CHECK(ModPlantLawnSlot(0x5b) == 3);
	CHECK(ModPlantLawnSlot(0x53) == 3);
	CHECK(ModPlantLawnSlot(0x6e) == 2);
	CHECK(ModPlantLawnSlot(0x60) == 1);
	CHECK(ModPlantLawnSlot(0x67) == 0);
	CHECK(ModPlantBossClass(kD_Normal, 120001, false));
	CHECK_FALSE(ModPlantBossClass(kD_Normal, 120000, false));
	CHECK(ModPlantBossClass(kD_Boss, 40000, false));
	CHECK(ModPlantBossClass(kD_Newspaper, 4500, true));
}

TEST_CASE_FIXTURE(ModPlantsOnFixture, "RewindGrass: one blow after 50 cs heals everyone and sends them to x 800") {
	CHECK(RewindGrassBlowDelayCs() == 50);
	CHECK(RewindGrassBlowAnimRate() == doctest::Approx(20.0f));
	CHECK(RewindGrassAffects(false, 0, 799, true));
	CHECK_FALSE(RewindGrassAffects(false, 0, 800, true)); // jge 800
	CHECK_FALSE(RewindGrassAffects(true, 0, 300, true));
	CHECK_FALSE(RewindGrassAffects(false, 1, 300, true));
	CHECK_FALSE(RewindGrassAffects(false, 2, 300, true));
	CHECK_FALSE(RewindGrassAffects(false, 3, 300, true));
	CHECK(RewindGrassAffects(false, 4, 300, true));
	CHECK_FALSE(RewindGrassAffects(false, 0, 300, false));
	CHECK(RewindGrassSendsBack(kD_Normal, false, false));
	CHECK_FALSE(RewindGrassSendsBack(kD_Bungee, false, false));
	CHECK_FALSE(RewindGrassSendsBack(kD_Normal, true, false));
	CHECK_FALSE(RewindGrassSendsBack(kD_Normal, false, true));
	CHECK(RewindGrassBackX() == doctest::Approx(800.0f));
	CHECK(RewindGrassResetsPhaseCounter(40));
	CHECK_FALSE(RewindGrassResetsPhaseCounter(41));
	ZombieHpSet hp{100, 270, 0, 370, 500, 1100, 0, 0};
	RewindGrassHeal(hp);
	CHECK(hp.body == 270);
	CHECK(hp.helm == 370);
	CHECK(hp.shield == 1100);
	CHECK(hp.flying == 0);
	ZombieHpSet over{300, 270, 0, 0, 0, 0, 0, 0}; // never lowered
	RewindGrassHeal(over);
	CHECK(over.body == 300);
	CHECK(RewindGrassUntouchable(0x6b));
	CHECK_FALSE(RewindGrassUntouchable(0x1b));
}

TEST_CASE_FIXTURE(ModPlantsOnFixture, "CharmedTallnut: first summon after 1500 bitten HP, then every 1200; 1/3 table") {
	CharmedTallnutBite b = CharmedTallnutOnBite(1496, 4, 8000, true);
	CHECK(b.summon);
	CHECK(b.counter == 300);
	b = CharmedTallnutOnBite(1495, 4, 8000, true);
	CHECK_FALSE(b.summon);
	CHECK(b.counter == 1499);
	b = CharmedTallnutOnBite(1499, 4, 2, true); // only the 2 HP left count: 1501 -> summon -> 301
	CHECK(b.summon);
	CHECK(b.counter == 301);
	b = CharmedTallnutOnBite(2996, 4, 8000, true); // q = 2: two steps off, one zombie
	CHECK(b.summon);
	CHECK(b.counter == 3000 - 2400);
	b = CharmedTallnutOnBite(1496, 4, 8000, false); // not a bite
	CHECK_FALSE(b.summon);
	CHECK(b.counter == 1496);
	CHECK(CharmedTallnutSummonRollRange() == 3);
	CHECK(CharmedTallnutSummonType(0) == kD_Newspaper);
	CHECK(CharmedTallnutSummonType(1) == kD_Football);
	CHECK(CharmedTallnutSummonType(2) == kD_Garg);
	CHECK(CharmedTallnutSummonType(3) == -1);
	CHECK(CharmedTallnutSummonBodyHp(270) == 365); // 270 * 1.35f = 364.50000644 -> 365
	CHECK(CharmedTallnutSummonBodyHp(3000) == 4050);
	CHECK(CharmedTallnutRiseCounterCs() == 50);
	CHECK(ModSeedCountsAsTallnut(0x67));
	CHECK_FALSE(ModSeedCountsAsTallnut(0x6e));
}

TEST_CASE_FIXTURE(ModPlantsOnFixture, "CharmedTallnut shadow: x1.5 bites, Gargantuar on entry, regen 75/100 cs, bite-back") {
	CHECK(CharmedTallnutShadowThreshold() == 2);
	CHECK(CharmedTallnutBiteDamage(4, true) == 6);
	CHECK(CharmedTallnutBiteDamage(4, false) == 4);
	CharmedTallnutGarg g = CharmedTallnutEnterShadow(6000, 8000); // exactly 75%
	CHECK(g.summon);
	CHECK(g.gargBodyHp == 3000);
	CHECK(g.plantHpAfter == 2800);
	g = CharmedTallnutEnterShadow(5999, 8000);
	CHECK_FALSE(g.summon);
	CHECK(g.plantHpAfter == 5999);
	NutRegen r = CharmedTallnutRegenTick(true, 0, 5000, 8000, 5000, 98);
	CHECK(r.counter == 99);
	CHECK(r.hp == 5000);
	r = CharmedTallnutRegenTick(true, 0, 5000, 8000, 5000, 99);
	CHECK(r.hp == 5075);
	CHECK(r.counter == 0);
	CHECK(r.lastHp == 5075);
	r = CharmedTallnutRegenTick(true, 0, 7990, 8000, 7990, 99);
	CHECK(r.hp == 8000);
	r = CharmedTallnutRegenTick(true, 0, 4996, 8000, 5000, 50); // bitten: restart
	CHECK(r.counter == 0);
	CHECK(r.lastHp == 4996);
	r = CharmedTallnutRegenTick(false, 0, 5000, 8000, 5000, 99);
	CHECK(r.hp == 5000);
	CHECK(r.counter == 99);
	r = CharmedTallnutRegenTick(true, 0x2b, 5000, 8000, 5000, 99);
	CHECK(r.hp == 5000);
	CHECK(CharmedTallnutBiteBackRollRange() == 200);
	CharmedTallnutBiteBack bb = CharmedTallnutOnEaten(true, 0, true, 0, 0);
	CHECK(bb.damageToEater == 8);
	CHECK(bb.charmEater);
	bb = CharmedTallnutOnEaten(true, 0, true, 0, 1);
	CHECK(bb.damageToEater == 8);
	CHECK_FALSE(bb.charmEater);
	bb = CharmedTallnutOnEaten(true, 31, true, 0x31, 199); // mad Newspaper with its object, variant 0x31
	CHECK(bb.charmEater);
	CHECK(bb.damageToEater == 0);
	bb = CharmedTallnutOnEaten(false, 0, true, 0, 0);
	CHECK_FALSE(bb.charmEater);
	CHECK(bb.damageToEater == 0);
}

TEST_CASE_FIXTURE(ModPlantsOnFixture, "Hallucination: the zombie that finishes it becomes a 270 HP Imp without armour") {
	CHECK(HallucinationTurnsEaterToImp(0x5c, 0, false));
	CHECK(HallucinationTurnsEaterToImp(0x5c, -3, false));
	CHECK_FALSE(HallucinationTurnsEaterToImp(0x5c, 1, false));
	CHECK_FALSE(HallucinationTurnsEaterToImp(0x5c, 0, true));
	CHECK_FALSE(HallucinationTurnsEaterToImp(0x0c, 0, false));
	HallucinationImp m = HallucinationImpMorph(1350);
	CHECK(m.zombieType == kD_Imp);
	CHECK(m.bodyHp == 270);
	CHECK(m.bodyMaxHp == 270);
	CHECK(m.rectX == 36);
	CHECK(m.rectW == 42);
	CHECK(m.attackRectX == 50);
	CHECK(m.attackRectW == 20);
	CHECK(m.reanimType == 57);
	CHECK(m.particle == 71);
	CHECK(HallucinationImpMorph(100).bodyHp == 100);
}

TEST_CASE_FIXTURE(ModPlantsOnFixture, "MagicDevourer: double-speed countdown to 10, swallows the plant into a seed card at 5") {
	CHECK(MagicDevourerCountdownTick(9, 400) == 399);
	CHECK(MagicDevourerCountdownTick(9, 10) == 9);
	CHECK(MagicDevourerCountdownTick(9, 9) == 9);
	CHECK(MagicDevourerCountdownTick(8, 400) == 400);
	CHECK(MagicDevourerSwallowsNow(9, 5, 0, 0, false));
	CHECK(MagicDevourerSwallowsNow(9, 5, 8, 5, false));
	CHECK_FALSE(MagicDevourerSwallowsNow(9, 5, 9, 0, false));
	CHECK_FALSE(MagicDevourerSwallowsNow(9, 5, 0, 6, false));
	CHECK_FALSE(MagicDevourerSwallowsNow(9, 6, 0, 0, false));
	CHECK_FALSE(MagicDevourerSwallowsNow(9, 4, 0, 0, false));
	CHECK_FALSE(MagicDevourerSwallowsNow(8, 5, 0, 0, false));
	CHECK_FALSE(MagicDevourerSwallowsNow(9, 5, 0, 0, true));
	CHECK(MagicDevourerCardCoinType() == 16);
	CHECK(MagicDevourerCardCoinMotion() == 2);
}

TEST_CASE_FIXTURE(ModPlantsOnFixture, "HolyFirePumpkin: a pumpkin that guards its square from the Zomboni freeze") {
	CHECK(HolyFirePumpkinActsAsPumpkin(0x6e));
	CHECK_FALSE(HolyFirePumpkinActsAsPumpkin(0x1e)); // the real Pumpkin needs no rule
	CHECK(HolyFirePumpkinGuardsSquare(0x6e));
	CHECK_FALSE(HolyFirePumpkinGuardsSquare(0x1e));
}

TEST_CASE_FIXTURE(VanillaWithRulesFixture, "fork D plants: everything is off in vanilla mode") {
	CHECK(ModPlantInitSeed(0x67) == 0x67);
	CHECK(ModPlantLawnSlot(0x6b) == 0);
	CHECK(RewindGrassBlowDelayCs() == 0);
	CHECK_FALSE(RewindGrassAffects(false, 0, 300, true));
	CHECK_FALSE(RewindGrassSendsBack(kD_Normal, false, false));
	CHECK_FALSE(RewindGrassResetsPhaseCounter(40));
	ZombieHpSet hp{100, 270, 0, 0, 0, 0, 0, 0};
	RewindGrassHeal(hp);
	CHECK(hp.body == 100);
	CHECK_FALSE(RewindGrassUntouchable(0x6b));
	CHECK_FALSE(CharmedTallnutOnBite(1496, 4, 8000, true).summon);
	CHECK(CharmedTallnutSummonRollRange() == 0);
	CHECK(CharmedTallnutSummonType(0) == -1);
	CHECK(CharmedTallnutSummonBodyHp(270) == 270);
	CHECK(CharmedTallnutBiteDamage(4, true) == 4);
	CHECK(CharmedTallnutShadowThreshold() == 0);
	CHECK_FALSE(CharmedTallnutEnterShadow(8000, 8000).summon);
	CHECK(CharmedTallnutRegenTick(true, 0, 5000, 8000, 5000, 99).hp == 5000);
	CHECK(CharmedTallnutBiteBackRollRange() == 0);
	CHECK_FALSE(CharmedTallnutOnEaten(true, 0, true, 0, 0).charmEater);
	CHECK_FALSE(ModSeedCountsAsTallnut(0x67));
	CHECK_FALSE(HallucinationTurnsEaterToImp(0x5c, 0, false));
	CHECK(HallucinationImpMorph(1350).zombieType == 0);
	CHECK(MagicDevourerCountdownTick(9, 400) == 400);
	CHECK_FALSE(MagicDevourerSwallowsNow(9, 5, 0, 0, false));
	CHECK(MagicDevourerCardCoinType() == 0);
	CHECK_FALSE(HolyFirePumpkinActsAsPumpkin(0x6e));
	CHECK_FALSE(HolyFirePumpkinGuardsSquare(0x6e));
}

// ---- Fork E tests ----------------------------------------------------------------------------------------------

TEST_CASE_FIXTURE(ModPlantsOnFixture, "FireDragon: 3 rows, x from -40 to +240, Zomboss only from column 7, never hypnotised") {
	// zombie type 0 (Normal), plant at row 2, x 300, col 3
	CHECK(FireDragonInRange(0, false, true, 2, 300, 2, 300, 3));
	CHECK(FireDragonInRange(0, false, true, 1, 300, 2, 300, 3));
	CHECK(FireDragonInRange(0, false, true, 3, 300, 2, 300, 3));
	CHECK_FALSE(FireDragonInRange(0, false, true, 0, 300, 2, 300, 3)); // 2 rows away
	CHECK_FALSE(FireDragonInRange(0, false, true, 4, 300, 2, 300, 3));
	CHECK(FireDragonInRange(0, false, true, 2, 260, 2, 300, 3));        // -40 edge
	CHECK_FALSE(FireDragonInRange(0, false, true, 2, 259, 2, 300, 3));
	CHECK(FireDragonInRange(0, false, true, 2, 540, 2, 300, 3));        // +240 edge
	CHECK_FALSE(FireDragonInRange(0, false, true, 2, 541, 2, 300, 3));
	CHECK_FALSE(FireDragonInRange(0, false, false, 2, 300, 2, 300, 3)); // EffectedByDamage(9) false
	CHECK_FALSE(FireDragonInRange(0, true, true, 2, 300, 2, 300, 3));   // hypnotised
	// Zomboss (25): any row/x, only from column 7
	CHECK(FireDragonInRange(25, false, false, 0, 900, 4, 600, 7));
	CHECK_FALSE(FireDragonInRange(25, false, true, 2, 300, 2, 540, 6));
	CHECK(FireDragonDamageFlags() == 9);
	CHECK(FireDragonAttackCs() == 70);
	CHECK(FireDragonHitAtCountdown() == 12);
}

TEST_CASE_FIXTURE(ModPlantsOnFixture, "FireDragon: 100 + maxHp/15 per zombie, cap 600, 1800 per flame, +400 poison in shadow") {
	CHECK(FireDragonHitDamage(270) == 118);
	CHECK(FireDragonHitDamage(3000) == 300);
	CHECK(FireDragonHitDamage(7499) == 600);  // 599.93 rounds to 600
	CHECK(FireDragonHitDamage(40000) == 600); // cap
	CHECK(FireDragonHitDamage(0) == 100);
	CHECK(FireDragonFlameBudget() == 1800);
	FireDragonHit h = FireDragonSpend(600, 1800, false);
	CHECK(h.damage == 600);
	CHECK(h.budgetLeft == 1200);
	CHECK(h.poisonAdd == 0);
	h = FireDragonSpend(600, 600, true); // exactly used up: still 600, then 0 left
	CHECK(h.damage == 600);
	CHECK(h.budgetLeft == 0);
	CHECK(h.poisonAdd == 400);
	h = FireDragonSpend(600, 250, false); // only what is left
	CHECK(h.damage == 250);
	CHECK(h.budgetLeft == 0);
	h = FireDragonSpend(118, 0, true); // budget gone: 0 damage, still marked
	CHECK(h.damage == 0);
	CHECK(h.poisonAdd == 400);
	CHECK(FireDragonClearsChillAndFreeze());
	CHECK(FireDragonZomboniFreezeImmune());
}

TEST_CASE_FIXTURE(ModPlantsOnFixture, "GoldenSunShroom: three large suns at 125/75/25 cs, vanishes at 150, idle in Last Stand") {
	GoldenSunShroomStep s = GoldenSunShroomUpdate(0, true, false, 23, 0);
	CHECK(s.state == 2);
	CHECK(s.disappearCs == 150);
	CHECK(s.timerCs == 150);
	CHECK_FALSE(s.dropSun);
	s = GoldenSunShroomUpdate(0, true, false, 2, 125);
	CHECK(s.dropSun);
	CHECK(s.coinType == 6);
	CHECK(s.coinMotion == 2);
	s = GoldenSunShroomUpdate(0, true, false, 2, 75);
	CHECK(s.dropSun);
	CHECK(s.coinMotion == 3);
	CHECK(GoldenSunShroomUpdate(0, true, false, 2, 25).dropSun);
	CHECK_FALSE(GoldenSunShroomUpdate(0, true, false, 2, 124).dropSun);
	CHECK_FALSE(GoldenSunShroomUpdate(0, true, false, 2, 150).dropSun);
	CHECK(GoldenSunShroomUpdate(31, true, false, 23, 0).state == -1); // Last Stand
	CHECK(GoldenSunShroomUpdate(0, false, false, 23, 0).state == -1); // not on board
	CHECK(GoldenSunShroomUpdate(0, true, true, 23, 0).state == -1);   // mid-air
	CHECK(GoldenSunShroomPacketStartCounter(0x59, 0) == 3500);
	CHECK(GoldenSunShroomPacketStartCounter(0x30, 0x59) == 3500);
	CHECK(GoldenSunShroomPacketStartCounter(9, 0) == -1);
	CHECK(ModPlantDodgesZombieShots(0x59)); // the one hook 374 list (0xc02c55..0xc02c6a) holds 0x59
	CHECK_FALSE(ModPlantDodgesZombieShots(9));
}

TEST_CASE_FIXTURE(ModPlantsOnFixture, "StrongCoffee: wakes the 3x3 at countdown 1, plants on any plant, heals 30% (cap 300) on death") {
	CHECK(StrongCoffeeTriggers(1, true));
	CHECK_FALSE(StrongCoffeeTriggers(2, true));
	CHECK_FALSE(StrongCoffeeTriggers(0, true));
	CHECK_FALSE(StrongCoffeeTriggers(1, false));
	CHECK(StrongCoffeeWakes(2, 4, 1, 3, false));
	CHECK(StrongCoffeeWakes(2, 4, 3, 5, false));
	CHECK(StrongCoffeeWakes(2, 4, 2, 4, false));
	CHECK_FALSE(StrongCoffeeWakes(2, 4, 0, 4, false));
	CHECK_FALSE(StrongCoffeeWakes(2, 4, 2, 6, false));
	CHECK_FALSE(StrongCoffeeWakes(2, 4, 2, 5, true)); // squash mid-air
	CHECK(StrongCoffeeWakeCs() == 100);
	// placement: an awake plant is fine now
	CHECK(CoffeeLikePlantingReason(0x73, false, true, false, 0, false) == 0);
	CHECK(CoffeeLikePlantingReason(0x23, false, true, false, 0, false) == 0);
	CHECK(CoffeeLikePlantingReason(0x5b, false, true, true, 0, false) == 0);
	CHECK(CoffeeLikePlantingReason(0x73, true, true, true, 0, false) == 1);   // a coffee already there
	CHECK(CoffeeLikePlantingReason(0x73, false, false, false, 0, false) == 13); // nothing to plant on
	CHECK(CoffeeLikePlantingReason(0x73, false, true, true, 5, false) == 13);  // already waking
	CHECK(CoffeeLikePlantingReason(0x73, false, true, true, 0, true) == 13);   // bungee grab
	CHECK(CoffeeLikePlantingReason(0, false, true, true, 0, false) == -1);
	// heal
	CoffeeHeal h = CoffeeBeanDeathHeal(0x73, 2, 100, 300); // 30 * 3 = 90
	CHECK(h.hp == 190);
	CHECK(h.maxHp == 300);
	h = CoffeeBeanDeathHeal(0x73, 2, 4000, 4000); // 1200 capped at 300, overheal raises max
	CHECK(h.hp == 4300);
	CHECK(h.maxHp == 4300);
	h = CoffeeBeanDeathHeal(0x23, 2, 250, 305); // 305/10 = 30 -> 90; 340 >= 305
	CHECK(h.hp == 340);
	CHECK(h.maxHp == 340);
	h = CoffeeBeanDeathHeal(0x73, 0, 100, 300); // did not do its job
	CHECK(h.hp == 100);
	h = CoffeeBeanDeathHeal(0, 2, 100, 300);
	CHECK(h.hp == 100);
}

TEST_CASE_FIXTURE(ModPlantsOnFixture, "LeafShield: zombie shots in 3 rows at its front edge bounce back, costing damage/8 + 2 HP") {
	// leaf at row 2, x 240, width 80: band x 290..330
	CHECK(LeafShieldReflects(0, 2, 290, 2, 240, 80));
	CHECK(LeafShieldReflects(0, 2, 330, 2, 240, 80));
	CHECK_FALSE(LeafShieldReflects(0, 2, 289, 2, 240, 80));
	CHECK_FALSE(LeafShieldReflects(0, 2, 331, 2, 240, 80));
	CHECK(LeafShieldReflects(0, 1, 300, 2, 240, 80));
	CHECK(LeafShieldReflects(0, 3, 300, 2, 240, 80));
	CHECK_FALSE(LeafShieldReflects(0, 4, 300, 2, 240, 80));
	CHECK_FALSE(LeafShieldReflects(1, 2, 300, 2, 240, 80)); // a plant shot / already reflected
	CHECK(LeafShieldHpCost(20) == 4);
	CHECK(LeafShieldHpCost(75) == 11);
	CHECK(LeafShieldHpCost(300) == 39);
	LeafShieldBounce b = LeafShieldBounceOf(6);
	CHECK(b.motionType == 0);
	CHECK_FALSE(b.negateVelX);
	CHECK(b.damageRangeFlags == 1);
	CHECK(b.flashCs == 25);
	b = LeafShieldBounceOf(7);
	CHECK(b.motionType == 7);
	CHECK(b.negateVelX);
	b = LeafShieldBounceOf(1); // lobbed keeps its motion
	CHECK(b.motionType == 1);
	CHECK_FALSE(b.negateVelX);
}

TEST_CASE_FIXTURE(ModPlantsOnFixture, "LeafShield: the mod projectile damage lookup 0x7b0890") {
	CHECK(ModProjectileDamage(0, 20, 0, false) == 20);
	CHECK(ModProjectileDamage(13, 20, 0, false) == 20);
	CHECK(ModProjectileDamage(7, 1, 0, false) == 2); // star doubled
	CHECK(ModProjectileDamage(7, 1, 0, true) == 1);  // not in I, Zombie
	CHECK(ModProjectileDamage(14, 0, 0, false) == 40);
	CHECK(ModProjectileDamage(15, 0, 0, false) == 50);
	CHECK(ModProjectileDamage(18, 0, 0, false) == 300);
	CHECK(ModProjectileDamage(22, 0, 77, false) == 77); // reads Projectile+0x80
	CHECK(ModProjectileDamage(24, 0, 0, false) == 0);
	CHECK(ModProjectileDamage(32, 0, 0, false) == 100);
	CHECK(ModProjectileDamage(33, 0, 0, false) == 40);
	CHECK(ModProjectileDamage(35, 0, 0, false) == 15);
	CHECK(ModProjectileDamage(36, 0, 0, false) == 0);
}

TEST_CASE_FIXTURE(ModPlantsOnFixture, "BossFlower: a usable packet of any vanilla seed 0..52 or a mod seed but Loaded Cannon") {
	CHECK(BossFlowerRollRange() == 128);
	CHECK(BossFlowerCoinType() == 16);
	CHECK(BossFlowerCoinMotion() == 2);
	CHECK(BossFlowerSeedFromRoll(0) == 0);
	CHECK(BossFlowerSeedFromRoll(52) == 52);
	CHECK(BossFlowerSeedFromRoll(53) == -1);
	CHECK(BossFlowerSeedFromRoll(79) == -1);
	CHECK(BossFlowerSeedFromRoll(0x50) == 0x50);
	CHECK(BossFlowerSeedFromRoll(0x54) == -1);
	CHECK(BossFlowerSeedFromRoll(0x5a) == -1);
	CHECK(BossFlowerSeedFromRoll(0x76) == 0x76);
	CHECK(BossFlowerSeedFromRoll(0x7b) == 0x7b);
	CHECK(BossFlowerSeedFromRoll(0x7f) == -1);
	CHECK(BossFlowerSeedFromRoll(127) == -1);
	int allowed = 0;
	for (int r = 0; r < 128; ++r)
		allowed += BossFlowerSeedFromRoll(r) >= 0;
	CHECK(allowed == 78);
}

TEST_CASE_FIXTURE(VanillaWithRulesFixture, "Fork E plants: every rule is inert while modPlants is off") {
	CHECK_FALSE(FireDragonInRange(0, false, true, 2, 300, 2, 300, 3));
	CHECK(FireDragonHitDamage(270) == 0);
	CHECK(FireDragonSpend(600, 1800, true).poisonAdd == 0);
	CHECK(FireDragonAttackCs() == 0);
	CHECK(FireDragonHitAtCountdown() == -1);
	CHECK_FALSE(FireDragonClearsChillAndFreeze());
	CHECK_FALSE(FireDragonZomboniFreezeImmune());
	CHECK(GoldenSunShroomUpdate(0, true, false, 23, 0).state == -1);
	CHECK(GoldenSunShroomPacketStartCounter(0x59, 0) == -1);
	CHECK_FALSE(StrongCoffeeTriggers(1, true));
	CHECK_FALSE(StrongCoffeeWakes(2, 4, 2, 4, false));
	CHECK(CoffeeLikePlantingReason(0x23, false, true, false, 0, false) == 13); // vanilla: needs a sleeping plant
	CHECK(CoffeeLikePlantingReason(0x23, false, true, true, 0, false) == 0);
	CHECK(CoffeeLikePlantingReason(0x73, false, true, true, 0, false) == -1);
	CHECK(CoffeeBeanDeathHeal(0x23, 2, 100, 300).hp == 100);
	CHECK_FALSE(LeafShieldReflects(0, 2, 300, 2, 240, 80));
	CHECK(LeafShieldHpCost(20) == 0);
	CHECK(LeafShieldBounceOf(6).motionType == 6);
	CHECK(ModProjectileDamage(7, 1, 0, false) == 1);
	CHECK(BossFlowerRollRange() == 0);
	CHECK(BossFlowerSeedFromRoll(0) == -1);
}

TEST_CASE("Mod plants: feature on but no pack rules = no effect (a missing key is never a rule)") {
	SetRules(nlohmann::json::object());
	SetModPlantsFeature(true);
	CHECK(ModPlantsOn());
	CHECK_FALSE(IsShadowPlant(0x50));
	CHECK(ShadowThreshold(0x69) == 0);
	CHECK(PrimalRepeaterRollRange() == 0);
	CHECK(PrimalRepeaterShotType(0, 0) == 0);
	CHECK(ShrapnelCherryShards().empty());
	CHECK(SpiritTailRollRange() == 0);
	CHECK(BossFlowerRollRange() == 0);
	CHECK(CharmedTallnutSummonRollRange() == 0);
	CHECK_FALSE(RewindGrassAffects(false, 0, 300, true));
	CHECK(ElementShroomTick(0, 0).resetCountdownTo == 0);
	CHECK_FALSE(SniperTimer(5).fire);
	CHECK(FireDragonHitDamage(270) == 0);
	CHECK(ModProjectileDamage(0x22, 0, 0, false) == 0);
	SetModPlantsFeature(false);
}

TEST_CASE("Mod plant pack file says the same as these test values") {
	const char* home = std::getenv("HOME");
	fs::path file = std::getenv("PVZU_PACK_DIR") ? fs::path(std::getenv("PVZU_PACK_DIR"))
	                                              : fs::path(home ? home : "") / "pvz-unnamed-pack";
	file /= "data/rules-mod-plants.json";
	if (!fs::exists(file)) { MESSAGE("pack not here, skipped: " << file.string()); return; }
	std::ifstream in(file);
	nlohmann::json pack = nlohmann::json::parse(in, nullptr, false);
	REQUIRE(pack.is_object());
	// The file also holds the mod-plant table's own keys (Plan 8 part 1: sprout rolls, packet cell); every behaviour
	// key must be there with the test's value.
	// Keep the table in a variable: items() only refers to it, and a temporary would be gone before the loop runs
	// (GCC/libstdc++ crashed here; C++20 does not extend the lifetime of PackRules()'s result).
	const nlohmann::json aRules = PackRules();
	for (const auto& [key, value] : aRules.items()) {
		CAPTURE(key);
		REQUIRE(pack.contains(key));
		CHECK(pack[key] == value);
	}
}
