#include "doctest/doctest.h"
#include "Mod/SeedSlots.h"
#include "Mod/Mode.h"
#include "GameConstants.h"

#include <vector>

using namespace pvzu;

TEST_CASE("SeedSlotsArrayHoldsUnnamedFifteen") {
	// the research notes (modes-ui-shop) section 4: Unnamed sells seed-slot upgrades until 9 are owned (6 + 9 = 15 slots).
	CHECK(kVanillaSeedSlots == 10);
	CHECK(kUnnamedSeedSlots == 15);
	CHECK(SEEDBANK_MAX == kUnnamedSeedSlots);
}

TEST_CASE("SeedSlotsUpgradeLimitPerMode") {
	CHECK(MaxSeedSlots(pvzu::GameMode::Vanilla) == 10);
	CHECK(MaxSeedSlots(pvzu::GameMode::Unnamed) == 15);
	CHECK(MaxPacketUpgrades(pvzu::GameMode::Vanilla) == 4); // vanilla IsItemSoldOut: purchases >= 4
	CHECK(MaxPacketUpgrades(pvzu::GameMode::Unnamed) == 9); // Unnamed .text 0x48a9e9: cmp [ecx+0x214], 9
	CHECK_FALSE(PacketUpgradeSoldOut(pvzu::GameMode::Vanilla, 3));
	CHECK(PacketUpgradeSoldOut(pvzu::GameMode::Vanilla, 4));
	CHECK_FALSE(PacketUpgradeSoldOut(pvzu::GameMode::Unnamed, 4));
	CHECK_FALSE(PacketUpgradeSoldOut(pvzu::GameMode::Unnamed, 8));
	CHECK(PacketUpgradeSoldOut(pvzu::GameMode::Unnamed, 9));
	CHECK(PacketUpgradeSoldOut(pvzu::GameMode::Unnamed, 12)); // a hand-edited profile stays sold out
}

TEST_CASE("SeedSlotsDevFlagTakesSixToFifteen") {
	CHECK(DevSlotsToUpgrades(6) == 0);
	CHECK(DevSlotsToUpgrades(10) == 4);
	CHECK(DevSlotsToUpgrades(12) == 6);
	CHECK(DevSlotsToUpgrades(15) == 9);
	CHECK(DevSlotsToUpgrades(16) == 9);  // clamped
	CHECK(DevSlotsToUpgrades(1) == 0);
}

TEST_CASE("SeedSlotsSaveKeepsTheVanillaChunkAtTen") {
	// Vanilla and upstream saves carry exactly 10 packets in the seed-packet chunk; packets 11-15 go in a chunk of
	// our own, written only when the bank has more than 10 (vanilla saves stay byte-identical).
	CHECK(kSavedPacketsInVanillaChunk == 10);
	for (int n = 0; n <= 10; ++n)
		CHECK(ExtraPacketsToSave(n) == 0);
	CHECK(ExtraPacketsToSave(11) == 1);
	CHECK(ExtraPacketsToSave(15) == 5);
	CHECK(ExtraPacketsToSave(40) == SEEDBANK_MAX - 10); // never past the array
}

TEST_CASE("SeedSlotsTopBankVanillaUpToTen") {
	// Board::GetSeedPacketPositionX / GetSeedBankExtraWidth, vanilla table.
	for (int i = 0; i < 7; ++i)
		CHECK(TopBankPacketX(i, 7) == i * 59 + 85);
	CHECK(TopBankPacketX(3, 8) == 3 * 54 + 81);
	CHECK(TopBankPacketX(3, 9) == 3 * 52 + 80);
	CHECK(TopBankPacketX(9, 10) == 9 * 51 + 79);
	CHECK(TopBankExtraWidth(6) == 0);
	CHECK(TopBankExtraWidth(7) == 60);
	CHECK(TopBankExtraWidth(8) == 76);
	CHECK(TopBankExtraWidth(9) == 112);
	CHECK(TopBankExtraWidth(10) == 153);
}

TEST_CASE("SeedSlotsTopBankGrowsPastTenWithoutOverlap") {
	// During the seed chooser the vanilla bank holds 11-15 packets: same 51 px pitch as 10, bank one pitch wider each.
	constexpr int kBankW = 446; // IMAGE_SEEDBANK
	for (int n = 11; n <= 15; ++n) {
		CAPTURE(n);
		for (int i = 0; i + 1 < n; ++i)
			CHECK(TopBankPacketX(i + 1, n) - TopBankPacketX(i, n) >= SEED_PACKET_WIDTH);
		CHECK(TopBankExtraWidth(n) == 153 + 51 * (n - 10));
		// The last packet ends inside the bank's frame, as at 10 (10: 9*51+79+50 = 588 of 599).
		const int aBankRight = kBankW + TopBankExtraWidth(n);
		CHECK(TopBankPacketX(n - 1, n) + SEED_PACKET_WIDTH <= aBankRight - 11);
	}
}

namespace {
// Covered x range (bank-local) and whether every source px is inside the packet area (no sun box, no left frame).
bool CoversExactly(const std::vector<BankPiece>& pieces, int from, int to) {
	int x = from;
	for (const BankPiece& p : pieces) {
		if (p.dstX != x || p.w <= 0)
			return false;
		x += p.w;
	}
	return x == to;
}
} // namespace

TEST_CASE("SeedSlotsTopBankExtensionPieces") {
	constexpr int kBankW = 446;
	// Up to 153 px (10 packets): one piece, exactly vanilla's draw (src x W-extra-12, at x W-12).
	const std::vector<BankPiece> v = TopBankExtensionPieces(kBankW, 153);
	REQUIRE(v.size() == 1);
	CHECK(v[0].dstX == kBankW - 12);
	CHECK(v[0].srcX == kBankW - 153 - 12);
	CHECK(v[0].w == 153 + 12);
	// Wider: wood tiles from the packet area, then vanilla's right end; never the sun box (x < 90).
	for (int n = 11; n <= 15; ++n) {
		const int e = TopBankExtraWidth(n);
		CAPTURE(e);
		const std::vector<BankPiece> p = TopBankExtensionPieces(kBankW, e);
		CHECK(CoversExactly(p, kBankW - 12, kBankW + e));
		for (const BankPiece& q : p) {
			CHECK(q.srcX >= 90);
			CHECK(q.srcX + q.w <= kBankW);
		}
		CHECK(p.back().srcX + p.back().w == kBankW); // ends on the bank's own right edge
		for (size_t i = 0; i + 1 < p.size(); ++i)
			CHECK(p[i].srcX + p[i].w <= kBankW - 12); // the right frame shows only once
	}
}
