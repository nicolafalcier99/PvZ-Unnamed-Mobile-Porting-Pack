// PVZU: start-up wiring: read pvzu-settings.json, load the Unnamed pack if wanted, pick the mode.
#pragma once

#include "Mod/Mode.h"

#include <filesystem>
#include <string>

namespace pvzu::engine {

std::filesystem::path SettingsFile();   // <appdata>/pvzu-settings.json
std::filesystem::path UnnamedPackDir(); // -pvzu-pack <dir>, else <appdata>/mods/unnamed
bool UnnamedPackInstalled(std::string* err);

// Called once from LawnApp::Init before profiles load. Logs why Unnamed fell back to Vanilla.
void ApplyStartupMode();

const Settings& StartupSettings();
// Saves a new wanted mode/language for the next start. Returns false on I/O error.
bool SaveWantedSettings(const Settings& settings);
// The Unnamed options panel (Mod/Engine/OptionsPanel.h): writes only "unnamedOptions", on top of the file as it is.
bool SaveUnnamedOptions(const std::map<std::string, int>& theOptions);
// The panel's CARD row: writes only "card", on top of the file as it is.
bool SaveCardSetting(bool theOn);

// Adventure zombie-hand scene (Mod/MenuIntro.h). Call when Adventure is tapped: true = play the 4.5 s scene (first
// time, or after a year away), false = start the level at once. Records the times in pvzu-settings.json.
bool TakeAdventureIntro();

} // namespace pvzu::engine
