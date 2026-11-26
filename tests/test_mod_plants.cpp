// PVZU: Unnamed's 26 mod-only plants (seeds 0x50..0x7f, exe table 0x860000) - Plan 8 part 1 infrastructure.
// Evidence: the research notes (w6-w7-and-new-content) 1.3, 5, 5.1, 7.2, 7.3. The table below is the study's table 5.1,
// transcribed separately; the last tests compare it with the pack's data/mod-plants.json and rules-mod-plants.json.
#include "doctest/doctest.h"
#include "Mod/Unnamed/ModPlants.h"
#include "Mod/Unnamed/Rules.h"
#include "Mod/Unnamed/State.h"
#include "Mod/Mode.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace pvzu::unnamed;
using nlohmann::json;

namespace {
const std::vector<std::string> kEngineNames = {
	"PEASHOOTER", "SUNFLOWER", "CHERRYBOMB", "WALLNUT", "POTATOMINE", "SNOWPEA",
	"CHOMPER", "REPEATER", "PUFFSHROOM", "SUNSHROOM", "FUMESHROOM", "GRAVEBUSTER",
	"HYPNOSHROOM", "SCAREDYSHROOM", "ICESHROOM", "DOOMSHROOM", "LILYPAD", "SQUASH",
	"THREEPEATER", "TANGLEKELP", "JALAPENO", "SPIKEWEED", "TORCHWOOD", "TALLNUT",
	"SEASHROOM", "PLANTERN", "CACTUS", "BLOVER", "SPLITPEA", "STARFRUIT",
	"PUMPKINSHELL", "MAGNETSHROOM", "CABBAGEPULT", "FLOWERPOT", "KERNELPULT", "INSTANT_COFFEE",
	"GARLIC", "UMBRELLA", "MARIGOLD", "MELONPULT", "GATLINGPEA", "TWINSUNFLOWER",
	"GLOOMSHROOM", "CATTAIL", "WINTERMELON", "GOLD_MAGNET", "SPIKEROCK", "COBCANNON",
	"IMITATER", "EXPLODE_O_NUT", "GIANT_WALLNUT", "SPROUT", "LEFTPEATER",
};

struct Row {
	int seed;
	const char* base;
	const char* nameEn;
	const char* nameZh;
	int cost;
	int recharge;
	int launch;
	bool shooter;
	std::array<int, 4> tint;
};

// Study 5.1 (cost, recharge, shooter/launch, name) + out/unnamed-tables.json (tint +0x24..+0x30).
const std::vector<Row> kTable = {
	{0x50, "PEASHOOTER", "Shadow Peashooter", "暗影豌豆射手", 150, 1500, 200, true, {80, 180, 70, 255}},
	{0x51, "SUNFLOWER", "Primal Sunflower", "原始向日葵", 75, 750, 2500, false, {210, 180, 60, 255}},
	{0x52, "CHERRYBOMB", "Shrapnel Cherry", "碎片樱桃", 250, 5500, 0, false, {100, 80, 50, 255}},
	{0x53, "WALLNUT", "Bowling Wall-nut", "保龄球坚果", 50, 2400, 0, false, {255, 150, 255, 255}},
	{0x55, "SNOWPEA", "Mystic-Ice Shooter", "玄冰射手", 200, 7500, 500, true, {0, 0, 255, 255}},
	{0x56, "CHOMPER", "Fire-dragon Grass", "火龙草", 250, 750, 0, false, {255, 128, 50, 255}},
	{0x57, "REPEATER", "Primal Repeater", "原始双发射手", 250, 750, 150, true, {0, 128, 64, 255}},
	{0x58, "PUFFSHROOM", "Toxic Shadow-shroom", "毒影菇", 25, 3000, 0, false, {80, 180, 70, 255}},
	{0x59, "SUNSHROOM", "Golden Sun-shroom", "黄金阳光菇", 0, 9000, 0, false, {225, 195, 170, 255}},
	{0x5b, "GRAVEBUSTER", "Magic Devourer", "魔术吞噬者", 50, 1500, 0, false, {200, 100, 80, 255}},
	{0x5c, "HYPNOSHROOM", "Hallucination-shroom", "致幻菇", 50, 3000, 0, false, {75, 125, 140, 255}},
	{0x5d, "SCAREDYSHROOM", "Prism-shroom", "棱镜菇", 50, 750, 0, false, {180, 200, 100, 240}},
	{0x67, "TALLNUT", "Charmed Tall-nut", "魅惑高坚果", 225, 3000, 0, false, {202, 73, 219, 255}},
	{0x69, "PLANTERN", "Moonflower", "月光花", 100, 3000, 2500, false, {50, 180, 70, 255}},
	{0x6b, "BLOVER", "Rewind Grass", "逆时草", 75, 6000, 0, false, {160, 80, 120, 255}},
	{0x6d, "STARFRUIT", "Dream Starfruit", "幻梦杨桃", 375, 4250, 250, true, {51, 161, 201, 255}},
	{0x6e, "PUMPKINSHELL", "Holy-fire Pumpkin", "圣火南瓜头", 200, 2400, 0, false, {240, 35, 0, 255}},
	{0x70, "CABBAGEPULT", "Dusk-pult", "幽暮投手", 200, 1500, 300, true, {80, 210, 200, 255}},
	{0x72, "KERNELPULT", "Butter-pult", "黄油投手", 175, 2400, 400, true, {210, 200, 50, 255}},
	{0x73, "INSTANT_COFFEE", "Strong Coffee Bean", "强效咖啡豆", 150, 1500, 0, false, {125, 180, 150, 255}},
	{0x75, "UMBRELLA", "Leaf Shield", "葉子盾护", 25, 1500, 0, false, {215, 80, 120, 255}},
	{0x76, "MARIGOLD", "Boss Flower", "大佬花", 475, 12000, 2400, false, {218, 112, 214, 255}},
	{0x78, "GATLINGPEA", "Sniper", "狙击射手", 350, 3000, 0, false, {120, 80, 250, 255}},
	{0x7a, "GLOOMSHROOM", "Element-shroom", "元素菇", 325, 3000, 200, true, {240, 30, 50, 255}},
	{0x7b, "CATTAIL", "Spirit-tail", "灵尾草", 400, 10000, 300, true, {128, 0, 128, 255}},
	{0x7f, "COBCANNON", "Loaded Cannon", "装填加农炮", 600, 6000, 0, false, {255, 165, 0, 255}},
};

json TableJson() {
	json t = json::object();
	for (const Row& r : kTable) {
		t[std::to_string(r.seed)] = {{"base", r.base}, {"name", {{"en", r.nameEn}, {"zh", r.nameZh}}},
			{"tooltip", {{"en", std::string("tip ") + r.nameEn}, {"zh", "zh tip"}}}, {"seedCost", r.cost},
			{"refreshTime", r.recharge}, {"launchRate", r.launch}, {"subclass", r.shooter ? "shooter" : "normal"},
			{"tint", {r.tint[0], r.tint[1], r.tint[2], r.tint[3]}}};
	}
	return t;
}

struct TableGuard {
	TableGuard() {
		std::string err;
		REQUIRE(SetModPlants(TableJson(), kEngineNames, &err));
		SetRules(json{{"modPlants.sproutBankRoll", 48}, {"modPlants.sproutCoinRoll", 53}});
	}
	~TableGuard() {
		ClearModPlants();
		SetRules(json::object());
	}
};

fs::path PackDir() {
	const char* home = std::getenv("HOME");
	return std::getenv("PVZU_PACK_DIR") ? fs::path(std::getenv("PVZU_PACK_DIR"))
	                                     : fs::path(home ? home : "") / "pvz-unnamed-pack";
}
} // namespace

