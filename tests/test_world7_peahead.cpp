// PVZU: Plan 9 R5, the Peashooter Zombie fire-rate ramp (pure rules; evidence in
// the research notes (r5-peashooter-zombie) and pvz-unnamed-pack/SOURCES-peahead-ramp.md: hook 572's tail 0x7958fe).
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

// The Unnamed values (same as pvz-unnamed-pack/data/rules-peahead-ramp.json; the last test compares the two).
const nlohmann::json kPeaRampRules = {
	{"peaHeadRamp.roll", 75},
	{"peaHeadRamp.baseInterval", 150},
};

struct PeaRampRulesOn {
	PeaRampRulesOn() { SetRules(kPeaRampRules); }
	~PeaRampRulesOn() { SetRules(nlohmann::json::object()); }
};

} // namespace

TEST_CASE("PeaHeadRampVanillaWithoutRules") {
	SetRules(nlohmann::json::object());
	CHECK(PeaHeadRampRollRange() == 0);           // no roll drawn
	CHECK(PeaHeadRampAfterShot(0, 0) == 0);       // never raises
	CHECK(PeaHeadShotInterval(0, 150) == 150);    // vanilla mPhaseCounter = 150 (0x5275af)
	CHECK(PeaHeadShotInterval(40, 150) == 150);   // even with a raise count left over
}

TEST_CASE("PeaHeadRampRaisesWhenRollReachesCount") {
	PeaRampRulesOn on;
	// 0x795909: Rand(75); 0x795913: cmp roll, [Zombie+0x12c]; jl -> no raise; else inc
	CHECK(PeaHeadRampRollRange() == 75);
	CHECK(PeaHeadRampAfterShot(0, 0) == 1);       // the first shot always raises
	CHECK(PeaHeadRampAfterShot(10, 9) == 10);     // roll < n: no raise
	CHECK(PeaHeadRampAfterShot(10, 10) == 11);    // roll == n: raise
	CHECK(PeaHeadRampAfterShot(10, 74) == 11);
	CHECK(PeaHeadRampAfterShot(74, 74) == 75);    // last possible raise
	for (int r = 0; r < 75; ++r) CHECK_MESSAGE(PeaHeadRampAfterShot(75, r) == 75, r);   // Rand(75) < 75: capped
}

TEST_CASE("PeaHeadShotIntervalIs150MinusRaises") {
	PeaRampRulesOn on;
	// 0x795925: mov eax, 150; sub eax, [Zombie+0x12c]; mov [Zombie+0x68], eax
	CHECK(PeaHeadShotInterval(0, 150) == 150);
	CHECK(PeaHeadShotInterval(1, 150) == 149);
	CHECK(PeaHeadShotInterval(75, 150) == 75);    // Almanac: at most +100% fire rate
	CHECK(PeaHeadShotInterval(200, 150) == 75);   // a bad saved count never goes past the cap
	CHECK(PeaHeadShotInterval(-5, 150) == 150);
}

TEST_CASE("PeaHeadRampChanceFallsWithRaises") {
	PeaRampRulesOn on;
	// chance of a raise = (75 - n) / 75 (Almanac: the fewer raises so far, the higher the chance)
	for (int n : {0, 25, 50, 74}) {
		int raises = 0;
		for (int r = 0; r < 75; ++r) raises += PeaHeadRampAfterShot(n, r) - n;
		CHECK_MESSAGE(raises == 75 - n, n);
	}
	// over a long life the count climbs to the cap; ~370 shots on average (75 * H(75))
	std::mt19937 rng(20261123);
	int n = 0, shots = 0;
	while (n < 75 && shots < 5000) {
		n = PeaHeadRampAfterShot(n, std::uniform_int_distribution<int>(0, 74)(rng));
		++shots;
	}
	CHECK(n == 75);
	CHECK(shots > 150);
	CHECK(shots < 1500);
}

TEST_CASE("PeaHeadRampPackRulesMatchTheseValues") {
	PeaRampRulesOn on;
	for (const auto& k : PeaHeadRampRuleKeys()) CHECK_MESSAGE(kPeaRampRules.contains(k), k);
	CHECK(PeaHeadRampRuleKeys().size() == kPeaRampRules.size());
	const char* home = std::getenv("HOME");
	fs::path dir = std::getenv("PVZU_PACK_DIR") ? fs::path(std::getenv("PVZU_PACK_DIR"))
	                                             : fs::path(home ? home : "") / "pvz-unnamed-pack";
	fs::path file = dir / "data/rules-peahead-ramp.json";
	if (!fs::exists(file)) { MESSAGE("pack not here, skipped: " << file.string()); return; }
	std::ifstream in(file);
	nlohmann::json pack = nlohmann::json::parse(in, nullptr, false);
	REQUIRE(pack.is_object());
	CHECK(pack == kPeaRampRules);
	std::ifstream mj(dir / "mod.json");
	nlohmann::json mod = nlohmann::json::parse(mj, nullptr, false);
	REQUIRE(mod.is_object());
	CHECK(mod["features"].value("peaHeadRamp", false));
}
