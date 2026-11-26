// PVZU: Plan 8 part 3c pure helpers (shadow state over the lawn, Moonflower sun gates, zombie status, mod-plant
// placement) and the saved per-object state they need. Evidence: the research notes (mod-plants-behaviour) 1.2, 1.3, 0x67,
// 0x69; pvz-unnamed-pack/SOURCES-mod-plants.md ("Plan 8 part 3c").
#include "doctest/doctest.h"
#include "Mod/GameData.h"
#include "Mod/ModPack.h"
#include "Mod/Unnamed/ModPlantRules.h"
#include "Mod/Unnamed/ModPlantShadow.h"
#include "Mod/Unnamed/Rules.h"
#include "Mod/Unnamed/State.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <vector>

using namespace pvzu;
using namespace pvzu::unnamed;
namespace fs = std::filesystem;

namespace {
// The shadow keys of rules-mod-plants.json this file needs, plus every key of rules-mod-plants-shadow.json (the last
// test compares the second group with the pack file).
const char* kShadowRulesJson = R"json(
{
	"modPlants.shadow.on": 1,
	"modPlants.shadow.skipModeA": 43,
	"modPlants.shadow.skipModeB": 50,
	"modPlants.shadow.rangeCells": 1,
	"modPlants.shadow.moonflowerPoint": 1,
	"modPlants.shadow.shadowedMoonflowerBonus": 1,
	"modPlants.shadow.thresholdDefault": 1,
	"modPlants.shadow.thresholdMoonflower": 3,
	"modPlants.shadow.thresholdFireDragon": 3,
	"modPlants.shadow.thresholdDreamStarfruit": 3,
	"modPlants.shadow.thresholdCharmedTallnut": 2,
	"modPlants.modSun.on": 1,
	"modPlants.charmedTallnut.actsAsTallnut": 1,
	"modPlants.dreamStarfruit.weakenTintGrey": 48
}
)json";
const char* kNewPackKeysJson = R"json(
{
	"modPlants.shadow.dreamStarfruitFlicker": 6,
	"modPlants.modSun.skipModes": [43, 50, 71, 72],
	"modPlants.modSun.firstTimerFullRange": 1,
	"modPlants.repeaterPoison.tintR": 0,
	"modPlants.repeaterPoison.tintG": 255,
	"modPlants.repeaterPoison.tintB": 64,
	"modPlants.loadedCannon.twoSquares": 1
}
)json";

nlohmann::json AllRules() {
	nlohmann::json j = nlohmann::json::parse(kShadowRulesJson);
	j.update(nlohmann::json::parse(kNewPackKeysJson));
	return j;
}

void SetModPlantsFeature(bool on) {
	GameData::Get().ResetToVanilla();
	if (on) {
		ModPack p;
		p.id = "test-mod-plants-shadow";
		p.features["modPlants"] = true;
		std::string err;
		REQUIRE_MESSAGE(GameData::Get().ApplyPack(p, &err), err);
	}
}

struct ShadowOn {
	ShadowOn() { SetRules(AllRules()); SetModPlantsFeature(true); }
	~ShadowOn() { SetRules(nlohmann::json::object()); SetModPlantsFeature(false); }
};
struct ShadowVanilla {
	ShadowVanilla() { SetRules(AllRules()); SetModPlantsFeature(false); }
	~ShadowVanilla() { SetRules(nlohmann::json::object()); }
};

constexpr int kMoon = 0x69, kShadowPea = 0x50, kDusk = 0x70, kToxic = 0x58, kFireDragon = 0x56, kDream = 0x6d,
	kCharmed = 0x67, kPrimalSun = 0x51, kPeashooter = 0, kTallnut = 23, kLoaded = 0x7f;

ShadowLawnPlant P(int seed, int row, int col, bool shadow = false) {
	ShadowLawnPlant p;
	p.seed = seed;
	p.row = row;
	p.col = col;
	p.shadow = shadow;
	return p;
}
} // namespace

