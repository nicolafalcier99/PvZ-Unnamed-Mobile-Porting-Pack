// PVZU Plan 9 C1: the test cheats (pure rules, layout, the cheat bot's plan). Not mod behaviour: a testing aid,
// hidden unless the pack's feature "testCheats" is on (Mod/TestCheats.h).
#include "doctest/doctest.h"
#include "Mod/Hud.h"
#include "Mod/Mode.h"
#include "Mod/OptionsPanel.h"
#include "Mod/TestCheats.h"

#include <algorithm>
#include <filesystem>
#include <set>
#include <tuple>

using namespace pvzu;
namespace c = pvzu::cheats;
namespace fs = std::filesystem;

TEST_CASE("TestCheatsDefaultOffAndCycle") {
	c::Cheats t;
	for (const auto& i : c::CheatTable())
		CHECK(t.Get(i.id) == 0);
	CHECK(c::CheatTable().size() == 8);
	CHECK(t.Cycle(c::CheatId::InfiniteSun) == 1);
	CHECK(t.Cycle(c::CheatId::InfiniteSun) == 0);
	// speed 1x 2x 4x 8x
	CHECK(t.Cycle(c::CheatId::Speed) == 1);
	CHECK(t.Cycle(c::CheatId::Speed) == 2);
	CHECK(t.Cycle(c::CheatId::Speed) == 3);
	CHECK(t.Cycle(c::CheatId::Speed) == 0);
	// seed slots: Normal, 6 .. 15
	for (int i = 1; i <= 10; i++)
		CHECK(t.Cycle(c::CheatId::SeedSlots) == i);
	CHECK(t.Cycle(c::CheatId::SeedSlots) == 0);
	t.Set(c::CheatId::Speed, 9);
	CHECK(t.Get(c::CheatId::Speed) == 0);
	t.Set(c::CheatId::Invincible, -1);
	CHECK(t.Get(c::CheatId::Invincible) == 0);
}

TEST_CASE("TestCheatsTableKeysAndLabels") {
	std::set<std::string> keys, labels;
	for (const auto& i : c::CheatTable()) {
		CHECK(keys.insert(i.key).second);
		CHECK(labels.insert(i.label).second);
		CHECK(std::string(i.labelKey).rfind("PVZU_CHEAT_", 0) == 0);
		CHECK(i.values >= 2);
	}
	CHECK(std::string(c::CheatValueLabel(c::CheatId::InfiniteSun, 0)) == "Off");
	CHECK(std::string(c::CheatValueLabel(c::CheatId::InfiniteSun, 1)) == "On");
	CHECK(std::string(c::CheatValueLabel(c::CheatId::Speed, 0)) == "1x");
	CHECK(std::string(c::CheatValueLabel(c::CheatId::Speed, 3)) == "8x");
	CHECK(std::string(c::CheatValueLabel(c::CheatId::SeedSlots, 0)) == "Normal");
	CHECK(std::string(c::CheatValueLabel(c::CheatId::SeedSlots, 1)) == "6");
	CHECK(std::string(c::CheatValueLabel(c::CheatId::SeedSlots, 10)) == "15");
}

TEST_CASE("TestCheatsValues") {
	CHECK(c::CheatSeedSlots(0) == 0); // no change
	CHECK(c::CheatSeedSlots(1) == 6);
	CHECK(c::CheatSeedSlots(10) == 15);
	CHECK(c::CheatSeedSlots(11) == 0);
	CHECK(c::CheatSpeedFactor(0) == 1);
	CHECK(c::CheatSpeedFactor(1) == 2);
	CHECK(c::CheatSpeedFactor(2) == 4);
	CHECK(c::CheatSpeedFactor(3) == 8);
	// the HUD's 1x/2x button and the cheat: the faster wins
	CHECK(c::EffectiveSpeed(1, 0) == 1);
	CHECK(c::EffectiveSpeed(2, 0) == 2);
	CHECK(c::EffectiveSpeed(1, 2) == 4);
	CHECK(c::EffectiveSpeed(2, 3) == 8);
	CHECK(c::EffectiveSpeed(2, 1) == 2);
}

