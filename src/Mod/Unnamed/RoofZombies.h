// PVZU: Unnamed 1.3beta2 Roof zombies (Bungee, Ladder, Catapult, Gargantuar / Giga, thrown Imp) and their elite forms,
// as pure rules (rolls are passed in). Evidence per rule: the research notes (w5-roof) section 2 and
// pvz-unnamed-pack/SOURCES-roof-zombies.md (exe addresses). Every constant comes from data/rules-roof-zombies.json (keys
// "roofZombies.*") with the vanilla value as fallback, so a pack without a key plays that rule like vanilla.
// Wave values / pick weights (G7: Bungee 2, Ladder 3 / 2000, Catapult 3, Gargantuar 6) are in data/zombies.json.
//
// For the Dr. Zomboss port (boss bungees, study B8): RollBungee(roll, true) is the Jack-head roll of a boss bungee;
// the engine side wraps it as pvzu::engine::RoofBossBungeeScope (Mod/Engine/RoofEliteZombies.h).
#pragma once

#include "ConstEnums.h"
#include "Mod/Unnamed/Options.h"

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace pvzu::unnamed {

// Zombie+0x134 codes (ZombieState::variant) of the Roof forms.
namespace roofelite {
constexpr int WHEELBARROW = 0x16;  // Bungee: wheelbarrow head, 1200 HP, stays at the bottom, no planting in its 3x3
constexpr int JACK_BUNGEE = 0x17;  // Bungee: Jack-box head, grabs at once and leaves a popping Jack-in-the-Box behind
constexpr int SHOVEL = 0x18;       // Ladder: a shovel instead of the ladder (870 shield), digs up every plant it reaches
constexpr int POLE_HEAD = 0x19;    // Catapult: Pole Vaulter driver, shoots the front-most plant
constexpr int LADDER_PILE = 0x35;  // Ladder: 5 spare ladders, ladders every plant and calls a Bungee onto it
constexpr int GHOST_LADDER = 0x37; // Ladder: half see-through, teleports to the last unladdered plant, keeps its ladder
constexpr int STAFF = 0x106;       // Gargantuar / Giga: staff instead of the pole, throws random zombies, keeps throwing
}

bool IsRoofType(ZombieType type);   // Bungee, Ladder, Catapult, Gargantuar, Giga (the thrown Imp has no form here)
const char* RoofEliteName(ZombieType type, int variant);   // "none", "wheelbarrow", "jack-head", "shovel", ...

// ---- spawn rolls (hooks 488, 487, 498, 513) ----
// The elite option (exe 0x800258) reaches these rolls two ways: `setting` = the player's option, which self-patches
// the roll code (0x7b2000 "all" / 0x7b2100 "none" / 0x7b2200 default); `value` = what 0x800258 holds now, read
// directly by a few checks (the setting, or "all" while a staff Giga adds its thrown zombie, hook 565).
int BungeeRollRange(int setting = eliteoption::DEFAULT);   // Rand(12); "all": Rand(3) (0x796eb4 = 3), "none": 0
// roll < 3 passes: 0 / 2 -> wheelbarrow, 1 -> Jack head; forceJack (I, Zombie or a Zomboss bungee) -> Jack head.
int RollBungee(int roll, bool forceJack);
int LadderShovelRollRange(int setting = eliteoption::DEFAULT);   // Rand(4); "none": 0
int RollLadderShovel(int roll, int setting = eliteoption::DEFAULT); // == 2 -> shovel; "all": < 2 (0x796d2c `jge`)
// Only when the shovel roll failed; 0 when the value is "no elites" (0x95aa15); Rand(4), Rand(2) when the setting is
// "all" (0x95aa1f = 2).
int LadderSecondRollRange(int value, int setting = eliteoption::DEFAULT);
int RollLadderSecond(int roll, int plantCount); // 0 -> pile; 1 -> ghost with >= 5 plants on the board, else pile
int CatapultRollRange(int setting = eliteoption::DEFAULT);   // Rand(100); "none": 0 (the exe draws it and skips)
// < 50 -> pole head (< 83 when the setting is "all", 0x796e16); the rest up to 0x910000's bound (60; 100 with the
// setting "all", 0x910002) is the world-7 vase (World7Zombies) or, when the value is "all", the pole head (0x91009b).
int RollCatapult(int roll, int value, int setting = eliteoption::DEFAULT);
// The Gargantuar's vanilla pole-image roll aPoleHit = Rand(100) is reused: % 20 == 1 -> staff (5 %), "all elites" ->
// staff, "no elites" -> never.
bool GargStaffHit(int poleHit, int eliteOption);

// ---- hit points (hooks 488, 487; patches 0x522e87, 0x523e44) ----
int RoofBodyHp(ZombieType type, int variant, int vanilla);
int RoofShieldHp(ZombieType type, int variant, int vanilla);   // shovel: + 370 (its max stays 500, as the exe)

// ---- Bungee (hooks 527, 529, 46, 522, 524) ----
int BungeeBottomCounter(int variant, int vanilla);   // wheelbarrow never grabs (0x7fffffff), Jack head at once (0)
// CanPlantAt: a wheelbarrow at the bottom (not hypnotised, with its head) forbids the cells row +-1, mTargetCol +-1.
bool WheelbarrowBlocksCell(int bungeeRow, int bungeeTargetCol, int col, int row);
bool JackBungeeLeavesJack(int variant);
float JackBungeeJackX(float plantCellPixelX);        // GridToPixelX(plant col) - 30
// BungeeStealTarget: before the plant, a bungee takes a zombie of the other side standing on its target cell.
bool BungeeCarriesZombies();
bool BungeeCanCarry(bool bungeeHypnotised, ZombieType type, int bodyMaxHp, bool zombieHypnotised, bool zombieDead);
// The zombie's rect covers the column: col in [round(x / 80) - 1, round((x + w) / 80) - 1] (0x957680).
bool ZombieCoversColumn(int rectX, int rectWidth, int col);

// ---- Ladder (hooks 600, 601, 655, 722, 694, 702, 710, 672, 662, 619, 790, 647, 696) ----
// CanTargetPlant in the ladder phases: the shovel and the pile treat every plant as a ladder target (vanilla: nuts).
bool LadderTargetsAnyPlant(int variant);
// The pile's own CanTargetPlant (any phase): CHEW only where a ladder stands, LADDER anywhere without one;
// nullopt = the vanilla answer.
std::optional<bool> PileCanTarget(int variant, int attackType, bool cellHasLadder);
bool ShovelDigsUpPlant(int variant);           // plays FOLEY_USE_SHOVEL and kills the plant instead of a ladder
bool PileCallsBungee(int variant);             // Challenge::IZombiePlaceZombie(BUNGEE, col, row) on each ladder
bool KeepsShieldOnLadder(int variant);         // the ghost keeps its ladder after placing one
int PileSpares(int playerLevel);               // 5 spare ladders (11 from PlayerInfo level 60)
int PileLadderHp();                            // each new ladder 500 / 500
int PileBodyToShield(int variant, bool hasLadder, int bodyDamage);   // body damage also hits its ladder for half
bool PileOverflowsIntoNextLadder(int variant);  // damage left after a ladder breaks goes into the next one
bool PileBites(int variant, bool hasLadder);    // no bites while it carries a ladder
float LadderAnimRateMultiplier(int variant, bool hasLadder, bool eating);   // pile 1.6 / 3, ghost 2 while not eating
int GhostTeleportAge();                         // its 2nd tick on the board
// x = (max column of the plants in its row without a ladder + 1) * 80; maxColumn -1 (none) -> x = 0.
float GhostTeleportX(int maxColumnWithoutLadder);
int GhostBiteDamage(int variant, int plantMaxHp, int plantHp, int vanilla);   // max HP <= 1000: one bite; else 1
int GhostAlpha(int variant);                    // 255 = vanilla
bool SkipsShieldDamageImages(int variant);      // the shovel keeps its image (hook 696)

// ---- Catapult (hooks 544, 705, 793) ----
bool CatapultPrefers(int variant, int candidateColumn, int currentColumn);   // pole head: largest column, else smallest
struct FaithHit {
	int damage = 0;
	int bodyHp = 0;
	bool operator==(const FaithHit&) const = default;
};
// "Basketball Faith": while it has balls (not in Wall-nut Bowling), a hit that would bring the body to <= 190 locks it
// at 190 and only the damage's lowest bit gets through.
FaithHit CatapultFaith(int bodyHp, int damage, int ballsLeft, bool wallnutBowling);
bool CatapultTooToughToAsh(int ballsLeft);   // explosions take the 1800 damage path (then the lock), never "burned"

// ---- Gargantuar / Giga (hooks 565, 568; patches 0x527216, 0x527056) ----
bool GargThrowsAtAnyDistance();
float GargThrowDistance(float computed, float minimum);   // the clamp lost its jump: always the minimum
int StaffThrowRollRange();                                 // Rand(33), re-rolled
bool StaffThrowTypeAccepted(int roll);                     // 25 types, never Flag/Ducky/Snorkel/Bobsled/... /Boss
bool StaffKeepsThrowing(int boardZombies, bool hypnotised);   // stays in the throw while <= 50 zombies

// ---- thrown Imp (hook 630) ----
bool ThrownImpSkipsHouse(ZombiePhase phase);   // getting thrown / landing: no "reached the house"

// ---- art (study 7) ----
struct RoofArtImage {
	std::string track;
	std::string file;   // pack file name without extension
	bool operator==(const RoofArtImage&) const = default;
};
std::vector<RoofArtImage> RoofArtFor(ZombieType type, int variant);
// When any image file is missing the whole look is dropped (the base zombie is drawn; the form keeps its behaviour).
std::vector<RoofArtImage> ResolveRoofArt(ZombieType type, int variant, const std::function<bool(const std::string&)>& available);

// Every rules key the Roof zombies read (for the pack consistency test).
const std::vector<std::string>& RoofZombieRuleKeys();

} // namespace pvzu::unnamed
