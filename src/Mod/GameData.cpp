#include "Mod/GameData.h"
#include "Mod/ModPack.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace pvzu {
using nlohmann::json;

namespace {

// Applies one table: for each "NAME": {field: int} in src, finds the row by name and sets the field.
template <typename Row, typename Setter>
bool ApplyTable(const json& src, std::vector<Row>& rows, const char* what, Setter set, std::string* err) {
	for (auto& [name, fields] : src.items()) {
		auto it = std::find_if(rows.begin(), rows.end(), [&](const Row& r) { return r.name == name; });
		if (it == rows.end()) {
			*err = std::string(what) + ": unknown name \"" + name + "\"";
			return false;
		}
		if (!fields.is_object()) {
			*err = std::string(what) + " \"" + name + "\": must be an object of fields";
			return false;
		}
		for (auto& [field, value] : fields.items()) {
			if (!value.is_number_integer()) {
				*err = std::string(what) + " \"" + name + "\"." + field + ": must be a whole number";
				return false;
			}
			if (!set(*it, field, value.get<int>())) {
				*err = std::string(what) + " \"" + name + "\": unknown field \"" + field + "\"";
				return false;
			}
		}
	}
	return true;
}

bool SetPlant(PlantDefData& r, const std::string& f, int v) {
	if (f == "seedCost") r.seedCost = v;
	else if (f == "refreshTime") r.refreshTime = v;
	else if (f == "launchRate") r.launchRate = v;
	else return false;
	return true;
}
bool SetZombie(ZombieDefData& r, const std::string& f, int v) {
	if (f == "zombieValue") r.zombieValue = v;
	else if (f == "startingLevel") r.startingLevel = v;
	else if (f == "firstAllowedWave") r.firstAllowedWave = v;
	else if (f == "pickWeight") r.pickWeight = v;
	else return false;
	return true;
}
bool SetProjectile(ProjectileDefData& r, const std::string& f, int v) {
	if (f == "damage") r.damage = v;
	else return false;
	return true;
}

template <typename Row>
const Row& Find(const std::vector<Row>& rows, std::string_view name) {
	for (const auto& r : rows)
		if (r.name == name)
			return r;
	throw std::out_of_range("pvzu::GameData: unknown name " + std::string(name));
}

} // namespace

GameData& GameData::Get() {
	static GameData instance;
	return instance;
}

namespace {
template <typename Row>
bool NamesUnique(const std::vector<Row>& rows) {
	std::vector<std::string_view> names;
	for (const auto& r : rows) names.push_back(r.name);
	std::sort(names.begin(), names.end());
	return std::adjacent_find(names.begin(), names.end()) == names.end();
}
}

bool GameData::SetVanilla(GameTables vanilla) {
	if (!NamesUnique(vanilla.plants) || !NamesUnique(vanilla.zombies) || !NamesUnique(vanilla.projectiles))
		return false;
	mVanilla = std::move(vanilla);
	ResetToVanilla();
	return true;
}

void GameData::ResetToVanilla() {
	mActive = mVanilla;
	mFeatures.clear();
	mPackId.clear();
}

bool GameData::ApplyPack(const ModPack& pack, std::string* err) {
	GameTables next = mVanilla;
	if (!ApplyTable(pack.plants, next.plants, "plants", SetPlant, err) ||
		!ApplyTable(pack.zombies, next.zombies, "zombies", SetZombie, err) ||
		!ApplyTable(pack.projectiles, next.projectiles, "projectiles", SetProjectile, err)) {
		ResetToVanilla();
		return false;
	}
	mActive = std::move(next);
	mFeatures = pack.features;
	mPackId = pack.id;
	return true;
}

const PlantDefData& GameData::Plant(std::string_view name) const { return Find(mActive.plants, name); }
const ZombieDefData& GameData::Zombie(std::string_view name) const { return Find(mActive.zombies, name); }
const ProjectileDefData& GameData::Projectile(std::string_view name) const { return Find(mActive.projectiles, name); }

bool DumpActiveTables(const std::string& path) {
	ModPack p = PackFromTables(GameData::Get().Active(), "dump");
	json out = {{"plants", p.plants}, {"zombies", p.zombies}, {"projectiles", p.projectiles}};
	std::error_code ec;
	std::filesystem::path file(path);
	if (file.has_parent_path())
		std::filesystem::create_directories(file.parent_path(), ec);
	std::ofstream f(file, std::ios::binary);
	if (!f)
		return false;
	f << out.dump(1, '\t') << '\n';
	return static_cast<bool>(f);
}

bool GameData::Feature(std::string_view key) const {
	auto it = mFeatures.find(key);
	return it != mFeatures.end() && it->second;
}

} // namespace pvzu
