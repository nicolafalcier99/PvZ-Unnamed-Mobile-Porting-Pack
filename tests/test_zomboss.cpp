// PVZU: Plan 6 Task 4, Dr. Zomboss default settings (5-10). Pure rules; evidence in the research notes (w5-roof) section 4.1
// (B1-B10) and pvz-unnamed-pack/SOURCES-zomboss.md. Legendary Zomboss (4.2) is not ported.
#include "doctest/doctest.h"
#include "Mod/Unnamed/Rules.h"
#include "Mod/Unnamed/State.h"
#include "Mod/Unnamed/Zomboss.h"

#include "nlohmann/json.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <random>
#include <set>

using namespace pvzu::unnamed;
namespace fs = std::filesystem;

namespace {

// The Unnamed values (same as pvz-unnamed-pack/data/rules-zomboss.json; the last test compares the two).
const nlohmann::json kBossRules = {
	{"boss.hpAdventure", 50000},
	{"boss.hpOther", 80000},
	{"boss.lateSpawnRoll", 33},
	{"boss.lateSpawnExclude", {0, 1, 9, 10, 11, 13, 14, 16, 17, 20, 24, 25}},
	{"boss.lateSpawnExcludeRow0", {23, 32}},
	{"boss.lateSpawnExcludeYetiKilled", {19}},
	{"boss.stompNothingHeal", 1800},
	{"boss.crushHypnotisedDamage", 1800},
	{"boss.crushOpensPots", 1},
	{"boss.umbrellaRvDamage", 100},
	{"boss.umbrellaRvRowsBefore", 2},
	{"boss.umbrellaRvRowsAfter", 3},
	{"boss.umbrellaRvColsBefore", 2},
	{"boss.umbrellaRvColsAfter", 4},
	{"boss.spitPicksHarderBall", 1},
	{"boss.copiesHypnosis", 1},
	{"boss.bungeeJackForced", 1},
	{"boss.bungeeEliteRoll", 12},
	{"boss.bungeeEliteBelow", 3},
	{"boss.jackBungeeGrabCs", 0},
	{"boss.jackBungeeJackDx", -30},
	{"boss.everyBossBalls", 1},
	{"boss.oneHpRefusesFreeze", 1},
};

struct BossRulesOn {
	BossRulesOn() { SetRules(kBossRules); }
	~BossRulesOn() { SetRules(nlohmann::json::object()); }
};

constexpr int kSeedIceshroom = 14, kSeedJalapeno = 20, kSeedPeashooter = 0, kSeedImitater = 48, kSeedNone = -1;

} // namespace

TEST_CASE("ZombossVanillaWithoutRules") {
	SetRules(nlohmann::json::object());
	CHECK(BossStartHp(true, 40000) == 40000);
	CHECK(BossStartHp(false, 60000) == 60000);
	CHECK(BossLateSpawnRollRange() == 0); // vanilla list pick
	CHECK_FALSE(BossEmptyStompHeals());
	CHECK(BossHpAfterEmptyStomp(30000) == 30000);
	CHECK_FALSE(BossCrushesHypnotised());
	CHECK_FALSE(BossCrushOpensPots());
	CHECK_FALSE(UmbrellaBlocksRv(2, 1, 2, 1));
	CHECK(PickBossBall({{kSeedJalapeno, kSeedNone}}) == BossBall::VANILLA_ROLL);
	CHECK_FALSE(BossCopiesHypnosis());
	CHECK(BossBungeeEliteRollRange() == 0);
	CHECK_FALSE(EveryBossBallsBreak());
	CHECK_FALSE(ZombossRefusesFreeze(1));
}

TEST_CASE("ZombossHp") {
	// B1, hook 505 / .text 0x523618: `neg/sbb/and 0x7530/add 0xc350` = Adventure 50000, any other mode 80000
	BossRulesOn on;
	CHECK(BossStartHp(true, 40000) == 50000);
	CHECK(BossStartHp(false, 60000) == 80000);
}

TEST_CASE("ZombossLateSpawnAcceptSet") {
	// B3, hook 735 0x9579b0: Rand(33) re-rolled while in the exclusion list; row 0 also drops 23, 32; Yeti once killed
	BossRulesOn on;
	CHECK(BossLateSpawnRollRange() == 33);
	const std::set<int> excluded = {0, 1, 9, 10, 11, 13, 14, 16, 17, 20, 24, 25};
	for (int r = 0; r < 33; ++r) {
		CHECK_MESSAGE(BossLateSpawnAccepted(r, 2, false) == !excluded.contains(r), r);
		CHECK_MESSAGE(BossLateSpawnAccepted(r, 0, false) == (!excluded.contains(r) && r != 23 && r != 32), r);
		CHECK_MESSAGE(BossLateSpawnAccepted(r, 3, true) == (!excluded.contains(r) && r != 19), r);
	}
	CHECK_FALSE(BossLateSpawnAccepted(-1, 2, false));
	CHECK_FALSE(BossLateSpawnAccepted(33, 2, false));
}

