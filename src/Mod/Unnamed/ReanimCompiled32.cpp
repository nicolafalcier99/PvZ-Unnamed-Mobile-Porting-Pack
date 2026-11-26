// PVZU: reader for 32-bit (1.0.0.1051) compiled reanims; see ReanimCompiled32.h.
#include "Mod/Unnamed/ReanimCompiled32.h"

#include <cctype>
#include <cstring>

namespace pvzu::unnamed {

namespace {

struct Reader {
	const uint8_t* d;
	size_t n;
	size_t p = 0;
	bool ok = true;
	uint32_t U32() {
		if (p + 4 > n) { ok = false; return 0; }
		uint32_t v = 0;
		for (int i = 0; i < 4; ++i) v |= static_cast<uint32_t>(d[p + i]) << (8 * i);
		p += 4;
		return v;
	}
	float F32() { const uint32_t u = U32(); float f; std::memcpy(&f, &u, 4); return f; }
	std::string Str() {
		const uint32_t len = U32();
		if (!ok || len > n - p) { ok = false; return {}; }
		std::string s(reinterpret_cast<const char*>(d + p), len);
		p += len;
		return s;
	}
};

constexpr uint32_t kDefinitionSize = 16;   // tracks*, track count, fps, atlas*
constexpr uint32_t kTrackSize = 12;        // name*, transforms*, transform count
constexpr uint32_t kTransformSize = 44;    // 8 floats, image*, font*, text*

bool SameName(const std::string& a, const std::string& b) {
	if (a.size() != b.size()) return false;
	for (size_t i = 0; i < a.size(); ++i)
		if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i]))) return false;
	return true;
}

} // namespace

std::optional<uint32_t> CompiledUncompressedSize(const uint8_t* data, size_t size) {
	Reader r{data, size};
	if (r.U32() != 0xDEADFED4u || !r.ok) return std::nullopt;
	const uint32_t usize = r.U32();
	return r.ok ? std::optional<uint32_t>(usize) : std::nullopt;
}

std::optional<Reanim32> DecodeReanim32(const uint8_t* data, size_t size, std::string* err) {
	auto fail = [&](const char* why) -> std::optional<Reanim32> { if (err) *err = why; return std::nullopt; };
	Reader r{data, size};
	r.U32(); // schema hash of the 1051 build (not comparable with ours)
	if (r.p + kDefinitionSize > size) return fail("definition truncated");
	r.U32();                                 // tracks pointer
	const uint32_t trackCount = r.U32();
	Reanim32 out;
	out.fps = r.F32();
	r.U32();                                 // atlas pointer
	if (r.U32() != kTrackSize || !r.ok) return fail("track size is not the 32-bit layout");
	if (trackCount > (size - r.p) / kTrackSize) return fail("track table truncated");
	std::vector<uint32_t> counts(trackCount);
	for (uint32_t i = 0; i < trackCount; ++i) { r.U32(); r.U32(); counts[i] = r.U32(); }
	out.tracks.resize(trackCount);
	for (uint32_t i = 0; i < trackCount && r.ok; ++i) {
		Reanim32Track& t = out.tracks[i];
		t.name = r.Str();
		if (r.U32() != kTransformSize) return fail("transform size is not the 32-bit layout");
		if (!r.ok || counts[i] > (size - r.p) / kTransformSize) return fail("transforms truncated");
		t.transforms.resize(counts[i]);
		for (auto& x : t.transforms) {
			x.x = r.F32(); x.y = r.F32(); x.kx = r.F32(); x.ky = r.F32();
			x.sx = r.F32(); x.sy = r.F32(); x.f = r.F32(); x.a = r.F32();
			r.U32(); r.U32(); r.U32();       // image*, font*, text*
		}
		for (auto& x : t.transforms) { x.image = r.Str(); x.font = r.Str(); x.text = r.Str(); }
	}
	if (!r.ok) return fail("strings truncated");
	if (r.p != size) return fail("trailing bytes");
	return out;
}

std::optional<std::vector<MergeSlot>> PlanTrackMerge(const std::vector<std::string>& vanillaNames,
	const std::vector<int>& vanillaFrameCounts, const Reanim32& unnamed) {
	if (vanillaNames.size() != vanillaFrameCounts.size() || unnamed.tracks.empty()) return std::nullopt;
	const size_t frames = unnamed.tracks[0].transforms.size();
	for (const auto& t : unnamed.tracks)
		if (t.transforms.size() != frames) return std::nullopt;
	std::vector<bool> used(vanillaNames.size(), false);
	std::vector<MergeSlot> plan;
	for (size_t u = 0; u < unnamed.tracks.size(); ++u) {
		int found = -1;
		for (size_t v = 0; v < vanillaNames.size(); ++v)
			if (!used[v] && SameName(vanillaNames[v], unnamed.tracks[u].name)) { found = static_cast<int>(v); break; }
		if (found >= 0) {
			if (static_cast<size_t>(vanillaFrameCounts[found]) > frames) return std::nullopt; // Unnamed only appends frames
			used[found] = true;
			plan.push_back({true, found});
		} else {
			plan.push_back({false, static_cast<int>(u)});
		}
	}
	for (bool u : used)
		if (!u) return std::nullopt;
	return plan;
}

} // namespace pvzu::unnamed
