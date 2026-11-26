// PVZU: Plan 9 R2, the Vase Pole Vaulter (elite 0x103): vault vase, landing opens vases, vase zombies change side.
// Pure rules; evidence in the research notes (r2-vase-pole) and pvz-unnamed-pack/SOURCES-vase-pole.md.
#include "doctest/doctest.h"
#include "Mod/Unnamed/DayZombies.h"
#include "Mod/Unnamed/Levels.h"
#include "Mod/Unnamed/Rules.h"
#include "Mod/Unnamed/State.h"
#include "Mod/Unnamed/VasePole.h"

#include "nlohmann/json.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <vector>

using namespace pvzu::unnamed;
namespace fs = std::filesystem;

namespace {

// Same values as pvz-unnamed-pack/data/rules-vase-pole.json (the last test compares the two).
const nlohmann::json kVaseRules = {
	{"vasePole.enabled", 1},
	{"vasePole.colDivisor", 80},
	{"vasePole.zombieRollRange", 33},
	{"vasePole.zombieRejectMask", 35223552}, // bits 11, 12, 13, 14, 16, 19, 20, 25
	{"vasePole.catchFlags", 129},
	{"vasePole.catchFlagsHypnotised", 1},
	{"vasePole.potState", 5},
	{"vasePole.potSetsSide", 1},
	{"vasePole.awardOutsideVasebreaker", 0},
};

// rules-vase-pole.json plus the Day-level vault keys DecideVault reads (rules-day-levels.json).
nlohmann::json VaultRules() {
	nlohmann::json r = kVaseRules;
	r["dayLevels.poleGrave.enabled"] = 1;
	r["dayLevels.poleGrave.alwaysVariant"] = 0x29;
	r["dayLevels.poleGrave.skipVariant"] = 0x32;
	r["dayLevels.poleGrave.vaseVariant"] = 0x103;
	r["dayLevels.poleBonk.seedTallnut"] = 23;
	return r;
}

struct RulesGuard {
	explicit RulesGuard(const nlohmann::json& r) { SetRules(r); }
	~RulesGuard() { SetRules(nlohmann::json::object()); }
};

struct Script {
	std::vector<int> rolls;
	std::vector<int> ranges;
	size_t next = 0;
	int operator()(int range) {
		ranges.push_back(range);
		return next < rolls.size() ? rolls[next++] : 0;
	}
};

} // namespace

TEST_CASE("VasePoleOnlyWithThePackKey") {
	{
		RulesGuard g(kVaseRules);
		CHECK(VasePoleOn());
		CHECK(IsVasePole(elite::VASE_POLE));
		CHECK_FALSE(IsVasePole(elite::GLOVE));
		CHECK_FALSE(IsVasePole(0));
	}
	CHECK_FALSE(VasePoleOn()); // missing key = vanilla
	CHECK_FALSE(IsVasePole(elite::VASE_POLE));
}

TEST_CASE("VasePoleVaultColumn") { // 0xc03701..0xc0372b: (uint16 mX / 80) low byte, -1 (normal) / +1 (hypnotised)
	RulesGuard g(kVaseRules);
	CHECK(VasePoleVaultColumn(0, false) == -1);
	CHECK(VasePoleVaultColumn(79, false) == -1);
	CHECK(VasePoleVaultColumn(80, false) == 0);
	CHECK(VasePoleVaultColumn(479, false) == 4);
	CHECK(VasePoleVaultColumn(480, false) == 5);
	CHECK(VasePoleVaultColumn(480, true) == 7);
	CHECK(VasePoleVaultColumn(65536 + 480, false) == 5); // low 16 bits only
	CHECK(VasePoleVaultColumn(-1, false) == (65535 / 80 & 0xff) - 1); // low byte of the quotient
}

TEST_CASE("VasePoleLandColumn") { // 0xc038de..0xc038fe: PixelToGridX + 1 (normal) / - 1 (hypnotised)
	RulesGuard g(kVaseRules);
	CHECK(VasePoleLandColumn(4, false) == 5);
	CHECK(VasePoleLandColumn(4, true) == 3);
	CHECK(VasePoleLandColumn(-1, false) == 0);
}