TEST_CASE("ZombossLateSpawn2000Rolls") {
	BossRulesOn on;
	std::mt19937 rng(20261123);
	auto rand = [&](int n) { return static_cast<int>(rng() % static_cast<unsigned>(n)); };
	const std::set<int> want = {2, 3, 4, 5, 6, 7, 8, 12, 15, 18, 19, 21, 22, 23, 26, 27, 28, 29, 30, 31, 32};
	REQUIRE(want.size() == 21);
	struct Case { int row; bool yeti; std::set<int> expect; };
	std::set<int> row0 = want, yeti = want;
	row0.erase(23); row0.erase(32);
	yeti.erase(19);
	for (const Case& c : {Case{2, false, want}, Case{0, false, row0}, Case{4, true, yeti}}) {
		std::map<int, int> counts;
		for (int i = 0; i < 2000; ++i)
			++counts[static_cast<int>(PickBossLateSpawn(c.row, c.yeti, rand))];
		std::set<int> seen;
		for (auto [t, n] : counts) seen.insert(t);
		CHECK(seen == c.expect);
		const double expected = 2000.0 / static_cast<double>(c.expect.size());
		for (auto [t, n] : counts) // uniform: every type within 45% of the mean (2000 rolls)
			CHECK_MESSAGE((n > expected * 0.55 && n < expected * 1.45), "type " << t << " count " << n);
	}
}

TEST_CASE("ZombossEmptyStompHeals") {
	// B4, hook 737 0xc02800: no stompable row -> body HP += 1800 (no cap in the code) instead of returning
	BossRulesOn on;
	CHECK(BossEmptyStompHeals());
	CHECK(BossHpAfterEmptyStomp(30000) == 31800);
	CHECK(BossHpAfterEmptyStomp(49000) == 50800); // above the 50000 max: the exe adds and never clamps
}

TEST_CASE("ZombossCrushAreas") {
	// B5, hooks 738/739 (stomp) and 731 (RV): hypnotised zombies in the area take TakeDamage(1800, 1); the zombie's
	// rect columns are round(x / 80) - 1 and round((x + w) / 80) - 1 (FPU round to nearest even, 0x957680)
	BossRulesOn on;
	CHECK(BossCrushesHypnotised());
	CHECK(BossCrushDamage() == 1800);
	CHECK(RectColumn(40) == -1);  // 0.5 -> 0 (even)
	CHECK(RectColumn(120) == 1);  // 1.5 -> 2
	CHECK(RectColumn(200) == 1);  // 2.5 -> 2
	CHECK(RectColumn(479) == 5);
	// stomp: rows [target, target + 1], left column >= 5 and right column <= 9
	CHECK(StompHitsZombie(1, 440, 40, 1));    // cols 5..5
	CHECK(StompHitsZombie(2, 440, 40, 1));
	CHECK_FALSE(StompHitsZombie(3, 440, 40, 1));
	CHECK_FALSE(StompHitsZombie(0, 440, 40, 1));
	CHECK_FALSE(StompHitsZombie(1, 400, 40, 1)); // left col 4
	CHECK(StompHitsZombie(1, 700, 100, 1));   // cols 8..9
	CHECK_FALSE(StompHitsZombie(1, 760, 100, 1)); // right col 10
	// RV: right column in [targetCol, targetCol + 2], row - targetRow in [0, 1]
	CHECK(RvHitsZombie(2, 60, 40, 2, 0));     // right col round(100/80)-1 = 0
	CHECK(RvHitsZombie(3, 180, 40, 2, 0));    // right col 2
	CHECK_FALSE(RvHitsZombie(3, 260, 40, 2, 0)); // right col 3
	CHECK_FALSE(RvHitsZombie(4, 60, 40, 2, 0));
	CHECK_FALSE(RvHitsZombie(1, 60, 40, 2, 0));
	// a normal boss hits only hypnotised zombies (EffectedByDamage 0x8d), a hypnotised boss the normal ones (0xd)
	CHECK(BossCrushDamageFlags(false) == 0x8du);
	CHECK(BossCrushDamageFlags(true) == 0xdu);
	// Vasebreaker pots in the same squares open
	CHECK(BossCrushOpensPots());
	CHECK(StompOpensPot(5, 1, 1));
	CHECK(StompOpensPot(9, 2, 1));
	CHECK_FALSE(StompOpensPot(4, 1, 1));
	CHECK_FALSE(StompOpensPot(10, 1, 1));
	CHECK_FALSE(StompOpensPot(6, 3, 1));
	CHECK(RvOpensPot(1, 2, 2, 1));
	CHECK(RvOpensPot(3, 3, 2, 1));
	CHECK_FALSE(RvOpensPot(0, 2, 2, 1));
	CHECK_FALSE(RvOpensPot(4, 2, 2, 1));
	CHECK_FALSE(RvOpensPot(2, 4, 2, 1));
}

