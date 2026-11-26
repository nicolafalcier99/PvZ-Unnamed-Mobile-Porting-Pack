#include "Mod/Unnamed/Rules.h"
#include "Mod/Unnamed/LaterLevels.h"
#include "Mod/AssetOverlay.h"
#include <set>
#include "Mod/Engine/Startup.h"
#include "Mod/MenuIntro.h"

#include <chrono>
#include "Mod/CmdLine.h"
#include "Mod/Engine/EngineTables.h"
#include "Mod/Engine/HdTextures.h"
#include "Mod/Engine/ModPlants.h"
#include "Mod/GameData.h"
#include "Mod/ModPack.h"

#include "Common.h"
#include "PvzpLib/PvzpDebug.h"

namespace pvzu::engine {

namespace {
Settings gStartupSettings;
std::optional<int64_t> gPreviousPlay; // lastPlayedAt as found at start-up (before this run)
bool gIntroShownThisRun = false;

int64_t NowUnix() {
	return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

// Writes only the time fields, on top of the file as it is (a dev -pvzu-mode override must not be saved).
void SaveTimes(std::optional<int64_t> theIntroSeenAt, int64_t theLastPlayed) {
	Settings aFile = LoadSettings(SettingsFile());
	if (theIntroSeenAt)
		aFile.introSeenAt = gStartupSettings.introSeenAt = theIntroSeenAt;
	aFile.lastPlayedAt = gStartupSettings.lastPlayedAt = theLastPlayed;
	SaveSettings(SettingsFile(), aFile);
}
}

std::filesystem::path SettingsFile() { return std::filesystem::path(Sexy::GetAppDataPath("pvzu-settings.json")); }

std::filesystem::path UnnamedPackDir() {
	if (auto dir = CmdOption("-pvzu-pack"); dir && !dir->empty())
		return std::filesystem::path(*dir);
	return std::filesystem::path(Sexy::GetAppDataPath("mods/unnamed"));
}

bool UnnamedPackInstalled(std::string* err) {
	return LoadModPack(UnnamedPackDir(), err).has_value();
}

void ApplyStartupMode() {
	gStartupSettings = LoadSettings(SettingsFile());
	gPreviousPlay = gStartupSettings.lastPlayedAt;
	SaveTimes(std::nullopt, NowUnix());
	if (auto m = CmdOption("-pvzu-mode")) // dev override: -pvzu-mode unnamed|vanilla
		gStartupSettings.mode = (*m == "unnamed") ? GameMode::Unnamed : GameMode::Vanilla;

	bool packLoaded = false;
	std::optional<ModPack> pack; // PVZU Plan 8: kept for the mod-plant table (read after SyncActiveTables)
	if (gStartupSettings.mode == GameMode::Unnamed) {
		std::string err;
		pack = LoadModPack(UnnamedPackDir(), &err);
		packLoaded = pack && GameData::Get().ApplyPack(*pack, &err);
		if (packLoaded) {
			unnamed::SetRules(pack->rules);
			std::string levelsErr; // PVZU Plan 7: data/levels-*.json (Adventure 51-73); a bad table keeps no rows
			if (!unnamed::SetLaterLevels(pack->levels, &levelsErr))
				PvzpLogLn("PVZU: level table ignored: {}", levelsErr);
		}
		if (!packLoaded)
			PvzpLogLn("PVZU: Unnamed mode unavailable, using Vanilla: {}", err);
	}
	SetCurrentMode(EffectiveMode(gStartupSettings.mode, packLoaded));
	if (CurrentMode() == GameMode::Unnamed) {
		// English: skip Unnamed's translated UI art (baked Chinese text), keep its game art. Every language: skip art
		// laid out for Unnamed's 1067-wide exe (widened menu backs, 13-cell seeds.png).
		SetOverlayRoot(UnnamedPackDir() / "assets", LoadOverlaySkipLists(UnnamedPackDir() / "data", gStartupSettings.language));
		if (!std::filesystem::exists(UnnamedPackDir() / "assets" / "import-manifest.json"))
			PvzpLogLn("PVZU: Unnamed art not imported yet ({}); using vanilla art", (UnnamedPackDir() / "assets").string());
	} else {
		SetOverlayRoot(std::nullopt);
	}
	if (CurrentMode() == GameMode::Vanilla) {
		GameData::Get().ResetToVanilla();
		unnamed::SetRules(nlohmann::json::object());
		unnamed::ClearLaterLevels();
	}
	SyncActiveTables();
	SetUpModPlants(CurrentMode() == GameMode::Unnamed && pack ? &*pack : nullptr); // PVZU Plan 8: data/mod-plants.json
	hd::Init(); // after the overlay root: overlay art wins over HD twins
	PvzpLogLn("PVZU: mode {}", CurrentMode() == GameMode::Unnamed ? "Unnamed" : "Vanilla");
}

const Settings& StartupSettings() { return gStartupSettings; }

bool TakeAdventureIntro() {
	const int64_t aNow = NowUnix();
	const bool aPlay = ShouldPlayAdventureIntro(gStartupSettings.introSeenAt, gPreviousPlay, aNow, gIntroShownThisRun);
	if (aPlay)
		gIntroShownThisRun = true;
	SaveTimes(aPlay ? std::optional<int64_t>(aNow) : std::nullopt, aNow);
	return aPlay;
}

bool SaveUnnamedOptions(const std::map<std::string, int>& theOptions) {
	Settings aFile = LoadSettings(SettingsFile());
	aFile.unnamedOptions = gStartupSettings.unnamedOptions = theOptions;
	return SaveSettings(SettingsFile(), aFile);
}

bool SaveCardSetting(bool theOn) {
	Settings aFile = LoadSettings(SettingsFile());
	aFile.card = gStartupSettings.card = theOn;
	return SaveSettings(SettingsFile(), aFile);
}

bool SaveWantedSettings(const Settings& settings) {
	gStartupSettings = settings;
	return SaveSettings(SettingsFile(), settings);
}

} // namespace pvzu::engine
