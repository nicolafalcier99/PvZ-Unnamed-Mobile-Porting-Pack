// PVZU: engine sites of the Unnamed special levels 6-5 (55), 6-10 (60), 7-5 (65) and the hidden 71 / 73 (Plan 7 Task 3a).
// The level scripts themselves are Level55Shooter.*, Level65Snake.* and HiddenLevels.*, reached through the hooks of
// SpecialLevels.h (init / start / update / keys) and the one-line sites below. Every function returns the vanilla answer
// (or does nothing) unless the pack has the feature "specialLevels" and the board is an Adventure level 51-73 whose row
// in data/levels-51-73.json names the script (Mod/Engine/LaterLevels.h CurrentSpecialLevel). Pure rules:
// Mod/Unnamed/SpecialLevels.h; evidence: the research notes (w6-w7-and-new-content) 2.3, pvz-unnamed-pack/SOURCES-special-levels.md.
//
// Touch (a port addition; the exe only reads keys): on 6-5 a tap on a lawn row moves the plant there and a swipe up / down
// moves it one row; on 7-5 a swipe steers the head like W / A / S / D. A press counts only when it starts on the bare
// lawn with nothing in the cursor, so seed packets, coins, the shovel and tap-tap / drag planting work as before.
//
// Dev: with -pvzu-log-waves every rule that fires writes a "special ..." line; scripted keys for -pvzu-input are
// "key up|down|left|right|w|a|s|d".
#pragma once

#include "ConstEnums.h"
#include "Mod/Unnamed/LaterLevels.h"

class Board;
class Projectile;
class Zombie;
class SeedPacket;

namespace pvzu::engine {

bool SpecialLevelsOn();
// The special script of this board when the feature is on (None otherwise).
unnamed::SpecialLevel ActiveSpecial(const Board* theBoard);

// Board::GetNumSeedsInBank, last line: 55 has 4 seed slots, 65 has 3 (hook 142).
int SpecialBankSlots(Board* theBoard, int theVanilla);
// Zombie::ZombieInitialize right after the max HP are stored: 55 halves the current HP of non-ZomBotany zombies (hook 517),
// 71 doubles helmets and their max (hook 515).
void SpecialZombieSpawned(Zombie* theZombie);
// Zombie::DropHead after mHasHead = false: 55 pays sun for the zombie and upgrades the plant (hook 596).
void SpecialHeadDropped(Zombie* theZombie);
// Projectile::Update after mProjectileAge++: 65 replays the head's turns on every star (hook 401).
void SpecialProjectileTick(Projectile* theProjectile);
// The star snake (7-5 and mode 0x89, Mod/Engine/Level65Snake.h): chooser closed (hook 462 -> 0x7a2230), a seed packet
// clicked before it is picked up (hook 467 -> 0x7a20f9; true = handled), end of Projectile::DoImpact (true = the shot
// lives, Die cave 0x6511e3), and Projectile::Die (the slot's last state).
void SpecialChooserClosed(Board* theBoard);
bool SpecialPacketClicked(SeedPacket* thePacket);
bool SpecialShotSurvivesHit(Projectile* theProjectile);
void SpecialProjectileDied(Projectile* theProjectile);
// Board::PickZombieWaves after the level multipliers: 71's wave points (hook 21).
int SpecialWaveBudget(Board* theBoard, int thePointsBefore, int thePointsAfter);
// SeedChooserScreen::SeedNotAllowedToPick: 71 bars Blover, 73 bars eleven seeds (hook 425).
bool SpecialSeedBarred(Board* theBoard, SeedType theSeedType);
// Board::MouseDownWithPlant and CursorObject::Draw, next to the Column-mode test: 73 plants whole columns (hooks 71, 217).
// theSite: a word for the log (the planting site passes "plant"; the cursor draw passes nothing).
bool SpecialPlantsColumn(Board* theBoard, const char* theSite = nullptr);
// Board::PickZombieType: 73's Newspaper first wave (hook 43).
int SpecialFirstWave(Board* theBoard, ZombieType theZombieType, int theVanilla);
// CutScene::StartLevelIntro after mBossTime: 6-10 takes the table's introTimer (4000, hook 220), which makes the
// cut-scene bring in Dr. Zomboss (CutScene::Update, mBossTime > 0 -> Challenge::PlayBossEnter, 0x43bfa4).
int SpecialIntroBossTime(Board* theBoard, int theVanilla);
// Board::MouseDown (board coordinates, before the cursor dispatch) and Board::MouseUp (widget coordinates, first line).
void SpecialTouchDown(Board* theBoard, int x, int y, bool theOnBareLawn);
void SpecialTouchUp(Board* theBoard, int x, int y);

} // namespace pvzu::engine
