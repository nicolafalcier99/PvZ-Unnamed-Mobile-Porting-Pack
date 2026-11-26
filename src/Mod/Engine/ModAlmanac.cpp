#include "Mod/Engine/ModAlmanac.h"

#include "Mod/Engine/ModPlants.h"
#include "Mod/Engine/ModPlantSites.h"
#include "Mod/Engine/Startup.h"
#include "Mod/GameData.h"
#include "Mod/Mode.h"
#include "Mod/Unnamed/ModAlmanac.h"
#include "Mod/Unnamed/ModPlantRules.h"
#include "Mod/Unnamed/ModPlantShadow.h"
#include "Mod/Unnamed/ModPlants.h"
#include "Mod/Unnamed/Rules.h"

#include "LawnApp.h"
#include "Resources.h"
#include "Lawn/Board.h"
#include "Lawn/CursorObject.h"
#include "Lawn/MessageWidget.h"
#include "Lawn/Plant.h"
#include "Lawn/SeedPacket.h"
#include "Lawn/Widget/AlmanacDialog.h"
#include "PvzpLib/PvzpCommon.h"
#include "PvzpLib/PvzpDebug.h"
#include "PvzpLib/PvzpStringFile.h"
#include "PvzpLib/Reanimator.h"
#include "graphics/Graphics.h"
#include "widget/WidgetManager.h"

#include <cstdio>
#include <format>
#include <fstream>
#include <sstream>