TEST_CASE("TestCheatsMapRoundTripAndFile") {
	c::Cheats t;
	t.Set(c::CheatId::Invincible, 1);
	t.Set(c::CheatId::SeedSlots, 10);
	auto m = c::CheatsToMap(t);
	CHECK(m.at("invincible") == 1);
	CHECK(m.at("seedSlots") == 10);
	CHECK(c::CheatsFromMap(m) == t);
	CHECK(c::CheatsFromMap({{"speed", 7}, {"martian", 1}}) == c::Cheats{});
	// the level-jump return point lives in the same block and survives the round trip
	c::LevelReturn r{3, 12, 0};
	c::ReturnToMap(r, m);
	CHECK(c::ReturnFromMap(m) == r);
	CHECK(c::CheatsFromMap(m) == t);
	CHECK_FALSE(c::ReturnFromMap({}).Active());

	fs::path dir = fs::temp_directory_path() / "pvzu-cheats-test";
	fs::remove_all(dir);
	fs::create_directories(dir);
	Settings s;
	s.testCheats = m;
	REQUIRE(SaveSettings(dir / "pvzu-settings.json", s));
	CHECK(LoadSettings(dir / "pvzu-settings.json").testCheats == m);
	CHECK(LoadSettings(dir / "nope.json").testCheats.empty());
	fs::remove_all(dir);
}

TEST_CASE("TestCheatsLevelJumpKeepsTheFirstReturnPoint") {
	// the player's real progress is kept once; a second jump during a test keeps the first snapshot.
	c::LevelReturn none;
	CHECK_FALSE(none.Active());
	const c::LevelReturn first = c::BeginJump(none, 2, 11, 0);
	CHECK(first.Active());
	CHECK(first.level == 11);
	const c::LevelReturn second = c::BeginJump(first, 2, 70, 0);
	CHECK(second == first);
	// another profile: the snapshot is that profile's
	CHECK(c::BeginJump(none, 5, 3, 1).profileId == 5);
	CHECK(c::RestoreFor(first, 2));
	CHECK_FALSE(c::RestoreFor(first, 4));
	CHECK_FALSE(c::RestoreFor(none, 2));
}

TEST_CASE("TestCheatsLevelPicker") {
	CHECK(c::kPickerWorlds == 8);
	CHECK(c::LevelsInWorld(1) == 10);
	CHECK(c::LevelsInWorld(7) == 10);
	CHECK(c::LevelsInWorld(8) == 3); // hidden 71-73
	CHECK(c::LevelFor(1, 0) == 1);
	CHECK(c::LevelFor(5, 9) == 50);
	CHECK(c::LevelFor(7, 9) == 70);
	CHECK(c::LevelFor(8, 0) == 71);
	CHECK(c::LevelFor(8, 2) == 73);
	CHECK(c::LevelFor(8, 3) == 0);
	CHECK(c::LevelFor(0, 0) == 0);
	CHECK(c::LevelFor(9, 0) == 0);
	CHECK(c::LevelName(1) == "1-1");
	CHECK(c::LevelName(50) == "5-10");
	CHECK(c::LevelName(73) == "8-3");
	CHECK(c::WorldOfLevel(70) == 7);
	CHECK(c::WorldOfLevel(71) == 8);
	CHECK(c::WorldOfLevel(1) == 1);
}

TEST_CASE("TestCheatsFinalWave") {
	// mCurrentWave is the next wave to send; the final wave is numWaves - 1. Never goes back.
	CHECK(c::FinalWaveStart(0, 10) == 9);
	CHECK(c::FinalWaveStart(9, 10) == 9);
	CHECK(c::FinalWaveStart(10, 10) == 10);
	CHECK(c::FinalWaveStart(3, 0) == 3);
}

namespace {
bool InsideR(const HitRect& inner, const HitRect& outer) {
	return inner.x >= outer.x && inner.y >= outer.y && inner.x + inner.w <= outer.x + outer.w && inner.y + inner.h <= outer.y + outer.h;
}
bool OverlapR(const HitRect& a, const HitRect& b) { return a.x < b.x + b.w && b.x < a.x + a.w && a.y < b.y + b.h && b.y < a.y + a.h; }
void CheckButtons(std::vector<HitRect> all, const HitRect& panel, const HitRect& title, int minW) {
	for (size_t i = 0; i < all.size(); i++) {
		CAPTURE(i);
		CHECK(all[i].h >= kMinTouchSize);
		CHECK(all[i].w >= minW);
		CHECK(InsideR(all[i], panel));
		CHECK_FALSE(OverlapR(all[i], title));
		for (size_t j = i + 1; j < all.size(); j++)
			CHECK_FALSE(OverlapR(all[i], all[j]));
	}
}
HitRect Safe(int w, int h, int l, int r) { return {l + kOptionsMargin, kOptionsMargin, w - l - r - 2 * kOptionsMargin, h - 2 * kOptionsMargin}; }
} // namespace

