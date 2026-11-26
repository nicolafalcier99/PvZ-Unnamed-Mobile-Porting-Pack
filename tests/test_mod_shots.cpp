// PVZU: Plan 8 part 3a: carriers and looks of the mod projectile types (Mod/Unnamed/ModShots.h) and the new per-object
// state the shooter plants save (Mod/Unnamed/State.h).
#include "doctest/doctest.h"
#include "Mod/GameData.h"
#include "Mod/ModPack.h"
#include "Mod/Unnamed/ModPlantRules.h"
#include "Mod/Unnamed/ModShots.h"
#include "Mod/Unnamed/Rules.h"
#include "Mod/Unnamed/State.h"

#include <cstdlib>
#include <filesystem>
#include <string>

using namespace pvzu;
using namespace pvzu::unnamed;
namespace fs = std::filesystem;

namespace {
void SetModPlantsOn(bool on) {
	GameData::Get().ResetToVanilla();
	if (on) {
		ModPack p;
		p.id = "test-mod-shots";
		p.features["modPlants"] = true;
		std::string err;
		REQUIRE_MESSAGE(GameData::Get().ApplyPack(p, &err), err);
	}
}
} // namespace

TEST_CASE("Mod shots: exe types 0x12, 0x14..0x17, 0x20..0x23 ride on vanilla carriers") {
	CHECK(ModShotCarrier(0) == 0);
	CHECK(ModShotCarrier(12) == 12);
	CHECK(ModShotCarrier(13) == 13);
	CHECK(ModShotCarrier(0x14) == 4);
	CHECK(ModShotCarrier(0x15) == 1);
	CHECK(ModShotCarrier(0x16) == 0);
	CHECK(ModShotCarrier(0x17) == 7);
	CHECK(ModShotCarrier(0x21) == 2);
	CHECK(ModShotCarrier(0x22) == 0);
	CHECK(ModShotCarrier(0x23) == 0);
	CHECK(ModShotCarrier(14) == -1);
	CHECK(ModShotCarrier(0x18) == -1);
	CHECK(ModShotCarrier(0x12) == 9);   // Plan 9 R3: Catapult vase / bug spray ride on the basketball
	CHECK(ModShotCarrier(0x20) == 9);
	CHECK(ModShotCarrier(0x13) == -1);
	CHECK(ModShotCarrier(0x24) == -1);
	CHECK(ModShotCarrier(-1) == -1);
	CHECK(IsModShotType(0x16));
	CHECK(IsModShotType(0x21));
	CHECK(IsModShotType(0x23));
	CHECK_FALSE(IsModShotType(7));
	CHECK(IsModShotType(0x20));
	CHECK(IsModShotType(0x12));
	CHECK_FALSE(IsModShotType(0x13));
	CHECK_FALSE(IsModShotType(0x1f));
}

TEST_CASE("Mod shots: tints only for star kinds and for missing own art; the dusk shard is a plain cabbage") {
	nlohmann::json r = nlohmann::json::object();
	r["modPlants.dreamStarfruit.palette"] = {3383753, 16711680, 12583167, 7833753, 65280};
	SetRules(r);
	SetModPlantsOn(true);
	CHECK(ModShotTint(0, 0) == 0);
	CHECK(ModShotTint(2, 0) == 0);
	CHECK(ModShotTint(0x16, 0) == 0); // 0x959e90: only a star (7) with kind 1..5 is coloured; the rest Color::White
	CHECK(ModShotTint(0x21, 0) == 0); // hook 407: the dusk shard draws IMAGE_REANIM_CABBAGEPULT_CABBAGE, white
	CHECK(ModShotTint(0x22, 0) != 0); // stand-in colours, used only when the pack lacks the shot's own image
	CHECK(ModShotTint(0x23, 0) != 0);
	CHECK(ModShotTint(0x23, 0) != 0);
	CHECK(ModShotTint(0x22, 0) != ModShotTint(0x23, 0));
	CHECK(ModShotTint(7, 1) == 0x33a1c9u); // 0x959cc0 blue
	CHECK(ModShotTint(7, 2) == 0xff0000u);
	CHECK(ModShotTint(7, 5) == 0x00ff00u);
	CHECK(ModShotTint(7, 0) == 0);
	CHECK(ModShotTint(7, 6) == 0);
	SetModPlantsOn(false);
	CHECK(ModShotTint(7, 1) == 0); // vanilla mode: no star kinds
	SetRules(nlohmann::json::object());
}