namespace pvzu::engine {

namespace u = pvzu::unnamed;

namespace {
// The pack's text: English, with strings/mod-almanac-zh.txt on top in Chinese (keys it lacks stay English).
const u::ModAlmanacTexts& Texts() {
	static u::ModAlmanacTexts sTexts;
	static std::string sLoadedFor;
	const std::string aFor = UnnamedPackDir().string() + "|" + StartupSettings().language;
	if (aFor == sLoadedFor)
		return sTexts;
	sLoadedFor = aFor;
	sTexts.clear();
	auto load = [](const char* theName, u::ModAlmanacTexts& theInto) {
		const std::filesystem::path aFile = UnnamedPackDir() / "strings" / theName;
		std::ifstream in(aFile, std::ios::binary);
		if (!in)
			return false;
		std::stringstream s;
		s << in.rdbuf();
		u::ModAlmanacTexts aRead;
		std::string aErr;
		if (!u::ParseModAlmanacText(s.str(), &aRead, &aErr)) {
			PvzpLogLn("PVZU: {} not used: {}", aFile.string(), aErr);
			return false;
		}
		for (auto& [k, v] : aRead)
			theInto[k] = v;
		return true;
	};
	if (!load("mod-almanac-en.txt", sTexts))
		PvzpLogLn("PVZU: strings/mod-almanac-en.txt missing: the mod plants' Almanac pages and hints have no text");
	if (StartupSettings().language == "zh")
		load("mod-almanac-zh.txt", sTexts);
	return sTexts;
}

std::string Text(const std::string& theKey) {
	const auto& t = Texts();
	const auto it = t.find(theKey);
	return it == t.end() ? std::string() : it->second;
}

bool OnPlantsPage(const AlmanacDialog* d) { return d->mOpenPage == AlmanacPage::ALMANAC_PAGE_PLANTS; }
bool ModGridShown(const AlmanacDialog* d) { return ModAlmanacOn() && OnPlantsPage(d) && d->mPvzuModPage; }

int SeedIndex(SeedType theSeed) {
	const auto& aSeeds = u::ModPlantSeeds();
	for (size_t i = 0; i < aSeeds.size(); ++i)
		if (aSeeds[i] == static_cast<int>(theSeed))
			return static_cast<int>(i);
	return -1;
}

Sexy::Rect ToRect(const u::AlmanacRect& r) { return Sexy::Rect(r.x, r.y, r.w, r.h); }

bool HintsOn() { return ModPlantsOn() && GameData::Get().Feature("modAlmanac") && u::RuleInt("modAlmanac.hints", 0) != 0; }

// A level for the once-per-level latch: another board, level or mode, or the same board started again (its clock went
// back). (A restarted level on a board at the same address whose clock has passed the last hint keeps its latch: then
// the hints stay quiet, never repeat.)
std::uintptr_t LevelId(const Board* theBoard) {
	static const Board* sBoard = nullptr;
	static int sLevel = -1, sMode = -1;
	static uint32_t sLastCounter = 0;
	static std::uintptr_t sId = 0;
	const int aMode = static_cast<int>(theBoard->mApp->mGameMode);
	if (theBoard != sBoard || theBoard->mLevel != sLevel || aMode != sMode || theBoard->mMainCounter < sLastCounter) {
		sBoard = theBoard;
		sLevel = theBoard->mLevel;
		sMode = aMode;
		++sId;
	}
	sLastCounter = theBoard->mMainCounter;
	return sId;
}

u::HintLatch& gHintLatch() {
	static u::HintLatch sLatch;
	return sLatch;
}

bool IsHintStyle(MessageStyle s) { return s >= MessageStyle::MESSAGE_STYLE_HINT_LONG && s <= MessageStyle::MESSAGE_STYLE_HINT_TALL_LONG; }

void ShowHint(Board* theBoard, int theSeed, const std::string& theKind, const std::string& theText, MessageStyle theStyle) {
	if (theText.empty() || theBoard == nullptr || !theBoard->mAdvice)
		return;
	if (!u::HintMayShow(theBoard->mAdvice->IsBeingDisplayed(), IsHintStyle(theBoard->mAdvice->mMessageStyle)))
		return;
	theBoard->DisplayAdvice(theText, theStyle, AdviceType::ADVICE_NONE);
	std::fprintf(stderr, "PVZU hint: frame %d seed 0x%02x %s \"%s\"\n", static_cast<int>(theBoard->mMainCounter), theSeed, theKind.c_str(), theText.c_str());
}

int ShadowPoints(Plant* thePlant, int theSeed) {
	std::vector<u::ShadowLawnPlant> aLawn;
	for (Plant* aPlant : thePlant->mBoard->mPlants) {
		if (aPlant->mDead)
			continue;
		u::ShadowLawnPlant q;
		q.seed = static_cast<int>(ModSeedOf(aPlant));
		q.row = aPlant->mRow;
		q.col = aPlant->mPlantCol;
		q.shadow = aPlant->mPvzu.shadow != 0;
		q.squished = aPlant->mSquished;
		q.self = aPlant == thePlant;
		aLawn.push_back(q);
	}
	return u::ShadowPointsOnLawn(theSeed, thePlant->mRow, thePlant->mPlantCol, aLawn);
}
} // namespace

bool ModAlmanacOn() {
	return CurrentMode() == pvzu::GameMode::Unnamed && ModPlantsOn() && GameData::Get().Feature("modAlmanac") &&
		u::RuleInt("modAlmanac.pages", 0) != 0;
}

SeedType ModAlmanacBaseSeed(SeedType theSeedType) {
	return ModAlmanacOn() && IsModSeed(theSeedType) ? static_cast<SeedType>(u::ModBaseSeed(theSeedType)) : theSeedType;
}

void ModAlmanacTintPlant(Plant* thePlant, SeedType theSelectedSeed) {
	if (!ModAlmanacOn() || thePlant == nullptr || !IsModSeed(theSelectedSeed))
		return;
	const u::ModPlant* p = u::FindModPlant(theSelectedSeed);
	// off the lawn Plant::UpdateReanimColor does nothing, so the colour set here stays (in play: hook 313, ModPlantColor)
	if (Reanimation* aBody = thePlant->mApp->ReanimationTryToGet(thePlant->mBodyReanimID); aBody && p)
		aBody->mColorOverride = Sexy::Color(p->tint[0], p->tint[1], p->tint[2], p->tint[3]);
}

bool DrawModAlmanacGrid(AlmanacDialog* theDialog, Sexy::Graphics* g) {
	if (!ModGridShown(theDialog))
		return false;
	// a plain panel over the vanilla grid's slot outlines (they do not match the bigger packets)
	const u::AlmanacRect aArea = u::ModAlmanacGridArea();
	g->SetColor(Sexy::Color(190, 140, 90));
	g->FillRect(aArea.x + 4, aArea.y + 2, aArea.w - 8, aArea.h - 4);
	g->SetColor(Sexy::Color(242, 186, 128));
	g->FillRect(aArea.x + 6, aArea.y + 4, aArea.w - 12, aArea.h - 8);
	g->SetColor(Sexy::Color::White);

	const int aMouseX = theDialog->mApp->mWidgetManager->mLastMouseX - theDialog->mX;
	const int aMouseY = theDialog->mApp->mWidgetManager->mLastMouseY - theDialog->mY;
	const auto& aSeeds = u::ModPlantSeeds();
	const int aOver = theDialog->mMouseVisible ? u::ModAlmanacCellAt(aMouseX, aMouseY, static_cast<int>(aSeeds.size())) : -1;
	const float s = u::ModAlmanacPacketScale();
	for (size_t i = 0; i < aSeeds.size(); ++i) {
		const u::AlmanacRect r = u::ModAlmanacPacket(static_cast<int>(i));
		Sexy::Graphics aG(*g);
		aG.SetScale(s, s, static_cast<float>(r.x), static_cast<float>(r.y));
		DrawSeedPacket(&aG, static_cast<float>(r.x), static_cast<float>(r.y), static_cast<SeedType>(aSeeds[i]), SeedType::SEED_NONE, 0, 255, true, false);
		if (static_cast<int>(i) == aOver || aSeeds[i] == static_cast<int>(theDialog->mSelectedSeed))
			aG.DrawImage(Sexy::IMAGE_SEEDPACKETFLASH, r.x, r.y);
	}
	return true;
}

bool DrawModAlmanacCard(AlmanacDialog* theDialog, Sexy::Graphics* g) {
	const SeedType aSeed = theDialog->mSelectedSeed;
	if (!ModAlmanacOn() || !IsModSeed(aSeed))
		return false;
	PvzpDrawString(g, Plant::GetNameString(aSeed, SeedType::SEED_NONE), 617, 288, Sexy::FONT_DWARVENTODCRAFT18YELLOW, Sexy::Color::White,
		DS_ALIGN_CENTER);
	const std::string aText = ModAlmanacPageText(aSeed);
	const Sexy::Rect aTextRect(485, 309, 258, 205); // ends above the cost line (y 520)
	PvzpDrawStringWrapped(g, aText, aTextRect, Sexy::FONT_BRIANNETOD12, Sexy::Color(40, 50, 90), DS_ALIGN_LEFT);
	static SeedType sMeasured = SeedType::SEED_NONE; // log each page's text height once (Mac check: does it fit?)
	if (sMeasured != aSeed) {
		sMeasured = aSeed;
		const int aHeight = PvzpDrawStringWrappedHelper(g, std::string(PvzpStringTranslate(aText)), aTextRect, Sexy::FONT_BRIANNETOD12,
			Sexy::Color(40, 50, 90), DS_ALIGN_LEFT, false);
		std::fprintf(stderr, "PVZU almanac: page 0x%02x text height %d of %d%s\n", static_cast<int>(aSeed), aHeight, aTextRect.mHeight,
			aHeight > aTextRect.mHeight ? " OVERFLOWS" : "");
	}

	const PlantDefinition& aDef = GetPlantDefinition(aSeed);
	// as the vanilla page: the [COST] / [WAIT_TIME] words come from the game's strings (PvzpReplaceString translates them)
	PvzpDrawStringWrapped(g, PvzpReplaceString(std::format("{{KEYWORD}}{{COST}}:{{STAT}} {}", aDef.mSeedCost), "{COST}", "[COST]"),
		Sexy::Rect(485, 520, 134, 50), Sexy::FONT_BRIANNETOD12, Sexy::Color::White, DS_ALIGN_LEFT);
	PvzpDrawStringWrapped(g, PvzpReplaceString(std::format("{{KEYWORD}}{{WAIT_TIME}}: {{STAT}}{} {}", u::ModAlmanacRechargeSeconds(aDef.mRefreshTime),
		Text("MOD_ALMANAC_SECONDS")), "{WAIT_TIME}", "[WAIT_TIME]"), Sexy::Rect(600, 520, 139, 50), Sexy::FONT_BRIANNETOD12, Sexy::Color(40, 50, 90),
		DS_ALIGN_RIGHT);
	return true;
}

void DrawModAlmanacButton(AlmanacDialog* theDialog, Sexy::Graphics* g) {
	if (!ModAlmanacOn() || !OnPlantsPage(theDialog))
		return;
	const u::AlmanacRect b = u::ModAlmanacButton();
	const int aMouseX = theDialog->mApp->mWidgetManager->mLastMouseX - theDialog->mX;
	const int aMouseY = theDialog->mApp->mWidgetManager->mLastMouseY - theDialog->mY;
	const bool aOver = theDialog->mMouseVisible && u::ModAlmanacButtonHit().Contains(aMouseX, aMouseY);
	// the Index button's art: its curled arrow points back on "First plants", mirrored (forward) on "More plants"
	Sexy::Image* aImage = aOver ? Sexy::IMAGE_ALMANAC_INDEXBUTTONHIGHLIGHT : Sexy::IMAGE_ALMANAC_INDEXBUTTON;
	const bool aBack = theDialog->mPvzuModPage;
	if (aBack)
		g->DrawImage(aImage, b.x, b.y);
	else
		g->DrawImageMirror(aImage, b.x, b.y, true);
	const std::string aLabel = Text(aBack ? "MOD_ALMANAC_BACK" : "MOD_ALMANAC_MORE_PLANTS");
	PvzpDrawString(g, aLabel, b.x + b.w / 2 + (aBack ? 8 : -8), b.y + 19, Sexy::FONT_BRIANNETOD12, Sexy::Color(42, 42, 90), DS_ALIGN_CENTER);
}

bool ModAlmanacHitTest(AlmanacDialog* theDialog, int x, int y, SeedType& theSeed) {
	if (!ModGridShown(theDialog))
		return false;
	theSeed = SeedType::SEED_NONE;
	if (!theDialog->mMouseVisible)
		return true;
	const auto& aSeeds = u::ModPlantSeeds();
	if (const int i = u::ModAlmanacCellAt(x, y, static_cast<int>(aSeeds.size())); i >= 0)
		theSeed = static_cast<SeedType>(aSeeds[i]);
	return true;
}

bool ModAlmanacMouseDown(AlmanacDialog* theDialog, int x, int y) {
	if (!ModAlmanacOn() || !OnPlantsPage(theDialog) || !u::ModAlmanacButtonHit().Contains(x, y))
		return false;
	theDialog->mPvzuModPage = !theDialog->mPvzuModPage;
	theDialog->mApp->PlaySample(Sexy::SOUND_TAP);
	// the first mod plant is shown when the grid opens on a vanilla plant (and the reverse)
	if (theDialog->mPvzuModPage != IsModSeed(theDialog->mSelectedSeed)) {
		theDialog->mSelectedSeed = theDialog->mPvzuModPage && !u::ModPlantSeeds().empty() ? static_cast<SeedType>(u::ModPlantSeeds()[0])
			: SeedType::SEED_PEASHOOTER;
		theDialog->SetupPlant();
	}
	std::fprintf(stderr, "PVZU almanac: %s grid\n", theDialog->mPvzuModPage ? "mod plants" : "vanilla");
	return true;
}

std::string ModAlmanacPageText(SeedType theSeedType) {
	if (!ModAlmanacOn() || !IsModSeed(theSeedType))
		return {};
	std::string aText = Text(u::ModPlantTextKey(theSeedType, "DESCRIPTION"));
	if (const std::string aHow = Text(u::ModPlantTextKey(theSeedType, "HOW_TO_USE")); !aHow.empty())
		aText += std::format("\n{{SHORTLINE}}\n{{KEYWORD}}{}{{NORMAL}} {}", Text("MOD_ALMANAC_HOW_TO_USE"), aHow);
	return aText;
}

std::string ModAlmanacSecondsWord() {
	const std::string aWord = Text("MOD_ALMANAC_SECONDS");
	return aWord.empty() ? std::string("s") : aWord;
}

bool ModAlmanacOverButton(AlmanacDialog* theDialog, int x, int y) {
	return ModAlmanacOn() && OnPlantsPage(theDialog) && u::ModAlmanacButtonHit().Contains(x - theDialog->mX, y - theDialog->mY);
}

// --- lawn hints ---------------------------------------------------------------------------------------------------
void ModHintPlanted(Plant* thePlant) {
	if (!HintsOn() || thePlant == nullptr || thePlant->mBoard == nullptr)
		return;
	Board* aBoard = thePlant->mBoard;
	const int aSeed = thePlant->mPvzu.modSeed;
	// only what the player plants (a packet or a dropped card in the cursor), not plants a level or a rule adds
	const CursorObject* aCursor = aBoard->mCursorObject.get();
	if (aSeed == 0 || aCursor == nullptr || static_cast<int>(aCursor->mType) != aSeed ||
		(aCursor->mCursorType != CursorType::CURSOR_TYPE_PLANT_FROM_BANK && aCursor->mCursorType != CursorType::CURSOR_TYPE_PLANT_FROM_USABLE_COIN))
		return;
	const std::string aShadowText = Text(u::ModHintKey("SHADOW", aSeed));
	const std::string aPlantedText = Text(u::ModHintKey("PLANTED", aSeed));
	const int aThreshold = u::IsShadowPlant(aSeed) ? u::ShadowThreshold(aSeed) : 0;
	const int aPoints = aThreshold > 0 ? ShadowPoints(thePlant, aSeed) : 0;
	const u::PlantedHint aHint = u::ChoosePlantedHint(!aShadowText.empty(), aPoints, aThreshold, !aPlantedText.empty());
	if (aHint == u::PlantedHint::None || !gHintLatch().Take(LevelId(aBoard), aSeed))
		return;
	const bool aShadow = aHint == u::PlantedHint::Shadow;
	ShowHint(aBoard, aSeed, aShadow ? std::format("shadow (points {} of {})", aPoints, aThreshold) : std::string("planted"),
		aShadow ? aShadowText : aPlantedText, MessageStyle::MESSAGE_STYLE_HINT_LONG);
}

void ModHintPlantTapped(Plant* thePlant) {
	if (!HintsOn() || thePlant == nullptr || thePlant->mBoard == nullptr || !thePlant->IsInPlay())
		return;
	Board* aBoard = thePlant->mBoard;
	const int aSeed = thePlant->mPvzu.modSeed;
	bool aShow = u::CannonTapNeedsHint(aSeed, static_cast<int>(thePlant->mState));
	if (!aShow && aSeed == u::kModSeedPrismShroom) {
		const bool aFree = aBoard->HasConveyorBeltSeedBank() || thePlant->mApp->mEasyPlantingCheat;
		aShow = u::PrismTapNeedsHint(aSeed, u::PrismShroomClickable(thePlant->mIsAsleep, thePlant->mShootingCounter),
			aFree || aBoard->CanTakeSunMoney(u::PrismShroomShotCost()));
	}
	if (aShow)
		ShowHint(aBoard, aSeed, "tap", Text(u::ModHintKey("TAP", aSeed)), MessageStyle::MESSAGE_STYLE_HINT_FAST);
}

bool ModHintPlantingRefused(Board* theBoard, PlantingReason theReason) {
	if (!HintsOn() || theBoard == nullptr || !theBoard->mCursorObject)
		return false;
	const int aSeed = static_cast<int>(theBoard->mCursorObject->mType);
	if (!u::RefusalHasOwnHint(aSeed, theReason == PlantingReason::PLANTING_NEEDS_SLEEPING))
		return false;
	const std::string aText = Text(u::ModHintKey("REFUSED", aSeed));
	if (aText.empty())
		return false;
	// as the Coffee Bean message it replaces: once per level (its help index), cleared when a planting succeeds
	if (!theBoard->mHelpDisplayed[AdviceType::ADVICE_PLANTING_NEED_SLEEPING])
		std::fprintf(stderr, "PVZU hint: frame %d seed 0x%02x refused \"%s\"\n", static_cast<int>(theBoard->mMainCounter), aSeed, aText.c_str());
	theBoard->DisplayAdvice(aText, MessageStyle::MESSAGE_STYLE_HINT_FAST, AdviceType::ADVICE_PLANTING_NEED_SLEEPING);
	return true;
}

} // namespace pvzu::engine
