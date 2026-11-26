// PVZU: Unnamed 1.3beta2 special levels (Plan 7 Task 3a): 6-5 (55) Mushroom-garden shooter, 7-5 (65) star snake and the
// hidden 71 / 73 rules, as pure functions. 6-10 (60) needs only the table's introTimer (Mod/Unnamed/LaterLevels.h); 7-10
// (70, the giant Newspaper boss) is another task's. Evidence: the research notes (w6-w7-and-new-content) section 2.3 and
// pvz-unnamed-pack/SOURCES-special-levels.md (the exe addresses, and what this task re-read: e.g. the Cabbage-pult sits in
// row 0, its "+0xc8 = 100" is a Magnet-shroom slot and not its health, 71's own wave branch is a x112 budget, 73 plants
// whole columns). Constants come from data/rules-special-levels.json (keys "specialLevels.*"); a missing key plays that
// rule like vanilla. The engine guards every call with the pack feature "specialLevels" (Mod/Engine/SpecialLevelHooks.h).
#pragma once

#include "Mod/Unnamed/LaterLevels.h"

#include <cstdint>
#include <utility>
#include <vector>

namespace pvzu::unnamed {

// A direction from a key or a touch gesture (screen y grows downwards).
enum class Steer { None, Up, Down, Left, Right };

// Board::GetNumSeedsInBank (hook 142 -> 0x799717 / 0xc030b0): 55 gives 4 slots, 65 gives 3; theVanilla otherwise.
int SpecialSeedSlots(SpecialLevel theSpecial, int theVanilla);

// ---- 6-5 (55): Mushroom-garden shooter ----
// SeedChooserScreen::OnStartButton (hook 462, 0x79842f): sun 0, bank 0 packets, one plant (Board::AddPlant(row EAX, col,
// seed)), mLaunchRate 50. seed -1: no rule (vanilla).
struct ShooterSetup {
	int seed = -1;
	int row = 0;
	int col = 0;
	int launchRate = -1;
	int startSun = -1;
};
ShooterSetup ShooterStart();
int ShooterMessageTime();        // Board::StartLevel (hook 160, 0x79a000): "use Up / Down" message, cs; -1 none
int ShooterFirstWaveCountdown(); // same hook: mZombieCountDown; -1 = vanilla
// Zombie::DropHead (hook 596, 0x7984c1): sun for a zombie that loses its head (ZombieValue x 5); 0 without the rule.
int ShooterKillSun(int theZombieValue);
// Same hook, after the sun: the plant that replaces theSeed in place (Cabbage -> Melon at >= 500 sun, Melon -> Winter
// Melon at >= 2000), or -1. One step per kill.
int ShooterUpgrade(int theSeed, int theSun);
int ShooterLaunchRate(int theSeed); // mLaunchRate of a shooter plant (start or upgrade); -1 = keep the plant's own
// Board::KeyDown (hook 138, 0x79b0a0): the arrow keys (KeyCode 0x26 up, 0x28 down).
Steer SteerFromArrow(int theKeyCode);
int ShooterStepRow(int theRow, Steer theDir, int theNumRows); // one row, never off rows 0..theNumRows-1
// Zombie::ZombieInitialize (hook 517 chain, 0x799787, at the point where the max HP are stored): types below
// shooter.halfHpBelowType (26: every non-ZomBotany type) spawn with half body, helmet and shield HP; every max HP stays.
bool ShooterHalvesHp(int theZombieType);
struct ZombieHp {
	int body = 0, bodyMax = 0, helm = 0, helmMax = 0, shield = 0, shieldMax = 0;
};
void ShooterHalveHp(ZombieHp& theHp);

// ---- touch (the phone has no arrow or W/A/S/D keys; a port addition, not in the exe) ----
struct Touch {
	bool swipe = false;
	Steer dir = Steer::None; // swipes only: the longer axis
};
// A press-to-release move of at least touch.swipeMin board pixels is a swipe, anything shorter a tap.
Touch ClassifyTouch(int theDx, int theDy);
// 6-5: the row the plant goes to. A tap on another lawn row jumps there (the same as that many arrow presses); a swipe
// up / down is one arrow press; anything else keeps thePlantRow.
int ShooterTouchRow(int thePlantRow, const Touch& theTouch, int theTapRow, int theNumRows);

// ---- 7-5 (65): star snake ----
Steer SteerFromKey(int theKeyCode); // 'W' 'A' 'S' 'D'
// The head (projectile slot 0): flying level (vy == 0) only W / S turn it, otherwise only A / D; the new velocity is
// starSnake.speed (3) on one axis. false: the key does nothing.
bool SteerHead(Steer theDir, float& theVx, float& theVy);
void ApplyTurn(Steer theDir, float& theVx, float& theVy); // a replayed turn (hook 401, 0x7a2014..0x7a2061)
// The exe's turn table at 0x810000: entry i = (head's age when it turned, key). The head writes entry <its turn count>;
// every star (type 7) with Projectile+0x84 == 0 replays entry <its own count> when its age equals the recorded age.
// The entries live in a flat (age, Steer) vector: the board's saved BoardState::starTurns, or the object's own one.
class StarTurns {
public:
	StarTurns() : mTurns(&mOwn) {}
	explicit StarTurns(std::vector<int32_t>& theStore) : mTurns(&theStore) {}
	StarTurns(const StarTurns&) = delete;
	StarTurns& operator=(const StarTurns&) = delete;
	void Reset() { mTurns->clear(); }
	bool Record(int theIndex, int theAge, Steer theDir); // false past starSnake.maxTurns
	bool TurnAt(int theIndex, int theAge, Steer& theDir) const;

private:
	std::vector<int32_t> mOwn;
	std::vector<int32_t>* mTurns;
};
// The head (a star with Projectile+0x84 != 0) outside x 0..899, y 40..599 loses the level (scene 4, hook 401 0x7a206d).
bool StarOutsideField(int theX, int theY);

// 7-5 and mode 0x89 Glutton Star (the research notes (glutton-star)). Start, SeedChooserScreen::OnStartButton (hook 462 ->
// 0x7a2230): three stars flying right, head first (so it is item 0 of the projectile array), each with its own age.
struct SnakeStar {
	int x = 0, y = 0, age = 0;
};
std::vector<SnakeStar> SnakeStartStars(); // empty = no snake (rules missing)
int SnakeStartRow();
int SnakeStartRenderOrder();
std::vector<int> SnakeBankSeeds(); // the packets after the chooser (Starfruit, Sunflower, Wall-nut); empty = keep
// SeedPacket::MouseDown (hook 467 -> 0x7a20f9): a click on this packet buys a star at the tail instead of picking the
// plant up; -1 = no such packet.
int SnakeGrowSeed();
// The new star: starSnake.growGapTicks behind the tail along its velocity (x87 fistp rounding), that much younger.
SnakeStar SnakeGrowBehind(int theTailX, int theTailY, float theTailVx, float theTailVy, int theTailAge);
int SnakeGrowRenderOrder();
int SnakeGrowShadowYOffset(); // mShadowY = mY + this
// Projectile::Die cave 0x6511e3 with hook 401 resetting the counter each tick: a snake star lives through every hit.
bool SnakeStarsPierce();

// ---- hidden 71 (Hidden1) / 73 (Hidden3) ----
// Board::PickZombieWaves (hook 21, 0x95c0c0): 71 multiplies the wave's points by 28 and runs the x4 tail (0x409960):
// thePointsBefore x 112 instead of thePointsAfter (the vanilla / Unnamed multiplier result).
int SpecialWavePoints(SpecialLevel theSpecial, int thePointsBefore, int thePointsAfter);
// Board::UpdateSunSpawning (hook 93, 0x95c140): 71 plants a Blover at column 7, row -1 every 1000 ticks.
bool AutoPlantDue(SpecialLevel theSpecial, int theCounter);
// V2: 72 and 73 have their own multipliers too (0xc13070 x20, 0xc13300 x15, then the x4 tail): keys
// specialLevels.hidden1|hidden2|hidden3.wavePointsMultiplier (112 / 80 / 60).
// V2: Plant::Update (hook 316, 0x463ed7 -> 0xc13150 / 0xc1301a): on 72 every plant on the board takes the bite function
// 0x950000 (plant, 1, 1) when Board mMainCounter % 6 == 1 (1 HP per 6 ticks = 100 HP per 600 cs).
bool PlantDrainDue(SpecialLevel theSpecial, int theMainCounter);
int PlantDrainDamage();
// The bite function's own exits (0x950005..0x95007d): asleep -> bitten; state 1 -> bitten; states 3..7 -> spared;
// outside I, Zombie levels seeds 2, 14, 15, 20, 27, 0x52, 0x6b -> spared. theSeed: the mod seed when the plant has one.
bool PlantDrainSkips(int theSeed, int theState, bool theAsleep, bool theIZombieLevel);
struct AutoPlant {
	int seed = -1, col = 0, row = 0;
};
AutoPlant HiddenAutoPlant();
int SpecialHelmHp(SpecialLevel theSpecial, int theHelmHp); // ZombieInitialize (hook 515, 0x95c110): 71 doubles helmets
// SeedChooserScreen (hook 425, 0x95c180 / 0xc13330): 71 bars Blover, 73 bars eleven instant / utility seeds.
bool SpecialSeedBanned(SpecialLevel theSpecial, int theSeed);
// Board::MouseDownWithPlant / CursorObject::Draw (hooks 71, 217; 0xc13490, 0xc13440): on 73 (board and profile level)
// a plant is planted in its whole column, like "Column Like You See 'Em", while no zombie is on the lawn and the lawn
// holds at most 29 plants (counted after the clicked plant went in).
bool SpecialColumnPlanting(SpecialLevel theSpecial, int theProfileLevel, int thePlants, int theZombies);
// Board::PickZombieType (hook 43, 0xc13520): Adventure 73 (board and profile) reads 5 as the Newspaper's first wave.
int SpecialFirstAllowedWave(SpecialLevel theSpecial, bool theAdventure, int theProfileLevel, int theZombieType,
	int theVanilla);

} // namespace pvzu::unnamed
