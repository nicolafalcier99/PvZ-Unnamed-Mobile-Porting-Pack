// PVZU: Unnamed changes to the vanilla mini-games, puzzles and survival modes (the research notes (modes-ui-shop) 2.1, 2.2,
// 9, Final table A; exe reads in pvz-unnamed-pack/SOURCES-modes-vanilla.md). The rules come from a fixture with the same
// numbers as pvz-unnamed-pack/data/rules-modes-vanilla.json (the last test compares the two). Without rules (vanilla)
// every function gives the vanilla answer.
#include "doctest/doctest.h"
#include "ConstEnums.h"
#include "Mod/Unnamed/ModesVanilla.h"
#include "Mod/Unnamed/Rules.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace pvzu::unnamed;

namespace {

const std::vector<std::string> kZombieNames = {
	"NORMAL", "FLAG", "TRAFFIC_CONE", "POLEVAULTER", "PAIL", "NEWSPAPER",
	"DOOR", "FOOTBALL", "DANCER", "BACKUP_DANCER", "DUCKY_TUBE", "SNORKEL",
	"ZAMBONI", "BOBSLED", "DOLPHIN_RIDER", "JACK_IN_THE_BOX", "BALLOON", "DIGGER",
	"POGO", "YETI", "BUNGEE", "LADDER", "CATAPULT", "GARGANTUAR",
	"IMP", "BOSS", "PEA_HEAD", "WALLNUT_HEAD", "JALAPENO_HEAD", "GATLING_HEAD",
	"SQUASH_HEAD", "TALLNUT_HEAD", "REDEYE_GARGANTUAR",
};

const nlohmann::json kRules = nlohmann::json::parse(R"({
	"modesVanilla.challenge.11": [0, 2, 0, 5],
	"modesVanilla.challenge.12": [0, 2, 1, 6],
	"modesVanilla.challenge.14": [0, 2, 3, 8],
	"modesVanilla.challenge.15": [0, 2, 4, 9],
	"modesVanilla.challenge.49": [2, 3, 0, 10],
	"modesVanilla.allow.22.add": ["POLEVAULTER", "GATLING_HEAD"],
	"modesVanilla.allow.22.remove": ["TRAFFIC_CONE"],
	"modesVanilla.allow.29.add": ["FOOTBALL", "DANCER", "JACK_IN_THE_BOX", "BALLOON", "LADDER"],
	"modesVanilla.allow.34.add": ["POLEVAULTER", "DANCER"],
	"modesVanilla.allow.36.add": ["BUNGEE", "LADDER", "GARGANTUAR", "JACK_IN_THE_BOX"],
	"modesVanilla.allow.38.add": ["FOOTBALL", "JACK_IN_THE_BOX", "GATLING_HEAD"],
	"modesVanilla.allow.40.add": ["PEA_HEAD", "DANCER", "CATAPULT"],
	"modesVanilla.allow.40.remove": ["PAIL", "YETI"],
	"modesVanilla.allow.41.add": ["POLEVAULTER", "DOLPHIN_RIDER", "BUNGEE", "CATAPULT"],
	"modesVanilla.allow.45.add": ["POLEVAULTER", "DOOR", "DANCER", "ZAMBONI", "SQUASH_HEAD"],
	"modesVanilla.allow.45.remove": ["TRAFFIC_CONE"],
	"modesVanilla.art.ignorePictureSeeds": [11],
	"modesVanilla.graveDanger.gravesPerWave": 5,
	"modesVanilla.bannedSeeds.45": [11],
	"modesVanilla.bowling.zombie.3": 27,
	"modesVanilla.bowling.zombie.4": 31,
	"modesVanilla.bowling.packetFrom": 5,
	"modesVanilla.bowling.packetSeeds": [3, 49, 50],
	"modesVanilla.scaryPot.directSunFrom": 16,
	"modesVanilla.scaryPot.sunValue": 25,
	"modesVanilla.iz.cost.62": 125,
	"modesVanilla.iz.footballToggleZombie": 5,
	"modesVanilla.iz.dancerCompanions": [61, 62, 63, 67, 72, 73],
	"modesVanilla.iz.bungeeCompanions": [60, 61, 62, 63, 64, 65, 67, 68, 69, 70, 71, 72, 73, 74],
	"modesVanilla.iz.longPressTicks": 50,
	"modesVanilla.starfruit.noExtraStarsMode": 70,
	"modesVanilla.zen.gloveSkipsBusySquash": 1,
	"modesVanilla.numWaves.17": 30,
	"modesVanilla.numWaves.27": 40,
	"modesVanilla.numWaves.30": 16,
	"modesVanilla.numWaves.33": 40,
	"modesVanilla.numWaves.45": 30,
	"modesVanilla.numWaves.48": 40,
	"modesVanilla.conveyor.21": [[7, 25], [3, 15], [15, 5], [20, 15], [16, 30], [14, 10]],
	"modesVanilla.whack.bank": [2, 2, 2, 20, 20, 20, 15, 15, 15, 14],
	"modesVanilla.whack.groupSize": 4,
	"modesVanilla.whack.graveTarget": 30
})");

