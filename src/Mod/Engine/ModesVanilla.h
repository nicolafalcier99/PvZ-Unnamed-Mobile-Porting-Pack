// PVZU: engine side of the Unnamed changes to the vanilla mini-games, puzzles and survival modes (pure rules in
// Mod/Unnamed/ModesVanilla.h; evidence the research notes (modes-ui-shop) 2.1, 2.2, 9 and pvz-unnamed-pack/SOURCES-modes-vanilla.md).
// Every function is inert (returns false / the vanilla value) unless the pack's feature "modesVanilla" is on, so Vanilla
// mode is unchanged.
//
// Hook for the 48 mod-only modes (Plan "modes-mod", study 2.3): GetChallengeDefinition() asks ModesChallengeDefinition()
// first for every index. A mod mode table can answer there for indices >= the vanilla NUM_CHALLENGE_MODES once the
// screen's button array (ChallengeScreen::mChallengeButtons, sized NUM_CHALLENGE_MODES) and MAX_CHALLANGE_PAGES grow;
// nothing here assumes 72 entries beyond that array.
//
// Dev flags (no effect unless given):
//   -pvzu-dev-mode <n>            start game mode n (GameMode number, e.g. 29 = Zombies on Speed) after loading
//   -pvzu-dev-challenge-page <p>  open the challenge screen on page p (0 Survival, 1 Mini-games, 2 Limbo, 3 Puzzle)
//   -pvzu-dev-modes <scenario>    in-mode checks: bowling (zombies in every row, nuts rolled), iz (combos placed),
//                                 art (Grave Buster on art cells), zen (glove on a jumping Squash)
// Log lines (-pvzu-log-waves): modesAllowed, modesGraves, modesBan, modesBowling, modesPot, modesIZ, modesArt, modesZen.
#pragma once

#include "ConstEnums.h"

class Board;
class Challenge;
class ChallengeDefinition;
class LawnApp;
class Plant;
class SeedPacket;
class Zombie;
class HitResult;
struct PvzpWeightedArray;

namespace pvzu::engine {

bool ModesVanillaOn();

// GetChallengeDefinition: the Unnamed entry (page/row/col/icon edited), or nullptr for the vanilla one.
const ChallengeDefinition* ModesChallengeDefinition(int theChallengeIndex);

// End of Challenge::InitZombieWaves: the mode's Unnamed allow-list edits (hooks 177-180).
void ModesAllowList(Challenge* theChallenge);

// Challenge::CanPlantAt (art challenges): true = this seed also ignores the art picture (hook 175: Grave Buster).
bool ModesArtIgnoresPicture(SeedType theSeedType);

// Challenge::SpawnZombieWave, Grave Danger after wave 5: how many random graves (vanilla 1, hook 184).
int ModesGraveDangerGraves();
// SeedChooserScreen::SeedNotAllowedToPick: true = Unnamed bans the seed in this mode (Grave Danger: Grave Buster).
bool ModesSeedBanned(LawnApp* theApp, SeedType theSeedType);

// Plant::UpdateBowling after mLaunchCounter++: true = the Unnamed reward was given (vanilla coins skipped).
bool ModesBowlingRicochet(Plant* theNut, int thePosX);

// Challenge::ScaryPotterOpenPot, sun pot: true = the sun went straight to the bank (no sun drops).
bool ModesScaryPotDirectSun(Challenge* theChallenge, int theSunCount);

// ---- I, Zombie ----
// Plant::GetCost for I, Zombie cards: false = vanilla price.
bool ModesIZombieCost(SeedType theSeedType, int& theCost);
// Challenge::IZombieSeedTypeToZombieType: the Football card's Newspaper toggle.
ZombieType ModesIZombieCardZombie(SeedType theSeedType, ZombieType theVanilla);
// Board::MouseDown with a card in the cursor, before MouseDownWithPlant: a Dancer/Bungee card under the click takes the
// held card as its companion (hook 78).
void ModesIZombieCarrierClick(Board* theBoard, const HitResult& theHit);
// Board::MouseDown on a seed packet with nothing held: right click (theClickCount <= 0) on Football / Dancer / Bungee.
// true = handled (the packet is not picked up).
bool ModesIZombiePacketClick(Board* theBoard, SeedPacket* thePacket, int theClickCount);
// Board::MouseDown / MouseUp: on the phone a press held 0.5 s on one of those cards counts as the right click.
void ModesIZombiePressDown(Board* theBoard, const HitResult& theHit);
void ModesIZombiePressUp(Board* theBoard, int x, int y);
// Challenge::IZombieMouseDownWithZombie before IZombiePlaceZombie (hook 193): the held card's companion (a Bungee's
// zombie is made here, before the Bungee, as in the exe); ModesIZombiePlaced (inside IZombiePlaceZombie, after
// AddZombieInRow) hands it to the Bungee; ModesIZombieAfterPlace clears what is left.
void ModesIZombieBeforePlace(Board* theBoard, ZombieType theZombieType);
void ModesIZombiePlaced(Zombie* thePlaced);
void ModesIZombieAfterPlace();
// NightEliteInit, Dancer: true = this Dancer is placed with a companion (no roll drawn): random summoner, follower type.
bool ModesForcedDancer(int& theFollowerType);

// Plant::StarFruitFire (FogStarfruitVolleyDone): no 1/3 roll and no extra stars in I, Zombie Endless (0x95b790).
bool ModesStarfruitNoExtraStars(LawnApp* theApp);

// ZenGarden::MouseDownWithTool, glove: false = leave this plant (a Squash that is not idle).
bool ModesGloveCanPickUp(Plant* thePlant);

// ---- V2 (C23): mini-games changed in place in .text ----
// Board::PickZombieWaves, end of the non-Adventure branch: Bowling 30, Bowling 2 / Column / Bungee Blitz 40, Grave
// Danger 30, Whack a Zombie 16 (0x409394..0x40946c, hook 0x79a207).
int ModesNumWaves(LawnApp* theApp, int theVanilla);
// Challenge::UpdateConveyorBelt, inside a vanilla mode's pick branch: the Unnamed picks (Invisi-ghoul 0x423250).
void ModesConveyorPicks(LawnApp* theApp, PvzpWeightedArray* thePicks, int& theCount);
// Board::InitLevel, Whack a Zombie mini-game (not 2-5): true = the Unnamed 10-packet bank was set (hook 34).
bool ModesWhackBank(Board* theBoard);
// Board::GetNumSeedsInBank for Whack a Zombie: 10 in the mini-game (0x41bf32), theVanilla (3) in Adventure 2-5.
int ModesWhackSlots(Board* theBoard, int theVanilla);
// Challenge::WhackAZombieSpawning: zombies per spawn (0x42618f: 4) and the grave fill target (0x426043: 30).
int ModesWhackGroupSize(int theVanilla);
int ModesWhackGraveTarget(int theVanilla);

// ---- Dev ----
bool ModesDevStart(LawnApp* theApp);     // TitleScreen once loaded: -pvzu-dev-mode / -pvzu-dev-challenge-page
void ModesDevTick(Board* theBoard, int thePlayTicks); // DevBoardTick: -pvzu-dev-modes scenarios, play line

} // namespace pvzu::engine
