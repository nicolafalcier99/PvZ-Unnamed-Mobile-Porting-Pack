// PVZU: Unnamed 1.3beta2 Pool zombies (Snorkel, Dolphin Rider, Zomboni) and their elite forms, as pure rules (rolls are
// passed in). Evidence per rule: the research notes (w3-pool) Z1-Z5 and
// pvz-unnamed-pack/SOURCES-pool-zombies.md (exe addresses). Every constant comes from data/rules-pool-zombies.json (keys
// "poolZombies.*") with the vanilla value as fallback, so a pack without a key plays that rule like vanilla.
// Bobsled Team and Ducky Tube have no rule change (Z4, Z5); the Bobsled/ice art is served by the asset overlay.
#pragma once

#include "ConstEnums.h"
#include "Mod/Unnamed/Options.h"

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace pvzu::unnamed {

// Zombie+0x134 codes (ZombieState::variant) of the Pool forms.
namespace poolelite {
constexpr int LOTUS = 0xB;          // Snorkel: Lily Pad on its head, never eats plants
constexpr int FOOTBALL_HEAD = 0xC;  // Dolphin Rider: Football-Zombie head, jumps 4x as far
}
// Zombie+0x148 of a Zomboni (ZombieState::extra): the hidden elite (not in the Almanac).
namespace zamboni {
constexpr int NORMAL = 0;
constexpr int ORANGE = 1;   // slow (0.2), armoured, does not freeze plants
constexpr int RED = 2;      // after the armour breaks: fast (0.45), normal damage, freezes plants
}

bool IsPoolType(ZombieType type);   // Snorkel, Dolphin Rider, Zomboni
const char* PoolEliteName(ZombieType type, int variant, int extra);   // "none", "lotus", "football-head", "orange", "red"

// ---- spawn rolls ----
// eliteOption = eliteoption::* (Mod/Unnamed/Options.h). NONE -> no roll. ALL: Snorkel and Dolphin still draw their
// Rand(n) and always hit (0x7b2000 turns the jumps at 0x796967 / 0x7969a9 into `jo`; the patch follows the setting);
// the Zomboni draws nothing and is always orange (0x95a40d reads the option's value: `== 2 -> +0x148 = 1`).
int PoolRollRange(ZombieType type, int eliteOption = eliteoption::DEFAULT);   // n of Rand(n); 0 = no roll
int RollSnorkel(int roll, int eliteOption = eliteoption::DEFAULT);   // -> variant
int RollDolphin(int roll, int eliteOption = eliteoption::DEFAULT);   // -> variant
int RollZamboni(int roll);            // -> extra (zamboni::ORANGE or NORMAL)
bool ZamboniAlwaysOrange(int eliteOption);   // ALL, and the pack has the Zomboni roll
bool ZamboniRollAllowed(int gameMode, bool isAdventure, int boardLevel);   // not mode 0x89, not adventure level 65
int PoolBodyHp(ZombieType type, int vanilla);   // Dolphin Rider 870

// ---- movement and eating ----
float DolphinJumpVelX(int variant, float vanilla);   // PHASE_DOLPHIN_IN_JUMP start
bool PoolEatsPlants(ZombieType type, int variant);   // the lotus Snorkel walks through plants

// ---- Zomboni: plants frozen for good (Z3a) ----
bool ZamboniFreezesPlants(int state, bool mindControlled, int zombieHeight);
bool InZamboniFreezeBox(int zombieRow, int zombieX, int plantRow, int plantX);
// seedType = engine seed code (the mod's own +80 codes included); squareHasModPumpkin = a 0x6E plant in the square
bool ZamboniFreezeImmune(int seedType, bool squareHasModPumpkin);
int ZamboniFreezeTintRed();     // reanim colour override red / green of a frozen plant (blue kept)
int ZamboniFreezeTintGreen();

// ---- Zomboni: hidden elite (Z3c) ----
// Speed while the tyres are whole: the vanilla curve value comes in.
float ZamboniVelX(int state, int bodyHp, float vanilla);
struct ZamboniHit {
	int damage;   // body damage that lands
	int state;    // state after the hit
};
ZamboniHit ZamboniEliteHit(int state, int damage, int bodyHp);
int ZamboniStateAfterTick(int state, int bodyHp);   // orange at <= 350 HP turns red
struct Rgb {
	int r, g, b;
	bool operator==(const Rgb&) const = default;
};
std::optional<Rgb> ZamboniTint(int state);
int ZamboniIceMaxX(int vanilla);   // ice timer bound 800 -> 933 (wide lawn)

// ---- Ice-shroom freezes the Zomboni (Z3b): replaces CanBeFrozen's `if (!CanBeChilled()) return false;` ----
bool PoolFreezeGateOn();
bool PoolFreezeGate(int variant, bool mindControlled, bool hasHead, int bodyHp);
bool BossRefusesFreeze(ZombieType type, int bodyHp);   // hook 713

// ---- art ----
struct PoolArtImage {
	std::string track;
	std::string file;   // pack file name without extension; "IMAGE_BLANK" = engine blank
	bool operator==(const PoolArtImage&) const = default;
};
struct PoolArt {
	std::vector<PoolArtImage> images;
	std::vector<std::string> hidePrefixes;
	std::vector<std::string> showTracks;
	bool lilyPadHead = false;   // attach REANIM_LILYPAD "anim_idle" @15 to anim_head1 (helper 0x794800, as the umbrella leaf)
	bool empty() const { return images.empty() && hidePrefixes.empty() && showTracks.empty() && !lilyPadHead; }
};
PoolArt PoolArtFor(ZombieType type, int variant);
// When any image file is missing the whole look is dropped (the base zombie is drawn; the form keeps its behaviour).
// available("IMAGE_BLANK") is never asked.
PoolArt ResolvePoolArt(ZombieType type, int variant, const std::function<bool(const std::string&)>& available);

// Every rules key the Pool zombies read (for the pack consistency test).
const std::vector<std::string>& PoolZombieRuleKeys();

} // namespace pvzu::unnamed
