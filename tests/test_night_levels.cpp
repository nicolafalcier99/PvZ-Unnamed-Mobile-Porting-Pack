// PVZU: Unnamed Night (Adventure 2-1..2-10) level rules: start graves, allow-list extras, the gold Football's
// grave spawn and the Dancer's "Death Carnival" (the research notes (w2-night) sections 2.3, 2.4, 3, 4, 5).
// Rules come from a fixture with the same numbers as pvz-unnamed-pack/data/rules-night-levels.json.
#include "doctest/doctest.h"
#include "Mod/Unnamed/Levels.h"
#include "Mod/Unnamed/NightLevels.h"
#include "Mod/Unnamed/Rules.h"

#include <functional>
#include <map>
#include <vector>

using namespace pvzu::unnamed;

namespace {
constexpr int kNormal = 0, kFlag = 1, kCone = 2, kPole = 3, kBucket = 4, kNewspaper = 5, kDoor = 6, kFootball = 7,
			  kDancer = 8, kBackup = 9, kJack = 15, kGargantuar = 23, kImp = 24;
constexpr int kGold = 0x102, kRandomFollowers = 0x104, kTrophy = 9, kMoonwalker = 10;

nlohmann::json NightLevelRules() {
	nlohmann::json r = {
		{"nightLevels.allowExtra.count", 1},
		{"nightLevels.allowExtra.0.level", 11},
		{"nightLevels.allowExtra.0.type", 2},
		{"nightLevels.goldFootball.type", 7},
		{"nightLevels.goldFootball.variant", 0x102},
		{"nightLevels.goldFootball.graveType", 7},
		{"nightLevels.carnival.summonerType", 8},
		{"nightLevels.carnival.everyBackups", 12},
		{"nightLevels.carnival.skipVariant", 0x104},
		{"nightLevels.carnival.passesHypnosis", 1},
	};
	// exe Board::InitLevel 0x40A945.. (AddGraveStones 0x408FC0): level -> {first column, counts from it to column 8}
	const std::map<int, std::vector<int>> kExe = {
		{11, {6, 1, 1, 2}}, {12, {6, 1, 1, 2}}, {13, {6, 1, 1, 2}},
		{14, {5, 1, 1, 2, 3}}, {16, {5, 1, 1, 2, 3}}, {18, {5, 1, 1, 2, 3}},
		{17, {4, 1, 2, 2, 3, 3}}, {19, {4, 1, 2, 2, 3, 3}},
		{20, {3, 1, 2, 2, 2, 3, 3}},
	};
	for (const auto& [level, row] : kExe) {
		const std::string k = "nightLevels.startGraves." + std::to_string(level);
		r[k + ".firstCol"] = row[0];
		for (size_t i = 1; i < row.size(); i++)
			r[k + ".c" + std::to_string(row[0] + static_cast<int>(i) - 1)] = row[i];
	}
	return r;
}

// Plan 2's rules-day-levels.json grave re-roll keys (hook 88 is one site for every level).
nlohmann::json WithDayReroll(nlohmann::json r) {
	r["dayLevels.graveRise.rerollRange"] = 25;
	r["dayLevels.graveRise.rerollAcceptMask"] = 25199359;
	r["dayLevels.graveRise.rerollAgainMask"] = 256;
	r["dayLevels.graveRise.rerollMisses"] = 2;
	return r;
}

struct RulesGuard {
	explicit RulesGuard(const nlohmann::json& r) { SetRules(r); }
	~RulesGuard() { SetRules(nlohmann::json::object()); }
};

GraveLayout Layout(std::initializer_list<std::pair<int, int>> cells) {
	GraveLayout a{};
	for (auto [col, count] : cells)
		a[col] = count;
	return a;
}
}

