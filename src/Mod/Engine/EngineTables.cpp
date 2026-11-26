#include "Mod/Engine/EngineTables.h"
#include "Mod/GameData.h"

#include "Lawn/Plant.h"
#include "Lawn/Projectile.h"
#include "Lawn/Zombie.h"

#include <array>
#include <utility>

namespace pvzu::engine {

namespace {

// Pack keys = engine enum names without prefix (display names repeat: 7 zombies are "ZOMBIE").
constexpr const char* kPlantNames[SeedType::NUM_SEED_TYPES] = {
	"PEASHOOTER", "SUNFLOWER", "CHERRYBOMB", "WALLNUT", "POTATOMINE", "SNOWPEA",
	"CHOMPER", "REPEATER", "PUFFSHROOM", "SUNSHROOM", "FUMESHROOM", "GRAVEBUSTER",
	"HYPNOSHROOM", "SCAREDYSHROOM", "ICESHROOM", "DOOMSHROOM", "LILYPAD", "SQUASH",
	"THREEPEATER", "TANGLEKELP", "JALAPENO", "SPIKEWEED", "TORCHWOOD", "TALLNUT",
	"SEASHROOM", "PLANTERN", "CACTUS", "BLOVER", "SPLITPEA", "STARFRUIT",
	"PUMPKINSHELL", "MAGNETSHROOM", "CABBAGEPULT", "FLOWERPOT", "KERNELPULT", "INSTANT_COFFEE",
	"GARLIC", "UMBRELLA", "MARIGOLD", "MELONPULT", "GATLINGPEA", "TWINSUNFLOWER",
	"GLOOMSHROOM", "CATTAIL", "WINTERMELON", "GOLD_MAGNET", "SPIKEROCK", "COBCANNON",
	"IMITATER", "EXPLODE_O_NUT", "GIANT_WALLNUT", "SPROUT", "LEFTPEATER",
};
constexpr const char* kZombieNames[NUM_ZOMBIE_TYPES] = {
	"NORMAL", "FLAG", "TRAFFIC_CONE", "POLEVAULTER", "PAIL", "NEWSPAPER",
	"DOOR", "FOOTBALL", "DANCER", "BACKUP_DANCER", "DUCKY_TUBE", "SNORKEL",
	"ZAMBONI", "BOBSLED", "DOLPHIN_RIDER", "JACK_IN_THE_BOX", "BALLOON", "DIGGER",
	"POGO", "YETI", "BUNGEE", "LADDER", "CATAPULT", "GARGANTUAR",
	"IMP", "BOSS", "PEA_HEAD", "WALLNUT_HEAD", "JALAPENO_HEAD", "GATLING_HEAD",
	"SQUASH_HEAD", "TALLNUT_HEAD", "REDEYE_GARGANTUAR",
};
constexpr const char* kProjectileNames[NUM_PROJECTILES] = {
	"PEA", "SNOWPEA", "CABBAGE", "MELON", "PUFF", "WINTERMELON", "FIREBALL", "STAR",
	"SPIKE", "BASKETBALL", "KERNEL", "COBBIG", "BUTTER", "ZOMBIE_PEA",
};

struct Tables {
	std::array<PlantDefinition, SeedType::NUM_SEED_TYPES> plants;
	std::array<ZombieDefinition, NUM_ZOMBIE_TYPES> zombies;
	std::array<ProjectileDefinition, NUM_PROJECTILES> projectiles;
	bool registered = false;
};

Tables& T() {
	static Tables t = [] {
		Tables init;
		for (int i = 0; i < SeedType::NUM_SEED_TYPES; ++i) init.plants[i] = gPlantDefs[i];
		for (int i = 0; i < NUM_ZOMBIE_TYPES; ++i) init.zombies[i] = gZombieDefs[i];
		for (int i = 0; i < NUM_PROJECTILES; ++i) init.projectiles[i] = gProjectileDefinition[i];
		return init;
	}();
	return t;
}

} // namespace

void RegisterVanillaTables() {
	Tables& t = T();
	if (t.registered)
		return;
	GameTables v;
	for (int i = 0; i < SeedType::NUM_SEED_TYPES; ++i) {
		const PlantDefinition& d = gPlantDefs[i];
		v.plants.push_back({kPlantNames[i], d.mSeedCost, d.mRefreshTime, d.mLaunchRate});
	}
	for (int i = 0; i < NUM_ZOMBIE_TYPES; ++i) {
		const ZombieDefinition& d = gZombieDefs[i];
		v.zombies.push_back({kZombieNames[i], d.mZombieValue, d.mStartingLevel, d.mFirstAllowedWave, d.mPickWeight});
	}
	for (int i = 0; i < NUM_PROJECTILES; ++i)
		v.projectiles.push_back({kProjectileNames[i], gProjectileDefinition[i].mDamage});
	const bool aUnique = GameData::Get().SetVanilla(std::move(v));
	(void)aUnique; // names are generated from enums and unique by construction (checked by tests)
	t.registered = true;
	SyncActiveTables();
}

void SyncActiveTables() {
	Tables& t = T();
	const GameTables& a = GameData::Get().Active();
	if (a.plants.size() != t.plants.size() || a.zombies.size() != t.zombies.size() || a.projectiles.size() != t.projectiles.size())
		return; // not registered yet: engine keeps vanilla copies
	for (size_t i = 0; i < t.plants.size(); ++i) {
		t.plants[i].mSeedCost = a.plants[i].seedCost;
		t.plants[i].mRefreshTime = a.plants[i].refreshTime;
		t.plants[i].mLaunchRate = a.plants[i].launchRate;
	}
	for (size_t i = 0; i < t.zombies.size(); ++i) {
		t.zombies[i].mZombieValue = a.zombies[i].zombieValue;
		t.zombies[i].mStartingLevel = a.zombies[i].startingLevel;
		t.zombies[i].mFirstAllowedWave = a.zombies[i].firstAllowedWave;
		t.zombies[i].mPickWeight = a.zombies[i].pickWeight;
	}
	for (size_t i = 0; i < t.projectiles.size(); ++i)
		t.projectiles[i].mDamage = a.projectiles[i].damage;
}

const PlantDefinition& ActivePlant(SeedType s) { return T().plants[s]; }
const ZombieDefinition& ActiveZombie(ZombieType z) { return T().zombies[z]; }
const ProjectileDefinition& ActiveProjectile(ProjectileType p) { return T().projectiles[p]; }

} // namespace pvzu::engine
