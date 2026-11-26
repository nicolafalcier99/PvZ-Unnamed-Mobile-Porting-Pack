#include "Mod/Mode.h"

#include "nlohmann/json.hpp"

#include <fstream>

namespace pvzu {
using nlohmann::json;

const char* UserDataDirName(GameMode mode) {
	return mode == GameMode::Unnamed ? "userdata-unnamed" : "userdata";
}

std::string UserDataPath(GameMode mode, std::string_view file) {
	std::string out = UserDataDirName(mode);
	if (!file.empty()) {
		out += '/';
		out += file;
	}
	return out;
}

Settings LoadSettings(const std::filesystem::path& file) {
	Settings s;
	std::ifstream in(file, std::ios::binary);
	if (!in)
		return s;
	json j = json::parse(in, nullptr, /*allow_exceptions=*/false);
	if (!j.is_object())
		return s;
	if (j.contains("mode") && j["mode"].is_string() && j["mode"].get<std::string>() == "unnamed")
		s.mode = GameMode::Unnamed;
	if (j.contains("language") && j["language"].is_string()) {
		std::string lang = j["language"].get<std::string>();
		if (lang == "en" || lang == "zh")
			s.language = lang;
	}
	if (j.contains("speed") && j["speed"].is_number_integer() && j["speed"].get<int>() == 2)
		s.speed = 2;
	if (j.contains("hd") && j["hd"].is_boolean())
		s.hd = j["hd"].get<bool>();
	if (j.contains("introSeenAt") && j["introSeenAt"].is_number_integer())
		s.introSeenAt = j["introSeenAt"].get<int64_t>();
	if (j.contains("lastPlayedAt") && j["lastPlayedAt"].is_number_integer())
		s.lastPlayedAt = j["lastPlayedAt"].get<int64_t>();
	if (j.contains("card") && j["card"].is_boolean())
		s.card = j["card"].get<bool>();
	if (j.contains("unnamedOptions") && j["unnamedOptions"].is_object())
		for (const auto& [aKey, aValue] : j["unnamedOptions"].items())
			if (aValue.is_number_integer())
				s.unnamedOptions[aKey] = aValue.get<int>();
	if (j.contains("testCheats") && j["testCheats"].is_object())
		for (const auto& [aKey, aValue] : j["testCheats"].items())
			if (aValue.is_number_integer())
				s.testCheats[aKey] = aValue.get<int>();
	return s;
}

bool SaveSettings(const std::filesystem::path& file, const Settings& s) {
	json j = {{"mode", s.mode == GameMode::Unnamed ? "unnamed" : "vanilla"}, {"language", s.language},
		{"speed", s.speed == 2 ? 2 : 1}};
	if (s.hd.has_value())
		j["hd"] = *s.hd;
	if (s.introSeenAt.has_value())
		j["introSeenAt"] = *s.introSeenAt;
	if (s.lastPlayedAt.has_value())
		j["lastPlayedAt"] = *s.lastPlayedAt;
	if (s.card)
		j["card"] = true;
	if (!s.unnamedOptions.empty())
		j["unnamedOptions"] = s.unnamedOptions;
	if (!s.testCheats.empty())
		j["testCheats"] = s.testCheats;
	std::error_code ec;
	if (file.has_parent_path())
		std::filesystem::create_directories(file.parent_path(), ec);
	std::ofstream out(file, std::ios::binary);
	if (!out)
		return false;
	out << j.dump(1, '\t') << '\n';
	return static_cast<bool>(out);
}

int NextSpeed(int speed) { return speed == 1 ? 2 : 1; }

std::string SpeedLabel(int speed) { return std::to_string(speed == 2 ? 2 : 1) + "x"; }

GameMode EffectiveMode(GameMode wanted, bool packLoaded) {
	return (wanted == GameMode::Unnamed && packLoaded) ? GameMode::Unnamed : GameMode::Vanilla;
}

namespace {
GameMode gMode = GameMode::Vanilla;
}
GameMode CurrentMode() { return gMode; }
void SetCurrentMode(GameMode mode) { gMode = mode; }

} // namespace pvzu
