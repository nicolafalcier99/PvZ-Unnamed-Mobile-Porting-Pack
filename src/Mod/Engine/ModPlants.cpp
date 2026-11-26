#include "Mod/Engine/ModPlants.h"
#include "Mod/Engine/TestCheats.h" // PVZU Plan 9 C1

#include "Mod/CmdLine.h"
#include "Mod/Engine/ModPlantSites.h"
#include "Mod/Engine/ModAlmanac.h" // modAlmanac hints
#include "Mod/Engine/ModPlantsShooters.h" // Plan 8 part 3a
#include "Mod/Engine/ModPlantsShadow.h" // Plan 8 part 3c: shadow colour
#include "Mod/Engine/ModModes.h" // SeedTypeByName (-pvzu-dev-bank / -pvzu-dev-put)
#include "Mod/Engine/Startup.h"
#include "Mod/GameData.h"
#include "Mod/ModPack.h"
#include "Mod/Unnamed/DayPlants.h"
#include "Mod/Unnamed/ModPlantRules.h"
#include "Mod/Unnamed/ModPlants.h"
#include "Mod/Unnamed/Options.h" // CardLatch (the options panel's CARD row)
#include "Mod/Unnamed/Rules.h"

#include "LawnApp.h"
#include "Resources.h"
#include "Lawn/Board.h"
#include "Lawn/CursorObject.h"
#include "Lawn/GridItem.h"
#include "Lawn/Plant.h"
#include "Lawn/SeedPacket.h"
#include "Lawn/System/PlayerInfo.h"
#include "Lawn/Widget/SeedChooserScreen.h"
#include "PvzpLib/PvzpCommon.h"
#include "PvzpLib/PvzpDebug.h"
#include "graphics/Graphics.h"
#include "graphics/MemoryImage.h"
#include "imagelib/ImageLib.h"
#include "misc/SexyMatrix.h"

#include <algorithm>
#include <array>
#include <format>
#include <memory>

