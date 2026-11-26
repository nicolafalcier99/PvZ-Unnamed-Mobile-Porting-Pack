#include "Mod/TitleSkip.h"

namespace pvzu {

bool ShouldSkipTitleScreen(bool mobile, bool demoPlayback, bool skipFlag) {
	return !demoPlayback && (mobile || skipFlag);
}

} // namespace pvzu
