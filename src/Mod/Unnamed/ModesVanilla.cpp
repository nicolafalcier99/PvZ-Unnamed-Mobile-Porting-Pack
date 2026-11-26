// PVZU: Unnamed changes to the vanilla modes. Section numbers refer to the research notes (modes-ui-shop); exe addresses
// per key are in pvz-unnamed-pack/SOURCES-modes-vanilla.md.
#include "Mod/Unnamed/ModesVanilla.h"

#include "Mod/Unnamed/Rules.h"

#include "ConstEnums.h"

#include <algorithm>
#include <format>

namespace pvzu::unnamed {

namespace {

// A rule that only some modes have: read without the "missing" warning.
int RI(const std::string& key, int vanilla) { return HasRule(key) ? RuleInt(key, vanilla) : vanilla; }

const nlohmann::json* RA(const std::string& key) { return HasRule(key) ? RuleArray(key) : nullptr; }

bool ListHas(const std::string& key, int value) {
	const nlohmann::json* a = RA(key);
	if (!a)
		return false;
	for (const auto& v : *a)
		if (v.is_number_integer() && v.get<int>() == value)
			return true;
	return false;
}

std::vector<int> Names(const std::string& key, const std::vector<std::string>& names, std::string* err) {
	std::vector<int> out;
	const nlohmann::json* a = RA(key);
	if (!a)
		return out;
	for (const auto& v : *a) {
		const std::string n = v.is_string() ? v.get<std::string>() : v.dump();
		auto it = std::find(names.begin(), names.end(), n);
		if (it == names.end()) {
			if (err)
				*err += (err->empty() ? "unknown zombie " : ", ") + n;
			continue;
		}
		out.push_back(static_cast<int>(it - names.begin()));
	}
	return out;
}

// 0x8f01a8: I, Zombie card (SEED_ZOMBIE_NORMAL + i) -> zombie type. The same as the vanilla
// Challenge::IZombieSeedTypeToZombieType (1051 .text 0x42a03e).
constexpr int kCardZombie[] = {
	ZOMBIE_NORMAL, ZOMBIE_TRAFFIC_CONE, ZOMBIE_POLEVAULTER, ZOMBIE_PAIL, ZOMBIE_LADDER, ZOMBIE_DIGGER, ZOMBIE_BUNGEE,
	ZOMBIE_FOOTBALL, ZOMBIE_BALLOON, ZOMBIE_DOOR, ZOMBIE_ZAMBONI, ZOMBIE_POGO, ZOMBIE_DANCER, ZOMBIE_GARGANTUAR, ZOMBIE_IMP,
};
constexpr int kNumCards = static_cast<int>(sizeof(kCardZombie) / sizeof(kCardZombie[0]));

} // namespace

bool ChallengeSlotFor(int theGameMode, ChallengeSlot& theSlot) {
	const nlohmann::json* a = RA(std::format("modesVanilla.challenge.{}", theGameMode));
	if (!a || a->size() != 4)
		return false;
	for (const auto& v : *a)
		if (!v.is_number_integer())
			return false;
	ChallengeSlot s{(*a)[0].get<int>(), (*a)[1].get<int>(), (*a)[2].get<int>(), (*a)[3].get<int>()};
	// The screen draws 4 rows of 5 (ChallengeScreen ctor) on pages 0..3.
	if (s.page < CHALLENGE_PAGE_SURVIVAL || s.page >= MAX_CHALLANGE_PAGES || s.row < 0 || s.row > 3 || s.col < 0 || s.col > 4 ||
		s.icon < 0)
		return false;
	theSlot = s;
	return true;
}

AllowEdit AllowListEdit(int theGameMode, const std::vector<std::string>& theZombieNames, std::string* err) {
	const std::string k = std::format("modesVanilla.allow.{}", theGameMode);
	return {Names(k + ".add", theZombieNames, err), Names(k + ".remove", theZombieNames, err)};
}

void ApplyAllowEdit(const AllowEdit& theEdit, bool* theAllowed, int theCount) {
	for (int t : theEdit.add)
		if (t >= 0 && t < theCount)
			theAllowed[t] = true;
	for (int t : theEdit.remove)
		if (t >= 0 && t < theCount)
			theAllowed[t] = false;
}

bool ArtIgnoresPicture(int theSeed) { return ListHas("modesVanilla.art.ignorePictureSeeds", theSeed); }

int GraveDangerGravesPerWave(int theVanilla) { return std::max(0, RI("modesVanilla.graveDanger.gravesPerWave", theVanilla)); }

bool SeedBannedInMode(int theGameMode, int theSeed) {
	return ListHas(std::format("modesVanilla.bannedSeeds.{}", theGameMode), theSeed);
}

Ricochet BowlingRicochet(int theCount) {
	const int zombie = RI(std::format("modesVanilla.bowling.zombie.{}", theCount), -1);
	if (zombie >= 0)
		return {RicochetKind::Zombie, zombie};
	const int from = RI("modesVanilla.bowling.packetFrom", -1);
	const nlohmann::json* seeds = RA("modesVanilla.bowling.packetSeeds");
	if (from < 0 || theCount < from || !seeds || seeds->empty())
		return {};
	const int i = std::min(theCount - from, static_cast<int>(seeds->size()) - 1); // the exe clamps the count to 7
	const auto& v = (*seeds)[i];
	return v.is_number_integer() ? Ricochet{RicochetKind::SeedPacket, v.get<int>()} : Ricochet{};
}

int ScaryPotDirectSun(int theSunCount) {
	const int from = RI("modesVanilla.scaryPot.directSunFrom", 0);
	if (from <= 0 || theSunCount < from)
		return 0;
	return theSunCount * RI("modesVanilla.scaryPot.sunValue", 25);
}

int IZombieCardZombie(int theSeed) {
	const int i = theSeed - SEED_ZOMBIE_NORMAL;
	return i >= 0 && i < kNumCards ? kCardZombie[i] : -1;
}

int IZombieZombieCard(int theZombieType) {
	for (int i = 0; i < kNumCards; i++)
		if (kCardZombie[i] == theZombieType)
			return SEED_ZOMBIE_NORMAL + i;
	return -1;
}

int IZombieCardCost(int theSeed, int theVanilla) { return RI(std::format("modesVanilla.iz.cost.{}", theSeed), theVanilla); }

int IZombieFootballZombie(bool theToggled, int theVanilla) {
	return theToggled ? RI("modesVanilla.iz.footballToggleZombie", theVanilla) : theVanilla;
}

namespace {
const char* CompanionKey(int theCarrierSeed) {
	if (theCarrierSeed == SEED_ZOMBIE_DANCER) return "modesVanilla.iz.dancerCompanions";
	if (theCarrierSeed == SEED_ZOMBIE_BUNGEE) return "modesVanilla.iz.bungeeCompanions";
	return nullptr;
}
} // namespace

bool IZombieIsCarrier(int theSeed) {
	const char* k = CompanionKey(theSeed);
	return k && HasRule(k);
}

int IZombieCompanionFor(int theCarrierSeed, int theHeldSeed) {
	const char* k = CompanionKey(theCarrierSeed);
	if (!k || !ListHas(k, theHeldSeed))
		return -1;
	return IZombieCardZombie(theHeldSeed);
}

int IZombieCarrierCost(int theCarrierSeed, int theCompanionZombie, int theBaseCost, int theCompanionCardCost) {
	if (theCompanionZombie < 0 || !IZombieIsCarrier(theCarrierSeed))
		return theBaseCost;
	return theBaseCost + theCompanionCardCost;
}

bool IZombieRightClickToggles(int theSeed) {
	return theSeed == SEED_ZOMBIE_FOOTBALL && HasRule("modesVanilla.iz.footballToggleZombie");
}

bool StarfruitNoExtraStars(int theGameMode) {
	const int m = RI("modesVanilla.starfruit.noExtraStarsMode", -1);
	return m >= 0 && m == theGameMode;
}

bool GloveCanPickUp(int theSeed, int thePlantState) {
	if (RI("modesVanilla.zen.gloveSkipsBusySquash", 0) == 0)
		return true;
	return theSeed != SEED_SQUASH || thePlantState == 0;
}

// ---- V2 (C23) ----

int ModeNumWaves(int theGameMode, int theVanilla) {
	const int aWaves = RI(std::format("modesVanilla.numWaves.{}", theGameMode), theVanilla);
	return aWaves > 0 ? aWaves : theVanilla;
}

bool ConveyorPicks(int theGameMode, std::vector<std::pair<int, int>>& thePicks) {
	thePicks.clear();
	const nlohmann::json* a = RA(std::format("modesVanilla.conveyor.{}", theGameMode));
	if (!a)
		return false;
	for (const auto& p : *a)
		if (p.is_array() && p.size() == 2 && p[0].is_number_integer() && p[1].is_number_integer())
			thePicks.emplace_back(p[0].get<int>(), p[1].get<int>());
	return !thePicks.empty();
}

std::vector<int> WhackBank() {
	std::vector<int> out;
	if (const nlohmann::json* a = RA("modesVanilla.whack.bank"))
		for (const auto& v : *a)
			if (v.is_number_integer())
				out.push_back(v.get<int>());
	return out;
}

int WhackGroupSize(int theVanilla) { return RI("modesVanilla.whack.groupSize", theVanilla); }
int WhackGraveTarget(int theVanilla) { return RI("modesVanilla.whack.graveTarget", theVanilla); }

} // namespace pvzu::unnamed
