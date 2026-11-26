// PVZU: Unnamed 1.3beta2 level 70 (7-10), the giant Newspaper Zombie boss, as pure rules (rolls are passed in).
// Evidence: the research notes (w6-w7-and-new-content) section 3 and pvz-unnamed-pack/SOURCES-level70-boss.md (exe addresses per
// rule; the attack loop 0x7ab000 and its helpers were read for this port, the study had only the spawn and the phases).
// Every constant comes from data/rules-level70-boss.json (keys "level70Boss.*"); with no key a rule is off (vanilla).
// The engine guards each call with GameData::Get().Feature("level70Boss") (Mod/Engine/Level70Boss.h).
#pragma once

#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace pvzu::unnamed {

namespace boss70 {
constexpr int kVariant = 0x7F;      // exe Zombie+0x134 of the boss form (0x7a07f8); ZombieState.variant
constexpr int kNewspaper = 5;       // ZombieType of the boss (0xc11030 pushes 5)
constexpr int kPhaseReading = 29;   // ZombiePhase PHASE_NEWSPAPER_READING (exe 0x1d)
constexpr int kPhaseMaddening = 30; // PHASE_NEWSPAPER_MADDENING (0x1e)
constexpr int kPhaseMad = 31;       // PHASE_NEWSPAPER_MAD (0x1f)
} // namespace boss70

// ---- the level and the trigger (0x7a07a0, reached right after Board::SpawnZombieWave, .text 0x413fdf) ----
int Boss70Level();                                   // 70; 0 = no rules (vanilla)
bool Boss70IsLevel(int boardLevel);                  // exe 0x7aa100 tests Board::mLevel only
// The exe tests mCurrentWave == 10 after the spawn; the engine hook runs before it, so the wave being sent is 9.
bool Boss70Triggers(int boardLevel, int waveBeingSent);

// ---- the boss (0xc11030, 0x7a07f8..0x7a0884) ----
struct Boss70Spawn {
	int row = 2, col = 8;   // Challenge::IZombiePlaceZombie(5, col 8) with row 2
	float scale = 1.0f;     // +0x11c = 3.0
	int hp = 0;             // body HP = max = 150000, no shield (the Almanac's 50000 + 100000 split is not in the code)
	int firstAttackCs = 0;  // +0x130 = 750: the attack timer
};
Boss70Spawn Boss70SpawnRules();
struct Boss70Rect {
	int x = 0, y = 0, w = 0, h = 0;
	bool operator==(const Boss70Rect&) const = default;
};
Boss70Rect Boss70HitRect(Boss70Rect theRect); // width x3, height x2, y moved up by the old height
float Boss70PinX();                          // hook 603 (0x7a0c62): the boss never walks, mPosX = 640

// "Boss class" (0x7aa320): max HP above 120000, or Dr. Zomboss. Butter, the boss's own picks and the Sniper use it.
bool IsBossClass(int bodyMaxHealth, int zombieType);
// The live boss form (0x7aa130): a Newspaper with variant 0x7f on level 70.
bool IsBoss70(int zombieType, int variant, int boardLevel);

// ---- darkness and light (0x7aa7a0, 0x7aa8a0) ----
enum class Boss70Light { DARK, DAY };
int Boss70Background(Boss70Light theLight);  // BackgroundType: 1 night / 0 day
// Plant::SetSleeping argument when the light changes (0x7aa680): dark puts day plants to sleep and wakes mushrooms,
// day does the opposite.
bool Boss70PlantSleeps(Boss70Light theLight, bool nocturnal);
// One random zombie per plant on the lawn (Board+0xbc = mPlants.mSize), each Rand(33) re-rolled over the excluded types
// (Snorkel, Bobsled, Dolphin, Yeti, Boss), placed in row Rand(5), column 8.
int Boss70ReinforcementRollRange();
bool Boss70ReinforcementAccepted(int roll);
int PickBoss70Reinforcement(const std::function<int(int)>& rand);
int Boss70ReinforcementRows();
int Boss70ReinforcementCol();
// The new seed packets (0x7aa750 + 0xc11070): the first min(bank size, 10) packets get these types, refresh time 0.
// Mod-only seeds the engine does not have (>= theEngineSeeds) use the pack's stand-in, or -1 (an empty packet).
std::vector<int> Boss70Bank(Boss70Light theLight, int theNumPackets, int theEngineSeeds);
int Boss70StandIn(int theSeed, int theEngineSeeds); // -1 when it has none
// Light (0x7aa8a0): one random non-empty packet per live elite zombie becomes empty, at most 4 and the bank size.
bool Boss70CountsAsElite(int variant, bool bossClass); // variant 1..0x7e, not boss class
int Boss70LightRemovals(int theElites, int theNumPackets);

// ---- phases (hook 707 at Zombie::TakeDamage, 0x7a0a70; HP is read before the hit) ----
enum class Boss70Text { NONE, DARK_HUNT, PHASE2, PHASE3, LIGHT, LASER, RAY_GUN };
struct Boss70Phase {
	bool giveShield = false;  // shield type 2 (newspaper) with 1 HP: the next hit breaks it, the boss goes mad
	int shieldHp = 0;
	int phase2MaxHp = -1;     // set max HP (and the player's sun) and show PHASE2
	int phase2Sun = -1;
	int phase3MaxHp = -1;     // set max HP, the boss form ends (variant 0), show PHASE3
	bool operator==(const Boss70Phase&) const = default;
};
Boss70Phase Boss70BeforeHit(int phase, int hp, int maxHp);
// Hook 101 (Board::UpdateProgressMeter): the boss's lost HP while it lives, 150 once dying, 0 before wave 10;
// -1 = the vanilla meter (wave >= 10 and no boss).
int Boss70ProgressWidth(bool bossFound, bool bossAlive, int hp, int maxHp, int currentWave);
// Draw (hooks 650/651): the shield render group is drawn even with no shield; the hands go over it except while
// maddening or mad.
bool Boss70DrawsShieldGroup();
bool Boss70HandsOverPaper(int phase);

