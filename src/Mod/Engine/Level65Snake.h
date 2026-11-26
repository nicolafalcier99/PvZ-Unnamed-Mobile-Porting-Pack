// PVZU: the star snake of 7-5 (level 65) and of mod mode 0x89 Glutton Star; one code in the exe ("mode 0x89 or level 65",
// 0x7aaee0). Study: the research notes (glutton-star); evidence table: pvz-unnamed-pack/SOURCES-special-levels.md "7-5".
// - start (SeedChooserScreen closes, hook 462 -> 0x7a2230): the turn table is cleared, three stars fly right along row 2
//   (head first, so it is item 0 of the projectile array), the bank becomes Starfruit, Sunflower, Wall-nut;
// - the Starfruit packet (SeedPacket::MouseDown, hook 467 -> 0x7a20f9) is never picked up: a click pays its cost and adds
//   a star behind the tail (the head's Projectile mCobTargetRow = exe +0x84 holds the tail's id);
// - W / A / S / D or a swipe (Board::KeyDown, hook 138) turn the head = item 0, 90 degrees only, and record (age, key) at
//   index <its turns> (mTargetZombieID = exe +0x88) of the board's saved turn table (BoardState::starTurns);
// - every star (Projectile::Update, hook 401) replays entry <its turns> at the same age; the head off x 0..899 / y 40..599
//   loses the level; snake stars never die from a hit or at a field edge (Projectile::Die cave 0x6511e3 for every Die
//   made inside CheckForCollision, counter reset each tick; Plan 9 R1), so every edge loses, at the next tick.
// A dead slot keeps a copy of its last state (BoardState::starSlots), standing in for the exe's stale memory, which the
// snake code reads by index. Pure rules: Mod/Unnamed/SpecialLevels.h. Called from SpecialLevels.cpp / SpecialLevelHooks.cpp.
#pragma once

#include "Mod/Unnamed/SpecialLevels.h"
#include "misc/KeyCodes.h"

class Board;
class Projectile;
class SeedPacket;

namespace pvzu::engine {

bool SnakeOn(const Board* theBoard);                  // 7-5 (feature specialLevels) or mode 0x89 (feature modModes)
void SnakeInit(Board* theBoard);                      // Board::InitLevel end: empty turn table and slot copies
void SnakeStart(Board* theBoard);                     // SeedChooserScreen::CloseSeedChooser end
bool SnakePacketClick(SeedPacket* thePacket);         // SeedPacket::MouseDown before the pick-up; true = handled
bool SnakeKey(Board* theBoard, Sexy::KeyCode theKey); // Board::KeyDown; true = W / A / S / D used
bool SnakeSteer(Board* theBoard, unnamed::Steer theDir, const char* theSource);
void SnakeProjectileTick(Projectile* theProjectile);  // Projectile::Update after mProjectileAge++
bool SnakeShotSurvivesHit(Projectile* theProjectile); // end of Projectile::DoImpact; true = the star flies on
void SnakeProjectileDied(Projectile* theProjectile);  // Projectile::Die: keep the slot's last state

} // namespace pvzu::engine