TEST_CASE("NightStartGravesPerLevel") {
	RulesGuard g(NightLevelRules());
	GraveLayout a{};
	// 2-1..2-3: 6:1 7:1 8:2
	for (int level : {11, 12, 13}) {
		REQUIRE(NightStartGraves(level, a));
		CHECK(a == Layout({{6, 1}, {7, 1}, {8, 2}}));
		CHECK(GraveTotal(a) == 4);
	}
	// 2-4, 2-6 and 2-8 (exe 0x40A978 `cmp eax,0x12`; GOTY puts 2-8 with the 11 group): 5:1 6:1 7:2 8:3
	for (int level : {14, 16, 18}) {
		REQUIRE(NightStartGraves(level, a));
		CHECK(a == Layout({{5, 1}, {6, 1}, {7, 2}, {8, 3}}));
		CHECK(GraveTotal(a) == 7);
	}
	// 2-7, 2-9: 4:1 5:2 6:2 7:3 8:3
	for (int level : {17, 19}) {
		REQUIRE(NightStartGraves(level, a));
		CHECK(a == Layout({{4, 1}, {5, 2}, {6, 2}, {7, 3}, {8, 3}}));
		CHECK(GraveTotal(a) == 11);
	}
	// 2-10: 3:1 4:2 5:2 6:2 7:3 8:3
	REQUIRE(NightStartGraves(20, a));
	CHECK(a == Layout({{3, 1}, {4, 2}, {5, 2}, {6, 2}, {7, 3}, {8, 3}}));
	CHECK(GraveTotal(a) == 13);
	// 2-5 is Whack-a-Zombie (vanilla WhackAZombiePlaceGraves(9)): no table row, the engine keeps vanilla
	CHECK_FALSE(NightStartGraves(15, a));
	CHECK_FALSE(NightStartGraves(21, a));
}

TEST_CASE("NightStartGravesMissingRuleIsVanilla") {
	RulesGuard g(nlohmann::json::object());
	GraveLayout a{};
	CHECK_FALSE(NightStartGraves(18, a));
	CHECK(GraveTotal(a) == 0);
}

TEST_CASE("NightAllowExtraOnlyConeIn2_1") {
	RulesGuard g(NightLevelRules());
	CHECK(NightAllowExtra(kCone, 11));
	CHECK_FALSE(NightAllowExtra(kCone, 12)); // vanilla already allows it there; no extra needed
	CHECK_FALSE(NightAllowExtra(kCone, 10));
	for (int type : {kNormal, kFlag, kPole, kBucket, kNewspaper, kDoor, kFootball, kDancer, kBackup, kJack, kGargantuar, kImp})
		CHECK_FALSE(NightAllowExtra(type, 11));
	SetRules(nlohmann::json::object());
	CHECK_FALSE(NightAllowExtra(kCone, 11));
}

TEST_CASE("GoldFootballDeathRaisesFootballsFromGraves") {
	RulesGuard g(NightLevelRules());
	CHECK(GoldFootballRaisesGraves(kFootball, kGold, 3));
	CHECK(GoldFootballRaisesGraves(kFootball, kGold, 0));
	CHECK(GoldFootballGraveType() == kFootball);
	CHECK_FALSE(GoldFootballRaisesGraves(kFootball, 0, 3));      // plain Football
	CHECK_FALSE(GoldFootballRaisesGraves(kFootball, 0x1A, 3));   // a code that is not 0x102
	CHECK_FALSE(GoldFootballRaisesGraves(kNormal, kGold, 3));    // 0x102 only means "gold" on a Football
	CHECK_FALSE(GoldFootballRaisesGraves(kFootball, kGold, -2)); // cut-scene preview zombie (ZOMBIE_WAVE_CUTSCENE)
	CHECK_FALSE(GoldFootballRaisesGraves(kFootball, kGold, -1)); // debug / UI zombies
	SetRules(nlohmann::json::object());
	CHECK_FALSE(GoldFootballRaisesGraves(kFootball, kGold, 3));
}

