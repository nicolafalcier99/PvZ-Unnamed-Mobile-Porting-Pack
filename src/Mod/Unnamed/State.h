// PVZU: per-object Unnamed state kept on engine Plants/Zombies/Projectiles/GridItems and saved as an extra TLV field
// (only in Unnamed mode; older saves and vanilla saves simply lack it -> zeros).
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace pvzu::unnamed {

struct ZombieState {
	int32_t variant = 0;   // Unnamed elite form code (exe Zombie+0x134); 0 = normal
	int32_t counter = 0;   // per-variant timer/counter
	int32_t extra = 0;     // per-variant extra value
	int32_t flags = 0;     // level-rule bits, see kZombieFlag* (saved since Plan 2 Task 5; older blobs read 0)
	int32_t subVariant = 0; // second Unnamed form code (exe Zombie+0x14C): Newspaper football head 1, green Football 0x1A
	int32_t stored = 0;     // exe Zombie+0x12C of a random-summoner Dancer (variant 0x104): its follower type, -1 = random
	                        // (the "carnival" backup count of other Dancers is kept in `counter` by Night levels).
	                        // Saved since Plan 3 Task 2; older blobs read 0.
	// Mod-plant statuses (Plan 8 part 3, one owner: Mod/Engine/ModZombieStatus.h; the research notes (mod-plants-behaviour)
	// 1.3). Bytes 24..39, saved since then; older (24-byte) blobs read 0.
	int32_t toxicKind = 0;        // Toxic Shadow-shroom poison (exe word Zombie+0x140): 0 none, 1 normal, 2 ignores armour
	int32_t toxicTimer = 0;       // its timer (exe Zombie+0x148), 0x7ffff when set
	int32_t repeaterPoisonCs = 0; // Primal Repeater poison (exe side record Board+0x5888 entry +8): 1 damage per cs while > 0
	int32_t weakenCs = 0;         // "weakened" (exe word Zombie+0x1a, 16-bit): damage x2 while > 0, -1 per UpdateActions
	bool operator==(const ZombieState&) const = default;
};

// ZombieState::flags bits.
constexpr int32_t kZombieFlagVaultGraveMade = 1; // exe Zombie+0x12c: this Pole Vaulter already made its grave

struct PlantState {
	int32_t counter = 0;   // e.g. Peashooter speed-up n, Chomper eaten HP
	int32_t extra = 0;
	int32_t frozen = 0;    // 1 = frozen for good by a Zomboni (exe Plant+0x144 = 0, Pool study Z3a); saved since Plan 4,
	                       // older blobs read 0
	// Split Pea sweep mode (Fog study 2.5, exe Plant+0xE0..+0xEC). Saved since Plan 5 Task 1; older blobs read idle.
	int32_t sweepAngle = 0;  // +0xE0, bounces 30..97
	int32_t sweepDir = 0;    // +0xE4, 1 = falling
	int32_t sweepStep = 0;   // +0xE8, 0..5 (fires at 3)
	int32_t sweepTimer = -1; // +0xEC, -1 = idle, else cs since the sweep began
	// Mod-only plant (Plan 8, Mod/Unnamed/ModPlants.h): its seed 0x50..0x7f, 0 = an ordinary plant. The engine's
	// mSeedType holds the base plant (seed - 0x50). Saved since Plan 8; older blobs read 0.
	int32_t modSeed = 0;
	// Plan 8 part 3a (mod shooters, Mod/Engine/ModPlantsShooters.h), bytes 32..51. Saved since then; older blobs read 0.
	int32_t elementBits = 0;  // Element-shroom exe +0xcc: last three elements, newest bit 0 (0 fire, 1 ice)
	int32_t elementMode = 0;  // Element-shroom exe +0xcd: chosen element (click toggles)
	int32_t elementCount = 0; // Element-shroom exe +0xce: elements gained
	int32_t starColour = 0;   // Dream Starfruit exe +0xcd: colour index 0..4 (star kind = colour + 1)
	int32_t cannonAmmo = 0;   // Loaded Cannon exe +0x138: the seed it was loaded with (0x22 Kernel-pult), 0 = never
	// Plan 8 part 3c (Mod/Engine/ModPlantsShadow.h; the research notes (mod-plants-behaviour) 1.2, 0x67), bytes 52..59. Saved
	// since then; older blobs read 0.
	int32_t shadow = 0;        // shadow state (exe Plant+0xcc), recomputed every tick for shadow plants
	int32_t charmedBitten = 0; // Charmed Tall-nut: bitten HP towards the next summon (exe Plant+0x114)
	bool operator==(const PlantState&) const = default;
};

struct ProjectileState {
	int32_t fireTier = 0;  // Unnamed fire tier (exe Projectile+0x80): 0 = none, 1..5 Torchwood tiers, 3 = Snow Pea fire shot
	bool converted = false; // exe Projectile+0x84: ConvertToFireball ran once (Pool Torchwood tiers). Saved since Plan 4
	                        // Task 1; older 4-byte blobs read false.
	int32_t pierceHits = 0; // exe Projectile+0x18: hits a Cactus spike lived through (Fog study 2.3). Saved since Plan 5
	                        // Task 1; older blobs read 0.
	int32_t starCurve = 0;  // the exe's "curve tag" Projectile+0x88 on a Starfruit star (Plan 9 R1, the research notes (gaps)
	                        // r1-starfruit.md): kStarCurve* below. Same save slot as the old spareHits (Plan 5 Task 1),
	                        // whose 1 on a Starfruit star now reads as kStarCurveBoomerang; older blobs read 0.
	bool bigCabbage = false;  // exe Projectile+0x80 = 1 on a Cabbage-pult's big quick throw (Roof study 1.1): 400 damage,
	                          // drawn x3. Saved since Plan 6 Task 1; older blobs read false.
	int32_t butterStreak = 0; // exe Projectile+0x88 on a Kernel-pult's butter / Cob: kernels thrown before it (Roof study
	                          // 1.3), sets the butter time. Saved since Plan 6 Task 1; older blobs read 0.
	// Special mod-plant shots (Plan 8 part 3b, Mod/Engine/ModPlantsSpecials.h): a kModShot* kind. Saved since Plan 8
	// part 3; older blobs read 0. (Bytes 28..31 after it are reserved, written 0: they held a Prism beam's damage before
	// both Prism beams became one mod shot, modType 0x16 + field80; Decode converts such blobs.)
	int32_t modShot = 0;
	// Plan 8 part 3a (mod shooters, Mod/Unnamed/ModShots.h). Saved since then; older blobs read 0.
	int32_t modType = 0;  // exe Projectile+0x5c of a mod shot (0x16, 0x21, 0x22, 0x23 ...); 0 = the carrier type itself
	int32_t field80 = 0;  // exe Projectile+0x80 of a mod shot: star kind 1..5, prism damage (type 0x16: Prism-shroom
	                      // and Spirit-tail both, 0x7b08fb)
	int32_t cobAmmo = 0;  // exe Projectile+0x88 of a Loaded Cannon cob: its ammo seed (0x22 -> 3000 burn cap)
	bool operator==(const ProjectileState&) const = default;
};

// Per-grid-item Unnamed state (Plan 9 R2). The exe's GridItem+0x54 byte on a vase: its zombie comes out with
// mMindControlled = potHypnotised (hook 189; set by the vase pole, Mod/Unnamed/VasePole.h). Saved like the others.
struct GridState {
	int32_t potHypnotised = 0;
	bool operator==(const GridState&) const = default;
};
// ProjectileState::starCurve: how Projectile::Update bends a star (hook 400 -> 0x95b170). Boomerang = exe +0x88 276 (the
// five stars of a Starfruit), spiral i = exe +0x88 i (the four extra stars, i = 0..3; the exe accepts 0..4).
constexpr int32_t kStarCurveNone = 0;
constexpr int32_t kStarCurveBoomerang = 1;
constexpr int32_t kStarCurveSpiral0 = 2;

// ProjectileState::modShot kinds. Shard / core = the exe's Projectile+0x84 on a Shrapnel Cherry star (0x794017..0x7941da).
constexpr int32_t kModShotNone = 0;
constexpr int32_t kModShotShrapnelShard = 1; // flies, 250 extra on its one hit
constexpr int32_t kModShotShrapnelCore = 2;  // stays put, re-plants a Cherry Bomb at age 1400
// 3 is retired (it was Prism-shroom's beam; a Prism beam is now modType 0x16 with its damage in field80).
constexpr int32_t kModShotLeafReflected = 4; // a zombie shot a Leaf Shield turned: it now hits zombies (flags 1)

// The last state of a projectile slot whose star died in the star-snake modes (7-5, mode 0x89). The exe's dead slot keeps
// its memory, and the snake code reads it by index (the research notes (glutton-star) 6); the port frees and wipes the slot, so
// it keeps this copy instead. index = DataArray index; tail = exe +0x84 (mCobTargetRow); turns = exe +0x88.
struct StarSlot {
	int32_t index = 0, type = 0, x = 0, y = 0;
	float velX = 0.0f, velY = 0.0f;
	int32_t age = 0, tail = 0, turns = 0;
	bool operator==(const StarSlot&) const = default;
};

// Board-wide Unnamed state, saved as its own chunk only while it differs from the default (so vanilla and ordinary
// Unnamed saves stay byte-identical). Fog study 3.1 / 6: the ice-head Jack's global freeze (exe 0x800100 / 0x800104).
// Star snake (glutton-star.md): the turn table (exe 0x810000; pairs age, Steer) and the dead slots above. Saved since
// the Glutton Star task; an older 8-byte chunk reads both empty.
struct BoardState {
	int32_t iceFreezeActive = 0;
	int32_t iceFreezeCounter = 0;
	std::vector<int32_t> starTurns;
	std::vector<StarSlot> starSlots;
	bool IsDefault() const { return *this == BoardState{}; }
	bool operator==(const BoardState&) const = default;
};

std::vector<uint8_t> Encode(const ZombieState& s);
std::vector<uint8_t> Encode(const PlantState& s);
std::vector<uint8_t> Encode(const ProjectileState& s);
std::vector<uint8_t> Encode(const BoardState& s);
std::vector<uint8_t> Encode(const GridState& s);
// false (and zeroed output) if the blob is too short.
bool Decode(const uint8_t* data, size_t size, ZombieState& out);
bool Decode(const uint8_t* data, size_t size, PlantState& out);
bool Decode(const uint8_t* data, size_t size, ProjectileState& out);
bool Decode(const uint8_t* data, size_t size, BoardState& out);
bool Decode(const uint8_t* data, size_t size, GridState& out);

} // namespace pvzu::unnamed