TEST_CASE("ModPlantsSeedRangeAndBase") {
	CHECK_FALSE(IsModSeedRange(0x4f));
	CHECK(IsModSeedRange(0x50));
	CHECK(IsModSeedRange(0x7f));
	CHECK_FALSE(IsModSeedRange(0x80));
	CHECK_FALSE(IsModSeedRange(-1));
	CHECK(ModBaseSeed(0x50) == 0);   // Shadow Peashooter -> Peashooter
	CHECK(ModBaseSeed(0x67) == 23);  // Charmed Tall-nut -> Tall-nut
	CHECK(ModBaseSeed(0x7f) == 47);  // Loaded Cannon -> Cob Cannon
	CHECK(ModBaseSeed(0x54) == 4);   // empty slot still maps (never planted)
	CHECK(ModBaseSeed(7) == 7);
	CHECK(ModBaseSeed(-1) == -1);
}

TEST_CASE("ModPlantsTableHasThe26Plants") {
	TableGuard t;
	REQUIRE(ModPlantSeeds().size() == 26);
	for (const Row& r : kTable) {
		CAPTURE(r.seed);
		const ModPlant* p = FindModPlant(r.seed);
		REQUIRE(p != nullptr);
		CHECK(p->seed == r.seed);
		CHECK(p->base == r.seed - 0x50);
		CHECK(kEngineNames[p->base] == r.base);
		CHECK(p->nameEn == r.nameEn);
		CHECK(p->nameZh == r.nameZh);
		CHECK(p->seedCost == r.cost);
		CHECK(p->refreshTime == r.recharge);
		CHECK(p->launchRate == r.launch);
		CHECK(p->shooter == r.shooter);
		CHECK(p->tint == r.tint);
	}
	for (int aEmpty : {0x54, 0x5a, 0x5e, 0x66, 0x68, 0x6a, 0x6c, 0x6f, 0x71, 0x74, 0x77, 0x79, 0x7c, 0x7e})
		CHECK(FindModPlant(aEmpty) == nullptr);
	CHECK(FindModPlant(0) == nullptr);
	CHECK(FindModPlant(0x80) == nullptr);
	ClearModPlants();
	CHECK(FindModPlant(0x50) == nullptr);
	CHECK(ModPlantSeeds().empty());
}

