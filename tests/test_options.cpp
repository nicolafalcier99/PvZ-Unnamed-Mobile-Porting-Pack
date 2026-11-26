// PVZU: Unnamed settings/cheats panel (feature shopUi). Evidence: the research notes (modes-ui-shop) section 3,
// w6-w7-and-new-content.md 1.3, pvz-unnamed-pack SOURCES-shop-ui.md.
#include "doctest/doctest.h"
#include "Mod/Mode.h"
#include "Mod/OptionsPanel.h"
#include "Mod/Unnamed/Music.h"
#include "Mod/Unnamed/Options.h"

#include <algorithm>
#include <filesystem>
#include <set>

using namespace pvzu;
namespace u = pvzu::unnamed;
namespace eo = pvzu::unnamed::eliteoption;
namespace fs = std::filesystem;

TEST_CASE("OptionsDefaultsAreTheExeLoadValues") {
	// .NewSec globals are zero at load; Wisdom (tree power) is stored inverted (0x800254 ^ 1), so it is on.
	const u::Options o;
	CHECK(o.Get(u::OptionId::EliteZombies) == eo::DEFAULT);
	CHECK_FALSE(o.On(u::OptionId::DoubleSun));
	CHECK_FALSE(o.On(u::OptionId::AutoCollect));
	CHECK_FALSE(o.On(u::OptionId::FreePlanting));
	CHECK_FALSE(o.On(u::OptionId::CloseDoor));
	CHECK_FALSE(o.On(u::OptionId::FreezeZombies));
	CHECK(o.On(u::OptionId::TreePower));
	// R8 (the research notes (r8-options)): the other flags of the live dialog are zero at load too
	CHECK(o.Get(u::OptionId::Rain) == 0);
	CHECK_FALSE(o.On(u::OptionId::Gravestones));
	CHECK_FALSE(o.On(u::OptionId::NoSpawnLimit));
	CHECK_FALSE(o.On(u::OptionId::DaveNoPick));
	CHECK_FALSE(o.On(u::OptionId::SpawnForward));
	CHECK_FALSE(o.On(u::OptionId::FullFog));
}

TEST_CASE("OptionsCycleAndClamp") {
	u::Options o;
	// INFZOM: +1 mod 3 (0x79b375)
	CHECK(o.Cycle(u::OptionId::EliteZombies) == eo::NONE);
	CHECK(o.Cycle(u::OptionId::EliteZombies) == eo::ALL);
	CHECK(o.Cycle(u::OptionId::EliteZombies) == eo::DEFAULT);
	CHECK(o.Cycle(u::OptionId::DoubleSun) == 1);
	CHECK(o.Cycle(u::OptionId::DoubleSun) == 0);
	o.Set(u::OptionId::EliteZombies, 7);
	CHECK(o.Get(u::OptionId::EliteZombies) == eo::DEFAULT);
	o.Set(u::OptionId::TreePower, -1);
	CHECK(o.Get(u::OptionId::TreePower) == 1);
}

TEST_CASE("OptionsTableHasUniqueKeysAndLabels") {
	std::set<std::string> keys, labels;
	for (const auto& i : u::OptionTable()) {
		CHECK(keys.insert(i.key).second);
		CHECK(labels.insert(i.label).second);
		CHECK(i.values >= 2);
		CHECK(i.defaultValue < i.values);
	}
	CHECK(u::OptionTable().size() == 14); // 13 exe options + the port's music row
	CHECK(std::string(u::OptionValueLabel(u::OptionId::EliteZombies, eo::NONE)) == "None");
	CHECK(std::string(u::OptionValueLabel(u::OptionId::EliteZombies, eo::ALL)) == "All");
	CHECK(std::string(u::OptionValueLabel(u::OptionId::DoubleSun, 1)) == "On");
	CHECK(std::string(u::OptionValueLabel(u::OptionId::DoubleSun, 0)) == "Off");
}

TEST_CASE("OptionsMapRoundTrip") {
	u::Options o;
	o.Set(u::OptionId::EliteZombies, eo::ALL);
	o.Set(u::OptionId::AutoCollect, 1);
	o.Set(u::OptionId::TreePower, 0);
	const auto m = u::OptionsToMap(o);
	CHECK(m.at("eliteZombies") == 2);
	CHECK(m.at("treePower") == 0);
	CHECK(u::OptionsFromMap(m) == o);
	// unknown keys and bad values fall back to the defaults
	const u::Options back = u::OptionsFromMap({{"eliteZombies", 9}, {"martian", 1}, {"doubleSun", 1}});
	CHECK(back.Get(u::OptionId::EliteZombies) == eo::DEFAULT);
	CHECK(back.On(u::OptionId::DoubleSun));
	CHECK(u::OptionsFromMap({}) == u::Options{});
}

