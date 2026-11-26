#include "Mod/Engine/SpecialLevels.h"

#include "Mod/Engine/HiddenLevels.h"
#include "Mod/Engine/LaterLevels.h"
#include "Mod/Engine/Level55Shooter.h"
#include "Mod/Engine/Level65Snake.h"
#include "Mod/Engine/SpecialLevelHooks.h"
#include "Mod/Engine/Level70Boss.h"

#include "Lawn/Board.h"

namespace pvzu::engine {

namespace u = pvzu::unnamed;

// Each starts with the same test, so a level without a script costs one table lookup. The scripts check the pack feature
// themselves (Task 3a: "specialLevels" for 55 / 65 / 71-73; 60 needs only CutScene, see SpecialLevelHooks.h).

void SpecialLevelInit(Board* theBoard) {
	SnakeInit(theBoard); // 7-5 and mode 0x89 (checks both itself)
	const u::SpecialLevel aSpecial = CurrentSpecialLevel(theBoard);
	if (aSpecial == u::SpecialLevel::None)
		return;
	if (aSpecial == u::SpecialLevel::Hidden1 && SpecialLevelsOn()) HiddenInit(theBoard);
}

void SpecialLevelStart(Board* theBoard) {
	const u::SpecialLevel aSpecial = CurrentSpecialLevel(theBoard);
	if (aSpecial == u::SpecialLevel::None)
		return;
	if (aSpecial == u::SpecialLevel::Shooter && SpecialLevelsOn()) ShooterStart(theBoard);
}

void SpecialLevelUpdate(Board* theBoard) {
	const u::SpecialLevel aSpecial = CurrentSpecialLevel(theBoard);
	if (aSpecial == u::SpecialLevel::None)
		return;
	if (aSpecial == u::SpecialLevel::Hidden1 && SpecialLevelsOn()) HiddenUpdate(theBoard);
	if (CurrentSpecialLevel(theBoard) == u::SpecialLevel::Boss)
		Level70BossUpdate(theBoard); // 7-10: the giant Newspaper's attack loop (Mod/Engine/Level70Boss.h)
}

void SpecialLevelWave(Board* theBoard) {
	if (CurrentSpecialLevel(theBoard) == u::SpecialLevel::None)
		return;
	if (CurrentSpecialLevel(theBoard) == u::SpecialLevel::Boss)
		Level70BossWave(theBoard); // 7-10: the 10th wave brings the boss and the darkness (Mod/Engine/Level70Boss.h)
}

bool SpecialLevelKeyDown(Board* theBoard, Sexy::KeyCode theKey) {
	if (SnakeOn(theBoard))
		return SnakeKey(theBoard, theKey); // 7-5 and mode 0x89
	const u::SpecialLevel aSpecial = CurrentSpecialLevel(theBoard);
	if (aSpecial == u::SpecialLevel::None)
		return false;
	if (aSpecial == u::SpecialLevel::Shooter && SpecialLevelsOn()) return ShooterKey(theBoard, theKey);
	return false;
}

} // namespace pvzu::engine
