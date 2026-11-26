// PVZU: Plan 9 R7, the Tall-nut Zombie (type 31): Spirit Regeneration and the 1-in-12 form 0x101 (pure rules).
// Evidence: the research notes (r7-tallnut-zombie) and pvz-unnamed-pack/SOURCES-tallnut-zombie.md
// (hook 618 -> 0x79729c..0x797302, 0xc033f0, 0xc03360).
#include "doctest/doctest.h"
#include "Mod/Unnamed/Rules.h"
#include "Mod/Unnamed/World7Zombies.h"

#include "nlohmann/json.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <random>

using namespace pvzu::unnamed;
namespace fs = std::filesystem;

namespace {

// The Unnamed values (same as pvz-unnamed-pack/data/rules-tallnut-zombie.json; the last test compares the two).
const nlohmann::json kTallnutRules = {
	{"tallnutZombie.regen.periodCs", 100},
	{"tallnutZombie.regen.hp", 10},
	{"tallnutZombie.regen.undamagedExtraHp", 10},
	{"tallnutZombie.odd.summonPeriodCs", 1200},
	{"tallnutZombie.odd.summonAtCs", 600},
	{"tallnutZombie.odd.summonRoll", 8},
	{"tallnutZombie.odd.gargantuarBelow", 5},
	{"tallnutZombie.odd.riseCounterCs", 50},
	{"tallnutZombie.odd.alpha", 176},
};

struct TallnutRulesOn {
	TallnutRulesOn() { SetRules(kTallnutRules); }
	~TallnutRulesOn() { SetRules(nlohmann::json::object()); }
};

using R = TallnutRegen;

} // namespace

TEST_CASE("TallnutZombieVanillaWithoutRules") {
	SetRules(nlohmann::json::object());
	// no key: the counter never moves, no HP is added
	CHECK(TallnutZombieRegenTick(HELMTYPE_TALLNUT, 2400, 99, 0) == R{2400, 99, 0, 0});
	CHECK_FALSE(TallnutOddSummonsNow(w7::TALLNUT_ODD, true, 600));
	CHECK(TallnutOddSummonRollRange() == 0);
	CHECK(TallnutOddAlpha() < 0);
}

TEST_CASE("TallnutZombieRegenCountsToOneSecond") { // 0x7972b3: inc +0x114, cmp 0x64, jl
	TallnutRulesOn on;
	CHECK(TallnutZombieRegenTick(HELMTYPE_TALLNUT, 2400, 0, 2400) == R{2400, 1, 2400, 0});
	CHECK(TallnutZombieRegenTick(HELMTYPE_TALLNUT, 2400, 98, 2400) == R{2400, 99, 2400, 0});
	// the 100th update: counter back to 0 and the helm heals
	CHECK(TallnutZombieRegenTick(HELMTYPE_TALLNUT, 2400, 99, 2400).counter == 0);
	CHECK(TallnutZombieRegenTick(HELMTYPE_TALLNUT, 2400, 99, 2400).healed > 0);
}

TEST_CASE("TallnutZombieRegenDoublesWhenNotHurt") { // 0x7972d0..0x7972fc
	TallnutRulesOn on;
	// HP before (2000) >= recorded (2000): not hurt this second -> +20, record = new HP
	CHECK(TallnutZombieRegenTick(HELMTYPE_TALLNUT, 2000, 99, 2000) == R{2020, 0, 2020, 20});
	// hurt since the last regeneration (recorded 2000 > 1990) -> +10 only, record = 2000
	CHECK(TallnutZombieRegenTick(HELMTYPE_TALLNUT, 1990, 99, 2000) == R{2000, 0, 2000, 10});
	// the first regeneration of a new zombie: record 0 -> doubled
	CHECK(TallnutZombieRegenTick(HELMTYPE_TALLNUT, 2400, 99, 0) == R{2420, 0, 2420, 20});
}

TEST_CASE("TallnutZombieRegenHasNoCap") { // nothing reads +0xd4 (helm max)
	TallnutRulesOn on;
	int hp = 2400, counter = 0, recorded = 0;
	for (int tick = 0; tick < 100 * 60; ++tick) { // one minute untouched
		const R r = TallnutZombieRegenTick(HELMTYPE_TALLNUT, hp, counter, recorded);
		hp = r.helmHp;
		counter = r.counter;
		recorded = r.recordedHp;
	}
	CHECK(hp == 2400 + 60 * 20);
}

TEST_CASE("TallnutZombieRegenNeedsTheTallnutHelm") { // 0xc01316: cmp [esi+0xc4], 9
	TallnutRulesOn on;
	CHECK(TallnutZombieRegenTick(HELMTYPE_NONE, 0, 99, 0) == R{0, 99, 0, 0});
	CHECK(TallnutZombieRegenTick(HELMTYPE_WALLNUT, 1600, 99, 1600) == R{1600, 99, 1600, 0});
}