TEST_CASE("OptionsPersistInSettingsFile") {
	fs::path dir = fs::temp_directory_path() / "pvzu-options-test";
	fs::remove_all(dir);
	fs::create_directories(dir);
	Settings s;
	s.mode = GameMode::Unnamed;
	s.speed = 2;
	s.unnamedOptions = {{"eliteZombies", 1}, {"freePlanting", 1}};
	REQUIRE(SaveSettings(dir / "pvzu-settings.json", s));
	const Settings back = LoadSettings(dir / "pvzu-settings.json");
	CHECK(back.unnamedOptions == s.unnamedOptions);
	CHECK(back.speed == 2);
	// an old file without the block still loads, with no options (= every default)
	CHECK(LoadSettings(dir / "nope.json").unnamedOptions.empty());
	fs::remove_all(dir);
}

TEST_CASE("CardRowShowsTheImitaterCondition") {
	// w6-w7-and-new-content.md 1.3: hook 256 answers "Imitater bought" for mod seeds only while CARD (0x8001a0) is on,
	// so CARD on without the Imitater shows nothing; the row says so.
	CHECK(std::string(u::kCardRowLabel) == "Mod plants");
	const u::CardRowText off = u::CardRow(false, false, false);
	CHECK(std::string(off.value) == "Off");
	CHECK(std::string(off.hint).empty());
	CHECK_FALSE(off.nonDefault);
	CHECK(std::string(u::CardRow(false, true, false).hint).empty());
	const u::CardRowText on = u::CardRow(true, true, false);
	CHECK(std::string(on.value) == "On");
	CHECK(std::string(on.hint).empty());
	CHECK(on.nonDefault);
	const u::CardRowText waiting = u::CardRow(true, false, false);
	CHECK(std::string(waiting.value) == "On");
	CHECK(std::string(waiting.hint) == "Buy the Imitater in the shop");
	CHECK(std::string(waiting.hintKey) == "PVZU_OPT_CARD_NEEDS_IMITATER");
	CHECK(waiting.nonDefault);
	// changed while a seed chooser is open: it counts from the next one (the Imitater hint wins, it matters more)
	const u::CardRowText next = u::CardRow(true, true, true);
	CHECK(std::string(next.hint) == "From the next seed chooser");
	CHECK(std::string(next.hintKey) == "PVZU_OPT_CARD_NEXT_CHOOSER");
	CHECK(std::string(u::CardRow(false, true, true).hint) == "From the next seed chooser");
	CHECK(std::string(u::CardRow(false, false, true).hint) == "From the next seed chooser");
	CHECK(std::string(u::CardRow(true, false, true).hint) == "Buy the Imitater in the shop");
}

TEST_CASE("CardTapTakesEffectAtTheNextChooser") {
	// The chooser lays out its seeds (0..52 and 0x50..0x7f with CARD) when it is made, so a tap while it is open
	// waits for the next chooser; with no chooser open it counts at once (HasSeedType, almanac).
	u::CardLatch c(false);
	CHECK_FALSE(c.InEffect());
	c.Set(true, true);
	CHECK(c.Setting());
	CHECK_FALSE(c.InEffect());
	c.ChooserOpens();
	CHECK(c.InEffect());
	c.Set(false, false);
	CHECK_FALSE(c.Setting());
	CHECK_FALSE(c.InEffect());
	CHECK(u::CardLatch(true).InEffect());
}

TEST_CASE("PanelRowsAddCardOnlyWithModPlants") {
	CHECK(u::PanelRowCount(false) == 7);
	CHECK(u::PanelRowCount(true) == 8);
	CHECK(u::PanelRowCount(false, true) == 13);
	CHECK(u::PanelRowCount(true, true) == 14);
}

