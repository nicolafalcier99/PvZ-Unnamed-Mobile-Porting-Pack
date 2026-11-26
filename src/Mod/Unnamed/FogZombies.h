// PVZU: Unnamed 1.3beta2 Fog zombies (Jack-in-the-Box, Balloon, Digger, Pogo) and their elite forms, as pure rules
// (rolls are passed in). Evidence per rule: the research notes (w4-fog) section 3 and
// pvz-unnamed-pack/SOURCES-fog-zombies.md (exe addresses). Every constant comes from data/rules-fog-zombies.json (keys
// "fogZombies.*") with the vanilla value as fallback, so a pack without a key plays that rule like vanilla.
// Wave values / pick weights (Jack 2 / 1500, Pogo 2) are in data/zombies.json; the +133 px start offset already skips
// Jack-in-the-Box (DayZombies StartOffsetPx, cross-world finding X3).
#pragma once

#include "ConstEnums.h"
#include "Mod/Unnamed/Options.h"

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace pvzu::unnamed {

// Zombie+0x134 codes (ZombieState::variant) of the Fog forms.
namespace fogelite {
constexpr int ICE_JACK = 0xE;      // Ice-shroom head: freezes every plant ~3.5 s instead of exploding
constexpr int POTATO_JACK = 0xF;   // Potato Mine head: seed packets 1-9 to a full cooldown instead of exploding
constexpr int SCREWDRIVER = 0x10;  // Digger: surfaces at x < 90/170/250 and walks on to the house
constexpr int CHOCOLATE = 0x11;    // Pogo: -200 HP, always the long forward bounce (1.2 px/cs)
constexpr int GARLIC = 0x12;       // Pogo: changes lane at every bounce, 1 in 4 invisible
constexpr int CLOUD = 0x2B;        // Balloon: 190 body, its balloon never pops
}

// The global "elite zombies" option (exe 0x800258, cross-world X1; namespace eliteoption in Mod/Unnamed/Options.h):
// the Pogo reads it here (hook 501). Jack, Balloon and Digger do not read it, but "no elites" patches their roll
// jumps at run time (0x7b2100 writes 0x796a64, 0x7a62b0, 0x79747e), so the engine skips their rolls then.

bool IsFogType(ZombieType type);   // Jack-in-the-Box, Balloon, Digger, Pogo
const char* FogEliteName(ZombieType type, int variant);   // "none", "ice-head", "potato-head", "cloud", ...

// ---- spawn rolls (hooks 500, 503, 490, 501) ----
// eliteOption = the player's setting (these rolls are patched code, not readers of 0x800258): NONE -> no roll; ALL ->
// 0x7b2000: Jack Rand(8) -> Rand(2) (0x796a58), so ice or potato; Balloon and Digger draw their Rand(n) and always hit
// (the jumps at 0x7a62b0 / 0x79747e become `jo`).
int JackRollRange(int eliteOption = eliteoption::DEFAULT);   // n of Rand(n); 0 = no roll
int RollJack(int roll);                            // -> variant
bool JackRollAllowed(bool scaryPotterLevel);       // never in Vasebreaker
int BalloonRollRange(int eliteOption = eliteoption::DEFAULT);
int RollBalloon(int roll, int eliteOption = eliteoption::DEFAULT);
int DiggerRollRange(int eliteOption = eliteoption::DEFAULT);
int RollDigger(int roll, int eliteOption = eliteoption::DEFAULT);
int DiggerSurfaceRollRange();                      // the screwdriver's surfacing roll, Rand(3)
int DiggerSurfaceX(int roll);                      // (roll + 1) * 80 -> exe Zombie+0x14C
bool BalloonDiggerRollAllowed(bool iZombieLevel);  // the cloud and the screwdriver never roll in I, Zombie
int PogoRollRange(int eliteOption);
int RollPogo(int roll);
int PogoGarlicVisibleRollRange();                  // Rand(4)
bool PogoGarlicVisible(int roll);                  // roll != 0

// ---- hit points (in-place patches 0x522fc1, 0x5234b9, 0x522be9; hooks 501, 503) ----
int FogBodyHp(ZombieType type, int variant, int vanilla);
int FogHelmHp(ZombieType type, int vanilla);
int FogFlyingHp(ZombieType type, int vanilla);

// ---- Jack-in-the-Box explosion (hooks 560-562, 315) ----
enum class JackBlast {
	VANILLA,        // the vanilla blast (normal Jack, or the potato head in I, Zombie)
	FREEZE_PLANTS,  // ice head: every plant skips its update for a while (Board state), no kill
	PACKETS_COOL,   // potato head: seed packets 1..9 to a full cooldown, no kill
	ICE_ZOMBIES,    // hypnotised ice head: Ice-shroom on every zombie, no kill
	PACKETS_READY,  // hypnotised potato head: seed packets 1..9 ready at once
};
JackBlast JackExplosion(int variant, bool mindControlled, bool iZombieLevel);
int IceFreezeCounter(int plantCount);   // 350 x the plants on the board (one tick off per plant update)
int PotatoPacketCount();                // packets 0..8
// The global freeze, stepped once per plant update while active (exe 0x796045): counter - 1; while it is still > 0 the
// plant skips its whole update; at <= 0 the freeze ends. True = skip this plant's update.
struct IceFreeze {
	int active = 0;
	int counter = 0;
	bool operator==(const IceFreeze&) const = default;
};
bool IceFreezeStep(IceFreeze& freeze);

// ---- Balloon (hooks 521, 551, 676, cave 0x651615, patch 0x52f63e) ----
// PickRandomSpeed: a non-cloud Balloon that is not walking flies at 0.45; nullopt = the vanilla pick.
std::optional<float> BalloonVelX(ZombieType type, int variant, bool walkingPhase);
bool MowerSkips(ZombieType type, int flyingHealth);   // a Balloon with its balloon is neither mowed nor trips the mower
bool FlyersEat();                                     // the eating update no longer returns for a flyer
bool FlyingBalloonSkipsPlant(int seedType);           // a flying Balloon does not eat these
bool CloudKeepsFlying(int variant, bool flyingPhase); // LandFlyer: no pop, no pool death, it only sinks

// ---- Digger (hooks 654, 585, 586) ----
// CanTargetPlant while tunneling: true/false, or nullopt = the vanilla answer (unarmed Potato Mine).
std::optional<bool> TunnelingDiggerTargets(int seedType);
float DiggerSurfaceBelowX(int surfaceX, float vanilla);   // tunneling ends at x < surfaceX + 10 (vanilla 10)
bool DiggerRisesWithoutAxe(int variant);                   // phase RISE_WITHOUT_AXE: walks on to the house

// ---- Pogo (hooks 531-534, patch 0x5256eb) ----
enum class PogoMove { BOUNCE, HIGH_BOUNCE, FORWARD };
struct PogoNext {
	PogoMove move = PogoMove::BOUNCE;
	float velX = 0.0f;               // FORWARD only
	bool operator==(const PogoNext&) const = default;
};
// End of a bounce (mPhaseCounter 0). targetDx = zombie.mX - target.mX of the plant (or opposite-side zombie) in reach,
// nullopt = none. inHighBounce1 = the vanilla wind-up bounce is running.
PogoNext PogoBounceEnd(int variant, std::optional<int> targetDx, bool inHighBounce1);
bool PogoTargetsZombies();                 // the bounce end also looks for an opposite-side zombie
bool PogoTurnsYucky(int variant);          // garlic: mYuckyFace = 1 at every bounce end
int PogoYuckyCounter();                    // ... and mYuckyFaceCounter = 100
bool PogoBonksOn(int seedType);            // FORWARD_BOUNCE_2 at counter 70: the pogo loses its stick
bool PogoLandsOnZombies();
int PogoLandRectGrowPx();
int PogoLandDamage(int variant, bool firstZombie);

// ---- art (study 8) ----
struct FogArtImage {
	std::string track;
	std::string file;   // pack file name without extension; "IMAGE_BLANK" = engine blank
	bool operator==(const FogArtImage&) const = default;
};
struct FogArt {
	std::vector<FogArtImage> images;
	std::vector<std::string> hidePrefixes;
	ReanimationType headReanim = ReanimationType::REANIM_NONE;   // attached to anim_head1 like the umbrella leaf (helper 0x794800)
	std::string headAnim;   // the anim the helper plays on headReanim ("anim_head_idle", string 0x668c34)
	bool empty() const { return images.empty() && hidePrefixes.empty() && headReanim == ReanimationType::REANIM_NONE; }
};
FogArt FogArtFor(ZombieType type, int variant);
// When any image file is missing the whole look is dropped (the base zombie is drawn; the form keeps its behaviour).
FogArt ResolveFogArt(ZombieType type, int variant, const std::function<bool(const std::string&)>& available);
// The track the head reanim really plays: `wanted` when the reanim has it (case-insensitive), else its first track,
// because the exe's Reanimation::FindTrackIndex (0x472f30) returns 0 for a missing name. Potato Mine has no
// "anim_head_idle", so the potato head plays track 0 "anim_armed" (the popped-up mine), not "anim_idle" (the mound).
std::string FogHeadAnimTrack(const std::string& wanted, const std::vector<std::string>& trackNames);

// Every rules key the Fog zombies read (for the pack consistency test).
const std::vector<std::string>& FogZombieRuleKeys();

} // namespace pvzu::unnamed
