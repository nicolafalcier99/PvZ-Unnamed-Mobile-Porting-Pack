// PVZU: engine side of the Unnamed Day level rules (pure rules in Mod/Unnamed/Levels.h).
// Callers guard each call with GameData::Get().Feature("dayLevels").
#pragma once

#include "ConstEnums.h"

class Board;
class Plant;
class Zombie;

namespace pvzu::engine {

// Zombie::UpdateZombiePolevaulter IN_VAULT window: returns whether the vaulter bonks (Tall-nut-like plant);
// otherwise makes the vault grave once per zombie (.text 0x525fa0 -> 0x651a97).
bool UnnamedPoleVault(Zombie* theZombie, Plant* theTarget);
// Board::SpawnZombiesFromGraves: hook 88 re-roll of the vanilla pick.
ZombieType UnnamedGraveRiseType(Board* theBoard, ZombieType theVanillaType);
// Zombie::UpdateZombieRiseFromGrave end: hook 595 phase by type.
void UnnamedGraveRisePhase(Zombie* theZombie);
// Zombie::ApplyBurn: true = takes 1800 damage instead of turning to ash (body + helm + shield).
bool UnnamedTooToughToAsh(const Zombie* theZombie);

} // namespace pvzu::engine