TEST_CASE("MusicOptionDefaultsToOriginal") {
	// 2026-11-24: the mod's mp3s "stop and start"; the original soundtrack is the default.
	const u::Options o;
	CHECK(o.Get(u::OptionId::Music) == u::musicchoice::ORIGINAL);
	const u::OptionInfo& i = u::InfoFor(u::OptionId::Music);
	CHECK(std::string(i.key) == "music");
	CHECK(i.values == 2);
	CHECK(i.defaultValue == u::musicchoice::ORIGINAL);
	CHECK(i.music);
	CHECK_FALSE(i.extra);
	CHECK(std::string(i.label) == "Music");
	CHECK(std::string(u::OptionValueLabel(u::OptionId::Music, u::musicchoice::ORIGINAL)) == "Original");
	CHECK(std::string(u::OptionValueLabel(u::OptionId::Music, u::musicchoice::MOD)) == "Unnamed mod");
	CHECK(std::string(u::OptionValueLabelKey(u::OptionId::Music, 0)) == "PVZU_OPT_MUSIC_ORIGINAL");
	CHECK(std::string(u::OptionValueLabelKey(u::OptionId::Music, 1)) == "PVZU_OPT_MUSIC_MOD");
	u::Options c;
	CHECK(c.Cycle(u::OptionId::Music) == u::musicchoice::MOD);
	CHECK(c.Cycle(u::OptionId::Music) == u::musicchoice::ORIGINAL);
}

TEST_CASE("MusicOptionLoadsAndSaves") {
	CHECK(u::OptionsFromMap({}).Get(u::OptionId::Music) == u::musicchoice::ORIGINAL);         // old settings file
	CHECK(u::OptionsFromMap({{"music", 1}}).Get(u::OptionId::Music) == u::musicchoice::MOD);
	CHECK(u::OptionsFromMap({{"music", 2}}).Get(u::OptionId::Music) == u::musicchoice::ORIGINAL); // out of range
	CHECK(u::OptionsFromMap({{"music", -1}}).Get(u::OptionId::Music) == u::musicchoice::ORIGINAL);
	u::Options o;
	o.Set(u::OptionId::Music, u::musicchoice::MOD);
	CHECK(u::OptionsToMap(o).at("music") == 1);
	CHECK(u::OptionsToMap(u::Options()).at("music") == 0);
	// through pvzu-settings.json ("unnamedOptions")
	const fs::path dir = fs::temp_directory_path() / "pvzu-music-choice-test";
	fs::remove_all(dir);
	Settings s;
	s.unnamedOptions = u::OptionsToMap(o);
	REQUIRE(SaveSettings(dir / "pvzu-settings.json", s));
	const Settings back = LoadSettings(dir / "pvzu-settings.json");
	CHECK(back.unnamedOptions.at("music") == 1);
	CHECK(u::OptionsFromMap(back.unnamedOptions) == o);
	fs::remove_all(dir);
}

TEST_CASE("MusicRowShowsWithUnnamedMusic") {
	const std::vector<u::OptionId> base = u::PanelOptions(false, true);
	REQUIRE(base.size() == 8);
	CHECK(base.back() == u::OptionId::Music); // after the old rows, before CARD
	const std::vector<u::OptionId> all = u::PanelOptions(true, true);
	REQUIRE(all.size() == u::OptionTable().size());
	CHECK(all.back() == u::OptionId::Music);
	for (bool aExtra : {false, true}) {
		const std::vector<u::OptionId> v = u::PanelOptions(aExtra, false);
		CHECK(std::find(v.begin(), v.end(), u::OptionId::Music) == v.end());
	}
	CHECK(u::PanelRowCount(false, false, true) == 8);
	CHECK(u::PanelRowCount(true, true, true) == 15);
	// 15 rows still fit the phone in 3 columns of 5 (as the 14 rows of R8)
	const OptionsPanelLayout l14 = LayoutOptionsPanel(1300, 600, 0, 0, 14, true, true);
	const OptionsPanelLayout l15 = LayoutOptionsPanel(1300, 600, 0, 0, 15, true, true);
	CHECK(l15.columns == l14.columns);
	CHECK(l15.panel.h == l14.panel.h);
	CHECK(l15.rows.size() == 15);
}