TEST_CASE("ModPlantsNamesFollowTheLanguage") {
	TableGuard t;
	const ModPlant* p = FindModPlant(0x78);
	REQUIRE(p != nullptr);
	CHECK(ModPlantName(*p, "en") == "Sniper");
	CHECK(ModPlantName(*p, "zh") == "狙击射手");
	CHECK(ModPlantName(*p, "xx") == "Sniper");
	CHECK(ModPlantToolTip(*p, "en") == "tip Sniper");
	CHECK(ModPlantToolTip(*p, "zh") == "zh tip");
}

TEST_CASE("ModPlantsBadTablesKeepNoRows") {
	std::string err;
	json bad = TableJson();
	bad["80"]["base"] = "SUNFLOWER"; // base must be seed - 0x50
	CHECK_FALSE(SetModPlants(bad, kEngineNames, &err));
	CHECK(err.find("80") != std::string::npos);
	CHECK(ModPlantSeeds().empty());
	bad = TableJson();
	bad["79"] = bad["80"]; // outside 0x50..0x7f
	CHECK_FALSE(SetModPlants(bad, kEngineNames, &err));
	bad = TableJson();
	bad["81"].erase("seedCost");
	CHECK_FALSE(SetModPlants(bad, kEngineNames, &err));
	bad = TableJson();
	bad["82"]["tint"] = {1, 2, 3};
	CHECK_FALSE(SetModPlants(bad, kEngineNames, &err));
	CHECK(SetModPlants(json::object(), kEngineNames, &err)); // empty table: fine, nothing
	CHECK(ModPlantSeeds().empty());
}

TEST_CASE("CardHasSeedTypeHook256") {
	TableGuard t;
	// Below the Imitater and the Imitater itself: always the vanilla answer.
	for (int s : {0, 20, 47, 48}) {
		CHECK_FALSE(CardHasSeedType(s, true, true).has_value());
		CHECK_FALSE(CardHasSeedType(s, false, true).has_value());
	}
	// CARD on + Imitater bought: 49..52 and the mod plants.
	for (int s : {49, 50, 51, 52, 0x50, 0x67, 0x7f})
		CHECK(CardHasSeedType(s, true, true) == std::optional<bool>(true));
	// CARD on, no Imitater: none of them.
	for (int s : {49, 52, 0x50, 0x7f})
		CHECK(CardHasSeedType(s, true, false) == std::optional<bool>(false));
	// The gap 53..79 and empty mod slots: never (the exe's chooser skips them).
	for (int s : {53, 60, 79, 0x54, 0x7e})
		CHECK(CardHasSeedType(s, true, true) == std::optional<bool>(false));
	// CARD off: 49..52 keep the vanilla answer, mod seeds are never available.
	for (int s : {49, 52})
		CHECK_FALSE(CardHasSeedType(s, false, true).has_value());
	for (int s : {53, 0x50, 0x7f, 0x80})
		CHECK(CardHasSeedType(s, false, true) == std::optional<bool>(false));
}

