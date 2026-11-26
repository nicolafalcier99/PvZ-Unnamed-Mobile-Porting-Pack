// PVZU: Plan 8 part 3b - the engine glue for Unnamed's special (non-shooter) mod plants. The rules themselves are tested
// in test_mod_plant_rules.cpp; here: the per-shot state the glue saves (Shrapnel shards, Prism beam, a shot a Leaf
// Shield turned) and the pure helpers the glue calls (the research notes (mod-plants-behaviour) 0x52, 0x5d, 0x75, 0x76).
#include "doctest/doctest.h"
#include "Mod/GameData.h"
#include "Mod/ModPack.h"
#include "Mod/Unnamed/ModPlantRules.h"
#include "Mod/Unnamed/ModPlantSpecials.h"
#include "Mod/Unnamed/Rules.h"
#include "Mod/Unnamed/State.h"

#include <vector>

using namespace pvzu;
using namespace pvzu::unnamed;

namespace {
void SetFeature(bool on) {
	GameData::Get().ResetToVanilla();
	if (on) {
		ModPack p;
		p.id = "test-mod-plants-specials";
		p.features["modPlants"] = true;
		std::string err;
		REQUIRE_MESSAGE(GameData::Get().ApplyPack(p, &err), err);
	}
}

// The Boss Flower table 0xc000fd: seeds 0..52 and the 25 listed mod seeds allowed, everything else 0.
nlohmann::json BossFlowerRules() {
	nlohmann::json table = nlohmann::json::array();
	for (int s = 0; s < 128; ++s) {
		const bool mod = s == 0x50 || s == 0x51 || s == 0x52 || s == 0x53 || s == 0x55 || s == 0x56 || s == 0x57 ||
			s == 0x58 || s == 0x59 || s == 0x5b || s == 0x5c || s == 0x5d || s == 0x67 || s == 0x69 || s == 0x6b ||
			s == 0x6d || s == 0x6e || s == 0x70 || s == 0x72 || s == 0x73 || s == 0x75 || s == 0x76 || s == 0x78 ||
			s == 0x7a || s == 0x7b;
		table.push_back(s <= 52 || mod ? 1 : 0);
	}
	return {{"modPlants.bossFlower.rollRange", 128}, {"modPlants.bossFlower.seedTable", table}};
}

struct OnFixture {
	OnFixture() { SetRules(BossFlowerRules()); SetFeature(true); }
	~OnFixture() { SetRules(nlohmann::json::object()); SetFeature(false); }
};
} // namespace

TEST_CASE("Mod shot state (Shrapnel shard kind) survives save/load") {
	ProjectileState p;
	p.modShot = kModShotShrapnelCore;
	const auto blob = Encode(p);
	CHECK(blob.size() == 44);
	ProjectileState q;
	REQUIRE(Decode(blob.data(), blob.size(), q));
	CHECK(q == p);
	CHECK(q.modShot == kModShotShrapnelCore);

	// a 24-byte blob (before Plan 8 part 3) reads as an ordinary shot
	ProjectileState old;
	old.butterStreak = 3;
	auto oldBlob = Encode(old);
	oldBlob.resize(24);
	ProjectileState r;
	r.modShot = 9;
	REQUIRE(Decode(oldBlob.data(), oldBlob.size(), r));
	CHECK(r.butterStreak == 3);
	CHECK(r.modShot == kModShotNone);
	CHECK(r.modType == 0);
	CHECK(r.field80 == 0);
}

namespace {
void PutLe(std::vector<uint8_t>& b, size_t at, int32_t v) {
	for (int i = 0; i < 4; ++i)
		b[at + i] = static_cast<uint8_t>(static_cast<uint32_t>(v) >> (8 * i));
}
} // namespace