TEST_CASE("ExtraOptionsShowOnlyWithTheirFeature") {
	// a pack without "extraOptions" keeps the old panel: the first seven options, in the old order
	const auto& t = u::OptionTable();
	const std::vector<u::OptionId> base = u::PanelOptions(false);
	REQUIRE(base.size() == 7);
	for (size_t i = 0; i < base.size(); i++)
		CHECK(base[i] == t[i].id);
	const std::vector<u::OptionId> all = u::PanelOptions(true);
	REQUIRE(all.size() == t.size() - 1); // all but the music row (its own feature, MusicRowShowsWithUnnamedMusic)
	// the extra rows follow the live dialog's order (0xb72d48..: 墓碑模式, 无限制出怪, 戴夫不选卡, 刷新点前移, 下雨模式),
	// then the port's full-screen fog row
	const std::vector<u::OptionId> extra(all.begin() + 7, all.end());
	CHECK(extra == std::vector<u::OptionId>{u::OptionId::Gravestones, u::OptionId::NoSpawnLimit, u::OptionId::DaveNoPick,
		u::OptionId::SpawnForward, u::OptionId::Rain, u::OptionId::FullFog});
	for (u::OptionId id : extra)
		CHECK(u::InfoFor(id).extra);
	for (u::OptionId id : base)
		CHECK_FALSE(u::InfoFor(id).extra);
	CHECK(std::string(u::InfoFor(u::OptionId::Rain).key) == "rain");
	CHECK(std::string(u::InfoFor(u::OptionId::Gravestones).key) == "gravestones");
	CHECK(std::string(u::InfoFor(u::OptionId::NoSpawnLimit).key) == "noSpawnLimit");
	CHECK(std::string(u::InfoFor(u::OptionId::DaveNoPick).key) == "daveNoPick");
	CHECK(std::string(u::InfoFor(u::OptionId::SpawnForward).key) == "spawnForward");
	CHECK(std::string(u::InfoFor(u::OptionId::FullFog).key) == "fullFog");
}

TEST_CASE("RainCyclesOffRainStorm") {
	// typed RAIN 0x79b797: [0x80011c] += 1, >= 3 -> 0; dialog radios 默认 / 下雨天 / 暴风雨
	u::Options o;
	CHECK(o.Cycle(u::OptionId::Rain) == 1);
	CHECK(o.Cycle(u::OptionId::Rain) == 2);
	CHECK(o.Cycle(u::OptionId::Rain) == 0);
	CHECK(std::string(u::OptionValueLabel(u::OptionId::Rain, 0)) == "Off");
	CHECK(std::string(u::OptionValueLabel(u::OptionId::Rain, 1)) == "Rain");
	CHECK(std::string(u::OptionValueLabel(u::OptionId::Rain, 2)) == "Storm");
	CHECK(std::string(u::OptionValueLabelKey(u::OptionId::Rain, 1)) == "PVZU_OPT_RAIN_RAIN");
	CHECK(std::string(u::OptionValueLabelKey(u::OptionId::Rain, 2)) == "PVZU_OPT_RAIN_STORM");
	CHECK(std::string(u::OptionValueLabelKey(u::OptionId::Rain, 0)) == "PVZU_OPT_OFF");
}

TEST_CASE("RainWeatherIsRainThenStorm") {
	// hooks 117 / 164 / 185: rain != 0 -> rain; hooks 163 / 186: rain == 2 -> lightning (UpdateStormyNight, DrawStormNight)
	CHECK_FALSE(u::OptionWeather(0).rain);
	CHECK_FALSE(u::OptionWeather(0).storm);
	CHECK(u::OptionWeather(1).rain);
	CHECK_FALSE(u::OptionWeather(1).storm);
	CHECK(u::OptionWeather(2).rain);
	CHECK(u::OptionWeather(2).storm);
}

TEST_CASE("GravestoneModeRunsGraveDangerWaves") {
	// hook 182 (0xc11373): [0xc113a8] == 1 jumps past the `mode == GRAVE_DANGER` compare with ZF set
	CHECK(u::GraveDangerWaveRules(true, false));
	CHECK(u::GraveDangerWaveRules(false, true));
	CHECK(u::GraveDangerWaveRules(true, true));
	CHECK_FALSE(u::GraveDangerWaveRules(false, false));
}

