// PVZU: Unnamed's gameplay edge. Its screen is 1067 wide and its zombies start 133 px further right (hook 480), so
// it moved vanilla's x-800 limits: a zombie can be damaged while its rect x <= 1067 (.text 0x531BEC, patched in
// place in Zombie::EffectedByDamage, vanilla 0x320) and a projectile lives until x > 1068 (hook 377, 0x955AB0,
// Projectile::CheckForCollision, vanilla 800.0f). Plant attack rects (mX + 60, width 800) were left alone.
// Values: pvz-unnamed-pack/data/rules-fixes.json; a missing key keeps vanilla.
#pragma once

#include <string>
#include <vector>

namespace pvzu::unnamed {

int ZombieHittableMaxX(int vanilla);
float ProjectileMaxX(float vanilla);
const std::vector<std::string>& WideReachRuleKeys();

} // namespace pvzu::unnamed