TEST_CASE("ZombossUmbrellaBlocksRv") {
	// B6, hook 731 0xc02b20: an Umbrella Leaf with row in [tr-2, tr+3] and column in [tc-2, tc+4] takes 100 and the
	// RV crushes no plant
	BossRulesOn on;
	CHECK(UmbrellaRvDamage() == 100);
	CHECK(UmbrellaBlocksRv(2, 1, 2, 1));
	CHECK(UmbrellaBlocksRv(0, 0, 2, 2));   // rows 0..5, cols 0..6 for target (2, 2)
	CHECK(UmbrellaBlocksRv(5, 6, 2, 2));
	CHECK_FALSE(UmbrellaBlocksRv(0, 0, 3, 2)); // row 0 < 3 - 2
	CHECK_FALSE(UmbrellaBlocksRv(2, 7, 2, 2)); // col 7 > 2 + 4
	CHECK(UmbrellaBlocksRv(4, 0, 1, 0));
	CHECK_FALSE(UmbrellaBlocksRv(5, 0, 1, 0)); // row 5 > 1 + 3
}

TEST_CASE("ZombossPicksTheHarderBall") {
	// B7, hook 745 0x957a50: count Jalapeno vs Ice-shroom packets (an Imitater counts as its copy):
	// fewer Jalapenos -> ice ball, fewer Ice-shrooms -> fireball, equal -> the vanilla Rand(2)
	BossRulesOn on;
	CHECK(PickBossBall({}) == BossBall::VANILLA_ROLL);
	CHECK(PickBossBall({{kSeedPeashooter, kSeedNone}}) == BossBall::VANILLA_ROLL);
	CHECK(PickBossBall({{kSeedIceshroom, kSeedNone}}) == BossBall::ICE);
	CHECK(PickBossBall({{kSeedJalapeno, kSeedNone}}) == BossBall::FIRE);
	CHECK(PickBossBall({{kSeedJalapeno, kSeedNone}, {kSeedIceshroom, kSeedNone}}) == BossBall::VANILLA_ROLL);
	CHECK(PickBossBall({{kSeedJalapeno, kSeedNone}, {kSeedImitater, kSeedIceshroom}, {kSeedIceshroom, kSeedNone}}) ==
		BossBall::ICE);
	CHECK(PickBossBall({{kSeedImitater, kSeedJalapeno}}) == BossBall::FIRE);
	CHECK(PickBossBall({{kSeedImitater, kSeedNone}}) == BossBall::VANILLA_ROLL); // an Imitater with no copy counts as nothing
}

TEST_CASE("ZombossBallPick2000Banks") {
	// random seed banks of 0..10 packets: the pick always follows the counts, never the roll
	BossRulesOn on;
	std::mt19937 rng(5010);
	const int kinds[] = {kSeedIceshroom, kSeedJalapeno, kSeedPeashooter, kSeedImitater};
	int fire = 0, ice = 0, roll = 0;
	for (int i = 0; i < 2000; ++i) {
		std::vector<BossPacket> bank;
		int jal = 0, iceN = 0;
		const int n = static_cast<int>(rng() % 11);
		for (int k = 0; k < n; ++k) {
			int type = kinds[rng() % 4];
			int imit = type == kSeedImitater ? kinds[rng() % 3] : kSeedNone;
			const int eff = type == kSeedImitater ? imit : type;
			jal += eff == kSeedJalapeno;
			iceN += eff == kSeedIceshroom;
			bank.push_back({type, imit});
		}
		const BossBall b = PickBossBall(bank);
		const BossBall want = jal < iceN ? BossBall::ICE : jal > iceN ? BossBall::FIRE : BossBall::VANILLA_ROLL;
		REQUIRE(b == want);
		fire += b == BossBall::FIRE;
		ice += b == BossBall::ICE;
		roll += b == BossBall::VANILLA_ROLL;
	}
	CHECK(fire > 300);
	CHECK(ice > 300);
	CHECK(roll > 100);
}

