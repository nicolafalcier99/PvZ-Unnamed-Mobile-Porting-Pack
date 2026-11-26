#include "Mod/Unnamed/RoofPlants.h"

#include "Mod/GameData.h"
#include "Mod/Unnamed/Rules.h"

#include <cstdlib>

namespace pvzu::unnamed {

namespace {
// engine enums (same numbers as the exe)
constexpr int kSeedScaredy = 13, kSeedStarfruit = 29, kSeedFlowerPot = 33, kSeedMarigold = 38, kSeedMelon = 39,
	kSeedWinterMelon = 44;
constexpr int kZombieFlag = 1, kZombieNewspaper = 5, kZombieCatapult = 22, kZombieBoss = 25;
constexpr int kProjCabbage = 2, kProjKernel = 10, kProjCob = 11, kProjButter = 12;
constexpr int kPhaseDying = 1, kPhaseBurned = 2, kPhaseMowered = 3, kPhaseNewspaperMad = 31, kPhaseDiggerTunneling = 32,
	kPhaseDiggerPauseWithoutAxe = 34, kPhaseImpGettingThrown = 71, kPhaseBalloonFlying = 73, kPhaseBalloonPopping = 74;
constexpr int kStateScaredyScared = 21;
constexpr int kNewspaperBossForm = 0x7f; // 0x7aa130: Newspaper variant 0x7f

// Rule value while the feature is on, else the vanilla value.
int Roof(const char* key, int vanilla) {
	return RoofPlantsOn() ? RuleInt(key, vanilla) : vanilla;
}
float RoofF(const char* key, float vanilla) {
	return RoofPlantsOn() ? RuleFloat(key, vanilla) : vanilla;
}
bool RoofFlag(const char* key) {
	return Roof(key, 0) != 0;
}
}

bool RoofPlantsOn() {
	return GameData::Get().Feature("roofPlants");
}

// Cabbage-pult -------------------------------------------------------------------------------------

int CabbageQuickRollRange() {
	return Roof("roofPlants.cabbage.quickThrowRoll", 0);
}

CabbageQuick CabbageQuickThrow(int roll, int level) {
	CabbageQuick q;
	if (CabbageQuickRollRange() <= 0 || roll != 0) // 0xc05671: test eax,eax / jne skip
		return q;
	q.bigShot = true;
	q.velZ = RoofF("roofPlants.cabbage.bigVelZ", -7.0f);
	q.accZ = RoofF("roofPlants.cabbage.bigAccZ", 0.115f);
	// 0xc10780: level 0x37 (55) skips the plant part
	q.plantCost = level != Roof("roofPlants.cabbage.quickThrowFreeLevel", -1);
	if (q.plantCost) {
		q.launchCounter = Roof("roofPlants.cabbage.quickThrowLaunch", 0);
		q.hpCost = Roof("roofPlants.cabbage.quickThrowHpCost", 0);
		q.flashCs = Roof("roofPlants.cabbage.quickThrowFlashCs", 0);
	}
	return q;
}

int BigCabbageDamage(int projectileType, bool big, int tableDamage) {
	if (!big || projectileType != kProjCabbage) // 0xc05700: type 2 and +0x80 == 1
		return tableDamage;
	return Roof("roofPlants.cabbage.bigDamage", tableDamage);
}

float BigCabbageDrawScale(int projectileType, bool big, float vanillaScale) {
	if (!big || projectileType != kProjCabbage) // hook 408 0xc056c0: +0x80 == 1 -> scale 3.0
		return vanillaScale;
	return RoofF("roofPlants.cabbage.bigDrawScale", vanillaScale);
}

// Kernel-pult --------------------------------------------------------------------------------------

KernelShot KernelPityShot(int counter, int projectileType) {
	KernelShot s{projectileType, counter, -1};
	const int after = Roof("roofPlants.kernel.cobAfterKernels", 0);
	if (after <= 0)
		return s;
	if (counter == after) { // 0x797643: counter == 15 -> Cob, whatever was rolled
		s.projectileType = kProjCob;
	} else if (projectileType == kProjKernel) { // 0x79765c: a kernel adds one
		s.counter = counter + 1;
		return s;
	}
	if (s.projectileType == kProjCob || s.projectileType == kProjButter) { // 0x95a780: [+0x88] = counter, counter = 0
		s.carry = counter;
		s.counter = 0;
	}
	return s;
}

int ButterCs(int zombieType, int phase, int catapultBallsLeft, int gameMode, bool fromProjectile, int streak, int current,
	int vanillaValue) {
	const int base = Roof("roofPlants.kernel.butterBaseCs", 0);
	if (base <= 0)
		return vanillaValue;
	// 0x95a700: a Newspaper still reading (not mad) or a Catapult with balls left: 500, set
	if ((zombieType == kZombieNewspaper && phase != kPhaseNewspaperMad) || (zombieType == kZombieCatapult && catapultBallsLeft != 0))
		return Roof("roofPlants.kernel.butterFlatCs", vanillaValue);
	const int minCs = Roof("roofPlants.kernel.butterMinCs", 400);
	int t = minCs;
	if (gameMode != Roof("roofPlants.kernel.butterFixedMode", -1) && fromProjectile) {
		// do { edx *= 2; n--; } while (n > 0): 2^max(n, 1)
		int pow2 = 1, n = streak;
		do {
			pow2 *= 2;
			--n;
		} while (n > 0 && pow2 < (1 << 24));
		t = base * pow2;
		if (t <= minCs)
			t = minCs;
	}
	return RoofFlag("roofPlants.kernel.butterStacks") ? current + t : t;
}

// Garlic -------------------------------------------------------------------------------------------

int GarlicHp(int vanilla) {
	return Roof("roofPlants.garlic.hp", vanilla);
}

int GarlicPoisonPeriodCs() {
	return Roof("roofPlants.garlic.poisonPeriodCs", 0);
}

bool GarlicPoisonTick(int zombieAge) {
	const int period = GarlicPoisonPeriodCs();
	return period > 0 && zombieAge % period == 0; // 0x7953e0: unsigned age % 40
}

int GarlicPoisonDamage() {
	return Roof("roofPlants.garlic.poisonDamage", 0);
}

bool GarlicPoisonReaches(bool zombieIsBoss, int garlicCol, int garlicRow, int garlicX, int zombieRow, int zombieX) {
	if (GarlicPoisonPeriodCs() <= 0)
		return false;
	if (zombieIsBoss) // 0xc02680: the Boss counts when the Garlic stands in column >= 6
		return garlicCol >= Roof("roofPlants.garlic.poisonBossMinCol", 6);
	const int range = Roof("roofPlants.garlic.poisonRangePx", 0); // 0x794320
	return std::abs(zombieRow - garlicRow) <= 1 && zombieX <= garlicX + range && zombieX >= garlicX - range;
}

int GarlicBiteChill(int chill) {
	const int add = Roof("roofPlants.garlic.biteChillCs", 0);
	if (add <= 0 || chill > Roof("roofPlants.garlic.biteChillCap", 400)) // 0x65137a: jg skip
		return chill;
	return chill + add;
}

int GarlicHypnoRollRange() {
	return Roof("roofPlants.garlic.hypnoRoll", 0);
}

bool GarlicBiteRolls(int zombieType) {
	return GarlicHypnoRollRange() > 0 && zombieType != kZombieFlag && zombieType != kZombieNewspaper; // 0x651390
}

bool GarlicBiteHypnotises(int roll) {
	return GarlicHypnoRollRange() > 0 && roll == 0; // 0x6513a7: cmp eax,0 / jg skip
}

// Marigold -----------------------------------------------------------------------------------------

bool MarigoldShrinks() {
	return RoofFlag("roofPlants.marigold.shrink");
}

bool MarigoldBiteImmune(int seedType) {
	return seedType == kSeedMarigold && RoofFlag("roofPlants.marigold.biteImmune");
}

bool MarigoldCanShrink(int zombieType, int variant, bool dead, bool hypnotised, int bodyMaxHp) {
	if (!MarigoldShrinks() || dead || hypnotised)
		return false;
	if (bodyMaxHp > Roof("roofPlants.marigold.maxTargetHp", 0) || zombieType == kZombieBoss) // 0x7aa320, 0xc011f0
		return false;
	return !(zombieType == kZombieNewspaper && variant == kNewspaperBossForm); // 0x7aa130
}

bool MarigoldInRange(int marigoldRow, int marigoldX, int marigoldWidth, int zombieRow, int zombieX, int zombieRectWidth) {
	if (!MarigoldShrinks() || std::abs(zombieRow - marigoldRow) > 1)
		return false;
	const int front = marigoldX + Roof("roofPlants.marigold.rangeFrontPx", 0) + marigoldWidth; // 0xc01200, 0x6517fc
	const int back = marigoldX - Roof("roofPlants.marigold.rangeBackPx", 0) - zombieRectWidth;  // 0x651808
	return zombieX <= front && zombieX >= back;
}

void MarigoldShrink(ShrinkTarget& t) {
	if (!MarigoldShrinks())
		return;
	const int div = Roof("roofPlants.marigold.shrinkHpDivisor", 1);
	t.scale *= RoofF("roofPlants.marigold.shrinkScale", 1.0f);
	// cdq / and edx,3 / add / sar 2 = C++ integer division (toward 0). +0xe8 (flying max) is not touched.
	for (int* hp : {&t.body, &t.bodyMax, &t.helm, &t.helmMax, &t.shield, &t.shieldMax, &t.flying})
		*hp /= div;
	t.chill += Roof("roofPlants.marigold.chillAdd", 0);
}

// Melon-pult ---------------------------------------------------------------------------------------

namespace {
int VolleyChance(int seedType, int zombiesOnBoard) { // the roll fires when roll < chance
	const int base = Roof("roofPlants.melon.volleyBasePct", 0);
	if (base <= 0 || (seedType != kSeedMelon && seedType != kSeedWinterMelon))
		return -1;
	const int winter = seedType == kSeedWinterMelon ? Roof("roofPlants.melon.volleyWinterPct", 0) : 0;
	return base + winter + Roof("roofPlants.melon.volleyPerZombiePct", 0) * zombiesOnBoard;
}
bool VolleyAlways(int zombiesOnBoard) { // 0x79755d: 4n >= 100 jumps past the roll
	return Roof("roofPlants.melon.volleyPerZombiePct", 0) * zombiesOnBoard >= Roof("roofPlants.melon.volleyAlwaysAt", 1 << 30);
}
}

bool MelonVolleyNeedsRoll(int seedType, int zombiesOnBoard) {
	return VolleyChance(seedType, zombiesOnBoard) >= 0 && !VolleyAlways(zombiesOnBoard);
}

bool MelonVolleyFires(int seedType, int zombiesOnBoard, int roll) {
	const int chance = VolleyChance(seedType, zombiesOnBoard);
	if (chance < 0)
		return false;
	return VolleyAlways(zombiesOnBoard) || roll < chance; // Rand(100) <= 9 + 2 (type - 0x27) + 4n
}

bool MelonVolleyTarget(int zombieType, int phase, bool dead, bool hypnotised, bool sameRow, bool ahead) {
	if (Roof("roofPlants.melon.volleyBasePct", 0) <= 0 || dead || hypnotised || !sameRow || !ahead || zombieType == kZombieBoss)
		return false;
	switch (phase) { // 0x7975c7..0x7975f0
	case kPhaseBalloonFlying: case kPhaseBalloonPopping: case kPhaseDiggerTunneling: case kPhaseDiggerPauseWithoutAxe:
	case kPhaseImpGettingThrown: case kPhaseDying: case kPhaseBurned: case kPhaseMowered:
		return false;
	default:
		return true;
	}
}

// Flower Pot ---------------------------------------------------------------------------------------

bool ZombieShotSkipsPlant(int seedType, int plantState) {
	if (!RoofFlag("roofPlants.flowerPot.zombieShotsSkip"))
		return false;
	// 0xc02c30 (the exe's mod seeds 0x58, 0x59, 0x6d do not exist in this port)
	return (seedType == kSeedScaredy && plantState == kStateScaredyScared) || seedType == kSeedStarfruit || seedType == kSeedFlowerPot;
}

bool NoGraveOnFlowerPot() {
	return RoofFlag("roofPlants.flowerPot.noGraveOnPot");
}

} // namespace pvzu::unnamed