TEST_CASE_FIXTURE(ShadowOn, "Shadow points: a Moonflower in the 3x3 gives 1, a shadowed one 2, others 0 (0xc12bf0)") {
	std::vector<ShadowLawnPlant> lawn = {P(kMoon, 2, 3), P(kShadowPea, 2, 4), P(kDusk, 0, 0)};
	lawn[1].self = true;
	CHECK(ShadowPointsOnLawn(kShadowPea, 2, 4, lawn) == 1);
	lawn[0].shadow = true;
	CHECK(ShadowPointsOnLawn(kShadowPea, 2, 4, lawn) == 2);
	// outside the 3x3 (two columns away) and a non-Moonflower shadow plant next to it give nothing
	std::vector<ShadowLawnPlant> far = {P(kMoon, 2, 2), P(kDusk, 2, 5), P(kShadowPea, 2, 4)};
	far[2].self = true;
	CHECK(ShadowPointsOnLawn(kShadowPea, 2, 4, far) == 0);
	// a squished Moonflower and the plant itself never count
	std::vector<ShadowLawnPlant> squished = {P(kMoon, 1, 4), P(kShadowPea, 2, 4)};
	squished[0].squished = true;
	squished[1].self = true;
	CHECK(ShadowPointsOnLawn(kShadowPea, 2, 4, squished) == 0);
	// a non-shadow plant (Peashooter) never gets points
	CHECK(ShadowPointsOnLawn(kPeashooter, 2, 4, lawn) == 0);
}

TEST_CASE_FIXTURE(ShadowOn, "Shadow points: a Moonflower counts every shadow plant around it, Moonflowers too (0x7b072c)") {
	std::vector<ShadowLawnPlant> lawn = {P(kMoon, 2, 2), P(kToxic, 2, 1), P(kPrimalSun, 2, 3), P(kDusk, 3, 2),
		P(kPeashooter, 1, 2), P(kMoon, 1, 1)};
	lawn[0].self = true;
	CHECK(ShadowPointsOnLawn(kMoon, 2, 2, lawn) == 4); // Toxic, Primal Sunflower, Dusk-pult, the other Moonflower
	CHECK(ShadowStateStep(kMoon, false, 13, 4).shadow);
}

TEST_CASE_FIXTURE(ShadowOn, "Shadow thresholds: 3 Moonflower / Fire-dragon / Dream Starfruit, 2 Charmed Tall-nut, 1 others") {
	CHECK_FALSE(ShadowStateStep(kFireDragon, false, 13, 2).shadow);
	CHECK(ShadowStateStep(kFireDragon, false, 13, 3).shadow);
	CHECK_FALSE(ShadowStateStep(kDream, false, 13, 2).shadow);
	CHECK(ShadowStateStep(kDream, false, 13, 3).shadow);
	CHECK_FALSE(ShadowStateStep(kCharmed, false, 13, 1).shadow);
	CHECK(ShadowStateStep(kCharmed, false, 13, 2).shadow);
	CHECK(ShadowStateStep(kShadowPea, false, 13, 1).shadow);
	CHECK_FALSE(ShadowStateStep(kShadowPea, false, 13, 0).shadow);
	CHECK_FALSE(ShadowStateStep(kPeashooter, false, 13, 9).shadow);
}

TEST_CASE_FIXTURE(ShadowOn, "Shadow step: entering and leaving are reported; Zen Garden / Tree of Wisdom keep the old state") {
	ShadowChange in = ShadowStateStep(kCharmed, false, 13, 2);
	CHECK(in.shadow);
	CHECK(in.entered);
	CHECK_FALSE(in.left);
	ShadowChange stay = ShadowStateStep(kCharmed, true, 13, 2);
	CHECK(stay.shadow);
	CHECK_FALSE(stay.entered);
	ShadowChange out = ShadowStateStep(kCharmed, true, 13, 0);
	CHECK_FALSE(out.shadow);
	CHECK(out.left);
	CHECK(ShadowStateStep(kShadowPea, true, 43, 0).shadow); // 0x7b06c5: no update in mode 0x2b
	CHECK_FALSE(ShadowStateStep(kShadowPea, false, 50, 5).shadow);
	CHECK_FALSE(ShadowStateStep(kShadowPea, false, 50, 5).entered);
}

TEST_CASE_FIXTURE(ShadowOn, "Shadow tint flicker: Dream Starfruit 1 draw in 6 (0xc00e0d), every other shadow plant always") {
	CHECK(ShadowTintFlickerRange(kDream) == 6);
	CHECK(ShadowTintFlickerRange(kShadowPea) == 0);
	CHECK(ShadowTintFlickerRange(kMoon) == 0);
}

