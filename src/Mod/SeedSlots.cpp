#include "Mod/SeedSlots.h"

#include "GameConstants.h"

#include <algorithm>

namespace pvzu {

static_assert(SEEDBANK_MAX >= kUnnamedSeedSlots, "the seed-packet array must hold Unnamed's 15 slots");

namespace {
constexpr int kVanillaMaxExtra = 153; // GetSeedBankExtraWidth at 10 packets
constexpr int kPitchPastTen = 51;     // GetSeedPacketPositionX's pitch at 10 packets
constexpr int kBankRightEnd = 12;     // the bank art's right frame, which vanilla's extension piece repeats
constexpr int kWoodLeft = 100;        // the packet area's wood starts right of the sun box (art x 0..~80)
} // namespace

int MaxSeedSlots(GameMode mode) { return mode == GameMode::Unnamed ? kUnnamedSeedSlots : kVanillaSeedSlots; }

int MaxPacketUpgrades(GameMode mode) { return MaxSeedSlots(mode) - kBaseSeedSlots; }

bool PacketUpgradeSoldOut(GameMode mode, int purchases) { return purchases >= MaxPacketUpgrades(mode); }

int DevSlotsToUpgrades(int slots) {
	return std::clamp(slots, kBaseSeedSlots, static_cast<int>(SEEDBANK_MAX)) - kBaseSeedSlots;
}

int ExtraPacketsToSave(int numPackets) {
	return std::clamp(numPackets - kSavedPacketsInVanillaChunk, 0, SEEDBANK_MAX - kSavedPacketsInVanillaChunk);
}

int TopBankPacketX(int index, int numPackets) {
	if (numPackets <= 7) return index * 59 + 85;
	if (numPackets == 8) return index * 54 + 81;
	if (numPackets == 9) return index * 52 + 80;
	return index * kPitchPastTen + 79;
}

int TopBankExtraWidth(int numPackets) {
	if (numPackets <= 6) return 0;
	if (numPackets == 7) return 60;
	if (numPackets == 8) return 76;
	if (numPackets == 9) return 112;
	return kVanillaMaxExtra + kPitchPastTen * std::max(0, numPackets - kVanillaSeedSlots);
}

std::vector<BankPiece> TopBankExtensionPieces(int bankImageW, int extraWidth) {
	std::vector<BankPiece> pieces;
	const int aEnd = std::min(extraWidth, kVanillaMaxExtra); // vanilla's piece: the art's last (end + 12) px
	const int aTiled = extraWidth - aEnd;
	const int aTileW = bankImageW - kBankRightEnd - kWoodLeft;
	int x = bankImageW - kBankRightEnd;
	for (int left = aTiled; left > 0 && aTileW > 0;) {
		const int w = std::min(left, aTileW);
		pieces.push_back({x, kWoodLeft, w});
		x += w;
		left -= w;
	}
	pieces.push_back({x, bankImageW - aEnd - kBankRightEnd, aEnd + kBankRightEnd});
	return pieces;
}

} // namespace pvzu