TEST_CASE("CardChooserLayout") {
	TableGuard t;
	CHECK(ChooserSeedEnd(false) == kVanillaChooserSeeds);
	CHECK(ChooserSeedEnd(true) == kModSeedEnd);
	CHECK(CardChooserSlotCount() == 30);
	CHECK(CardChooserSlot(49) == 0);
	CHECK(CardChooserSlot(52) == 3);
	CHECK(CardChooserSlot(0x50) == 4);
	CHECK(CardChooserSlot(0x53) == 7);
	CHECK(CardChooserSlot(0x55) == 8);  // 0x54 is empty: no slot
	CHECK(CardChooserSlot(0x7f) == 29);
	CHECK(CardChooserSlot(0x54) == -1);
	CHECK(CardChooserSlot(48) == -1);
	CHECK(CardChooserSlot(0) == -1);
	CHECK(CardChooserSlot(60) == -1);
	// 8 columns, 53 px apart from x 22; rows like the vanilla chooser (7 rows: 70 px from 123, else 73 from 128).
	CHECK(CardChooserSpot(0, true).x == 22);
	CHECK(CardChooserSpot(0, true).y == 123);
	CHECK(CardChooserSpot(7, true).x == 22 + 7 * 53);
	CHECK(CardChooserSpot(8, true).x == 22);
	CHECK(CardChooserSpot(8, true).y == 123 + 70);
	CHECK(CardChooserSpot(29, false).y == 128 + 3 * 73);
	ClearModPlants();
	CHECK(CardChooserSlotCount() == 4);
}

TEST_CASE("SproutAndImitaterBecomeARandomImitaterHooks66And67") {
	TableGuard t;
	// Seed bank (hook 67): Sprout, or an Imitater copying a Sprout -> Rand(48).
	CHECK(SproutImitaterRoll(kSproutSeed, -1, true) == 48);
	CHECK(SproutImitaterRoll(kImitaterSeed, kSproutSeed, true) == 48);
	CHECK(SproutImitaterRoll(kImitaterSeed, 0, true) == 0);  // an ordinary Imitater plants as usual
	CHECK(SproutImitaterRoll(0, -1, true) == 0);
	CHECK(SproutImitaterRoll(0x50, -1, true) == 0);
	// Dropped seed packet (hook 66): Sprout or any Imitater -> Rand(53).
	CHECK(SproutImitaterRoll(kSproutSeed, -1, false) == 53);
	CHECK(SproutImitaterRoll(kImitaterSeed, 5, false) == 53);
	CHECK(SproutImitaterRoll(kImitaterSeed, -1, false) == 53);
	CHECK(SproutImitaterRoll(3, -1, false) == 0);
	// No rules (vanilla): never.
	SetRules(json::object());
	CHECK(SproutImitaterRoll(kSproutSeed, -1, true) == 0);
	CHECK(SproutImitaterRoll(kImitaterSeed, 5, false) == 0);
}

TEST_CASE("ImitaterCopyingAnImitaterMorphsIntoAPlantOnce") {
	TableGuard t;
	// The coin's Rand(53) can give 48 (exe 0x95750b does the same): that Imitater re-picks once from the same range
	// without 48, so ImitaterMorph never plants another Imitater-of-Imitater (no endless morph chain).
	int aAsked = -1;
	auto aRand = [&aAsked](int theValue) { return [&aAsked, theValue](int n) { aAsked = n; return theValue; }; };
	CHECK(ImitaterOfImitaterCopy(kImitaterSeed, aRand(0)) == 0);
	CHECK(aAsked == 52);
	CHECK(ImitaterOfImitaterCopy(kImitaterSeed, aRand(47)) == 47);
	CHECK(ImitaterOfImitaterCopy(kImitaterSeed, aRand(48)) == 49); // past the Imitater
	CHECK(ImitaterOfImitaterCopy(kImitaterSeed, aRand(51)) == 52);
	for (int r = 0; r < 52; ++r)
		CHECK(ImitaterOfImitaterCopy(kImitaterSeed, aRand(r)) != kImitaterSeed);
	aAsked = -1;
	CHECK(ImitaterOfImitaterCopy(5, aRand(0)) == 5); // any other copy: unchanged, no roll
	CHECK(ImitaterOfImitaterCopy(-1, aRand(0)) == -1);
	CHECK(aAsked == -1);
	// No rules (vanilla): unchanged.
	SetRules(json::object());
	CHECK(ImitaterOfImitaterCopy(kImitaterSeed, aRand(0)) == kImitaterSeed);
}

