// PVZU: Unnamed 1.3beta2 Night zombies (Newspaper, Screen Door, Football, Dancer) and their elite forms, as pure rules
// (rolls are passed in). Evidence per rule: the research notes (w2-night) section 2 and
// pvz-unnamed-pack/SOURCES-night-zombies.md (exe addresses). Every constant comes from data/rules-night-zombies.json
// (keys "nightZombies.*") with the vanilla value as fallback, so a pack without a key plays that rule like vanilla.
#pragma once

#include "ConstEnums.h"
#include "Mod/Unnamed/Options.h"

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace pvzu::unnamed {

// Zombie+0x134 codes of the Night forms (the research notes (zombies) Appendix A; 0 = none, as for Day).
namespace nightelite {
constexpr int TALLNUT_DOOR = 6;        // Screen Door
constexpr int TROPHY = 9;              // Dancer: trophy head, endless summons, 500 HP
constexpr int MOONWALKER = 10;         // Dancer: bald, moonwalks to the mower, never dances
constexpr int ALMANAC_PAPER = 0x2A;    // Newspaper: Almanac paper (only after level 60 or a finished adventure)
constexpr int KNIFE = 0x31;            // Newspaper: knife, 1500 HP, kills a plant at the first chew when mad
constexpr int GOLD_FOOTBALL = 0x102;   // Football: gold-dollar head, x1.5, squishes plants, graves raise Footballs
constexpr int RANDOM_SUMMONER = 0x104; // Dancer: summons random zombie types, moonwalks in for 30 s
}
// Zombie+0x14C codes (ZombieState::subVariant).
namespace nightsub {
constexpr int FOOTBALL_HEAD = 1;       // Newspaper: football head, x1.5 walk, paper soaks body damage while reading
constexpr int GREEN = 0x1A;            // Football: green armour, 300 + 3000 HP, faster when hurt
}

const char* NightEliteName(ZombieType type, int variant, int subVariant); // "none", "knife", "green+gold", ...
bool IsNightType(ZombieType type);   // Newspaper, Screen Door, Football, Dancer

struct NightForm {
	int variant = 0;
	int subVariant = 0;
	bool operator==(const NightForm&) const = default;
};

// ---- spawn rolls (study 2.1-2.4) ----
// n of the first Rand(n) of the type; 0 = no roll. Football rolls three times in a row (see FootballRolls).
// eliteOption = eliteoption::* (Mod/Unnamed/Options.h): NONE -> 0; ALL -> Newspaper Rand(4) (0xc05770), Dancer Rand(15)
// (0x79683c); the Screen Door keeps Rand(5) (option 2 patches its jump, 0x796166). Newspaper and Dancer read the
// option's current value, the Door's patch follows the setting.
int NightRollRange(ZombieType type, bool isIZombieLevel, int eliteOption = eliteoption::DEFAULT);
// Newspaper Rand(5): 0..1 football head, 2 Almanac paper (only when almanacAllowed, or always with ALL: 0x7a1a16),
// 3 knife, 4 plain.
NightForm RollNewspaper(int roll, bool almanacAllowed, int eliteOption = eliteoption::DEFAULT);
bool NewspaperAlmanacAllowed(int playerLevel, int finishedAdventures, bool isAdventure, int boardLevel);
NightForm RollDoor(int roll, int eliteOption = eliteoption::DEFAULT); // Rand(5) == 0 -> Tall-nut door (always with ALL)
// Football: Rand(4) (green), then Rand(13) (gold), then Rand(3) when not gold (the exe's world-7 black-armour roll,
// drawn and ignored below level 61).
struct FootballRolls { int green; int gold; int extra; };
int FootballGreenRange();
int FootballGoldRange();
int FootballExtraRange();
NightForm RollFootball(int greenRoll, int goldRoll);
bool FootballDrawsExtraRoll(const NightForm& f);
// Rand(60): 0..2 random summoner, 3..6 trophy, 7..14 moonwalker (trophy and moonwalker not in I, Zombie, except with
// ALL: 0xc043eb jumps past the I, Zombie check).
NightForm RollDancer(int roll, bool isIZombieLevel, int eliteOption = eliteoption::DEFAULT);
// Football with the elite option "all" (hook 489: 0xc12afd .. 0xc12b5f, 0x9dd37a .. 0x9dd419, 0xc12500, 0xc12c50).
// rand(n) = Rand(n), drawn in the exe's order. Profile level <= 60: Rand(16) < 13 -> green, then the gold Rand(13)
// (0 -> gold too) and, unless gold, the extra Rand(3); else gold, nothing more drawn. Profile level > 60: Rand(40):
// 0 -> gold; 1..13 -> green as above; 14..39 -> the gold Rand(13): != 0 -> black armour, == 0 -> gold, then the extra
// Rand(3) < 1 -> gold and black. black = the exe's black-armour art and HP (World7Zombies, feature world7Zombies).
struct FootballAllEliteForm { NightForm form; bool black = false; };
FootballAllEliteForm RollFootballAllElite(const std::function<int(int)>& rand, bool playerLevelAbove60);

// HP right after the type's init (body/helm/shield; vanilla = the engine's value).
int NightBodyHp(ZombieType type, const NightForm& f, int vanilla);
int NightHelmHp(ZombieType type, const NightForm& f, int vanilla);
int NightShieldHp(ZombieType type, const NightForm& f, int vanilla);
int DancerStartPhaseCounter(const NightForm& f, int vanilla);   // random summoner: 3000 cs moonwalk in
int DancerStartStored(const NightForm& f);                      // Zombie+0x12C: -1 (random follower type) for 0x104

// ---- movement ----
// Multiplier on the walking anim rate (UpdateAnimSpeed's _ground branch: speed follows it).
float NightWalkRateMultiplier(ZombieType type, const NightForm& f, int body, int bodyMax, int helm, int helmMax);
// Multiplier in ApplyAnimRate (all animations): the mad knife moves at 1/3.
float NightAnimRateMultiplier(ZombieType type, const NightForm& f, ZombiePhase phase);
bool DancerSkipsDancerUpdate(const NightForm& f);   // moonwalker
float MoonwalkVelX();
float MoonwalkAnimRate();

// ---- dancers (study 2.4; the summon wait and the re-summon of all four are from the exe, see SOURCES) ----
bool DancerSnapsWithoutCountdown();           // `jo` patch at 0x528CC2: snaps at the next frame-12 moment
bool DancerSkipsSummonCounterCheck(const NightForm& f);   // trophy: snaps at every frame-12 moment (endless)
bool DancerResummonsAll();                    // every summon makes 4 new backups and lets the old ones walk off
bool DancerCountsCarnival(const NightForm& f); // for Night levels (mPvzu.counter): every backup adds 1 to the 12-counter
bool SummonsRandomFollowers(const NightForm& f);
int RandomFollowerRollRange();                // Rand(33), re-drawn until accepted
bool RandomFollowerAccepted(int roll, int playerLevel);
bool DrawsWhite(ZombieType type, const NightForm& f);   // random summoner: reanim filter 2 (white)

// ---- eating ----
// nullopt = not a Night rule, use the Day/vanilla timing. rem = age % ticksBetweenEats.
std::optional<bool> NightBitesThisTick(ZombieType type, const NightForm& f, ZombiePhase phase, bool chilled, int rem);
int CheckIfPreyCaughtCallsPerTick(ZombiePhase phase);   // mad Newspaper 3, else 1
bool SkipsNormalBite(ZombieType type, const NightForm& f);   // knife: no 4 HP bites at all
// Chew effect on the bitten plant's flash countdown (Plant+0xB8): returns the new countdown.
int KnifeChewFlash(int flashAfterVanilla, ZombieType type, const NightForm& f, ZombiePhase phase, bool mindControlled,
	SeedType plantSeed);
constexpr int kKnifePendingKill = 0x10000;
bool KnifeKillsNow(int flashAfterDecrement);  // the countdown reached the pending-kill mark
bool SkipsGarlic(ZombieType type, const NightForm& f);
int MadButterCs(ZombieType type, ZombiePhase phase, int vanilla);
int EatZombieDamage(ZombieType eater, ZombiePhase eaterPhase, ZombieType target, int vanilla);
unsigned int EatZombieFlags(unsigned int vanilla);
// Green football chew event (hook 640): with its helmet on and age >= 800 cs it drops the helmet; +1000 body.
bool GreenDropsHelmOnChew(const NightForm& f, bool hasHelm, int zombieAge, int playerLevel);
int GreenChewBodyBonus(int playerLevel);

// ---- damage ----
bool ReadingPaperSoaksBodyDamage(ZombieType type, const NightForm& f, ZombiePhase phase);
bool ShieldAlwaysFirst(ZombieType type, const NightForm& f);       // Tall-nut door: pierce/bypass flags ignored
bool SkipsShieldDamageImages(ZombieType type, const NightForm& f);
int AlmanacPaperBodyPerHit(ZombieType type, const NightForm& f);
// Screen Door reflect (hook 709): projectile type/motion and damage flags as the engine has them.
bool DoorReflects(ShieldType shield, unsigned int damageFlags, ProjectileType projectile, ProjectileMotion motion);
int DoorReflectShieldBonus(int damage);       // door HP += damage - 1, so the hit costs it 1 HP
struct Reflection { ProjectileMotion motion; int damageRangeFlags; };
Reflection ReflectedShot(bool doorWalksBackwards);
bool GoldSquishes(ZombieType type, const NightForm& f);
bool GoldRaisesGraveFootballs(ZombieType type, const NightForm& f);   // for Night levels (DieNoLoot)

// ---- art (study 7) ----
struct NightArtImage {
	std::string track;
	std::string file;   // pack file name without extension
	bool operator==(const NightArtImage&) const = default;
};
struct NightArt {
	std::vector<NightArtImage> images;
	std::vector<std::string> hidePrefixes;
	std::vector<std::string> showTracks;   // tracks that exist only in Unnamed's own reanim (merged at load)
	bool empty() const { return images.empty() && hidePrefixes.empty() && showTracks.empty(); }
};
NightArt NightArtFor(ZombieType type, const NightForm& f);
// When any image is missing the whole look is dropped (the base zombie is drawn; the form keeps its behaviour).
NightArt ResolveNightArt(ZombieType type, const NightForm& f, const std::function<bool(const std::string&)>& available);
// Tracks that only Unnamed's Zombie_paper reanim has; hidden on every Newspaper unless its form shows them.
const std::vector<std::string>& UnnamedOnlyNewspaperTracks();
std::string NewspaperMadHeadFile(const NightForm& f);   // "" = vanilla mad head
std::string GreenHelmImage(int damageIndex);            // 1, 2 -> green_football_helmet2/3; "" otherwise
const char* KnifeAnim(const char* vanillaAnim);         // anim_gasp -> anim_gasp1, anim_eat_nopaper -> ..1

// Every rules key the Night zombies read (for the pack consistency test).
const std::vector<std::string>& NightZombieRuleKeys();

} // namespace pvzu::unnamed
