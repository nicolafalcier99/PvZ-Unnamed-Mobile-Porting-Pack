// PVZU: Plan 9 L3 pure helpers: the right-hold info tooltip (hook 57, weakened line 0x95a1b0), the Sniper head aim
// (hook 13) and target mark, the Element-shroom icons and digit (0x7b1536 / 0x7b158f), the Peashooter layer number
// (0xc05e7c). Evidence: the research notes (l3-visual-details); pack keys in data/rules-mod-visuals.json.
#include "doctest/doctest.h"
#include "Mod/GameData.h"
#include "Mod/ModPack.h"
#include "Mod/Unnamed/ModPlantRules.h"
#include "Mod/Unnamed/ModVisuals.h"
#include "Mod/Unnamed/Rules.h"

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>

using namespace pvzu;
using namespace pvzu::unnamed;
namespace fs = std::filesystem;

namespace {
// Same values as pvz-unnamed-pack/data/rules-mod-visuals.json (the last test compares them with the pack file).
const char* kVisualRulesJson = R"json(
{
	"modVisuals.infoTooltip": 1,
	"modVisuals.infoButtons": 2,
	"modVisuals.infoSkipMode": 21,
	"modVisuals.infoHoldTicks": 40,
	"modVisuals.peashooterLayers": 1,
	"modVisuals.sniperAim": 1,
	"modVisuals.sniperMark": 1,
	"modVisuals.elementIcons": 1,
	"modVisuals.packetPlantCels": 0
}
)json";

void SetFeatures(bool theVisuals, bool theModPlants) {
	GameData::Get().ResetToVanilla();
	if (!theVisuals && !theModPlants)
		return;
	ModPack p;
	p.id = "test-mod-visuals";
	if (theVisuals)
		p.features["modVisuals"] = true;
	if (theModPlants)
		p.features["modPlants"] = true;
	std::string err;
	REQUIRE_MESSAGE(GameData::Get().ApplyPack(p, &err), err);
}

struct VisualsOn {
	VisualsOn() { SetRules(nlohmann::json::parse(kVisualRulesJson)); SetFeatures(true, true); }
	~VisualsOn() { SetRules(nlohmann::json::object()); SetFeatures(false, false); }
};
struct VisualsVanilla {
	VisualsVanilla() { SetRules(nlohmann::json::parse(kVisualRulesJson)); SetFeatures(false, false); }
	~VisualsVanilla() { SetRules(nlohmann::json::object()); }
};

InfoLabels English() {
	InfoLabels l;
	l.helm = "Helm: ";
	l.shield = "Shield: ";
	l.balloon = "Balloon: ";
	l.hp = "HP: ";
	l.butter = "Butter: ";
	l.chill = "Chill: ";
	l.frozen = "Frozen: ";
	l.weakened = "Weakened: ";
	l.cs = "cs";
	return l;
}

constexpr int kPlaying = 3, kInvisighoul = 21, kRight = 2, kLeft = 1;
} // namespace

TEST_CASE_FIXTURE(VisualsOn, "Info tooltip gate: playing, not Invisighoul, right button only (0x955293..0x9552c0)") {
	CHECK(InfoToolTipShown(kPlaying, 0, kRight, 0));
	CHECK_FALSE(InfoToolTipShown(2, 0, kRight, 0));             // not SCENE_PLAYING
	CHECK_FALSE(InfoToolTipShown(kPlaying, kInvisighoul, kRight, 0));
	CHECK_FALSE(InfoToolTipShown(kPlaying, 0, kRight | kLeft, 0)); // == 2, not & 2
	CHECK_FALSE(InfoToolTipShown(kPlaying, 0, 0, 0));
	// port addition (phone): a finger (left button) held still for infoHoldTicks
	CHECK_FALSE(InfoToolTipShown(kPlaying, 0, kLeft, 39));
	CHECK(InfoToolTipShown(kPlaying, 0, kLeft, 40));
	CHECK_FALSE(InfoToolTipShown(kPlaying, kInvisighoul, kLeft, 400));
}

TEST_CASE_FIXTURE(VisualsVanilla, "Info tooltip: never without the feature") {
	CHECK_FALSE(InfoToolTipShown(kPlaying, 0, kRight, 0));
	CHECK_FALSE(InfoToolTipShown(kPlaying, 0, kLeft, 400));
	CHECK_FALSE(PeashooterLayersDrawn(0));
}