TEST_CASE("TestCheatsPageFitsThePhoneAndTheMac") {
	for (auto [w, l, r] : {std::tuple{1300, 57, 0}, {1300, 0, 57}, {1400, 0, 0}, {800, 0, 0}}) {
		CAPTURE(w);
		CAPTURE(l);
		const c::CheatsPageLayout p = c::LayoutCheatsPage(w, 600, l, r, static_cast<int>(c::CheatTable().size()));
		CHECK(InsideR(p.panel, Safe(w, 600, l, r)));
		REQUIRE(p.rows.size() == c::CheatTable().size());
		REQUIRE(p.actions.size() == static_cast<size_t>(c::kCheatActions));
		std::vector<HitRect> all = p.rows;
		all.insert(all.end(), p.actions.begin(), p.actions.end());
		all.push_back(p.back);
		all.push_back(p.done);
		CheckButtons(all, p.panel, p.title, 150);
		CHECK(c::HitIndex(p.rows, p.rows[2].x + 5, p.rows[2].y + 5) == 2);
		CHECK(c::HitIndex(p.actions, p.actions[1].x + 5, p.actions[1].y + 5) == 1);
		CHECK(c::HitIndex(p.rows, 0, 0) == -1);
	}
	CHECK(c::LayoutCheatsPage(1300, 600, 57, 0, 8).columns == 2); // 4 + 4 rows: two columns read best
}

TEST_CASE("TestCheatsLevelPickerFits") {
	for (auto [w, l, r] : {std::tuple{1300, 57, 0}, {1300, 0, 57}, {1400, 0, 0}, {800, 0, 0}}) {
		CAPTURE(w);
		const c::PickerLayout p = c::LayoutLevelPicker(w, 600, l, r);
		CHECK(InsideR(p.panel, Safe(w, 600, l, r)));
		REQUIRE(p.worlds.size() == static_cast<size_t>(c::kPickerWorlds));
		REQUIRE(p.levels.size() == 10u);
		REQUIRE(p.modes.size() == static_cast<size_t>(c::kPickerModes));
		std::vector<HitRect> all = p.worlds;
		all.insert(all.end(), p.levels.begin(), p.levels.end());
		all.insert(all.end(), p.modes.begin(), p.modes.end());
		all.push_back(p.back);
		CheckButtons(all, p.panel, p.title, 64);
	}
}

TEST_CASE("TestCheatsButtonOnTheOptionsTitle") {
	// R8 owns the footer (Win / Save / Load / Done): the cheats button sits at the right of the title band
	for (bool win : {true, false}) {
		for (int rows : {7, 8, 12, 14}) {
			for (auto [w, l, r] : {std::tuple{1300, 57, 0}, {1300, 0, 57}, {800, 0, 0}}) {
				const OptionsPanelLayout o = LayoutOptionsPanel(w, 600, l, r, rows, win);
				const HitRect b = c::OptionsTitleCheatsButton(o);
				CHECK(b.h >= kMinTouchSize);
				CHECK(b.w >= 150);
				CHECK(InsideR(b, o.panel));
				CHECK_FALSE(OverlapR(b, o.done));
				CHECK_FALSE(OverlapR(b, o.win));
				for (const HitRect& row : o.rows)
					CHECK_FALSE(OverlapR(b, row));
			}
		}
	}
}

