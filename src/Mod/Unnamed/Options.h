// PVZU: Unnamed's settings and cheats ("[quote not included]" right-click for a surprise, the research notes (modes-ui-shop) section 3;
// typed codes, w6-w7-and-new-content.md 1.3). The exe keeps them as globals saved in WMInit.ini [BasicSettings];
// here they are one table of options saved in pvzu-settings.json ("unnamedOptions") and shown in a touch panel
// (Mod/OptionsPanel.h, Mod/Engine/OptionsPanel.h). Pure rules; the engine passes in what it knows.
//
//   option          exe global        ini key           typed code  values
//   eliteZombies    0x800258          SpZombies         INFZOM      0 default, 1 no elites, 2 all elites
//   doubleSun       0x800110          Sun               SUN         0/1
//   autoCollect     0x800114          AutoCollecting    AUTO        0/1
//   freePlanting    LawnApp+0x814     Free              FREE        0/1 (vanilla's free-planting cheat)
//   closeDoor       0x8001a4          Shutdoor          -           0/1
//   freezeZombies   0x800250          -                 STOP        0/1
//   treePower       0x800254 ^ 1      Wisdom            HARDWS      1 on (default), 0 off
// R8 (feature "extraOptions", the research notes (r8-options); rows an older pack does not show):
//   gravestones     0xc113a8          (FogFullScreen)   -           0/1 墓碑模式, Grave Danger wave rules (hook 182)
//   noSpawnLimit    0x800278          Unlimited         HARDSP      0/1 无限制出怪, no wave / points gate (hook 42)
//   daveNoPick      0x800450          DaveChooseNot     -           0/1 戴夫不选卡 (hooks 436, 447)
//   spawnForward    0x80027c          -                 -           0/1 刷新点前移, no +133 px start (hooks 480..512)
//   rain            0x80011c          Rain              RAIN        0 off, 1 rain, 2 storm (hooks 117, 163, 164, 185, 186)
//   fullFog         0x800298          -                 -           0/1 全屏浓雾 (hook 145; a port row: in 1.3beta2
//                                                                   only dead dialog code writes it, r8-options.md 2)
// Port row (feature "unnamedMusic"; not in the exe, 2026-11-24):
//   music           -                 -                 -           0 Original (vanilla mainmusic.mo3, default),
//                                                                   1 Unnamed mod (lawnbgm(N).mp3); Mod/Unnamed/Music.h
//
// CARD (exe flag 0x8001a0, typed code CARD, a row of the exe's cheat panel, hook 764; w6-w7-and-new-content.md 1.3)
// is a row of the panel too, but it is stored as the top-level "card" setting (Settings::card, Mod/Mode.h), so it is
// not in the option table: the panel adds its row (PanelRowCount) when the pack has mod plants.
//
// Not here (see the report): FAST is the HUD's 1x/2x button (same effect: one extra Board update per frame, hook 167);
// WIN is a one-off action of the panel, not a stored option.
#pragma once

#include <map>
#include <string>
#include <vector>

