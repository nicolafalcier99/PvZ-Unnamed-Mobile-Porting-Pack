// PVZU: Vanilla / Unnamed mode. The mode is a setting read at start-up (switching asks for a restart,
// because images and profiles are loaded once). Each mode has its own save folder.
#pragma once

#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <string_view>

namespace pvzu {

enum class GameMode { Vanilla, Unnamed };

struct Settings {
	GameMode mode = GameMode::Vanilla;  // what the player picked
	std::string language = "en";        // "en" | "zh" (Unnamed text)
	int speed = 1;                      // play speed 1 or 2 (the HUD's speed button; Unnamed has a speed-up too)
	std::optional<bool> hd;             // HD art (Mod/HdArt.h): unset = on when an HD pack is installed
	std::optional<int64_t> introSeenAt;  // unix s: the Adventure zombie-hand scene last played (Mod/MenuIntro.h)
	std::optional<int64_t> lastPlayedAt; // unix s: the game was last started / played
	bool card = false;                  // Unnamed "CARD" toggle: mod-only plants in the chooser (Mod/Unnamed/ModPlants.h)
	std::map<std::string, int> unnamedOptions; // Unnamed settings/cheats panel by key (Mod/Unnamed/Options.h); empty = defaults
	std::map<std::string, int> testCheats;     // PVZU Plan 9 C1: the test cheats + level-jump return point (Mod/TestCheats.h)
};

// The speed button: 1x -> 2x -> 1x (anything else -> 1x), and its label ("1x", "2x").
int NextSpeed(int speed);
std::string SpeedLabel(int speed);

// "userdata" (vanilla, the upstream folder) or "userdata-unnamed".
const char* UserDataDirName(GameMode mode);
// "<dir>/<file>" relative to the app data folder; just "<dir>" for an empty file.
std::string UserDataPath(GameMode mode, std::string_view file);

// Missing or unreadable file, or unknown values -> defaults (Vanilla, "en").
Settings LoadSettings(const std::filesystem::path& file);
bool SaveSettings(const std::filesystem::path& file, const Settings& settings);

// Unnamed only when its pack loaded; otherwise Vanilla.
GameMode EffectiveMode(GameMode wanted, bool packLoaded);

// The mode in effect for this run (set once at start-up).
GameMode CurrentMode();
void SetCurrentMode(GameMode mode);
// Shorthand used at engine save-path sites.
inline std::string CurrentUserDataPath(std::string_view file) { return UserDataPath(CurrentMode(), file); }

} // namespace pvzu
