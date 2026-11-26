// PVZU: Unnamed moved vanilla's x-800 gameplay edge to its 1067-wide screen (bug 8: zombies could not be hit
// until they reached the lawn, because they start 133 px further right).
#include "doctest/doctest.h"
#include "Mod/Unnamed/Rules.h"
#include "Mod/Unnamed/WideReach.h"

#include "nlohmann/json.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>

using namespace pvzu::unnamed;
namespace fs = std::filesystem;

namespace {
const nlohmann::json kFixRules = {
	{"wideReach.zombieHitMaxX", 1067},   // .text 0x531BEC `cmp dword [esp+8], 0x42b` in Zombie::EffectedByDamage
	{"wideReach.projectileMaxX", 1068},  // hook 377 0x955AB0 `push 0x44858000` (1068.0f) in Projectile::CheckForCollision
};
}

TEST_CASE("WideReachIsVanillaWithoutRules") {
	SetRules(nlohmann::json::object());
	CHECK(ZombieHittableMaxX(800) == 800);
	CHECK(ProjectileMaxX(800.0f) == 800.0f);
}

TEST_CASE("WideReachUsesUnnamedEdges") {
	SetRules(kFixRules);
	CHECK(ZombieHittableMaxX(800) == 1067);
	CHECK(ProjectileMaxX(800.0f) == 1068.0f);
	// A Browncoat entering at the Unnamed start (780 + 133 = 913, rect x = 913 + 36 = 949) can be hit at once.
	CHECK(949 <= ZombieHittableMaxX(800));
	SetRules(nlohmann::json::object());
}

TEST_CASE("FixRulesFileMatches") {
	for (const auto& k : WideReachRuleKeys()) CHECK_MESSAGE(kFixRules.contains(k), k);
	CHECK(WideReachRuleKeys().size() == kFixRules.size());
	const char* home = std::getenv("HOME");
	fs::path file = std::getenv("PVZU_PACK_DIR") ? fs::path(std::getenv("PVZU_PACK_DIR"))
	                                              : fs::path(home ? home : "") / "pvz-unnamed-pack";
	file /= "data/rules-fixes.json";
	if (!fs::exists(file)) { MESSAGE("pack not here, skipped: " << file.string()); return; }
	std::ifstream in(file);
	nlohmann::json pack = nlohmann::json::parse(in, nullptr, false);
	REQUIRE(pack.is_object());
	CHECK(pack == kFixRules);
}
