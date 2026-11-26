// PVZU: Unnamed 1.3beta2 Day zombies and their elite forms, as pure rules (rolls are passed in).
// Evidence per rule: the research notes (zombies) (section numbers in the comments below).
// Every constant comes from the pack's data/rules-day-zombies.json (keys "dayZombies.*") with the vanilla value as
// fallback, so a pack without a key plays that rule like vanilla.
#pragma once

#include "ConstEnums.h"
#include "Mod/Unnamed/Options.h"

#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace pvzu::unnamed {

// Elite form codes = the exe's Zombie+0x134 values. The exe writes -1 for "no elite"; ZombieState keeps that as 0,
// so 0 is never used as an elite code (study 0.1).
namespace elite {
constexpr int NONE = 0;
constexpr int STICKY_NOTE = 1;    // Normal / Ducky Tube
constexpr int PHONOGRAPH = 2;     // Conehead
constexpr int DANCER_HEAD = 3;    // Conehead
constexpr int PUMPKIN = 4;        // Buckethead
constexpr int UMBRELLA = 5;       // Buckethead
constexpr int ROAD_SIGN = 0x29;   // Pole Vaulter, world 7 only
constexpr int GLOVE = 0x32;       // Pole Vaulter
constexpr int VASE_POLE = 0x103;  // Pole Vaulter
constexpr int SILVER_COIN = 0x105; // Normal
}

int StateVariantFromExe(int exeCode); // -1 -> 0, others unchanged
const char* EliteName(int variant);   // "none", "sticky-note", ... ("unknown" for other codes)

// ---- spawn (study 1, 2.1, 4.1, 5.1, 6.2) ----
// n of the Rand(n) elite roll for this type; 0 = this type never rolls. skyDropped = spawned by a bungee air-drop.
// eliteOption = eliteoption::* (Mod/Unnamed/Options.h): NONE -> 0; ALL -> the exe's option-2 range: Normal Rand(4)
// (0xc11160), Cone Rand(2) (0x795fbb), Pole Rand(13) / sky drop Rand(10) (0x7a1b91), Bucket Rand(2) (0x7b2000 patches
// 0x7970ec). Normal, Cone and Pole read the option's current value; the Bucket's patch follows the setting.
int EliteRollRange(ZombieType type, bool skyDropped = false, int eliteOption = eliteoption::DEFAULT);
// roll in [0, EliteRollRange(type, skyDropped, eliteOption)). worldNumber = adventure world (1 Day .. 5 Roof, 7 =
// levels 61-70); with ALL the Pole's road sign skips the world-7 check (0x95af49 `test bl,bl`).
int RollEliteVariant(ZombieType type, int roll, int worldNumber, bool skyDropped = false,
	int eliteOption = eliteoption::DEFAULT);
// I, Zombie Normal / Ducky Tube with option 2: 0x105 without a roll (0xc111c7).
bool NormalForcedCoin(ZombieType type, int eliteOption, bool iZombieLevel);
// +133 px added right after the generic `780 + Rand(40)` start x (not Jack-in-the-Box, not Jalapeno head).
float StartOffsetPx(ZombieType type);
// Pole Vaulter start x is `base + Rand(10)`; vanilla base 870.
int PoleStartBaseX(int vanillaBase);
// Zamboni / Catapult / Gargantuar (+ Giga) start x is `base + Rand(10)`: vanilla 800 / 825 / 845, Unnamed 933 /
// 958 / 978 (hooks 495 / 497 / 512, the research notes (r8-options) 5). Other types: vanillaBase.
int ZombieStartBaseX(ZombieType type, int vanillaBase);
// Body HP right after the type's init; vanilla = the engine's value.
int ZombieStartBodyHp(ZombieType type, int variant, int vanilla);
// Extra px the attack rect reaches forward (glove pole 112).
int EliteAttackRectExtraPx(int variant);

// ---- movement and eating (study 2.2, 3, 5.2, 7) ----
float EliteAnimRateMultiplier(ZombieType type, int variant);
bool EliteEatsPlants(int variant);
bool EliteShowsYuckyFace(int variant);
// The bite clock of CheckIfPreyCaught: vanilla bites when age % ticks == 0; Flag on remainders 1..5.
bool BitesThisTick(ZombieType type, int zombieAge, int ticksBetweenEats);

// ---- Flag sun drain (study 3) ----
// ageBeforeIncrement = mZombieAge as it is before Zombie::Update adds 1.
bool FlagDrainsThisTick(ZombieType type, int ageBeforeIncrement);
bool FlagDrainAllowedIn(GameMode mode, bool isIZombieLevel, bool isAdventure, int boardLevel);
int FlagSunDrainRollRange();      // n of Rand(n)
int FlagSunDrain(int roll);       // roll in [0, FlagSunDrainRollRange()) -> sun taken

// ---- auras: phonograph heal, pumpkin buckets (study 4.2, 5.2) ----
bool HasAura(int variant);
// Adds one tick to the aura timer; true when the pulse fires (timer back to 0).
bool AuraTick(int& counter);
bool InAura(int sourceRow, int sourceX, int targetRow, int targetX);
bool AuraSkips(ZombieType type, int bodyMaxHealth, int variant);   // boss-like zombies are never touched
bool PumpkinGivesBucketTo(ZombieType type, HelmType helm);
int PumpkinBucketHp();