TEST_CASE("TestCheatsMainMenuButton") {
	// Inside the centred 800-px menu (the side fill is drawn over everything outside it), in the sky at its top
	// right, clear of the tombstone (vanilla x >= 610, y < 80) and the welcome sign (vanilla x < 430).
	for (int aMenuLeft : {0, 250, 300}) {
		const HitRect b = c::MainMenuCheatsButton(aMenuLeft);
		CHECK(b.x >= aMenuLeft + 430);
		CHECK(b.x + b.w <= aMenuLeft + 800);
		CHECK(b.y >= 0);
		CHECK(b.y + b.h <= 80);
		CHECK(b.h >= kMinTouchSize);
		CHECK(b.w >= 150);
	}
}

// ---- the cheat bot's plan ----
TEST_CASE("CheatBotChooserSeeds") {
	for (c::BotLawn lawn : {c::BotLawn::Day, c::BotLawn::Night, c::BotLawn::Pool, c::BotLawn::Fog, c::BotLawn::Roof}) {
		const std::vector<int> s = c::BotChooserSeeds(lawn);
		CHECK(s.size() <= 15);
		CHECK(std::set<int>(s.begin(), s.end()).size() == s.size());
		// every seed the plan plants is picked
		for (int row = 0; row < 6; row++)
			for (int col = 0; col < 9; col++) {
				const c::BotCellPlan p = c::BotPlanFor(lawn, col, row == 2 || row == 3);
				for (int seed : {p.base, p.main, p.armour})
					if (seed >= 0)
						CHECK(std::find(s.begin(), s.end(), seed) != s.end());
			}
	}
	const auto roof = c::BotChooserSeeds(c::BotLawn::Roof);
	CHECK(std::find(roof.begin(), roof.end(), c::seed::FLOWERPOT) != roof.end());
	const auto pool = c::BotChooserSeeds(c::BotLawn::Pool);
	CHECK(std::find(pool.begin(), pool.end(), c::seed::LILYPAD) != pool.end());
}

TEST_CASE("CheatBotPlanPerCell") {
	// roof: a pot under everything, catapults on the left (straight shots hit the roof slope there)
	const c::BotCellPlan r0 = c::BotPlanFor(c::BotLawn::Roof, 0, false);
	CHECK(r0.base == c::seed::FLOWERPOT);
	CHECK(r0.main == c::seed::WINTERMELON);
	// pool water rows: a lily pad under everything
	CHECK(c::BotPlanFor(c::BotLawn::Pool, 4, true).base == c::seed::LILYPAD);
	CHECK(c::BotPlanFor(c::BotLawn::Pool, 4, false).base == -1);
	// spikerock at the right on land; none on water (no Spikeweed on a lily)
	CHECK(c::BotPlanFor(c::BotLawn::Day, 8, false).main == c::seed::SPIKEROCK);
	CHECK(c::BotPlanFor(c::BotLawn::Pool, 8, true).main != c::seed::SPIKEROCK);
	CHECK(c::BotPlanFor(c::BotLawn::Roof, 8, false).main != c::seed::SPIKEROCK);
	// pumpkins on the shooters, never on spikerock
	CHECK(c::BotPlanFor(c::BotLawn::Day, 3, false).armour == c::seed::PUMPKINSHELL);
	CHECK(c::BotPlanFor(c::BotLawn::Day, 8, false).armour == -1);
	// something against balloons in every row on land and water
	bool aCactus = false;
	for (int col = 0; col < 9; col++)
		aCactus |= c::BotPlanFor(c::BotLawn::Fog, col, false).main == c::seed::CACTUS;
	CHECK(aCactus);
	bool aCattail = false;
	for (int col = 0; col < 9; col++)
		aCattail |= c::BotPlanFor(c::BotLawn::Fog, col, true).main == c::seed::CATTAIL;
	CHECK(aCattail);
	// night: Gloom-shroom (awake at night); day: no mushrooms
	bool aGloom = false;
	for (int col = 0; col < 9; col++) {
		aGloom |= c::BotPlanFor(c::BotLawn::Night, col, false).main == c::seed::GLOOMSHROOM;
		CHECK(c::BotPlanFor(c::BotLawn::Day, col, false).main != c::seed::GLOOMSHROOM);
	}
	CHECK(aGloom);
}

