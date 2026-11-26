// PVZU: Plan 8 part 3c pure helpers; see ModPlantShadow.h and the research notes (mod-plants-behaviour) 1.2 / 1.3.
#include "Mod/Unnamed/ModPlantShadow.h"

#include "Mod/Unnamed/ModPlantRules.h"
#include "Mod/Unnamed/Rules.h"

namespace pvzu::unnamed {

namespace {
constexpr int kSeedDreamStarfruit = 0x6d, kSeedCharmedTallnut = 0x67, kSeedTallnut = 23;

int MP(const char* key, int off) {
	return ModPlantsOn() ? RuleInt(key, off) : off;
}
} // namespace

int ShadowPointsOnLawn(int selfSeed, int selfRow, int selfCol, const std::vector<ShadowLawnPlant>& lawn) {
	int points = 0;
	for (const ShadowLawnPlant& q : lawn) // 0x7b06d7: every plant on the board
		points += ShadowNeighbourPoints(selfSeed, q.seed, q.shadow, q.squished, q.self, q.row - selfRow, q.col - selfCol);
	return points;
}

ShadowChange ShadowStateStep(int selfSeed, bool wasShadow, int gameMode, int points) {
	ShadowChange c;
	c.shadow = wasShadow;
	if (!IsShadowPlant(selfSeed) || !ShadowUpdateRunsInMode(gameMode)) // 0x7b0660 list, 0x7b06c5 modes
		return c;
	c.shadow = ShadowStateFromPoints(selfSeed, points); // 0x7b075b setge
	c.entered = c.shadow && !wasShadow;               // 0x7b0766: the change hook runs on a change only
	c.left = !c.shadow && wasShadow;
	return c;
}

int ShadowTintFlickerRange(int seed) {
	return seed == kSeedDreamStarfruit ? MP("modPlants.shadow.dreamStarfruitFlicker", 0) : 0;
}

bool ModSunRunsInMode(int gameMode) {
	if (MP("modPlants.modSun.on", 0) == 0 || !ModPlantsOn())
		return false;
	const nlohmann::json* skip = RuleArray("modPlants.modSun.skipModes");
	if (skip == nullptr)
		return false; // no rule: no mod sun
	for (const auto& m : *skip)
		if (m.is_number_integer() && m.get<int>() == gameMode)
			return false;
	return true;
}

int ModSunFirstTimerRollRange(int seed, int launchRate) {
	if (!ModSunPlantProduces(seed) || MP("modPlants.modSun.firstTimerFullRange", 0) == 0 || launchRate < 0)
		return 0;
	return launchRate + 1; // 0x7a4770: Rand(rate + 1) for seeds >= 0x50
}

uint32_t RepeaterPoisonTint() {
	const int r = MP("modPlants.repeaterPoison.tintR", -1), g = MP("modPlants.repeaterPoison.tintG", -1),
		b = MP("modPlants.repeaterPoison.tintB", -1);
	if (r < 0 || g < 0 || b < 0)
		return 0;
	return (static_cast<uint32_t>(r) << 16) | (static_cast<uint32_t>(g) << 8) | static_cast<uint32_t>(b);
}

ZombieTint ZombieStatusTint(bool hasSideRecord, int repeaterPoisonCs, int weakenCs) {
	ZombieTint t;
	if (!ModPlantsOn() || !hasSideRecord)
		return t;
	if (repeaterPoisonCs > 0) { // 0xc10d3c cmp [record+8], 0; jle
		if (const uint32_t c = RepeaterPoisonTint()) {
			t.on = true;
			t.rgb = c;
			return t;
		}
	}
	const int g = MP("modPlants.dreamStarfruit.weakenTintGrey", -1);
	if (weakenCs > 0 && g >= 0) { // 0x95a370 cmp word [esi+0x1a], 0; jle; 0x95a377 0x30 0x30 0x30
		t.on = true;
		t.rgb = (static_cast<uint32_t>(g) << 16) | (static_cast<uint32_t>(g) << 8) | static_cast<uint32_t>(g);
	}
	return t;
}

int ZombieWeakenAdd(int weakenCs, int addCs) {
	if (!ModPlantsOn())
		return weakenCs;
	return static_cast<int16_t>(static_cast<uint16_t>(weakenCs + addCs)); // `add word [esi+0x1a], n`
}

bool ModNutFirstAidAllowed(int cursorSeed, int plantSeed) {
	if (!ModSeedCountsAsTallnut(kSeedCharmedTallnut)) // Charmed Tall-nut off: the vanilla rule stands
		return true;
	const bool cursorNut = cursorSeed == kSeedCharmedTallnut || cursorSeed == kSeedTallnut;
	const bool plantNut = plantSeed == kSeedCharmedTallnut || plantSeed == kSeedTallnut;
	if (!cursorNut || !plantNut)
		return true; // not this rule's business
	return cursorSeed == plantSeed; // 0x40e404: mSeedType == theSeedType
}

bool LoadedCannonNeedsTwoSquares() {
	return MP("modPlants.loadedCannon.twoSquares", 0) != 0;
}

bool LoadedCannonFits(int col, int gridCols, bool rightSquareFree) {
	if (!LoadedCannonNeedsTwoSquares())
		return true;
	return col >= 0 && col + 1 < gridCols && rightSquareFree;
}

} // namespace pvzu::unnamed
