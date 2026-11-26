// PVZU: Unnamed 1.3beta2 mod-only game modes 0x65..0x94 (48 modes; 0x93/0x94 are "Unnamed Level" placeholders).
// Evidence: the research notes (modes-ui-shop) 2.3 + pvz-unnamed-pack/SOURCES-mod-modes.md. Pure: the per-mode table comes from
// the pack's data/modes.json (SetModModes); the engine reads it only with GameData::Get().Feature("modModes")
// (Mod/Engine/ModModes.h). In the exe a mod mode is a plain mGameMode value above the vanilla ones, so the vanilla
// code treats it as a generic challenge (IsChallengeMode) and the mod's hooks add its rules on top.
#pragma once

#include "nlohmann/json.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace pvzu::unnamed {

inline constexpr int kFirstModMode = 0x65;
inline constexpr int kLastModMode = 0x94;
inline constexpr int kNumModModes = kLastModMode - kFirstModMode + 1;
inline constexpr int kNoModRule = -1;
// PlayerInfo::mChallengeRecords is indexed mode - 1 (LawnApp::GetCurrentChallengeIndex): mod modes need 148 entries.
inline constexpr int kNumRecordsWithModModes = kLastModMode; // index 0..147
inline constexpr int kNumVanillaRecords = 100;

bool IsModModeValue(int theMode);
int ModModeRecordIndex(int theMode); // theMode - 1
// LawnApp::HasBeatenChallenge for a mod mode (before its vanilla index assert): true and theBeaten = record > 0, or
// false for a vanilla mode. theRecords holds kNumRecordsWithModModes entries (PlayerInfo::mChallengeRecords).
bool ModModeBeaten(int theMode, const uint32_t* theRecords, bool& theBeaten);
// PlayerInfo::DeleteUserFiles: the mod-mode files of a profile in its user-data folder: user<id>-modes.json (records)
// and the game saves game<id>_<mode>.v4 / .dat (GetSavedGameName / GetLegacySavedGameName) of modes 101..148.
std::vector<std::string> ModModeUserFileNames(int theProfileId);

// built = playable here; needsModPlants = waits for the mod-only plants (Plan 8); unknown = rules not read yet;
// placeholder = the exe's own empty "Unnamed Level" slots.
enum class ModModeStatus { Built, NeedsModPlants, Unknown, Placeholder };
const char* ModModeStatusName(ModModeStatus theStatus);

struct ModModeDef {
	int mode = 0;
	std::string key;                   // "artIsAnExplosion": names the rule set in Mod/Engine/ModModes.cpp
	std::string name;                  // English (translation of the GBK name at 0x850000 + (mode-0x65)*0x10)
	std::string nameZh;                // the exe's name
	int background = 0;                // engine BackgroundType (= exe Board+0x554c), hook 25
	std::vector<std::string> allowed;  // the mode's own zombie keys (engine names without "ZOMBIE_"), hook 180 chain
	bool normal = true;                // Normal zombie in the list (0x79, 0x85: written false)
	bool pail = true;                  // Pail (and the vanilla Yeti line) kept; false = the 0x79af0b exit
	bool chooser = true;               // seed chooser shown (hooks 35/36)
	bool conveyor = false;             // conveyor belt bank (hook 140)
	std::vector<std::pair<std::string, int>> conveyorSeeds; // belt seeds and weights (hook 162)
	std::vector<std::string> bank;     // fixed packets written at InitLevel (no chooser), engine SeedType names
	bool skySun = true;                // falling sun (hook 94 turns it off)
	std::string nameColor = "gold";    // level-name colour (hook 130): gold red purple blue green pulse
	int seedSlots = kNoModRule;        // hook 142 GetNumSeedsInBank
	std::vector<std::string> bannedSeeds; // seed chooser hides these (hook 425); engine SeedType names without "SEED_"
	int firstWaveCountdown = kNoModRule;
	int waves = kNoModRule;            // kNoModRule = vanilla (40 for a generic challenge)
	int startSun = kNoModRule;
	int graves = kNoModRule;           // 0 = none, 1 = vanilla rule (night), kNoModRule = vanilla
	int icon = 0;                      // challenge thumbnail cell for the list screen
	ModModeStatus status = ModModeStatus::Unknown;
	std::string note;                  // why it is not built (shown in the list screen's tool tip line)
	nlohmann::json rules = nlohmann::json::object(); // the mode's own numbers (see SOURCES-mod-modes.md)
	bool Playable() const { return status == ModModeStatus::Built; }
	int Rule(std::string_view theKey, int theDefault) const;
};

// Object keyed "0x65".. "0x94". On any bad row: false, *err says which, no rows kept.
bool SetModModes(const nlohmann::json& theTable, std::string* err);
void ClearModModes();
const ModModeDef* FindModMode(int theMode);
std::vector<int> ModModeMenuOrder(); // every row, in mode order

// The mode's zombie allow list (without Yeti): Normal unless `normal` is false, the mode's own list, Cone when the mode
// has no list (the chain's fall-through 0x7a5e96), Pail when `pail`. Yeti follows Pail (the vanilla line after it).
std::vector<std::string> ModModeZombies(const ModModeDef& theDef);

// Level-name colour (hook 130, 0x7a3300). theCounter = Board::mMainCounter (for "pulse").
struct Rgb {
	int r = 224, g = 187, b = 98;
	bool operator==(const Rgb&) const = default;
};
Rgb ModModeNameColor(std::string_view theName, int theCounter);

// The list screen (Mod/Engine/ModModesScreen.cpp): a scrolling grid like the challenge page (800 x 600 screen).
struct ModeListLayout {
	int cols = 5;
	int left = 38, top = 93;      // first cell, as the challenge page (ChallengeScreen ctor: 38 + col*155, 93 + row*119)
	int colW = 155, rowH = 119;
	int cellW = 104, cellH = 115;
	int viewH = 440;              // visible list height (93..533; the back button sits at 568)
	int dragSlop = 8;             // px of finger travel before a press becomes a scroll
	int Rows(int theCount) const { return (theCount + cols - 1) / cols; }
	int MaxScroll(int theCount) const;
	int ClampScroll(int theScroll, int theCount) const;
	void CellPos(int theIndex, int theScroll, int& x, int& y) const;
	int HitTest(int x, int y, int theScroll, int theCount) const; // -1 = none
	bool IsDrag(int theFromY, int theToY) const;
};

} // namespace pvzu::unnamed