struct RulesGuard {
	explicit RulesGuard(const nlohmann::json& r) { SetRules(r); }
	~RulesGuard() { SetRules(nlohmann::json::object()); }
};

std::vector<bool> AllowedAfter(int theMode, std::vector<int> theVanilla) {
	std::vector<bool> aList(kZombieNames.size(), false);
	for (int t : theVanilla) aList[t] = true;
	bool aRaw[64] = {};
	for (size_t i = 0; i < aList.size(); i++) aRaw[i] = aList[i];
	std::string err;
	ApplyAllowEdit(AllowListEdit(theMode, kZombieNames, &err), aRaw, static_cast<int>(kZombieNames.size()));
	CHECK(err.empty());
	for (size_t i = 0; i < aList.size(); i++) aList[i] = aRaw[i];
	return aList;
}

std::vector<int> Types(const std::vector<bool>& theList) {
	std::vector<int> aOut;
	for (size_t i = 0; i < theList.size(); i++)
		if (theList[i]) aOut.push_back(static_cast<int>(i));
	return aOut;
}

} // namespace

TEST_CASE("ModesVanillaEnumNumbersMatchTheExe") {
	// The keys use the 1051 GameMode / SeedType / ZombieType numbers; PvZ-Portable's enums are the same.
	CHECK(GAMEMODE_SURVIVAL_ENDLESS_STAGE_1 == 11);
	CHECK(GAMEMODE_SURVIVAL_ENDLESS_STAGE_5 == 15);
	CHECK(GAMEMODE_CHALLENGE_SEEING_STARS == 0x16);
	CHECK(GAMEMODE_CHALLENGE_SPEED == 0x1d);
	CHECK(GAMEMODE_CHALLENGE_LAST_STAND == 0x1f);
	CHECK(GAMEMODE_CHALLENGE_POGO_PARTY == 0x22);
	CHECK(GAMEMODE_CHALLENGE_ART_CHALLENGE_WALLNUT == 0x24);
	CHECK(GAMEMODE_CHALLENGE_RESODDED == 0x26);
	CHECK(GAMEMODE_CHALLENGE_ART_CHALLENGE_SUNFLOWER == 0x28);
	CHECK(GAMEMODE_CHALLENGE_AIR_RAID == 0x29);
	CHECK(GAMEMODE_CHALLENGE_GRAVE_DANGER == 0x2d);
	CHECK(GAMEMODE_CHALLENGE_SQUIRREL == 0x31);
	CHECK(GAMEMODE_PUZZLE_I_ZOMBIE_ENDLESS == 0x46);
	CHECK(SEED_GRAVEBUSTER == 11);
	CHECK(SEED_EXPLODE_O_NUT == 49);
	CHECK(SEED_GIANT_WALLNUT == 50);
	CHECK(SEED_ZOMBIE_NORMAL == 0x3c);
	CHECK(SEED_ZOMBIE_POLEVAULTER == 0x3e);
	CHECK(SEED_ZOMBIE_BUNGEE == 0x42);
	CHECK(SEED_ZOMBIE_FOOTBALL == 0x43);
	CHECK(SEED_ZOMBIE_DANCER == 0x48);
	CHECK(SEED_ZOMBIE_IMP == 0x4a);
	CHECK(ZOMBIE_WALLNUT_HEAD == 27);
	CHECK(ZOMBIE_TALLNUT_HEAD == 31);
	CHECK(COIN_USABLE_SEED_PACKET == 16);
	CHECK(CHALLENGE_PAGE_SURVIVAL == 0);
	CHECK(CHALLENGE_PAGE_LIMBO == 2);
	CHECK(CURSOR_TYPE_GLOVE == 0xe);
	CHECK(kZombieNames.size() == static_cast<size_t>(NUM_ZOMBIE_TYPES));
}

