// PVZU: engine sites of the Unnamed mod-only game modes 0x65..0x94 (Mod/Unnamed/ModModes.h, pack data/modes.json,
// pvz-unnamed-pack/SOURCES-mod-modes.md). Every function returns at once ("vanilla") unless the pack feature "modModes"
// is on and LawnApp::mGameMode is a mod mode, so Vanilla mode and the vanilla game modes never change.
#pragma once

#include "ConstEnums.h"
#include "misc/KeyCodes.h"

#include <string>
#include <string_view>

class Board;
class LawnApp;
class ChallengeScreen;
class ChallengeDefinition;
class Plant;
class Zombie;
class Coin;
class PlayerInfo;
class SeedPacket;
class Projectile;
class SeedChooserScreen;
class Challenge;
class GridItem;
struct PvzpWeightedGridArray;
namespace Sexy { class Graphics; class Color; class Image; }
struct PvzpWeightedArray;

namespace pvzu::unnamed { struct ModModeDef; struct Touch; }

namespace pvzu::engine {

bool ModModesOn();
// The row of the running game mode; nullptr outside a mod mode (or with the feature off).
const unnamed::ModModeDef* CurrentModMode(LawnApp* theApp);
bool IsModModeRunning(LawnApp* theApp);
// The name the player reads: English, or the exe's Chinese name with language "zh".
std::string ModModeDisplayName(const unnamed::ModModeDef& theDef);

// LawnApp::GetCurrentChallengeDef: a ChallengeDefinition for a mod mode (level name, house message); nullptr = vanilla.
const ChallengeDefinition* ModModeChallengeDef(LawnApp* theApp);

// Board::PickBackground default branch (hook 25, 0x79a5fb). true = mBackground set.
bool ModModeBackground(Board* theBoard);
// Board::PickBackground plant rows for the garden backgrounds (mushroom garden, aquarium) used as a lawn (hooks 26/27).
bool ModModePlantRows(Board* theBoard);
// Board::InitZombieWaves after Challenge::InitZombieWaves (hook 180, 0x79ac00).
void ModModeAllowed(Board* theBoard);
// Board::PickZombieWaves: the wave count of a mod mode (theVanilla = 40, the generic challenge value).
int ModModeNumWaves(Board* theBoard, int theVanilla);
// Board::GetNumSeedsInBank (hook 142). true = theSlots set.
bool ModModeSeedSlots(Board* theBoard, int& theSlots);
// Board::StageHasGraveStones (hooks 143/144). true = theHas set.
bool ModModeGraves(Board* theBoard, bool& theHas);
// SeedChooserScreen: true = hide this seed in the chooser for the running mod mode (hook 425, 0x7b0f00).
bool ModModeHidesSeed(LawnApp* theApp, int theSeedType);
// Board::ChooseSeedsOnCurrentLevel (hooks 35/36). true = theShow set (false: no chooser).
bool ModModeChooser(Board* theBoard, bool& theShow);
// Board::HasConveyorBeltSeedBank (hook 140) and Challenge::UpdateConveyorBelt's seed list (hook 162).
bool ModModeConveyor(Board* theBoard);
bool ModModeConveyorSeeds(Board* theBoard, PvzpWeightedArray* theArray, int& theCount);
// Board::UpdateSunSpawning (hook 94): true = no falling sun.
bool ModModeNoSkySun(Board* theBoard);
// Board::DrawLevel text colour (hook 130).
void ModModeLevelNameColor(Board* theBoard, Sexy::Color& theColor);
// Engine SeedType of a pack seed name ("SUNFLOWER"); -1 when unknown.
int SeedTypeByName(std::string_view theName);

// Rule sites (each is a no-op outside its mode).
void ModModeInitLevel(Board* theBoard);   // end of Board::InitLevel
void ModModeStartLevel(Board* theBoard);  // Board::StartLevel
void ModModeUpdate(Board* theBoard);      // Board::Update while playing
void ModModeDraw(Board* theBoard, Sexy::Graphics* g); // end of Board::Draw (screen tint, messages)
bool ModModeKeyDown(Board* theBoard, Sexy::KeyCode theKey);

// Board rule sites (ModModeRules.cpp).
bool ModModeRains(LawnApp* theApp);                       // rain without lightning (0x74: hooks 117, 164, 185)
bool ModModeObjectsInvisible(LawnApp* theApp);            // Plant::Plant: mVisible false (0x8a: hook 2, V2: plants only)
// V2: Board::PickZombieWaves after the level multipliers: 0x6e x7 x4, 0x7e x(wave/8+1) x4 (hook 21).
int ModModeWaveBudget(Board* theBoard, int thePointsBefore, int thePointsAfter, int theWave);
// V2: Zombie::ZombieInitialize, with IsLittleTroubleLevel: 0x6e zombies are Little Trouble's (hook 516).
bool ModModeLittleZombies(LawnApp* theApp);
// V2: Plant::Die after mDead = true: 0x80 restarts the dead plant's packet (hook 361); not for an Imitater's morph.
void ModModePlantDied(Plant* thePlant);
struct ModModeImitaterMorphScope { ModModeImitaterMorphScope(); ~ModModeImitaterMorphScope(); }; // Plant::ImitaterMorph
bool ModModeBlocksPlanting(Board* theBoard);              // Board::CanPlantAt after Challenge::CanPlantAt (hook 46)
bool ModModeHidesRenderItem(Board* theBoard, int theRenderObjectType); // Board::DrawGameObjects (hooks 118, 122, 124)
bool ModModeNoWaveGate(Board* theBoard);                  // Board::PickZombieType: no first-wave / points test (hook 42)
bool ModModeBungeeWaves(Board* theBoard);                 // Board::SpawnZombieWave: the Bungee Blitz drop (hook 89)
void ModModeWaveSpawned(Board* theBoard);                 // after Board::SpawnZombieWave in UpdateZombieSpawning (0x68)
bool ModModeFogLight(Board* theBoard);                    // Board::UpdateFog instead of the plant loop (hook 135)
bool ModModeFogColumn(Board* theBoard, int& theColumn);   // Board::LeftFogColumn (hook 145)
void ModModeBlover(Board* theBoard);                      // after Plant::BlowAwayFliers set the fog timer (hook 342)
// Seed packets: before / after SeedPacket::WasPlanted's recharge (hooks 69, 70).
void ModModePacketPlanted(SeedPacket* thePacket);
void ModModePacketRecharge(SeedPacket* thePacket);
// Plants: Plant::UpdateBlink. Tick: true = the plant died (0x6f drift). Blinked: true = handled instead of DoBlink (0x65).
bool ModModePlantBlinkTick(Plant* thePlant);
bool ModModePlantBlinked(Plant* thePlant);
// Projectile::Update start (hook 402).
void ModModeProjectileTick(Projectile* theProjectile);
// Zombies.
void ModModeZombieInit(Zombie* theZombie);                // Zombie::ZombieInitialize after the Little Trouble block
void ModModeZombieAdded(Zombie* theZombie);               // Board::AddZombieInRow after ZombieInitialize (hook 44)
bool ModModeWalksBackwards(Zombie* theZombie);            // Zombie::IsWalkingBackwards (hook 642)
void ModModeZombieHit(Zombie* theZombie, int theDamage);  // Zombie::TakeDamage top (hook 706)
float ModModeAnimRate(Zombie* theZombie, float theRate);  // Zombie::ApplyAnimRate (hook 662)
bool ModModeZombieRowStep(Zombie* theZombie);             // Zombie::UpdateZombiePosition y step; true = handled (hook 615)
bool ModModeZombieBites(Zombie* theZombie, Plant* thePlant); // Zombie::EatPlant after StartEating; true = plant gone (679)
void ModModeDiggerSurfaced(Zombie* theZombie);            // Zombie::UpdateZombieDigger, the Digger comes up (hook 586)
bool ModModeHypnoEatsPlants(Zombie* theZombie);           // Zombie::UpdateEating: hypnotised zombies bite plants (674)

// ---- second pass (ModModeRules2.cpp) ----
void ModModeChooserClosed(Board* theBoard);                 // SeedChooserScreen::CloseSeedChooser end (hook 462; 0x798fdf)
bool ModModeSkySun(Board* theBoard, int& theCountdown);     // Board::UpdateSunSpawning: true = a large sun (0x66, hook 95)
void ModModeCoinFalling(Coin* theCoin, bool theLanded);     // Coin::UpdateFall (0x66, hooks 205/206)
bool ModModeCoinCollected(Coin* theCoin);                   // Coin::Update instead of UpdateCollected; true = handled (208)
bool ModModeZombieWalk(Zombie* theZombie, float theSpeed);  // Zombie::UpdateZombieWalking forward; true = moved (0x6d, 604)
float ModModeZombieYStep(Zombie* theZombie, float theGap);  // Zombie::UpdateZombiePosition: vanilla min(gap, 1) (616/617)
bool ModModeNoElites(Zombie* theZombie);                    // EliteInit: no elite roll (0x6d, 0x7a3081)
bool ModModeLoneWolfKey(Board* theBoard, Sexy::KeyCode theKey); // Board::KeyDown (0x72, hook 138)
bool ModModeTakesTouch(Board* theBoard);                    // 0x72 / 0x8e: taps / swipes move the plant (port addition)
void ModModeTouch(Board* theBoard, const unnamed::Touch& theTouch, int theTapX, int theTapY);
void ModModeFollowPress(Board* theBoard, int x, int y, bool theOnBareLawn); // Board::MouseDown: 0x78 / 0x8b follow a finger
void ModModeFollowLift();                                     // Board::MouseUp: the finger is gone
void ModModeHeadDropped(Zombie* theZombie);                 // Zombie::DropHead (0x72, hook 596)
void ModModeSquashSmash(Plant* thePlant, bool theBefore);   // Plant::UpdateSquash around DoSquashDamage (0x7f/0x90, 297)
void ModModeSquashHit(Plant* thePlant, Zombie* theZombie);  // Plant::DoSquashDamage before TakeDamage (296)
bool ModModeFinalWaveLost(Board* theBoard);                 // Board::NextWaveComing at the final wave; true = lost (96)
void ModModePlantAdded(Plant* thePlant);                    // Board::AddPlant after PlantInitialize (0x88, hook 38)
bool ModModeArtSeed(Board* theBoard, int theGridX, int theGridY, SeedType& theSeed); // GetArtChallengeSeed (168)
bool ModModeArtGrid(LawnApp* theApp);                       // the Art Challenge ghosts and rule run (0x88, hooks 170/174)
bool ModModeArtRefuses(Board* theBoard, int theGridX, int theGridY, SeedType theSeed); // Challenge::CanPlantAt (174)
bool ModModeCarnivalYoung(Projectile* theProjectile);       // CheckForCollision: no puff/off-screen death (0x8b, 376)
bool ModModeCarnivalKeepsStar(Projectile* theProjectile);   // CheckForCollision: no star y death (0x8b, 379)
bool ModModeNonScrolling(LawnApp* theApp);                  // CutScene::IsNonScrollingCutscene (0x8b, 219)
bool ModModeSkipsStarCurves(LawnApp* theApp);               // Projectile::Update: no Starfruit curves (0x8b, 0x95b712)
bool ModModeFreeUpgrades(Board* theBoard);                  // Board::CanPlantAt: upgrade plants need no base (hook 53)
bool ModModeRequirementsMet(Board* theBoard);               // Board::PlantingRequirementsMet: always (0x91, hook 153)
void ModModeSunLabel(Board* theBoard, std::string& theLabel); // the sun counter's text (0x91 "Unlimited", hook 471)
bool ModModeOwnWave(Board* theBoard);                       // Board::SpawnZombieWave: true = spawned (0x92, hook 90)
bool ModModeForcesSummoner(Zombie* theZombie);              // NightEliteInit: Dancer variant 0x104 (0x92, hook 100)
void ModModeChooserDraw(SeedChooserScreen* theChooser, Sexy::Graphics* g); // SeedChooserScreen::Draw (0x92, hook 469)
bool ModModeChooserClick(SeedChooserScreen* theChooser, int x, int y);    // SeedChooserScreen::MouseDown; true = used
// third pass (ModModeRules2.cpp)
bool ModModeDyingHittable(Zombie* theZombie);               // Zombie::EffectedByDamage: DYING still hittable (0x76, 714)
void ModModeZombieRevive(Zombie* theZombie);                // Zombie::TakeDamage: a hit revives (0x76, hook 706)
bool ModModeKeepsCorpse(Zombie* theZombie);                 // Zombie::UpdateDying: no fade (0x76, hook 725)
bool ModModeIsVasebreaker(LawnApp* theApp);                 // LawnApp::IsScaryPotterLevel (0x8f; 0x7d by ruling)
bool ModModeScaryPopulate(Challenge* theChallenge, PvzpWeightedGridArray* theGrid, int theCount); // hook 187 (0x7d)
void ModModePotZombie(Challenge* theChallenge, GridItem* thePot, Zombie* theZombie); // ScaryPotterOpenPot (0x8f, 189)
bool ModModePacketClick(SeedPacket* thePacket);             // SeedPacket::MouseDown: true = handled (0x8d, 0xc007d0)
bool ModModeZengKey(Board* theBoard, Sexy::KeyCode theKey); // Board::KeyDown W/A/S/D (0x8e, hook 138)
bool ModModeRowDamage(Plant* thePlant, Zombie* theZombie, int theDamage, unsigned int theFlags); // DoRowAreaDamage (275)
bool ModModePuffImpact(Projectile* theProjectile, Zombie* theZombie, unsigned int theFlags); // DoImpact (0x8e, 396)
void ModModeWave3(Board* theBoard, int theMode);
void ModModeInit3(Board* theBoard, int theMode);
bool ModModeIsSecondUncle(LawnApp* theApp);                // mod mode 0x83 runs the 7-10 boss (Level70Boss.cpp, 0x7aa100)
bool ModModeIsIZombie(LawnApp* theApp);                    // LawnApp::IsIZombieLevel (0x8c)
int ModModeIZombieColumns(LawnApp* theApp, int theColumns); // Challenge::IZombiePlacePlants (0x8c, hook 194)
void ModModeIZombiePlants(Challenge* theChallenge);         // Challenge::IZombieInitLevel (0x8c, hook 195)
ZombieType ModModeIZombieCard(Board* theBoard, ZombieType theVanilla, SeedType theSeed); // the Imp card (0x8c, 191)
SeedType ModModeImitaterPick(Plant* theImitater);           // Plant::ImitaterMorph (hook 348)
void ModModeBoardGone();                                    // Board::~Board: Color::White back to white (0x69)
void ModModeVisualWhite(Board* theBoard);                   // Board::Draw first / per tick: 0x69 tint only in play
int ModModeItemMode(LawnApp* theApp);                      // the Glove Wars' item mode (0x7aaf60): 0 none
bool ModModeGlovePickUp(Board* theBoard);                   // Board::PickUpTool shovel: the glove (hooks 76, 77)
bool ModModeGloveMouseDown(Board* theBoard, int x, int y, CursorType theCursorType); // Board::MouseDownWithTool (73)
void ModModeGloveMoved(Plant* thePlant);                    // ZenGarden::MovePlant end (hook 479)
Sexy::Image* ModModeShovelImage(Board* theBoard, Sexy::Image* theVanilla); // DrawShovel (hook 132)
bool ModModeHoldsShovel(Board* theBoard);                   // DrawShovel: the slot is empty while held (hook 133)
bool ModModeGloveRefuses(Board* theBoard, int theGridX);    // Board::CanPlantAt (0x90, hook 173)
// The plant on the glove (CURSOR_TYPE_PLANT_FROM_GLOVE), or nullptr when nothing is held or it is dead or gone (an item
// mode's lawn plant can be eaten while held). Port safety, also used by the vanilla Zen Garden sites.
Plant* ModModeGlovePlant(Board* theBoard);
// A gloved lawn plant (mPottedPlantIndex -1, item modes) has no potted-plant record: true and the seed / imitater to draw
// it with (Plant::DrawSeedType); false for a Zen Garden potted plant.
bool ModModeGloveLawnSeed(const Plant* thePlant, SeedType& theSeed, SeedType& theImitater);
// called from ModModeRules.cpp
void ModModeUpdate2(Board* theBoard, int theMode);
void ModModeDraw2(Board* theBoard, Sexy::Graphics* g, int theMode);
void ModModeStart2(Board* theBoard, int theMode);
void ModModeInit2(Board* theBoard, int theMode);

// Challenge screen: the "Unnamed modes" button and the scrolling list (Mod/Engine/ModModesScreen.cpp).
void ModModesAttach(ChallengeScreen* theScreen);
void ModModesDetach(ChallengeScreen* theScreen);
void ModModesOpenList(); // PVZU Plan 9 C1: open the list on the attached screen (test cheats' mode picker)

// PlayerInfo save/load: mod-mode records (mChallengeRecords[100..147]) in their own file, Unnamed only.
void ModModeRecordsLoad(PlayerInfo* thePlayer); // after PlayerInfo::LoadDetails
void ModModeRecordsSave(PlayerInfo* thePlayer); // after PlayerInfo::SaveDetails
void ModModeDeleteUserFiles(PlayerInfo* thePlayer); // end of PlayerInfo::DeleteUserFiles (Unnamed only)

// Dev: -pvzu-game-mode 0x67 starts that mode from the title screen (hidden runs). true = started.
bool ModModeDevStart(LawnApp* theApp);

} // namespace pvzu::engine