TEST_CASE("Mod shots: own art of the Primal Repeater peas (hook 405 0xc10b50, loader 0xc10e50)") {
	CHECK(ModShotImageFile(kShotKnockback) == "images/projectileprimitivepea"); // IMAGE_PROJECTILEPRIMITIVEPEA -> [0xc10f4b]
	CHECK(ModShotImageFile(kShotPoison) == "images/ProjectilePoisonPea");       // IMAGE_PROJECTILEPOISONPEA -> [0xc10f47]
	CHECK(ModShotImageFile(kShotPrism).empty()); // the engine's IMAGE_POOLSPARKLY (0x7a0e4a [0x6a75b8])
	CHECK(ModShotImageFile(kShotDusk).empty());  // the carrier's cabbage
	CHECK(ModShotImageFile(0).empty());
	CHECK(ModShotImageFile(7).empty());
	CHECK(ModShotImageFile(0x12).empty());
	CHECK(ModShotImageFile(0x20).empty());
}

TEST_CASE("Mod shots: the pack ships the Primal Repeater pea images") {
	const char* home = std::getenv("HOME");
	const fs::path aPack = std::getenv("PVZU_PACK_DIR") ? fs::path(std::getenv("PVZU_PACK_DIR"))
	                                                   : fs::path(home ? home : "") / "pvz-unnamed-pack";
	if (!fs::exists(aPack / "assets" / "images")) {
		MESSAGE("pack assets not here, skipped: " << aPack.string());
		return;
	}
	for (int aType : {kShotKnockback, kShotPoison})
		CHECK_MESSAGE(fs::exists(aPack / "assets" / (std::string(ModShotImageFile(aType)) + ".png")), aType);
}

TEST_CASE("Mod shots: the Repeater-poison record skips zombies from wave -2 / -3 (0xc10e10)") {
	CHECK(RepeaterPoisonRecordFor(0));
	CHECK(RepeaterPoisonRecordFor(5));
	CHECK(RepeaterPoisonRecordFor(-1));
	CHECK_FALSE(RepeaterPoisonRecordFor(-2));
	CHECK_FALSE(RepeaterPoisonRecordFor(-3));
}

TEST_CASE("Shooter state survives save/load; older blobs read zeros") {
	PlantState p{};
	p.modSeed = 0x7a;
	p.elementBits = 5;
	p.elementMode = 1;
	p.elementCount = 7;
	p.starColour = 3;
	p.cannonAmmo = 0x22;
	auto pb = Encode(p);
	PlantState p2;
	REQUIRE(Decode(pb.data(), pb.size(), p2));
	CHECK(p2 == p);
	REQUIRE(Decode(pb.data(), 32, p2)); // a Plan 8 part 1 blob: modSeed only
	CHECK(p2.modSeed == 0x7a);
	CHECK(p2.elementBits == 0);
	CHECK(p2.elementCount == 0);
	CHECK(p2.cannonAmmo == 0);

	ZombieState z{};
	z.variant = 3;
	z.weakenCs = 400;
	z.repeaterPoisonCs = 1000;
	auto zb = Encode(z);
	ZombieState z2;
	REQUIRE(Decode(zb.data(), zb.size(), z2));
	CHECK(z2 == z);
	REQUIRE(Decode(zb.data(), 24, z2)); // before Plan 8 part 3a
	CHECK(z2.variant == 3);
	CHECK(z2.weakenCs == 0);
	CHECK(z2.repeaterPoisonCs == 0);

	ProjectileState s{};
	s.fireTier = 2;
	s.modType = 0x22;
	s.field80 = 475;
	s.cobAmmo = 0x22;
	auto sb = Encode(s);
	ProjectileState s2;
	REQUIRE(Decode(sb.data(), sb.size(), s2));
	CHECK(s2 == s);
	REQUIRE(Decode(sb.data(), 24, s2)); // before Plan 8 part 3a
	CHECK(s2.fireTier == 2);
	CHECK(s2.modType == 0);
	CHECK(s2.field80 == 0);
	CHECK(s2.cobAmmo == 0);
}
