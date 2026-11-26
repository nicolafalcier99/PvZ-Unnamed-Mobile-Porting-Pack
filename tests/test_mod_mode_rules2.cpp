// PVZU: pure rules of the Unnamed mod-only modes, second pass (pvz-unnamed-pack SOURCES-mod-modes.md; every number is a
// constant in the cited .NewSec code): 0x66, 0x6d, 0x72, 0x7f/0x90, 0x88, 0x8b, 0x91, 0x92 and the later ones.
#include "doctest/doctest.h"
#include "Mod/Unnamed/ModModeRules.h"

using namespace pvzu::unnamed;

TEST_CASE("0x66 Sun Storm: sky-sun countdown halved and capped at 450 (hook 95)") {
	CHECK(SunStormCountdown(600) == 300);
	CHECK(SunStormCountdown(901) == 450);
	CHECK(SunStormCountdown(1224) == 450);
	CHECK(SunStormCountdown(425) == 212);
}

TEST_CASE("0x66 Sun Storm: the charged sun's blast box around (round(x)-30, round(y)-30) (hook 208)") {
	// zombies: |zy - cy| <= 120 and zx - 125 <= cx <= zx + 130
	CHECK(SunStormHitsZombie(400, 200, 400, 320));
	CHECK_FALSE(SunStormHitsZombie(400, 200, 400, 321));
	CHECK(SunStormHitsZombie(400, 200, 525, 200));
	CHECK_FALSE(SunStormHitsZombie(400, 200, 526, 200));
	CHECK(SunStormHitsZombie(400, 200, 270, 200));
	CHECK_FALSE(SunStormHitsZombie(400, 200, 269, 200));
	// plants: |py - cy| <= 100, same x window
	CHECK(SunStormHitsPlant(400, 200, 400, 300));
	CHECK_FALSE(SunStormHitsPlant(400, 200, 400, 301));
	CHECK(SunStormBlastDamage() == 270);
}

TEST_CASE("0x6d Night Terror: the 25 craters of hook 462") {
	static const char* kRows[5] = {"..XXX.XXX", "..X.X.X..", "..X.X.XXX", "..X.X...X", "XXX.XXXXX"};
	int aCount = 0;
	for (int r = 0; r < 5; r++)
		for (int c = 0; c < 9; c++) {
			CHECK(NightTerrorCrater(c, r) == (kRows[r][c] == 'X'));
			aCount += NightTerrorCrater(c, r) ? 1 : 0;
		}
	CHECK(aCount == 25);
	CHECK_FALSE(NightTerrorCrater(0, 5));
}

TEST_CASE("0x6d Night Terror: the scripted walk (hook 604)") {
	int r = 0;
	CHECK(NightTerrorStep(700, 50, r) == TerrorMove::Walk);
	CHECK(r == 0);
	// row 0 down to row 2 at x 495..499
	CHECK(NightTerrorStep(497, 50, r) == TerrorMove::Hold);
	CHECK(r == 1);
	CHECK(NightTerrorStep(497, 120, r) == TerrorMove::Hold);
	CHECK(r == 1);
	CHECK(NightTerrorStep(497, 150, r) == TerrorMove::Hold);
	CHECK(r == 2);
	// row 2 reached: walk right
	CHECK(NightTerrorStep(497, 250, r) == TerrorMove::WalkRight);
	CHECK(NightTerrorStep(600, 250, r) == TerrorMove::WalkRight);
	// right end 655..660: down to row 4, hold to y 443
	CHECK(NightTerrorStep(656, 250, r) == TerrorMove::Hold);
	CHECK(r == 3);
	CHECK(NightTerrorStep(656, 350, r) == TerrorMove::Hold);
	CHECK(r == 4);
	CHECK(NightTerrorStep(656, 443, r) == TerrorMove::Hold);
	CHECK(NightTerrorStep(656, 450, r) == TerrorMove::Walk);
	CHECK(NightTerrorStep(500, 450, r) == TerrorMove::Walk);
	// x 335..340: up to row 0
	CHECK(NightTerrorStep(338, 450, r) == TerrorMove::Hold);
	CHECK(r == 3);
	CHECK(NightTerrorStep(338, 350, r) == TerrorMove::Hold);
	CHECK(r == 2);
	CHECK(NightTerrorStep(338, 250, r) == TerrorMove::Hold);
	CHECK(r == 1);
	CHECK(NightTerrorStep(338, 150, r) == TerrorMove::Hold);
	CHECK(r == 0);
	CHECK(NightTerrorStep(338, 54, r) == TerrorMove::Hold);
	CHECK(NightTerrorStep(338, 50, r) == TerrorMove::Walk);
	CHECK(NightTerrorStep(250, 50, r) == TerrorMove::Walk);
	// x 170..175: down to row 4, then into the house
	CHECK(NightTerrorStep(172, 50, r) == TerrorMove::Hold);
	CHECK(r == 1);
	r = 4;
	CHECK(NightTerrorStep(172, 450, r) == TerrorMove::Walk);
	CHECK(NightTerrorStep(100, 450, r) == TerrorMove::Walk);
}