// ---- deaths (study 4.3, 5.3) ----
bool DeathEffectAllowed(ZombieHeight height, int fromWave);
bool SummonsDancersOnDeath(int variant);
bool SkyDropsOnDeath(int variant);
int SkyDropTypeRollRange();                 // 0 = keep the vanilla picker
bool SkyDropTypeAccepted(int roll);         // roll of Rand(SkyDropTypeRollRange()); accepted roll = ZombieType

// ---- silver coin (study 2.3) ----
bool CoinSquishes(ZombieType type, int variant, bool hasArm, bool hasHead);
bool CoinCopiesOnBite(ZombieType type, int variant, int plantHealth);
// The copy is a Football zombie when the player's adventure level is above 60 and Rand(10) == 0.
bool CoinCopyNeedsFootballRoll(int playerLevel);
int CoinCopyFootballRollRange();
bool CoinCopyIsFootball(int roll);

// ---- Pole Vaulter forms that change the vault grave (study 6.2; the grave itself is Plan 2 Task 5) ----
bool PoleMakesGraveOnVault(int variant);

// ---- glove pole leap (Plan 9 V1; the research notes (v1-zombie-contradictions) item 1) ----
// Hook 361 (Plant::Die 0x467a05 -> 0x95adf0): when a plant dies, every glove Pole (type 3, +0x134 0x32) in its row,
// standing right of it, alive and not mid-vault (phase != 12) is set to plant.mX + 96, the plant it is eating takes
// an 800 bite (0x95af00 -> 0x950000), it stops eating and vaults at once (phase 12, anim_jump 24 fps). It keeps its
// pole after landing when it still had one (phase 11 -> +0x148 = 1, hook 557), else +0x148 = 2 and mVelX 0.5.
// The exe runs the loop only while the caller's EBX <= 0x1000000: a small number or a stack address passes, a heap
// pointer does not. Mapped per caller (exe reads in the gap file):
enum class PlantDeathCause {
	OTHER,       // not traced, or the caller holds a heap pointer: plant abilities (ash plants), Plant::Squish
	             // (Gargantuar, Zomboni), Bungee theft, mod-only paths -> no leap
	EATEN,       // Zombie::EatPlant 0x52fd55 (EBX = 1, the mPlantsEaten step)
	SHOVEL,      // Board shovel 0x4111df (EBX = 1, mPlantsShoveled)
	BLAST,       // Board::KillAllPlantsInRadius 0x41cbf0 (EBX = an int argument): Jack-in-the-Box and the like
	REPLACED,    // Board::MouseDownWithPlant 0x4108fe..0x410ba5 (EBX = the seed or a stack address): upgrades, first aid
	POOL_SQUASH, // Plant::UpdateSquash 0x460d06 (EBX = 0x20): a Squash that lands in water
	LEAP_BITE,   // the leap's own 800 bite (0x950000 keeps the damage in EBX)
};
bool PlantDeathWakesGlove(PlantDeathCause cause); // false without the pack key (vanilla)
struct GloveLeapCheck {
	ZombieType type = ZOMBIE_INVALID;
	int variant = 0;
	ZombiePhase phase = PHASE_ZOMBIE_NORMAL;
	bool deadOrDying = false;
	int zombieRow = 0;
	int zombieX = 0; // GameObject mX (int)
	int plantRow = 0;
	int plantX = 0;
};
bool GloveLeapsFor(const GloveLeapCheck& c); // no hypnosis test in the exe
struct GloveLeap {
	float posX = 0;  // new mPosX
	int jumpMode = 0; // +0x148: 1 = kept its pole (lands back in phase 11), 2 = no pole
	float velX = 0;  // mVelX during the vault
	int bite = 0;    // damage to the plant it was eating
	bool operator==(const GloveLeap&) const = default;
};
GloveLeap GloveLeapAt(int plantX, ZombiePhase phase);
// Hook 557 (0x526114 -> 0x95ad80): landing with +0x148 == 1 clears it and goes back to phase 11 (the attack rect
// is not reset, then StartWalkAnim(0)).
bool GloveLandingKeepsPole(int variant, int jumpMode);

// ---- art (study 0.3, 9) ----
struct EliteArtImage {
	std::string track;  // reanim track whose image is replaced
	std::string file;   // pack file name without extension, e.g. "images/ZombieNoteSmall"; "IMAGE_BLANK" = engine blank
	bool operator==(const EliteArtImage&) const = default;
};
struct EliteArt {
	std::vector<EliteArtImage> images;
	std::vector<std::string> hidePrefixes;  // ReanimShowPrefix(..., hidden)
	bool umbrellaLeaf = false;              // attach REANIM_UMBRELLALEAF "anim_idle" @15 to anim_head1
	bool empty() const { return images.empty() && hidePrefixes.empty() && !umbrellaLeaf; }
};
EliteArt EliteArtFor(ZombieType type, int variant);
// EliteArtFor, but when any of its images is not available the whole look is dropped (the base zombie is drawn;
// the elite keeps its behaviour). available("IMAGE_BLANK") is never asked.
EliteArt ResolveEliteArt(ZombieType type, int variant, const std::function<bool(const std::string&)>& available);

// ---- dev flag -pvzu-force-variant <ZOMBIETYPE>:<variant> ----
struct ForcedVariant {
	std::string typeName;  // engine enum name without ZOMBIE_, e.g. "PAIL"
	int variant = 0;       // decimal or 0x hex
	bool operator==(const ForcedVariant&) const = default;
};
std::optional<ForcedVariant> ParseForcedVariant(std::string_view text);

// Every rules key the Day zombies read (for the pack consistency test).
const std::vector<std::string>& DayZombieRuleKeys();

} // namespace pvzu::unnamed