TEST_CASE("ChallengeTableEndlessSurvivalsOnTheSurvivalPage") {
	RulesGuard g(kRules);
	// gChallengeDefs 0x6a2ba0 (24 x 72): Day/Night/Fog/Roof Endless sit on the Survival page next to Pool Endless,
	// with the Hard icons 5/6/8/9; Pool Endless (13) keeps its vanilla entry (page 0, row 2, col 2, icon 10).
	const struct { int mode; ChallengeSlot slot; } kRows[] = {
		{11, {0, 2, 0, 5}}, {12, {0, 2, 1, 6}}, {14, {0, 2, 3, 8}}, {15, {0, 2, 4, 9}},
		{49, {CHALLENGE_PAGE_LIMBO, 3, 0, 10}}, // Squirrel: Limbo row 2 col 3 -> row 3 col 0
	};
	for (const auto& r : kRows) {
		ChallengeSlot s;
		CAPTURE(r.mode);
		REQUIRE(ChallengeSlotFor(r.mode, s));
		CHECK(s == r.slot);
	}
	ChallengeSlot s;
	CHECK_FALSE(ChallengeSlotFor(13, s));
	CHECK_FALSE(ChallengeSlotFor(GAMEMODE_TREE_OF_WISDOM, s)); // only its name differs (Chinese, kept English here)
	CHECK_FALSE(ChallengeSlotFor(GAMEMODE_CHALLENGE_WAR_AND_PEAS, s));
}

TEST_CASE("ChallengeTableVanillaWithoutRules") {
	ChallengeSlot s;
	for (int m = 0; m < 80; m++)
		CHECK_FALSE(ChallengeSlotFor(m, s));
}

TEST_CASE("ChallengeSlotBadRowIsIgnored") {
	nlohmann::json r = kRules;
	r["modesVanilla.challenge.11"] = {0, 2, 0};       // too short
	r["modesVanilla.challenge.12"] = {0, 9, 1, 6};    // row off the page
	r["modesVanilla.challenge.14"] = {7, 2, 3, 8};    // no such page
	RulesGuard g(r);
	ChallengeSlot s;
	CHECK_FALSE(ChallengeSlotFor(11, s));
	CHECK_FALSE(ChallengeSlotFor(12, s));
	CHECK_FALSE(ChallengeSlotFor(14, s));
}

