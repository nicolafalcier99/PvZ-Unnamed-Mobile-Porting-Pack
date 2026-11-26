// PVZU: Plan 9 R2, engine side of the Vase Pole Vaulter (elite 0x103; pure rules in Mod/Unnamed/VasePole.h).
// Callers guard each call with GameData::Get().Feature("vasePole").
#pragma once

class Challenge;
class GridItem;
class Zombie;

namespace pvzu::engine {

// The IN_VAULT window (0xc036e0, from UnnamedPoleVault when DecideVault says vase): squish the cell, a random vase
// there, the other side's zombies in reach become vases too. Once per zombie (ZombieState flag, exe +0x12c).
void VasePoleVault(Zombie* thePole);
// Zombie::UpdateZombiePolevaulter jump end, after mX = mPosX (hook 558, 0xc038c0): opens the vases of the landing cell.
void VasePoleLanded(Zombie* thePole);
// Challenge::ScaryPotterOpenPot, zombie vase (hook 189, 0x9594b3): the zombie's side = the vase's flag.
void VasePolePotZombie(GridItem* thePot, Zombie* theZombie);
// Challenge::ScaryPotterOpenPot tail: false = skip the level award / puzzle phase (a landing outside Vasebreaker).
bool VasePoleAwardAllowed(Challenge* theChallenge);

} // namespace pvzu::engine
