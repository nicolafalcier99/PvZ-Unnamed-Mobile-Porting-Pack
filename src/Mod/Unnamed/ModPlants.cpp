#include "Mod/Unnamed/ModPlants.h"
#include "Mod/Unnamed/Rules.h"

#include <algorithm>
#include <map>

namespace pvzu::unnamed {

namespace {
std::map<int, ModPlant> gPlants;
std::vector<int> gSeeds;

bool ReadInt(const nlohmann::json& theRow, const char* theKey, int* out) {
	auto it = theRow.find(theKey);
	if (it == theRow.end() || !it->is_number_integer())
		return false;
	*out = it->get<int>();
	return true;
}

bool ReadText(const nlohmann::json& theRow, const char* theKey, std::string* theEn, std::string* theZh) {
	auto it = theRow.find(theKey);
	if (it == theRow.end() || !it->is_object())
		return false;
	auto en = it->find("en"), zh = it->find("zh");
	if (en == it->end() || zh == it->end() || !en->is_string() || !zh->is_string())
		return false;
	*theEn = en->get<std::string>();
	*theZh = zh->get<std::string>();
	return true;
}

bool ReadRow(const std::string& theKey, const nlohmann::json& theRow, const std::vector<std::string>& theEngineNames,
	ModPlant* out, std::string* err) {
	auto fail = [&](const std::string& theWhy) {
		*err = "mod-plants.json: plant \"" + theKey + "\": " + theWhy;
		return false;
	};
	int aSeed = -1;
	try {
		size_t aUsed = 0;
		aSeed = std::stoi(theKey, &aUsed);
		if (aUsed != theKey.size())
			aSeed = -1;
	} catch (...) {
		aSeed = -1;
	}
	if (!IsModSeedRange(aSeed))
		return fail("the key must be a seed number 80..127");
	if (!theRow.is_object())
		return fail("must be an object");
	out->seed = aSeed;
	out->base = ModBaseSeed(aSeed);
	auto aBase = theRow.find("base");
	if (aBase == theRow.end() || !aBase->is_string() || out->base >= static_cast<int>(theEngineNames.size()) ||
		aBase->get<std::string>() != theEngineNames[out->base])
		return fail("\"base\" must be the plant at seed - 80 (" +
			(out->base < static_cast<int>(theEngineNames.size()) ? theEngineNames[out->base] : std::string("?")) + ")");
	if (!ReadText(theRow, "name", &out->nameEn, &out->nameZh) || out->nameEn.empty())
		return fail("needs \"name\": {\"en\": ..., \"zh\": ...}");
	if (!ReadText(theRow, "tooltip", &out->tooltipEn, &out->tooltipZh))
		return fail("needs \"tooltip\": {\"en\": ..., \"zh\": ...}");
	if (!ReadInt(theRow, "seedCost", &out->seedCost) || !ReadInt(theRow, "refreshTime", &out->refreshTime) ||
		!ReadInt(theRow, "launchRate", &out->launchRate))
		return fail("needs whole numbers seedCost, refreshTime, launchRate");
	auto aSub = theRow.find("subclass");
	if (aSub == theRow.end() || !aSub->is_string() || (*aSub != "shooter" && *aSub != "normal"))
		return fail("\"subclass\" must be \"shooter\" or \"normal\"");
	out->shooter = *aSub == "shooter";
	auto aTint = theRow.find("tint");
	if (aTint == theRow.end() || !aTint->is_array() || aTint->size() != 4)
		return fail("\"tint\" must be [r, g, b, a]");
	for (size_t i = 0; i < 4; ++i) {
		if (!(*aTint)[i].is_number_integer() || (*aTint)[i].get<int>() < 0 || (*aTint)[i].get<int>() > 255)
			return fail("\"tint\" values must be 0..255");
		out->tint[i] = (*aTint)[i].get<int>();
	}
	return true;
}
} // namespace

bool IsModSeedRange(int theSeed) { return theSeed >= kFirstModSeed && theSeed < kModSeedEnd; }

int ModBaseSeed(int theSeed) { return IsModSeedRange(theSeed) ? theSeed - kFirstModSeed : theSeed; }

bool SetModPlants(const nlohmann::json& theTable, const std::vector<std::string>& theEngineNames, std::string* err) {
	ClearModPlants();
	if (!theTable.is_object()) {
		*err = "mod-plants.json: top level must be an object keyed by seed";
		return false;
	}
	std::map<int, ModPlant> aPlants;
	for (auto& [k, v] : theTable.items()) {
		ModPlant aPlant;
		if (!ReadRow(k, v, theEngineNames, &aPlant, err))
			return false;
		aPlants[aPlant.seed] = std::move(aPlant);
	}
	gPlants = std::move(aPlants);
	for (const auto& [s, p] : gPlants)
		gSeeds.push_back(s);
	return true;
}

void ClearModPlants() {
	gPlants.clear();
	gSeeds.clear();
}

const ModPlant* FindModPlant(int theSeed) {
	auto it = gPlants.find(theSeed);
	return it == gPlants.end() ? nullptr : &it->second;
}

const std::vector<int>& ModPlantSeeds() { return gSeeds; }

const std::string& ModPlantName(const ModPlant& thePlant, std::string_view theLanguage) {
	return theLanguage == "zh" ? thePlant.nameZh : thePlant.nameEn;
}

const std::string& ModPlantToolTip(const ModPlant& thePlant, std::string_view theLanguage) {
	return theLanguage == "zh" ? thePlant.tooltipZh : thePlant.tooltipEn;
}

std::optional<bool> CardHasSeedType(int theSeed, bool theCard, bool theImitaterBought) {
	if (theSeed <= kImitaterSeed)
		return std::nullopt; // the hook's own 0x30 branch is the vanilla Imitater test
	if (theSeed <= kCardLastExtraSeed)
		return theCard ? std::optional<bool>(theImitaterBought) : std::nullopt;
	if (!theCard)
		return false;
	return FindModPlant(theSeed) != nullptr && theImitaterBought;
}

int ChooserSeedEnd(bool theCard) { return theCard ? kModSeedEnd : kVanillaChooserSeeds; }

int CardChooserSlot(int theSeed) {
	if (theSeed >= kCardFirstExtraSeed && theSeed <= kCardLastExtraSeed)
		return theSeed - kCardFirstExtraSeed;
	auto it = std::find(gSeeds.begin(), gSeeds.end(), theSeed);
	return it == gSeeds.end() ? -1 : (kCardLastExtraSeed - kCardFirstExtraSeed + 1) + static_cast<int>(it - gSeeds.begin());
}

int CardChooserSlotCount() { return (kCardLastExtraSeed - kCardFirstExtraSeed + 1) + static_cast<int>(gSeeds.size()); }

ChooserSpot CardChooserSpot(int theSlot, bool theHas7Rows) {
	const int aRow = theSlot / kCardChooserColumns, aCol = theSlot % kCardChooserColumns;
	// The vanilla SeedChooserScreen::GetSeedPositionInChooser numbers.
	return {aCol * 53 + 22, theHas7Rows ? aRow * 70 + 123 : aRow * 73 + 128};
}

int SproutImitaterRoll(int theCursorSeed, int theCursorImitater, bool theFromSeedBank) {
	if (theFromSeedBank) {
		if (theCursorSeed == kSproutSeed || (theCursorSeed == kImitaterSeed && theCursorImitater == kSproutSeed))
			return std::max(0, RuleInt("modPlants.sproutBankRoll", 0));
		return 0;
	}
	if (theCursorSeed == kSproutSeed || theCursorSeed == kImitaterSeed)
		return std::max(0, RuleInt("modPlants.sproutCoinRoll", 0));
	return 0;
}

int ImitaterOfImitaterCopy(int theCopy, const std::function<int(int)>& theRand) {
	const int aRange = RuleInt("modPlants.sproutCoinRoll", 0);
	if (theCopy != kImitaterSeed || aRange <= kImitaterSeed + 1)
		return theCopy;
	const int aPick = theRand(aRange - 1);
	return aPick >= kImitaterSeed ? aPick + 1 : aPick;
}

} // namespace pvzu::unnamed
