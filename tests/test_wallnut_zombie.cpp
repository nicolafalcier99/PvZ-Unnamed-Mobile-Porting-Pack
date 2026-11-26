// PVZU: Plan 9 R6, the Wall-nut Zombie's self-copy (Almanac 神谕心法) as pure rules. Evidence:
// the research notes (r6-wallnut-zombie) and pvz-unnamed-pack/SOURCES-wallnut-zombie.md (exe 0x7971ea..0x797297, 0x797d32).
#include "doctest/doctest.h"
#include "Mod/Unnamed/Rules.h"
#include "Mod/Unnamed/World7Zombies.h"

#include "nlohmann/json.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <vector>

using namespace pvzu::unnamed;
namespace fs = std::filesystem;

namespace {

// The Unnamed values (same as pvz-unnamed-pack/data/rules-wallnut-zombie.json; the last test compares the two).
const nlohmann::json kWallnutRules = {
	{"wallnutZombie.copy.on", 1},
	{"wallnutZombie.copy.quietTicks", 3600},
	{"wallnutZombie.copy.maxX", 800},
};

struct WallnutRulesOn {
	explicit WallnutRulesOn(nlohmann::json extra = nlohmann::json::object()) {
		nlohmann::json all = kWallnutRules;
		for (auto it = extra.begin(); it != extra.end(); ++it)
			all[it.key()] = it.value();
		SetRules(all);
	}
	~WallnutRulesOn() { SetRules(nlohmann::json::object()); }
};

// an eligible Wall-nut Zombie on screen
WallnutCopyView Nut() {
	WallnutCopyView v;
	v.type = ZOMBIE_WALLNUT_HEAD;
	v.helm = HELMTYPE_WALLNUT;
	v.fromWave = 3;
	v.x = 600;
	return v;
}

} // namespace

TEST_CASE("WallnutCopyIsOffWithoutRules") {
	SetRules(nlohmann::json::object());
	CHECK_FALSE(WallnutCopyOn());
	CHECK_FALSE(WallnutCopyEligible(Nut()));
	// the tick still counts nothing towards a copy: quiet ticks 0 = never
	WallnutCopyStep s{0, 0, false};
	for (int i = 0; i < 5000; ++i) {
		s = WallnutCopyTick(s.lastHelmHp, s.quietTicks, 1600);
		CHECK_FALSE(s.copy);
	}
}

TEST_CASE("WallnutCopyEligibility") { // 0x7971eb, 0x7971f5, 0x797d32..0x797d6a
	WallnutRulesOn on;
	CHECK(WallnutCopyEligible(Nut()));
	{ auto v = Nut(); v.type = ZOMBIE_TALLNUT_HEAD; CHECK_FALSE(WallnutCopyEligible(v)); }   // 0x1f is R7's block
	{ auto v = Nut(); v.type = ZOMBIE_NORMAL; CHECK_FALSE(WallnutCopyEligible(v)); }
	{ auto v = Nut(); v.helm = HELMTYPE_NONE; CHECK_FALSE(WallnutCopyEligible(v)); }        // nut eaten away
	{ auto v = Nut(); v.helm = HELMTYPE_PAIL; CHECK_FALSE(WallnutCopyEligible(v)); }
	{ auto v = Nut(); v.fromWave = 0; CHECK(WallnutCopyEligible(v)); }                     // a copy (wave 0) copies too
	{ auto v = Nut(); v.fromWave = -1; CHECK_FALSE(WallnutCopyEligible(v)); }              // debug / cut-scene / UI
	{ auto v = Nut(); v.fromWave = -2; CHECK_FALSE(WallnutCopyEligible(v)); }
	{ auto v = Nut(); v.x = 800; CHECK(WallnutCopyEligible(v)); }                          // `jg` 0x320
	{ auto v = Nut(); v.x = 801; CHECK_FALSE(WallnutCopyEligible(v)); }
	{ auto v = Nut(); v.x = -40; CHECK(WallnutCopyEligible(v)); }
	{ auto v = Nut(); v.dead = true; CHECK_FALSE(WallnutCopyEligible(v)); }
	{ auto v = Nut(); v.hasHead = false; CHECK_FALSE(WallnutCopyEligible(v)); }
	{ auto v = Nut(); v.hypnotised = true; CHECK_FALSE(WallnutCopyEligible(v)); }
	WallnutRulesOn wide({{"wallnutZombie.copy.maxX", 900}});
	{ auto v = Nut(); v.x = 850; CHECK(WallnutCopyEligible(v)); }
}

