// PVZU: what Unnamed 1.3beta2 changes in the vanilla mini-games, puzzles and survival modes, as pure rules.
// Evidence: the research notes (modes-ui-shop) sections 2.1, 2.2, 9, Final table A, and the exe reads
// listed per key in pvz-unnamed-pack/SOURCES-modes-vanilla.md (several study readings are corrected there).
// Every constant comes from the pack's data/rules-modes-vanilla.json (keys "modesVanilla.*"); a missing key plays that
// rule like vanilla. The engine guards each call with GameData::Get().Feature("modesVanilla") (Mod/Engine/ModesVanilla.h).
// The 48 mod-only modes (0x65-0x94, study 2.3) are not here.
#pragma once

#include <string>
#include <utility>
#include <vector>

namespace pvzu::unnamed {

// ---- Challenge screen table (study 2.1; in-place .data edit of gChallengeDefs at 0x6a2ba0) ----
struct ChallengeSlot {
	int page = 0, row = 0, col = 0, icon = 0;
	bool operator==(const ChallengeSlot&) const = default;
};
// modesVanilla.challenge.<game mode> = [page, row, col, icon]. false: the vanilla entry stands.
bool ChallengeSlotFor(int theGameMode, ChallengeSlot& theSlot);

// ---- Zombie allow lists (Challenge::InitZombieWaves; hooks 177, 178, 179, 180) ----
// modesVanilla.allow.<game mode>.add / .remove: pack zombie keys (engine names without "ZOMBIE_"). Applied after the
// vanilla list (and after the Yeti line), so the result equals the exe's. Unknown names are skipped and named in *err.
struct AllowEdit {
	std::vector<int> add;
	std::vector<int> remove;
	bool Empty() const { return add.empty() && remove.empty(); }
};
AllowEdit AllowListEdit(int theGameMode, const std::vector<std::string>& theZombieNames, std::string* err);
void ApplyAllowEdit(const AllowEdit& theEdit, bool* theAllowed, int theCount);

// ---- Art challenges (Challenge::CanPlantAt; hook 175) ----
// A seed that ignores the art picture like vanilla's Lily Pad and Pumpkin (Unnamed adds the Grave Buster).
bool ArtIgnoresPicture(int theSeed);

// ---- Grave Danger (Challenge::SpawnZombieWave hook 184; seed filter 0x7b0f00 chain) ----
int GraveDangerGravesPerWave(int theVanilla);            // graves per non-flag wave after wave 5 (Unnamed 5)
// The seed chooser's "allowed to pick" filter (vanilla: Last Stand's sun plants). Unnamed: Grave Danger bans the Grave
// Buster. Only the vanilla modes' extra rows are here; the mod modes' rows belong to the mod-mode work.
bool SeedBannedInMode(int theGameMode, int theSeed);

// ---- Wall-nut Bowling (Plant::UpdateBowling; hooks 304, 305) ----
enum class RicochetKind { Vanilla, Zombie, SeedPacket };
struct Ricochet {
	RicochetKind kind = RicochetKind::Vanilla;
	int value = -1; // Zombie: zombie type (hypnotised, at the nut); SeedPacket: seed type of a usable seed packet coin
	bool operator==(const Ricochet&) const = default;
};
// theCount: the Wall-nut's ricochet count after this hit (Plant::mLaunchCounter). Vanilla: Vanilla for every count.
Ricochet BowlingRicochet(int theCount);

// ---- Vasebreaker (Challenge::ScaryPotterOpenPot; hook 188) ----
// Sun added straight to the bank (and no sun drops) when a sun pot holds this many; 0 = vanilla drops.
int ScaryPotDirectSun(int theSunCount);

// ---- I, Zombie (Board::MouseDown hooks 78, 79; Plant::GetCost hook 363 + the patched jump table 0x467bac;
//      IZombieSeedTypeToZombieType hook 190; IZombiePlaceZombie hook 193) ----
// The exe's I, Zombie card -> zombie type table (0x8f01a8, index seed - SEED_ZOMBIE_NORMAL); -1 outside the cards.
int IZombieCardZombie(int theSeed);
// The card that makes a zombie type (inverse, table 0x8f01b7); -1 when no card makes it.
int IZombieZombieCard(int theZombieType);
// Card price before any combination (Unnamed: Pole Vaulter 125, from the patched jump table). theVanilla = engine cost.
int IZombieCardCost(int theSeed, int theVanilla);
// Football card toggled to Newspaper (right click): the zombie it places.
int IZombieFootballZombie(bool theToggled, int theVanilla);
// Dancer and Bungee cards take a companion: the card held while one of them is clicked. -1: that held card is not taken.
// Dancer: Cone, Pole, Pail, Football, Dancer, Gargantuar (its backup dancers become that zombie). Bungee: any zombie
// card but the Bungee (it drops that zombie).
int IZombieCompanionFor(int theCarrierSeed, int theHeldSeed);
// Price of a carrier card with that companion zombie (-1 = none): base + the companion card's price.
int IZombieCarrierCost(int theCarrierSeed, int theCompanionZombie, int theBaseCost, int theCompanionCardCost);
bool IZombieIsCarrier(int theSeed);       // Dancer or Bungee card, when the rules give it a companion list
bool IZombieRightClickToggles(int theSeed); // the Football card's Newspaper toggle

// ---- I, Zombie Endless (Plant::StarFruitFire; hook 287) ----
// 0x95b790: mode 0x46 jumps to 0x95b0e7 with EAX = App != 0, so the jne skips the four extra stars: never, no roll drawn
bool StarfruitNoExtraStars(int theGameMode);

// ---- Zen Garden (ZenGarden::MouseDownWithTool; hook 478) ----
bool GloveCanPickUp(int theSeed, int thePlantState); // Unnamed: not a Squash that is not idle

// ---- V2 (C23): mini-game changes patched in place in .text (pvz-unnamed-pack/SOURCES-modes-vanilla.md) ----
// Board::PickZombieWaves: modesVanilla.numWaves.<game mode>; theVanilla when no key.
int ModeNumWaves(int theGameMode, int theVanilla);
// Challenge::UpdateConveyorBelt: modesVanilla.conveyor.<game mode> = [[seed, weight], ...]. false: vanilla picks.
bool ConveyorPicks(int theGameMode, std::vector<std::pair<int, int>>& thePicks);
// Whack a Zombie (the mini-game; adventure 2-5 keeps its bank): the packets in order (empty: vanilla bank).
std::vector<int> WhackBank();
// Challenge::WhackAZombieSpawning (both the mini-game and 2-5, the exe has no mode test there): zombies per spawn
// before the double/triple rolls, and the grave count it fills up to at countdown 100.
int WhackGroupSize(int theVanilla);
int WhackGraveTarget(int theVanilla);

} // namespace pvzu::unnamed