TEST_CASE("DaveDoesntPick") {
	// hook 436 (0xc13410 -> 0x95b7ba): level 73 or 65 -> no picks; [0x800450] == 1 -> no picks; else the vanilla gate
	CHECK(u::DavePicksSeeds(true, 12, false));
	CHECK_FALSE(u::DavePicksSeeds(false, 12, false)); // first adventure: vanilla gate
	CHECK_FALSE(u::DavePicksSeeds(true, 12, true));
	CHECK_FALSE(u::DavePicksSeeds(true, 65, false));
	CHECK_FALSE(u::DavePicksSeeds(true, 73, false));
	CHECK(u::DavePicksSeeds(true, 64, false));
	// hook 447 (0xc03630): with the option a seed Dave picked is freed
	CHECK(u::DavePickLocks(true, false));
	CHECK_FALSE(u::DavePickLocks(true, true));
	CHECK_FALSE(u::DavePickLocks(false, false));
}

TEST_CASE("SpawnPointForwardDropsTheWideLawnOffset") {
	// hook 480: forward -> no +133; hooks 492/495/497/512: forward base = default - 133 (822/800/825/845)
	CHECK(u::ForwardStartOffset(133.0f, false) == 133.0f);
	CHECK(u::ForwardStartOffset(133.0f, true) == 0.0f);
	CHECK(u::ForwardStartBaseX(955, 133, false) == 955);
	CHECK(u::ForwardStartBaseX(955, 133, true) == 822);
	CHECK(u::ForwardStartBaseX(933, 133, true) == 800);
	CHECK(u::ForwardStartBaseX(958, 133, true) == 825);
	CHECK(u::ForwardStartBaseX(978, 133, true) == 845);
	CHECK(u::ForwardStartBaseX(870, 0, true) == 870); // no wide-lawn offset in the pack: nothing to drop
}

TEST_CASE("QuickSaveSlotIsItsOwnFile") {
	// port ruling (r8-options.md section 6): the slot sits next to the mode's save, never the same file
	CHECK(u::QuickSaveName("/x/game1_0.v4") == "/x/game1_0.v4.quick");
	CHECK(u::QuickSaveName("") == "");
}

TEST_CASE("DoubleSunRemapsCollectedSun") {
	// hook 204: 15 -> 25, 25 -> 50, 50 -> 75; other values (e.g. a 5 from a coin) unchanged; off = vanilla
	CHECK(u::CollectedSunValue(15, true) == 25);
	CHECK(u::CollectedSunValue(25, true) == 50);
	CHECK(u::CollectedSunValue(50, true) == 75);
	CHECK(u::CollectedSunValue(5, true) == 5);
	CHECK(u::CollectedSunValue(25, false) == 25);
	CHECK(u::CollectedSunValue(50, false) == 50);
}

TEST_CASE("DoubleSunAndFastSpeedUpTheConveyor") {
	// V2 (C2), hooks 161 (0x7a031a, Challenge::UpdateConveyorBelt countdown +0x5c) and 473 (0x7a0375,
	// SeedBank::UpdateConveyorBelt packet slide): step 1, x2 with Sun (0x800110), x2 again with Fast (0x800118)
	CHECK(u::ConveyorStep(false, false) == 1);
	CHECK(u::ConveyorStep(true, false) == 2);
	CHECK(u::ConveyorStep(false, true) == 2);
	CHECK(u::ConveyorStep(true, true) == 4);
	// the countdown and the packet offset never go below 0 (0x7a0336 min with the counter; 0x489ca1 setle/and)
	CHECK(u::ConveyorStepDown(10, 2) == 8);
	CHECK(u::ConveyorStepDown(1, 2) == 0);
	CHECK(u::ConveyorStepDown(3, 4) == 0);
	CHECK(u::ConveyorStepDown(0, 1) == 0);
}

TEST_CASE("AutoCollectTakesCoinTypesOneToSix") {
	// hook 207: !beingCollected && (flag == 0 || type > 6) -> vanilla, else collected
	for (int t = 1; t <= 6; t++)
		CHECK(u::CoinCollectsItself(t, false, true));
	CHECK_FALSE(u::CoinCollectsItself(7, false, true));  // final seed packet
	CHECK_FALSE(u::CoinCollectsItself(8, false, true));  // trophy
	CHECK_FALSE(u::CoinCollectsItself(0, false, true));
	CHECK_FALSE(u::CoinCollectsItself(4, true, true));   // already flying to the bank
	CHECK_FALSE(u::CoinCollectsItself(4, false, false));
}