TEST_CASE("ModPlantsPackFilesMatchTheStudy") {
	std::ifstream in(PackDir() / "data" / "mod-plants.json");
	REQUIRE(in);
	json aTable = json::parse(in);
	std::string err;
	REQUIRE_MESSAGE(SetModPlants(aTable, kEngineNames, &err), err);
	REQUIRE(ModPlantSeeds().size() == kTable.size());
	for (const Row& r : kTable) {
		CAPTURE(r.seed);
		const ModPlant* p = FindModPlant(r.seed);
		REQUIRE(p != nullptr);
		CHECK(kEngineNames[p->base] == r.base);
		CHECK(p->nameEn == r.nameEn);
		CHECK(p->nameZh == r.nameZh);
		CHECK(p->seedCost == r.cost);
		CHECK(p->refreshTime == r.recharge);
		CHECK(p->launchRate == r.launch);
		CHECK(p->shooter == r.shooter);
		CHECK(p->tint == r.tint);
		CHECK(p->tooltipZh.empty()); // public pack: the exe's Chinese tooltips are not included
		CHECK_FALSE(p->tooltipEn.empty()); // modAlmanac: every English tooltip says how to use it, the Loaded Cannon's too
	}
	ClearModPlants();

	std::ifstream rin(PackDir() / "data" / "rules-mod-plants.json");
	REQUIRE(rin);
	json aRules = json::parse(rin);
	CHECK(aRules.value("modPlants.sproutBankRoll", 0) == 48);
	CHECK(aRules.value("modPlants.sproutCoinRoll", 0) == 53);
	CHECK(aRules.value("modPlants.packetCell", 0) == 12);

	std::ifstream min(PackDir() / "mod.json");
	REQUIRE(min);
	CHECK(json::parse(min)["features"].value("modPlants", false));
}

TEST_CASE("PlantStateKeepsTheModSeedOlderBlobsReadZero") {
	PlantState s;
	s.counter = 2;
	s.sweepTimer = 9;
	s.modSeed = 0x67;
	auto blob = Encode(s);
	CHECK(blob.size() == 60); // Plan 8 part 3a shooter fields (32..51), part 3c shadow state + Charmed Tall-nut counter (52..59)
	PlantState back;
	REQUIRE(Decode(blob.data(), blob.size(), back));
	CHECK(back == s);
	PlantState old; // a 28-byte blob (Plan 5 .. Plan 7 saves): an ordinary plant
	REQUIRE(Decode(blob.data(), 28, old));
	CHECK(old.modSeed == 0);
	CHECK(old.sweepTimer == 9);
}

TEST_CASE("CardSettingRoundTripsAndDefaultsOff") {
	const fs::path dir = fs::temp_directory_path() / "pvzu-test-card";
	fs::create_directories(dir);
	CHECK_FALSE(pvzu::Settings{}.card);
	pvzu::Settings s;
	REQUIRE(pvzu::SaveSettings(dir / "pvzu-settings.json", s));
	{
		std::ifstream in(dir / "pvzu-settings.json");
		CHECK_FALSE(json::parse(in).contains("card")); // files without CARD stay as they were
	}
	s.card = true;
	REQUIRE(pvzu::SaveSettings(dir / "pvzu-settings.json", s));
	CHECK(pvzu::LoadSettings(dir / "pvzu-settings.json").card);
	std::ofstream(dir / "odd.json") << R"({"mode":"unnamed","card":"yes"})";
	CHECK_FALSE(pvzu::LoadSettings(dir / "odd.json").card);
	fs::remove_all(dir);
}