TEST_CASE("0x6d Night Terror: y step 0.1875 when 1 px or more away, HP x(wave+4)/4 (hooks 616, 617, 517)") {
	CHECK(NightTerrorYStep(100.0f) == doctest::Approx(0.1875f));
	CHECK(NightTerrorYStep(1.0f) == doctest::Approx(0.1875f));
	CHECK(NightTerrorYStep(0.5f) == doctest::Approx(0.5f));
	CHECK(NightTerrorHp(270, 0) == 270);
	CHECK(NightTerrorHp(270, 4) == 540);
	CHECK(NightTerrorHp(270, 40) == 2970);
	CHECK(NightTerrorHp(370, 1) == 462);
}

TEST_CASE("0x72 Lone Wolf: launch rates after an upgrade (0x798629): 50, Gatling 96, Repeater 64; sun = value x 5") {
	CHECK(LoneWolfLaunchRate(0) == 50);
	CHECK(LoneWolfLaunchRate(5) == 50);
	CHECK(LoneWolfLaunchRate(7) == 64);
	CHECK(LoneWolfLaunchRate(18) == 50);
	CHECK(LoneWolfLaunchRate(40) == 96);
	CHECK(LoneWolfKillSun(4) == 20);
}

TEST_CASE("0x7f / 0x90 Trick of Tricks: a Squash that hits nothing counts; 15 drops the bag (hooks 296, 297, 96, 234)") {
	int aCount = 0;
	CHECK_FALSE(TrickOfTricksSmash(aCount, 2).counted);
	CHECK(aCount == 0);
	for (int i = 0; i < 14; i++)
		CHECK_FALSE(TrickOfTricksSmash(aCount, 0).bag);
	CHECK(aCount == 14);
	CHECK(TrickOfTricksLost(aCount));
	const TrickMiss m = TrickOfTricksSmash(aCount, 0);
	CHECK(m.counted);
	CHECK(m.bag);
	CHECK(aCount == 15);
	CHECK_FALSE(TrickOfTricksLost(aCount));
	CHECK(TrickOfTricksSmash(aCount, 0).bag); // every later miss drops another bag (literal)
	CHECK(TrickHudX(9) == 660);
	CHECK(TrickHudX(10) == 645);
}

