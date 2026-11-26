#include "Mod/Unnamed/LaterLevels.h"

#include "Mod/Unnamed/Rules.h"

#include <algorithm>
#include <charconv>
#include <format>
#include <map>

namespace pvzu::unnamed {

namespace {
constexpr int kMaxWaves = 100;  // Board.h MAX_ZOMBIE_WAVES (mZombiesInWave rows)
constexpr int kNumRows = 6;     // Board::mPlantRow
constexpr const char* kBackgrounds[] = {"DAY", "NIGHT", "POOL", "FOG", "ROOF", "NIGHT_ROOF", "MUSHROOM_GARDEN", "GREENHOUSE",
	"ZOMBIQUARIUM"};
constexpr const char* kSpecials[] = {"none", "shooter", "starSnake", "boss", "hidden1", "hidden2", "hidden3"};

std::map<int, LaterLevel> gTable;

bool IntField(const nlohmann::json& theRow, const char* theKey, int theMin, int theMax, int& theOut, std::string* err,
	int theLevel) {
	auto it = theRow.find(theKey);
	if (it == theRow.end())
		return true;
	if (!it->is_number_integer() || it->get<long long>() < theMin || it->get<long long>() > theMax) {
		*err = std::format("levels: {} \"{}\" must be a whole number {}..{}", theLevel, theKey, theMin, theMax);
		return false;
	}
	theOut = it->get<int>();
	return true;
}

bool ReadRow(int theLevel, const nlohmann::json& theRow, LaterLevel& theOut, std::string* err) {
	if (!theRow.is_object()) {
		*err = std::format("levels: {} must be an object", theLevel);
		return false;
	}
	theOut.level = theLevel;
	theOut.stage = theRow.value("stage", std::string());
	const std::string aBackground = theRow.value("background", std::string());
	theOut.background = BackgroundCode(aBackground);
	if (theOut.background < 0) {
		*err = std::format("levels: {} has unknown background \"{}\"", theLevel, aBackground);
		return false;
	}
	if (!theRow.contains("waves")) {
		*err = std::format("levels: {} needs \"waves\"", theLevel);
		return false;
	}
	if (!IntField(theRow, "waves", 1, kMaxWaves, theOut.waves, err, theLevel) ||
		!IntField(theRow, "startSun", 0, 1000000, theOut.startSun, err, theLevel) ||
		!IntField(theRow, "freePotColumns", 0, 9, theOut.freePotColumns, err, theLevel) ||
		!IntField(theRow, "firstWaveCountdown", 1, 1000000, theOut.firstWaveCountdown, err, theLevel) ||
		!IntField(theRow, "fogColumn", 0, 1000, theOut.fogColumn, err, theLevel) ||
		!IntField(theRow, "introTimer", 0, 1000000, theOut.introTimer, err, theLevel))
		return false;
	if (auto it = theRow.find("allowed"); it != theRow.end()) {
		if (!it->is_array()) {
			*err = std::format("levels: {} \"allowed\" must be a list of zombie names", theLevel);
			return false;
		}
		for (const auto& aName : *it) {
			if (!aName.is_string()) {
				*err = std::format("levels: {} \"allowed\" must be a list of zombie names", theLevel);
				return false;
			}
			theOut.allowed.push_back(aName.get<std::string>());
		}
	}
	if (auto it = theRow.find("special"); it != theRow.end()) {
		const std::string aName = it->is_string() ? it->get<std::string>() : std::string();
		theOut.special = SpecialFromName(aName);
		if (theOut.special == SpecialLevel::None && aName != SpecialName(SpecialLevel::None)) {
			*err = std::format("levels: {} has unknown special \"{}\"", theLevel, aName);
			return false;
		}
	}
	return true;
}

int AreaOf(int theLevel) { return (theLevel - 1) / 10 + 1; }
} // namespace

const char* SpecialName(SpecialLevel theSpecial) {
	const int i = static_cast<int>(theSpecial);
	return i >= 0 && i < static_cast<int>(std::size(kSpecials)) ? kSpecials[i] : kSpecials[0];
}

SpecialLevel SpecialFromName(std::string_view theName) {
	for (int i = 0; i < static_cast<int>(std::size(kSpecials)); i++)
		if (theName == kSpecials[i])
			return static_cast<SpecialLevel>(i);
	return SpecialLevel::None;
}

int BackgroundCode(std::string_view theName) {
	for (int i = 0; i < static_cast<int>(std::size(kBackgrounds)); i++)
		if (theName == kBackgrounds[i])
			return i;
	return -1;
}

bool SetLaterLevels(const nlohmann::json& theTable, std::string* err) {
	gTable.clear();
	if (!theTable.is_object()) {
		*err = "levels: top level must be an object keyed by level number";
		return false;
	}
	std::map<int, LaterLevel> aTable;
	for (const auto& [aKey, aRow] : theTable.items()) {
		int aLevel = 0;
		const auto [aEnd, aErr] = std::from_chars(aKey.data(), aKey.data() + aKey.size(), aLevel);
		if (aErr != std::errc() || aEnd != aKey.data() + aKey.size() || aLevel < 1) {
			*err = "levels: key \"" + aKey + "\" is not a level number";
			return false;
		}
		LaterLevel aOut;
		if (!ReadRow(aLevel, aRow, aOut, err))
			return false;
		aTable[aLevel] = std::move(aOut);
	}
	gTable = std::move(aTable);
	return true;
}

void ClearLaterLevels() { gTable.clear(); }

const LaterLevel* FindLaterLevel(int theLevel) {
	auto it = gTable.find(theLevel);
	return it == gTable.end() ? nullptr : &it->second;
}

bool UsesLaterTable(int theLevel) {
	const int aLast = RuleInt("laterLevels.lastVanillaLevel", -1);
	return aLast > 0 && theLevel > aLast && FindLaterLevel(theLevel) != nullptr;
}

std::vector<int> ResolveZombies(const std::vector<std::string>& theNames, const std::vector<std::string>& theEngineNames,
	std::string* err) {
	std::vector<int> aOut;
	for (const std::string& aName : theNames) {
		auto it = std::find(theEngineNames.begin(), theEngineNames.end(), aName);
		if (it == theEngineNames.end()) {
			*err += (err->empty() ? "unknown zombie " : ", ") + aName;
			continue;
		}
		aOut.push_back(static_cast<int>(it - theEngineNames.begin()));
	}
	return aOut;
}

int VanillaLevelIndex(int theLevel, int theNumVanillaLevels) {
	return std::clamp(theLevel - 1, 0, std::max(theNumVanillaLevels - 1, 0));
}

int AllowTableIndex(int theLevel, int theNumVanillaLevels) {
	const int aAbove = RuleInt("laterLevels.allowTableIndexAbove", kNoLaterRule);
	if (aAbove >= 0 && theLevel > theNumVanillaLevels)
		return std::min(aAbove, std::max(theNumVanillaLevels - 1, 0));
	return VanillaLevelIndex(theLevel, theNumVanillaLevels);
}

int FinalAdventureLevel(int theVanillaFinalLevel) {
	return RuleInt("laterLevels.finalLevel", theVanillaFinalLevel);
}

FinishStep FinishAdventureLevel(int theLevel, int theFinalLevel) {
	FinishStep aStep;
	if (theLevel == theFinalLevel) {
		aStep.nextLevel = 1;
		aStep.finishedAdventure = true;
	} else {
		aStep.nextLevel = theLevel + 1;
	}
	return aStep;
}

int StageArea(int theLevel, int theVanillaMaxArea) {
	const int aMax = RuleInt("laterLevels.stageAreaMax", theVanillaMaxArea);
	return std::clamp(AreaOf(theLevel), 1, std::max(aMax, 1));
}

std::string StageName(int theLevel, int theVanillaMaxArea) {
	const int aArea = StageArea(theLevel, theVanillaMaxArea);
	return std::format("{}-{}", aArea, theLevel - (aArea - 1) * 10); // LawnApp::GetStageString's form
}

bool PlantRowsFor(int theBackground, std::array<int, 6>& theRows) {
	const nlohmann::json* aRows = RuleArray(std::format("laterLevels.rows.{}", theBackground));
	if (aRows == nullptr || aRows->size() != kNumRows)
		return false;
	for (int i = 0; i < kNumRows; i++) {
		if (!(*aRows)[i].is_number_integer())
			return false;
		theRows[i] = (*aRows)[i].get<int>();
	}
	return true;
}

bool BackgroundCountsAsRoof(int theBackground) {
	const nlohmann::json* aList = RuleArray("laterLevels.roofBackgrounds");
	if (aList == nullptr)
		return false;
	for (const auto& v : *aList)
		if (v.is_number_integer() && v.get<int>() == theBackground)
			return true;
	return false;
}

int SkipClickFromPointer(int theButtonId, int theHeldTicks) {
	if (theButtonId < 0)
		return -1;
	if (theButtonId == 2)
		return 3;
	const int aLong = RuleInt("laterLevels.skip.longPressTicks", kNoLaterRule);
	return aLong > 0 && theHeldTicks >= aLong ? -1 : 1;
}

SkipAction LevelSkip::Press(SkipButton theButton, int theClick, int theProfileLevel, int theFinishedAdventure) {
	SkipAction aOut;
	const int aNeeds = RuleInt("laterLevels.skip.needsFinishedAdventure", kNoLaterRule);
	if (aNeeds < 0 || theFinishedAdventure < aNeeds) // 0x7a1600: PlayerInfo and mFinishedAdventure > 0
		return aOut;
	const int aMin = RuleInt("laterLevels.skip.min", 1);
	const int aMax = RuleInt("laterLevels.skip.max", 71);
	if (theButton == SkipButton::Adventure && theClick == -1) {
		mTarget = theProfileLevel;
		if (!mActive) { // the "Level skip" dialog; a second right click falls through to the normal Adventure start
			mActive = true;
			aOut.kind = SkipAction::Opened;
		}
		aOut.target = mTarget;
		return aOut;
	}
	if (!mActive)
		return aOut;
	aOut.target = mTarget;
	if (theClick != 3) {
		if (theButton == SkipButton::Minigame || theButton == SkipButton::Puzzle) {
			const int aStep = RuleInt(theButton == SkipButton::Minigame ? "laterLevels.skip.bigStep" : "laterLevels.skip.smallStep",
				theButton == SkipButton::Minigame ? 10 : 1);
			mTarget = std::clamp(mTarget + theClick * aStep, aMin, aMax);
			aOut.kind = SkipAction::Stepped;
			aOut.target = mTarget;
			return aOut;
		}
		if (theButton != SkipButton::Survival)
			return aOut; // any other button works as usual, the mode stays on
		if (theClick != -1) {
			aOut.kind = SkipAction::AskConfirm;
			return aOut;
		}
	}
	// middle click anywhere, or a right click on Survival: "level skip cancelled"
	Reset();
	aOut.kind = SkipAction::Cancelled;
	return aOut;
}

int LevelSkip::Confirm() {
	const int aLevel = mTarget;
	Reset();
	return aLevel;
}

void LevelSkip::Decline() {}

void LevelSkip::Reset() {
	mActive = false;
	mTarget = 0;
}

std::string SkipTargetName(int theLevel) {
	int aArea = theLevel / 10 + 1;
	int aSub = theLevel % 10;
	if (aSub == 0) {
		aArea = theLevel / 10;
		aSub = 10;
	}
	return std::format("{}-{}", aArea, aSub);
}

} // namespace pvzu::unnamed
