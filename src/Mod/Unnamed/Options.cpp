#include "Mod/Unnamed/Options.h"

#include <algorithm>

namespace pvzu::unnamed {

const std::vector<OptionInfo>& OptionTable() {
	static const std::vector<OptionInfo> kTable = {
		{OptionId::EliteZombies, "eliteZombies", 3, eliteoption::DEFAULT, "PVZU_OPT_ELITES", "Elite zombies"},    // 精英僵尸
		{OptionId::DoubleSun, "doubleSun", 2, 0, "PVZU_OPT_DOUBLE_SUN", "Double sun"},                         // 双倍阳光
		{OptionId::AutoCollect, "autoCollect", 2, 0, "PVZU_OPT_AUTO_COLLECT", "Auto-collect"},                 // 自动收集
		{OptionId::FreePlanting, "freePlanting", 2, 0, "PVZU_OPT_FREE_PLANTING", "Free planting"},             // 免费种植
		{OptionId::CloseDoor, "closeDoor", 2, 0, "PVZU_OPT_CLOSE_DOOR", "Close the door"},                     // 关闭家门
		{OptionId::FreezeZombies, "freezeZombies", 2, 0, "PVZU_OPT_FREEZE", "Zombies stop"},                   // STOP
		{OptionId::TreePower, "treePower", 2, 1, "PVZU_OPT_TREE_POWER", "Tree power"},                         // 智慧树之力 (power of the Tree of Wisdom)
		// R8, the live dialog's order (0xb72d48..): 墓碑模式, 无限制出怪, 戴夫不选卡, 刷新点前移, 下雨模式; then full fog
		{OptionId::Gravestones, "gravestones", 2, 0, "PVZU_OPT_GRAVESTONES", "Gravestone mode", true},        // 墓碑模式
		{OptionId::NoSpawnLimit, "noSpawnLimit", 2, 0, "PVZU_OPT_NO_SPAWN_LIMIT", "No spawn limit", true},     // 无限制出怪
		{OptionId::DaveNoPick, "daveNoPick", 2, 0, "PVZU_OPT_DAVE_NO_PICK", "Dave doesn't pick", true},       // 戴夫不选卡
		{OptionId::SpawnForward, "spawnForward", 2, 0, "PVZU_OPT_SPAWN_FORWARD", "Spawn point forward", true}, // 刷新点前移
		{OptionId::Rain, "rain", 3, 0, "PVZU_OPT_RAIN", "Rain", true},                                         // 下雨模式
		{OptionId::FullFog, "fullFog", 2, 0, "PVZU_OPT_FULL_FOG", "Full-screen fog", true},                    // 全屏浓雾
		// port row (feature unnamedMusic): 0 Original (default), 1 Unnamed mod (Mod/Unnamed/Music.h musicchoice)
		{OptionId::Music, "music", 2, 0, "PVZU_OPT_MUSIC", "Music", false, true},
	};
	return kTable;
}

const OptionInfo& InfoFor(OptionId id) {
	const auto& t = OptionTable();
	return *std::find_if(t.begin(), t.end(), [id](const OptionInfo& i) { return i.id == id; });
}

namespace {
size_t IndexOf(OptionId id) {
	const auto& t = OptionTable();
	return static_cast<size_t>(std::find_if(t.begin(), t.end(), [id](const OptionInfo& i) { return i.id == id; }) - t.begin());
}
} // namespace

Options::Options() {
	for (const auto& i : OptionTable())
		mValues.push_back(i.defaultValue);
}

int Options::Get(OptionId id) const { return mValues[IndexOf(id)]; }

void Options::Set(OptionId id, int value) {
	const OptionInfo& i = InfoFor(id);
	mValues[IndexOf(id)] = value >= 0 && value < i.values ? value : i.defaultValue;
}

int Options::Cycle(OptionId id) {
	Set(id, (Get(id) + 1) % InfoFor(id).values);
	return Get(id);
}

Options OptionsFromMap(const std::map<std::string, int>& theMap) {
	Options o;
	for (const auto& i : OptionTable())
		if (auto it = theMap.find(i.key); it != theMap.end())
			o.Set(i.id, it->second);
	return o;
}

std::map<std::string, int> OptionsToMap(const Options& theOptions) {
	std::map<std::string, int> m;
	for (const auto& i : OptionTable())
		m[i.key] = theOptions.Get(i.id);
	return m;
}

const char* OptionValueLabelKey(OptionId id, int value) {
	if (id == OptionId::Music)
		return value != 0 ? "PVZU_OPT_MUSIC_MOD" : "PVZU_OPT_MUSIC_ORIGINAL";
	if (id == OptionId::Rain && value != 0)
		return value == 2 ? "PVZU_OPT_RAIN_STORM" : "PVZU_OPT_RAIN_RAIN";
	if (id == OptionId::EliteZombies)
		return value == eliteoption::NONE ? "PVZU_OPT_ELITES_NONE" : value == eliteoption::ALL ? "PVZU_OPT_ELITES_ALL" : "PVZU_OPT_ELITES_DEFAULT";
	return value != 0 ? "PVZU_OPT_ON" : "PVZU_OPT_OFF";
}

const char* OptionValueLabel(OptionId id, int value) {
	if (id == OptionId::Music)
		return value != 0 ? "Unnamed mod" : "Original";
	if (id == OptionId::Rain && value != 0) // 默认 / 下雨天 / 暴风雨
		return value == 2 ? "Storm" : "Rain";
	if (id == OptionId::EliteZombies) // 默认 / 无精英 / 全精英
		return value == eliteoption::NONE ? "None" : value == eliteoption::ALL ? "All" : "Default";
	return value != 0 ? "On" : "Off";
}

CardRowText CardRow(bool theCardOn, bool theImitaterOwned, bool thePending) {
	CardRowText t = theCardOn ? CardRowText{"PVZU_OPT_ON", "On", "", "", true} : CardRowText{"PVZU_OPT_OFF", "Off", "", "", false};
	if (theCardOn && !theImitaterOwned) {
		t.hintKey = "PVZU_OPT_CARD_NEEDS_IMITATER";
		t.hint = "Buy the Imitater in the shop";
	} else if (thePending) {
		t.hintKey = "PVZU_OPT_CARD_NEXT_CHOOSER";
		t.hint = "From the next seed chooser";
	}
	return t;
}

std::vector<OptionId> PanelOptions(bool theExtra, bool theMusic) {
	std::vector<OptionId> v;
	for (const auto& i : OptionTable())
		if ((theExtra || !i.extra) && (theMusic || !i.music))
			v.push_back(i.id);
	return v;
}

int PanelRowCount(bool theModPlantsOn, bool theExtra, bool theMusic) {
	return static_cast<int>(PanelOptions(theExtra, theMusic).size()) + (theModPlantsOn ? 1 : 0);
}

Weather OptionWeather(int theRainOption) { return {theRainOption != 0, theRainOption == 2}; }

bool GraveDangerWaveRules(bool theGraveDangerMode, bool theGravestones) { return theGraveDangerMode || theGravestones; }

bool DavePicksSeeds(bool theVanillaPicks, int theLevel, bool theDaveNoPick) {
	return theVanillaPicks && !theDaveNoPick && theLevel != 65 && theLevel != 73;
}

bool DavePickLocks(bool thePicked, bool theDaveNoPick) { return thePicked && !theDaveNoPick; }

float ForwardStartOffset(float theOffset, bool theForward) { return theForward ? 0.0f : theOffset; }

int ForwardStartBaseX(int theBase, int theOffset, bool theForward) { return theForward ? theBase - theOffset : theBase; }

std::string QuickSaveName(const std::string& theModeSaveName) {
	return theModeSaveName.empty() ? std::string() : theModeSaveName + ".quick";
}

int ConveyorStep(bool theDoubleSun, bool theFast) {
	int aStep = 1; // 0x7a031a / 0x7a0375: `mov eax,1; cmp [0x800110],1; add eax,eax; cmp [0x800118],1; add eax,eax`
	if (theDoubleSun)
		aStep *= 2;
	if (theFast)
		aStep *= 2;
	return aStep;
}

int ConveyorStepDown(int theValue, int theStep) { return theValue > theStep ? theValue - theStep : 0; }

int CollectedSunValue(int theValue, bool theDoubleSun) {
	if (!theDoubleSun)
		return theValue;
	switch (theValue) { // 0x797c32: the checks run 15, 50, 25 in that order, each value maps once
	case 15: return 25;
	case 50: return 75;
	case 25: return 50;
	default: return theValue;
	}
}

bool CoinCollectsItself(int theCoinType, bool theBeingCollected, bool theAutoCollect) {
	return theAutoCollect && !theBeingCollected && theCoinType >= 1 && theCoinType <= 6;
}

bool HouseCheckRuns(bool theIZombieLevel, bool theCloseDoor) { return !theIZombieLevel && !theCloseDoor; }


int EffectiveEliteOption(int theSetting, bool theFeatureOn) {
	if (!theFeatureOn || theSetting < eliteoption::DEFAULT || theSetting > eliteoption::ALL)
		return eliteoption::DEFAULT;
	return theSetting;
}

OptionRoll TallnutOddRoll(int theEliteOption, int theDefaultRange) {
	const int t = 2 - theEliteOption;
	if (t == 2)
		return {std::max(theDefaultRange, 0), 0};
	return {0, t};
}

} // namespace pvzu::unnamed