namespace pvzu::unnamed {

// The global "elite zombies" option (exe 0x800258): 0 default, 1 no elites, 2 all elites.
namespace eliteoption {
constexpr int DEFAULT = 0;
constexpr int NONE = 1;
constexpr int ALL = 2;
}

enum class OptionId { EliteZombies, DoubleSun, AutoCollect, FreePlanting, CloseDoor, FreezeZombies, TreePower,
	Gravestones, NoSpawnLimit, DaveNoPick, SpawnForward, Rain, FullFog, Music };

struct OptionInfo {
	OptionId id;
	const char* key;        // pvzu-settings.json "unnamedOptions" key
	int values;             // the value cycles 0 .. values-1
	int defaultValue;
	const char* labelKey;   // string id (LawnStrings / pack), English fallback below
	const char* label;
	bool extra = false;     // R8 row: shown (and in effect) only with the pack feature "extraOptions"
	bool music = false;     // the music row: shown only with the pack feature "unnamedMusic"
};
// Panel order (the exe dialog's order where it has the option: Sun, Auto, Fast, Free, Shutdoor, Wisdom, ..., SpZombies
// last; elites first here, it is the one a player changes most).
const std::vector<OptionInfo>& OptionTable();
const OptionInfo& InfoFor(OptionId id);

class Options {
public:
	Options(); // every option at its default
	int Get(OptionId id) const;
	void Set(OptionId id, int value);  // out of range -> default
	int Cycle(OptionId id);            // next value (wraps); returns it
	bool On(OptionId id) const { return Get(id) != 0; }
	bool operator==(const Options&) const = default;

private:
	std::vector<int> mValues;
};

// pvzu-settings.json form: every option by key. Unknown keys and out-of-range values -> defaults.
Options OptionsFromMap(const std::map<std::string, int>& theMap);
std::map<std::string, int> OptionsToMap(const Options& theOptions);

// Value shown on the option's button: "Off"/"On"; elites "Default"/"None"/"All".
const char* OptionValueLabelKey(OptionId id, int value);
const char* OptionValueLabel(OptionId id, int value);

// ---- the CARD row ----
// Hook 256: mod seeds (and 49..52) are owned only while CARD is on AND the Imitater is bought; CARD on without the
// Imitater shows nothing new, so the row says what is missing. thePending: changed while a seed chooser is open (it
// counts from the next one, see CardLatch).
inline constexpr const char* kCardRowLabelKey = "PVZU_OPT_CARD";
inline constexpr const char* kCardRowLabel = "Mod plants";
struct CardRowText {
	const char* valueKey;
	const char* value;   // "On" / "Off"
	const char* hintKey; // "" = no hint line
	const char* hint;
	bool nonDefault;     // green pill (CARD's default is off)
};
CardRowText CardRow(bool theCardOn, bool theImitaterOwned, bool thePending);
// The options the panel shows, in its order: the table without the R8 rows unless theExtra ("extraOptions") and
// without the music row unless theMusic ("unnamedMusic").
std::vector<OptionId> PanelOptions(bool theExtra, bool theMusic = false);
// Rows of the panel: those options, plus CARD when the pack has mod plants (its last row).
int PanelRowCount(bool theModPlantsOn, bool theExtra = false, bool theMusic = false);

// CARD as saved vs CARD in effect. The seed chooser lays its seeds out when it is made (0..52 and 0x50..0x7f with
// CARD), so a change while a chooser is open waits for the next chooser; with none open it counts at once.
class CardLatch {
public:
	explicit CardLatch(bool theSetting = false) : mSetting(theSetting), mInEffect(theSetting) {}
	void Set(bool theSetting, bool theChooserOpen) {
		mSetting = theSetting;
		if (!theChooserOpen)
			mInEffect = theSetting;
	}
	void ChooserOpens() { mInEffect = mSetting; }
	bool Setting() const { return mSetting; }
	bool InEffect() const { return mInEffect; }

private:
	bool mSetting;
	bool mInEffect;
};

// ---- effects (pure) ----
// Hook 204 (0x797c32), at the "Board sun += value" of a collected sun: 15 -> 25, 25 -> 50, 50 -> 75 with double sun.
int CollectedSunValue(int theValue, bool theDoubleSun);
// V2 (C2): hooks 161 (0x7a031a) and 473 (0x7a0375): the conveyor's countdown (Challenge+0x5c) and each packet's slide
// (every 4 ticks) step by 1, x2 with double sun (0x800110), x2 again with Fast (0x800118); never below 0.
int ConveyorStep(bool theDoubleSun, bool theFast);
int ConveyorStepDown(int theValue, int theStep);
// Hook 207 (0x7982d0), Coin::Update: a coin of type 1..6 (silver, gold, diamond, sun, small sun, large sun) not
// being collected yet collects itself while auto-collect is on.
bool CoinCollectsItself(int theCoinType, bool theBeingCollected, bool theAutoCollect);
// Hook 630 (0x7a0281): the zombie "reached the house" check runs unless it is an I, Zombie level or the door is closed.
bool HouseCheckRuns(bool theIZombieLevel, bool theCloseDoor);
// Hook 478 (the Zen glove and a busy Squash) lives in Mod/Unnamed/ModesVanilla.h (GloveCanPickUp), feature modesVanilla.
// Elite option in effect: the setting when the panel's feature is on, else the default.
int EffectiveEliteOption(int theSetting, bool theFeatureOn);
// ---- R8 (the research notes (r8-options)) ----
// Rain 0x80011c: 1 = the rain of It's Raining Seeds / Stormy Night (hooks 117, 164, 185), 2 = also its lightning and
// darkness (hooks 163, 186). Looks only.
struct Weather { bool rain = false; bool storm = false; };
Weather OptionWeather(int theRainOption);
// Hook 182 (0xc11373): Challenge::SpawnZombieWave runs Grave Danger's flag-wave / wave > 5 graves with 墓碑模式 on.
bool GraveDangerWaveRules(bool theGraveDangerMode, bool theGravestones);
// Hook 436 (0xc13410 -> 0x95b7ba): Dave picks when the vanilla gate says so, not at 7-5 (65) / 7-10 (73), not with
// 戴夫不选卡.
bool DavePicksSeeds(bool theVanillaPicks, int theLevel, bool theDaveNoPick);
// Hook 447 (0xc03630): a seed Dave picked stays locked in the bank unless 戴夫不选卡 is on (then it is freed).
bool DavePickLocks(bool thePicked, bool theDaveNoPick);
// Hooks 480 / 492 / 495 / 497 / 512: 刷新点前移 drops the wide lawn's start offset (+133) and takes it off the four
// fixed start bases (955 -> 822, 933 -> 800, 958 -> 825, 978 -> 845).
float ForwardStartOffset(float theOffset, bool theForward);
int ForwardStartBaseX(int theBase, int theOffset, bool theForward);
// 存档 / 读档 (0xb72a00, hooks 244 / 769): the port keeps the quick slot in its own file next to the mode's save
// (PvZ-Portable rewrites the mode's save on every pause). "" stays "".
std::string QuickSaveName(const std::string& theModeSaveName);

// Hook 508 (0xc033a0), Tall-nut head: `t = 2 - option; if (t == 2) t = Rand(12); t == 0 -> 0x101`. Returns the
// roll range to draw (0 = draw nothing) and, when nothing is drawn, the value t takes.
struct OptionRoll { int range = 0; int fixedRoll = 0; };
OptionRoll TallnutOddRoll(int theEliteOption, int theDefaultRange);

} // namespace pvzu::unnamed
