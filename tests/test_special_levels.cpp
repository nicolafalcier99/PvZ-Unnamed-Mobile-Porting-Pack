// PVZU: Plan 7 Task 3a, the Unnamed special levels 6-5 (55, Mushroom-garden shooter), 6-10 (60, intro boss timer),
// 7-5 (65, star snake) and the hidden 71 / 72 / 73 (pure rules; evidence in the research notes (w6-w7-and-new-content)
// section 2.3 and pvz-unnamed-pack/SOURCES-special-levels.md, which also lists the re-reads done for this task).
#include "doctest/doctest.h"
#include "Mod/Unnamed/LaterLevels.h"
#include "Mod/Unnamed/Rules.h"
#include "Mod/Unnamed/SpecialLevels.h"
#include "Mod/Unnamed/State.h"
#include "nlohmann/json.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>

using namespace pvzu::unnamed;
namespace fs = std::filesystem;

namespace {
// Engine seed / zombie / key values used below (ConstEnums.h, KeyCodes.h).
constexpr int kCabbage = 32, kMelon = 39, kWinterMelon = 44, kBlover = 27, kPeashooter = 0, kStarfruit = 29;
constexpr int kNormal = 0, kNewspaper = 5, kFootball = 7, kBoss = 25, kPeaHead = 26, kJalapenoHead = 28;
constexpr int kKeyUp = 0x26, kKeyDown = 0x28, kKeyLeft = 0x25;

// The Unnamed values (same as pvz-unnamed-pack/data/rules-special-levels.json; the last test compares the two).
const nlohmann::json kRules = {
	{"specialLevels.shooter.seedSlots", 4},
	{"specialLevels.shooter.plantSeed", 32},
	{"specialLevels.shooter.plantRow", 0},
	{"specialLevels.shooter.plantCol", 0},
	{"specialLevels.shooter.launchRate", 50},
	{"specialLevels.shooter.startSun", 0},
	{"specialLevels.shooter.messageTime", 250},
	{"specialLevels.shooter.firstWaveCountdown", 250},
	{"specialLevels.shooter.killSunMultiplier", 5},
	{"specialLevels.shooter.upgrade1.from", 32},
	{"specialLevels.shooter.upgrade1.to", 39},
	{"specialLevels.shooter.upgrade1.sun", 500},
	{"specialLevels.shooter.upgrade2.from", 39},
	{"specialLevels.shooter.upgrade2.to", 44},
	{"specialLevels.shooter.upgrade2.sun", 2000},
	{"specialLevels.shooter.halfHpBelowType", 26},
	{"specialLevels.touch.swipeMin", 40},
	{"specialLevels.starSnake.seedSlots", 3},
	{"specialLevels.starSnake.speed", 3.0},
	{"specialLevels.starSnake.maxTurns", 2560},
	{"specialLevels.starSnake.fieldMinX", 0},
	{"specialLevels.starSnake.fieldMaxX", 899},
	{"specialLevels.starSnake.fieldMinY", 40},
	{"specialLevels.starSnake.fieldMaxY", 599},
	{"specialLevels.starSnake.startStars", {{120, 280, 12}, {102, 280, 6}, {84, 280, 0}}},
	{"specialLevels.starSnake.startRow", 2},
	{"specialLevels.starSnake.startRenderOrder", 322688},
	{"specialLevels.starSnake.bankSeeds", {29, 1, 3}},
	{"specialLevels.starSnake.growSeed", 29},
	{"specialLevels.starSnake.growGapTicks", 6},
	{"specialLevels.starSnake.growRenderOrder", 343000},
	{"specialLevels.starSnake.growShadowYOffset", 82},
	{"specialLevels.starSnake.starsPierce", 1},
	{"specialLevels.hidden1.wavePointsMultiplier", 112},
	{"specialLevels.hidden1.autoPlantEvery", 1000},
	{"specialLevels.hidden1.autoPlantSeed", 27},
	{"specialLevels.hidden1.autoPlantCol", 7},
	{"specialLevels.hidden1.autoPlantRow", -1},
	{"specialLevels.hidden1.helmMultiplier", 2},
	{"specialLevels.hidden1.bannedSeeds", {27}},
	{"specialLevels.hidden2.wavePointsMultiplier", 80},
	{"specialLevels.hidden2.drainEvery", 6},
	{"specialLevels.hidden2.drainPhase", 1},
	{"specialLevels.hidden2.drainDamage", 1},
	{"specialLevels.hidden2.drainSkipStates", {3, 4, 5, 6, 7}},
	{"specialLevels.hidden2.drainSkipSeeds", {2, 14, 15, 20, 27, 82, 107}},
	{"specialLevels.hidden3.level", 73},
	{"specialLevels.hidden3.wavePointsMultiplier", 60},
	{"specialLevels.hidden3.bannedSeeds", {2, 4, 11, 12, 14, 15, 17, 20, 27, 35, 38}},
	{"specialLevels.hidden3.columnMaxPlants", 29},
	{"specialLevels.hidden3.newspaperType", 5},
	{"specialLevels.hidden3.newspaperFirstWave", 5},
};

struct RulesGuard {
	RulesGuard() { SetRules(kRules); }
	~RulesGuard() { SetRules(nlohmann::json::object()); }
};
struct VanillaGuard {
	VanillaGuard() { SetRules(nlohmann::json::object()); }
};

fs::path PackFile(const char* theName) {
	const char* home = std::getenv("HOME");
	fs::path dir = std::getenv("PVZU_PACK_DIR") ? fs::path(std::getenv("PVZU_PACK_DIR"))
	                                             : fs::path(home ? home : "") / "pvz-unnamed-pack";
	return dir / theName;
}
} // namespace

