// PVZU: Unnamed 1.3beta2's 26 mod-only plants (seeds 0x50..0x7f, the exe's table at 0x860000), infrastructure only
// (Plan 8 part 1). Evidence: the research notes (w6-w7-and-new-content) sections 1.3, 5, 5.1, 7.2, 7.3 and
// pvz-unnamed-pack/SOURCES-mod-plants.md. Pure functions: the table comes from the pack's data/mod-plants.json
// (SetModPlants), the constants from data/rules-mod-plants.json ("modPlants.*"); with neither (vanilla) nothing here
// changes the game. The engine side is Mod/Engine/ModPlants.h (guarded by GameData::Get().Feature("modPlants")).
//
// A mod plant reuses its base plant: the vanilla seed at the same index (seed - 0x50; the table's reanim and packet
// rows are the base's). Until Plan 8 part 2 it also behaves as the base plant; its own behaviour goes through the
// hooks in Mod/Engine/ModPlantSites.h.
#pragma once

#include "nlohmann/json.hpp"

#include <array>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace pvzu::unnamed {

inline constexpr int kFirstModSeed = 0x50;     // 80: first mod seed (table row 0)
inline constexpr int kModSeedEnd = 0x80;       // 128: one past the last mod seed (the exe's chooser stops at 0x7f)
inline constexpr int kAllSeedTypes = kModSeedEnd; // size of a seed-indexed array that also holds mod seeds
inline constexpr int kVanillaChooserSeeds = 49; // engine NUM_SEEDS_IN_CHOOSER (Peashooter..Imitater)
inline constexpr int kImitaterSeed = 48, kSproutSeed = 51, kCardFirstExtraSeed = 49, kCardLastExtraSeed = 52;

struct ModPlant {
	int seed = 0;               // 0x50..0x7f
	int base = 0;               // vanilla SeedType it reuses = seed - 0x50
	std::string nameEn, nameZh; // literal names (hook 365 prints them with "%s", not as [KEY])
	std::string tooltipEn, tooltipZh;
	int seedCost = 0;           // table +0x10 (hook 364)
	int refreshTime = 0;        // table +0x14, cs (hook 368)
	int launchRate = 0;         // table +0x1c (1051 PlantDefinition order); not applied in part 1 (base behaviour)
	bool shooter = false;       // table +0x18 subclass; not applied in part 1
	std::array<int, 4> tint{255, 255, 255, 255}; // table +0x24..+0x30 RGBA: the plant's colour override (0x7a3d0e)
};

// The seed range of mod plants (0x50..0x7f), whether or not the table has a row for it.
bool IsModSeedRange(int theSeed);
// The base plant of any seed: seed - 0x50 for the mod range, else the seed itself.
int ModBaseSeed(int theSeed);

// Replaces the table with data/mod-plants.json (an object keyed by decimal seed). theEngineNames = engine plant names
// by SeedType ("PEASHOOTER"...), used to check each row's "base". On any bad row: false, *err says which, no rows kept.
bool SetModPlants(const nlohmann::json& theTable, const std::vector<std::string>& theEngineNames, std::string* err);
void ClearModPlants();
const ModPlant* FindModPlant(int theSeed);  // nullptr: not a mod seed, an empty slot (0x54...), or no table
const std::vector<int>& ModPlantSeeds();    // seeds with a row, ascending

// Name and tooltip in the game's language ("zh" -> Chinese, anything else English).
const std::string& ModPlantName(const ModPlant& thePlant, std::string_view theLanguage);
const std::string& ModPlantToolTip(const ModPlant& thePlant, std::string_view theLanguage);

// LawnApp::HasSeedType, hook 256 (0x7a055c). theCard = the "CARD" toggle (exe flag 0x8001a0; on the phone a setting).
// nullopt = the vanilla answer stands. With CARD on, seeds 49..52 and every mod plant with a row are available when
// the Imitater is bought (PlayerInfo+0x1e0); the exe also answers yes for 53..79 and the empty mod slots, which its
// chooser never lists (it jumps 0x35 -> 0x50): here they are never available. With CARD off a mod seed is never
// available (vanilla: seed < GetSeedsAvailable() <= 49).
std::optional<bool> CardHasSeedType(int theSeed, bool theCard, bool theImitaterBought);

// Seed chooser (hooks 148, 423, 424, 431). The vanilla loops run to kVanillaChooserSeeds; with CARD they run to
// kModSeedEnd (the 53..79 gap has no seeds: CardHasSeedType says no).
int ChooserSeedEnd(bool theCard);
// The extra seeds CARD lists, in the exe's order: 49..52, then the mod plants with a row. Slot in that list, or -1.
int CardChooserSlot(int theSeed);
int CardChooserSlotCount();
// Packet position of a slot in the extra panel: 8 columns 53 px apart and the vanilla chooser's row pitch (70 px with
// 7 rows, else 73), relative to the panel's left edge (the port's panel sits right of the vanilla one; the exe stacks
// the rows under it, 51 x 71 px, in its own 1067-wide layout).
struct ChooserSpot { int x = 0; int y = 0; };
ChooserSpot CardChooserSpot(int theSlot, bool theHas7Rows);
inline constexpr int kCardChooserColumns = 8;

// Planting a Sprout or an Imitater-Sprout (hooks 66, 67). Returns the Rand() range of the plant the Imitater will copy,
// or 0 to plant as usual. From the seed bank (hook 67, 0x7a071a): Sprout, or an Imitater copying a Sprout ->
// modPlants.sproutBankRoll (48: Peashooter..Cob Cannon). From a dropped seed packet (hook 66, 0x957500): Sprout or
// any Imitater -> modPlants.sproutCoinRoll (53: every seed 0..52). The planted seed becomes the Imitater either way.
int SproutImitaterRoll(int theCursorSeed, int theCursorImitater, bool theFromSeedBank);
// Plant::ImitaterMorph: the coin's Rand(53) can pick 48, an Imitater copying an Imitater (the exe does the same,
// 0x95750b, and its morph then plants the same Imitater again for ever). Such a copy re-picks once from the coin range
// without 48 (rand(range - 1), then skips 48); any other copy, or no rule (vanilla), is returned unchanged.
int ImitaterOfImitaterCopy(int theCopy, const std::function<int(int)>& theRand);

} // namespace pvzu::unnamed
