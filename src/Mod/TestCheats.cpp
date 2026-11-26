#include "Mod/TestCheats.h"

#include "Mod/Hud.h"

#include <algorithm>
#include <format>

namespace pvzu::cheats {

const std::vector<CheatInfo>& CheatTable() {
	static const std::vector<CheatInfo> kTable = {
		{CheatId::InfiniteSun, "infiniteSun", 2, "PVZU_CHEAT_SUN", "Infinite sun"},
		{CheatId::NoRecharge, "noRecharge", 2, "PVZU_CHEAT_RECHARGE", "No recharge"},
		{CheatId::PlantAnything, "plantAnything", 2, "PVZU_CHEAT_ANYTHING", "Plant anything"},
		{CheatId::Invincible, "invincible", 2, "PVZU_CHEAT_INVINCIBLE", "Invincible plants"},
		{CheatId::SeedSlots, "seedSlots", 11, "PVZU_CHEAT_SLOTS", "Seed slots"},
		{CheatId::Speed, "speed", 4, "PVZU_CHEAT_SPEED", "Speed"},
		{CheatId::AllModes, "allModes", 2, "PVZU_CHEAT_MODES", "All modes open"},
		{CheatId::GameButton, "gameButton", 2, "PVZU_CHEAT_GAME_BUTTON", "Cheats button in game"},
	};
	return kTable;
}

const CheatInfo& InfoFor(CheatId id) { return CheatTable()[static_cast<size_t>(id)]; }

Cheats::Cheats() : mValues(CheatTable().size(), 0) {}

int Cheats::Get(CheatId id) const { return mValues[static_cast<size_t>(id)]; }

void Cheats::Set(CheatId id, int value) {
	mValues[static_cast<size_t>(id)] = value >= 0 && value < InfoFor(id).values ? value : 0;
}

int Cheats::Cycle(CheatId id) {
	Set(id, (Get(id) + 1) % InfoFor(id).values);
	return Get(id);
}

Cheats CheatsFromMap(const std::map<std::string, int>& theMap) {
	Cheats c;
	for (const CheatInfo& i : CheatTable())
		if (auto it = theMap.find(i.key); it != theMap.end())
			c.Set(i.id, it->second);
	return c;
}

std::map<std::string, int> CheatsToMap(const Cheats& theCheats) {
	std::map<std::string, int> m;
	for (const CheatInfo& i : CheatTable())
		m[i.key] = theCheats.Get(i.id);
	return m;
}

const char* CheatValueLabel(CheatId id, int value) {
	static const char* const kSlots[] = {"Normal", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15"};
	static const char* const kSpeed[] = {"1x", "2x", "4x", "8x"};
	if (id == CheatId::SeedSlots)
		return value >= 0 && value <= 10 ? kSlots[value] : kSlots[0];
	if (id == CheatId::Speed)
		return value >= 0 && value <= 3 ? kSpeed[value] : kSpeed[0];
	return value != 0 ? "On" : "Off";
}

int CheatSeedSlots(int theValue) { return theValue >= 1 && theValue <= 10 ? theValue + 5 : 0; }

int CheatSpeedFactor(int theValue) { return theValue >= 0 && theValue <= 3 ? 1 << theValue : 1; }

int EffectiveSpeed(int theHudSpeed, int theCheatValue) { return std::max(theHudSpeed, CheatSpeedFactor(theCheatValue)); }

NoRechargeStep NoRechargeFor(bool theActive, bool theRefreshing) {
	if (theRefreshing)
		return NoRechargeStep::FinishRefresh;
	return theActive ? NoRechargeStep::Leave : NoRechargeStep::Activate;
}

int NoRechargeCounter(int theCounter, int theRefreshTime) { return std::max(theCounter, theRefreshTime); }

bool BotStuckEater(bool theEating, bool theTunneling, bool theFlying, bool theInvincible) {
	return theEating && theInvincible && (theTunneling || theFlying);
}

bool BotStallWatch::Update(int theFrame, int theCount, int theHpSum, int theLimit) {
	if (theCount == 0 || since < 0 || theCount != count || theHpSum != hp) {
		since = theFrame;
		count = theCount;
		hp = theHpSum;
		return false;
	}
	if (theFrame - since < theLimit)
		return false;
	since = theFrame;
	return true;
}

int BotShooterStep(int thePlantRow, int theTargetRow) {
	if (theTargetRow < 0 || theTargetRow == thePlantRow)
		return 0;
	return theTargetRow > thePlantRow ? 1 : -1;
}

BotTurn BotSnakeTurn(float theX, float theY, float theVx, float theVy) {
	constexpr float kLeft = 140.0f, kRight = 760.0f, kTop = 120.0f, kBottom = 500.0f;
	if (theVx > 0 && theX >= kRight)
		return theY >= kBottom ? BotTurn::Up : BotTurn::Down;
	if (theVy > 0 && theY >= kBottom)
		return theX <= kLeft ? BotTurn::Right : BotTurn::Left;
	if (theVx < 0 && theX <= kLeft)
		return theY <= kTop ? BotTurn::Down : BotTurn::Up;
	if (theVy < 0 && theY <= kTop)
		return theX >= kRight ? BotTurn::Left : BotTurn::Right;
	return BotTurn::None;
}

// ---- level jump ----
namespace {
constexpr const char* kReturnProfile = "returnProfile";
constexpr const char* kReturnLevel = "returnLevel";
constexpr const char* kReturnFinished = "returnFinished";
} // namespace

LevelReturn ReturnFromMap(const std::map<std::string, int>& theMap) {
	LevelReturn r;
	const auto p = theMap.find(kReturnProfile), l = theMap.find(kReturnLevel), f = theMap.find(kReturnFinished);
	if (p == theMap.end() || l == theMap.end() || p->second < 0)
		return r;
	r.profileId = p->second;
	r.level = l->second;
	r.finished = f != theMap.end() ? f->second : 0;
	return r;
}

void ReturnToMap(const LevelReturn& theReturn, std::map<std::string, int>& theMap) {
	theMap.erase(kReturnProfile);
	theMap.erase(kReturnLevel);
	theMap.erase(kReturnFinished);
	if (!theReturn.Active())
		return;
	theMap[kReturnProfile] = theReturn.profileId;
	theMap[kReturnLevel] = theReturn.level;
	theMap[kReturnFinished] = theReturn.finished;
}

LevelReturn BeginJump(const LevelReturn& theCurrent, int theProfileId, int theLevel, int theFinished) {
	if (theCurrent.Active() && theCurrent.profileId == theProfileId)
		return theCurrent;
	return LevelReturn{theProfileId, theLevel, theFinished};
}

bool RestoreFor(const LevelReturn& theReturn, int theProfileId) { return theReturn.Active() && theReturn.profileId == theProfileId; }

int LevelsInWorld(int theWorld) {
	if (theWorld >= 1 && theWorld <= 7)
		return 10;
	return theWorld == 8 ? kLastLevel - 70 : 0;
}

int LevelFor(int theWorld, int theIndex) {
	if (theIndex < 0 || theIndex >= LevelsInWorld(theWorld))
		return 0;
	return (theWorld - 1) * 10 + theIndex + 1;
}

int WorldOfLevel(int theLevel) { return theLevel <= 0 ? 1 : (theLevel - 1) / 10 + 1; }

std::string LevelName(int theLevel) { return std::format("{}-{}", WorldOfLevel(theLevel), (theLevel - 1) % 10 + 1); }

int FinalWaveStart(int theCurrentWave, int theNumWaves) { return theNumWaves <= 0 ? theCurrentWave : std::max(theCurrentWave, theNumWaves - 1); }

// ---- layouts ----
namespace {
constexpr int kCheatsTitleH = 44; // a little lower than the options title so 4 rows of cheats fit 600 px in 2 columns
constexpr int kStep = kOptionsRowH + kOptionsGap;

struct Frame {
	int x, w;  // panel
	int innerX, innerW;
};
Frame CentredFrame(int theScreenW, int theSafeLeft, int theSafeRight, int theInnerW) {
	const int aSafeX = theSafeLeft + kOptionsMargin;
	const int aSafeW = std::max(0, theScreenW - theSafeLeft - theSafeRight - 2 * kOptionsMargin);
	const int aW = kOptionsPad + kOptionsPadRight + theInnerW;
	const int aX = aSafeX + (aSafeW - aW) / 2;
	return {aX, aW, aX + kOptionsPad, theInnerW};
}
int SafeInnerW(int theScreenW, int theSafeLeft, int theSafeRight) {
	return std::max(0, theScreenW - theSafeLeft - theSafeRight - 2 * kOptionsMargin) - kOptionsPad - kOptionsPadRight;
}
// n equal buttons across a width
std::vector<HitRect> Row(int x, int y, int w, int n, int h = kOptionsRowH) {
	std::vector<HitRect> out;
	const int aW = (w - (n - 1) * kOptionsGap) / n;
	for (int i = 0; i < n; i++)
		out.push_back({x + i * (aW + kOptionsGap), y, aW, h});
	return out;
}
} // namespace

CheatsPageLayout LayoutCheatsPage(int theScreenW, int theScreenH, int theSafeLeft, int theSafeRight, int theRows) {
	CheatsPageLayout l;
	const int aInner = SafeInnerW(theScreenW, theSafeLeft, theSafeRight);
	l.columns = std::clamp((aInner + kOptionsGap) / (kOptionsMinColW + kOptionsGap), 1, 2); // 8 cheats: 4 + 4
	const int aColW = std::min(kOptionsMaxColW, (aInner - (l.columns - 1) * kOptionsGap) / l.columns);
	const int aPerCol = (theRows + l.columns - 1) / l.columns;
	const Frame f = CentredFrame(theScreenW, theSafeLeft, theSafeRight, l.columns * aColW + (l.columns - 1) * kOptionsGap);
	const int aH = 2 * kOptionsPad + kCheatsTitleH + aPerCol * kStep + kStep + kOptionsRowH;
	l.panel = {f.x, std::max(kOptionsMargin, (theScreenH - aH) / 2), f.w, aH};
	l.title = {f.innerX, l.panel.y + kOptionsPad, f.innerW, kCheatsTitleH - kOptionsGap};
	const int aTop = l.panel.y + kOptionsPad + kCheatsTitleH;
	for (int i = 0; i < theRows; i++)
		l.rows.push_back({f.innerX + (i / aPerCol) * (aColW + kOptionsGap), aTop + (i % aPerCol) * kStep, aColW, kOptionsRowH});
	l.actions = Row(f.innerX, aTop + aPerCol * kStep, f.innerW, kCheatActions);
	const int aFooterY = l.panel.y + aH - kOptionsPad - kOptionsRowH;
	const int aFooterW = std::min(kOptionsFooterW, aColW);
	l.back = {f.innerX, aFooterY, aFooterW, kOptionsRowH};
	l.done = {f.innerX + f.innerW - aFooterW, aFooterY, aFooterW, kOptionsRowH};
	return l;
}

PickerLayout LayoutLevelPicker(int theScreenW, int theScreenH, int theSafeLeft, int theSafeRight) {
	PickerLayout l;
	const int aInner = std::min(SafeInnerW(theScreenW, theSafeLeft, theSafeRight), 2 * kOptionsMaxColW + kOptionsGap + 200);
	const Frame f = CentredFrame(theScreenW, theSafeLeft, theSafeRight, aInner);
	// title, world tabs, 2 rows of 5 levels, modes, footer
	const int aH = 2 * kOptionsPad + kCheatsTitleH + 4 * kStep + kOptionsRowH;
	l.panel = {f.x, std::max(kOptionsMargin, (theScreenH - aH) / 2), f.w, aH};
	l.title = {f.innerX, l.panel.y + kOptionsPad, f.innerW, kCheatsTitleH - kOptionsGap};
	int y = l.panel.y + kOptionsPad + kCheatsTitleH;
	l.worlds = Row(f.innerX, y, f.innerW, kPickerWorlds);
	y += kStep;
	for (int r = 0; r < 2; r++, y += kStep)
		for (const HitRect& b : Row(f.innerX, y, f.innerW, 5))
			l.levels.push_back(b);
	l.modes = Row(f.innerX, y, f.innerW, kPickerModes);
	y += kStep;
	l.back = {f.innerX, y, std::min(kOptionsFooterW, f.innerW / 3), kOptionsRowH};
	return l;
}

int HitIndex(const std::vector<HitRect>& theRects, int x, int y) {
	for (size_t i = 0; i < theRects.size(); i++)
		if (Inside(theRects[i], x, y))
			return static_cast<int>(i);
	return -1;
}

HitRect OptionsTitleCheatsButton(const OptionsPanelLayout& theLayout) {
	constexpr int kW = 200;
	const HitRect& t = theLayout.title;
	// from just inside the frame's top border down to the title band's foot (the rows start one gap lower)
	const int aY = theLayout.panel.y + kOptionsPad - 10;
	return {t.x + t.w - kW, aY, kW, kOptionsRowH};
}

HitRect MainMenuCheatsButton(int theMenuLeft) {
	constexpr int kW = 180;
	return {theMenuLeft + 800 - kOptionsMargin - kW, 8, kW, kOptionsRowH};
}

// ---- the in-game button ----
bool GameButtonShown(const GameButtonState& s) {
	return s.feature && s.toggle && s.inLevel && !s.dialogOpen && !s.paused && !s.daveTalking && !s.gardenOrTree;
}

GameButtonPlace GameButtonRect(const HitRect& theMenu, int theBarRight) {
	GameButtonPlace p;
	const int aLeftX = theMenu.x - kCardGap - theMenu.w;
	if (aLeftX >= theBarRight + kCardGap) {
		p.draw = {aLeftX, theMenu.y, theMenu.w, theMenu.h};
		p.hit = {aLeftX, 0, theMenu.w, std::max(kMinTouchSize, p.draw.y + p.draw.h)};
		return p;
	}
	p.underMenu = true;
	const int aMenuBottom = theMenu.y + theMenu.h;
	p.draw = {theMenu.x, aMenuBottom, theMenu.w, theMenu.h};
	p.hit = {theMenu.x, aMenuBottom + 2, theMenu.w, kGameButtonUnderMenuH};
	return p;
}

// ---- the cheat bot ----
BotCellPlan BotPlanFor(BotLawn theLawn, int theCol, bool theWater) {
	using namespace seed;
	BotCellPlan p;
	const bool aRoof = theLawn == BotLawn::Roof;
	const bool aWater = theWater && (theLawn == BotLawn::Pool || theLawn == BotLawn::Fog);
	const bool aNight = theLawn == BotLawn::Night || theLawn == BotLawn::Fog;
	if (aRoof)
		p.base = FLOWERPOT;
	else if (aWater)
		p.base = LILYPAD;
	if (aRoof) {
		// straight shots from the left half of the roof hit the slope: catapults there
		static const int kRoof[9] = {WINTERMELON, WINTERMELON, WINTERMELON, WINTERMELON, WINTERMELON, GATLINGPEA, GATLINGPEA, TALLNUT, GATLINGPEA};
		p.main = kRoof[std::clamp(theCol, 0, 8)];
	} else {
		static const int kLand[9] = {WINTERMELON, WINTERMELON, CACTUS, GATLINGPEA, GATLINGPEA, TORCHWOOD, GATLINGPEA, TALLNUT, SPIKEROCK};
		p.main = kLand[std::clamp(theCol, 0, 8)];
		if (aNight && theCol == 6)
			p.main = GLOOMSHROOM;
		if (aWater && p.main == CACTUS)
			p.main = CATTAIL; // on the lily pad: homing spikes, pops balloons
		if (aWater && p.main == SPIKEROCK)
			p.main = GATLINGPEA; // no Spikeweed on a lily pad
	}
	if (p.main != SPIKEROCK)
		p.armour = PUMPKINSHELL;
	return p;
}

std::vector<int> BotChooserSeeds(BotLawn theLawn) {
	std::vector<int> out;
	auto add = [&](int s) {
		if (s >= 0 && std::find(out.begin(), out.end(), s) == out.end())
			out.push_back(s);
	};
	for (bool aWater : {false, true})
		for (int col = 0; col < 9; col++) {
			const BotCellPlan p = BotPlanFor(theLawn, col, aWater);
			add(p.base);
			add(p.main);
			add(p.armour);
		}
	return out;
}

} // namespace pvzu::cheats