TEST_CASE("VasePoleVaultAndLandingMeetOnThePlantCell") {
	// Geometry of vanilla UpdateZombiePolevaulter: start mX0, jump distance d = mX0 - plant.mX - 80, the vase window at
	// anim time ~0.6 (mX = mX0 - 0.6 d), the landing mPosX = mX0 - d - 150 (PixelToGridX = (x - 40) / 80 on the lawn).
	RulesGuard g(kVaseRules);
	for (int col = 1; col <= 8; col++) {
		const int plantX = 40 + 80 * col;
		for (int mX0 = plantX + 90; mX0 <= plantX + 115; mX0++) {
			const int d = mX0 - plantX - 80;
			const int vaultX = mX0 - (6 * d) / 10;
			const int landX = mX0 - d - 150;
			const int landGrid = (landX - 40) / 80; // landX - 40 = 80 col - 70 > 0
			CHECK_MESSAGE(VasePoleVaultColumn(vaultX, false) == col, "col " << col << " mX0 " << mX0);
			CHECK_MESSAGE(VasePoleLandColumn(landGrid, false) == col, "col " << col << " mX0 " << mX0);
		}
	}
}

TEST_CASE("VasePoleHypnotisedSkipsItsOwnVase") { // 0xc0372d: jg 0xc037c7
	RulesGuard g(kVaseRules);
	CHECK(VasePoleMakesOwnVase(false));
	CHECK_FALSE(VasePoleMakesOwnVase(true));
}

TEST_CASE("VasePolePlantLeftCancels") { // 0xc03745..0xc03781
	CHECK(VasePolePlantLeft(false, false, 0));
	CHECK(VasePolePlantLeft(false, false, 1)); // being grabbed still counts
	CHECK_FALSE(VasePolePlantLeft(true, false, 0));
	CHECK_FALSE(VasePolePlantLeft(false, true, 0));
	CHECK_FALSE(VasePolePlantLeft(false, false, 2)); // rising with a bungee
}

TEST_CASE("VasePoleRandomZombie") { // 0xc0378e..0xc037be: Rand(33), re-rolled on 11-14, 16, 19, 20, 25
	RulesGuard g(kVaseRules);
	for (int t : {11, 12, 13, 14, 16, 19, 20, 25})
		CHECK_FALSE_MESSAGE(VasePoleZombieAccepted(t), t);
	for (int t : {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 15, 17, 18, 21, 22, 23, 24, 26, 31, 32})
		CHECK_MESSAGE(VasePoleZombieAccepted(t), t);
	Script s;
	s.rolls = {11, 25, 20, 3};
	CHECK(VasePoleRollZombie(std::ref(s)) == 3);
	CHECK(s.ranges == std::vector<int>{33, 33, 33, 33});
	Script all;
	all.rolls = {32};
	CHECK(VasePoleRollZombie(std::ref(all)) == 32);
	Script stuck; // a rand that only gives rejected types still ends (the exe loops forever; it never happens)
	stuck.rolls = std::vector<int>(5000, 12);
	CHECK(VasePoleRollZombie(std::ref(stuck)) == 0);
}

TEST_CASE("VasePoleCatchFlags") { // 0xc0381f..0xc0383c
	RulesGuard g(kVaseRules);
	CHECK(VasePoleCatchFlags(false) == 0x81u); // ground + only mind-controlled: the player's hypnotised zombies
	CHECK(VasePoleCatchFlags(true) == 0x01u);  // ground: the enemies
}

TEST_CASE("VasePoleCaughtZombieChangesSide") { // 0xc03846: flag = (zombie.mMindControlled != 1)
	CHECK(VasePoleCaughtPotHypnotised(false));
	CHECK_FALSE(VasePoleCaughtPotHypnotised(true));
}

