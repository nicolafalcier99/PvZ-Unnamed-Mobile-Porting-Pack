// PVZU: Plan 9 R2, the Vase Pole Vaulter (elite 0x103), as pure rules (rolls passed in).
// Evidence: the research notes (r2-vase-pole) (exe 0xc036e0..0xc038a6, hook 558 0xc038c0,
// hook 189 0x9594a0). Constants come from the pack's data/rules-vase-pole.json (keys "vasePole.*"); a pack without
// "vasePole.enabled" plays vanilla (no vase). Engine side: Mod/Engine/VasePole.h (feature "vasePole").
#pragma once

#include <functional>
#include <string>
#include <vector>

namespace pvzu::unnamed {

bool VasePoleOn();                 // vasePole.enabled
bool IsVasePole(int variant);      // variant == elite::VASE_POLE and VasePoleOn()

// Vault, anim time in (0.6, 0.7] (0xc03701..0xc0372b): (uint16 mX / 80) low byte, -1 for a normal pole (the vaulted
// plant's cell), +1 for a hypnotised one.
int VasePoleVaultColumn(int zombieX, bool hypnotised);
// Landing (hook 558, 0xc038de..0xc038fe): Board::PixelToGridX(mX, mY) + 1 for a normal pole, - 1 for a hypnotised one.
int VasePoleLandColumn(int pixelGridX, bool hypnotised);
// A normal pole squishes its cell, checks it is empty and puts a random vase there; a hypnotised one only catches
// (0xc0372d jg 0xc037c7).
bool VasePoleMakesOwnVase(bool hypnotised);
// After the squish: a plant still in the cell (not dead, not squished, not rising with a bungee) cancels the vase and
// the catch (0xc03745..0xc03781). bungeeState = engine PlantOnBungeeState (2 = rising).
bool VasePolePlantLeft(bool dead, bool squished, int bungeeState);

// The random vase's zombie: Rand(33) re-rolled on the rejected types (0xc0378e..0xc037be).
bool VasePoleZombieAccepted(int zombieType);
int VasePoleRollZombie(const std::function<int(int)>& rand);

// Catch loop (0xc037c7..0xc03844): EffectedByDamage flags of the caught zombie: 0x81 (ground + only mind-controlled)
// for a normal pole, 1 (ground) for a hypnotised one.
unsigned VasePoleCatchFlags(bool poleHypnotised);
struct VaseRect {
	int x = 0, y = 0, w = 0, h = 0;
};
bool VasePoleRectsOverlap(const VaseRect& a, const VaseRect& b); // 0x955e40: strict, both ways
struct VaseCatchInput {
	bool self = false;      // the pole itself (EffectedByDamage never passes it; kept explicit)
	bool bossClass = false; // 0x7aa320, intended reading (study 4)
	bool sameRow = false;
	bool overlap = false;   // pole attack rect vs zombie rect
	bool effected = false;  // EffectedByDamage(VasePoleCatchFlags(...))
};
bool VasePoleCatches(const VaseCatchInput& in);
// The caught zombie's vase hypnotises its zombie when the caught zombie was not hypnotised (0xc03846 setne).
bool VasePoleCaughtPotHypnotised(bool caughtHypnotised);

// The vase: GridItem state (5 = zombie-hand look) and hook 189 (vase zombie mMindControlled = the vase's flag).
int VasePolePotState();
bool VasePolePotSetsSide();
// The vanilla ScaryPotterOpenPot tail (level award / puzzle phase) after a landing opened a vase; port ruling: only in
// Vasebreaker levels while the pack says vasePole.awardOutsideVasebreaker = 0 (study 4).
bool VasePoleAwardAllowed(bool vasebreakerLevel);

const std::vector<std::string>& VasePoleRuleKeys();

} // namespace pvzu::unnamed
