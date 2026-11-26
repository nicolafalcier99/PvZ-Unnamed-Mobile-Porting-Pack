// PVZU: Plan 9 V1, the zombie rules where the Chinese community and the port disagreed and the exe sided with the
// community, as pure rules (rolls passed in). Evidence per rule (exe addresses, the community claim, the decision):
// the research notes (v1-zombie-contradictions) and pvz-unnamed-pack/SOURCES-zombie-gaps-v1.md.
// Every switch and constant comes from data/rules-zombie-gaps-v1.json (keys "zombieGapsV1.*"); a missing key plays
// that rule like the port did before (vanilla or the earlier Unnamed reading). The glove Pole leap (item 1) lives in
// DayZombies.h with the other glove rules.
#pragma once

#include "ConstEnums.h"

#include <optional>
#include <string>
#include <vector>

namespace pvzu::unnamed {

// ---- item 3: the Newspaper's gasp and burns (TakeBodyDamage cave 0x651d22; ApplyBurn 0x6514d5) ----
// While a zombie is in PHASE_NEWSPAPER_MADDENING its body HP is never written (flash and chill still happen).
bool GaspIgnoresBodyDamage(ZombiePhase phase);
// ApplyBurn turns into TakeDamage(1800, 0x12) for every Newspaper (so a reading one only loses its paper) and for a
// white random summoner Dancer (0x104) still dancing in with its arm (0xc05490).
bool BurnBecomesHit(ZombieType type, int variant, ZombiePhase phase, bool hasArm);

// ---- item 6: the red-eye Newspaper (subVariant 1) while it reads ----
// TakeDamage entry 0xc05900: its hit flags are cleared (no bypass, no shield-and-body, no freeze, flashes).
bool ClearsHitFlags(ZombieType type, int subVariant, ZombiePhase phase);
// Garlic aura 0xc05ad0: the Garlic rips its paper at once (DropShield) instead of the 1-damage poke.
bool GarlicRipsPaper(ZombieType type, int subVariant, ZombiePhase phase);

// ---- item 12: the knife Newspaper's bite (EatPlant 0x651210 -> 0xc02730) ----
// The knife skips the 4-HP bite only when mad, holding its knife, no ice-head Jack freeze and the plant not frozen.
bool KnifeSkipsBite(ZombiePhase phase, bool hasObject, bool iceFreezeActive, bool plantFrozen);
bool KnifeBiteConditionsOn(); // false = the earlier port reading (type + variant only)

// ---- item 8: the white random summoner Dancer (0x104) while it dances in (phase 40) ----
bool MoonwalkerRefusesFreeze(int variant, ZombiePhase phase); // CanBeFrozen 0xc053c0 (butter and Ice-shroom)
// TakeDamage entry 0xc05400: with its arm, a hit with damage + 200 >= body is cancelled, body = max / 2, arm off.
std::optional<int> MoonwalkerSavedBody(ZombieType type, int variant, ZombiePhase phase, bool hasArm, int damage, int body,
	int bodyMax);
bool MoonwalkerBiteRestartsSummon(int variant, ZombiePhase phase); // UpdateZombieDancer 0xc05510: counter = 0

// ---- item 4: road-sign Pole 0x29 (hook 555 0x9589f1): the vault keeps its running mVelX ----
bool VaultKeepsSpeed(int variant);

// ---- item 5: staff Gargantuar (hook 565 0xc11500, 0xc11740): only the first throw is a thrown Imp; later
// summons are plain spawns (row of PickRowForNewZombie(ZOMBIE_IMP), the type's own start x), never thrown ----
bool StaffSummonsWithoutThrow(int variant, bool heldImp);

// ---- item 9: garlic Pogo (hook 614 -> 0x95b010): squishes plants as a Zomboni does, every tick ----
bool PogoCrushesPlants(ZombieType type, int variant, bool flatTires);
// hook 531 0x95991c: only the 2nd and later zombies it lands on make a yucky face
bool PogoLandMakesYucky(int variant, bool firstHit);

// ---- item 10: green Football bite (EatPlant 0xc12660 -> 0xc12f40, before the normal 4) ----
// x = (helmMax + bodyMax) / (hasHelm * helm + body) * 7; plant HP = round-to-even(HP - x); in I, Zombie a large sun
// when round(x) >= 75.
struct GreenBite {
	int plantHealth = 0;
	bool largeSun = false;
	bool operator==(const GreenBite&) const = default;
};
std::optional<GreenBite> GreenFootballBite(ZombieType type, int subVariant, int plantHealth, bool hasHelm, int helm,
	int helmMax, int body, int bodyMax, bool iZombieLevel);

// ---- item 11: ladder pile (0x35) bites (hook 672 0x79c300) and targets (0xc02c90 only in phases 76 / 77) ----
// N = 4 (8 chilled), rem = age % N: with a ladder in hand bites when 0 < rem < 6, without one every tick.
std::optional<bool> PileBitesThisTick(int variant, bool holdsLadder, int zombieAge, bool chilled);
bool PileTargetRuleOnlyWhileCarrying(); // true: the pile's CHEW/LADDER target rule only in phases 76 / 77

// ---- item 13 (#19): a plain Zomboni never slows down (0x52a81e reads 14.0f as a double, ~0): 0.25 ----
std::optional<float> PlainZamboniVelX();

const std::vector<std::string>& ZombieGapsV1RuleKeys();

} // namespace pvzu::unnamed
