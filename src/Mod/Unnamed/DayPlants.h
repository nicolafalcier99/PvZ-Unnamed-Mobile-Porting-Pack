// PVZU: Unnamed 1.3beta2 Day-plant rules (evidence: the research notes (plants)).
// Pure functions: the engine rolls with its own RNG (Sexy::Rand(range)) and passes the roll in.
// Every constant is a pack rule (pvz-unnamed-pack/data/rules-day-plants.json, keys "dayPlants.*") whose
// fallback is the vanilla behaviour, so a missing key switches that one rule off.
#pragma once

#include "Mod/Unnamed/State.h"

#include <string_view>

namespace pvzu::unnamed {

// Feature switch for every engine site: GameData::Get().Feature("dayPlants") (off in vanilla mode).
bool DayPlantsOn();

// --- Peashooter (study 1a, 1c, 1d) -----------------------------------------------------------
// Checked every UpdateShooting tick: the raise roll happens when the blink countdown is <= 1.
bool PeashooterRaiseDue(int blinkCountdown);
int PeashooterRaiseRollRange();
struct LaunchRaise {
	bool raised = false;
	int launchRate = 0;
	int layers = 0;
};
// roll = Rand(PeashooterRaiseRollRange()); raises when roll != 0: rate -5, floor 25 (layer counted above it).
LaunchRaise PeashooterRaise(int roll, int launchRate, int layers);
int PeashooterLayersGuard(int layers); // exe: layers <= -1 -> 1; the port counter starts at 0, so <= 0 -> 1
enum class ShotTiming { Vanilla, EndShot, FireNow };
// While a Peashooter shot is running: fire at launch counter 1, end the shot at 0.
ShotTiming PeashooterShotTiming(int launchCounter);
struct FirstAdventureStart {
	bool applies = false;
	int launchRate = 0;
	int layers = 0;
};
// Planted from the seed bank while Adventure was never finished: 15 layers, launch rate 80.
FirstAdventureStart PeashooterFirstAdventureStart(bool finishedAdventure);

// --- Sunflower (study 2) ---------------------------------------------------------------------
int SunflowerBonusRollRange();
bool SunflowerBonusSun(int roll); // extra small sun (15)

// --- Cherry Bomb (study 3a) ------------------------------------------------------------------
int CherryMineRollRange();
// On its own fuse only (not crushed): leaves a Potato Mine on its square.
bool CherryBombLeavesMine(int roll, int hp, int maxHp, bool iZombieLevel);

// --- Wall-nut (study 4a) ---------------------------------------------------------------------
// Once per blink tick. s.counter = ticks of unchanged HP, s.extra = last HP. Returns the new HP.
int WallnutHealTick(PlantState& s, int hp, int maxHp, bool wallnutBowlingLevel);

// --- Potato Mine (study 5) -------------------------------------------------------------------
int PotatoMineArmingCs();
int PotatoMineBlastRadius();
bool PotatoMineBlastBurns();

// --- Snow Pea and fire tiers (study 6) -------------------------------------------------------
int SnowPeaFireRollRange();
bool SnowPeaFireShot(int roll);
int SnowPeaFireTier();
// Fireball damage by mod fire tier (0 = untiered: the table's damage).
int FireTierDamage(int tier, int vanillaDamage);
bool FireTierChills(int tier);
int FireTierChillCs();
int FireTierArmDamage();
unsigned FireTierArmDamageFlags();
bool FireTierTint(int tier, int& r, int& g, int& b); // false: keep the vanilla fire-pea look

// --- Chomper (study 7) -----------------------------------------------------------------------
int ChomperDigestCs(int eatenHp);          // T from body + helm + shield HP at the bite
int ChomperDigestCountdown(int digestCs);  // T if T >= 1000 else 2700
int ChomperSunCoin(int eatenHp);           // engine CoinType: 5 small, 4 sun, 6 large
int ChomperSunRollRange();
bool ChomperDropsSun(int roll, int digestCs);
int ChomperSunFlashCs();
bool ChomperRefusesSwallow(int bodyMaxHealth, bool newspaperWithoutPaper, bool football);

// --- Repeater (study 8) ----------------------------------------------------------------------
int RepeaterDiagRollRange();
bool RepeaterDiagonal(int roll); // two extra diagonal peas
float RepeaterDiagVelX();
float RepeaterDiagVelY();        // the down pea uses -VelY

// --- dev log ---------------------------------------------------------------------------------
// Appends "<tick> <text>" to the file given by -pvzu-log-plants <file> (no-op without it).
void LogPlantAbility(int tick, std::string_view text);

} // namespace pvzu::unnamed