TEST_CASE_FIXTURE(ShadowOn, "Moonflower sun: not in Zen Garden, Tree of Wisdom, modes 0x47 / 0x48; first timer Rand(rate + 1)") {
	CHECK(ModSunRunsInMode(0));
	CHECK(ModSunRunsInMode(13));
	CHECK_FALSE(ModSunRunsInMode(43));
	CHECK_FALSE(ModSunRunsInMode(50));
	CHECK_FALSE(ModSunRunsInMode(71));
	CHECK_FALSE(ModSunRunsInMode(72));
	CHECK(ModSunFirstTimerRollRange(kMoon, 2500) == 2501);
	CHECK(ModSunFirstTimerRollRange(kPeashooter, 2500) == 0);
}

TEST_CASE_FIXTURE(ShadowOn, "Repeater poison tint RGB(0, 255, 64) (0xc10d5a); weakened is a 16-bit word (0xc060e9 add word)") {
	CHECK(RepeaterPoisonTint() == 0x00ff40u);
	CHECK(ZombieWeakenAdd(0, 400) == 400);
	CHECK(ZombieWeakenAdd(100, 200) == 300);
	CHECK(ZombieWeakenAdd(32700, 400) == -32436); // the exe word wraps: such a zombie is no longer weakened
}

TEST_CASE_FIXTURE(ShadowOn, "Zombie status tint: poison green first, then weakened grey 48 as colour and additive, wave zombies only") {
	// Zombie::DrawReanim colour chain (0x52d2bf): 0xc10d30 Repeater poison (record +8 > 0) -> RGB(0, 255, 64); else
	// 0x95a365: 0x52ae40 (mFromWave not -2 / -3) and word +0x1a > 0 -> 0x95a377 edx = ecx = eax = 0x30, then 0x52d2d9
	// stores them as the colour AND the additive colour (0x52d382: additive on); else the vanilla chain (mind control...).
	ZombieTint t = ZombieStatusTint(true, 500, 0);
	CHECK(t.on);
	CHECK(t.rgb == 0x00ff40u);
	t = ZombieStatusTint(true, 500, 400); // poisoned and weakened: the poison is tested first
	CHECK(t.on);
	CHECK(t.rgb == 0x00ff40u);
	t = ZombieStatusTint(true, 0, 400);
	CHECK(t.on);
	CHECK(t.rgb == 0x303030u);
	CHECK_FALSE(ZombieStatusTint(false, 0, 400).on); // a cut-scene / UI zombie (mFromWave -2 / -3): no record, no grey
	CHECK_FALSE(ZombieStatusTint(false, 500, 0).on);
	CHECK_FALSE(ZombieStatusTint(true, 0, 0).on);
	CHECK_FALSE(ZombieStatusTint(true, 0, -436).on); // a wrapped word is not weakened (`jle`)
}

TEST_CASE_FIXTURE(ShadowOn, "Nut first aid: a Charmed Tall-nut and a vanilla Tall-nut never replace each other (0x957620)") {
	CHECK(ModNutFirstAidAllowed(kCharmed, kCharmed));
	CHECK_FALSE(ModNutFirstAidAllowed(kCharmed, kTallnut));
	CHECK_FALSE(ModNutFirstAidAllowed(kTallnut, kCharmed));
	CHECK(ModNutFirstAidAllowed(kTallnut, kTallnut));
}

TEST_CASE_FIXTURE(ShadowOn, "Loaded Cannon needs its own square and the next one (port: it covers two columns, hook 40)") {
	CHECK(LoadedCannonNeedsTwoSquares());
	CHECK(LoadedCannonFits(3, 9, true));
	CHECK_FALSE(LoadedCannonFits(3, 9, false));
	CHECK_FALSE(LoadedCannonFits(8, 9, true)); // no column 9
	CHECK(LoadedCannonFits(7, 9, true));
}

