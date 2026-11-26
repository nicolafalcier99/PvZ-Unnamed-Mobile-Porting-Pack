#include "Mod/Unnamed/State.h"

#include <bit>

namespace pvzu::unnamed {

namespace {
void Put(std::vector<uint8_t>& b, int32_t v) {
	const auto u = static_cast<uint32_t>(v);
	for (int i = 0; i < 4; ++i) b.push_back(static_cast<uint8_t>(u >> (8 * i)));
}
// Before the Prism merge: modShot 3 = a Prism-shroom beam, its damage in bytes 28..31 (now modType 0x16 + field80).
constexpr int32_t kLegacyModShotPrismBeam = 3, kLegacyPrismExeType = 0x16;
int32_t Get(const uint8_t* d) {
	uint32_t u = 0;
	for (int i = 0; i < 4; ++i) u |= static_cast<uint32_t>(d[i]) << (8 * i);
	return static_cast<int32_t>(u);
}
}

std::vector<uint8_t> Encode(const ZombieState& s) {
	std::vector<uint8_t> b;
	Put(b, s.variant); Put(b, s.counter); Put(b, s.extra); Put(b, s.flags); Put(b, s.subVariant); Put(b, s.stored);
	Put(b, s.toxicKind); Put(b, s.toxicTimer); Put(b, s.repeaterPoisonCs); Put(b, s.weakenCs); // Plan 8 part 3 (bytes 24..39)
	return b;
}
std::vector<uint8_t> Encode(const PlantState& s) {
	std::vector<uint8_t> b;
	Put(b, s.counter); Put(b, s.extra); Put(b, s.frozen);
	Put(b, s.sweepAngle); Put(b, s.sweepDir); Put(b, s.sweepStep); Put(b, s.sweepTimer);
	Put(b, s.modSeed); // Plan 8
	Put(b, s.elementBits); Put(b, s.elementMode); Put(b, s.elementCount); Put(b, s.starColour); Put(b, s.cannonAmmo); // Plan 8 part 3a (32..51)
	Put(b, s.shadow); Put(b, s.charmedBitten); // Plan 8 part 3c (bytes 52..59)
	return b;
}
std::vector<uint8_t> Encode(const ProjectileState& s) {
	std::vector<uint8_t> b;
	Put(b, s.fireTier); Put(b, s.converted ? 1 : 0); Put(b, s.pierceHits); Put(b, s.starCurve);
	Put(b, s.bigCabbage ? 1 : 0); Put(b, s.butterStreak);
	Put(b, s.modShot); Put(b, 0);                          // Plan 8 part 3b (bytes 24..31; 28..31 reserved)
	Put(b, s.modType); Put(b, s.field80); Put(b, s.cobAmmo); // Plan 8 part 3a (bytes 32..43)
	return b;
}
bool Decode(const uint8_t* d, size_t n, ZombieState& out) {
	out = {};
	if (n < 12) return false;
	out.variant = Get(d); out.counter = Get(d + 4); out.extra = Get(d + 8);
	if (n >= 16) out.flags = Get(d + 12); // 12-byte blobs (Task 2 format) have no flags
	if (n >= 24) { out.subVariant = Get(d + 16); out.stored = Get(d + 20); } // 16-byte blobs: no Night fields
	if (n >= 40) { // 24-byte blobs (before Plan 8 part 3): no mod-plant status
		out.toxicKind = Get(d + 24); out.toxicTimer = Get(d + 28); out.repeaterPoisonCs = Get(d + 32); out.weakenCs = Get(d + 36);
	}
	return true;
}
bool Decode(const uint8_t* d, size_t n, PlantState& out) {
	out = {};
	if (n < 8) return false;
	out.counter = Get(d); out.extra = Get(d + 4);
	if (n >= 12) out.frozen = Get(d + 8); // 8-byte blobs (before Plan 4): not frozen
	if (n >= 28) { // 12-byte blobs (before Plan 5): an idle Split Pea
		out.sweepAngle = Get(d + 12); out.sweepDir = Get(d + 16); out.sweepStep = Get(d + 20); out.sweepTimer = Get(d + 24);
	}
	if (n >= 32) out.modSeed = Get(d + 28); // 28-byte blobs (before Plan 8): an ordinary plant
	if (n >= 52) { // 32-byte blobs (before Plan 8 part 3a): no shooter state
		out.elementBits = Get(d + 32); out.elementMode = Get(d + 36); out.elementCount = Get(d + 40);
		out.starColour = Get(d + 44); out.cannonAmmo = Get(d + 48);
	}
	if (n >= 60) { out.shadow = Get(d + 52); out.charmedBitten = Get(d + 56); } // 52-byte blobs (before Plan 8 part 3c)
	return true;
}

bool Decode(const uint8_t* d, size_t n, ProjectileState& out) {
	out = {};
	if (n < 4) return false;
	out.fireTier = Get(d);
	if (n >= 8) out.converted = Get(d + 4) != 0; // 4-byte blobs (before Plan 4 Task 1) have no flag
	if (n >= 16) { out.pierceHits = Get(d + 8); out.starCurve = Get(d + 12); } // 8-byte blobs (before Plan 5 Task 1)
	if (n >= 24) { out.bigCabbage = Get(d + 16) != 0; out.butterStreak = Get(d + 20); } // 16-byte blobs (before Plan 6 Task 1)
	if (n >= 32) out.modShot = Get(d + 24); // 24-byte blobs (before Plan 8 part 3)
	if (n >= 44) { out.modType = Get(d + 32); out.field80 = Get(d + 36); out.cobAmmo = Get(d + 40); } // 32-byte blobs
	if (n >= 32 && out.modShot == kLegacyModShotPrismBeam) { // never shipped, read anyway: the one Prism path
		out.modShot = kModShotNone;
		out.modType = kLegacyPrismExeType;
		out.field80 = Get(d + 28);
	}
	return true;
}

std::vector<uint8_t> Encode(const GridState& s) {
	std::vector<uint8_t> b;
	Put(b, s.potHypnotised);
	return b;
}
bool Decode(const uint8_t* d, size_t n, GridState& out) {
	out = {};
	if (n < 4) return false;
	out.potHypnotised = Get(d);
	return true;
}

std::vector<uint8_t> Encode(const BoardState& s) {
	std::vector<uint8_t> b;
	Put(b, s.iceFreezeActive); Put(b, s.iceFreezeCounter);
	if (s.starTurns.empty() && s.starSlots.empty())
		return b; // no snake: the same 8 bytes as before
	Put(b, static_cast<int32_t>(s.starTurns.size()));
	for (int32_t v : s.starTurns) Put(b, v);
	Put(b, static_cast<int32_t>(s.starSlots.size()));
	for (const StarSlot& t : s.starSlots) {
		Put(b, t.index); Put(b, t.type); Put(b, t.x); Put(b, t.y);
		Put(b, std::bit_cast<int32_t>(t.velX)); Put(b, std::bit_cast<int32_t>(t.velY));
		Put(b, t.age); Put(b, t.tail); Put(b, t.turns);
	}
	return b;
}

bool Decode(const uint8_t* d, size_t n, BoardState& out) {
	out = {};
	if (n < 8) return false;
	out.iceFreezeActive = Get(d); out.iceFreezeCounter = Get(d + 4);
	if (n < 12)
		return true; // a chunk from before the star snake
	auto aFail = [&out]() { out = {}; return false; };
	size_t at = 8;
	const int32_t aTurns = Get(d + at);
	at += 4;
	if (aTurns < 0 || n < at + 4 * static_cast<size_t>(aTurns) + 4)
		return aFail();
	for (int32_t i = 0; i < aTurns; ++i, at += 4)
		out.starTurns.push_back(Get(d + at));
	const int32_t aSlots = Get(d + at);
	at += 4;
	constexpr size_t kSlotBytes = 9 * 4;
	if (aSlots < 0 || n < at + kSlotBytes * static_cast<size_t>(aSlots))
		return aFail();
	for (int32_t i = 0; i < aSlots; ++i, at += kSlotBytes) {
		const uint8_t* p = d + at;
		out.starSlots.push_back({Get(p), Get(p + 4), Get(p + 8), Get(p + 12), std::bit_cast<float>(Get(p + 16)),
			std::bit_cast<float>(Get(p + 20)), Get(p + 24), Get(p + 28), Get(p + 32)});
	}
	return true;
}

} // namespace pvzu::unnamed