// PVZU Plan 9 C2: "no recharge" ends a refresh through the packet's own update (SeedPacket::Update: Activate +
// FlashIfReady), where the level-1 tutorial moves from "refresh peashooter" to "pick up peashooter"; setting the
// packet ready directly skipped that, and 1-1 (first time) never sent a zombie (bot run u-l01: wave 0/4 for 17000
// frames).
TEST_CASE("TestCheatsNoRechargeFinishesRefreshThroughThePacket") {
	CHECK(c::NoRechargeFor(true, false) == c::NoRechargeStep::Leave);         // ready
	CHECK(c::NoRechargeFor(false, true) == c::NoRechargeStep::FinishRefresh); // recharging: counter to the end
	CHECK(c::NoRechargeFor(true, true) == c::NoRechargeStep::FinishRefresh);
	CHECK(c::NoRechargeFor(false, false) == c::NoRechargeStep::Activate);     // set inactive (C1 behaviour kept)
	CHECK(c::NoRechargeCounter(0, 750) == 750);   // SeedPacket::Update: ++counter > time -> ready on its next update
	CHECK(c::NoRechargeCounter(900, 750) == 900); // never goes back
}

// PVZU Plan 9 C2: with invincible plants, an Unnamed digger that bites underground (hook 654) or a balloon that eats
// while flying (patch 0x52f63e) chews one plant forever and nothing of the bot's can hit it: 4-6, 4-7, 6-2, 6-3 never
// ended (bot runs: 3..20 such zombies "eating" at x 680..710 for 25000 frames). The bot's guard kills them (logged).
TEST_CASE("TestCheatsBotGuardKillsStuckEaters") {
	CHECK(c::BotStuckEater(true, true, false, true));        // tunneling digger chewing an invincible plant
	CHECK(c::BotStuckEater(true, false, true, true));        // flying balloon chewing
	CHECK_FALSE(c::BotStuckEater(false, true, false, true)); // just tunneling
	CHECK_FALSE(c::BotStuckEater(true, false, false, true)); // a walker eating: plants can hit it
	CHECK_FALSE(c::BotStuckEater(true, true, false, false)); // plants not invincible: the plant dies, the zombie goes on
}

// PVZU Plan 9 C2: a stall (live zombies, none hurt or killed for theLimit frames) ends in the bot's kill-all, logged:
// with invincible plants and no shooter in the bank (0x71, 0x76, 0x89) zombies chew forever.
TEST_CASE("TestCheatsBotStallWatch") {
	c::BotStallWatch w;
	CHECK_FALSE(w.Update(0, 3, 900, 2000));
	CHECK_FALSE(w.Update(1500, 3, 900, 2000));
	CHECK(w.Update(2000, 3, 900, 2000));          // nothing changed for 2000 frames
	CHECK_FALSE(w.Update(2100, 3, 900, 2000));    // the watch restarts after a stall
	CHECK_FALSE(w.Update(3000, 3, 880, 2000));    // hurt: progress
	CHECK_FALSE(w.Update(4900, 3, 880, 2000));
	CHECK_FALSE(w.Update(5100, 4, 1150, 2000));   // a new zombie: progress
	CHECK_FALSE(w.Update(9000, 0, 0, 2000));      // no zombies: never a stall
	CHECK_FALSE(w.Update(9100, 2, 500, 2000));
}

// PVZU Plan 9 C2: the bot plays the two keyboard levels. 6-5 (shooter, Up / Down): the plant goes one row towards the
// zombie nearest the house. 7-5 / 0x89 (star snake, W A S D): the head runs a rectangle x 140..760, y 120..500 inside
// the losing edges (x 0..899, y 40..599), turning 90 degrees only.
TEST_CASE("TestCheatsBotKeyboardLevels") {
	CHECK(c::BotShooterStep(0, 3) == 1);
	CHECK(c::BotShooterStep(4, 1) == -1);
	CHECK(c::BotShooterStep(2, 2) == 0);
	CHECK(c::BotShooterStep(2, -1) == 0); // no zombie
	using c::BotTurn;
	CHECK(c::BotSnakeTurn(400, 280, 3, 0) == BotTurn::None);  // the start: right along row 2
	CHECK(c::BotSnakeTurn(761, 280, 3, 0) == BotTurn::Down);
	CHECK(c::BotSnakeTurn(761, 501, 0, 3) == BotTurn::Left);
	CHECK(c::BotSnakeTurn(139, 501, -3, 0) == BotTurn::Up);
	CHECK(c::BotSnakeTurn(139, 119, 0, -3) == BotTurn::Right);
	CHECK(c::BotSnakeTurn(500, 119, 3, 0) == BotTurn::None);
	CHECK(c::BotSnakeTurn(761, 119, 3, 0) == BotTurn::Down);
	CHECK(c::BotSnakeTurn(761, 300, 0, 3) == BotTurn::None);
	CHECK(c::BotSnakeTurn(780, 119, 0, -3) == BotTurn::Left);  // never turns towards a near edge
	CHECK(c::BotSnakeTurn(130, 501, 0, 3) == BotTurn::Right);
}