TEST_CASE_FIXTURE(VisualsOn, "Zombie info label: body first, two fields per line, the weakened line last (0x955330..0x95a234)") {
	ZombieInfo z;
	z.body = 270;
	z.bodyMax = 270;
	CHECK(ZombieInfoLabel(z, English()) == "270/270 HP: 270/270");

	// Conehead with a chilled, buttered, frozen and weakened status: helm counted while helmType > 0
	z.helmType = 1;
	z.helm = 300;
	z.helmMax = 370;
	z.butterCs = 120;
	z.chillCs = 500;
	z.iceCs = 300;
	z.weakenCs = 380;
	CHECK(ZombieInfoLabel(z, English()) ==
		"270/270 Helm: 300/370\nHP: 570/640 Butter: 120cs\nChill: 500cs Frozen: 300cs\nWeakened: 380cs");

	// the helm already fell off (type 0) but its max is still > 0: shown, not counted in the total
	z.helmType = 0;
	z.helm = 0;
	z.butterCs = z.chillCs = z.iceCs = 0;
	CHECK(ZombieInfoLabel(z, English()) == "270/270 Helm: 0/370\nHP: 270/640 Weakened: 380cs");
}

TEST_CASE_FIXTURE(VisualsOn, "Zombie info label: shield and balloon; balloon HP always in the total (0x9555f2)") {
	ZombieInfo z;
	z.body = 100;
	z.bodyMax = 270;
	z.shieldType = 1;
	z.shield = 1100;
	z.shieldMax = 1100;
	z.flying = 20;
	z.flyingMax = 20;
	CHECK(ZombieInfoLabel(z, English()) == "100/270 Shield: 1100/1100\nBalloon: 20/20 HP: 1220/1390");
	z.shieldType = 0; // the door is gone: its HP leaves the total, its line stays
	CHECK(ZombieInfoLabel(z, English()) == "100/270 Shield: 1100/1100\nBalloon: 20/20 HP: 120/1390");
}

TEST_CASE("Plant info label: health hp/max (0xc05fea)") {
	CHECK(PlantInfoLabel(150, 300, "Health: ") == "Health: 150/300");
}

TEST_CASE_FIXTURE(VisualsOn, "Sniper head: angle atan2(plantY - zombieY, |dx|), rotation about (40, 40) (0x7a7ef0, 0x7aa500)") {
	CHECK(SniperHeadAngle(100, 200, 500, 200) == doctest::Approx(0.0f));
	CHECK(SniperHeadAngle(100, 200, 200, 100) == doctest::Approx(0.785398f)); // zombie up-right: positive, aims up
	CHECK(SniperHeadAngle(100, 200, 0, 100) == doctest::Approx(0.785398f));   // |dx|: a zombie behind aims the same way
	CHECK(SniperHeadAngle(100, 200, 200, 300) == doctest::Approx(-0.785398f));

	const AimMatrix m = SniperHeadMatrix(0.5f);
	const float c = std::cos(0.5f), s = std::sin(0.5f);
	CHECK(m.m00 == doctest::Approx(c));
	CHECK(m.m01 == doctest::Approx(s));
	CHECK(m.m10 == doctest::Approx(-s));
	CHECK(m.m11 == doctest::Approx(c));
	CHECK(m.m02 == doctest::Approx(40.0f * (1.0f - c - s)));
	CHECK(m.m12 == doctest::Approx(40.0f * (1.0f - c + s)));
	// (40, 40) stays where it is
	CHECK(m.m00 * 40 + m.m01 * 40 + m.m02 == doctest::Approx(40.0f));
	CHECK(m.m10 * 40 + m.m11 * 40 + m.m12 == doctest::Approx(40.0f));
	// the muzzle side (1, 0) goes up (y < 0) for a positive angle
	CHECK(m.m10 * 1 < 0.0f);
}

TEST_CASE_FIXTURE(VisualsOn, "Sniper head and mark: only while aiming at a target, on the lawn (0x7a7ea9..0x7a7ed5, 0xc10f70)") {
	CHECK(SniperAims(0x78, 0x22, 7, true));
	CHECK_FALSE(SniperAims(0x78, 0, 7, true));    // idle
	CHECK_FALSE(SniperAims(0x78, 0x22, 0, true)); // no target
	CHECK_FALSE(SniperAims(0x78, 0x22, 7, false)); // dead, squished, asleep or off the board
	CHECK_FALSE(SniperAims(0x2b, 0x22, 7, true)); // not the Sniper
	CHECK(SniperMarkOffsetX() == 20);
	CHECK(SniperMarkOffsetY() == 0);
}

