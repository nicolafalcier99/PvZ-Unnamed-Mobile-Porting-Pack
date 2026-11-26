// PVZU: a mod pack on disk: <dir>/mod.json + optional data/plants.json, data/zombies.json,
// data/projectiles.json. Tables are keyed by engine name; every field is optional (missing = vanilla).
#pragma once

#include "nlohmann/json.hpp"

#include <filesystem>
#include <map>
#include <optional>
#include <string>

namespace pvzu {

struct GameTables;

struct ModPack {
	std::string id, name, version;
	std::filesystem::path root;
	nlohmann::json plants = nlohmann::json::object();
	nlohmann::json zombies = nlohmann::json::object();
	nlohmann::json projectiles = nlohmann::json::object();
	std::map<std::string, bool, std::less<>> features;
	nlohmann::json rules = nlohmann::json::object(); // data/rules.json (Unnamed constants)
	nlohmann::json levels = nlohmann::json::object(); // data/levels-*.json: per-level rows keyed by level number
	nlohmann::json modPlants = nlohmann::json::object(); // data/mod-plants.json: Unnamed's mod-only plants by seed (Plan 8)
};

// Reads and syntax-checks a pack. Returns nullopt and fills *err on missing dir, bad JSON or a
// missing/invalid mod.json. Field-level checks happen in GameData::ApplyPack.
std::optional<ModPack> LoadModPack(const std::filesystem::path& dir, std::string* err);

// A pack that sets every field to the given tables (used for dumps and round-trip tests).
ModPack PackFromTables(const GameTables& tables, const std::string& id);

} // namespace pvzu