TEST_CASE("0x88 Territorial: the first plant claims the cell (hooks 38, 168, 174)") {
	CHECK(TerritorialStore(0, 0) == 1);  // Peashooter -> 1
	CHECK(TerritorialStore(0, 48) == 0); // Imitater never claims
	CHECK(TerritorialStore(8, 3) == 8);  // claimed already
	CHECK(TerritorialSeed(0) == -1);
	CHECK(TerritorialSeed(1) == 0);
	CHECK(TerritorialSeed(17) == 16);
	CHECK_FALSE(TerritorialRefuses(-1, 3));
	CHECK_FALSE(TerritorialRefuses(3, 3));
	CHECK(TerritorialRefuses(3, 0));
	CHECK_FALSE(TerritorialRefuses(3, 16)); // Lily Pad
	CHECK_FALSE(TerritorialRefuses(3, 11)); // Grave Buster
	CHECK_FALSE(TerritorialRefuses(3, 30)); // Pumpkin
	CHECK_FALSE(TerritorialRefuses(3, 0x5b));
	CHECK_FALSE(TerritorialRefuses(3, 0x6e));
}

TEST_CASE("0x8b Bullet Carnival: tiers 0x8f3464 and goals 0x8d3464") {
	// engine ProjectileType: PEA 0 SNOWPEA 1 CABBAGE 2 MELON 3 PUFF 4 WINTERMELON 5 FIREBALL 6 STAR 7 SPIKE 8 BASKETBALL 9
	// KERNEL 10 COBBIG 11 BUTTER 12
	static const int kTier[13] = {2, -1, 3, 8, 1, 9, -1, 4, 5, 6, 0, 10, 7};
	for (int t = 0; t < 13; t++)
		CHECK(CarnivalTier(t) == kTier[t]);
	CHECK(CarnivalTier(13) == -1);
	CHECK(CarnivalGoal(0) == 5);
	CHECK(CarnivalGoal(9) == 8000);
	CHECK(CarnivalGoal(10) == 0);
	CHECK(CarnivalTypeOfTier(1) == 4);
	CHECK(CarnivalTypeOfTier(10) == 11);
	CHECK(CarnivalTypeOfTier(11) == -1);
	CHECK(CarnivalEatScore(0) == 1);
	CHECK(CarnivalEatScore(3) == 16);
}

TEST_CASE("0x8b Bullet Carnival: spawn weights and places (0x7b8c69..0x7b8dfa)") {
	// player Kernel (tier 0): target tier 1
	CHECK(CarnivalWeight(4, 0) == 800);  // Puff, d 0
	CHECK(CarnivalWeight(10, 0) == 720); // Kernel, d -1
	CHECK(CarnivalWeight(0, 0) == 720);  // Pea, d 1
	CHECK(CarnivalWeight(7, 0) == 1);    // Star tier 4, d 3: default 1
	CHECK(CarnivalWeight(1, 0) == 1);    // Snow Pea (no tier)
	CHECK(CarnivalWeight(10, 9) == 0);   // Kernel for a tier-9 player: d -10
	CHECK_FALSE(CarnivalSpawnEdge(450));
	const CarnivalSpawn a = CarnivalSpawnPlace(450, 10); // x 400, not an edge: y = 10 - 40
	CHECK(a.x == 400);
	CHECK(a.y == -30);
	const CarnivalSpawn b = CarnivalSpawnPlace(450, 40); // r >= 30 -> + 620
	CHECK(b.y == 620);
	CHECK(CarnivalSpawnEdge(10));       // x -40
	CHECK(CarnivalSpawnEdge(860));      // x 810
	CHECK_FALSE(CarnivalSpawnEdge(34)); // x -16
	const CarnivalSpawn c = CarnivalSpawnPlace(10, 500);
	CHECK(c.x == -40);
	CHECK(c.y == 460);
}

TEST_CASE("0x8b Bullet Carnival: rects (0x7b8e0b): Cob x+w/4, w-2*(w/4); Star and tier >= 6 full; else half") {
	CarnivalRect r = CarnivalBulletRect(11, 100, 50, 40, 20);
	CHECK(r.x == 110);
	CHECK(r.w == 20);
	r = CarnivalBulletRect(7, 100, 50, 40, 20);
	CHECK(r.w == 40);
	CHECK(r.h == 20);
	r = CarnivalBulletRect(9, 100, 50, 41, 21);
	CHECK(r.w == 41);
	r = CarnivalBulletRect(0, 100, 50, 41, 21);
	CHECK(r.x == 100);
	CHECK(r.w == 20);
	CHECK(r.h == 10);
	CHECK(CarnivalOverlap({0, 0, 10, 10}, {9, 9, 5, 5}));
	CHECK_FALSE(CarnivalOverlap({0, 0, 10, 10}, {10, 0, 5, 5}));
}