TEST_CASE("CloseDoorSkipsTheHouseCheck") {
	// hook 630: cVar1 = IsIZombieLevel(); if (flag == 1) cVar1 = 1; if (!cVar1) CheckForBoardEdge
	CHECK(u::HouseCheckRuns(false, false));
	CHECK_FALSE(u::HouseCheckRuns(false, true));
	CHECK_FALSE(u::HouseCheckRuns(true, false));
}

TEST_CASE("ZenGloveLeavesABusySquash") {
	// hook 478: cursor 14 && (seed != 0x11 || state == 0) -> vanilla pick-up
	// hook 478 (Zen glove) is tested in test_modes_vanilla.cpp (one copy, ModesVanilla.h)
}

TEST_CASE("EliteOptionIsTheSettingOnlyWithTheFeature") {
	CHECK(u::EffectiveEliteOption(eo::ALL, true) == eo::ALL);
	CHECK(u::EffectiveEliteOption(eo::NONE, true) == eo::NONE);
	CHECK(u::EffectiveEliteOption(eo::NONE, false) == eo::DEFAULT);
	CHECK(u::EffectiveEliteOption(5, true) == eo::DEFAULT);
}

TEST_CASE("TallnutOddFollowsTheEliteOption") {
	// hook 508: t = 2 - option; t == 2 -> Rand(12); t == 0 -> variant 0x101
	CHECK(u::TallnutOddRoll(eo::DEFAULT, 12).range == 12);
	const auto none = u::TallnutOddRoll(eo::NONE, 12);
	CHECK(none.range == 0);
	CHECK(none.fixedRoll == 1);
	const auto all = u::TallnutOddRoll(eo::ALL, 12);
	CHECK(all.range == 0);
	CHECK(all.fixedRoll == 0);
	CHECK(u::TallnutOddRoll(eo::DEFAULT, 0).range == 0); // no rule: no roll
}

namespace {
bool InsideRect(const HitRect& inner, const HitRect& outer) {
	return inner.x >= outer.x && inner.y >= outer.y && inner.x + inner.w <= outer.x + outer.w && inner.y + inner.h <= outer.y + outer.h;
}
bool Overlap(const HitRect& a, const HitRect& b) {
	return a.x < b.x + b.w && b.x < a.x + a.w && a.y < b.y + b.h && b.y < a.y + a.h;
}
void CheckPanel(int w, int h, int safeLeft, int safeRight, int rows, bool win, bool save = false, int minH = kMinTouchSize) {
	CAPTURE(w);
	CAPTURE(safeLeft);
	CAPTURE(rows);
	const OptionsPanelLayout l = LayoutOptionsPanel(w, h, safeLeft, safeRight, rows, win, save);
	const HitRect safe{safeLeft + kOptionsMargin, kOptionsMargin, w - safeLeft - safeRight - 2 * kOptionsMargin, h - 2 * kOptionsMargin};
	CHECK(InsideRect(l.panel, safe));
	REQUIRE(static_cast<int>(l.rows.size()) == rows);
	std::vector<HitRect> all = l.rows;
	all.push_back(l.done);
	if (win)
		all.push_back(l.win);
	else
		CHECK(l.win.w == 0);
	if (save) {
		all.push_back(l.save);
		all.push_back(l.load);
	} else {
		CHECK(l.save.w == 0);
		CHECK(l.load.w == 0);
	}
	for (size_t i = 0; i < all.size(); i++) {
		CHECK(all[i].h >= minH);
		CHECK(all[i].w >= (minH < kMinTouchSize ? 150 : 200));
		CHECK(InsideRect(all[i], l.panel));
		CHECK_FALSE(Overlap(all[i], l.title));
		for (size_t j = i + 1; j < all.size(); j++)
			CHECK_FALSE(Overlap(all[i], all[j]));
	}
}
} // namespace