// ---- 6-5 (55): hook 142 (0x799717) seed slots, hook 462 (0x79842f) start, hook 160 (0x79a000) message ----

TEST_CASE("SpecialSeedSlotsShooter4Snake3OthersVanilla") {
	RulesGuard g;
	CHECK(SpecialSeedSlots(SpecialLevel::Shooter, 7) == 4);
	CHECK(SpecialSeedSlots(SpecialLevel::StarSnake, 7) == 3);
	CHECK(SpecialSeedSlots(SpecialLevel::None, 7) == 7);
	CHECK(SpecialSeedSlots(SpecialLevel::Hidden1, 8) == 8);
	CHECK(SpecialSeedSlots(SpecialLevel::Boss, 6) == 6); // the boss level is another task's
}

TEST_CASE("ShooterStartIsOneCabbageAtColumn0Row0FiringEvery50") {
	RulesGuard g;
	const ShooterSetup s = ShooterStart();
	CHECK(s.seed == kCabbage);
	CHECK(s.row == 0); // EAX = 0 at 0x798466 (Board::AddPlant takes the row in EAX)
	CHECK(s.col == 0); // push 0 at 0x79846b
	CHECK(s.launchRate == 50); // Plant+0x5c = mLaunchRate
	CHECK(s.startSun == 0);
	CHECK(ShooterMessageTime() == 250);
	CHECK(ShooterFirstWaveCountdown() == 250);
}

TEST_CASE("ShooterKillGivesFiveTimesTheZombieValue") {
	RulesGuard g;
	CHECK(ShooterKillSun(1) == 5);
	CHECK(ShooterKillSun(7) == 35); // Football
	CHECK(ShooterKillSun(0) == 0);
}

TEST_CASE("ShooterUpgradesCabbageToMelonAt500AndMelonToWinterAt2000") {
	RulesGuard g;
	CHECK(ShooterUpgrade(kCabbage, 499) == -1);
	CHECK(ShooterUpgrade(kCabbage, 500) == kMelon);
	CHECK(ShooterUpgrade(kCabbage, 2500) == kMelon); // one step per kill (0x7985f7)
	CHECK(ShooterUpgrade(kMelon, 1999) == -1);
	CHECK(ShooterUpgrade(kMelon, 2000) == kWinterMelon);
	CHECK(ShooterUpgrade(kWinterMelon, 9000) == -1);
	CHECK(ShooterUpgrade(kPeashooter, 9000) == -1);
	CHECK(ShooterLaunchRate(kMelon) == 50);
	CHECK(ShooterLaunchRate(kWinterMelon) == 50);
}