TEST_CASE("AllowListsMatchTheExe") {
	RulesGuard g(kRules);
	// Vanilla lists are PvZ-Portable's Challenge::InitZombieWaves (= 1051 .text). Default branch: Normal, Cone, Pail.
	const std::vector<int> kDefault = {ZOMBIE_NORMAL, ZOMBIE_TRAFFIC_CONE, ZOMBIE_PAIL};
	// hook 177 (0x7a03c3): Zombies on Speed gains Football, Dancer, Jack, Balloon, Ladder (BL = 1: sets, not clears).
	CHECK(Types(AllowedAfter(GAMEMODE_CHALLENGE_SPEED, {ZOMBIE_NORMAL, ZOMBIE_TRAFFIC_CONE, ZOMBIE_DOLPHIN_RIDER, ZOMBIE_POLEVAULTER})) ==
		std::vector<int>{ZOMBIE_NORMAL, ZOMBIE_TRAFFIC_CONE, ZOMBIE_POLEVAULTER, ZOMBIE_FOOTBALL, ZOMBIE_DANCER, ZOMBIE_DOLPHIN_RIDER,
			ZOMBIE_JACK_IN_THE_BOX, ZOMBIE_BALLOON, ZOMBIE_LADDER});
	// hook 178 (0x7a0416): Pogo Party = Pogo + Pole Vaulter + Dancer.
	CHECK(Types(AllowedAfter(GAMEMODE_CHALLENGE_POGO_PARTY, {ZOMBIE_POGO})) ==
		std::vector<int>{ZOMBIE_POLEVAULTER, ZOMBIE_DANCER, ZOMBIE_POGO});
	// hook 179 (0x7a0457): Air Raid = Balloon + Pole Vaulter, Dolphin Rider, Bungee, Catapult.
	CHECK(Types(AllowedAfter(GAMEMODE_CHALLENGE_AIR_RAID, {ZOMBIE_BALLOON})) ==
		std::vector<int>{ZOMBIE_POLEVAULTER, ZOMBIE_DOLPHIN_RIDER, ZOMBIE_BALLOON, ZOMBIE_BUNGEE, ZOMBIE_CATAPULT});
	// hook 180 (0x79ac00, stolen bytes at 0x425cad = the default branch's Cone write): Seeing Stars sets Normal, Gatling
	// head, Pole and returns past the Cone write -> no Cone.
	CHECK(Types(AllowedAfter(GAMEMODE_CHALLENGE_SEEING_STARS, kDefault)) ==
		std::vector<int>{ZOMBIE_NORMAL, ZOMBIE_POLEVAULTER, ZOMBIE_PAIL, ZOMBIE_GATLING_HEAD});
	// Art Wall-nut writes Cone itself, + Bungee, Ladder, Gargantuar, Jack.
	CHECK(Types(AllowedAfter(GAMEMODE_CHALLENGE_ART_CHALLENGE_WALLNUT, kDefault)) ==
		std::vector<int>{ZOMBIE_NORMAL, ZOMBIE_TRAFFIC_CONE, ZOMBIE_PAIL, ZOMBIE_JACK_IN_THE_BOX, ZOMBIE_BUNGEE, ZOMBIE_LADDER,
			ZOMBIE_GARGANTUAR});
	// Resodded: Cone, Football, Jack, Gatling head.
	CHECK(Types(AllowedAfter(GAMEMODE_CHALLENGE_RESODDED, kDefault)) ==
		std::vector<int>{ZOMBIE_NORMAL, ZOMBIE_TRAFFIC_CONE, ZOMBIE_PAIL, ZOMBIE_FOOTBALL, ZOMBIE_JACK_IN_THE_BOX, ZOMBIE_GATLING_HEAD});
	// Art Sunflower: Pea head, Cone, Dancer, Catapult, then 0x79af0b `xor ebx, ebx` -> the Pail write and the Yeti
	// write (0x425d8b, `mov [..+0x54e7], bl`) store false. Normal was set before the jump (0x425ca7).
	CHECK(Types(AllowedAfter(GAMEMODE_CHALLENGE_ART_CHALLENGE_SUNFLOWER, {ZOMBIE_NORMAL, ZOMBIE_TRAFFIC_CONE, ZOMBIE_PAIL, ZOMBIE_YETI})) ==
		std::vector<int>{ZOMBIE_NORMAL, ZOMBIE_TRAFFIC_CONE, ZOMBIE_DANCER, ZOMBIE_CATAPULT, ZOMBIE_PEA_HEAD});
	// Grave Danger: Pole, Screen Door, Dancer, Zomboni, Squash head; no Cone.
	CHECK(Types(AllowedAfter(GAMEMODE_CHALLENGE_GRAVE_DANGER, kDefault)) ==
		std::vector<int>{ZOMBIE_NORMAL, ZOMBIE_POLEVAULTER, ZOMBIE_PAIL, ZOMBIE_DOOR, ZOMBIE_DANCER, ZOMBIE_ZAMBONI, ZOMBIE_SQUASH_HEAD});
	// Every other default-branch mode ends in 0x7a5e96 `mov [ecx+0x54d6], bl` (Cone) -> vanilla.
	CHECK(Types(AllowedAfter(GAMEMODE_CHALLENGE_ICE, kDefault)) == kDefault);
	CHECK(AllowListEdit(GAMEMODE_CHALLENGE_LAST_STAND, kZombieNames, nullptr).Empty());
}

TEST_CASE("AllowListsVanillaWithoutRules") {
	for (int m = 0; m < 80; m++)
		CHECK(AllowListEdit(m, kZombieNames, nullptr).Empty());
}