// ---- the in-game "Cheats" button (2026-11-25: "change cheat setting during a game, so I need to see the button
// to open cheats menu") ----
TEST_CASE("TestCheatsGameButtonToggleDefaultsOffAndRoundTrips") {
	const c::CheatInfo& i = c::InfoFor(c::CheatId::GameButton);
	CHECK(std::string(i.key) == "gameButton");
	CHECK(i.values == 2);
	CHECK(std::string(i.labelKey) == "PVZU_CHEAT_GAME_BUTTON");
	CHECK_FALSE(c::Cheats{}.On(c::CheatId::GameButton));
	// a settings file written before the toggle existed: off
	CHECK_FALSE(c::CheatsFromMap({{"infiniteSun", 1}, {"speed", 2}}).On(c::CheatId::GameButton));

	c::Cheats t;
	CHECK(t.Cycle(c::CheatId::GameButton) == 1);
	t.Set(c::CheatId::InfiniteSun, 1);
	std::map<std::string, int> m = c::CheatsToMap(t);
	CHECK(m.at("gameButton") == 1);
	fs::path dir = fs::temp_directory_path() / "pvzu-cheats-button-test";
	fs::remove_all(dir);
	fs::create_directories(dir);
	Settings s;
	s.testCheats = m;
	REQUIRE(SaveSettings(dir / "pvzu-settings.json", s));
	const c::Cheats back = c::CheatsFromMap(LoadSettings(dir / "pvzu-settings.json").testCheats);
	CHECK(back == t);
	CHECK(back.On(c::CheatId::GameButton));
	CHECK(back.On(c::CheatId::InfiniteSun));
	t.Cycle(c::CheatId::GameButton); // off again
	s.testCheats = c::CheatsToMap(t);
	REQUIRE(SaveSettings(dir / "pvzu-settings.json", s));
	CHECK_FALSE(c::CheatsFromMap(LoadSettings(dir / "pvzu-settings.json").testCheats).On(c::CheatId::GameButton));
	fs::remove_all(dir);
}

TEST_CASE("TestCheatsGameButtonShownOnlyInAPlayingLevelWithBothSwitchesOn") {
	c::GameButtonState s;
	s.feature = true;
	s.toggle = true;
	s.inLevel = true;
	CHECK(c::GameButtonShown(s));
	// vanilla mode / the pack without testCheats: never, whatever the saved toggle says
	auto off = s;
	off.feature = false;
	CHECK_FALSE(c::GameButtonShown(off));
	off = s;
	off.toggle = false;
	CHECK_FALSE(c::GameButtonShown(off));
	off = s;
	off.inLevel = false; // menus, seed chooser, level intro, fade-out after the award, level complete
	CHECK_FALSE(c::GameButtonShown(off));
	off = s;
	off.dialogOpen = true; // Menu, the options panel, the cheats page itself
	CHECK_FALSE(c::GameButtonShown(off));
	off = s;
	off.paused = true;
	CHECK_FALSE(c::GameButtonShown(off));
	off = s;
	off.daveTalking = true; // the bar slides away while Dave talks
	CHECK_FALSE(c::GameButtonShown(off));
	off = s;
	off.gardenOrTree = true; // Zen Garden and the Tree of Wisdom are not levels
	CHECK_FALSE(c::GameButtonShown(off));
	CHECK_FALSE(c::GameButtonShown(c::GameButtonState{}));
}