TEST_CASE("0x91 / 0x73: packets 1..5 copy packet 0, the Imitater's copy (0x798fdf)") {
	CHECK(AllTheSameSeed(7, -1) == 7);
	CHECK(AllTheSameSeed(48, 2) == 2);
	CHECK(AllTheSameSeed(-1, -1) == -1);
	CHECK(kAllTheSameSun == 0x70000000);
}

TEST_CASE("0x92 Elite Zombies' Challenge: every wave slot spawns the Dancer; the Zombie Sea fills all 50 (hook 90)") {
	CHECK(EliteChallengeSlot(0, false) == 8);
	CHECK(EliteChallengeSlot(4, false) == 8);
	CHECK(EliteChallengeSlot(-1, false) == -1);
	CHECK(EliteChallengeSlot(-1, true) == 8);
}

TEST_CASE("0x71 The Counter: at most 6 plants (hook 46 chain 0x7a7532)") {
	CHECK_FALSE(CounterRefuses(5));
	CHECK(CounterRefuses(6));
}

TEST_CASE("0x76 Schrodinger: corpses of types >= 5 stay, except Zamboni, Bungee, Catapult, Boss (hook 725)") {
	for (int t : {0, 1, 2, 3, 4, 12, 20, 22, 25})
		CHECK_FALSE(SchrodingerKeepsCorpse(t));
	for (int t : {5, 6, 7, 8, 23, 32})
		CHECK(SchrodingerKeepsCorpse(t));
}

TEST_CASE("0x7d Random Vasebreaker: Rand(65) per vase (hook 187, 0x7a8651)") {
	CHECK(RandomVaseRoll(0).kind == VaseKind::Sun);
	CHECK(RandomVaseRoll(1).kind == VaseKind::Zombie);
	CHECK(RandomVaseRoll(1).zombie == 0);
	CHECK(RandomVaseRoll(25).zombie == 24);
	CHECK(RandomVaseRoll(26).kind == VaseKind::Seed); // zombie type 25 (Boss) becomes a seed vase
	CHECK(RandomVaseRoll(27).zombie == 26);
	CHECK(RandomVaseRoll(32).zombie == 31);
	CHECK(RandomVaseRoll(33).kind == VaseKind::Seed);
	CHECK(RandomVaseRoll(64).kind == VaseKind::Seed);
	int aZombies = 0, aSeeds = 0;
	for (int r = 0; r < 65; r++) {
		aZombies += RandomVaseRoll(r).kind == VaseKind::Zombie;
		aSeeds += RandomVaseRoll(r).kind == VaseKind::Seed;
	}
	CHECK(aZombies == 31);
	CHECK(aSeeds == 33);
}

TEST_CASE("0x7d Random Vasebreaker: seed re-roll 0x7b2350 (Rand(128), no Imitater / Sprout / 53..79 / empty mod rows)") {
	CHECK(RandomVaseSeedOk(0, false));
	CHECK(RandomVaseSeedOk(47, false));
	CHECK_FALSE(RandomVaseSeedOk(48, false));
	CHECK(RandomVaseSeedOk(49, false));
	CHECK(RandomVaseSeedOk(50, false));
	CHECK_FALSE(RandomVaseSeedOk(51, false));
	CHECK(RandomVaseSeedOk(52, false));
	CHECK_FALSE(RandomVaseSeedOk(53, false));
	CHECK_FALSE(RandomVaseSeedOk(0x4f, true));
	CHECK(RandomVaseSeedOk(0x72, true));
	CHECK_FALSE(RandomVaseSeedOk(0x54, false));
	CHECK_FALSE(RandomVaseSeedOk(128, true));
}

