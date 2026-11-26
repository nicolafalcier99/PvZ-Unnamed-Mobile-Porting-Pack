// PVZU: Plan 9 L3, Unnamed's small visual details, engine side (rules in Mod/Unnamed/ModVisuals.h; evidence
// the research notes (l3-visual-details)). Called from four marked engine sites:
//   Board::DrawGameObjects  after Plant::Draw   -> ModPlantDrawOver  (Element-shroom icons + digit, Peashooter layers)
//   Board::DrawGameObjects  after Zombie::Draw  -> ModZombieDrawOver (IMAGE_TARGET on a Sniper's target)
//   Attachment::SetMatrix   reanim effect       -> ModReanimOverlay  (the Sniper's head turned to its target)
//   Board::UpdateToolTip    before the switch   -> ModInfoToolTip    (right-hold / long-press zombie and plant info)
// All inert without the pack features "modPlants" / "modVisuals".
#pragma once

namespace Sexy {
class SexyTransform2D;
}

class Board;
class Plant;
class Zombie;
class Reanimation;
class HitResult;
namespace Sexy {
class Graphics;
}

namespace pvzu::engine {

// g is translated to the plant / zombie (BeginDraw), as in the exe after its translation undo.
void ModPlantDrawOver(Plant* thePlant, Sexy::Graphics* g);
void ModZombieDrawOver(Zombie* theZombie, Sexy::Graphics* g);
// true: theMatrix is the reanim's overlay matrix instead of the attachment position (hook 13 skips the copy).
bool ModReanimOverlay(Reanimation* theReanim, Sexy::SexyTransform2D& theMatrix);
// true: the tooltip was filled (the vanilla switch is skipped). Mouse in board coordinates (zoom applied).
bool ModInfoToolTip(Board* theBoard, const HitResult* theHitResult, int theMouseX, int theMouseY);

} // namespace pvzu::engine
