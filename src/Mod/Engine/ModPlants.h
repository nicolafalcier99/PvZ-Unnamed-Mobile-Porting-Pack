// PVZU: engine side of Unnamed's 26 mod-only plants (Plan 8 part 1: obtaining, chooser, packets, planting, saves).
// Rules: Mod/Unnamed/ModPlants.h; evidence: the research notes (w6-w7-and-new-content) 1.3, 5, 7.2, 7.3 and
// pvz-unnamed-pack/SOURCES-mod-plants.md. Everything is inert unless the active pack has the feature "modPlants" and a
// data/mod-plants.json; the chooser and HasSeedType parts also need CARD (setting "card" in pvzu-settings.json, or
// the dev flag -pvzu-card) and an Imitater bought, as in the exe. CARD is also a row of the Unnamed options panel
// (Mod/Engine/OptionsPanel.h); a change counts from the next seed chooser (Mod/Unnamed/Options.h CardLatch).
//
// How a mod plant lives in the engine:
//   * seed packets, the cursor and the chooser carry the mod seed (0x50..0x7f), like the exe;
//   * a planted mod plant is an ordinary engine Plant of its BASE seed (seed - 0x50) with Plant::mPvzu.modSeed set
//     (saved in the plant's PVZU TLV field). So it behaves as the base plant until Plan 8 part 2 fills the hooks in
//     Mod/Engine/ModPlantSites.h, which dispatch on ModSeedOf(plant).
//   * GetPlantDefinition / Plant::GetCost / GetRefreshTime / GetNameString / GetToolTip answer from the mod table
//     (exe hooks 364, 365, 367, 368); drawing uses the base art times the table's tint (hook 313, 0x7a3d0e) on a
//     cell-12 packet (hook 465).
//
// Dev options (Mac checks):
//   -pvzu-card                  CARD on for this run (as the setting "card": true)
//   -pvzu-dev-imitater          with -pvzu-level: the profile owns the Imitater (the second CARD condition)
//   -pvzu-dev-mod-plants <tick> at that tick of play, plant every mod plant through Board::AddPlant on free squares
//                               (and log each one); -pvzu-log-plants <file> gets one line per planting
//   -pvzu-dev-list-plants <tick> at that tick of play, log every plant on the lawn with its mod seed (save checks)
//   -pvzu-dev-mod-bank <n>      at the first tick of play, fill the seed bank with the mod plants from the n-th on
#pragma once

#include "ConstEnums.h"

#include <optional>
#include <string>

class Board;
class LawnApp;
class Plant;
class PlantDefinition;
class SeedChooserScreen;
namespace Sexy {
class Graphics;
class Image;
class Color;
}
namespace pvzu {
struct ModPack;
}

