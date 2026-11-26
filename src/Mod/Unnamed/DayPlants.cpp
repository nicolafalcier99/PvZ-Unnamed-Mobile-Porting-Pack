#include "Mod/Unnamed/DayPlants.h"

#include "Mod/CmdLine.h"
#include "Mod/GameData.h"
#include "Mod/Unnamed/Rules.h"

#include <cstdio>
#include <string>

namespace pvzu::unnamed {

namespace {
constexpr int kCoinSun = 4, kCoinSmallSun = 5, kCoinLargeSun = 6; // engine CoinType (same as the exe)

std::string TierKey(const char* field, int tier) {
	return std::string("dayPlants.fireTier.") + field + std::to_string(tier);
}
}

bool DayPlantsOn() {
	return GameData::Get().Feature("dayPlants");
}

// Peashooter -------------------------------------------------------------------------------------

bool PeashooterRaiseDue(int blinkCountdown) {
	return blinkCountdown <= RuleInt("dayPlants.peashooter.raiseWhenBlinkCountdownAtMost", 1);
}

int PeashooterRaiseRollRange() {
	return RuleInt("dayPlants.peashooter.raiseRollRange", 2);
}

LaunchRaise PeashooterRaise(int roll, int launchRate, int layers) {
	LaunchRaise r{false, launchRate, layers};
	const int step = RuleInt("dayPlants.peashooter.raiseStepCs", 0); // vanilla: never raises
	if (step <= 0 || roll == 0)
		return r;
	const int floor = RuleInt("dayPlants.peashooter.minLaunchRateCs", 25);
	r.raised = true;
	r.launchRate = launchRate - step;
	if (r.launchRate <= floor)
		r.launchRate = floor;
	else
		r.layers = layers + 1;
	return r;
}

int PeashooterLayersGuard(int layers) {
	return layers <= 0 ? 1 : layers; // exe: +0x80 starts at -1 (0x45dc0a) and the guard makes it 1; the port starts at 0
}

ShotTiming PeashooterShotTiming(int launchCounter) {
	const int fireAt = RuleInt("dayPlants.peashooter.fireAtLaunchCounter", -1); // vanilla: off
	if (fireAt < 0)
		return ShotTiming::Vanilla;
	if (launchCounter == fireAt)
		return ShotTiming::FireNow;
	if (launchCounter == fireAt - 1)
		return ShotTiming::EndShot;
	return ShotTiming::Vanilla;
}

FirstAdventureStart PeashooterFirstAdventureStart(bool finishedAdventure) {
	FirstAdventureStart s;
	s.launchRate = RuleInt("dayPlants.peashooter.firstAdventureLaunchRateCs", 0);
	s.layers = RuleInt("dayPlants.peashooter.firstAdventureLayers", 0);
	s.applies = !finishedAdventure && s.launchRate > 0;
	return s;
}

// Sunflower --------------------------------------------------------------------------------------

int SunflowerBonusRollRange() {
	return RuleInt("dayPlants.sunflower.bonusRollRange", 100);
}

bool SunflowerBonusSun(int roll) {
	return roll <= RuleInt("dayPlants.sunflower.bonusMaxRoll", -1);
}

// Cherry Bomb ------------------------------------------------------------------------------------

int CherryMineRollRange() {
	return RuleInt("dayPlants.cherryBomb.mineRollRange", 100);
}

bool CherryBombLeavesMine(int roll, int hp, int maxHp, bool iZombieLevel) {
	if (iZombieLevel)
		return false;
	const int need = RuleInt("dayPlants.cherryBomb.mineRequiresHp", 300);
	if (hp != need || maxHp != need)
		return false;
	return roll <= RuleInt("dayPlants.cherryBomb.mineMaxRoll", -1);
}

// Wall-nut ---------------------------------------------------------------------------------------

int WallnutHealTick(PlantState& s, int hp, int maxHp, bool wallnutBowlingLevel) {
	const int delay = RuleInt("dayPlants.wallnut.healDelayCs", 0); // vanilla: no heal
	if (delay <= 0)
		return hp;
	if (wallnutBowlingLevel && RuleInt("dayPlants.wallnut.healInBowling", 0) == 0)
		return hp;
	if (hp != s.extra) {
		s.counter = 0;
		s.extra = hp;
		return hp;
	}
	s.counter += 1;
	if (s.counter >= delay) {
		hp = maxHp;
		s.counter = 0;
		s.extra = hp;
	}
	return hp;
}

// Potato Mine ------------------------------------------------------------------------------------

int PotatoMineArmingCs() {
	return RuleInt("dayPlants.potatoMine.armingCs", 1500);
}

int PotatoMineBlastRadius() {
	return RuleInt("dayPlants.potatoMine.blastRadiusPx", 60);
}

bool PotatoMineBlastBurns() {
	return RuleInt("dayPlants.potatoMine.blastBurns", 0) != 0;
}

// Snow Pea and fire tiers ------------------------------------------------------------------------

int SnowPeaFireRollRange() {
	return RuleInt("dayPlants.snowPea.fireRollRange", 100);
}

bool SnowPeaFireShot(int roll) {
	return roll <= RuleInt("dayPlants.snowPea.fireMaxRoll", -1);
}

int SnowPeaFireTier() {
	return RuleInt("dayPlants.snowPea.fireTier", 3);
}

int FireTierDamage(int tier, int vanillaDamage) {
	if (tier < 1 || tier > 5)
		return vanillaDamage;
	return RuleInt(TierKey("damage", tier), vanillaDamage);
}

bool FireTierChills(int tier) {
	if (FireTierChillCs() <= 0)
		return false;
	return tier >= RuleInt("dayPlants.fireTier.chillMinTier", 3) && tier <= RuleInt("dayPlants.fireTier.chillMaxTier", 5);
}

int FireTierChillCs() {
	return RuleInt("dayPlants.fireTier.chillCs", 0);
}

int FireTierArmDamage() {
	return RuleInt("dayPlants.fireTier.armBodyDamage", 0);
}

unsigned FireTierArmDamageFlags() {
	return static_cast<unsigned>(RuleInt("dayPlants.fireTier.armDamageFlags", 10));
}

bool FireTierTint(int tier, int& r, int& g, int& b) {
	if (tier < 1 || tier > 5)
		return false;
	const std::string base = "dayPlants.fireTier.tier" + std::to_string(tier) + "Tint";
	r = RuleInt(base + "R", -1);
	g = RuleInt(base + "G", -1);
	b = RuleInt(base + "B", -1);
	return r >= 0 && g >= 0 && b >= 0;
}

// Chomper ----------------------------------------------------------------------------------------

int ChomperDigestCs(int eatenHp) {
	const int lowAdd = RuleInt("dayPlants.chomper.digestLowAdd", -1);
	if (lowAdd < 0)
		return 4000; // vanilla digest time
	if (eatenHp <= RuleInt("dayPlants.chomper.digestSplitHp", 1500))
		return eatenHp * RuleInt("dayPlants.chomper.digestLowMul", 2) + lowAdd;
	const int div = RuleInt("dayPlants.chomper.digestHighDiv", 2);
	return eatenHp / (div > 0 ? div : 1) + RuleInt("dayPlants.chomper.digestHighAdd", 3250);
}

int ChomperDigestCountdown(int digestCs) {
	if (digestCs >= RuleInt("dayPlants.chomper.digestMinCs", 0))
		return digestCs;
	return RuleInt("dayPlants.chomper.digestFallbackCs", 4000);
}

int ChomperSunCoin(int eatenHp) {
	if (eatenHp <= RuleInt("dayPlants.chomper.smallSunMaxHp", 870))
		return kCoinSmallSun;
	if (eatenHp <= RuleInt("dayPlants.chomper.sunMaxHp", 1370))
		return kCoinSun;
	return kCoinLargeSun;
}

int ChomperSunRollRange() {
	return RuleInt("dayPlants.chomper.sunRollRange", 0); // vanilla: no sun
}

bool ChomperDropsSun(int roll, int digestCs) {
	if (ChomperSunRollRange() <= 0 || digestCs <= 0)
		return false;
	return roll <= digestCs;
}

int ChomperSunFlashCs() {
	return RuleInt("dayPlants.chomper.sunFlashCs", 100);
}

bool ChomperRefusesSwallow(int bodyMaxHealth, bool newspaperWithoutPaper, bool football) {
	const int maxHp = RuleInt("dayPlants.chomper.unswallowableMaxHpAbove", 0);
	if (maxHp > 0 && bodyMaxHealth > maxHp)
		return true;
	if (newspaperWithoutPaper && RuleInt("dayPlants.chomper.refuseNewspaperWithoutPaper", 0) != 0)
		return true;
	return football && RuleInt("dayPlants.chomper.refuseFootball", 0) != 0;
}

// Repeater ---------------------------------------------------------------------------------------

int RepeaterDiagRollRange() {
	return RuleInt("dayPlants.repeater.diagRollRange", 100);
}

bool RepeaterDiagonal(int roll) {
	return roll <= RuleInt("dayPlants.repeater.diagMaxRoll", -1);
}

float RepeaterDiagVelX() {
	return RuleFloat("dayPlants.repeater.diagVelX", 2.8211734f);
}

float RepeaterDiagVelY() {
	return RuleFloat("dayPlants.repeater.diagVelY", 1.7864928f);
}

// dev log ----------------------------------------------------------------------------------------

void LogPlantAbility(int tick, std::string_view text) {
	static std::FILE* sFile = nullptr;
	static bool sTried = false;
	if (!sTried) {
		sTried = true;
		if (auto path = CmdOption("-pvzu-log-plants"); path && !path->empty())
			sFile = std::fopen(path->c_str(), "a");
	}
	if (!sFile)
		return;
	std::fprintf(sFile, "%d %.*s\n", tick, static_cast<int>(text.size()), text.data());
	std::fflush(sFile);
}

} // namespace pvzu::unnamed