TEST_CASE("AllowListUnknownNameIsReported") {
	nlohmann::json r = kRules;
	r["modesVanilla.allow.29.add"] = {"FOOTBALL", "NOT_A_ZOMBIE"};
	RulesGuard g(r);
	std::string err;
	const AllowEdit e = AllowListEdit(GAMEMODE_CHALLENGE_SPEED, kZombieNames, &err);
	CHECK(e.add == std::vector<int>{ZOMBIE_FOOTBALL});
	CHECK(err.find("NOT_A_ZOMBIE") != std::string::npos);
}

TEST_CASE("ArtChallengeGraveBusterIgnoresThePicture") {
	CHECK_FALSE(ArtIgnoresPicture(SEED_GRAVEBUSTER));
	RulesGuard g(kRules);
	// hook 175 (0xc113f0) replaced `cmp ebp, 0x1e; je` (Pumpkin) with Grave Buster, Pumpkin and their +80 forms; the
	// Lily Pad check stays at 0x42563d. Lily Pad and Pumpkin are vanilla (Challenge::CanPlantAt), not in this list.
	CHECK(ArtIgnoresPicture(SEED_GRAVEBUSTER));
	CHECK_FALSE(ArtIgnoresPicture(SEED_PEASHOOTER));
	CHECK_FALSE(ArtIgnoresPicture(SEED_WALLNUT));
}

TEST_CASE("GraveDangerFiveGravesAndNoGraveBuster") {
	CHECK(GraveDangerGravesPerWave(1) == 1);
	CHECK_FALSE(SeedBannedInMode(GAMEMODE_CHALLENGE_GRAVE_DANGER, SEED_GRAVEBUSTER));
	RulesGuard g(kRules);
	CHECK(GraveDangerGravesPerWave(1) == 5);               // hook 184: GraveDangerSpawnRandomGrave x5 (0xc11350)
	CHECK(SeedBannedInMode(GAMEMODE_CHALLENGE_GRAVE_DANGER, SEED_GRAVEBUSTER)); // 0x7b0f6e: mode 0x2d, seed 0xb -> 0
	CHECK_FALSE(SeedBannedInMode(GAMEMODE_CHALLENGE_GRAVE_DANGER, SEED_SUNFLOWER));
	CHECK_FALSE(SeedBannedInMode(GAMEMODE_CHALLENGE_ICE, SEED_GRAVEBUSTER));
	// Last Stand's sun plants are vanilla (SeedChooserScreen::SeedNotAllowedToPick), so no row here.
	CHECK_FALSE(SeedBannedInMode(GAMEMODE_CHALLENGE_LAST_STAND, SEED_SUNFLOWER));
}

TEST_CASE("BowlingRicochetRewards") {
	for (int n = 0; n < 12; n++)
		CHECK(BowlingRicochet(n).kind == RicochetKind::Vanilla);
	RulesGuard g(kRules);
	// 0x46300e -> 0xc110d0: count 3 / 4 -> AddZombieInRow((count == 4) * 4 + 0x1b), x = the nut's mX, hypnotised;
	// count >= 5 -> AddCoin(mX, mY, 16, 3) with the seed from 03 31 32 (0xc10ac3), count clamped to 7.
	CHECK(BowlingRicochet(1).kind == RicochetKind::Vanilla);
	CHECK(BowlingRicochet(2).kind == RicochetKind::Vanilla); // the vanilla silver coin (0x462ff6) stays
	CHECK(BowlingRicochet(3) == Ricochet{RicochetKind::Zombie, ZOMBIE_WALLNUT_HEAD});
	CHECK(BowlingRicochet(4) == Ricochet{RicochetKind::Zombie, ZOMBIE_TALLNUT_HEAD});
	CHECK(BowlingRicochet(5) == Ricochet{RicochetKind::SeedPacket, SEED_WALLNUT});
	CHECK(BowlingRicochet(6) == Ricochet{RicochetKind::SeedPacket, SEED_EXPLODE_O_NUT});
	CHECK(BowlingRicochet(7) == Ricochet{RicochetKind::SeedPacket, SEED_GIANT_WALLNUT});
	CHECK(BowlingRicochet(30) == Ricochet{RicochetKind::SeedPacket, SEED_GIANT_WALLNUT});
}

