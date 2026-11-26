// PVZU: Plan 9 R9 small rules (the research notes (r9-small-rules); exe reads in pvz-unnamed-pack/SOURCES-small-rules.md).
// The pity hypnotise of plain stars (DoImpact hooks 396/397, 0xc10000 counter, 0xc10050 roll) and the Last Stand /
// 0x66 / 0x78 chooser rows for the mod sun plants 0x51 / 0x59 (0x7b0f85). Rolls are passed in. Every rule is also
// checked with the "smallRules" feature off (vanilla mode) while the rules are loaded. The last test compares the
// fixture with the pack file.
#include "doctest/doctest.h"
#include "Mod/GameData.h"
#include "Mod/ModPack.h"
#include "Mod/Unnamed/ModesVanilla.h"
#include "Mod/Unnamed/Rules.h"
#include "Mod/Unnamed/SmallRules.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>

using namespace pvzu;
using namespace pvzu::unnamed;
namespace fs = std::filesystem;

namespace {
// Same values as pvz-unnamed-pack/data/rules-small-rules.json (the last test compares the two).
nlohmann::json PackRules() {
	return nlohmann::json{
		{"smallRules.pity.rollRange", 14370},
		{"smallRules.pity.counterAt", 14370},
		{"smallRules.flyingHitByFlags", 13},
		{"modesVanilla.bannedSeeds.31", {81, 89}},
		{"modesVanilla.bannedSeeds.102", {81, 89}},
		{"modesVanilla.bannedSeeds.120", {81, 89}},
	};
}

void SetFeature(bool on) {
	GameData::Get().ResetToVanilla();
	if (on) {
		ModPack p;
		p.id = "test-small-rules";
		p.features["smallRules"] = true;
		std::string err;
		REQUIRE_MESSAGE(GameData::Get().ApplyPack(p, &err), err);
	}
}

struct UnnamedSmall {
	UnnamedSmall() { SetRules(PackRules()); SetFeature(true); }
	~UnnamedSmall() { SetRules(nlohmann::json::object()); SetFeature(false); }
};
struct VanillaWithRules {
	VanillaWithRules() { SetRules(PackRules()); SetFeature(false); }
	~VanillaWithRules() { SetRules(nlohmann::json::object()); }
};

constexpr int kProjPea = 0, kProjCabbage = 2, kProjStar = 7, kProjSpike = 8;
constexpr int kLastStand = 31, kSunStorm = 0x66, kMode78 = 0x78, kGraveDanger = 45;
constexpr int kSeedSunflower = 1, kSeedPrimalSunflower = 0x51, kSeedGoldenSunShroom = 0x59, kSeedShadowPea = 0x50;
}

TEST_CASE_FIXTURE(UnnamedSmall, "Pity: 1 in 14370, counter full at 14370 (0xc10050: cmp [0xc10033], 0x3822; Rand(0x3822))") {
	CHECK(SmallRulesOn());
	CHECK(PityRollRange() == 14370);
	CHECK(PityCounterAt() == 14370);
}

TEST_CASE_FIXTURE(UnnamedSmall, "Pity counter: +1 per star damage outside I, Zombie only (0xc10000: cmp ebx, 7; call 0x4537d0)") {
	CHECK(PityCountsStar(kProjStar, false));
	CHECK_FALSE(PityCountsStar(kProjStar, true));
	CHECK_FALSE(PityCountsStar(kProjPea, false));
	CHECK_FALSE(PityCountsStar(kProjSpike, false));
	CHECK_FALSE(PityCountsStar(0x16, false)); // mod types >= 0xe never reach 0xc10000
}

TEST_CASE_FIXTURE(UnnamedSmall, "Pity candidate: a plain star (+0x80 == 0), not the Shrapnel core, on a zombie that is not boss class") {
	CHECK(PityCandidate(kProjStar, 0, false, false));
	CHECK_FALSE(PityCandidate(kProjStar, 1, false, false));  // kind stars take the kind block (0xc10050 jg)
	CHECK_FALSE(PityCandidate(kProjStar, 5, false, false));
	CHECK_FALSE(PityCandidate(kProjStar, -1, false, false)); // jne 0xc100a2
	CHECK_FALSE(PityCandidate(kProjStar, 0, true, false));   // 0x79ef88: +0x84 == 2 skips
	CHECK_FALSE(PityCandidate(kProjStar, 0, false, true));   // 0x7aa320
	CHECK_FALSE(PityCandidate(kProjPea, 0, false, false));   // 0xc0571e: cmp [edi+0x5c], 7; jne TakeDamage
	CHECK_FALSE(PityCandidate(kProjCabbage, 0, false, false));
	CHECK_FALSE(PityCandidate(kProjSpike, 0, false, false));
}

