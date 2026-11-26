// PVZU: engine wiring of Unnamed's special (non-shooter) mod plants, Plan 8 part 3b. Rules: Mod/Unnamed/ModPlantRules.h
// (pure, rolls passed in); evidence: the research notes (mod-plants-behaviour), one section per plant (exe addresses there).
//
//   0x51 Primal Sunflower   own sun cycle (0x7b0200): 25 + 15 sun; shadowed 50 + 15 or (Rand(25) < 5) a sun-plant packet
//   0x52 Shrapnel Cherry    Cherry blast + 6 flying shards + a core that plants a Cherry Bomb at age 1400 (0x794000)
//   0x53 Bowling Wall-nut   rolls like a bowling nut on any level, never eaten (hooks 307, 370, 676)
//   0x56 Fire-dragon Grass  flame over 3 rows -40..+240 px, 1800 damage per flame, thaws (0x7b0400)
//   0x59 Golden Sun-shroom  three large suns in 150 cs, then gone; its packet starts recharging (0x7b0ce0, hook 472)
//   0x5b Magic Devourer     Grave Buster life; at countdown 5 the plant under it becomes a free seed packet (0x7b07c0)
//   0x5c Hallucination      the zombie that eats it to 0 HP becomes a 270 HP Imp (0x7a65eb)
//   0x5d Prism-shroom       never fires alone; a click/tap pays 50 sun for a 300 + min(sun,1000)/4 shot (0x7a6014)
//   0x6b Rewind Grass       once, 50 cs after planting: zombies left of x 800 healed and sent back; untouchable
//   0x6e Holy-fire Pumpkin  a Pumpkin (base) whose square the Zomboni cannot freeze; no Pumpkin blast
//   0x73 Strong Coffee Bean wakes its 3x3 one tick before the Coffee special; on death heals the plant under it
//   0x75 Leaf Shield        turns zombie shots in 3 rows back at zombies, damage/8 + 2 HP each (0x7b0da0)
//   0x76 Boss Flower        Marigold timer; each drop is a free seed packet of any of 78 seeds (hooks 291, 292, 310)
//
// Everything is reached only for a planted mod plant (Plant::mPvzu.modSeed != 0) or a shot one of them made
// (Projectile::mPvzu.modShot != 0), so vanilla and ordinary Unnamed plants never get here.
//
// Dev (Mac checks):
//   -pvzu-dev-specials <tick>  at that play tick, lay out the 13 plants with zombies to work on (and log every ability
//                              to -pvzu-log-plants); sun is kept at 2000 until 1500 ticks later (then 20, for the Prism
//                              buzzer); Primal Sunflower / Boss Flower drop every 150 cs or sooner; packet coins older
//                              than 150 cs are picked up and planted through Coin::MouseDown + Board::MouseDown.
//                              -pvzu-dev-specials-shadow: these plants count as shadowed (Primal Sunflower's packets
//                              without Moonflowers). Prism clicks come from -pvzu-input (the log names the point).
#pragma once

#include "ConstEnums.h"

#include <optional>

class Board;
class Plant;
class Projectile;
class Zombie;
namespace Sexy {
class Image;
}

namespace pvzu::engine {

// Dispatch targets of the hooks in ModPlantSites.cpp (same meaning and return values as there).
void SpecialsOnPlanted(Plant* thePlant);
bool SpecialsUpdate(Plant* thePlant);
bool SpecialsFire(Plant* thePlant, Zombie* theTargetZombie, int theRow, int thePlantWeapon);
bool SpecialsDoSpecial(Plant* thePlant);
void SpecialsOnDie(Plant* thePlant);
// Plant::Die of the vanilla Coffee Bean (hook 362 -> 0xc11ce0 takes seed 0x23 too, no mode gate): after waking its
// plant (state DOINGSPECIAL) it heals the top plant under it like Strong Coffee (CoffeeBeanDeathHeal). Plan 9 R9.
void CoffeeBeanDies(Plant* thePlant);

// Plant::MouseDown (hook 339): true = the click was this plant's (Prism-shroom pays and fires, or buzzes).
bool ModPlantClicked(Plant* thePlant);
// Board's hover cursor (hook 54): an awake idle Prism-shroom shows the finger like a ready Cob Cannon.
bool ModPlantShowsFinger(Plant* thePlant);
// Plant::GetPlantAttackRect (hook 370): a Bowling Wall-nut uses the bowling rect on every level.
bool ModPlantBowlingRect(Plant* thePlant);
// Plant::UpdateBowling on any lawn (review fix; the exe's 0x462ce0 tests GridToPixelY(0, row) and rows 0..4): the y a
// rolling nut has when it sits on its row at its x (on the roof it follows the slope), and the board's last row.
// On 5-row flat lawns (every vanilla bowling level) both give the vanilla values.
int BowlingRowY(Plant* thePlant);
int BowlingLastRow(Board* theBoard);
// Plant::Squish, Zombie::FindCatapultTarget, Projectile::FindCollisionTargetPlant: Rewind Grass is never squished, lobbed
// at or hit by zombie shots (hooks 302, 543, 373 / 0x7c0800).
bool ModPlantUntouchable(const Plant* thePlant);
// Zombie::CanTargetPlant (hook 676, and Rewind Grass's eat / smash hooks): false for Bowling Wall-nut and Rewind Grass.
std::optional<bool> ModCanTargetPlant(const Plant* thePlant);
// Vanilla's Hypno-shroom checks (Zombie::EatPlant's instant list, AnimateChewSound's hypnosis) compare the raw seed in
// the exe; a Hallucination-shroom (Hypno base) is eaten as a plain plant.
bool ModPlantEatenAsPlain(const Plant* thePlant);
// Zombie::EatPlant, the bite that takes the plant to 0 HP (0x52fd38 -> 0x7a65eb): Hallucination turns the eater into an
// Imp. Returns the zombie that goes on (the Imp, or theZombie).
Zombie* ModPlantEatenToDeath(Zombie* theZombie, Plant* thePlant);
// Board::FindUmbrellaPlant: a mod plant on an Umbrella Leaf base (Leaf Shield) gives no umbrella cover.
bool ModPlantGivesUmbrellaCover(const Plant* thePlant);

// Projectile::Update after the age step (hook 400): true = the shot is gone (a Shrapnel core planted its Cherry Bomb).
bool SpecialShotUpdate(Projectile* theShot);
// Projectile::CheckForCollision: a zombie shot a Leaf Shield turned looks for zombies, not plants (hook 380 0x795e39).
bool ModShotHitsZombies(const Projectile* theShot);
// Projectile::DoImpact on a zombie, before TakeDamage: Shrapnel's 250 extra + effects. (A Prism beam is a mod shot,
// modType 0x16: ModShotImpact in ModPlantsShooters.h owns its damage and art.)
int SpecialShotImpact(Projectile* theShot, Zombie* theZombie, int theDamage);

// SeedPacket::SetPacketType (hook 472): a Golden Sun-shroom packet starts recharging like an upgrade packet.
bool ModPacketStartsRecharging(SeedType theSeedType, SeedType theImitaterType);

// DevHarness tick: -pvzu-dev-specials.
void SpecialsDevTick(Board* theBoard, int thePlayTicks);

} // namespace pvzu::engine