namespace pvzu::engine {

namespace u = pvzu::unnamed;

static_assert(u::kAllSeedTypes == sizeof(SeedChooserScreen::mChosenSeeds) / sizeof(ChosenSeed),
	"SeedChooserScreen::mChosenSeeds must hold every mod seed");
static_assert(u::kVanillaChooserSeeds == SeedType::NUM_SEEDS_IN_CHOOSER);
static_assert(u::kImitaterSeed == SeedType::SEED_IMITATER && u::kSproutSeed == SeedType::SEED_SPROUT);

namespace {
constexpr int kModSlots = u::kModSeedEnd - u::kFirstModSeed;
std::array<PlantDefinition, kModSlots> gDefs{};
std::array<bool, kModSlots> gHasDef{};
bool gOn = false;

int Slot(SeedType theSeedType) { return static_cast<int>(theSeedType) - u::kFirstModSeed; }

const u::ModPlant* Row(SeedType theSeedType) { return gOn ? u::FindModPlant(theSeedType) : nullptr; }

std::string Hex(int theSeed) { return std::format("0x{:02x}", theSeed); }

const char* EngineName(int theSeed) {
	const auto& aPlants = GameData::Get().Vanilla().plants;
	return theSeed >= 0 && static_cast<size_t>(theSeed) < aPlants.size() ? aPlants[theSeed].name.c_str() : "?";
}

void Log(const Board* theBoard, const std::string& theLine) {
	u::LogPlantAbility(theBoard ? static_cast<int>(theBoard->mMainCounter) : -1, theLine);
	std::fprintf(stderr, "PVZU: %s\n", theLine.c_str());
}

Sexy::Color Tint(SeedType theSeedType) {
	const u::ModPlant* p = Row(theSeedType);
	return p ? Sexy::Color(p->tint[0], p->tint[1], p->tint[2], p->tint[3]) : Sexy::Color(255, 255, 255, 255);
}

// The colour a tinted mod seed draws with on top of what the Graphics already colourises with (greying, darkening).
void ApplyTint(Sexy::Graphics* g, SeedType theSeedType) {
	const Sexy::Color aTint = Tint(theSeedType);
	const Sexy::Color aBase = g->GetColorizeImages() ? g->mColor : Sexy::Color(255, 255, 255, 255);
	g->SetColorizeImages(true);
	g->SetColor(Sexy::Color(aBase.mRed * aTint.mRed / 255, aBase.mGreen * aTint.mGreen / 255, aBase.mBlue * aTint.mBlue / 255,
		aBase.mAlpha * aTint.mAlpha / 255));
}

std::unique_ptr<Sexy::MemoryImage> LoadPacketCell() {
	const std::filesystem::path aFile = UnnamedPackDir() / "assets" / "images" / "seeds.png";
	std::unique_ptr<ImageLib::Image> aSheet(ImageLib::GetImage(aFile.string(), false));
	constexpr int kCells = 13; // Unnamed's resources.xml: SEEDS cols="13" (data/overlay-skip-always.txt)
	const int aCell = u::RuleInt("modPlants.packetCell", 12);
	if (!aSheet || aSheet->mWidth % kCells != 0 || aCell < 0 || aCell >= kCells) {
		PvzpLogLn("PVZU: mod-plant packet back {} not usable; vanilla packet back instead", aFile.string());
		return nullptr;
	}
	const int aW = aSheet->mWidth / kCells, aH = aSheet->mHeight;
	auto aImage = std::make_unique<Sexy::MemoryImage>();
	aImage->mBits = std::make_unique<uint32_t[]>(static_cast<size_t>(aW) * aH + 1);
	aImage->mWidth = aW;
	aImage->mHeight = aH;
	aImage->mHasTrans = true;
	aImage->mHasAlpha = true;
	for (int y = 0; y < aH; ++y)
		std::copy_n(aSheet->mBits.get() + static_cast<size_t>(y) * aSheet->mWidth + aCell * aW, aW,
			aImage->mBits.get() + static_cast<size_t>(y) * aW);
	aImage->mBits[static_cast<size_t>(aW) * aH] = Sexy::MEMORYCHECK_ID;
	return aImage;
}

int PanelX() { return Sexy::IMAGE_SEEDCHOOSER_BACKGROUND ? Sexy::IMAGE_SEEDCHOOSER_BACKGROUND->mWidth + 4 : 469; }

u::CardLatch gCard; // the "card" setting, and the value the chooser in use was made with
bool gCheatCard = false; // PVZU Plan 9 C1: test cheat "plant anything" as the chooser in use was made with

bool CardFlag() {
	static const bool aFlag = CmdFlag("-pvzu-card");
	return aFlag;
}
} // namespace

void SetUpModPlants(const ModPack* thePack) {
	gOn = false;
	gCard = u::CardLatch(StartupSettings().card);
	gHasDef.fill(false);
	u::ClearModPlants();
	if (thePack == nullptr || !GameData::Get().Feature("modPlants"))
		return;
	std::vector<std::string> aNames;
	for (const auto& p : GameData::Get().Vanilla().plants)
		aNames.push_back(p.name);
	std::string aErr;
	if (!u::SetModPlants(thePack->modPlants, aNames, &aErr)) {
		PvzpLogLn("PVZU: mod plants ignored: {}", aErr);
		return;
	}
	for (int aSeed : u::ModPlantSeeds()) {
		const u::ModPlant& p = *u::FindModPlant(aSeed);
		PlantDefinition d = GetPlantDefinition(static_cast<SeedType>(p.base)); // art: the base plant's reanim
		d.mSeedType = static_cast<SeedType>(aSeed);
		d.mSeedCost = p.seedCost;
		d.mRefreshTime = p.refreshTime;
		d.mLaunchRate = p.launchRate;
		d.mSubClass = p.shooter ? PlantSubClass::SUBCLASS_SHOOTER : PlantSubClass::SUBCLASS_NORMAL;
		d.mPlantName = p.nameEn.c_str(); // logs only; the shown name is ModPlantNameString
		gDefs[aSeed - u::kFirstModSeed] = d;
		gHasDef[aSeed - u::kFirstModSeed] = true;
	}
	gOn = !u::ModPlantSeeds().empty();
	PvzpLogLn("PVZU: {} mod plants, CARD {}", u::ModPlantSeeds().size(), CardOn() ? "on" : "off");
}

bool ModPlantsOn() { return gOn; }

bool CardOn() { return gOn && (gCard.InEffect() || CardFlag() || gCheatCard); } // PVZU Plan 9 C1: + test cheat

bool CardSetting() { return gCard.Setting() || CardFlag(); }
bool CardPending() { return gCard.Setting() != gCard.InEffect(); }

bool ImitaterOwned(LawnApp* theApp) {
	return theApp && theApp->mPlayerInfo && theApp->mPlayerInfo->mPurchases[StoreItem::STORE_ITEM_PLANT_IMITATER] > 0;
}

void SetCard(LawnApp* theApp, bool theOn, bool theSave) {
	gCard.Set(theOn, theApp && theApp->mSeedChooserScreen != nullptr);
	if (theSave)
		SaveCardSetting(theOn);
	PvzpLogLn("PVZU: CARD setting {} (in effect {})", theOn ? "on" : "off", CardOn() ? "on" : "off");
}

void ModChooserOpens() {
	gCard.ChooserOpens();
	gCheatCard = TestCheatAllSeeds(); // PVZU Plan 9 C1: latched like CARD (the chooser lays out its seeds once)
}

bool IsModSeed(SeedType theSeedType) { return Row(theSeedType) != nullptr; }

SeedType ModSeedOf(const Plant* thePlant) {
	const SeedType aMod = ModSeed(thePlant);
	return aMod != SeedType::SEED_NONE ? aMod : (thePlant ? thePlant->mSeedType : SeedType::SEED_NONE);
}

const PlantDefinition* ModPlantDefinition(SeedType theSeedType) {
	if (!u::IsModSeedRange(theSeedType))
		return nullptr;
	if (gOn && gHasDef[Slot(theSeedType)])
		return &gDefs[Slot(theSeedType)];
	return &GetPlantDefinition(static_cast<SeedType>(u::ModBaseSeed(theSeedType))); // empty slot / no table: the base
}

std::optional<std::string> ModPlantNameString(SeedType theSeedType) {
	if (const u::ModPlant* p = Row(theSeedType))
		return u::ModPlantName(*p, StartupSettings().language);
	return std::nullopt;
}

std::optional<std::string> ModPlantToolTip(SeedType theSeedType) {
	if (const u::ModPlant* p = Row(theSeedType))
		return u::ModPlantToolTip(*p, StartupSettings().language);
	return std::nullopt;
}

std::optional<bool> ModHasSeedType(LawnApp* theApp, SeedType theSeedType) {
	if (!gOn)
		return std::nullopt;
	return u::CardHasSeedType(theSeedType, CardOn(), ImitaterOwned(theApp) || TestCheatAllSeeds()); // PVZU Plan 9 C1: + test cheat
}

SeedType ModPlantingSeed(SeedType theCursorSeed) {
	if (!u::IsModSeedRange(theCursorSeed))
		return theCursorSeed;
	const SeedType aBase = static_cast<SeedType>(u::ModBaseSeed(theCursorSeed));
	return Plant::IsUpgrade(aBase) ? SeedType::SEED_PEASHOOTER : aBase;
}

SeedType ModCostSeed(Board* theBoard, SeedType thePlantingSeed) {
	const SeedType aCursor = theBoard->mCursorObject->mType;
	return u::IsModSeedRange(aCursor) ? aCursor : thePlantingSeed;
}

SeedType ModAddPlantBase(SeedType theSeedType) { return static_cast<SeedType>(u::ModBaseSeed(theSeedType)); }

void ModPlantPlanted(Plant* thePlant, SeedType theAskedSeed) {
	if (thePlant == nullptr || !u::IsModSeedRange(theAskedSeed))
		return;
	thePlant->mPvzu.modSeed = theAskedSeed;
	const u::ModPlant* p = Row(theAskedSeed);
	Log(thePlant->mBoard, std::format("modPlant planted {} {} base {} col {} row {} hp {}", Hex(theAskedSeed),
		p ? p->nameEn : std::string("(no row)"), EngineName(thePlant->mSeedType), thePlant->mPlantCol, thePlant->mRow,
		thePlant->mPlantHealth));
	ModPlantOnPlanted(thePlant);
	ModHintPlanted(thePlant); // feature modAlmanac: a once-per-level hint when the player plants it
}

void ModSproutToImitater(SeedType& theSeedType, SeedType& theImitaterType, bool theFromSeedBank) {
	if (!gOn)
		return;
	const int aRoll = u::SproutImitaterRoll(theSeedType, theImitaterType, theFromSeedBank);
	if (aRoll <= 0)
		return;
	const SeedType aWas = theSeedType;
	theImitaterType = static_cast<SeedType>(Sexy::Rand(aRoll));
	theSeedType = SeedType::SEED_IMITATER;
	Log(nullptr, std::format("modPlant sprout {} from {} -> Imitater copying {} (Rand({}))", EngineName(aWas),
		theFromSeedBank ? "bank" : "packet", EngineName(theImitaterType), aRoll));
}

SeedType ModImitaterOfImitater(SeedType theCopy) {
	if (!gOn || theCopy != SeedType::SEED_IMITATER)
		return theCopy;
	const SeedType aPick = static_cast<SeedType>(u::ImitaterOfImitaterCopy(theCopy, [](int n) { return Sexy::Rand(n); }));
	Log(nullptr, std::format("modPlant imitater copying an Imitater -> copies {}", EngineName(aPick)));
	return aPick;
}

Sexy::Color ModPlantColor(const Plant* thePlant, const Sexy::Color& theVanilla) {
	if (Sexy::Color aShadow; ModPlantShadowColor(thePlant, aShadow)) // hook 313 -> 0xc00e00 (Plan 8 part 3c)
		return aShadow;
	const SeedType aMod = ModSeed(thePlant);
	if (Sexy::Color aShooter = theVanilla; gOn && ShooterPlantColor(thePlant, aShooter)) // Plan 8 part 3a
		return aShooter;
	return aMod != SeedType::SEED_NONE && Row(aMod) ? Tint(aMod) : theVanilla;
}

bool DrawModSeedType(Sexy::Graphics* g, SeedType theSeedType, int theDrawVariation, float x, float y) {
	if (!u::IsModSeedRange(theSeedType))
		return false;
	Sexy::Graphics aG(*g);
	ApplyTint(&aG, theSeedType);
	Plant::DrawSeedType(&aG, static_cast<SeedType>(u::ModBaseSeed(theSeedType)), SeedType::SEED_NONE,
		static_cast<DrawVariation>(theDrawVariation), x, y);
	return true;
}

bool ApplyModTint(Sexy::Graphics* g, SeedType theSeedType) {
	if (!u::IsModSeedRange(theSeedType))
		return false;
	ApplyTint(g, theSeedType);
	return true;
}

Sexy::Image* ModPacketImage(SeedType theSeedType) {
	if (!gOn || !u::IsModSeedRange(theSeedType))
		return nullptr;
	// Never destroyed: a MemoryImage unregisters from the app when deleted, and the app is gone at static exit.
	static Sexy::MemoryImage* sCell = LoadPacketCell().release();
	return sCell;
}

int ChooserSeedEnd() { return u::ChooserSeedEnd(CardOn()); }

bool ModChooserPosition(SeedChooserScreen* theChooser, int theSeed, int& x, int& y) {
	if (!CardOn())
		return false;
	const int aSlot = u::CardChooserSlot(theSeed);
	if (aSlot < 0)
		return false;
	const u::ChooserSpot aSpot = u::CardChooserSpot(aSlot, theChooser->Has7Rows());
	x = PanelX() + aSpot.x;
	y = aSpot.y;
	return true;
}

void DrawModChooserPanel(SeedChooserScreen* theChooser, Sexy::Graphics* g) {
	if (!CardOn() || Sexy::IMAGE_SEEDCHOOSER_BACKGROUND == nullptr)
		return;
	Sexy::Image* aBack = Sexy::IMAGE_SEEDCHOOSER_BACKGROUND;
	const int aCount = u::CardChooserSlotCount();
	const int aRows = (aCount + u::kCardChooserColumns - 1) / u::kCardChooserColumns;
	const bool a7Rows = theChooser->Has7Rows();
	const int aX = PanelX(), aTop = 87;
	const int aBottom = u::CardChooserSpot((aRows - 1) * u::kCardChooserColumns, a7Rows).y + SEED_PACKET_HEIGHT + 10;
	constexpr int kEdge = 16; // the back's bottom rim, drawn under the last row
	{
		Sexy::Graphics aG(*g);
		aG.ClipRect(aX, aTop, aBack->mWidth, aBottom - aTop);
		aG.DrawImage(aBack, aX, aTop);
	}
	g->DrawImage(aBack, aX, aBottom, Sexy::Rect(0, aBack->mHeight - kEdge, aBack->mWidth, kEdge));
	const std::string aTitle = StartupSettings().language == "zh" ? "更多植物" : "MORE PLANTS";
	PvzpDrawString(g, aTitle, aX + aBack->mWidth / 2, 110, Sexy::FONT_DWARVENTODCRAFT18YELLOW, Sexy::Color::White, DS_ALIGN_CENTER);

	for (int aSlot = 0; aSlot < aRows * u::kCardChooserColumns; ++aSlot) {
		const u::ChooserSpot aSpot = u::CardChooserSpot(aSlot, a7Rows);
		SeedType aSeed = SeedType::SEED_NONE;
		for (int s = u::kCardFirstExtraSeed; s < u::kModSeedEnd && aSeed == SeedType::SEED_NONE; ++s)
			if (u::CardChooserSlot(s) == aSlot)
				aSeed = static_cast<SeedType>(s);
		if (aSeed != SeedType::SEED_NONE && theChooser->mApp->HasSeedType(aSeed)) {
			if (theChooser->mChosenSeeds[aSeed].mSeedState != ChosenSeedState::SEED_IN_CHOOSER)
				DrawSeedPacket(g, aX + aSpot.x, aSpot.y, aSeed, SeedType::SEED_NONE, 0, 55, true, false);
		} else {
			g->DrawImage(Sexy::IMAGE_SEEDPACKETSILHOUETTE, aX + aSpot.x, aSpot.y);
		}
	}
}

void ModPlantsDevProfile(LawnApp* theApp) {
	if (theApp->mPlayerInfo && CmdFlag("-pvzu-dev-imitater")) {
		theApp->mPlayerInfo->mPurchases[StoreItem::STORE_ITEM_PLANT_IMITATER] = 1;
		std::fprintf(stderr, "PVZU dev: profile owns the Imitater\n");
	}
}

namespace {
bool PlantForTest(Board* theBoard, SeedType theSeed) {
	const SeedType aPlanting = ModPlantingSeed(theSeed);
	for (int aCol = 0; aCol < MAX_GRID_SIZE_X; ++aCol) {
		for (int aRow = 0; aRow < MAX_GRID_SIZE_Y; ++aRow) {
			if (theBoard->GetTopPlantAt(aCol, aRow, PlantPriority::TOPPLANT_ANY) || theBoard->GetGraveStoneAt(aCol, aRow) ||
				theBoard->CanPlantAt(aCol, aRow, SeedType::SEED_PEASHOOTER) != PlantingReason::PLANTING_OK)
				continue;
			// What the base needs under it: a grave (Grave Buster), a sleeping mushroom (Coffee Bean).
			if (aPlanting == SeedType::SEED_GRAVEBUSTER)
				theBoard->AddAGraveStone(aCol, aRow);
			if (aPlanting == SeedType::SEED_INSTANT_COFFEE)
				theBoard->AddPlant(aCol, aRow, SeedType::SEED_PUFFSHROOM, SeedType::SEED_NONE);
			if (theBoard->CanPlantAt(aCol, aRow, aPlanting) != PlantingReason::PLANTING_OK)
				continue;
			theBoard->AddPlant(aCol, aRow, theSeed, SeedType::SEED_NONE);
			return true;
		}
	}
	return false;
}
} // namespace

void ModPlantsDevTick(Board* theBoard, int thePlayTicks) {
	static const int aPlantAt = CmdOptionInt("-pvzu-dev-mod-plants", -1);
	static const int aListAt = CmdOptionInt("-pvzu-dev-list-plants", -1);
	static const int aBankFrom = CmdOptionInt("-pvzu-dev-mod-bank", -1);
	ShootersDevTick(theBoard, thePlayTicks); // Plan 8 part 3a: -pvzu-dev-shooters / -pvzu-dev-cannon
	// Phone bugs 2026-11-23 (Cob Cannon, Magic Devourer): -pvzu-dev-bank SEED,0x5b,... sets the packets from slot 0;
	// -pvzu-dev-put SEED:col:row,...,GRAVE:col:row lays out the lawn at play tick 1 (SEED = engine name or 0x.. mod seed);
	// -pvzu-dev-cob-log <every> logs each cob / Loaded Cannon (state, countdown) and the cursor every <every> ticks.
	if (thePlayTicks == 1) {
		static const std::string aBank = CmdOption("-pvzu-dev-bank").value_or("");
		int aSlot = 0;
		for (size_t aPos = 0; aPos < aBank.size() && aSlot < theBoard->mSeedBank->mNumPackets;) {
			const size_t aEnd = std::min(aBank.find(',', aPos), aBank.size());
			if (const int aSeed = SeedTypeByName(aBank.substr(aPos, aEnd - aPos)); aSeed >= 0)
				theBoard->mSeedBank->mSeedPackets[aSlot++].SetPacketType(static_cast<SeedType>(aSeed), SeedType::SEED_NONE);
			aPos = aEnd + 1;
		}
		static const std::string aPut = CmdOption("-pvzu-dev-put").value_or("");
		for (size_t aPos = 0; aPos < aPut.size();) {
			const size_t aEnd = std::min(aPut.find(',', aPos), aPut.size());
			const std::string aItem = aPut.substr(aPos, aEnd - aPos);
			aPos = aEnd + 1;
			const size_t c1 = aItem.find(':'), c2 = c1 == std::string::npos ? c1 : aItem.find(':', c1 + 1);
			if (c2 == std::string::npos)
				continue;
			const int aCol = std::atoi(aItem.c_str() + c1 + 1), aRow = std::atoi(aItem.c_str() + c2 + 1);
			if (aItem.compare(0, c1, "GRAVE") == 0) {
				if (!theBoard->GetGraveStoneAt(aCol, aRow))
					theBoard->AddAGraveStone(aCol, aRow);
			} else if (const int aSeed = SeedTypeByName(aItem.substr(0, c1)); aSeed >= 0) {
				theBoard->AddPlant(aCol, aRow, static_cast<SeedType>(aSeed), SeedType::SEED_NONE);
			}
			Log(theBoard, std::format("dev put {}", aItem));
		}
	}
	if (static const int aCobLog = CmdOptionInt("-pvzu-dev-cob-log", 0); aCobLog > 0 && thePlayTicks % aCobLog == 0) {
		for (Plant* aPlant : theBoard->mPlants)
			if (!aPlant->mDead && aPlant->mSeedType == SeedType::SEED_COBCANNON)
				Log(theBoard, std::format("dev cob col {} row {} mod {} state {} countdown {}", aPlant->mPlantCol, aPlant->mRow,
					Hex(aPlant->mPvzu.modSeed), static_cast<int>(aPlant->mState), aPlant->mStateCountdown));
		Log(theBoard, std::format("dev frame {} cursor type {} seed {}", theBoard->mApp->mUpdateCount, static_cast<int>(theBoard->mCursorObject->mCursorType),
			static_cast<int>(theBoard->mCursorObject->mType)));
	}
	if (thePlayTicks == 1 && aBankFrom >= 0) {
		const auto& aSeeds = u::ModPlantSeeds();
		for (int i = 0; i < theBoard->mSeedBank->mNumPackets && aBankFrom + i < static_cast<int>(aSeeds.size()); ++i)
			theBoard->mSeedBank->mSeedPackets[i].SetPacketType(static_cast<SeedType>(aSeeds[aBankFrom + i]), SeedType::SEED_NONE);
		Log(theBoard, std::format("modPlant dev bank: mod plants from #{}", aBankFrom));
	}
	if (thePlayTicks == aPlantAt) {
		Log(theBoard, std::format("modPlant test: {} mod plants, CARD {}", u::ModPlantSeeds().size(), CardOn() ? "on" : "off"));
		for (int aSeed : u::ModPlantSeeds())
			if (!PlantForTest(theBoard, static_cast<SeedType>(aSeed)))
				Log(theBoard, std::format("modPlant test: no square for {}", Hex(aSeed)));
	}
	// -pvzu-dev-imitater-of-imitater <tick>: an Imitater copying an Imitater (a Sprout coin's Rand(53) = 48) on (2, 2),
	// then every 50 ticks what stands there (does it end as a plant, or morph into the same Imitater again?)
	static const int aImitaterAt = CmdOptionInt("-pvzu-dev-imitater-of-imitater", -1);
	if (aImitaterAt > 0 && thePlayTicks == aImitaterAt) {
		Plant* aPlant = theBoard->AddPlant(2, 2, SeedType::SEED_IMITATER, SeedType::SEED_IMITATER);
		Log(theBoard, std::format("modPlant dev imitater-of-imitater planted at 2,2: seed {} copy {}", EngineName(aPlant->mSeedType),
			EngineName(aPlant->mImitaterType)));
	}
	if (aImitaterAt > 0 && thePlayTicks > aImitaterAt && (thePlayTicks - aImitaterAt) % 50 == 0)
		if (Plant* aPlant = theBoard->GetTopPlantAt(2, 2, PlantPriority::TOPPLANT_ANY))
			Log(theBoard, std::format("modPlant dev imitater-of-imitater t+{}: 2,2 holds {} copy {} state {}", thePlayTicks - aImitaterAt,
				EngineName(aPlant->mSeedType), EngineName(aPlant->mImitaterType), static_cast<int>(aPlant->mState)));
	if (thePlayTicks == aListAt) {
		int aMods = 0;
		for (Plant* aPlant : theBoard->mPlants) {
			if (aPlant->mDead)
				continue;
			const SeedType aMod = ModSeed(aPlant);
			if (aMod != SeedType::SEED_NONE)
				aMods++;
			Log(theBoard, std::format("modPlant list col {} row {} seed {} base {} mod {} name \"{}\" cost {}", aPlant->mPlantCol,
				aPlant->mRow, Hex(ModSeedOf(aPlant)), EngineName(aPlant->mSeedType),
				aMod != SeedType::SEED_NONE ? Hex(aMod) : std::string("-"), Plant::GetNameString(ModSeedOf(aPlant)),
				Plant::GetCost(ModSeedOf(aPlant))));
		}
		std::string aBank;
		for (int i = 0; i < theBoard->mSeedBank->mNumPackets; ++i)
			aBank += (i ? "," : "") + Hex(theBoard->mSeedBank->mSeedPackets[i].mPacketType);
		Log(theBoard, std::format("modPlant list: {} mod plants on the lawn; bank {}", aMods, aBank));
		std::string aNotRec; // the chooser's "not recommended" flags (bit 0 = sleeps by day) of the mod mushrooms
		for (int aSeed : {u::kModSeedGoldenSunShroom, u::kModSeedHallucinationShroom, u::kModSeedPrismShroom, u::kModSeedElementShroom})
			aNotRec += std::format(" {}={}", Hex(aSeed), theBoard->SeedNotRecommendedForLevel(static_cast<SeedType>(aSeed)));
		Log(theBoard, std::format("modPlant list: night={} notRecommended{}", theBoard->StageIsNight() ? 1 : 0, aNotRec));
	}
}

} // namespace pvzu::engine