namespace pvzu::engine {

// Called from ApplyStartupMode after the pack is applied: loads its data/mod-plants.json when the feature is on;
// nullptr (vanilla, or no pack) clears the table.
void SetUpModPlants(const ModPack* thePack);
bool ModPlantsOn();      // feature "modPlants" and a table
bool CardOn();           // ModPlantsOn() and CARD in effect (setting as of the last chooser, or -pvzu-card)

// The options panel's CARD row: the saved setting (or -pvzu-card), whether a change waits for the next seed chooser,
// whether the profile owns the Imitater (the exe's second condition, hook 256), and a tap (theSave false: a dev run).
bool CardSetting();
bool CardPending();
bool ImitaterOwned(LawnApp* theApp);
void SetCard(LawnApp* theApp, bool theOn, bool theSave);
// SeedChooserScreen's constructor, before it lays out its seeds: a CARD change made since counts from here.
void ModChooserOpens();

// A mod seed with a row in the table (only when ModPlantsOn()).
bool IsModSeed(SeedType theSeedType);
// The seed a planted plant came from: its mod seed, or its own mSeedType.
SeedType ModSeedOf(const Plant* thePlant);

// GetPlantDefinition for seeds 0x50..0x7f: the mod row (cost, recharge, name; reanim of the base), or the base row for
// an empty slot. nullptr for any other seed (vanilla path).
const PlantDefinition* ModPlantDefinition(SeedType theSeedType);
// Plant::GetNameString / GetToolTip of a mod seed: the literal table text in the game's language. nullopt otherwise.
std::optional<std::string> ModPlantNameString(SeedType theSeedType);
std::optional<std::string> ModPlantToolTip(SeedType theSeedType);

// LawnApp::HasSeedType (hook 256). nullopt = vanilla answer.
std::optional<bool> ModHasSeedType(LawnApp* theApp, SeedType theSeedType);

// Board::GetSeedTypeInCursor for a mod seed in the cursor: the seed whose placement rules apply. The base plant,
// except bases that vanilla only plants as an upgrade (Gatling Pea, Gloom-shroom, Cattail, Cob Cannon): the exe's
// CanPlantAt has no upgrade test for 0x78/0x7a/0x7b/0x7f, so they go on a free square like a Peashooter.
SeedType ModPlantingSeed(SeedType theCursorSeed);
// Board::MouseDownWithPlant pays for the packet in the cursor (the mod cost), not for its planting seed.
SeedType ModCostSeed(Board* theBoard, SeedType thePlantingSeed);

// Board::AddPlant: theSeedType becomes the base seed; after the plant is made, ModPlantPlanted tags it.
SeedType ModAddPlantBase(SeedType theSeedType);
void ModPlantPlanted(Plant* thePlant, SeedType theAskedSeed);
// Board::MouseDownWithPlant before AddPlant from the seed bank / a dropped packet (hooks 67 / 66): a Sprout (or an
// Imitater-Sprout; from a packet any Imitater) becomes an Imitater copying a random plant.
void ModSproutToImitater(SeedType& theSeedType, SeedType& theImitaterType, bool theFromSeedBank);
// Plant::ImitaterMorph: an Imitater copying an Imitater (the coin's Rand(53) = 48, as in the exe) re-picks once without
// 48 (Mod/Unnamed/ModPlants.h ImitaterOfImitaterCopy); anything else is returned unchanged. Inert unless ModPlantsOn().
SeedType ModImitaterOfImitater(SeedType theCopy);

// Plant::UpdateReanimColor's plain white: the mod plant's tint (hook 313).
Sexy::Color ModPlantColor(const Plant* thePlant, const Sexy::Color& theVanilla);
// Plant::DrawSeedType / SeedPacketDrawSeed: true = drew the mod seed (base art, tinted) and the caller returns.
bool DrawModSeedType(Sexy::Graphics* g, SeedType theSeedType, int theDrawVariation, float x, float y);
// SeedPacketDrawSeed: for a mod seed, multiplies g's colour by its tint and returns true (the caller then draws the
// base seed with g).
bool ApplyModTint(Sexy::Graphics* g, SeedType theSeedType);
// The cell-12 packet back of a mod seed (from the pack's own seeds.png), or nullptr (vanilla cell).
Sexy::Image* ModPacketImage(SeedType theSeedType);

// Seed chooser with CARD: loops run to ChooserSeedEnd(); seeds 49..52 and the mod plants sit in a second panel right
// of the vanilla one (DrawModChooserPanel draws its back and the empty-slot shadows).
int ChooserSeedEnd();
bool ModChooserPosition(SeedChooserScreen* theChooser, int theSeed, int& x, int& y);
void DrawModChooserPanel(SeedChooserScreen* theChooser, Sexy::Graphics* g);

// -pvzu-dev-imitater on the dev profile (DevStartLevel); per-tick dev work (DevBoardTick).
void ModPlantsDevProfile(LawnApp* theApp);
void ModPlantsDevTick(Board* theBoard, int thePlayTicks);

} // namespace pvzu::engine