TEST_CASE("0x8d Glove War 3: ten plants in columns 0-1 and their +80 packets; a packet toggles the form (0xc004c8, 0xc007d0)") {
	static const int kBases[10] = {9, 23, 25, 27, 29, 32, 34, 35, 37, 42};
	for (int k = 0; k < 10; k++) {
		const GloveWar3Spot s = GloveWar3Start(k);
		CHECK(s.seed == kBases[k]);
		CHECK(s.packet == kBases[k] + 0x50);
		CHECK(s.col == (k < 5 ? 0 : 1));
		CHECK(s.row == k % 5);
	}
	CHECK(GloveWar3Toggle(0x59) == 0x09);
	CHECK(GloveWar3Toggle(0x09) == 0x59);
	CHECK(GloveWar3Toggle(0x2a) == 0x7a);
	CHECK(GloveWar3NoPlantRefresh(1000) == 4000);
}

TEST_CASE("0x8d Glove War 3: a packet with no plant of the other form plants at (0, 0), else the next free cell (port ruling)") {
	// The exe calls AddPlant(0, 0) with no CanPlantAt (0xc008bb..0xc008c5): a second plant would stack on (0, 0).
	std::array<bool, 45> aFree{};
	aFree.fill(true);
	Cell c = GloveWar3PlantCell(aFree);
	CHECK((c.col == 0 && c.row == 0));
	aFree[GloveWar3CellIndex(0, 0)] = false;
	c = GloveWar3PlantCell(aFree);
	CHECK((c.col == 0 && c.row == 1)); // down column 0 first, nearest the house
	for (int r = 0; r < 5; r++)
		aFree[GloveWar3CellIndex(0, r)] = false;
	aFree[GloveWar3CellIndex(1, 0)] = false;
	c = GloveWar3PlantCell(aFree);
	CHECK((c.col == 1 && c.row == 1));
	aFree.fill(false);
	c = GloveWar3PlantCell(aFree);
	CHECK(c.col == -1); // a full lawn: the click is refused
	CHECK(GloveWar3CellIndex(8, 4) == 44);
}

TEST_CASE("0x8d Glove War 3: the sun never goes below 0 (port ruling over the exe's plain subtraction)") {
	CHECK(GloveWar3SunAfter(500, 225) == 275);
	CHECK(GloveWar3SunAfter(50, 225) == 0);
	CHECK(GloveWar3SunAfter(0, 0) == 0);
}

TEST_CASE("0x8e Brother Zeng: W/S rows, A/D columns; wave 9 Element-shroom, wave 20 puffs (hooks 138, 99, 275)") {
	int c = 3, r = 2;
	ZengStep('W', c, r);
	CHECK((c == 3 && r == 1));
	ZengStep('S', c, r);
	ZengStep('S', c, r);
	CHECK(r == 3);
	ZengStep('A', c, r);
	CHECK(c == 2);
	ZengStep('D', c, r);
	ZengStep('D', c, r);
	CHECK(c == 4);
	ZengStep('X', c, r);
	CHECK((c == 4 && r == 3));
	CHECK_FALSE(ZengElementWave(8));
	CHECK(ZengElementWave(9));
	CHECK_FALSE(ZengPuffs(19));
	CHECK(ZengPuffs(20));
	CHECK(kZengPuffDamage == 80);
}

