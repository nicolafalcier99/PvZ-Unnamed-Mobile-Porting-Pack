#include "Mod/Unnamed/ModModes.h"

#include <algorithm>
#include <cstdlib>
#include <format>
#include <map>

namespace pvzu::unnamed {

namespace {
std::map<int, ModModeDef> gModes;

bool ParseMode(const std::string& theKey, int& theMode) {
	char* aEnd = nullptr;
	const long aValue = std::strtol(theKey.c_str(), &aEnd, 0);
	if (aEnd == theKey.c_str() || *aEnd != '\0')
		return false;
	theMode = static_cast<int>(aValue);
	return true;
}

bool ParseStatus(const std::string& theName, ModModeStatus& theStatus) {
	for (ModModeStatus s : {ModModeStatus::Built, ModModeStatus::NeedsModPlants, ModModeStatus::Unknown, ModModeStatus::Placeholder})
		if (theName == ModModeStatusName(s)) {
			theStatus = s;
			return true;
		}
	return false;
}

bool Strings(const nlohmann::json& theRow, const char* theField, std::vector<std::string>& theOut) {
	theOut.clear();
	if (!theRow.contains(theField))
		return true;
	const nlohmann::json& a = theRow[theField];
	if (!a.is_array())
		return false;
	for (const auto& s : a) {
		if (!s.is_string())
			return false;
		theOut.push_back(s.get<std::string>());
	}
	return true;
}

bool Int(const nlohmann::json& theRow, const char* theField, int& theOut) {
	if (!theRow.contains(theField))
		return true;
	if (!theRow[theField].is_number_integer())
		return false;
	theOut = theRow[theField].get<int>();
	return true;
}
} // namespace

bool IsModModeValue(int theMode) { return theMode >= kFirstModMode && theMode <= kLastModMode; }
int ModModeRecordIndex(int theMode) { return theMode - 1; }

bool ModModeBeaten(int theMode, const uint32_t* theRecords, bool& theBeaten) {
	if (!IsModModeValue(theMode))
		return false;
	theBeaten = theRecords[ModModeRecordIndex(theMode)] > 0;
	return true;
}

std::vector<std::string> ModModeUserFileNames(int theProfileId) {
	std::vector<std::string> aNames{std::format("user{}-modes.json", theProfileId)};
	for (int aMode = kFirstModMode; aMode <= kLastModMode; aMode++) {
		aNames.push_back(std::format("game{}_{}.v4", theProfileId, aMode));
		aNames.push_back(std::format("game{}_{}.dat", theProfileId, aMode));
	}
	return aNames;
}

const char* ModModeStatusName(ModModeStatus theStatus) {
	switch (theStatus) {
	case ModModeStatus::Built: return "built";
	case ModModeStatus::NeedsModPlants: return "needsModPlants";
	case ModModeStatus::Unknown: return "unknown";
	case ModModeStatus::Placeholder: return "placeholder";
	}
	return "unknown";
}

int ModModeDef::Rule(std::string_view theKey, int theDefault) const {
	const auto it = rules.find(std::string(theKey));
	return it != rules.end() && it->is_number() ? it->get<int>() : theDefault;
}

bool SetModModes(const nlohmann::json& theTable, std::string* err) {
	gModes.clear();
	auto Fail = [&](const std::string& theWhy) {
		gModes.clear();
		if (err)
			*err = theWhy;
		return false;
	};
	if (!theTable.is_object())
		return Fail("modes table must be an object keyed by mode (\"0x65\")");
	std::map<int, ModModeDef> aModes;
	for (const auto& [aKey, aRow] : theTable.items()) {
		int aMode = 0;
		if (!ParseMode(aKey, aMode) || !IsModModeValue(aMode))
			return Fail(std::format("mode \"{}\" is not 0x65..0x94", aKey));
		if (!aRow.is_object())
			return Fail(std::format("mode {}: row must be an object", aKey));
		ModModeDef d;
		d.mode = aMode;
		d.key = aRow.value("key", "");
		d.name = aRow.value("name", "");
		d.nameZh = aRow.value("nameZh", "");
		d.note = aRow.value("note", "");
		if (!Int(aRow, "background", d.background) || d.background < 0 || d.background > 8)
			return Fail(std::format("mode {}: background must be 0..8", aKey));
		if (!Strings(aRow, "allowed", d.allowed) || !Strings(aRow, "bannedSeeds", d.bannedSeeds))
			return Fail(std::format("mode {}: allowed/bannedSeeds must be lists of names", aKey));
		if (!Int(aRow, "seedSlots", d.seedSlots) ||
			!Int(aRow, "firstWaveCountdown", d.firstWaveCountdown) || !Int(aRow, "waves", d.waves) ||
			!Int(aRow, "startSun", d.startSun) || !Int(aRow, "graves", d.graves) || !Int(aRow, "icon", d.icon))
			return Fail(std::format("mode {}: a number field is not an integer", aKey));
		auto Bool = [&](const char* theField, bool& theOut) {
			if (!aRow.contains(theField))
				return true;
			if (!aRow[theField].is_boolean())
				return false;
			theOut = aRow[theField].get<bool>();
			return true;
		};
		if (!Bool("normal", d.normal) || !Bool("pail", d.pail) || !Bool("chooser", d.chooser) ||
			!Bool("conveyor", d.conveyor) || !Bool("skySun", d.skySun))
			return Fail(std::format("mode {}: normal/pail/chooser/conveyor/skySun must be true or false", aKey));
		if (!Strings(aRow, "bank", d.bank))
			return Fail(std::format("mode {}: bank must be a list of seed names", aKey));
		if (aRow.contains("conveyorSeeds")) {
			const nlohmann::json& c = aRow["conveyorSeeds"];
			if (!c.is_array())
				return Fail(std::format("mode {}: conveyorSeeds must be a list of [seed, weight]", aKey));
			for (const auto& e : c) {
				if (!e.is_array() || e.size() != 2 || !e[0].is_string() || !e[1].is_number_integer())
					return Fail(std::format("mode {}: conveyorSeeds must be a list of [seed, weight]", aKey));
				d.conveyorSeeds.emplace_back(e[0].get<std::string>(), e[1].get<int>());
			}
		}
		d.nameColor = aRow.value("nameColor", "gold");
		if (d.nameColor != "gold" && d.nameColor != "red" && d.nameColor != "purple" && d.nameColor != "blue" &&
			d.nameColor != "green" && d.nameColor != "pulse")
			return Fail(std::format("mode {}: nameColor must be gold|red|purple|blue|green|pulse", aKey));
		if (!ParseStatus(aRow.value("status", "unknown"), d.status))
			return Fail(std::format("mode {}: status must be built|needsModPlants|unknown|placeholder", aKey));
		if (aRow.contains("rules")) {
			if (!aRow["rules"].is_object())
				return Fail(std::format("mode {}: rules must be an object", aKey));
			d.rules = aRow["rules"];
		}
		aModes[aMode] = std::move(d);
	}
	gModes = std::move(aModes);
	if (err)
		err->clear();
	return true;
}

void ClearModModes() { gModes.clear(); }

const ModModeDef* FindModMode(int theMode) {
	const auto it = gModes.find(theMode);
	return it == gModes.end() ? nullptr : &it->second;
}

std::vector<int> ModModeMenuOrder() {
	std::vector<int> aList;
	for (const auto& [aMode, aDef] : gModes)
		aList.push_back(aMode);
	return aList;
}

std::vector<std::string> ModModeZombies(const ModModeDef& theDef) {
	std::vector<std::string> aList;
	if (theDef.normal)
		aList.push_back("NORMAL");
	aList.insert(aList.end(), theDef.allowed.begin(), theDef.allowed.end());
	if (theDef.allowed.empty())
		aList.push_back("TRAFFIC_CONE");
	if (theDef.pail)
		aList.push_back("PAIL");
	return aList;
}

Rgb ModModeNameColor(std::string_view theName, int theCounter) {
	if (theName == "red")
		return {194, 19, 37};
	if (theName == "purple")
		return {92, 0, 170};
	if (theName == "blue")
		return {20, 126, 220};
	if (theName == "green")
		return {10, 204, 20};
	if (theName == "pulse") {
		const int v = theCounter & 0xff;
		return v <= 127 ? Rgb{61, v, 61} : Rgb{v / 2 + 36, 61, 61};
	}
	return {};
}

int ModeListLayout::MaxScroll(int theCount) const { return std::max(0, Rows(theCount) * rowH - viewH); }

int ModeListLayout::ClampScroll(int theScroll, int theCount) const {
	return std::clamp(theScroll, 0, MaxScroll(theCount));
}

void ModeListLayout::CellPos(int theIndex, int theScroll, int& x, int& y) const {
	x = left + (theIndex % cols) * colW;
	y = top + (theIndex / cols) * rowH - theScroll;
}

int ModeListLayout::HitTest(int x, int y, int theScroll, int theCount) const {
	if (y < top || y >= top + viewH || x < left)
		return -1;
	const int aCol = (x - left) / colW;
	const int aRow = (y - top + theScroll) / rowH;
	if (aCol >= cols || (x - left) % colW >= cellW || (y - top + theScroll) % rowH >= cellH)
		return -1;
	const int aIndex = aRow * cols + aCol;
	return aIndex < theCount ? aIndex : -1;
}

bool ModeListLayout::IsDrag(int theFromY, int theToY) const { return std::abs(theToY - theFromY) > dragSlop; }

} // namespace pvzu::unnamed
