#include "Mod/Unnamed/PoolPlants.h"

#include "Mod/GameData.h"
#include "Mod/Unnamed/DayPlants.h"
#include "Mod/Unnamed/Rules.h"

#include <cmath>
#include <string>

namespace pvzu::unnamed {

namespace {
// engine enums (same numbers as the exe)
constexpr int kSeedIceshroom = 14, kSeedTanglekelp = 19, kSeedSpikeweed = 21, kSeedTallnut = 23, kSeedSpikerock = 46,
	kSeedImitater = 48;
constexpr int kZombieBungee = 20, kZombieGargantuar = 23, kZombieBoss = 25, kZombieSquashHead = 30, kZombieRedeye = 32;
constexpr int kProjPea = 0, kProjSnowPea = 1, kProjFireball = 6;
constexpr int kBgPool = 2, kBgFog = 3;

// Rule value while the feature is on, else the vanilla value.
int Pool(const char* key, int vanilla) {
	return PoolPlantsOn() ? RuleInt(key, vanilla) : vanilla;
}

bool TorchTiersOn() {
	return Pool("poolPlants.torch.maxTier", 0) > 0;
}

// x87 FISTP with the default control word: round half to even.
int RoundHalfEven(double v) {
	return static_cast<int>(std::nearbyint(v));
}
}

bool PoolPlantsOn() {
	return GameData::Get().Feature("poolPlants");
}

// Squash ------------------------------------------------------------------------------------------

int SquashReach(bool zombieEating) {
	return zombieEating ? Pool("poolPlants.squash.reachEatingPx", 110) : Pool("poolPlants.squash.reachPx", 70);
}

int SquashSurviveRollRange() {
	return Pool("poolPlants.squash.surviveRoll", 100);
}

bool SquashSurvives(int roll) {
	return roll < Pool("poolPlants.squash.surviveBelow", 0); // vanilla: never
}

// Threepeater -------------------------------------------------------------------------------------

int ThreepeaterPeaRollRange() {
	return Pool("poolPlants.threepeater.roll", 0);
}

PeaKind ThreepeaterPeaKind(int roll) {
	if (ThreepeaterPeaRollRange() <= 0)
		return PeaKind::Pea;
	if (roll == Pool("poolPlants.threepeater.snowRoll", -1))
		return PeaKind::Snow;
	if (roll == Pool("poolPlants.threepeater.fireRoll", -1))
		return PeaKind::Fire;
	return PeaKind::Pea;
}

// Tangle Kelp -------------------------------------------------------------------------------------

int TangleKelpDrags() {
	return Pool("poolPlants.tangle.drags", 1);
}

bool TangleKelpReadyAgain(int dragsDone) {
	return dragsDone < TangleKelpDrags();
}

TanglePlacement TangleKelpPlacement(int seedType, bool underPlantIsImitater, bool hasNormalPlant) {
	if (seedType != kSeedTanglekelp || Pool("poolPlants.tangle.anyGround", 0) == 0)
		return TanglePlacement::Vanilla;
	if (underPlantIsImitater || hasNormalPlant)
		return TanglePlacement::NotHere;
	return TanglePlacement::Ok;
}

bool SpikeweedNeedsGround(int seedType) {
	if (seedType == kSeedSpikeweed)
		return Pool("poolPlants.spikeweed.anyGround", 0) == 0;
	return seedType == kSeedSpikerock;
}

RoofPot SpikeweedRoofPot(int seedType, bool hasFlowerPot) {
	if (seedType != kSeedSpikeweed || Pool("poolPlants.spikeweed.anyGround", 0) == 0)
		return RoofPot::Vanilla;
	return hasFlowerPot ? RoofPot::Refused : RoofPot::Ok; // 0x95a805 `cmp byte [esp+0x17],0` (the pot flag)
}

bool TangleKelpSkipsTarget(int zombieType, bool inPool, int bodyMaxHealth) {
	const int maxHp = Pool("poolPlants.tangle.maxTargetHp", 0);
	if (maxHp <= 0)
		return !inPool; // vanilla: pool zombies only
	if (zombieType == kZombieBoss || zombieType == kZombieBungee)
		return true;
	if (inPool)
		return false;
	if (zombieType == kZombieGargantuar || zombieType == kZombieRedeye)
		return true;
	return bodyMaxHealth > maxHp;
}

// Jalapeno ----------------------------------------------------------------------------------------

bool JalapenoBurnsColumn() {
	return Pool("poolPlants.jalapeno.burnsColumn", 0) != 0;
}

int JalapenoColumnLanes(int background) {
	if (!JalapenoBurnsColumn())
		return 0;
	return (background == kBgPool || background == kBgFog) ? 6 : 5;
}

int JalapenoColumnFireDx() {
	return Pool("poolPlants.jalapeno.columnFireDx", 40);
}

int JalapenoIceCutDx() {
	return Pool("poolPlants.jalapeno.iceCutDx", 80);
}

bool JalapenoColumnHits(int rectX, int rectWidth, int col) {
	if (!JalapenoBurnsColumn())
		return false;
	const int first = RoundHalfEven(static_cast<double>(rectX) / 80.0) - 1;
	const int last = RoundHalfEven((static_cast<double>(rectX) + rectWidth) / 80.0) - 1;
	return col >= first && col <= last;
}

int JalapenoIceRollRange() {
	return Pool("poolPlants.jalapeno.iceRoll", 100);
}

bool JalapenoIceEligible(int hp, int maxHp) {
	const int full = Pool("poolPlants.jalapeno.fullHp", 0);
	return full > 0 && hp == full && maxHp == full;
}

bool JalapenoLeavesIce(int roll) {
	return roll <= Pool("poolPlants.jalapeno.iceMax", -1);
}

int JalapenoIceSeed() {
	return kSeedIceshroom;
}

std::vector<BarrageShot> JalapenoBarrage() {
	std::vector<BarrageShot> out;
	if (!PoolPlantsOn())
		return out;
	const nlohmann::json* list = RuleArray("poolPlants.jalapeno.barrage");
	if (!list)
		return out;
	for (const auto& e : *list) {
		if (!e.is_array() || e.size() != 5)
			continue;
		bool ok = true;
		for (const auto& v : e)
			ok = ok && v.is_number();
		if (!ok)
			continue;
		BarrageShot s;
		s.dx = e[0].get<int>();
		s.dy = e[1].get<int>();
		// the exe: vx/vy stored as float32, then the helper does fld vx; fadd/fsub 0.25|0.2; fstp (one float rounding)
		s.vx = e[2].get<float>() + e[4].get<float>();
		s.vy = e[3].get<float>();
		out.push_back(s);
	}
	return out;
}

int JalapenoBarrageConversions() {
	return Pool("poolPlants.jalapeno.barrageConversions", 0);
}

int JalapenoBarrageMotion() {
	return Pool("poolPlants.jalapeno.barrageMotion", 7);
}

int JalapenoBarrageRangeFlags() {
	return Pool("poolPlants.jalapeno.barrageRangeFlags", 1);
}

int JalapenoBarrageRenderOrder() {
	return Pool("poolPlants.jalapeno.barrageRenderOrder", 342525);
}

// Spikeweed ---------------------------------------------------------------------------------------

bool SpikeweedStunEligible(int seedType, bool zombieHasHead, int butteredCounter, int zombiePhase) {
	if (SpikeweedStunCs() <= 0)
		return false;
	if (seedType != kSeedSpikeweed && seedType != kSeedSpikerock)
		return false;
	// phases 1..3 = dying, burned, mowered
	return zombieHasHead && butteredCounter == 0 && (zombiePhase < 1 || zombiePhase > 3);
}

int SpikeweedStunRollRange() {
	return Pool("poolPlants.spikeweed.stunRoll", 3);
}

bool SpikeweedStuns(int roll) {
	return SpikeweedStunCs() > 0 && roll != Pool("poolPlants.spikeweed.stunSkip", 1);
}

int SpikeweedStunCs() {
	return Pool("poolPlants.spikeweed.stunCs", 0);
}

// Torchwood ---------------------------------------------------------------------------------------

TorchAction TorchwoodAction(int projectileType, int fireTier) {
	if (projectileType == kProjPea)
		return TorchAction::ToFireball;
	if (projectileType == kProjSnowPea)
		return TorchAction::ToPea;
	if (projectileType == kProjFireball && TorchTiersOn() && fireTier < Pool("poolPlants.torch.maxTier", 0))
		return TorchAction::ToFireball;
	return TorchAction::None;
}

bool FireballConvert(ProjectileState& s, int hitTorchwoodGridX, int gridX) {
	if (!TorchTiersOn())
		return hitTorchwoodGridX != gridX; // vanilla guard: once per Torchwood column
	if (hitTorchwoodGridX != -1 && !s.converted) {
		// hook 411: a pea a Torchwood just thawed (ConvertToPea set the column) -> tier -1, no column guard
		s.converted = true;
		s.fireTier = -1;
	} else {
		s.converted = true;
		if (hitTorchwoodGridX == gridX)
			return false;
	}
	// hook 412 (inside the vanilla body, after the new fire-pea reanim)
	switch (s.fireTier) {
	case 1: s.fireTier = 2; break;
	case 2: case 4: s.fireTier = 5; break;
	case 3: s.fireTier = 4; break;
	case -1: s.fireTier = 3; break;
	default: s.fireTier = 1; break;
	}
	return true;
}

int SnowPeaFireShotStartTier() {
	return TorchTiersOn() ? -1 : SnowPeaFireTier();
}

bool FireTierLook(int tier, int& r, int& g, int& b) {
	if (tier == 3)
		return FireTierTint(3, r, g, b);
	if (!TorchTiersOn() || (tier != 2 && tier != 4 && tier != 5))
		return false;
	const std::string base = "poolPlants.torch.tier" + std::to_string(tier) + "Tint";
	r = RuleInt(base + "R", -1);
	g = RuleInt(base + "G", -1);
	b = RuleInt(base + "B", -1);
	return r >= 0 && g >= 0 && b >= 0;
}

// Tall-nut ----------------------------------------------------------------------------------------

int TallnutRegenTick(PlantState& s, int hp, int maxHp, bool zenGarden) {
	const int ticks = Pool("poolPlants.tallnut.regenTicks", 0); // vanilla: no regeneration
	if (ticks <= 0 || zenGarden)
		return hp;
	if (hp == s.extra) {
		s.counter += 1;
		if (s.counter < ticks)
			return hp;
		hp += Pool("poolPlants.tallnut.regenHp", 0);
		if (hp > maxHp)
			hp = maxHp;
	}
	s.counter = 0;
	s.extra = hp;
	return hp;
}

int TallnutThornsDamage(int plantSeedType) {
	return plantSeedType == kSeedTallnut ? Pool("poolPlants.tallnut.thorns", 0) : 0;
}

unsigned TallnutThornsFlags() {
	return static_cast<unsigned>(Pool("poolPlants.tallnut.thornsFlags", 0));
}

bool TallnutSmashed(int plantSeedType, int zombieType) {
	if (plantSeedType != kSeedTallnut || TallnutSmashDamage() <= 0)
		return false;
	return zombieType == kZombieGargantuar || zombieType == kZombieRedeye || zombieType == kZombieSquashHead;
}

int TallnutSmashDamage() {
	return Pool("poolPlants.tallnut.smashDamage", 0);
}

} // namespace pvzu::unnamed