TEST_CASE("ScaryPotBigSunGoesStraightToTheBank") {
	CHECK(ScaryPotDirectSun(40) == 0);
	RulesGuard g(kRules);
	// hook 188 (0xc01f10): mSunCount >= 16 -> sun += count * [0x4309f0] (= 25), no drops. Vanilla pots hold 1..3.
	CHECK(ScaryPotDirectSun(3) == 0);
	CHECK(ScaryPotDirectSun(15) == 0);
	CHECK(ScaryPotDirectSun(16) == 400);
	CHECK(ScaryPotDirectSun(20) == 500);
}

TEST_CASE("IZombieCardTables") {
	// 0x8f01a8: card -> zombie (= vanilla IZombieSeedTypeToZombieType); 0x8f01b7: zombie -> card.
	CHECK(IZombieCardZombie(SEED_ZOMBIE_NORMAL) == ZOMBIE_NORMAL);
	CHECK(IZombieCardZombie(SEED_ZOMBIE_LADDER) == ZOMBIE_LADDER);
	CHECK(IZombieCardZombie(SEED_ZOMBIE_SCREEN_DOOR) == ZOMBIE_DOOR);
	CHECK(IZombieCardZombie(SEED_ZOMBONI) == ZOMBIE_ZAMBONI);
	CHECK(IZombieCardZombie(SEED_ZOMBIE_IMP) == ZOMBIE_IMP);
	CHECK(IZombieCardZombie(SEED_PEASHOOTER) == -1);
	for (int s = SEED_ZOMBIE_NORMAL; s <= SEED_ZOMBIE_IMP; s++)
		CHECK(IZombieZombieCard(IZombieCardZombie(s)) == s);
	CHECK(IZombieZombieCard(ZOMBIE_NEWSPAPER) == -1);
	CHECK(IZombieZombieCard(ZOMBIE_YETI) == -1);
}

TEST_CASE("IZombieCardsVanillaWithoutRules") {
	CHECK(IZombieCardCost(SEED_ZOMBIE_POLEVAULTER, 75) == 75);
	CHECK(IZombieFootballZombie(true, ZOMBIE_FOOTBALL) == ZOMBIE_FOOTBALL);
	CHECK(IZombieCompanionFor(SEED_ZOMBIE_DANCER, SEED_ZOMBIE_TRAFFIC_CONE) == -1);
	CHECK(IZombieCompanionFor(SEED_ZOMBIE_BUNGEE, SEED_ZOMBIE_NORMAL) == -1);
	CHECK_FALSE(IZombieIsCarrier(SEED_ZOMBIE_DANCER));
	CHECK_FALSE(IZombieRightClickToggles(SEED_ZOMBIE_FOOTBALL));
	CHECK(IZombieCarrierCost(SEED_ZOMBIE_DANCER, ZOMBIE_TRAFFIC_CONE, 350, 75) == 350);
}

