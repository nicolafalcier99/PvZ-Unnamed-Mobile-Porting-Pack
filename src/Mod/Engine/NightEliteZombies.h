// PVZU: engine side of the Unnamed Night zombies (Plan 3 Task 2). Each function is called from one `// PVZU:` site in
// Zombie.cpp / Plant.cpp / Projectile.cpp / Reanimator.cpp / EliteZombies.cpp and returns the vanilla answer (or does
// nothing) unless the active pack has the feature "nightZombies". Rules: Mod/Unnamed/NightZombies.h.
//
// Dev option (with the Day ones, -pvzu-force-variant TYPE:v and -pvzu-log-zombies <file>):
//   -pvzu-force-subvariant <TYPE>:<code>[,...]   second form code (Zombie+0x14C), e.g. FOOTBALL:0x1A, NEWSPAPER:1
//   (a forced type rolls nothing: -pvzu-force-variant FOOTBALL:0 -pvzu-force-subvariant FOOTBALL:0x1A = green only)
#pragma once

#include "ConstEnums.h"

#include <optional>
#include <string>

class Plant;
class Projectile;
class Reanimation;
class ReanimatorDefinition;
class Zombie;
namespace Sexy { class Image; }

namespace pvzu::engine {

bool NightZombiesOn();

// ZombieInitialize (from EliteInit, before the spawn log; off-board zombies such as the street preview only get
// Unnamed's extra Newspaper tracks hidden) and after a saved game is loaded
void NightEliteInit(Zombie* theZombie, bool theOnBoard);
void NightAfterLoad(Zombie* theZombie);
std::string FormName(Zombie* theZombie);   // "knife", "green+gold", Day names for Day types

// Dancers
bool SkipsDancerUpdate(Zombie* theZombie);                 // UpdateActions: the moonwalker never dances
bool DancerSkipsSummonCounterCheck(Zombie* theZombie);     // UpdateZombieDancer: trophy
bool DancerSnapsWithoutCountdown();                        // UpdateZombieDancer: `jo` patch
bool DancerReplacesFollower(Zombie* theDancer, int theSlot); // SummonBackupDancers: true = summon this slot now
// SummonBackupDancer: the random summoner's followers; nullopt = not handled (vanilla backup dancer)
std::optional<ZombieID> SummonNightFollower(Zombie* theDancer, int theRow, int thePosX);

// movement and animation
float NightWalkRate(Zombie* theZombie, float theAnimRate);   // UpdateAnimSpeed _ground branch
float NightAnimRate(Zombie* theZombie, float theAnimRate);   // ApplyAnimRate
bool MoonwalkStartWalk(Zombie* theZombie);                   // StartWalkAnim: true = handled
void MoonwalkResume(Zombie* theZombie);                      // RemoveIceTrap / RemoveButter
const char* NewspaperAnim(Zombie* theZombie, const char* theVanillaAnim);   // knife: anim_gasp1, anim_eat_nopaper1
Sexy::Image* NewspaperMadHead(Zombie* theZombie, Sexy::Image* theVanilla);
void NightBeforeDraw(Zombie* theZombie, Reanimation* theBodyReanim);       // random summoner: white filter

// eating
std::optional<bool> NightBitesThisTick(Zombie* theZombie, int theTicksBetweenEats);
int PreyChecksPerTick(Zombie* theZombie);                    // UpdatePlaying: mad Newspaper checks 3 times
bool SkipsNormalBite(Zombie* theZombie);                     // EatPlant: the knife never bites for 4
void NightChewEffect(Zombie* theZombie, Plant* thePlant);    // AnimateChewEffect: knife mark, green helmet drop
void PlantFlashTick(Plant* thePlant);                        // Plant::Animate after the flash countdown: knife kill
bool SkipsGarlic(Zombie* theZombie);
int ButterCs(Zombie* theZombie, int theVanilla);
int EatZombieDamage(Zombie* theEater, Zombie* theTarget, int theVanilla);
unsigned int EatZombieFlags(unsigned int theVanilla);
void NightZombieStep(Zombie* theZombie);                     // CheckForZombieStep: the gold Football squishes

// damage
bool PaperSoaksBodyDamage(Zombie* theZombie, int theDamage); // TakeBodyDamage: true = handled (paper took it)
bool ShieldAlwaysFirst(Zombie* theZombie);                   // TakeDamage: Tall-nut door
bool SkipsShieldDamageImages(Zombie* theZombie);             // TakeShieldDamage
void AlmanacPaperHit(Zombie* theZombie);                     // TakeShieldDamage: +15 body per paper hit
Sexy::Image* HelmDamageImage(Zombie* theZombie, int theDamageIndex, Sexy::Image* theVanilla); // green helmet

// projectiles (Screen Door reflect)
void DoorReflect(Projectile* theProjectile, Zombie* theZombie, int theDamage, unsigned int theDamageFlags);
bool ProjectileSurvivesDie(Projectile* theProjectile);       // a reflected shot lives through its impact once
bool ReflectedShotCollision(Projectile* theProjectile);      // CheckForCollision: true = handled

// for Night levels (graves): the gold Football's death raises a Football from every grave (0xC03520)
bool GoldRaisesGraveFootballs(Zombie* theZombie);
// for Night levels: the "carnival" count lives in mPvzu.counter; random summoners (0x104) do not count
bool DancerCountsCarnival(Zombie* theDancer);

// reanims: Unnamed-only tracks of Unnamed's own compiled reanim, merged into the vanilla definition at load
void MergeUnnamedReanimTracks(const std::string& theReanimFile, ReanimatorDefinition* theDefinition);
void HideUnnamedOnlyTracks(Reanimation* theReanim, ZombieType theType);   // SetupReanimLayers (Almanac pictures too)

} // namespace pvzu::engine
