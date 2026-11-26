// PVZU: Plan 8 part 3a: how the engine carries Unnamed's projectile types that PvZ-Portable lacks (exe Projectile+0x5c
// >= 0x14). Evidence: the research notes (mod-plants-behaviour) 1.4 (damage table 0x7b0890, images 0x7a0e1d..0x7a0e5e; the
// 0x21/0x22/0x23 images of hook 405 / 407: the research notes (l2-shot-art)) and the plant sections (0x70 Dusk-pult,
// 0x57 Primal Repeater, 0x7b Spirit-tail). Pure; the engine side is Mod/Engine/ModPlantsShooters.*.
//
// A mod projectile is an engine Projectile of a vanilla "carrier" type (so every engine table, collision and motion
// path that indexes by ProjectileType stays in range) with ProjectileState::modType = the exe type. Its damage comes from
// ModProjectileDamage(exe type) and its own impact rules; its art is the exe's: the carrier's image, IMAGE_POOLSPARKLY
// (0x16) or the mod's own file (ModShotImageFile). Vanilla and ordinary Unnamed shots have modType 0.
#pragma once

#include <cstdint>
#include <string_view>

namespace pvzu::unnamed {

// Exe projectile types of the mod (0x12 Catapult vase, 0x14 puff, 0x15 snow pea, 0x16 prism beam, 0x17 star, 0x20
// Catapult bug spray, 0x21 dusk shard, 0x22 knock-back pea, 0x23 poison pea; 0x12 / 0x20: Mod/Unnamed/CatapultShots.h).
inline constexpr int kShotPrism = 0x16, kShotDusk = 0x21, kShotKnockback = 0x22, kShotPoison = 0x23;
bool IsModShotType(int theExeType);

// The engine ProjectileType that carries an exe type: 0..13 themselves; 0x14 puff (4), 0x15 snow pea (1), 0x16 pea (0,
// drawn as the pool sparkle, 0x7a0e5e), 0x17 star (7), 0x21 cabbage (2, hook 407 draws it as a cabbage), 0x22 / 0x23
// pea (0), 0x12 / 0x20 basketball (9). -1 = no such type (6 fireball, 13 zombie pea are valid carriers, never mod shots).
int ModShotCarrier(int theExeType);

// Colour (0xRRGGBB) multiplied over the shot's art; 0 = draw it untinted. theStarKind = Projectile+0x80 on a star
// (1..5, Dream Starfruit / Spirit-tail): the kind's palette colour (0x959cc0); the exe colours no other shot (0x959e90).
// 0x22 / 0x23: an amber / violet stand-in, used only when their own image (ModShotImageFile) is missing from the pack.
uint32_t ModShotTint(int theExeType, int theStarKind);

// The mod's own image of a shot, as a file under the pack's assets (no extension), drawn at scale 1, untinted (hook 405
// 0xc10b50): 0x22 images/projectileprimitivepea, 0x23 images/ProjectilePoisonPea. Empty = the engine's image stays
// (0x16 IMAGE_POOLSPARKLY, 0x21 the cabbage). Evidence: the research notes (l2-shot-art).
std::string_view ModShotImageFile(int theExeType);

// 0xc10e10: the per-zombie side record (Repeater poison) exists only for zombies whose mFromWave is not -2 / -3.
bool RepeaterPoisonRecordFor(int theFromWave);

} // namespace pvzu::unnamed