TEST_CASE("ZombossBungeesAreOneInFourJackHeads") {
	// B8, hooks 488 + 741: the boss's bungees roll Rand(12) <= 2 and that pass is forced to 0x17 (Jack head):
	// P = 3/12 = 1/4. The Jack-head bungee grabs at once (counter 0) and drops a Jack at x = plant x - 30.
	BossRulesOn on;
	CHECK(BossBungeeEliteRollRange() == 12);
	CHECK(BossBungeeVariant(0) == zomboss::JACK_HEAD_BUNGEE);
	CHECK(BossBungeeVariant(1) == zomboss::JACK_HEAD_BUNGEE);
	CHECK(BossBungeeVariant(2) == zomboss::JACK_HEAD_BUNGEE);
	CHECK(BossBungeeVariant(3) == 0);
	CHECK(BossBungeeVariant(11) == 0);
	CHECK(JackBungeeGrabCs(300) == 0);
	CHECK(JackBungeeJackDx() == -30);
	std::mt19937 rng(741);
	int jacks = 0;
	for (int i = 0; i < 2000; ++i)
		jacks += BossBungeeVariant(static_cast<int>(rng() % 12)) == zomboss::JACK_HEAD_BUNGEE;
	CHECK(jacks > 420); // 500 expected
	CHECK(jacks < 580);
	CHECK(BossCopiesHypnosis());
	CHECK(BossBungeeJackForced());
}

TEST_CASE("ZombossFreezeRules") {
	// B9, hooks 340/341: every non-hypnotised boss loses its ball (vanilla: only the first boss); hook 713: a boss at
	// 1 HP cannot be frozen. Freeze damage 200 is the cross-world Ice-trap rule (hook 718, nightPlants.iceTrap.damage).
	BossRulesOn on;
	CHECK(EveryBossBallsBreak());
	CHECK(BossBallBreaks(false));
	CHECK_FALSE(BossBallBreaks(true));
	CHECK(ZombossRefusesFreeze(1));
	CHECK_FALSE(ZombossRefusesFreeze(2));
	CHECK_FALSE(ZombossRefusesFreeze(0));
}

TEST_CASE("ZombossStateSurvivesSaveLoad") {
	// No new per-object field: the healed HP is the vanilla mBodyHealth (saved), the Jack-head bungee is
	// ZombieState.variant 0x17 and the boss id is the bungee's vanilla mFollowerZombieID[0]. The variant round-trips.
	ZombieState s;
	s.variant = zomboss::JACK_HEAD_BUNGEE;
	auto b = Encode(s);
	ZombieState back;
	REQUIRE(Decode(b.data(), b.size(), back));
	CHECK(back == s);
	CHECK(BoardState{}.IsDefault()); // the boss adds nothing to the board chunk, so vanilla saves stay identical
}

TEST_CASE("ZombossPackRulesMatchTheseValues") {
	for (const auto& k : ZombossRuleKeys()) CHECK_MESSAGE(kBossRules.contains(k), k);
	CHECK(ZombossRuleKeys().size() == kBossRules.size());
	const char* home = std::getenv("HOME");
	fs::path dir = std::getenv("PVZU_PACK_DIR") ? fs::path(std::getenv("PVZU_PACK_DIR"))
	                                             : fs::path(home ? home : "") / "pvz-unnamed-pack";
	fs::path file = dir / "data/rules-zomboss.json";
	if (!fs::exists(file)) { MESSAGE("pack not here, skipped: " << file.string()); return; }
	std::ifstream in(file);
	nlohmann::json pack = nlohmann::json::parse(in, nullptr, false);
	REQUIRE(pack.is_object());
	CHECK(pack == kBossRules);
	// B9 freeze damage 200 lives in the Night plants file (hook 718, shared by every world)
	std::ifstream night(dir / "data/rules-night-plants.json");
	nlohmann::json np = nlohmann::json::parse(night, nullptr, false);
	REQUIRE(np.is_object());
	CHECK(np.value("nightPlants.iceTrap.damage", 0) == 200);
	// mod.json switches the feature on
	std::ifstream mod(dir / "mod.json");
	nlohmann::json m = nlohmann::json::parse(mod, nullptr, false);
	REQUIRE(m.is_object());
	CHECK(m["features"].value("zomboss", false));
}