TEST_CASE_FIXTURE(UnnamedSmall, "Pity roll: only while the counter is below 14370; roll 0 hypnotises; a full counter always does") {
	CHECK(PityNeedsRoll(0));
	CHECK(PityNeedsRoll(14369));
	CHECK_FALSE(PityNeedsRoll(14370)); // jge: no Rand drawn
	CHECK_FALSE(PityNeedsRoll(20000));
	CHECK(PityHypnotises(0, 0));
	CHECK_FALSE(PityHypnotises(0, 1));
	CHECK_FALSE(PityHypnotises(14369, 14369));
	CHECK(PityHypnotises(14370, 7)); // roll ignored
	CHECK(PityHypnotises(14370, -1));
}

TEST_CASE_FIXTURE(VanillaWithRules, "Pity: feature off = no counter, no candidate, no hypnotise") {
	CHECK_FALSE(SmallRulesOn());
	CHECK(PityRollRange() == 0);
	CHECK_FALSE(PityCountsStar(kProjStar, false));
	CHECK_FALSE(PityCandidate(kProjStar, 0, false, false));
	CHECK_FALSE(PityNeedsRoll(0));
	CHECK_FALSE(PityHypnotises(14370, 0));
	CHECK_FALSE(PityHypnotises(0, 0));
}

TEST_CASE("Pity: feature on but no pack rules = no effect") {
	SetRules(nlohmann::json::object());
	SetFeature(true);
	CHECK(PityRollRange() == 0);
	CHECK_FALSE(PityCandidate(kProjStar, 0, false, false));
	CHECK_FALSE(PityHypnotises(0, 0));
	SetFeature(false);
}

TEST_CASE_FIXTURE(UnnamedSmall, "Chooser: Last Stand, 0x66 and 0x78 also bar Primal Sunflower 0x51 and Golden Sun-shroom 0x59 (0x7b0fb2..0x7b0fbe)") {
	for (int aMode : {kLastStand, kSunStorm, kMode78}) {
		CHECK(SeedBannedInMode(aMode, kSeedPrimalSunflower));
		CHECK(SeedBannedInMode(aMode, kSeedGoldenSunShroom));
		CHECK_FALSE(SeedBannedInMode(aMode, kSeedShadowPea));
		CHECK_FALSE(SeedBannedInMode(aMode, kSeedSunflower)); // the vanilla five: engine (31) and modes.json (0x66, 0x78)
	}
	CHECK_FALSE(SeedBannedInMode(kGraveDanger, kSeedGoldenSunShroom));
	CHECK_FALSE(SeedBannedInMode(0, kSeedGoldenSunShroom)); // Adventure
}

// V2 (C10, 1.3beta1 "[quote not included]"): hook 715 (0x531c52 -> 0xc11cb0), the last ground test of
// Zombie::EffectedByDamage: a flying zombie is hit when the flags' low byte is exactly 13 (lobbers 0x45ebb5, Squash)
TEST_CASE_FIXTURE(UnnamedSmall, "Flying zombies: flags 13 (lobbed shots, Squash) hit them, other ground flags do not") {
	CHECK(FlagsHitFlying(13));
	CHECK(FlagsHitFlying(0x10d)); // `cmp bl, 0xd`: the low byte only
	CHECK_FALSE(FlagsHitFlying(1));
	CHECK_FALSE(FlagsHitFlying(9));
	CHECK_FALSE(FlagsHitFlying(11));
	CHECK_FALSE(FlagsHitFlying(0x8d));
}

TEST_CASE_FIXTURE(VanillaWithRules, "Flying zombies: feature off = vanilla (no ground flags hit them)") {
	CHECK_FALSE(FlagsHitFlying(13));
}

TEST_CASE("Small rules: the fixture equals pvz-unnamed-pack/data/rules-small-rules.json") {
	const char* home = std::getenv("HOME");
	fs::path file = std::getenv("PVZU_PACK_DIR") ? fs::path(std::getenv("PVZU_PACK_DIR"))
	                                              : fs::path(home ? home : "") / "pvz-unnamed-pack";
	file /= "data/rules-small-rules.json";
	if (!fs::exists(file)) { MESSAGE("pack not here, skipped: " << file.string()); return; }
	std::ifstream in(file);
	nlohmann::json pack = nlohmann::json::parse(in, nullptr, false);
	REQUIRE(pack.is_object());
	CHECK(pack == PackRules());
}