TEST_CASE("OptionsPanelFitsThePhoneAndTheMac") {
	CheckPanel(1300, 600, 57, 0, 7, true);   // S26 Ultra 19.5:9, camera hole left
	CheckPanel(1300, 600, 0, 57, 7, false);  // phone turned the other way
	CheckPanel(800, 600, 0, 0, 7, true);     // vanilla 4:3 width
	CheckPanel(1400, 600, 0, 0, 7, true);
	CheckPanel(1300, 600, 57, 0, 8, true);   // + the CARD row (mod plants in the pack)
	CheckPanel(1300, 600, 0, 57, 8, false);
	CheckPanel(800, 600, 0, 0, 8, true);
	CheckPanel(1300, 600, 57, 0, 8, false);
	// R8: 13 options + CARD, with the Save / Load game buttons (in a level) or without (chooser, menu)
	CheckPanel(1300, 600, 57, 0, 14, true, true);
	CheckPanel(1300, 600, 0, 57, 14, true, true);
	CheckPanel(1300, 600, 57, 0, 14, false, false);
	CheckPanel(1300, 600, 57, 0, 13, true, true);
	CheckPanel(1400, 600, 0, 0, 14, true, true);
	CheckPanel(1067, 600, 0, 0, 14, true, true);    // the mod's own 16:9 screen
	CheckPanel(800, 600, 0, 0, 14, true, true, 40); // 4:3 (no phone is this narrow): rows shrink to fit, never below 40
	CHECK(LayoutOptionsPanel(1300, 600, 57, 0, 14, true, true).columns == 3);
	CHECK(LayoutOptionsPanel(1300, 600, 57, 0, 8, true).columns == 2); // the old panel keeps its look
	CHECK(LayoutOptionsPanel(1300, 600, 57, 0, 8, true).rows[0].h == kOptionsRowH);
	const OptionsPanelLayout l = LayoutOptionsPanel(1300, 600, 57, 0, 7, true);
	CHECK(l.columns == 2);
	// centred in the safe area
	const int safeMid = 57 + (1300 - 57) / 2;
	CHECK(std::abs(l.panel.x + l.panel.w / 2 - safeMid) <= 1);
}

TEST_CASE("OptionsRowHitTestWithCard") {
	const OptionsPanelLayout l = LayoutOptionsPanel(1300, 600, 57, 0, 8, true);
	for (int i = 0; i < 8; i++)
		CHECK(OptionsRowAt(l, l.rows[i].x + l.rows[i].w / 2, l.rows[i].y + l.rows[i].h / 2) == i);
}

TEST_CASE("OptionsRowHitTestThreeColumns") {
	const OptionsPanelLayout l = LayoutOptionsPanel(1300, 600, 57, 0, 14, true, true);
	for (int i = 0; i < 14; i++)
		CHECK(OptionsRowAt(l, l.rows[i].x + l.rows[i].w / 2, l.rows[i].y + l.rows[i].h / 2) == i);
	CHECK(OptionsRowAt(l, l.save.x + 2, l.save.y + 2) == -1);
	CHECK(OptionsRowAt(l, l.load.x + 2, l.load.y + 2) == -1);
	CHECK(l.save.y == l.done.y); // one footer line: Win, Save, Load ... Done
	CHECK(l.load.x > l.save.x);
}

TEST_CASE("OptionsRowHitTest") {
	const OptionsPanelLayout l = LayoutOptionsPanel(1300, 600, 57, 0, 7, true);
	for (int i = 0; i < 7; i++)
		CHECK(OptionsRowAt(l, l.rows[i].x + l.rows[i].w / 2, l.rows[i].y + l.rows[i].h / 2) == i);
	CHECK(OptionsRowAt(l, l.done.x + 2, l.done.y + 2) == -1);
	CHECK(OptionsRowAt(l, 0, 0) == -1);
	// column by column: row 1 is under row 0
	CHECK(l.rows[1].x == l.rows[0].x);
	CHECK(l.rows[1].y > l.rows[0].y);
}

TEST_CASE("LongPressFiresOnceAndEatsTheRelease") {
	LongPress p;
	CHECK_FALSE(p.Poll(10, true, 50)); // not pressed
	p.Down(100);
	CHECK(p.Pressed());
	CHECK_FALSE(p.Poll(149, true, 50));
	CHECK(p.Poll(150, true, 50));
	CHECK_FALSE(p.Poll(151, true, 50)); // once
	CHECK(p.Up());                      // the release belongs to the long press
	CHECK_FALSE(p.Pressed());
	// a short tap: the release is the button's
	p.Down(200);
	CHECK_FALSE(p.Poll(220, true, 50));
	CHECK_FALSE(p.Up());
	// sliding off cancels
	p.Down(300);
	CHECK_FALSE(p.Poll(320, false, 50));
	CHECK_FALSE(p.Poll(400, true, 50));
	CHECK_FALSE(p.Up());
	// a non-positive hold never fires
	p.Down(500);
	CHECK_FALSE(p.Poll(9999, true, 0));
	CHECK_FALSE(p.Up());
}
