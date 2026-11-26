// PVZU: runtime game tables. Engine-independent: the engine registers its vanilla tables at start-up
// (src/Mod/Engine/EngineTables.cpp) and reads the active values back. A mod pack overrides single fields.
#pragma once

#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace pvzu {

struct PlantDefData {
	std::string name;     // engine name, e.g. "POTATO_MINE"
	int seedCost = 0;
	int refreshTime = 0;  // centiseconds
	int launchRate = 0;
	bool operator==(const PlantDefData&) const = default;
};

struct ZombieDefData {
	std::string name;     // e.g. "BUCKETHEAD_ZOMBIE"
	int zombieValue = 0;
	int startingLevel = 0;
	int firstAllowedWave = 0;
	int pickWeight = 0;
	bool operator==(const ZombieDefData&) const = default;
};

struct ProjectileDefData {
	std::string name;     // e.g. "PEA"
	int damage = 0;
	bool operator==(const ProjectileDefData&) const = default;
};

struct GameTables {
	std::vector<PlantDefData> plants;          // index = engine SeedType
	std::vector<ZombieDefData> zombies;        // index = engine ZombieType
	std::vector<ProjectileDefData> projectiles; // index = engine ProjectileType
	bool operator==(const GameTables&) const = default;
};

struct ModPack;

class GameData {
public:
	static GameData& Get();

	// Sets vanilla and resets Active to it. Returns false (and keeps the old tables) if any table has
	// a repeated name, because packs address rows by name.
	bool SetVanilla(GameTables vanilla);
	void ResetToVanilla();
	// Applies a pack on top of vanilla. On any error: returns false, fills *err, stays vanilla.
	bool ApplyPack(const ModPack& pack, std::string* err);

	const GameTables& Vanilla() const { return mVanilla; }
	const GameTables& Active() const { return mActive; }

	// Lookup by engine name; throws std::out_of_range for unknown names (programming error).
	const PlantDefData& Plant(std::string_view name) const;
	const ZombieDefData& Zombie(std::string_view name) const;
	const ProjectileDefData& Projectile(std::string_view name) const;

	bool Feature(std::string_view key) const;          // false when no pack or key absent
	const std::string& ActivePackId() const { return mPackId; }

private:
	GameTables mVanilla;
	GameTables mActive;
	std::map<std::string, bool, std::less<>> mFeatures;
	std::string mPackId;
};

// Writes GameData::Get().Active() as {"plants":{...},"zombies":{...},"projectiles":{...}} JSON
// (same field names as a pack). Creates parent folders. Returns false on I/O error.
bool DumpActiveTables(const std::string& path);

} // namespace pvzu
