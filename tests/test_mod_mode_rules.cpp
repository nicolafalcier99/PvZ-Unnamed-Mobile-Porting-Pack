// PVZU: pure rules of the Unnamed mod-only modes (pvz-unnamed-pack SOURCES-mod-modes.md; every number is a constant in the
// cited .NewSec code).
#include "doctest/doctest.h"
#include "Mod/Unnamed/ModModeRules.h"

using namespace pvzu::unnamed;

TEST_CASE("0x73 Seed Slot Crisis: Rand(6) picks Sunflower, Potato Mine, Snow Pea, Repeater, Wall-nut, Blover (hook 69)") {
	CHECK(SlotCrisisSeed(0) == 1);
	CHECK(SlotCrisisSeed(1) == 4);
	CHECK(SlotCrisisSeed(2) == 5);
	CHECK(SlotCrisisSeed(3) == 7);
	CHECK(SlotCrisisSeed(4) == 3);
	CHECK(SlotCrisisSeed(5) == 27);
}

TEST_CASE("0x77 Upstream: a hit pushes (damage >> 1) + 1 px, at most 160 (hook 706)") {
	CHECK(UpstreamPush(20) == 11);
	CHECK(UpstreamPush(80) == 41);
	CHECK(UpstreamPush(1) == 1);
	CHECK(UpstreamPush(318) == 160);
	CHECK(UpstreamPush(1800) == 160);
}

TEST_CASE("0x6c Zombies Run Run Run: rate x(1 + 0.125*(wave-5)) from wave 6 (0x7a6b80)") {
	CHECK(ZombiesRunFactor(0) == doctest::Approx(1.0f));
	CHECK(ZombiesRunFactor(5) == doctest::Approx(1.0f));
	CHECK(ZombiesRunFactor(6) == doctest::Approx(1.125f));
	CHECK(ZombiesRunFactor(10) == doctest::Approx(1.625f));
	CHECK(ZombiesRunFactor(30) == doctest::Approx(4.125f));
}

TEST_CASE("0x75 Heisenberg: the drawn row is a Rand(5) roll other than the real row (hook 615)") {
	CHECK(HeisenbergRow(2, {2, 2, 4}) == 4);
	CHECK(HeisenbergRow(0, {3}) == 3);
	CHECK(HeisenbergRow(1, {1}) == -1);
}

TEST_CASE("0x82 Sow Beans: pea -> melon, snow pea -> winter melon, zombie pea -> melon (hook 402)") {
	CHECK(SowBeansProjectile(0) == 3);
	CHECK(SowBeansProjectile(1) == 5);
	CHECK(SowBeansProjectile(13) == 3);
	CHECK(SowBeansProjectile(6) == 6); // fireball unchanged
	CHECK(SowBeansProjectile(7) == 7); // star unchanged
}

TEST_CASE("0x6f Force-8 Gale: gust every Rand(1500)+2500 cs, first -> then flips; drift 1 px on roll 1 (hooks 98, 318)") {
	GaleState s;
	CHECK(GaleTick(s, 100) == 1); // timer 0 at wave 8: gust now
	CHECK(s.dir == 1);
	CHECK(s.timer == 2600);
	for (int i = 0; i < 2600; i++)
		CHECK(GaleTick(s, 0) == 0);
	CHECK(s.timer == 0);
	CHECK(GaleTick(s, 0) == 3); // -> becomes <-
	CHECK(s.dir == -1);
	CHECK(s.timer == 2500);
	s.timer = 0;
	CHECK(GaleTick(s, 1499) == 2); // <- becomes ->
	CHECK(s.dir == 1);
	CHECK(s.timer == 3999);
	CHECK(GaleDrift(1, 0) == 0);
	CHECK(GaleDrift(1, 1) == 1);
	CHECK(GaleDrift(-1, 1) == -1);
	CHECK(GaleDrift(1, 2) == 0);
	CHECK(GaleDrift(0, 1) == 0);
}