TEST_CASE("ShooterArrowsMoveOneRowInsideRows0To4") {
	RulesGuard g;
	CHECK(SteerFromArrow(kKeyUp) == Steer::Up);
	CHECK(SteerFromArrow(kKeyDown) == Steer::Down);
	CHECK(SteerFromArrow(kKeyLeft) == Steer::None);
	CHECK(ShooterStepRow(2, Steer::Up, 5) == 1);
	CHECK(ShooterStepRow(0, Steer::Up, 5) == 0);   // 0x79b0d6: row 0 does not move
	CHECK(ShooterStepRow(4, Steer::Down, 5) == 4); // 0x79b0f9: row 4 does not move
	CHECK(ShooterStepRow(3, Steer::Down, 5) == 4);
	CHECK(ShooterStepRow(3, Steer::Left, 5) == 3);
}

TEST_CASE("ShooterHalvesBodyHelmAndShieldButNoMaxHp") {
	RulesGuard g;
	CHECK(ShooterHalvesHp(kNormal));
	CHECK(ShooterHalvesHp(kFootball));
	CHECK(ShooterHalvesHp(kBoss));
	CHECK_FALSE(ShooterHalvesHp(kPeaHead));
	CHECK_FALSE(ShooterHalvesHp(kJalapenoHead));
	ZombieHp hp{270, 270, 1400, 1400, 1100, 1100};
	ShooterHalveHp(hp);
	CHECK(hp.body == 135);
	CHECK(hp.helm == 700);
	CHECK(hp.shield == 550);
	// 0x797e04 stored the body max before 0x799787 runs; 0x523fc4 / 0x523fca then store the full helmet and shield
	// (EDX / EAX, untouched by the hook) over the halved max values it wrote
	CHECK(hp.bodyMax == 270);
	CHECK(hp.helmMax == 1400);
	CHECK(hp.shieldMax == 1100);
}

TEST_CASE("TouchTapOrSwipe") {
	RulesGuard g;
	CHECK_FALSE(ClassifyTouch(10, -20).swipe);
	Touch t = ClassifyTouch(5, -60);
	CHECK(t.swipe);
	CHECK(t.dir == Steer::Up);
	CHECK(ClassifyTouch(-80, 30).dir == Steer::Left);
	CHECK(ClassifyTouch(80, 30).dir == Steer::Right);
	CHECK(ClassifyTouch(10, 45).dir == Steer::Down);
	// 6-5 on the phone: a tap on a row jumps there, a swipe up / down is one arrow press
	CHECK(ShooterTouchRow(0, ClassifyTouch(0, 0), 3, 5) == 3);
	CHECK(ShooterTouchRow(2, ClassifyTouch(0, 0), -1, 5) == 2); // a tap off the lawn rows
	CHECK(ShooterTouchRow(2, ClassifyTouch(0, 0), 5, 5) == 2);
	CHECK(ShooterTouchRow(2, ClassifyTouch(0, -90), 4, 5) == 1);
	CHECK(ShooterTouchRow(4, ClassifyTouch(0, 90), 0, 5) == 4);
	CHECK(ShooterTouchRow(2, ClassifyTouch(90, 0), 0, 5) == 2); // a sideways swipe does nothing
}

TEST_CASE("ShooterRulesMissingMeanVanilla") {
	VanillaGuard g;
	CHECK(ShooterStart().seed == -1);
	CHECK(ShooterKillSun(7) == 0);
	CHECK(ShooterUpgrade(kCabbage, 9999) == -1);
	CHECK(ShooterLaunchRate(kCabbage) == -1);
	CHECK_FALSE(ShooterHalvesHp(kNormal));
	CHECK(ShooterMessageTime() == -1);
	CHECK(ShooterFirstWaveCountdown() == -1);
	CHECK(SpecialSeedSlots(SpecialLevel::Shooter, 6) == 6);
}

