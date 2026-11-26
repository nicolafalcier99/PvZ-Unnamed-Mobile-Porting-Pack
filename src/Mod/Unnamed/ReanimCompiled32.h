// PVZU: reader for Unnamed's own compiled reanims (32-bit PopCap 1.0.0.1051 layout, as shipped in the pack under
// assets/compiled/reanim/*.reanim.compiled). The GOTY engine cannot load them (64-bit layout, other schema hash), so
// the tracks that only Unnamed has (e.g. the Newspaper's Zombie_knife) are read here and merged into the vanilla
// definition at load time. Pure functions; zlib is done by the caller.
#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace pvzu::unnamed {

struct Reanim32Transform {
	float x = 0, y = 0, kx = 0, ky = 0, sx = 0, sy = 0, f = 0, a = 0; // -10000 = "same as the previous frame"
	std::string image, font, text;
};
struct Reanim32Track {
	std::string name;
	std::vector<Reanim32Transform> transforms;
};
struct Reanim32 {
	float fps = 12.0f;
	std::vector<Reanim32Track> tracks;
};

// File header: cookie 0xDEADFED4 + uncompressed size; the zlib stream starts at byte 8.
std::optional<uint32_t> CompiledUncompressedSize(const uint8_t* data, size_t size);
// The uncompressed payload: schema hash, the 16-byte definition, then tracks (12 bytes each) and their transforms
// (44 bytes each) with their strings. nullopt (and *err) on any size mismatch.
std::optional<Reanim32> DecodeReanim32(const uint8_t* data, size_t size, std::string* err = nullptr);

// Where each track of the merged definition comes from, in Unnamed's draw order.
struct MergeSlot {
	bool fromVanilla = false;
	int index = 0;   // vanilla track index or Unnamed track index
	bool operator==(const MergeSlot&) const = default;
};
// Every vanilla track must exist in Unnamed's (same name, no more frames: Unnamed appends its new animations after the
// GOTY frames) and every Unnamed track must have the same frame count; otherwise nullopt (the vanilla definition is
// kept). Names compare case-insensitively. The engine then builds every track from Unnamed's data (what Unnamed runs).
std::optional<std::vector<MergeSlot>> PlanTrackMerge(const std::vector<std::string>& vanillaNames,
	const std::vector<int>& vanillaFrameCounts, const Reanim32& unnamed);

} // namespace pvzu::unnamed