TEST_CASE("0x8e Brother Zeng on the phone: a swipe is one W/A/S/D step, a tap one step toward the cell (port addition)") {
	Touch aSwipe;
	aSwipe.swipe = true;
	aSwipe.dir = Steer::Up;
	CHECK(ZengTouchKey(3, 2, aSwipe, -1, -1) == 'W');
	aSwipe.dir = Steer::Down;
	CHECK(ZengTouchKey(3, 2, aSwipe, -1, -1) == 'S');
	aSwipe.dir = Steer::Left;
	CHECK(ZengTouchKey(3, 2, aSwipe, 8, 4) == 'A'); // a swipe ignores where it started
	aSwipe.dir = Steer::Right;
	CHECK(ZengTouchKey(3, 2, aSwipe, -1, -1) == 'D');
	const Touch aTap;
	CHECK(ZengTouchKey(3, 2, aTap, 3, 0) == 'W'); // straight up: one row, not two
	CHECK(ZengTouchKey(3, 2, aTap, 3, 4) == 'S');
	CHECK(ZengTouchKey(3, 2, aTap, 0, 2) == 'A');
	CHECK(ZengTouchKey(3, 2, aTap, 8, 3) == 'D'); // the longer axis first
	CHECK(ZengTouchKey(3, 2, aTap, 4, 0) == 'W');
	CHECK(ZengTouchKey(3, 2, aTap, 4, 3) == 'S'); // a tie: the row first
	CHECK(ZengTouchKey(3, 2, aTap, 3, 2) == 0);   // the plant's own cell
	CHECK(ZengTouchKey(3, 2, aTap, -1, 2) == 0);  // off the lawn
	CHECK(ZengTouchKey(3, 2, aTap, 9, 2) == 0);
	CHECK(ZengTouchKey(3, 2, aTap, 3, 5) == 0);
}

TEST_CASE("0x8c I Am Luck: the mod's Imitater table 0xc003ce (Rand(128) re-rolled until the byte is 1, hook 348)") {
	for (int s = 0; s < 48; s++)
		CHECK(ImitaterTableAllows(s));
	CHECK_FALSE(ImitaterTableAllows(48));
	CHECK(ImitaterTableAllows(49));
	CHECK(ImitaterTableAllows(50));
	CHECK_FALSE(ImitaterTableAllows(51));
	CHECK(ImitaterTableAllows(52));
	for (int s = 53; s < 0x50; s++)
		CHECK_FALSE(ImitaterTableAllows(s));
	int aMod = 0;
	for (int s = 0x50; s < 0x80; s++)
		aMod += ImitaterTableAllows(s) ? 1 : 0;
	CHECK(aMod == 22); // the 26 mod plants less Mystic-Ice 0x55, Holy-fire Pumpkin 0x6e, Boss Flower 0x76, 0x7f
	CHECK_FALSE(ImitaterTableAllows(0x55));
	CHECK_FALSE(ImitaterTableAllows(0x6e));
	CHECK_FALSE(ImitaterTableAllows(0x76));
	CHECK_FALSE(ImitaterTableAllows(0x7f));
	CHECK(ImitaterTableAllows(0x7b));
	CHECK_FALSE(ImitaterTableAllows(128));
	CHECK(kIAmLuckImpTypes == 33);
	CHECK(kIAmLuckBossHp == 500);
}

TEST_CASE("0x69 Visual Disorder: Color::White alpha 40; new colour every 128 ticks from wave 12 (hook 103, 0x798ca8)") {
	CHECK(kVisualDisorderAlpha == 40);
	CHECK_FALSE(VisualDisorderRecolours(11, 128));
	CHECK(VisualDisorderRecolours(12, 128));
	CHECK(VisualDisorderRecolours(12, 0));
	CHECK_FALSE(VisualDisorderRecolours(12, 129));
	CHECK(VisualDisorderRecolours(30, 256 * 3));
}

