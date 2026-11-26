// PVZU: engine side of Plan 9 R4 (Football gaps, elite-option-0 branches). Each function is called from one `// PVZU:`
// site in Zombie.cpp / Plant.cpp or from the Night / World-7 bridges, and returns the vanilla answer (or does nothing)
// unless the active pack has the feature "eliteGaps". Rules: Mod/Unnamed/EliteGaps.h; evidence:
// the research notes (r4-football) and pvz-unnamed-pack/SOURCES-elite-gaps.md. Log lines (-pvzu-log-zombies) carry "gaps".
// Dev: -pvzu-force-variant FOOTBALL:0 -pvzu-force-subvariant FOOTBALL:0x1A (green) / 0x19 (black), -pvzu-dev-player-level.
#pragma once

#include "Mod/Unnamed/NightZombies.h"

class Reanimation;
class Zombie;

namespace pvzu::engine {

bool EliteGapsOn();

// Zombie::TakeDamage after the weakened doubling: black armour x0.85 / x1.2, floor 1, small hits Rand(5) (hook 705)
int BlackArmourDamageTaken(Zombie* theZombie, int theDamage);
// Zombie::CanBeFrozen: a black Football cannot be frozen (hook 712)
bool BlackArmourRefusesFreeze(Zombie* theZombie);

// NightEliteInit, Football with option 0: the whole hook-489 chain (gold + black, grave rises). false = not handled.
bool FootballDefaultRoll(Zombie* theZombie, int theEliteValue, pvzu::unnamed::NightForm& theForm);

// Zombie::Animate, the not-eating branch: the green Football's event and its per-tick speed refresh (hook 640)
void GreenFootballWalkTick(Zombie* theZombie);
// true = the Night port's chew-site version of the event is off (the event runs in Animate)
bool GreenEventInAnimate();
// Zombie::Update's type list: run UpdateZombieGatlingHead (vanilla: theVanilla = type is the Gatling head) (hook 628)
bool RunsGatlingHeadUpdate(Zombie* theZombie, bool theVanilla);
// UpdateZombieGatlingHead: a Gatling-head Football fires at 8 more counters (hook 575)
bool GatlingFootballExtraShot(Zombie* theZombie);
// HeadPeaFired: false = no three-way spread (a Football's peas, 0xc12f00)
bool HeadPeasMaySpread(Zombie* theZombie);

// NightEliteInit, Newspaper: the profile > 70 form and paper HP (0xc05788), and the I, Zombie roll (0xc05770)
pvzu::unnamed::NightForm NewspaperForm(Zombie* theZombie, const pvzu::unnamed::NightForm& theRolled, int theRoll, bool theAlmanac, int theOption);
void NewspaperPaperHp(Zombie* theZombie);
bool NewspaperRollsInIZombie();

// EliteInit (Normal) / NightEliteInit (Newspaper, Dancer) before their rolls: the football helmet (0xc109bf)
void FootballHelmCheck(Zombie* theZombie, int theOption);
// EliteInit, Normal in I, Zombie: the Rand(7) / Rand(30) coin roll; -1 = not handled (0xc111af)
int IZombieNormalVariant(Zombie* theZombie, int theOption);
// TakeHelmDamage images, DropHelm, Magnet: the helmet's track ("anim_football" on a non-Football, hooks 697 / 699)
const char* FootballHelmTrack(Zombie* theZombie);
// SetupReanimLayers: hide "anim_football" on every plain-layer zombie (hook 519)
void HideFootballHelmTrack(Reanimation* theReanim);
// MergeUnnamedReanimTracks: Unnamed's Zombie.reanim (its anim_football track) is used too
bool MergesPlainZombieReanim();

} // namespace pvzu::engine