// ---- 7-5 (65): hook 138 (0x79b3f1..) steering, hook 401 (0x7a1fc1) replay ----

TEST_CASE("SnakeKeysWASD") {
	CHECK(SteerFromKey('W') == Steer::Up);
	CHECK(SteerFromKey('A') == Steer::Left);
	CHECK(SteerFromKey('S') == Steer::Down);
	CHECK(SteerFromKey('D') == Steer::Right);
	CHECK(SteerFromKey('Q') == Steer::None);
	CHECK(SteerFromKey(kKeyUp) == Steer::None);
}

TEST_CASE("SnakeHeadTurnsOnlyAcrossItsCourse") {
	RulesGuard g;
	float vx = 3.33f, vy = 0.0f; // flying right: only W / S turn it
	CHECK_FALSE(SteerHead(Steer::Left, vx, vy));
	CHECK(vx == doctest::Approx(3.33f));
	CHECK(SteerHead(Steer::Up, vx, vy));
	CHECK(vx == 0.0f);
	CHECK(vy == -3.0f);
	CHECK_FALSE(SteerHead(Steer::Down, vx, vy)); // vy != 0: only A / D
	CHECK(SteerHead(Steer::Right, vx, vy));
	CHECK(vx == 3.0f);
	CHECK(vy == 0.0f);
	float dx = 3.33f, dy = 3.33f; // a diagonal star: vy != 0, so A / D first
	CHECK_FALSE(SteerHead(Steer::Up, dx, dy));
	CHECK(SteerHead(Steer::Left, dx, dy));
	CHECK(dx == -3.0f);
	CHECK(dy == 0.0f);
}

TEST_CASE("SnakeTurnTableReplaysAtTheSameAge") {
	RulesGuard g;
	StarTurns t;
	CHECK(t.Record(0, 120, Steer::Up));
	CHECK(t.Record(1, 200, Steer::Left));
	Steer s = Steer::None;
	CHECK_FALSE(t.TurnAt(0, 119, s));
	CHECK(t.TurnAt(0, 120, s));
	CHECK(s == Steer::Up);
	CHECK(t.TurnAt(1, 200, s));
	CHECK(s == Steer::Left);
	CHECK_FALSE(t.TurnAt(2, 200, s)); // nothing recorded there yet
	CHECK_FALSE(t.Record(5000, 1, Steer::Up)); // past maxTurns
	CHECK(t.Record(0, 300, Steer::Down)); // a new head writes over entry 0, as the exe's table at 0x810000
	CHECK(t.TurnAt(0, 300, s));
	CHECK(s == Steer::Down);
	CHECK(t.Record(2, 0, Steer::Up));
	CHECK_FALSE(t.TurnAt(2, 0, s)); // age 0 never replays (0x7a1ffc)
	t.Reset();
	CHECK_FALSE(t.TurnAt(0, 300, s));
	float vx = 0, vy = 0;
	ApplyTurn(Steer::Down, vx, vy);
	CHECK(vy == 3.0f);
	CHECK(vx == 0.0f);
}

TEST_CASE("SnakeFieldIs0To899By40To599") {
	RulesGuard g;
	CHECK_FALSE(StarOutsideField(0, 40));
	CHECK_FALSE(StarOutsideField(899, 599));
	CHECK(StarOutsideField(-1, 300));
	CHECK(StarOutsideField(900, 300));
	CHECK(StarOutsideField(400, 39));
	CHECK(StarOutsideField(400, 600));
}

// ---- 7-5 / mode 0x89: the snake's start (0x7a2230), growth (0x7a20f9) and hits (0x6511e3); study glutton-star.md ----

TEST_CASE("SnakeStartsAsThreeStarsHeadFirst") {
	RulesGuard g;
	const std::vector<SnakeStar> s = SnakeStartStars();
	REQUIRE(s.size() == 3);
	CHECK(s[0].x == 120); // the head: made first, so item 0 of the projectile array
	CHECK(s[0].y == 280);
	CHECK(s[0].age == 12);
	CHECK(s[1].x == 102);
	CHECK(s[1].age == 6);
	CHECK(s[2].x == 84);
	CHECK(s[2].age == 0);
	CHECK(SnakeStartRow() == 2);
	CHECK(SnakeStartRenderOrder() == 0x4ec80);
}