TEST_CASE("item mode (0x7aaf60): 0x6b 1, 0x87 2, 0x90 3, 0x8d 4; the glove's render order (hook 479)") {
	CHECK(ItemModeOf(0x6b) == 1);
	CHECK(ItemModeOf(0x87) == 2);
	CHECK(ItemModeOf(0x90) == 3);
	CHECK(ItemModeOf(0x8d) == 4);
	CHECK(ItemModeOf(0x7f) == 0);
	CHECK(GloveRenderOrder(2, 300, 0) == 2 * 10000 + 302805 - 300);
	CHECK(GloveRenderOrder(2, 300, 16) == 2 * 10000 + 302805 - 300 - 5);
	CHECK(GloveRenderOrder(2, 300, 33) == 2 * 10000 + 302805 - 300 - 5);
	CHECK(GloveRenderOrder(2, 300, 30) == 2 * 10000 + 302805 - 300 + 5);
	// I8: the chooser greys Gatling..Cob (40..47) in item modes 1, 2, 4, not 3
	CHECK(ItemModeHidesSeed(1, 40));
	CHECK(ItemModeHidesSeed(4, 47));
	CHECK_FALSE(ItemModeHidesSeed(3, 40));
	CHECK_FALSE(ItemModeHidesSeed(1, 39));
	CHECK_FALSE(ItemModeHidesSeed(1, 48));
	// 0x90: the glove puts a plant down only on columns 0..3 (hook 173)
	CHECK_FALSE(GloveRefusesColumn(3, 3));
	CHECK(GloveRefusesColumn(3, 4));
	CHECK_FALSE(GloveRefusesColumn(1, 8));
}

TEST_CASE("0x8f Tall-nut's Probe: two vases; the marked one holds a 150000 HP Tall-nut head (0xc01d00, 0xc01e00)") {
	CHECK(kProbeSeedVase.col == 0);
	CHECK(kProbeSeedVase.row == 0);
	CHECK(kProbeBossVase.col == 8);
	CHECK(kProbeBossVase.row == 4);
	CHECK(kProbeBossBody == 150000);
	CHECK(kProbeBossHelm == 120000);
	CHECK(kProbeBossShield == 30000);
	CHECK(kProbeOpenCells.size() == 10);
	CHECK(kProbeOpenCells[0].row == 1);
	CHECK(kProbeOpenCells[0].col == 7);
}

// ---- V2 (the research notes (v2-plant-mode-contradictions)) ----

TEST_CASE("V2 0x6e Little Zombie, Big Trouble: x7 then the Little Trouble x4 tail; 0x7e x(wave/8+1) x4 (hook 21)") {
	// 0x7a7836: cmp eax,0x6e; imul ecx,ecx,7 -> 0x409960 add ecx,ecx; add ecx,ecx
	CHECK(ModeWavePoints(kLittleZombie, 3, 3, 0) == 84);
	CHECK(ModeWavePoints(kLittleZombie, 1, 1, 19) == 28);
	// 0x7a7847: cmp eax,0x7e; ([esp+0xc] >> 3) + 1
	CHECK(ModeWavePoints(kPincerAttack, 2, 2, 0) == 8);
	CHECK(ModeWavePoints(kPincerAttack, 2, 2, 7) == 8);
	CHECK(ModeWavePoints(kPincerAttack, 2, 2, 8) == 16);
	CHECK(ModeWavePoints(kPincerAttack, 2, 2, 17) == 24);
	CHECK(ModeWavePoints(kGloveWar, 3, 12, 5) == 12); // any other mode keeps its own budget
	CHECK(ModeWavePoints(0, 3, 12, 5) == 12);
}

TEST_CASE("V2 0x6e Little Zombie, Big Trouble: zombies run vanilla Little Trouble's block (hook 516)") {
	CHECK(ModeLittleZombies(kLittleZombie));
	CHECK_FALSE(ModeLittleZombies(kBigZombie));
	CHECK_FALSE(ModeLittleZombies(0));
}