TEST_CASE("TestCheatsGameButtonPlaceOnThePhoneAndAt800") {
	constexpr int kLawnTop = 80; // Board grid row 0 starts at y 80 (vanilla GridToPixelY)
	const HitRect aMenu800{681, -10, 117, 46}; // Board::Board's Menu button
	// phone 1300 x 600 with the HUD (camera hole left, or rounded corner right): Menu kept 119 px from the right inset.
	// 2026-11-26 (S26): "Cheat game button in game level it's not aligned with the menu one" -> the button is a
	// copy of Menu's stone: same top, height and width (the same art, so the label sits at the same height), a
	// kCardGap gap to Menu's left, wherever Menu is (its rect is read at run time).
	for (auto [l, r] : {std::pair{57, 0}, {0, 57}, {0, 0}, {57, 57}}) {
		for (int aMenuY : {-10, 0, -4}) {
			CAPTURE(l);
			CAPTURE(r);
			CAPTURE(aMenuY);
			const HitRect aMenu{1300 - r - 119, aMenuY, 117, 46};
			const HitRect aSpeed = TopBarRect(HudBox::Speed);
			const int aBarRight = l + aSpeed.x + aSpeed.w;
			const c::GameButtonPlace p = c::GameButtonRect(aMenu, aBarRight);
			CHECK_FALSE(p.underMenu);
			CHECK(p.draw.y == aMenu.y); // same top as Menu
			CHECK(p.draw.h == aMenu.h); // same height: the label's vertical centre matches
			CHECK(p.draw.w == aMenu.w); // same stone art width as Menu
			CHECK(p.draw.x + p.draw.w + kCardGap == aMenu.x); // a small, fixed gap to Menu's left
			CHECK(p.draw.x >= aBarRight + kCardGap); // clear of sun, shovel, speed
			CHECK(p.hit.x == p.draw.x);
			CHECK(p.hit.w == p.draw.w);
			CHECK(p.hit.x + p.hit.w <= aMenu.x);
			CHECK(p.hit.h >= kMinTouchSize); // finger-sized
			CHECK(p.hit.w >= kMinTouchSize);
			CHECK(p.hit.y == 0); // from the screen top
			CHECK(p.hit.y + p.hit.h >= aMenu.y + aMenu.h); // all of the stone that is on the screen
			CHECK(p.hit.y + p.hit.h < kLawnTop); // never over the lawn
			CHECK(Inside(p.hit, p.draw.x + 2, std::max(0, p.draw.y) + 2));
		}
	}
	// 800 wide, vanilla top bank: 6 slots leave room left of Menu, in line with it the same way
	{
		const int aShovelRight = 456 + 70; // Board::GetShovelButtonRect, ShovelBank.png 70 wide
		const c::GameButtonPlace p = c::GameButtonRect(aMenu800, aShovelRight);
		CHECK_FALSE(p.underMenu);
		CHECK(p.draw.x >= aShovelRight + kCardGap);
		CHECK(p.draw.y == aMenu800.y);
		CHECK(p.draw.h == aMenu800.h);
		CHECK(p.draw.w == aMenu800.w);
		CHECK(p.draw.x + p.draw.w + kCardGap == aMenu800.x);
		CHECK(p.hit.x + p.hit.w <= aMenu800.x);
		CHECK(p.hit.y + p.hit.h < kLawnTop);
	}
	// 7..10 slots push the shovel up to Menu: the button goes under Menu (Menu's column and art), still above the lawn
	// and on the screen; the 10-slot shovel ends 2 px left of Menu, so the column is as clear of it as Menu is
	for (int aExtra : {60, 76, 112, 153}) {
		CAPTURE(aExtra);
		const int aShovelRight = 456 + aExtra + 70;
		const c::GameButtonPlace p = c::GameButtonRect(aMenu800, aShovelRight);
		CHECK(p.underMenu);
		CHECK(p.draw.x == aMenu800.x);
		CHECK(p.draw.w == aMenu800.w);
		CHECK(p.draw.h == aMenu800.h);
		CHECK(p.draw.x >= aShovelRight);
		CHECK(p.draw.x + p.draw.w <= 800);
		CHECK(p.draw.y == aMenu800.y + aMenu800.h); // right under Menu's stone
		CHECK(p.hit.y >= aMenu800.y + aMenu800.h);
		CHECK(p.hit.y + p.hit.h <= kLawnTop);
		CHECK(p.hit.h >= 40);
		CHECK(p.hit.w >= kMinTouchSize);
	}
}