TEST_CASE("SnakeBankIsStarfruitSunflowerWallnut") {
	RulesGuard g;
	CHECK(SnakeBankSeeds() == std::vector<int>{kStarfruit, 1, 3});
	CHECK(SnakeGrowSeed() == kStarfruit);
}

TEST_CASE("SnakeGrowsOneStarSixTicksBehindTheTail") {
	RulesGuard g;
	// tail flying right at x 84: the new star 18 px behind it, 6 ticks younger, drawn 82 px over its shadow
	SnakeStar n = SnakeGrowBehind(84, 280, 3.0f, 0.0f, 40);
	CHECK(n.x == 66);
	CHECK(n.y == 280);
	CHECK(n.age == 34);
	n = SnakeGrowBehind(300, 100, 0.0f, -3.0f, 6); // tail flying up: the new star below it
	CHECK(n.x == 300);
	CHECK(n.y == 118);
	CHECK(n.age == 0);
	CHECK(SnakeGrowRenderOrder() == 0x53bd8);
	CHECK(SnakeGrowShadowYOffset() == 82);
	CHECK(SnakeStarsPierce());
}

TEST_CASE("SnakeTurnTableLivesInTheBoardStateAndSurvivesSaveLoad") {
	RulesGuard g;
	BoardState b;
	CHECK(b.IsDefault());
	{
		StarTurns t(b.starTurns); // the board's table (exe 0x810000), not a static
		CHECK(t.Record(0, 120, Steer::Up));
		CHECK(t.Record(1, 200, Steer::Left));
	}
	CHECK_FALSE(b.IsDefault()); // so the board chunk is written while a snake has turned
	b.starSlots.push_back({0, 7, 897, 280, 3.0f, 0.0f, 300, 0x20002, 4}); // a dead head kept for the exe's stale memory
	const std::vector<uint8_t> aBlob = Encode(b);
	BoardState b2;
	REQUIRE(Decode(aBlob.data(), aBlob.size(), b2));
	CHECK(b2 == b);
	StarTurns t2(b2.starTurns);
	Steer s = Steer::None;
	CHECK(t2.TurnAt(1, 200, s));
	CHECK(s == Steer::Left);
	// an older 8-byte chunk (ice freeze only) still loads, with no turns
	BoardState old;
	old.iceFreezeActive = 1;
	old.iceFreezeCounter = 5;
	const std::vector<uint8_t> aOld = {1, 0, 0, 0, 5, 0, 0, 0};
	BoardState b3;
	REQUIRE(Decode(aOld.data(), aOld.size(), b3));
	CHECK(b3 == old);
	CHECK(Encode(old).size() == 8); // no snake: the chunk is byte-identical to before
	// a truncated table is refused, not half-read
	BoardState b4;
	CHECK_FALSE(Decode(aBlob.data(), 14, b4));
	CHECK(b4 == BoardState{});
}

TEST_CASE("SnakeRulesMissingMeanNoSnake") {
	VanillaGuard g;
	CHECK(SnakeStartStars().empty());
	CHECK(SnakeBankSeeds().empty());
	CHECK(SnakeGrowSeed() == -1);
	CHECK_FALSE(SnakeStarsPierce());
}

// ---- 71: hooks 21, 93, 515, 425; 73: hooks 71, 217, 425, 43 ----

TEST_CASE("Hidden1WavePointsAre112TimesTheBudgetBeforeMultipliers") {
	RulesGuard g;
	CHECK(SpecialWavePoints(SpecialLevel::Hidden1, 1, 4) == 112);
	CHECK(SpecialWavePoints(SpecialLevel::Hidden1, 10, 40) == 1120);
	CHECK(SpecialWavePoints(SpecialLevel::None, 10, 40) == 40);
	CHECK(SpecialWavePoints(SpecialLevel::Boss, 10, 40) == 40);
}