TEST_CASE("VasePoleCatchTest") {
	const VaseRect pole{100, 0, 70, 115};
	CHECK(VasePoleRectsOverlap(pole, {150, 10, 40, 100}));
	CHECK_FALSE(VasePoleRectsOverlap(pole, {170, 10, 40, 100})); // touching edges do not overlap (0x955e40 jle)
	CHECK_FALSE(VasePoleRectsOverlap(pole, {20, 10, 80, 100}));
	CHECK(VasePoleRectsOverlap(pole, {21, 10, 80, 100}));
	CHECK_FALSE(VasePoleRectsOverlap(pole, {120, 115, 20, 20}));

	VaseCatchInput in;
	in.sameRow = true;
	in.overlap = true;
	in.effected = true;
	CHECK(VasePoleCatches(in));
	in.self = true;
	CHECK_FALSE(VasePoleCatches(in));
	in.self = false;
	in.bossClass = true;
	CHECK_FALSE(VasePoleCatches(in));
	in.bossClass = false;
	in.sameRow = false;
	CHECK_FALSE(VasePoleCatches(in));
	in.sameRow = true;
	in.overlap = false;
	CHECK_FALSE(VasePoleCatches(in));
	in.overlap = true;
	in.effected = false;
	CHECK_FALSE(VasePoleCatches(in));
}

TEST_CASE("VasePolePotLookAndSide") {
	{
		RulesGuard g(kVaseRules);
		CHECK(VasePolePotState() == 5);
		CHECK(VasePolePotSetsSide());
		CHECK_FALSE(VasePoleAwardAllowed(false)); // ruling: no award from a landing in a Day level
		CHECK(VasePoleAwardAllowed(true));        // Vasebreaker levels keep the vanilla tail
	}
	CHECK_FALSE(VasePolePotSetsSide());
	CHECK(VasePoleAwardAllowed(false)); // vanilla: the tail always runs
}

TEST_CASE("VasePoleVaultDecision") { // .text 0x525fa0 -> 0x651a97 -> 0xc036e0
	RulesGuard g(VaultRules());
	VaultInput in;
	in.variant = 0x103;
	in.hasPlant = true;
	in.plantSeed = 0; // Peashooter
	VaultDecision r = DecideVault(in);
	CHECK(r.vase);
	CHECK_FALSE(r.grave);
	CHECK_FALSE(r.bonk);

	in.hypnotised = true; // hypnosis does not stop the vase path (it only flips the column)
	CHECK(DecideVault(in).vase);
	in.hypnotised = false;

	in.graveMade = true; // +0x12c already set
	CHECK_FALSE(DecideVault(in).vase);
	in.graveMade = false;

	in.hasPlant = false; // no target in the window: still the vase path
	CHECK(DecideVault(in).vase);

	in.hasPlant = true;
	in.plantSeed = 23; // Tall-nut: the bonk runs first
	r = DecideVault(in);
	CHECK(r.bonk);
	CHECK_FALSE(r.vase);

	in.plantSeed = 0;
	in.variant = 0; // an ordinary pole: grave, no vase
	r = DecideVault(in);
	CHECK(r.grave);
	CHECK_FALSE(r.vase);
}

TEST_CASE("VasePoleVaultDecisionNeedsThePackKey") {
	nlohmann::json r = VaultRules();
	r.erase("vasePole.enabled");
	RulesGuard g(r);
	VaultInput in;
	in.variant = 0x103;
	CHECK_FALSE(DecideVault(in).vase);
	CHECK_FALSE(DecideVault(in).grave); // still no grave for the vase form
}

TEST_CASE("GridStateRoundTrip") {
	GridState s;
	s.potHypnotised = 1;
	auto blob = Encode(s);
	GridState q;
	REQUIRE(Decode(blob.data(), blob.size(), q));
	CHECK(q == s);
	GridState z;
	z.potHypnotised = 7;
	CHECK_FALSE(Decode(blob.data(), 2, z));
	CHECK(z == GridState{});
}

TEST_CASE("VasePolePackRulesMatchTheseValues") {
	for (const auto& k : VasePoleRuleKeys()) CHECK_MESSAGE(kVaseRules.contains(k), k);
	CHECK(VasePoleRuleKeys().size() == kVaseRules.size());
	const char* home = std::getenv("HOME");
	fs::path file = std::getenv("PVZU_PACK_DIR") ? fs::path(std::getenv("PVZU_PACK_DIR"))
	                                              : fs::path(home ? home : "") / "pvz-unnamed-pack";
	file /= "data/rules-vase-pole.json";
	if (!fs::exists(file)) { MESSAGE("pack not here, skipped: " << file.string()); return; }
	std::ifstream in(file);
	nlohmann::json pack = nlohmann::json::parse(in, nullptr, false);
	REQUIRE(pack.is_object());
	CHECK(pack == kVaseRules);
}