TEST_CASE_FIXTURE(VisualsVanilla, "Sniper looks: nothing without modPlants") {
	CHECK_FALSE(SniperAims(0x78, 0x22, 7, true));
	CHECK_FALSE(ElementIconsDrawn(0x7a, 3, false));
}

TEST_CASE_FIXTURE(VisualsOn, "Element-shroom icons: bit 4 left .. bit 1 right; ice = snow pea, fire = fire pea 20 left 25 up (0x7b158f)") {
	CHECK_FALSE(ElementIconsDrawn(0x7a, 0, false)); // no element gained yet
	CHECK(ElementIconsDrawn(0x7a, 1, false));
	CHECK_FALSE(ElementIconsDrawn(0x7a, 1, true)); // squished
	CHECK_FALSE(ElementIconsDrawn(0x2a, 1, false)); // vanilla Gloom-shroom

	const auto ice = ElementShroomIcons(0b111, 80, 80);
	for (int i = 0; i < 3; i++) {
		CHECK(ice[i].ice);
		CHECK(ice[i].x == 3 + 20 * i);
		CHECK(ice[i].y == 80 - 25);
	}
	const auto mixed = ElementShroomIcons(0b100, 80, 80); // oldest ice, two newer fire
	CHECK(mixed[0].ice);
	CHECK(mixed[0].x == 3);
	CHECK_FALSE(mixed[1].ice);
	CHECK(mixed[1].x == 3 + 20 - 20);
	CHECK(mixed[1].y == 80 - 25 - 25);
	CHECK_FALSE(mixed[2].ice);
	CHECK(mixed[2].x == 3 + 40 - 20);
}

TEST_CASE_FIXTURE(VisualsOn, "Element-shroom digit: countdown/100 + 1 at (width/2 - 6, 25), 1..9 only (0x7b1536)") {
	CHECK(ElementShroomCountdownDigit(800) == 9 - 0); // 800/100 + 1 = 9
	CHECK(ElementShroomCountdownDigit(899) == 9);
	CHECK(ElementShroomCountdownDigit(900) == 0); // 10: not drawn
	CHECK(ElementShroomCountdownDigit(0) == 1);
	CHECK(ElementDigitX(80) == 34);
	CHECK(ElementDigitY() == 25);
}

TEST_CASE_FIXTURE(VisualsOn, "Peashooter layer number: raw seed 0 only, at (2, 70) (0xc05e7c)") {
	CHECK(PeashooterLayersDrawn(0));
	CHECK_FALSE(PeashooterLayersDrawn(0x50)); // Shadow Peashooter (its own seed number)
	CHECK_FALSE(PeashooterLayersDrawn(7));
}

// Phone report 2026-11-23 (Rewind Grass "2 different images"): the exe jumps over DrawSeedPacket's static
// packet_plants.png cels (0x4873e0: e9 6e 02 00 00 = jmp 0x487653, where vanilla has `cmp edi,4; jne`), so every
// packet draws its plant from the reanim, like the HUD card and the lawn. The pack's own cel 4 (a blue Blover) is unused.
TEST_CASE_FIXTURE(VisualsOn, "Packets: Unnamed never draws the static packet_plants cels (0x4873e0 jmp 0x487653)") {
	CHECK_FALSE(PacketPlantCelsUsed());
}

TEST_CASE_FIXTURE(VisualsVanilla, "Packets: vanilla keeps its packet_plants cels") {
	CHECK(PacketPlantCelsUsed());
}

TEST_CASE("Pack: rules-mod-visuals.json has the same values as this test, and the feature is on") {
	const char* aDir = std::getenv("PVZU_PACK_DIR");
	if (aDir == nullptr) {
		MESSAGE("PVZU_PACK_DIR not set: pack file not checked");
		return;
	}
	std::ifstream aIn(fs::path(aDir) / "data" / "rules-mod-visuals.json");
	REQUIRE(aIn.good());
	const nlohmann::json aPack = nlohmann::json::parse(aIn);
	CHECK(aPack == nlohmann::json::parse(kVisualRulesJson));
	std::ifstream aMod(fs::path(aDir) / "mod.json");
	REQUIRE(aMod.good());
	const nlohmann::json aModJson = nlohmann::json::parse(aMod);
	CHECK(aModJson["features"].value("modVisuals", false));
}