TEST_CASE_FIXTURE(ShadowVanilla, "Shadow helpers: no effect while modPlants is off") {
	std::vector<ShadowLawnPlant> lawn = {P(kMoon, 2, 3), P(kShadowPea, 2, 4)};
	lawn[1].self = true;
	CHECK(ShadowPointsOnLawn(kShadowPea, 2, 4, lawn) == 0);
	CHECK_FALSE(ShadowStateStep(kShadowPea, false, 13, 3).shadow);
	CHECK(ShadowTintFlickerRange(kDream) == 0);
	CHECK_FALSE(ModSunRunsInMode(13));
	CHECK(ModSunFirstTimerRollRange(kMoon, 2500) == 0);
	CHECK(RepeaterPoisonTint() == 0u);
	CHECK_FALSE(ZombieStatusTint(true, 500, 400).on);
	CHECK(ZombieWeakenAdd(0, 400) == 0);
	CHECK(ModNutFirstAidAllowed(kCharmed, kTallnut)); // vanilla rule untouched
	CHECK_FALSE(LoadedCannonNeedsTwoSquares());
	CHECK(LoadedCannonFits(8, 9, false));
}

TEST_CASE("Shadow helpers: feature on but no pack rules = no effect") {
	SetRules(nlohmann::json::object());
	SetModPlantsFeature(true);
	CHECK_FALSE(ModSunRunsInMode(13));
	CHECK(ModSunFirstTimerRollRange(kMoon, 2500) == 0);
	CHECK(RepeaterPoisonTint() == 0u);
	CHECK_FALSE(ZombieStatusTint(true, 500, 400).on);
	CHECK_FALSE(LoadedCannonNeedsTwoSquares());
	CHECK(ShadowTintFlickerRange(kDream) == 0);
	SetModPlantsFeature(false);
}

TEST_CASE("Zombie status survives save/load; older blobs read no poison, not weakened") {
	ZombieState z{3, 4, 5, 1, 2, -1};
	z.toxicKind = 2;
	z.toxicTimer = 524000;
	z.repeaterPoisonCs = 1000;
	z.weakenCs = 377;
	auto b = Encode(z);
	CHECK(b.size() == 40);
	ZombieState back;
	REQUIRE(Decode(b.data(), b.size(), back));
	CHECK(back == z);
	ZombieState old; // a 24-byte blob (Plan 3 .. Plan 8 part 2 saves)
	REQUIRE(Decode(b.data(), 24, old));
	CHECK(old.stored == -1);
	CHECK(old.toxicKind == 0);
	CHECK(old.toxicTimer == 0);
	CHECK(old.repeaterPoisonCs == 0);
	CHECK(old.weakenCs == 0);
}

TEST_CASE("Plant shadow state and Charmed Tall-nut bite counter survive save/load; older blobs read 0") {
	PlantState s;
	s.modSeed = 0x67;
	s.shadow = 1;
	s.charmedBitten = 1234;
	s.elementBits = 5;  // the shooter fields sit before the shadow ones (bytes 32..51)
	s.cannonAmmo = 0x22;
	auto b = Encode(s);
	CHECK(b.size() == 60);
	PlantState back;
	REQUIRE(Decode(b.data(), b.size(), back));
	CHECK(back == s);
	PlantState old; // a 32-byte blob (Plan 8 part 1 saves)
	REQUIRE(Decode(b.data(), 32, old));
	CHECK(old.modSeed == 0x67);
	CHECK(old.shadow == 0);
	CHECK(old.charmedBitten == 0);
	PlantState shooters; // a 52-byte blob (Plan 8 part 3a): shooter state, no shadow state
	REQUIRE(Decode(b.data(), 52, shooters));
	CHECK(shooters.elementBits == 5);
	CHECK(shooters.cannonAmmo == 0x22);
	CHECK(shooters.shadow == 0);
	CHECK(shooters.charmedBitten == 0);
}

TEST_CASE("Mod plant shadow pack file says the same as these test values") {
	const char* home = std::getenv("HOME");
	fs::path file = std::getenv("PVZU_PACK_DIR") ? fs::path(std::getenv("PVZU_PACK_DIR"))
	                                              : fs::path(home ? home : "") / "pvz-unnamed-pack";
	file /= "data/rules-mod-plants-shadow.json";
	if (!fs::exists(file)) { MESSAGE("pack not here, skipped: " << file.string()); return; }
	std::ifstream in(file);
	nlohmann::json pack = nlohmann::json::parse(in, nullptr, false);
	REQUIRE(pack.is_object());
	const nlohmann::json want = nlohmann::json::parse(kNewPackKeysJson);
	CHECK(pack == want);
}
