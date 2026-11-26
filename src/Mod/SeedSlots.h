// PVZU: seed slot counts. Vanilla sells 4 slot upgrades (6 + 4 = 10 slots); Unnamed sells 9 (6 + 9 = 15 slots,
// the research notes (modes-ui-shop) section 4: IsItemSoldOut(PACKET_UPGRADE) is `>= 9`, prices 75 / 500 / 2000 / 8000
// as vanilla, the seed bank stays purchases + 6). The engine's seed-packet array holds 15 (SEEDBANK_MAX); saves keep
// vanilla's 10-packet layout and add packets 11-15 in a chunk of their own (Lawn/System/SaveGame.cpp).
// During the seed chooser the vanilla top bank shows the packets; past 10 it grows one packet pitch per slot.
#pragma once

#include "Mod/Mode.h"

#include <vector>

namespace pvzu {

constexpr int kBaseSeedSlots = 6;      // slots before any upgrade
constexpr int kVanillaSeedSlots = 10;
constexpr int kUnnamedSeedSlots = 15;

int MaxSeedSlots(GameMode mode);
int MaxPacketUpgrades(GameMode mode);  // slot upgrades the shop sells
bool PacketUpgradeSoldOut(GameMode mode, int purchases);
// -pvzu-dev-slots N (6..15, clamped): the slot upgrades that give N slots.
int DevSlotsToUpgrades(int slots);

// Saves: the seed-packet chunk keeps exactly vanilla's 10 packets; the rest (bank of 11-15) goes in a PVZU chunk.
constexpr int kSavedPacketsInVanillaChunk = kVanillaSeedSlots;
int ExtraPacketsToSave(int numPackets);

// The vanilla top bank (bank-local x; Board::GetSeedPacketPositionX / GetSeedBankExtraWidth): vanilla's table up to
// 10 packets, 51 px apart (10's pitch) and one pitch wider per packet past 10.
int TopBankPacketX(int index, int numPackets);
int TopBankExtraWidth(int numPackets);
// How to draw the bank's extension right of the art (bank-local dst x, art src x, width): vanilla's one piece up to
// 153 px; wider, tiles of the packet area's wood then vanilla's right end.
struct BankPiece { int dstX = 0, srcX = 0, w = 0; };
std::vector<BankPiece> TopBankExtensionPieces(int bankImageW, int extraWidth);

} // namespace pvzu