TEST_CASE("IZombieCardsUnnamed") {
	RulesGuard g(kRules);
	// GetCost jump table 0x467bac: index 6 (Pole) points at the 125 block 0x467b71 (Cone's 75 is 0x467b65).
	CHECK(IZombieCardCost(SEED_ZOMBIE_POLEVAULTER, 75) == 125);
	CHECK(IZombieCardCost(SEED_ZOMBIE_TRAFFIC_CONE, 75) == 75);
	CHECK(IZombieCardCost(SEED_ZOMBIE_BUNGEE, 125) == 125);
	// hook 190 (0x42a08f -> 0xc05b85): the Football card places a Newspaper while the toggle (0xc05bdb) is 1.
	CHECK(IZombieFootballZombie(true, ZOMBIE_FOOTBALL) == ZOMBIE_NEWSPAPER);
	CHECK(IZombieFootballZombie(false, ZOMBIE_FOOTBALL) == ZOMBIE_FOOTBALL);
	CHECK(IZombieRightClickToggles(SEED_ZOMBIE_FOOTBALL));
	CHECK_FALSE(IZombieRightClickToggles(SEED_ZOMBIE_DANCER));
	// hook 78 (0xc04520): Dancer takes Cone, Pole, Pail, Football, Dancer, Gargantuar.
	CHECK(IZombieIsCarrier(SEED_ZOMBIE_DANCER));
	CHECK(IZombieIsCarrier(SEED_ZOMBIE_BUNGEE));
	CHECK_FALSE(IZombieIsCarrier(SEED_ZOMBIE_FOOTBALL));
	CHECK(IZombieCompanionFor(SEED_ZOMBIE_DANCER, SEED_ZOMBIE_TRAFFIC_CONE) == ZOMBIE_TRAFFIC_CONE);
	CHECK(IZombieCompanionFor(SEED_ZOMBIE_DANCER, SEED_ZOMBIE_GARGANTUAR) == ZOMBIE_GARGANTUAR);
	CHECK(IZombieCompanionFor(SEED_ZOMBIE_DANCER, SEED_ZOMBIE_DANCER) == ZOMBIE_DANCER);
	CHECK(IZombieCompanionFor(SEED_ZOMBIE_DANCER, SEED_ZOMBIE_NORMAL) == -1);
	CHECK(IZombieCompanionFor(SEED_ZOMBIE_DANCER, SEED_ZOMBIE_IMP) == -1);
	// 0x8f0140: Bungee takes every zombie card 0x3c..0x4a but itself.
	CHECK(IZombieCompanionFor(SEED_ZOMBIE_BUNGEE, SEED_ZOMBIE_NORMAL) == ZOMBIE_NORMAL);
	CHECK(IZombieCompanionFor(SEED_ZOMBIE_BUNGEE, SEED_ZOMBIE_IMP) == ZOMBIE_IMP);
	CHECK(IZombieCompanionFor(SEED_ZOMBIE_BUNGEE, SEED_ZOMBIE_GARGANTUAR) == ZOMBIE_GARGANTUAR);
	CHECK(IZombieCompanionFor(SEED_ZOMBIE_BUNGEE, SEED_ZOMBIE_BUNGEE) == -1);
	CHECK(IZombieCompanionFor(SEED_ZOMBIE_BUNGEE, SEED_PEASHOOTER) == -1);
	CHECK(IZombieCompanionFor(SEED_ZOMBIE_FOOTBALL, SEED_ZOMBIE_NORMAL) == -1);
	// Price: Dancer 350 + the companion card's price (0xc045f5 = GetCost(table) + 0x15e), Bungee 125 + (0x8f007e).
	CHECK(IZombieCarrierCost(SEED_ZOMBIE_DANCER, -1, 350, 0) == 350);
	CHECK(IZombieCarrierCost(SEED_ZOMBIE_DANCER, ZOMBIE_TRAFFIC_CONE, 350, 75) == 425);
	CHECK(IZombieCarrierCost(SEED_ZOMBIE_BUNGEE, ZOMBIE_GARGANTUAR, 125, 300) == 425);
	CHECK(IZombieCarrierCost(SEED_ZOMBIE_BUNGEE, -1, 125, 0) == 125);
}

TEST_CASE("StarfruitNeverSpiralsInIZombieEndless") {
	CHECK_FALSE(StarfruitNoExtraStars(GAMEMODE_PUZZLE_I_ZOMBIE_ENDLESS));
	RulesGuard g(kRules);
	// 0x95b790: mode 0x46 jumps to 0x95b0e7 with EAX = App (non-zero), so the jne skips the spawn: no roll, no stars
	CHECK(StarfruitNoExtraStars(GAMEMODE_PUZZLE_I_ZOMBIE_ENDLESS));
	CHECK_FALSE(StarfruitNoExtraStars(GAMEMODE_PUZZLE_I_ZOMBIE_9));
	CHECK_FALSE(StarfruitNoExtraStars(GAMEMODE_ADVENTURE));
}

TEST_CASE("ZenGloveLeavesABusySquash") {
	CHECK(GloveCanPickUp(SEED_SQUASH, 3));
	RulesGuard g(kRules);
	CHECK(GloveCanPickUp(SEED_SQUASH, 0));       // hook 478 (0xc02f10): Squash with state 0 is picked up
	CHECK_FALSE(GloveCanPickUp(SEED_SQUASH, 3));
	CHECK(GloveCanPickUp(SEED_PEASHOOTER, 3));
}

