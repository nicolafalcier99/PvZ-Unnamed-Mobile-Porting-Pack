// PVZU: pure rules of the Unnamed mod-only modes (numbers from pvz-unnamed-pack/SOURCES-mod-modes.md; rolls passed in).
// The engine calls these from Mod/Engine/ModModeRules.cpp only while the mode runs.
#pragma once

#include "Mod/Unnamed/SpecialLevels.h" // Touch / Steer (0x8e on the phone)

#include <array>
#include <utility>
#include <string>
#include <vector>

namespace pvzu::unnamed {

// Mode numbers (LawnApp::mGameMode).
enum ModMode : int {
	kArtIsAnExplosion = 0x65, kSunStorm = 0x66, kNoWater = 0x67, kWarOfAttrition = 0x68, kVisualDisorder = 0x69,
	kDownstream = 0x6a, kGloveWar = 0x6b, kZombiesRun = 0x6c, kNightTerror = 0x6d, kLittleZombie = 0x6e,
	kForce8Gale = 0x6f, kTroopsFromSky = 0x70, kTheCounter = 0x71, kLoneWolf = 0x72, kSlotCrisis = 0x73, kRainyDay = 0x74,
	kHeisenberg = 0x75, kSchrodinger = 0x76, kUpstream = 0x77, kSunflowerCommander = 0x78, kGiantCrisis = 0x79,
	kBigZombie = 0x7a, kFrequencySwitch = 0x7b, kLightAndMist = 0x7c, kRandomVasebreaker = 0x7d, kPincerAttack = 0x7e,
	kTrickOfTricks = 0x7f, kLimitedSupply = 0x80, kLuck = 0x81, kSowBeans = 0x82, kSecondUncle = 0x83, kEyeForEye = 0x84,
	kCounterEspionage = 0x85, kLeaningLeft = 0x86, kGloveWar2 = 0x87, kTerritorial = 0x88, kGluttonStar = 0x89,
	kInvisibleGuardian = 0x8a, kBulletCarnival = 0x8b, kIAmLuck = 0x8c, kGloveWar3 = 0x8d, kZengWasd = 0x8e,
	kTallnutProbe = 0x8f, kTrickOfTricks2 = 0x90, kAllTheSame = 0x91, kEliteChallenge = 0x92,
};

// 0x73 Seed Slot Crisis (hook 69, 0x799681): the packet becomes one of six seeds by Rand(6). Engine SeedType values.
int SlotCrisisSeed(int theRoll6); // 0 Sunflower 1, 1 Potato Mine 4, 2 Snow Pea 5, 3 Repeater 7, 4 Wall-nut 3, 5 Blover 27

// 0x77 Upstream (hook 706, 0x7a6900): px a hit pushes the zombie toward the house: (damage >> 1) + 1, at most 160.
int UpstreamPush(int theDamage);

// 0x6c Zombies Run Run Run (hook 662 -> 0x7a6b80): anim-rate factor from the current wave: 1 + 0.125 * (wave - 5) from
// wave 6, else 1.
float ZombiesRunFactor(int theCurrentWave);

// 0x75 Heisenberg's Zombie (hook 615, 0x798e27): the row a zombie is drawn in. theRolls = successive Rand(5) results;
// rolls equal to theRow are skipped (the exe re-rolls). Returns -1 when every roll equals theRow (ask for more rolls).
int HeisenbergRow(int theRow, const std::vector<int>& theRolls);

// 0x82 Sow Beans, Reap Melons (hook 402, 0x7a81e0): projectile type each tick. Engine ProjectileType values:
// PEA 0 -> MELON 3, SNOWPEA 1 -> WINTERMELON 5, ZOMBIE_PEA 13 -> MELON 3; others unchanged.
int SowBeansProjectile(int theType);

// 0x6f Force-8 Gale (hooks 98, 318).
struct GaleState {
	int timer = 0;   // cs to the next gust (exe Challenge+0x58)
	int dir = 0;     // 0 = calm, +1 = right, -1 = left (exe Challenge+0x70)
};
// Called each tick from the first wave index 8 on. theRoll1500 = Rand(1500), used only when a gust happens.
// Returns the message to show: 0 none, 1 "wind rises ->", 2 "changed ->", 3 "changed <-".
int GaleTick(GaleState& theState, int theRoll1500);
// Plant drift for one blink tick: theRoll = Rand(36 - wave). Returns the px to add to mX (0 or +-1).
int GaleDrift(int theDir, int theRoll);

// 0x7b Frequency Switch (hook 104 -> 0x7a7c71): f 0 -> 1 at the first wave, then 1 <-> 2 every 400..600 cs.
struct FrequencyState {
	int f = 0;      // 0 off, 1 plants hidden, 2 zombies hidden (exe Challenge+0x70)
	int timer = 0;  // exe Challenge+0x58
};
void FrequencyTick(FrequencyState& theState, int theCurrentWave, int theRoll201);

// 0x7c Light and Mist (hook 135, 0x7a8351): fog radius of a sun coin (engine CoinType: SUN 4 -> 2, SMALLSUN 5 -> 1,
// LARGESUN 6 -> 3; else 0) and the cell row of its centre y ((y - 90) / 85, C division).
int LightSunRadius(int theCoinType);
int LightSunRow(int theCenterY);

// 0x65 Art Is an Explosion (hook 319, 0x7a45c0): coins dropped by an exploded plant from two Rand(4) rolls; the
// second roll is only made when the first paid. Returns {large sun motion or -1, sun motion or -1}; motion 2 = FROM_PLANT,
// 3 = COIN.
std::array<int, 2> ArtExplosionCoins(int theRoll1, int theRoll2);

// 0x72 Lone Wolf (hook 596 -> 0x7984c1): next seed of the moved plant after a head drops, from the sun after the add.
// Engine SeedType: Peashooter 0 -> Snow Pea 5 (100) -> Repeater 7 (750) -> Threepeater 18 (2000) -> Gatling 40 (5000).
// Returns theSeed when no upgrade happens (at most one step per head).
int LoneWolfUpgrade(int theSeed, int theSun);

// 0x6a Downstream (hook 679, 0x796560): new x of a bitten plant (theRoll41 = Rand(41)) and whether it falls off.
struct DownstreamPush {
	int x = 0;
	int col = 0;
	bool offLawn = false;
};
DownstreamPush DownstreamBite(int theX, int theRoll41);

// 0x68 War of Attrition (0x7995a6): after a wave spawns, the lawn flips when the new mCurrentWave is even and not 0.
bool AttritionFlips(int theCurrentWaveAfterSpawn);
// The mushrooms of the exe's IsNocturnal (0x794ac0): seeds 8, 9, 10, 12, 13, 14, 15, 24, 31, 42.
bool AttritionNocturnal(int theSeed);

// ---- second pass (2026-11-23) ----

// 0x66 Sun Storm. Hook 95 (0x7990d0): the sky-sun countdown after the vanilla roll t: min((unsigned)t >> 1, 450).
int SunStormCountdown(int theVanilla);
// Hook 208 (0x798721): a charged large sun clicked while falling explodes around (round(mPosX)-30, round(mPosY)-30):
// zombies with |mY - cy| <= 120, plants with |mY - cy| <= 100, both when mX - 125 <= cx <= mX + 130; 270 damage.
bool SunStormHitsZombie(int theCx, int theCy, int theZombieX, int theZombieY);
bool SunStormHitsPlant(int theCx, int theCy, int thePlantX, int thePlantY);
int SunStormBlastDamage();

// 0x6d Night Terror. Hook 462 (0x7962d5): 25 permanent craters; rows 1..4 then become dirt (zombies only in row 0).
bool NightTerrorCrater(int theCol, int theRow);
// Hook 604 (0x7989c0, Zombie::UpdateZombieWalking forward branch): x = mX, y = mY; may change theRow (mRow).
enum class TerrorMove { Walk, Hold, WalkRight };
TerrorMove NightTerrorStep(int theX, int theY, int& theRow);
float NightTerrorYStep(float theGap);       // hooks 616/617: 0.1875 px when the gap is >= 1, else the gap
int NightTerrorHp(int theHp, int theWave); // hook 517 (0x7a3081): hp * (mCurrentWave + 4) >> 2

// 0x72 Lone Wolf (0x798629): mLaunchRate of the moved plant: 50, Gatling Pea 96, Repeater 64; head sun = value x 5.
int LoneWolfLaunchRate(int theSeed);
int LoneWolfKillSun(int theZombieValue);

// 0x7f / 0x90 Trick of Tricks (hooks 296, 297): a Squash smash that hit no zombie with a head counts; from 15 on every
// such miss drops a money bag. Hook 96: fewer than 15 when the final wave comes = lost. Hook 234: HUD x.
inline constexpr int kTricksToWin = 15;
inline constexpr int kTrickFlags = 4;
struct TrickMiss {
	bool counted = false;
	bool bag = false;
};
TrickMiss TrickOfTricksSmash(int& theCount, int theHits);
bool TrickOfTricksLost(int theCount);
int TrickHudX(int theCount); // 660, 645 from 10

// 0x88 Territorial (hooks 38, 168, 174): the grid byte is 0 or seed + 1 (Imitater never claims); claimed cells take only
// that seed, Lily Pad, Grave Buster, Pumpkin and the mod seeds 0x5b / 0x6e.
int TerritorialStore(int theStored, int theSeed);
int TerritorialSeed(int theStored); // -1 = unclaimed
bool TerritorialRefuses(int theClaimedSeed, int theSeed);

// 0x8b Bullet Carnival (hook 165 -> 0x7b8a66). Engine ProjectileType values 0..12.
int CarnivalTier(int theType);      // table 0x8f3464; -1 = not eatable
int CarnivalGoal(int theTier);      // table 0x8d3464: score to grow out of theTier
int CarnivalTypeOfTier(int theTier); // first type with that tier, -1 none (0x7b8bbb)
int CarnivalEatScore(int theTier);  // (tier + 1)^2
int CarnivalWeight(int theType, int thePlayerTier); // 1, or 800 - 80 |d| when d = tier - (player + 1) < 3
bool CarnivalSpawnEdge(int theRoll900);             // x = roll - 50 outside -16..809
struct CarnivalSpawn {
	int x = 0, y = 0;
};
CarnivalSpawn CarnivalSpawnPlace(int theRoll900, int theRollY); // theRollY = Rand(680) on an edge, else Rand(60)
struct CarnivalRect {
	int x = 0, y = 0, w = 0, h = 0;
};
CarnivalRect CarnivalBulletRect(int theType, int x, int y, int w, int h); // 0x7b8e0b
bool CarnivalOverlap(const CarnivalRect& a, const CarnivalRect& b);
inline constexpr int kCarnivalWinType = 11; // Cob Cannon shell

// 0x91 All the Same One (also 0x73, 0xc030e0 -> 0x798fdf): the seed packets 1..5 get (Imitater -> its copy).
int AllTheSameSeed(int thePacketType, int theImitaterType);
inline constexpr int kAllTheSameSun = 0x70000000; // hook 471: sun every update, shown as "Unlimited"

// 0x92 Elite Zombies' Challenge (hook 90, 0xc05074): a wave slot's type; -1 ends the wave unless the Zombie Sea is on.
inline constexpr int kEliteChallengeZombie = 8; // Dancer (0xc05142)
int EliteChallengeSlot(int theSlotType, bool theZombieSea);

// 0x71 The Counter (hook 46 chain 0x7a7532): no planting once Board::mPlants holds 6.
bool CounterRefuses(int thePlantCount);

// 0x76 Schrodinger's Zombie (hook 725, 0x7a6811): the dying zombie of these types never starts to fade.
bool SchrodingerKeepsCorpse(int theZombieType);

// 0x7d Random Vasebreaker (hook 187 -> 0x7a8611): Rand(65) per vase; columns 0-1 stay empty.
enum class VaseKind { Sun, Zombie, Seed };
struct VaseRoll {
	VaseKind kind = VaseKind::Seed;
	int zombie = -1;
};
VaseRoll RandomVaseRoll(int theRoll65);
// 0x7b2350 (EAX = 1): s = Rand(128) is re-rolled while it is 48, 51, 53..79, or a mod seed whose row is empty.
bool RandomVaseSeedOk(int theSeed, bool theModRowExists);

// 0x8d Glove War 3 (Board::InitLevel 0xc004c8..0xc005cc; packet click 0xc007d0..0xc008ce).
struct GloveWar3Spot {
	int seed = 0, packet = 0, col = 0, row = 0;
};
GloveWar3Spot GloveWar3Start(int theIndex); // 0..9
int GloveWar3Toggle(int theSeed);           // T < 0x30 ? T + 0x50 : T - 0x50
int GloveWar3NoPlantRefresh(int theRefreshTime); // no plant of the other form on the lawn: + 3000

// 0x8e Brother Zeng and WASD (hooks 138 0xc00bd0, 99 0xc010c0, 275 0xc00a60, 396 0xc01110).
void ZengStep(int theKey, int& theCol, int& theRow); // 'W' 'S' rows, 'A' 'D' columns (the caller checks the cell)
// Port addition (the phone has no W/A/S/D): a swipe is one step its way; a tap on another lawn cell is one step toward
// it, along the longer axis (the row on a tie). The key to feed ZengStep, or 0 (own cell, off the lawn).
int ZengTouchKey(int thePlantCol, int thePlantRow, const Touch& theTouch, int theTapCol, int theTapRow);
bool ZengElementWave(int theCurrentWave);            // mCurrentWave == 9: the Gloom-shroom becomes the Element-shroom
bool ZengPuffs(int theCurrentWave);                  // from wave 20 the Element-shroom also fires a homing puff
inline constexpr int kZengPuffDamage = 80;

// 0x8c I Am Luck: an Imitater with no copy picks Rand(128) until the exe's byte table 0xc003ce allows it (hook 348,
// global in the mod); in 0x8c its cost is taken from the sun. The Imp card is a Rand(33) zombie (hook 191); the Zomboss
// has 500 HP (hooks 505/506).
bool ImitaterTableAllows(int theSeed);
inline constexpr int kIAmLuckImpTypes = 33, kIAmLuckBossHp = 500, kIAmLuckColumns = 9, kIAmLuckImitaters = 45;

// 0x69 Visual Disorder (hook 103, 0x798ca8): every tick Color::White's alpha = 40; from mCurrentWave 12, when
// (mMainCounter & 0x7f) == 0, its r, g, b become Rand(255) each.
inline constexpr int kVisualDisorderAlpha = 40;
bool VisualDisorderRecolours(int theCurrentWave, int theMainCounter);

// The "item mode" of the Glove Wars (0x7aaf60 -> 0xc006d0 -> 0x7aaf87): 1 = 0x6b, 2 = 0x87, 4 = 0x8d, 3 = 0x90, else 0.
// The shovel becomes the Zen glove, which moves lawn plants (hooks 73, 76, 77, 103, 132, 133, 479).
int ItemModeOf(int theMode);
int GloveRenderOrder(int theRow, int theX, int theSeed); // row*10000 + 302805 - x; Lily Pad / Flower Pot -5, Pumpkin +5
bool ItemModeHidesSeed(int theItemMode, int theSeed);    // hooks 438 / 452: 40..47 in item modes 1, 2, 4
bool GloveRefusesColumn(int theItemMode, int theGridX);  // hook 173 (0xc03210): item mode 3 only on columns 0..3

// 0x8f Tall-nut's Probe (Board::InitLevel 0xc01d00, ScaryPotterOpenPot hook 189 -> 0xc01e00).
struct Cell {
	int col = 0, row = 0;
};
// 0x8d, port rulings: the exe's no-plant branch AddPlant(0, 0) has no CanPlantAt, so it could stack a second plant on
// (0, 0); here (0, 0) if free, else the first free cell down column 0, then column 1, ...; {-1, -1}: refused.
// theFree[GloveWar3CellIndex(col, row)] = CanPlantAt is OK for the packet's form.
int GloveWar3CellIndex(int theCol, int theRow); // col * 5 + row
Cell GloveWar3PlantCell(const std::array<bool, 45>& theFree);
int GloveWar3SunAfter(int theSun, int theCost);  // never below 0
inline constexpr Cell kProbeSeedVase{0, 0};  // seed 0x76 Boss Flower
inline constexpr Cell kProbeBossVase{8, 4};  // zombie 0x1f Tall-nut head, marked 0x134100
inline constexpr int kProbeSeed = 0x76, kProbeZombie = 0x1f, kProbeMarker = 0x134100;
inline constexpr int kProbeBossBody = 150000, kProbeBossHelm = 120000, kProbeBossShield = 30000;
inline constexpr float kProbeBossScale = 1.95f;
inline constexpr std::array<Cell, 10> kProbeOpenCells{{{7, 1}, {7, 2}, {7, 3}, {7, 4}, {8, 1}, {7, 0}, {8, 0}, {8, 2}, {8, 3}, {8, 4}}};

// ---- V2 (the research notes (v2-plant-mode-contradictions)) ----
// Board::PickZombieWaves (hook 21, 0x7a7836): 0x6e x7 and 0x7e x((theWave >> 3) + 1), then vanilla Little Trouble's x4
// tail (0x409960) instead of the other multipliers; any other mode keeps thePointsAfter.
int ModeWavePoints(int theMode, int thePointsBefore, int thePointsAfter, int theWave);
// Zombie::ZombieInitialize (hook 516, 0x7a7b00): 0x6e also runs vanilla's Little Trouble block (scale 0.5, HP / 4).
bool ModeLittleZombies(int theMode);
// 0x6b Glove War, chooser closed (hook 462 chain -> 0x7a4c50): bank packet i is planted at column i (row 0) for i < 5,
// else column i - 5 (row 1); an Imitater packet plants its copy, an empty one a Flower Pot; the bank becomes Blover +
// Cherry Bomb.
void GloveWarStartCell(int theSlot, int& theCol, int& theRow);
int GloveWarStartSeed(int thePacketType, int theImitaterType);
inline constexpr std::array<int, 2> kGloveWarBank{27, 2};
// 0x80 Limited Supply, Plant::Die (hook 361, 0x7a8966): the first packet not recharging whose seed is the dead plant's
// (an Imitater packet: its copy, for a plant that came from an Imitater) starts its recharge. -1 = none.
struct LimitedPacket {
	int type = -1;
	int imitaterType = -1;
	bool refreshing = false;
};
int LimitedSupplyPacketToRestart(const std::vector<LimitedPacket>& thePackets, int thePlantSeed, int thePlantImitater);
// 0x8a Invisible Guardian, chooser closed (0x798f01): 2n swaps of two different Rand(n) packets (types resolved for the
// Imitater), then every card but Sunflower, Sun-shroom and Twin Sunflower is hidden (0x798f90).
template <typename Rand>
void InvisibleGuardianShuffle(std::vector<int>& theTypes, Rand&& theRand) {
	const int n = static_cast<int>(theTypes.size());
	if (n < 2)
		return;
	for (int k = 0; k < 2 * n; k++) {
		int i = theRand(n), j = theRand(n);
		while (i == j) {
			i = theRand(n);
			j = theRand(n);
		}
		std::swap(theTypes[i], theTypes[j]);
	}
}
bool InvisibleGuardianCardShown(int theSeed);

} // namespace pvzu::unnamed