TEST_CASE("TallnutZombieRegenUnderSteadyFire") { // hurt every second: +10 a second
	TallnutRulesOn on;
	int hp = 2400, counter = 0, recorded = 0, healed = 0;
	for (int tick = 0; tick < 100 * 10; ++tick) {
		if (tick % 50 == 25)
			hp -= 20; // a pea every half second
		const R r = TallnutZombieRegenTick(HELMTYPE_TALLNUT, hp, counter, recorded);
		hp = r.helmHp;
		counter = r.counter;
		recorded = r.recordedHp;
		healed += r.healed;
	}
	CHECK(healed == 20 + 9 * 10); // the first second is doubled (record 0), then hurt every second
	CHECK(hp == 2400 - 20 * 20 + healed);
}

TEST_CASE("TallnutOddSummonTiming") { // 0xc033f0: scene 3, age % 1200 == 600 (age before ++)
	TallnutRulesOn on;
	CHECK(TallnutOddSummonsNow(w7::TALLNUT_ODD, true, 600));
	CHECK(TallnutOddSummonsNow(w7::TALLNUT_ODD, true, 1800));
	CHECK(TallnutOddSummonsNow(w7::TALLNUT_ODD, true, 3000));
	CHECK_FALSE(TallnutOddSummonsNow(w7::TALLNUT_ODD, true, 0));
	CHECK_FALSE(TallnutOddSummonsNow(w7::TALLNUT_ODD, true, 599));
	CHECK_FALSE(TallnutOddSummonsNow(w7::TALLNUT_ODD, true, 1200));
	CHECK_FALSE(TallnutOddSummonsNow(w7::TALLNUT_ODD, false, 600)); // not SCENE_PLAYING
	CHECK_FALSE(TallnutOddSummonsNow(0, true, 600));                 // the ordinary Tall-nut Zombie
	CHECK_FALSE(TallnutOddSummonsNow(0x100, true, 600));             // the mode-0x8f boss form
}

TEST_CASE("TallnutOddSummonType") { // Rand(8): 0..4 Gargantuar (0x17), 5..7 Football (7)
	TallnutRulesOn on;
	CHECK(TallnutOddSummonRollRange() == 8);
	for (int roll = 0; roll <= 4; ++roll) CHECK(TallnutOddSummonType(roll) == ZOMBIE_GARGANTUAR);
	for (int roll = 5; roll <= 7; ++roll) CHECK(TallnutOddSummonType(roll) == ZOMBIE_FOOTBALL);
	std::mt19937 rng{20261123};
	int garg = 0;
	constexpr int kRolls = 4000;
	for (int i = 0; i < kRolls; ++i)
		garg += TallnutOddSummonType(std::uniform_int_distribution<int>(0, TallnutOddSummonRollRange() - 1)(rng)) == ZOMBIE_GARGANTUAR;
	CHECK(garg > kRolls * 5 / 8 - 150);
	CHECK(garg < kRolls * 5 / 8 + 150);
}

TEST_CASE("TallnutOddRiseAndLook") {
	TallnutRulesOn on;
	CHECK(TallnutOddRiseCounterCs() == 50);   // mov [new+0x68], 0x32
	CHECK(TallnutOddAlpha() == 176);          // 0xc0337a: mov edi, 0xb0
	CHECK(TallnutOddSummonFromWave() >= 0);   // a non-wave number (the exe passes the Board pointer)
	CHECK(TallnutOddSummonFromWave() > 100);
}

TEST_CASE("TallnutPackRulesMatchTheseValues") {
	TallnutRulesOn on;
	for (const auto& k : TallnutZombieRuleKeys()) CHECK_MESSAGE(kTallnutRules.contains(k), k);
	CHECK(TallnutZombieRuleKeys().size() == kTallnutRules.size());
	const char* home = std::getenv("HOME");
	fs::path dir = std::getenv("PVZU_PACK_DIR") ? fs::path(std::getenv("PVZU_PACK_DIR"))
	                                             : fs::path(home ? home : "") / "pvz-unnamed-pack";
	fs::path file = dir / "data/rules-tallnut-zombie.json";
	if (!fs::exists(dir / "mod.json")) { MESSAGE("pack not here, skipped: " << dir.string()); return; }
	REQUIRE(fs::exists(file));
	std::ifstream in(file);
	nlohmann::json pack = nlohmann::json::parse(in, nullptr, false);
	REQUIRE(pack.is_object());
	CHECK(pack == kTallnutRules);
	std::ifstream mj(dir / "mod.json");
	nlohmann::json mod = nlohmann::json::parse(mj, nullptr, false);
	REQUIRE(mod.is_object());
	CHECK(mod["features"].value("tallnutZombie", false));
}