TEST_CASE("A Prism beam is one mod shot: exe type 0x16 with its damage in field80 (0x7b08fb); old prism blobs convert") {
	// Prism-shroom's and Spirit-tail's beams are both exe type 0x16 whose damage is Projectile+0x80.
	ProjectileState p;
	p.modType = 0x16;
	p.field80 = 425;
	const auto blob = Encode(p);
	ProjectileState q;
	REQUIRE(Decode(blob.data(), blob.size(), q));
	CHECK(q == p);
	CHECK(q.modShot == kModShotNone);

	// blobs from before the merge (never shipped): mod shot kind 3 with its damage in bytes 28..31
	auto legacy = Encode(ProjectileState{});
	PutLe(legacy, 24, 3);
	PutLe(legacy, 28, 425);
	ProjectileState r;
	REQUIRE(Decode(legacy.data(), legacy.size(), r));
	CHECK(r.modShot == kModShotNone);
	CHECK(r.modType == 0x16);
	CHECK(r.field80 == 425);
	legacy.resize(32); // and the 32-byte form (before Plan 8 part 3a)
	REQUIRE(Decode(legacy.data(), legacy.size(), r));
	CHECK(r.modShot == kModShotNone);
	CHECK(r.modType == 0x16);
	CHECK(r.field80 == 425);
	// the reserved bytes 28..31 are written as 0 and ignored for every other kind
	auto shard = Encode(ProjectileState{});
	PutLe(shard, 24, kModShotShrapnelShard);
	PutLe(shard, 28, 77);
	REQUIRE(Decode(shard.data(), shard.size(), r));
	CHECK(r.modShot == kModShotShrapnelShard);
	CHECK(r.modType == 0);
	CHECK(r.field80 == 0);
	CHECK(Encode(r) == Encode(ProjectileState{.modShot = kModShotShrapnelShard}));
}

TEST_CASE("Mod shot kinds are distinct and 0 means an ordinary shot") {
	CHECK(kModShotNone == 0);
	const std::vector<int> kinds = {kModShotShrapnelShard, kModShotShrapnelCore, kModShotLeafReflected};
	for (size_t i = 0; i < kinds.size(); ++i) {
		CHECK(kinds[i] != kModShotNone);
		for (size_t j = i + 1; j < kinds.size(); ++j)
			CHECK(kinds[i] != kinds[j]);
	}
	// exe Projectile+0x84: shard 1, core 2 (0x794017..0x7941da)
	CHECK(kModShotShrapnelShard == 1);
	CHECK(kModShotShrapnelCore == 2);
	CHECK(ShrapnelKindOfShot(kModShotShrapnelShard) == 1);
	CHECK(ShrapnelKindOfShot(kModShotShrapnelCore) == 2);
	CHECK(ShrapnelKindOfShot(kModShotLeafReflected) == 0);
	CHECK(ShrapnelKindOfShot(kModShotNone) == 0);
}

TEST_CASE_FIXTURE(OnFixture, "Boss Flower draw: Rand(128) again until the table allows the seed (hook 292 0xc000e0)") {
	// rolls 0x7f (Loaded Cannon, not allowed), 0x54 (empty slot), then 0x76
	std::vector<int> rolls = {0x7f, 0x54, 0x76};
	size_t next = 0;
	std::vector<int> ranges;
	const auto rand = [&](int n) { ranges.push_back(n); return rolls[next++]; };
	const auto any = [](int) { return true; };
	CHECK(BossFlowerDrawSeed(rand, any) == 0x76);
	CHECK(ranges == std::vector<int>{128, 128, 128});

	// a seed the pack has no row for is drawn again (the exe's table only names rows it has)
	rolls = {0x53, 2};
	next = 0;
	const auto noBowling = [](int s) { return s != 0x53; };
	CHECK(BossFlowerDrawSeed(rand, noBowling) == 2);

	// nothing allowed: gives up after maxTries instead of looping forever
	rolls.assign(5, 0x7f);
	next = 0;
	CHECK(BossFlowerDrawSeed(rand, any, 5) == -1);
}

TEST_CASE("Boss Flower draw is off without the feature") {
	SetRules(BossFlowerRules());
	SetFeature(false);
	int calls = 0;
	const auto rand = [&](int) { ++calls; return 3; };
	CHECK(BossFlowerDrawSeed(rand, [](int) { return true; }) == -1);
	CHECK(calls == 0);
	SetRules(nlohmann::json::object());
}

TEST_CASE("Packet coin seeds: vanilla seeds always, mod seeds only with a table row") {
	const auto rows = [](int s) { return s == 0x51 || s == 0x59; };
	CHECK(PacketCoinSeedUsable(1, rows));
	CHECK(PacketCoinSeedUsable(48, rows)); // Imitater
	CHECK(PacketCoinSeedUsable(52, rows)); // Leftpeater
	CHECK(PacketCoinSeedUsable(0x51, rows));
	CHECK_FALSE(PacketCoinSeedUsable(0x52, rows));
	CHECK_FALSE(PacketCoinSeedUsable(53, rows));
	CHECK_FALSE(PacketCoinSeedUsable(-1, rows));
	CHECK_FALSE(PacketCoinSeedUsable(0x80, rows));
}