// ---- the attack loop (0x7ab000, every update of the live boss) ----
enum class Boss70Attack { DARK = 0, LIGHT = 1, LASER = 2, SKY = 3, RAY_GUN = 4, FRENZY = 5, REST = 6 };
const char* Boss70AttackName(int theAttack);
// The loop does nothing while the boss fades, is buttered, frozen, hypnotised, headless, dead, dying, burned, mowed or
// maddening.
bool Boss70TickRuns(bool fading, bool buttered, bool iceTrapped, bool hypnotised, bool hasHead, bool dead, int phase);
int Boss70AttackRollRange(bool mad);                   // Rand(4) reading, Rand(7) mad
int Boss70NextTimer(int roll, bool mad);               // Rand(300) + 1200, x2/3 when mad
int Boss70TimerRollRange();
int Boss70RowRollRange();                              // the boss moves to another row Rand(5) each cycle
// Mad only, before the pick: a column of one type (Rand(5) -> Door, Dancer, Zomboni, Ladder, Gatling head) in rows
// 4..0 at column 8 (0x7aab60): x1.5 size, x4 HP, gone after 400 cs; the Door one also wears a 4400 bucket.
int Boss70ColumnRollRange();
int Boss70ColumnType(int roll);
float Boss70ColumnScale();
int Boss70ColumnHpFactor();
int Boss70ColumnFadeCs();
int Boss70ColumnDoorBucketHp();
// RAY_GUN (mode 4, mad only): four melons per 50 cs at an angle that sweeps +-60 degrees; they fly left at -1 px/cs
// and hit plants (damage-range flags 0, hook 380).
bool Boss70RayGunFires(int timer);
float Boss70RayGunAngle(int timer);                    // sin(timer x 0.0314) x 1.0472 rad
float Boss70RayGunSpeedX();
int Boss70RayGunDx();
int Boss70RayGunDy();
int Boss70RayGunProjectile();                          // ProjectileType 3 (melon), motion 7
// DARK when mad (0x7aa9d0): every plant loses round(maxHP x f) HP, f = (p/50 + 50/p) x 0.08 with p = min(plants, 50);
// the boss heals min(total >> 4, 1800).
double Boss70DrainFactor(int plants);
int Boss70DrainHp(int plantMaxHealth, double factor);
int Boss70DrainHeal(int total);
int Boss70DrainFlashCs();
// LIGHT when mad (0x7aac50): the boss eats every other non-boss zombie within 1 row and 80 px; each heals it by
// min(HP sum >> 3, 150); eating any starts the board-wide freeze (exe 0x800100/0x800104) for plants x eaten x 10.
bool Boss70EatReaches(int bossRow, int bossX, int zombieRow, int zombieX, int zombieRectWidth);
int Boss70EatHeal(int zombieHpSum);
int Boss70EatFreeze(int plants, int eaten);
// After a mad DARK or a mad LIGHT that ate, packets 0 and 1 (if not empty) become this seed (exe 0x59, a mod plant).
int Boss70MadPacketSeed(int theEngineSeeds);
// SKY (mode 3, 0x7aaa70): two sky drops; then, if zombies < plants, twelve bungees at these cells; mad: graves on
// columns 7..8 of every row and the graves rise (0x7aa1d0, 0x412ce0).
std::vector<std::pair<int, int>> Boss70BungeeCells(); // (row, col)
int Boss70GraveFirstCol();
int Boss70GraveLastCol();
// Sprout cards (seed 0x33, or an Imitater of one) are planted as an Imitater of Rand(48) (0x7a071a).
bool Boss70SproutRandom(int seed, int imitaterOf);
int Boss70SproutRollRange();
int Boss70SproutSeed();                                // the plant type it becomes (Imitater)
// Hook 631 (Zombie::UpdatePlaying): butter of 30000 cs or more is cut at once on a boss-class zombie; on a maddening
// Newspaper it hypnotises it instead. false = the vanilla countdown.
struct Boss70Butter {
	bool cut = false;
	bool hypnotise = false;
};
Boss70Butter Boss70ButterTick(int butteredCounter, int bodyMaxHealth, int zombieType, int phase);
// Gatling head the mad boss holds in place of its paper (0x7aa560: REANIM_GATLINGPEA on "Zombie_paper_paper").
int Boss70HeadAnimRate();

std::string Boss70TextKey(Boss70Text theText);         // "PVZU_BOSS70_..." (strings/en-corrections-level70-boss.txt)
const char* Boss70TextFallback(Boss70Text theText);    // plain English when the pack has no text
int Boss70TextCs();                                    // each message shows 250 cs

// Every rules key the boss reads (for the pack consistency test).
const std::vector<std::string>& Boss70RuleKeys();

} // namespace pvzu::unnamed