TEST_CASE("V2 0x6b Glove War: chosen seeds planted on rows 0-1, Imitater as its copy, none as a Flower Pot (0x7a4c50)") {
	int aCol = -1, aRow = -1;
	GloveWarStartCell(0, aCol, aRow);
	CHECK((aCol == 0 && aRow == 0));
	GloveWarStartCell(4, aCol, aRow);
	CHECK((aCol == 4 && aRow == 0));
	GloveWarStartCell(5, aCol, aRow);
	CHECK((aCol == 0 && aRow == 1));
	GloveWarStartCell(9, aCol, aRow);
	CHECK((aCol == 4 && aRow == 1));
	GloveWarStartCell(12, aCol, aRow);
	CHECK((aCol == 7 && aRow == 1));
	CHECK(GloveWarStartSeed(48, 7) == 7);
	CHECK(GloveWarStartSeed(48, -1) == 33);
	CHECK(GloveWarStartSeed(-1, -1) == 33);
	CHECK(GloveWarStartSeed(3, -1) == 3);
	CHECK(kGloveWarBank[0] == 27); // Blover
	CHECK(kGloveWarBank[1] == 2);  // Cherry Bomb
}

TEST_CASE("V2 0x80 Limited Supply: the first grey packet of the dead plant's seed starts recharging (hook 361, 0x7a8966)") {
	using P = LimitedPacket;
	CHECK(LimitedSupplyPacketToRestart({P{3, -1, false}, P{0, -1, false}}, 0, -1) == 1); // a Peashooter dies
	CHECK(LimitedSupplyPacketToRestart({P{0, -1, true}}, 0, -1) == -1);                 // already recharging
	CHECK(LimitedSupplyPacketToRestart({P{0, -1, false}, P{0, -1, false}}, 0, -1) == 0); // the lowest index only
	// a Repeater copied by an Imitater (plant imitater 48): not a plain Repeater packet, but the Imitater packet of 7
	CHECK(LimitedSupplyPacketToRestart({P{7, -1, false}}, 7, 48) == -1);
	CHECK(LimitedSupplyPacketToRestart({P{7, -1, false}, P{48, 7, false}}, 7, 48) == 1);
	// an Imitater not morphed yet (seed 48, imitater 7) matches the Imitater packet of 7
	CHECK(LimitedSupplyPacketToRestart({P{48, 7, false}}, 48, 7) == 0);
	// an Imitater packet with no copy never matches; a plain plant never matches an Imitater packet
	CHECK(LimitedSupplyPacketToRestart({P{48, -1, false}}, 48, 7) == -1);
	CHECK(LimitedSupplyPacketToRestart({P{48, 0, false}}, 0, -1) == -1);
	CHECK(LimitedSupplyPacketToRestart({}, 0, -1) == -1);
}

TEST_CASE("V2 0x8a Invisible Guardian: only Sunflower, Sun-shroom and Twin Sunflower cards stay shown (0x798f90)") {
	CHECK(InvisibleGuardianCardShown(1));
	CHECK(InvisibleGuardianCardShown(9));
	CHECK(InvisibleGuardianCardShown(0x29));
	CHECK_FALSE(InvisibleGuardianCardShown(0));
	CHECK_FALSE(InvisibleGuardianCardShown(0x30));
	CHECK_FALSE(InvisibleGuardianCardShown(0x51)); // Primal Sunflower is hidden too
}

TEST_CASE("V2 0x8a Invisible Guardian: 2n swaps of Rand(n) pairs, equal pairs rerolled (0x798f01)") {
	// pairs (0,1) (2,2)->(2,0) (1,2) (0,2) (2,1) (1,0): six swaps for n = 3
	const std::vector<int> aRolls = {0, 1, 2, 2, 2, 0, 1, 2, 0, 2, 2, 1, 1, 0};
	size_t aNext = 0;
	std::vector<int> aTypes = {GloveWarStartSeed(48, 7), 1, 3}; // an Imitater of 7 enters as 7
	InvisibleGuardianShuffle(aTypes, [&](int n) {
		CHECK(n == 3);
		return aRolls.at(aNext++);
	});
	CHECK(aNext == aRolls.size());
	CHECK(aTypes == std::vector<int>{3, 7, 1});
	std::vector<int> aOne = {5};
	int aDrawn = 0;
	InvisibleGuardianShuffle(aOne, [&](int) { ++aDrawn; return 0; });
	CHECK(aOne == std::vector<int>{5});
	CHECK(aDrawn == 0);
}
