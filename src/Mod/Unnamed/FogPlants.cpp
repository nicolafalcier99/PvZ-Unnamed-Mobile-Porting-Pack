#include "Mod/Unnamed/FogPlants.h"

#include "Mod/GameData.h"
#include "Mod/Unnamed/Rules.h"

#include <cmath>

namespace pvzu::unnamed {

namespace {
// engine enums (same numbers as the exe)
constexpr int kSeedCherry = 2, kSeedCactus = 26, kSeedPumpkin = 30, kSeedGatling = 40, kSeedGloom = 42, kSeedCattail = 43;
constexpr int kZombieZamboni = 12, kZombieCatapult = 22, kZombieGargantuar = 23, kZombieImp = 24, kZombieSquashHead = 30,
	kZombieRedeye = 32;
constexpr int kProjSpike = 8, kProjCob = 11;

// Rule value while the feature is on, else the vanilla value.
int Fog(const char* key, int vanilla) {
	return FogPlantsOn() ? RuleInt(key, vanilla) : vanilla;
}
float FogF(const char* key, float vanilla) {
	return FogPlantsOn() ? RuleFloat(key, vanilla) : vanilla;
}
bool FogFlag(const char* key) {
	return Fog(key, 0) != 0;
}
}

bool FogPlantsOn() {
	return GameData::Get().Feature("fogPlants");
}

// Plantern -----------------------------------------------------------------------------------------

int PlanternFogRadius() {
	return Fog("fogPlants.plantern.fogRadius", 4);
}

int PlanternLaunchCounter(int seedType, int launchRate, int roll, bool planternNear) {
	const int divisor = Fog("fogPlants.plantern.speedupDivisor", 0);
	if (!planternNear || divisor <= 0 || launchRate < Fog("fogPlants.plantern.speedupMinRate", 0))
		return launchRate - roll;
	int rate = launchRate / divisor; // cave 0x651670: rate >>= 1
	if (seedType == kSeedGloom)
		rate = launchRate;           // bonus cancelled
	else if (seedType == kSeedGatling || seedType == kSeedCattail)
		rate += Fog("fogPlants.plantern.speedupAddGatlingCattail", 0);
	return rate - roll;
}

int PlanternHealTickCs() {
	return Fog("fogPlants.plantern.healTickCs", 0);
}

void PlanternHealStep(int& hp, int& maxHp) {
	if (PlanternHealTickCs() <= 0)
		return;
	if (hp < (maxHp >> 1)) // 0xc06190
		maxHp -= Fog("fogPlants.plantern.maxHpDecayBelowHalf", 0);
	if (hp < maxHp)        // 0x95b8b2
		hp += Fog("fogPlants.plantern.healPerTick", 0);
}

// Cactus -------------------------------------------------------------------------------------------

bool CactusSecondVolley(int launchCounter) {
	const int at = Fog("fogPlants.cactus.secondVolleyAtCounter", -1);
	return at >= 0 && launchCounter == at;
}

bool CactusShotAnim(float& animRate, int& shootingCounter) {
	const int counter = Fog("fogPlants.cactus.shootingCounter", 0);
	if (counter <= 0)
		return false;
	animRate = FogF("fogPlants.cactus.animRate", 35.0f);
	shootingCounter = counter;
	return true;
}

bool CactusBecomesSpikeweed(int plantSeedType, int zombieType, bool alreadySquished) {
	return plantSeedType == kSeedCactus && !alreadySquished && FogFlag("fogPlants.cactus.toSpikeweed") &&
		(zombieType == kZombieZamboni || zombieType == kZombieCatapult);
}

bool SpikeSurvivesHit(ProjectileState& s, int projectileType) {
	if (projectileType != kProjSpike || s.pierceHits >= Fog("fogPlants.cactus.spikePierceMax", 0))
		return false;
	s.pierceHits += 1;
	return true;
}

// Blover -------------------------------------------------------------------------------------------

bool BloverStorm() {
	return FogFlag("fogPlants.blover.storm");
}

BloverHit BloverOnZombie(int zombieType, bool mindControlled, bool flyingBalloon) {
	BloverHit h;
	if (!BloverStorm() || mindControlled)
		return h;
	h.touched = true;
	h.blowAway = zombieType == kZombieImp;
	h.damage = Fog("fogPlants.blover.damage", 0);
	h.blowIfArmless = true;
	h.balloonChillCs = flyingBalloon ? Fog("fogPlants.blover.balloonChillCs", 0) : 0;
	return h;
}

// Split Pea ----------------------------------------------------------------------------------------

SplitShot SplitPeaShotKind(bool behindPlant) {
	if (!FogFlag("fogPlants.splitPea.fireFront"))
		return SplitShot::Pea;
	return behindPlant ? SplitShot::Snow : SplitShot::Fire;
}

int SplitPeaSweepRollRange() {
	return Fog("fogPlants.splitPea.sweepRoll", 0);
}

bool SplitPeaStartsSweep(int roll) {
	return SplitPeaSweepRollRange() > 0 && roll == 0;
}

int SplitPeaInitialSweepTimer() {
	return SplitPeaSweepRollRange() > 0 && RuleInt("fogPlants.splitPea.sweepOnPlant", 0) == 1 ? 0 : -1;
}

bool SplitPeaSweeping(const PlantState& s) {
	return SplitPeaSweepRollRange() > 0 && s.sweepTimer != -1;
}

void SplitPeaSweepTick(PlantState& s) {
	if (!SplitPeaSweeping(s))
		return;
	// hook 314 (0xc11b00), once per Plant::Update
	s.sweepTimer += 1;
	s.sweepAngle += 1;
	if (s.sweepAngle > Fog("fogPlants.splitPea.sweepFallAbove", 96))
		s.sweepDir = 1;
	if (s.sweepAngle < Fog("fogPlants.splitPea.sweepRiseBelow", 31))
		s.sweepDir = 0;
	if (s.sweepDir == 1)
		s.sweepAngle -= 2;
	s.sweepStep += 1;
	if (s.sweepStep >= Fog("fogPlants.splitPea.sweepSteps", 6))
		s.sweepStep = 0;
}

int SplitPeaSweepLaunchCounter() {
	return Fog("fogPlants.splitPea.sweepLaunchCounter", 0);
}

bool SplitPeaSweepFiresNow(const PlantState& s) {
	return SplitPeaSweeping(s) && s.sweepStep == Fog("fogPlants.splitPea.sweepFireStep", 3);
}

bool SplitPeaSweepEnds(PlantState& s) {
	if (!SplitPeaSweeping(s) || s.sweepTimer < Fog("fogPlants.splitPea.sweepDurationCs", 500))
		return false;
	s.sweepTimer = -1; // hook 278 (0xc11990): 499 < timer -> -1
	return true;
}

void SplitPeaSweepVelocity(int angle, float& vx, float& vy) {
	// hook 354 (0xc11a70), x87: fsin((phase - angle) / div) / speedDiv, stored as float32
	const double phase = FogF("fogPlants.splitPea.sweepPhase", -31.4f);
	const double div = FogF("fogPlants.splitPea.sweepDiv", 20.0f);
	const double speedDiv = FogF("fogPlants.splitPea.sweepSpeedDiv", 0.15f);
	const double a = (phase - static_cast<double>(angle)) / div;
	vx = static_cast<float>(std::sin(a) / speedDiv);
	vy = static_cast<float>(std::cos(a) / speedDiv);
}

int SplitPeaSweepMotion() {
	return Fog("fogPlants.splitPea.sweepMotion", 0);
}

int SplitPeaSweepRangeFlags() {
	return Fog("fogPlants.splitPea.sweepRangeFlags", 0);
}

// Starfruit ----------------------------------------------------------------------------------------

float StarfruitStarSpeedMult() {
	return FogF("fogPlants.starfruit.starSpeedMult", 1.0f);
}

int StarfruitStarCurve() {
	return Fog("fogPlants.starfruit.returnAt", 0) > 0 ? kStarCurveBoomerang : kStarCurveNone;
}

int StarfruitExtraRollRange() {
	return Fog("fogPlants.starfruit.extraStarsRoll", 0);
}

bool StarfruitExtraStars(int roll) {
	return StarfruitExtraRollRange() > 0 && roll == 0;
}

int StarfruitExtraStarCount() {
	return Fog("fogPlants.starfruit.extraStars", 0);
}

int StarfruitExtraStarCurve(int i) {
	return kStarCurveSpiral0 + i; // 0x95b14f: +0x88 = the loop index
}

bool StarCurveStep(int curve, int age, float& vx, float& vy) {
	if (!FogPlantsOn())
		return false;
	if (curve == kStarCurveBoomerang) { // 0x95b1ab: +0x88 == 276
		const int a = Fog("fogPlants.starfruit.returnAt", 0), b = Fog("fogPlants.starfruit.returnAgainAt", 0);
		if (age <= 0 || (age != a && age != b))
			return false;
		vx = -vx;
		vy = -vy;
		return true;
	}
	const int tag = curve - kStarCurveSpiral0; // 0x95b1d9: 0 <= +0x88 <= 4
	if (curve < kStarCurveSpiral0 || tag > Fog("fogPlants.starfruit.spiralMaxTag", -1))
		return false;
	// x87 as in the exe: theta = age * 0.05f + (tag - 1) * pi / 2; vx uses the 80-bit theta, vy the float one (fst [esp])
	const double kPi = 3.14159265358979323846;
	const double speed = static_cast<double>(FogF("fogPlants.starfruit.spiralSpeed", 0.0f));
	const double theta = age * static_cast<double>(FogF("fogPlants.starfruit.spiralTurn", 0.0f)) + (tag - 1) * (kPi / 2);
	vx = static_cast<float>(std::cos(theta) * age * speed);
	vy = static_cast<float>(std::sin(static_cast<double>(static_cast<float>(theta))) * age * speed);
	return true;
}

int StarPierceLimit() {
	return Fog("fogPlants.starfruit.pierceLimit", 0);
}

bool StarSurvivesDie(ProjectileState& s, int limit) {
	if (s.pierceHits + 1 >= limit) // cmp dword [+0x18], limit; jge die (the counter is pierceHits + 1)
		return false;
	s.pierceHits += 1;
	return true;
}

// Pumpkin ------------------------------------------------------------------------------------------

bool PumpkinExplodesWhenBitten(int plantSeedType, int hpBeforeBite) {
	const int at = Fog("fogPlants.pumpkin.explodeAtHp", 0);
	return plantSeedType == kSeedPumpkin && at > 0 && hpBeforeBite <= at;
}

bool PumpkinExplodesWhenSmashed(int plantSeedType, int zombieType) {
	return plantSeedType == kSeedPumpkin && FogFlag("fogPlants.pumpkin.explodesWhenSmashed") &&
		(zombieType == kZombieGargantuar || zombieType == kZombieRedeye || zombieType == kZombieSquashHead);
}

int PumpkinExplodeSeed() {
	return kSeedCherry;
}

// Magnet-shroom, Cattail ---------------------------------------------------------------------------

bool MagnetAllRows() {
	return FogFlag("fogPlants.magnet.allRows");
}

bool MagnetUnlimitedRadius() {
	return FogFlag("fogPlants.magnet.unlimitedRadius");
}

int CattailCobRollRange() {
	return Fog("fogPlants.cattail.cobRoll", 0);
}

bool CattailFiresCob(int roll) {
	return CattailCobRollRange() > 0 && roll < Fog("fogPlants.cattail.cobBelow", 0);
}

int CattailCobProjectile() {
	return kProjCob;
}

} // namespace pvzu::unnamed
