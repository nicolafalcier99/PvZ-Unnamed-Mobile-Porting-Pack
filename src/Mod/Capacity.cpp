// PVZU: see Capacity.h.
#include "Mod/Capacity.h"

#include <algorithm>

namespace pvzu {

bool SavedArraySizeLoadable(uint32_t theSaved, uint32_t theCurrent) { return theSaved > 0 && theSaved <= theCurrent; }

int ShotsThatFit(int theWanted, int theProjectilesFree, int theReanimsFree, int theMargin) {
	// each shot: one projectile, two reanims (its fire-pea body now, a splash fire on impact)
	const int aRoom = std::min(theProjectilesFree - theMargin, (theReanimsFree - theMargin) / 2);
	return std::clamp(aRoom, 0, std::max(theWanted, 0));
}

} // namespace pvzu
