// PVZU: see HdTextures.h.
#include "Mod/Engine/HdTextures.h"

#include "Mod/AssetOverlay.h"
#include "Mod/CmdLine.h"
#include "Mod/Engine/Startup.h"
#include "Mod/HdArt.h"

#include "Common.h"
#include "PvzpLib/PvzpDebug.h"
#include "graphics/MemoryImage.h"
#include "imagelib/ImageLib.h"
#include "SexyAppBase.h"
#include "graphics/GLInterface.h"

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <thread>
#include <unordered_map>

namespace pvzu::hd {

namespace fs = std::filesystem;

namespace {

struct GroupStats {
	int count = 0;
	int64_t estBytes = 0; // twin texels x 4 bytes, before piece padding and mipmaps
};

struct State {
	bool active = false;
	bool log = false;
	fs::path dir;
	PackInfo info;                                      // scale 0 when there is no PC pack
	std::unordered_map<std::string, std::string> index; // key -> file
	fs::path unnamedDir;                                // PVZU U1: hd-unnamed, twins of the overlay's own art
	PackInfo unnamedInfo;
	std::unordered_map<std::string, std::string> unnamedIndex;
	int64_t budgetBytes = 0;
	std::mutex statsMutex;
	std::map<std::string, GroupStats> groups;
	std::atomic<int64_t> liveAll{0}, liveHd{0}, peakAll{0}, peakHd{0};
	int64_t lastLoggedPeak = 0;
	std::atomic<int> twins{0}, uploads{0}, refusedBudget{0}, generated{0}, overlaySkipped{0}, unnamedTwins{0}, samePcTwins{0};
};

State& S() {
	static State* s = new State; // never destroyed: images outlive static destruction order
	return *s;
}

thread_local std::string tLoadGroup;

bool ReadFile(const fs::path& p, std::string& out) {
	std::ifstream in(p, std::ios::binary);
	if (!in)
		return false;
	out.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
	return true;
}

std::optional<std::pair<int, int>> PngFileSize(const std::string& file) {
	unsigned char head[24] = {};
	FILE* fp = std::fopen(file.c_str(), "rb");
	if (fp == nullptr)
		return std::nullopt;
	const size_t n = std::fread(head, 1, sizeof(head), fp);
	std::fclose(fp);
	return PngSize(head, n);
}

void NoteGroup(const std::string& key, int64_t bytes) {
	State& s = S();
	std::string group = tLoadGroup;
	if (group.empty()) {
		const size_t slash = key.find('/');
		group = "(" + (slash == std::string::npos ? std::string("other") : key.substr(0, slash)) + ")";
	}
	std::scoped_lock lk(s.statsMutex);
	GroupStats& g = s.groups[group];
	g.count++;
	g.estBytes += bytes;
}

// Decodes the twin's file into bits; false (twin unusable) when unreadable or not the size the header said.
bool EnsureBits(Twin& t) {
	if (t.bits)
		return true;
	if (t.file.empty())
		return false;
	std::unique_ptr<ImageLib::Image> img(ImageLib::GetImage(t.file, false));
	if (!img || img->mWidth != t.hdW || img->mHeight != t.hdH) {
		PvzpLogLn("PVZU HD: cannot read twin {} (dropped)", t.file);
		t.file.clear();
		return false;
	}
	t.bits = std::move(img->mBits);
	return true;
}

Twin* ValidTwin(Sexy::MemoryImage* theImage) {
	if (theImage == nullptr || !theImage->mPvzuHd)
		return nullptr;
	Twin* t = theImage->mPvzuHd.get();
	if (!TwinValid(t->attachedAt, theImage->mBitsChangedCount)) {
		theImage->mPvzuHd.reset(); // stale for good: the bits changed after the twin was attached
		return nullptr;
	}
	return t;
}

// A pack folder's hd.json: nullopt when absent or broken (logged).
std::optional<PackInfo> ReadPackInfo(const fs::path& dir) {
	std::string text;
	if (!ReadFile(dir / "hd.json", text))
		return std::nullopt;
	std::optional<PackInfo> info = ParsePackInfo(text);
	if (!info)
		PvzpLogLn("PVZU HD: {} has a broken hd.json; that pack is off", dir.string());
	return info;
}

void IndexPack(const fs::path& dir, std::unordered_map<std::string, std::string>& index) {
	std::error_code ec;
	for (fs::recursive_directory_iterator it(dir, ec), end; !ec && it != end; it.increment(ec)) {
		if (!it->is_regular_file(ec))
			continue;
		const std::string rel = fs::relative(it->path(), dir, ec).generic_string();
		const std::string key = KeyForPackFile(rel);
		if (!key.empty() && !IsAlphaCompanionKey(key))
			index[key] = it->path().string();
	}
}

fs::path PackDir(const char* theFlag, const char* theAppDataName) {
	if (auto dir = CmdOption(theFlag); dir && !dir->empty())
		return fs::path(*dir);
	return fs::path(Sexy::GetAppDataPath(theAppDataName));
}

} // namespace

void Init() {
	State& s = S();
	s.log = CmdFlag("-pvzu-log-hd");
	s.dir = PackDir("-pvzu-hd", "hd");
	std::optional<PackInfo> info = ReadPackInfo(s.dir);
	// PVZU U1: the hd-unnamed pack (twins of the overlay's own art) only matters while the overlay is on.
	std::optional<PackInfo> unnamedInfo;
	if (OverlayRoot()) {
		s.unnamedDir = PackDir("-pvzu-hd-unnamed", "hd-unnamed");
		unnamedInfo = ReadPackInfo(s.unnamedDir);
	}
	s.active = HdWanted(engine::StartupSettings().hd, info.has_value() || unnamedInfo.has_value());
	if (!s.active) {
		if (info || unnamedInfo)
			PvzpLogLn("PVZU HD: pack at {} turned off in pvzu-settings.json", s.dir.string());
		return;
	}
	s.budgetBytes = static_cast<int64_t>(CmdOptionInt("-pvzu-hd-budget-mb", 700)) << 20;
	if (info) {
		s.info = *info;
		IndexPack(s.dir, s.index);
		PvzpLogLn("PVZU HD: pack {} scale {} ({} twins, {} files indexed), budget {} MB", s.dir.string(), s.info.scale,
			s.info.count, s.index.size(), s.budgetBytes >> 20);
		if (s.log)
			std::fprintf(stderr, "PVZU HD: pack %s scale %d, %zu twins indexed, budget %lld MB\n", s.dir.string().c_str(),
				s.info.scale, s.index.size(), static_cast<long long>(s.budgetBytes >> 20));
	}
	if (unnamedInfo) {
		s.unnamedInfo = std::move(*unnamedInfo);
		IndexPack(s.unnamedDir, s.unnamedIndex);
		PvzpLogLn("PVZU HD: Unnamed pack {} scale {} ({} files indexed, {} same as PC)", s.unnamedDir.string(),
			s.unnamedInfo.scale, s.unnamedIndex.size(), s.unnamedInfo.samePc.size());
		if (s.log)
			std::fprintf(stderr, "PVZU HD: Unnamed pack %s scale %d, %zu twins indexed, %zu same as PC\n",
				s.unnamedDir.string().c_str(), s.unnamedInfo.scale, s.unnamedIndex.size(), s.unnamedInfo.samePc.size());
	}
}

bool Active() { return S().active; }

void SetLoadGroup(const std::string& theGroup) { tLoadGroup = theGroup; }

void OnImageLoaded(Sexy::MemoryImage* theImage, const std::string& theFileName) {
	State& s = S();
	if (!s.active || theImage == nullptr)
		return;
	const std::string key = Key(theFileName);
	auto it = s.index.find(key);
	auto un = s.unnamedIndex.find(key);
	const bool inPc = it != s.index.end();
	if (!inPc && un == s.unnamedIndex.end())
		return;
	const bool served = OverlayRoot() && OverlayServes(key, [](const std::string& n) { return OverlayFileExists(n); });
	// Same "zh" test as LoadOverlaySkipLists: any other language skips overlay-en-skip.txt.
	const UnnamedLookup u = LookUpUnnamed(s.unnamedInfo, key, un != s.unnamedIndex.end(), engine::StartupSettings().language == "zh");
	const bool samePc = served && u.samePc;
	std::string file;
	switch (PickTwin(served, u.inPack, samePc, inPc)) {
	case TwinFrom::None:
		if (served)
			s.overlaySkipped++; // Unnamed art with no twin of its own: a PC-based twin would be wrong
		return;
	case TwinFrom::Pc:
		file = it->second;
		if (samePc)
			s.samePcTwins++;
		break;
	case TwinFrom::Unnamed:
		file = un->second;
		s.unnamedTwins++;
		break;
	}
	auto size = PngFileSize(file);
	const int scale = size ? ScaleFor(theImage->mWidth, theImage->mHeight, size->first, size->second) : 0;
	if (scale == 0) {
		if (s.log)
			std::fprintf(stderr, "PVZU HD: %s twin is %dx%d for a %dx%d image: not used\n", key.c_str(),
				size ? size->first : 0, size ? size->second : 0, theImage->mWidth, theImage->mHeight);
		return;
	}
	auto t = std::make_shared<Twin>();
	t->file = file;
	t->key = key;
	t->scale = scale;
	t->hdW = size->first;
	t->hdH = size->second;
	t->attachedAt = theImage->mBitsChangedCount;
	theImage->mPvzuHd = std::move(t);
	s.twins++;
	NoteGroup(key, static_cast<int64_t>(size->first) * size->second * 4);
}

void KeepIfMatchedBefore(Sexy::MemoryImage* theImage, int theCountBefore) {
	if (theImage == nullptr || !theImage->mPvzuHd)
		return;
	if (theImage->mPvzuHd->attachedAt == theCountBefore)
		theImage->mPvzuHd->attachedAt = theImage->mBitsChangedCount;
}

Twin* TwinForUpload(Sexy::MemoryImage* theImage, int theMaxTextureSize) {
	State& s = S();
	if (!s.active)
		return nullptr;
	Twin* t = ValidTwin(theImage);
	if (t == nullptr)
		return nullptr;
	if (MaxLogicalPiece(theMaxTextureSize, t->scale) < 64)
		return nullptr;
	const int64_t add = static_cast<int64_t>(t->hdW) * t->hdH * 4;
	if (!BudgetAllows(s.liveAll.load(), add, s.budgetBytes)) {
		if (s.refusedBudget++ == 0 || s.log)
			PvzpLogLn("PVZU HD: budget {} MB reached; {} stays PC", s.budgetBytes >> 20, t->key);
		return nullptr;
	}
	std::scoped_lock lk(t->mutex);
	if (!EnsureBits(*t)) {
		theImage->mPvzuHd.reset();
		return nullptr;
	}
	return t;
}

void UploadDone(Twin* theTwin) {
	if (theTwin == nullptr)
		return;
	S().uploads++;
	std::scoped_lock lk(theTwin->mutex);
	if (!theTwin->file.empty())
		theTwin->bits.reset(); // generated twins keep theirs: they cannot be reloaded
}

void OnAtlasCreated(Sexy::MemoryImage* theAtlas, const std::vector<AtlasPart>& theParts) {
	State& s = S();
	if (!s.active || theAtlas == nullptr)
		return;
	// The atlas scale: the parts' (all 2x in a 2x pack). Parts at another scale are treated as having no twin.
	int scale = 0;
	for (const AtlasPart& p : theParts)
		if (Twin* t = ValidTwin(dynamic_cast<Sexy::MemoryImage*>(p.image))) {
			scale = t->scale;
			break;
		}
	if (scale == 0)
		return;
	const int W = theAtlas->mWidth * scale, H = theAtlas->mHeight * scale;
	auto bits = std::make_unique<uint32_t[]>(static_cast<size_t>(W) * H);
	std::fill_n(bits.get(), static_cast<size_t>(W) * H, 0u);
	int withTwin = 0;
	for (const AtlasPart& p : theParts) {
		auto* mem = dynamic_cast<Sexy::MemoryImage*>(p.image);
		if (mem == nullptr)
			continue;
		const IntRect r = ScaleRect({p.x, p.y, p.w, p.h}, scale);
		Twin* t = ValidTwin(mem);
		bool copied = false;
		if (t != nullptr && t->scale == scale) {
			std::scoped_lock lk(t->mutex);
			if (EnsureBits(*t) && t->hdW == r.w && t->hdH == r.h) {
				for (int y = 0; y < r.h; y++)
					std::copy_n(t->bits.get() + static_cast<size_t>(y) * r.w, r.w, bits.get() + static_cast<size_t>(r.y + y) * W + r.x);
				copied = true;
				withTwin++;
				if (!t->file.empty())
					t->bits.reset(); // the atlas holds them now; a direct draw reloads them
			}
		}
		if (!copied) {
			// No twin (overlay art, a GIF): the part's own pixels, each repeated scale x scale.
			const uint32_t* src = mem->GetBits();
			if (src == nullptr)
				continue;
			for (int y = 0; y < r.h; y++)
				for (int x = 0; x < r.w; x++)
					bits[static_cast<size_t>(r.y + y) * W + r.x + x] = src[static_cast<size_t>(y / scale) * p.w + x / scale];
		}
	}
	if (withTwin == 0)
		return;
	auto t = std::make_shared<Twin>();
	t->scale = scale;
	t->hdW = W;
	t->hdH = H;
	t->key = "(atlas)";
	t->bits = std::move(bits);
	t->attachedAt = theAtlas->mBitsChangedCount;
	theAtlas->mPvzuHd = std::move(t);
	s.generated++;
	NoteGroup("(atlas)/", static_cast<int64_t>(W) * H * 4);
}

void AttachRenderedTwin(Sexy::MemoryImage* theImage, const std::function<void(Sexy::Graphics*)>& theDraw) {
	State& s = S();
	Sexy::SexyAppBase* app = Sexy::gSexyAppBase;
	if (!s.active || theImage == nullptr || app == nullptr || app->mGLInterface == nullptr || std::this_thread::get_id() != app->mPrimaryThreadId)
		return;
	const int scale = RenderScale(s.info.scale, s.unnamedInfo.scale);
	std::vector<uint32_t> out;
	if (!app->mGLInterface->PvzuRenderToBits(theImage->mWidth, theImage->mHeight, scale, theDraw, out))
		return;
	const int W = theImage->mWidth * scale, H = theImage->mHeight * scale;
	// One ring of colour under the fully transparent pixels next to drawn ones (like the engine's sanding).
	std::vector<uint32_t> bled = out;
	for (int y = 0; y < H; y++)
		for (int x = 0; x < W; x++) {
			if (out[static_cast<size_t>(y) * W + x] >> 24)
				continue;
			uint32_t r = 0, g = 0, b = 0, n = 0;
			for (int dy = -1; dy <= 1; dy++)
				for (int dx = -1; dx <= 1; dx++) {
					const int nx = x + dx, ny = y + dy;
					if (nx < 0 || ny < 0 || nx >= W || ny >= H)
						continue;
					const uint32_t p = out[static_cast<size_t>(ny) * W + nx];
					if (p >> 24) {
						r += (p >> 16) & 0xFF;
						g += (p >> 8) & 0xFF;
						b += p & 0xFF;
						n++;
					}
				}
			if (n)
				bled[static_cast<size_t>(y) * W + x] = ((r / n) << 16) | ((g / n) << 8) | (b / n);
		}
	auto t = std::make_shared<Twin>();
	t->scale = scale;
	t->hdW = W;
	t->hdH = H;
	t->key = "(cache)";
	t->bits = std::make_unique<uint32_t[]>(bled.size());
	std::copy(bled.begin(), bled.end(), t->bits.get());
	t->attachedAt = theImage->mBitsChangedCount;
	theImage->mPvzuHd = std::move(t);
	s.generated++;
	NoteGroup("(cache)/", static_cast<int64_t>(W) * H * 4);
}

void NoteTextureBytes(int64_t theBytes, bool theHd) {
	State& s = S();
	const int64_t all = s.liveAll += theBytes;
	int64_t peak = s.peakAll.load();
	while (all > peak && !s.peakAll.compare_exchange_weak(peak, all)) {}
	if (theHd) {
		const int64_t hd = s.liveHd += theBytes;
		int64_t ph = s.peakHd.load();
		while (hd > ph && !s.peakHd.compare_exchange_weak(ph, hd)) {}
	}
	if (s.log && s.peakAll.load() > s.lastLoggedPeak + (32LL << 20)) {
		s.lastLoggedPeak = s.peakAll.load();
		std::fprintf(stderr, "PVZU HD: textures live %.1f MB (HD %.1f MB), peak %.1f MB (HD %.1f MB)\n",
			Megabytes(s.liveAll.load()), Megabytes(s.liveHd.load()), Megabytes(s.peakAll.load()), Megabytes(s.peakHd.load()));
	}
}

void LogSummary(const char* theWhen) {
	State& s = S();
	if (!s.log)
		return;
	std::fprintf(stderr, "PVZU HD %s: active %d, twins %d (generated %d), uploads %d, budget refusals %d, overlay wins %d, "
		"Unnamed twins %d, Unnamed same as PC %d\n",
		theWhen, s.active ? 1 : 0, s.twins.load(), s.generated.load(), s.uploads.load(), s.refusedBudget.load(),
		s.overlaySkipped.load(), s.unnamedTwins.load(), s.samePcTwins.load());
	std::fprintf(stderr, "PVZU HD %s: textures live %.1f MB (HD %.1f MB), peak %.1f MB (HD %.1f MB)\n", theWhen,
		Megabytes(s.liveAll.load()), Megabytes(s.liveHd.load()), Megabytes(s.peakAll.load()), Megabytes(s.peakHd.load()));
	std::scoped_lock lk(s.statsMutex);
	for (const auto& [name, g] : s.groups)
		std::fprintf(stderr, "PVZU HD %s:   group %-28s %4d twins %7.1f MB (twin pixels)\n", theWhen, name.c_str(), g.count, Megabytes(g.estBytes));
}

} // namespace pvzu::hd