TEST_CASE("CarnivalEvery12BackupsRaisesGraves") {
	RulesGuard g(NightLevelRules());
	int counter = 0;
	int raises = 0;
	for (int i = 1; i <= 36; i++) {
		const CarnivalStep s = CarnivalOnBackupSummoned(kDancer, 0, counter);
		counter = s.counter;
		raises += s.raise ? 1 : 0;
		CHECK(s.raise == (i % 12 == 0)); // 0x7966C0: cmp [ebx+0x12c],0xc; jl
		CHECK(counter == i % 12);        // reset to 0 at 12
	}
	CHECK(raises == 3);
	// boundary: 10 -> 11 no raise; 11 -> 12 raise and reset
	CHECK_FALSE(CarnivalOnBackupSummoned(kDancer, 0, 10).raise);
	CHECK(CarnivalOnBackupSummoned(kDancer, 0, 10).counter == 11);
	CHECK(CarnivalOnBackupSummoned(kDancer, 0, 11).raise);
	CHECK(CarnivalOnBackupSummoned(kDancer, 0, 11).counter == 0);
	// trophy (endless summons) and moonwalker forms count as well; only 0x104 (random followers) does not
	CHECK(CarnivalOnBackupSummoned(kDancer, kTrophy, 11).raise);
	CHECK(CarnivalOnBackupSummoned(kDancer, kMoonwalker, 11).raise);
	CHECK_FALSE(CarnivalOnBackupSummoned(kDancer, kRandomFollowers, 11).raise);
	CHECK(CarnivalOnBackupSummoned(kDancer, kRandomFollowers, 11).counter == 11); // untouched (it stores a type there)
	// only a Dancing Zombie counts (the Cone "dancer head" elite also calls SummonBackupDancer, on its death)
	CHECK_FALSE(CarnivalOnBackupSummoned(kCone, 3, 11).raise);
	CHECK(CarnivalPassesHypnosis());
}

TEST_CASE("CarnivalMissingRuleIsVanilla") {
	RulesGuard g(nlohmann::json::object());
	const CarnivalStep s = CarnivalOnBackupSummoned(kDancer, 0, 11);
	CHECK_FALSE(s.raise);
	CHECK(s.counter == 11);
	CHECK_FALSE(CarnivalPassesHypnosis());
}

// Final-wave grave re-roll at night: the same hook 88 table as Day (Plan 2). Every roll sequence is enumerated, so
// these are exact probabilities: 12 types x 1/16 and the vanilla pick 1/4; the level's allow list is not consulted.
TEST_CASE("NightGraveRerollTableIsOneSixteenthEach") {
	RulesGuard g(WithDayReroll(NightLevelRules()));
	constexpr int kVanillaPick = 100; // marker, not a zombie type
	std::map<int, double> p;
	// first roll r1 (1/25 each); 8 rolls again with no effect (renormalise over 24); one miss allowed
	std::function<void(int, double)> walk = [&](int misses, double weight) {
		for (int r = 0; r < 25; r++) {
			if (r == 8)
				continue;
			const double w = weight / 24.0;
			int scripted[] = {r};
			int k = 0;
			// GraveRiseType's answer for this single roll: accepted, or a miss
			const int one = GraveRiseType(kVanillaPick, [&](int range) {
				CHECK(range == 25);
				return k++ == 0 ? scripted[0] : 13; // a second roll (if asked) is a miss
			});
			if (one != kVanillaPick) {
				p[one] += w;
			} else if (misses == 0) {
				walk(1, w);
			} else {
				p[kVanillaPick] += w;
			}
		}
	};
	walk(0, 1.0);
	const int kTypes[] = {kNormal, kFlag, kCone, kPole, kBucket, kNewspaper, kDoor, kFootball, kBackup, kJack, kGargantuar, kImp};
	for (int t : kTypes)
		CHECK(p[t] == doctest::Approx(1.0 / 16.0));
	CHECK(p[kVanillaPick] == doctest::Approx(0.25));
	CHECK(p.size() == 13); // nothing else can rise: no pool/air/roof types (Review focus 4; art checked on the Mac)
	CHECK(p.count(kDancer) == 0);
}