TEST_CASE("0x7b Frequency Switch: off until the first wave, then plants/zombies hidden in turns of 400..600 cs") {
	FrequencyState s;
	FrequencyTick(s, 0, 0);
	CHECK(s.f == 0);
	FrequencyTick(s, 1, 50);
	CHECK(s.f == 1);
	CHECK(s.timer == 450);
	for (int i = 0; i < 449; i++)
		FrequencyTick(s, 1, 0);
	CHECK(s.f == 1);
	FrequencyTick(s, 1, 200);
	CHECK(s.f == 2);
	CHECK(s.timer == 600);
	s.timer = 1;
	FrequencyTick(s, 2, 0);
	CHECK(s.f == 1);
}

TEST_CASE("0x7c Light and Mist: sun radius by coin type, row (y-90)/85 (hook 135)") {
	CHECK(LightSunRadius(4) == 2);
	CHECK(LightSunRadius(5) == 1);
	CHECK(LightSunRadius(6) == 3);
	CHECK(LightSunRadius(0) == 0);
	CHECK(LightSunRow(90) == 0);
	CHECK(LightSunRow(175) == 1);
	CHECK(LightSunRow(600) == 6);
	CHECK(LightSunRow(80) == 0); // C division truncates toward zero
}

TEST_CASE("0x65 Art Is an Explosion: large sun on Rand(4) >= 2, then a sun on a second roll (hook 319)") {
	CHECK(ArtExplosionCoins(0, 3) == std::array<int, 2>{-1, -1});
	CHECK(ArtExplosionCoins(1, 3) == std::array<int, 2>{-1, -1});
	CHECK(ArtExplosionCoins(2, 0) == std::array<int, 2>{2, -1});
	CHECK(ArtExplosionCoins(3, 2) == std::array<int, 2>{3, 2});
	CHECK(ArtExplosionCoins(2, 3) == std::array<int, 2>{2, 3});
}

TEST_CASE("0x72 Lone Wolf: one upgrade step per head at 100/750/2000/5000 sun (0x798505..0x7985a3)") {
	CHECK(LoneWolfUpgrade(0, 99) == 0);
	CHECK(LoneWolfUpgrade(0, 100) == 5);
	CHECK(LoneWolfUpgrade(5, 700) == 5);
	CHECK(LoneWolfUpgrade(5, 750) == 7);
	CHECK(LoneWolfUpgrade(0, 750) == 5);
	CHECK(LoneWolfUpgrade(7, 2000) == 18);
	CHECK(LoneWolfUpgrade(5, 2000) == 7);
	CHECK(LoneWolfUpgrade(18, 4999) == 18);
	CHECK(LoneWolfUpgrade(18, 5000) == 40);
	CHECK(LoneWolfUpgrade(0, 9000) == 5);
	CHECK(LoneWolfUpgrade(40, 9000) == 40);
}

TEST_CASE("0x6a Downstream: a bite pushes the plant 40..80 px left; x <= -32 is off the lawn (hook 679)") {
	DownstreamPush p = DownstreamBite(400, 0);
	CHECK(p.x == 360);
	CHECK(p.col == 4);
	CHECK_FALSE(p.offLawn);
	p = DownstreamBite(400, 40);
	CHECK(p.x == 320);
	CHECK(p.col == 4);
	p = DownstreamBite(40, 40);
	CHECK(p.x == -40);
	CHECK(p.col == 0);
	CHECK(p.offLawn);
	p = DownstreamBite(50, 40);
	CHECK(p.x == -30);
	CHECK(p.col == 0);
	CHECK_FALSE(p.offLawn);
}

TEST_CASE("0x68 War of Attrition: flips after even waves; the exe's nocturnal list") {
	CHECK_FALSE(AttritionFlips(0));
	CHECK_FALSE(AttritionFlips(1));
	CHECK(AttritionFlips(2));
	CHECK_FALSE(AttritionFlips(3));
	CHECK(AttritionFlips(4));
	for (int s : {8, 9, 10, 12, 13, 14, 15, 24, 31, 42})
		CHECK(AttritionNocturnal(s));
	for (int s : {0, 1, 11, 16, 23, 25, 48})
		CHECK_FALSE(AttritionNocturnal(s));
}
