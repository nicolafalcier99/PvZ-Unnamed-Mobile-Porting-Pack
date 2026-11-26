// PVZU: see MenuIntro.h.
#include "Mod/MenuIntro.h"

namespace pvzu {

bool ShouldPlayAdventureIntro(std::optional<int64_t> theIntroSeenAt, std::optional<int64_t> thePreviousPlay, int64_t theNow,
	bool theShownThisSession) {
	if (theShownThisSession)
		return false;
	if (!theIntroSeenAt)
		return true;
	return thePreviousPlay && theNow - *thePreviousPlay >= kLongAbsenceSeconds;
}

} // namespace pvzu