TEST_CASE("WallnutCopyTickCountsQuietTicks") { // 0x797208..0x79723f
	WallnutRulesOn on;
	CHECK(WallnutCopyQuietTicks() == 3600);
	// first tick: last seen 0 (fresh zombie), helm 1600 -> counts
	CHECK(WallnutCopyTick(0, 0, 1600) == WallnutCopyStep{1600, 1, false});
	// same helm: counts on
	CHECK(WallnutCopyTick(1600, 41, 1600) == WallnutCopyStep{1600, 42, false});
	// helm went down: counter back to 0, last seen updated, no copy
	CHECK(WallnutCopyTick(1600, 3000, 1580) == WallnutCopyStep{1580, 0, false});
	// helm went up (a heal): counts (`jle`)
	CHECK(WallnutCopyTick(1500, 7, 1510) == WallnutCopyStep{1510, 8, false});
	// the 3600th quiet tick copies and starts again
	CHECK(WallnutCopyTick(1600, 3599, 1600) == WallnutCopyStep{1600, 0, true});
}

TEST_CASE("WallnutCopyEvery36SecondsUntilHit") {
	WallnutRulesOn on;
	WallnutCopyStep s{0, 0, false};
	int helm = 1600;
	std::vector<int> copies;
	for (int t = 1; t <= 12000; ++t) {
		if (t == 5000) helm -= 20;   // a pea at tick 5000
		s = WallnutCopyTick(s.lastHelmHp, s.quietTicks, helm);
		if (s.copy) copies.push_back(t);
	}
	// 7200 would be next, but the hit at 5000 restarts the count: 3600 quiet ticks after it, then 12200 > 12000
	CHECK(copies == std::vector<int>{3600, 8600});
}

TEST_CASE("WallnutCopyDamageMatchesTheHelm") { // 0x79727e: TakeDamage(copy helm - own helm, flags 1)
	WallnutRulesOn on;
	CHECK(WallnutCopyDamage(1600, 1600) == 0);
	CHECK(WallnutCopyDamage(1600, 420) == 1180);
	CHECK(WallnutCopyDamage(1600, 1700) == -100);   // TakeDamage then does nothing, as in the exe
	CHECK(WallnutCopyDamageFlags() == 1);           // bit 0 = DAMAGE_BYPASSES_SHIELD
	CHECK(WallnutCopyFromWave() == 0);              // IZombiePlaceZombie adds with fromWave 0
}

TEST_CASE("WallnutPackRulesMatchTheseValues") {
	WallnutRulesOn on;
	for (const auto& k : WallnutZombieRuleKeys()) CHECK_MESSAGE(kWallnutRules.contains(k), k);
	CHECK(WallnutZombieRuleKeys().size() == kWallnutRules.size());
	const char* home = std::getenv("HOME");
	fs::path dir = std::getenv("PVZU_PACK_DIR") ? fs::path(std::getenv("PVZU_PACK_DIR"))
	                                             : fs::path(home ? home : "") / "pvz-unnamed-pack";
	fs::path file = dir / "data/rules-wallnut-zombie.json";
	if (!fs::exists(dir / "mod.json")) { MESSAGE("pack not here, skipped: " << dir.string()); return; }
	REQUIRE(fs::exists(file));
	std::ifstream in(file);
	nlohmann::json pack = nlohmann::json::parse(in, nullptr, false);
	REQUIRE(pack.is_object());
	CHECK(pack == kWallnutRules);
	std::ifstream mj(dir / "mod.json");
	nlohmann::json mod = nlohmann::json::parse(mj, nullptr, false);
	REQUIRE(mod.is_object());
	CHECK(mod["features"].value("wallnutZombieCopy", false));
}