TEST_CASE("ModesVanillaPackRulesMatchTheseValues") {
	const char* home = std::getenv("HOME");
	fs::path file = std::getenv("PVZU_PACK_DIR") ? fs::path(std::getenv("PVZU_PACK_DIR"))
	                                              : fs::path(home ? home : "") / "pvz-unnamed-pack";
	file /= "data/rules-modes-vanilla.json";
	if (!fs::exists(file)) { MESSAGE("pack not here, skipped: " << file.string()); return; }
	std::ifstream in(file);
	nlohmann::json pack = nlohmann::json::parse(in, nullptr, false);
	REQUIRE(pack.is_object());
	CHECK(pack == kRules);
}

// ---- V2 (C23): vanilla mini-games the exe changes in place in .text ----

TEST_CASE("NumWavesOfTheChangedMiniGames") {
	// Board::PickZombieWaves 0x4093ec (Whack 16), 0x4093fe/0x409408 -> 0x40946c (Bowling, Grave Danger 30),
	// 0x409444/0x409459 -> 0x409460 (Column, Bowling 2 40), hook 0x79a207 (Bungee Blitz 40)
	CHECK(ModeNumWaves(GAMEMODE_CHALLENGE_WALLNUT_BOWLING, 20) == 20);
	RulesGuard g(kRules);
	CHECK(ModeNumWaves(GAMEMODE_CHALLENGE_WALLNUT_BOWLING, 20) == 30);
	CHECK(ModeNumWaves(GAMEMODE_CHALLENGE_WALLNUT_BOWLING_2, 30) == 40);
	CHECK(ModeNumWaves(GAMEMODE_CHALLENGE_COLUMN, 30) == 40);
	CHECK(ModeNumWaves(GAMEMODE_CHALLENGE_BUNGEE_BLITZ, 30) == 40);
	CHECK(ModeNumWaves(GAMEMODE_CHALLENGE_GRAVE_DANGER, 20) == 30);
	CHECK(ModeNumWaves(GAMEMODE_CHALLENGE_WHACK_A_ZOMBIE, 12) == 16);
	CHECK(ModeNumWaves(GAMEMODE_CHALLENGE_AIR_RAID, 20) == 20); // 0x409403 -> 0x409394: unchanged
	CHECK(ModeNumWaves(GAMEMODE_CHALLENGE_POGO_PARTY, 30) == 30);
}

TEST_CASE("InvisighoulConveyorPicks") {
	std::vector<std::pair<int, int>> aPicks;
	CHECK_FALSE(ConveyorPicks(GAMEMODE_CHALLENGE_INVISIGHOUL, aPicks));
	RulesGuard g(kRules);
	// Challenge::UpdateConveyorBelt 0x423250: {7,25} {3,15} {15,5} {20,15} {16,30} {14,10}
	REQUIRE(ConveyorPicks(GAMEMODE_CHALLENGE_INVISIGHOUL, aPicks));
	const std::vector<std::pair<int, int>> aWant = {{SEED_REPEATER, 25}, {SEED_WALLNUT, 15}, {SEED_DOOMSHROOM, 5},
		{SEED_JALAPENO, 15}, {SEED_LILYPAD, 30}, {SEED_ICESHROOM, 10}};
	CHECK(aPicks == aWant);
	CHECK_FALSE(ConveyorPicks(GAMEMODE_CHALLENGE_COLUMN, aPicks));
}

TEST_CASE("WhackAZombieBankGroupAndGraves") {
	CHECK(WhackBank().empty());
	CHECK(WhackGroupSize(1) == 1);
	CHECK(WhackGraveTarget(5) == 5);
	RulesGuard g(kRules);
	// Board::InitLevel 0x40b84d..0x40b888 + hook 34 0x959340: Cherry x3, Jalapeno x3, Doom x3, Ice
	const std::vector<int> aBank = {SEED_CHERRYBOMB, SEED_CHERRYBOMB, SEED_CHERRYBOMB, SEED_JALAPENO, SEED_JALAPENO,
		SEED_JALAPENO, SEED_DOOMSHROOM, SEED_DOOMSHROOM, SEED_DOOMSHROOM, SEED_ICESHROOM};
	CHECK(WhackBank() == aBank);
	CHECK(WhackGroupSize(1) == 4);    // WhackAZombieSpawning 0x42618f `mov [esp+0x1c],4`
	CHECK(WhackGraveTarget(5) == 30); // 0x426043 `mov ecx,0x1e; sub ecx,GetGraveStonesCount()`
}