// V2 (C1): 72 `imul ecx,ecx,0x14` (0xc13070) and 73 `imul ecx,ecx,0xf` (0xc13300) before the x4 tail 0x409960
TEST_CASE("Hidden2WavePointsAre80AndHidden3Are60TimesTheBudgetBeforeMultipliers") {
	RulesGuard g;
	CHECK(SpecialWavePoints(SpecialLevel::Hidden2, 3, 12) == 240);
	CHECK(SpecialWavePoints(SpecialLevel::Hidden2, 10, 40) == 800);
	CHECK(SpecialWavePoints(SpecialLevel::Hidden3, 10, 40) == 600);
}

// V2 (C1): 72 drains every plant (hook 316, Plant::Update 0x463ed7 -> 0xc13150 / 0xc1301a): when mMainCounter % 6 == 1
// the bite function 0x950000 runs with (plant, 1, 1): 1 HP per 6 ticks = 100 HP per 600 cs ("[quote not included]")
TEST_CASE("Hidden2DrainsOneHpEverySixTicks") {
	RulesGuard g;
	CHECK(PlantDrainDue(SpecialLevel::Hidden2, 1));
	CHECK(PlantDrainDue(SpecialLevel::Hidden2, 7));
	CHECK(PlantDrainDue(SpecialLevel::Hidden2, 601));
	CHECK_FALSE(PlantDrainDue(SpecialLevel::Hidden2, 0));
	CHECK_FALSE(PlantDrainDue(SpecialLevel::Hidden2, 6));
	CHECK_FALSE(PlantDrainDue(SpecialLevel::Hidden1, 1));
	CHECK_FALSE(PlantDrainDue(SpecialLevel::None, 1));
	int aDue = 0;
	for (int t = 1; t <= 600; t++)
		aDue += PlantDrainDue(SpecialLevel::Hidden2, t) ? 1 : 0;
	CHECK(aDue * PlantDrainDamage() == 100);
}

// 0x950005..0x95007d: asleep -> bitten; state 1 -> bitten; states 3..7 (Squash) -> spared; outside I, Zombie the
// instants 2, 14, 15, 20, 27 and mod seeds 0x52, 0x6b -> spared
TEST_CASE("Hidden2DrainSparesSquashStatesAndInstants") {
	RulesGuard g;
	CHECK_FALSE(PlantDrainSkips(kPeashooter, 0, false, false));
	CHECK(PlantDrainSkips(2, 0, false, false));            // Cherry Bomb
	CHECK(PlantDrainSkips(kBlover, 0, false, false));
	CHECK(PlantDrainSkips(107, 0, false, false));          // 0x6b
	CHECK(PlantDrainSkips(82, 0, false, false));           // 0x52
	CHECK_FALSE(PlantDrainSkips(15, 0, true, false));      // an asleep Doom-shroom is drained (0x950005 first)
	CHECK(PlantDrainSkips(17, 5, false, false));           // Squash falling
	CHECK_FALSE(PlantDrainSkips(17, 1, false, false));     // state 1 is drained (0x950015)
	CHECK_FALSE(PlantDrainSkips(2, 0, false, true));       // I, Zombie level: no seed list
}

TEST_CASE("Hidden1BloverEvery1000Ticks") {
	RulesGuard g;
	CHECK_FALSE(AutoPlantDue(SpecialLevel::Hidden1, 0));
	CHECK_FALSE(AutoPlantDue(SpecialLevel::Hidden1, 999));
	CHECK(AutoPlantDue(SpecialLevel::Hidden1, 1000));
	CHECK(AutoPlantDue(SpecialLevel::Hidden1, 3000));
	CHECK_FALSE(AutoPlantDue(SpecialLevel::Hidden2, 1000));
	const AutoPlant p = HiddenAutoPlant();
	CHECK(p.seed == kBlover);
	CHECK(p.col == 7);
	CHECK(p.row == -1); // EAX = -1 at 0x95c166
}

TEST_CASE("Hidden1HelmetsDouble") {
	RulesGuard g;
	CHECK(SpecialHelmHp(SpecialLevel::Hidden1, 370) == 740);
	CHECK(SpecialHelmHp(SpecialLevel::Hidden3, 370) == 370);
}

