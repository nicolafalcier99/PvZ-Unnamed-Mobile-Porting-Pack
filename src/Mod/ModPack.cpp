#include "Mod/ModPack.h"
#include "Mod/GameData.h"

#include <algorithm>
#include <fstream>
#include <vector>

namespace pvzu {
namespace fs = std::filesystem;
using nlohmann::json;

namespace {

bool ReadJsonFile(const fs::path& file, json* out, std::string* err) {
	std::ifstream in(file, std::ios::binary);
	if (!in) {
		*err = "cannot open " + file.string();
		return false;
	}
	try {
		*out = json::parse(in);
	} catch (const json::exception& e) {
		*err = file.filename().string() + ": " + e.what();
		return false;
	}
	return true;
}

bool ReadOptionalTable(const fs::path& file, json* out, std::string* err) {
	std::error_code ec;
	if (!fs::exists(file, ec))
		return true;
	if (!ReadJsonFile(file, out, err))
		return false;
	if (!out->is_object()) {
		*err = file.filename().string() + ": top level must be an object keyed by name";
		return false;
	}
	return true;
}

// Every <dataDir>/<prefix>*.json in name order into *out; a key set by two files is an error.
bool MergeSplitFiles(const fs::path& dataDir, const std::string& prefix, const char* what, json* out, std::string* err) {
	std::error_code ec;
	std::vector<fs::path> files;
	for (const auto& entry : fs::directory_iterator(dataDir, ec)) {
		const std::string fn = entry.path().filename().string();
		if (fn.starts_with(prefix) && fn.ends_with(".json"))
			files.push_back(entry.path());
	}
	std::sort(files.begin(), files.end());
	for (const auto& file : files) {
		json part = json::object();
		if (!ReadOptionalTable(file, &part, err))
			return false;
		for (auto& [k, v] : part.items()) {
			if (out->contains(k)) {
				*err = file.filename().string() + ": " + what + " \"" + k + "\" already set in another " + prefix + "* file";
				return false;
			}
			(*out)[k] = v;
		}
	}
	return true;
}

} // namespace

std::optional<ModPack> LoadModPack(const fs::path& dir, std::string* err) {
	std::error_code ec;
	if (!fs::is_directory(dir, ec)) {
		*err = "mod pack folder not found: " + dir.string();
		return std::nullopt;
	}
	json meta;
	if (!ReadJsonFile(dir / "mod.json", &meta, err))
		return std::nullopt;
	if (!meta.is_object() || !meta.contains("id") || !meta["id"].is_string()) {
		*err = "mod.json: needs a string \"id\"";
		return std::nullopt;
	}
	ModPack pack;
	pack.root = dir;
	pack.id = meta["id"].get<std::string>();
	pack.name = meta.value("name", pack.id);
	pack.version = meta.value("version", "");
	if (meta.contains("features")) {
		if (!meta["features"].is_object()) {
			*err = "mod.json: \"features\" must be an object of true/false";
			return std::nullopt;
		}
		for (auto& [k, v] : meta["features"].items()) {
			if (!v.is_boolean()) {
				*err = "mod.json: feature \"" + k + "\" must be true or false";
				return std::nullopt;
			}
			pack.features[k] = v.get<bool>();
		}
	}
	if (!ReadOptionalTable(dir / "data" / "plants.json", &pack.plants, err) ||
		!ReadOptionalTable(dir / "data" / "zombies.json", &pack.zombies, err) ||
		!ReadOptionalTable(dir / "data" / "projectiles.json", &pack.projectiles, err) ||
		!ReadOptionalTable(dir / "data" / "rules.json", &pack.rules, err) ||
		!ReadOptionalTable(dir / "data" / "mod-plants.json", &pack.modPlants, err))
		return std::nullopt;
	// data/rules-*.json: one file per area (so parallel work never edits the same file); merged in name order.
	// data/levels-*.json (PVZU Plan 7): per-level rows keyed by level number, merged the same way.
	if (!MergeSplitFiles(dir / "data", "rules-", "rule", &pack.rules, err) ||
		!MergeSplitFiles(dir / "data", "levels-", "level", &pack.levels, err))
		return std::nullopt;
	return pack;
}

ModPack PackFromTables(const GameTables& t, const std::string& id) {
	ModPack p;
	p.id = id;
	p.name = id;
	for (const auto& d : t.plants)
		p.plants[d.name] = {{"seedCost", d.seedCost}, {"refreshTime", d.refreshTime}, {"launchRate", d.launchRate}};
	for (const auto& d : t.zombies)
		p.zombies[d.name] = {{"zombieValue", d.zombieValue}, {"startingLevel", d.startingLevel},
			{"firstAllowedWave", d.firstAllowedWave}, {"pickWeight", d.pickWeight}};
	for (const auto& d : t.projectiles)
		p.projectiles[d.name] = {{"damage", d.damage}};
	return p;
}

} // namespace pvzu