TEST_CASE("HiddenChooserBans") {
	RulesGuard g;
	CHECK(SpecialSeedBanned(SpecialLevel::Hidden1, kBlover));
	CHECK_FALSE(SpecialSeedBanned(SpecialLevel::Hidden1, 2));
	for (int s : {2, 4, 11, 12, 14, 15, 17, 20, 27, 35, 38})
		CHECK(SpecialSeedBanned(SpecialLevel::Hidden3, s));
	CHECK_FALSE(SpecialSeedBanned(SpecialLevel::Hidden3, kStarfruit));
	CHECK_FALSE(SpecialSeedBanned(SpecialLevel::Hidden2, kBlover));
	CHECK_FALSE(SpecialSeedBanned(SpecialLevel::None, kBlover));
}

TEST_CASE("Hidden3ColumnPlantingWhileNoZombiesAndAtMost29Plants") {
	RulesGuard g;
	CHECK(SpecialColumnPlanting(SpecialLevel::Hidden3, 73, 1, 0));
	CHECK(SpecialColumnPlanting(SpecialLevel::Hidden3, 73, 29, 0));
	CHECK_FALSE(SpecialColumnPlanting(SpecialLevel::Hidden3, 73, 30, 0)); // `cmp [ebp+0xbc], 0x1d; setg`
	CHECK_FALSE(SpecialColumnPlanting(SpecialLevel::Hidden3, 73, 3, 1));  // `cmp [ebp+0xa0], 0`
	CHECK_FALSE(SpecialColumnPlanting(SpecialLevel::Hidden3, 72, 3, 0));  // the profile must be on 73 too
	CHECK_FALSE(SpecialColumnPlanting(SpecialLevel::Hidden1, 73, 3, 0));
}

TEST_CASE("Hidden3NewspaperFromWave5") {
	RulesGuard g;
	CHECK(SpecialFirstAllowedWave(SpecialLevel::Hidden3, true, 73, kNewspaper, 1) == 5);
	CHECK(SpecialFirstAllowedWave(SpecialLevel::Hidden3, false, 73, kNewspaper, 1) == 1); // Adventure only
	CHECK(SpecialFirstAllowedWave(SpecialLevel::Hidden3, true, 72, kNewspaper, 1) == 1);
	CHECK(SpecialFirstAllowedWave(SpecialLevel::Hidden3, true, 73, kFootball, 5) == 5);
	CHECK(SpecialFirstAllowedWave(SpecialLevel::Hidden1, true, 73, kNewspaper, 1) == 1);
}

TEST_CASE("HiddenRulesMissingMeanVanilla") {
	VanillaGuard g;
	CHECK(SpecialWavePoints(SpecialLevel::Hidden1, 10, 40) == 40);
	CHECK(SpecialWavePoints(SpecialLevel::Hidden2, 10, 40) == 40);
	CHECK_FALSE(PlantDrainDue(SpecialLevel::Hidden2, 1));
	CHECK_FALSE(AutoPlantDue(SpecialLevel::Hidden1, 1000));
	CHECK(SpecialHelmHp(SpecialLevel::Hidden1, 370) == 370);
	CHECK_FALSE(SpecialSeedBanned(SpecialLevel::Hidden1, kBlover));
	CHECK_FALSE(SpecialColumnPlanting(SpecialLevel::Hidden3, 73, 1, 0));
	CHECK(SpecialFirstAllowedWave(SpecialLevel::Hidden3, true, 73, kNewspaper, 1) == 1);
}

// ---- the pack holds the same numbers ----

TEST_CASE("PackSpecialLevelRulesMatchTheStudy") {
	std::ifstream in(PackFile("data/rules-special-levels.json"));
	REQUIRE(in.good());
	const nlohmann::json aPack = nlohmann::json::parse(in);
	CHECK(aPack == kRules);
	std::ifstream mod(PackFile("mod.json"));
	REQUIRE(mod.good());
	CHECK(nlohmann::json::parse(mod)["features"]["specialLevels"] == true);
}
