#include "Mod/HdArt.h"

#include "nlohmann/json.hpp"

#include <algorithm>
#include <cctype>

namespace pvzu::hd {

namespace {
constexpr std::string_view kImageExts[] = {".png", ".jpg", ".jpeg", ".gif", ".bmp", ".tga"};

std::string Lower(std::string_view s) {
	std::string out(s);
	for (char& c : out) {
		c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
		if (c == '\\')
			c = '/';
	}
	return out;
}

bool EndsWith(std::string_view s, std::string_view suffix) {
	return s.size() >= suffix.size() && s.substr(s.size() - suffix.size()) == suffix;
}
} // namespace

std::string Key(std::string_view gameName) {
	std::string k = Lower(gameName);
	while (k.rfind("./", 0) == 0)
		k.erase(0, 2);
	const size_t slash = k.rfind('/');
	const size_t dot = k.rfind('.');
	if (dot != std::string::npos && (slash == std::string::npos || dot > slash)) {
		const std::string_view ext = std::string_view(k).substr(dot);
		if (std::find(std::begin(kImageExts), std::end(kImageExts), ext) != std::end(kImageExts))
			k.resize(dot);
	}
	return k;
}

std::string KeyForPackFile(std::string_view relPath) {
	const std::string lower = Lower(relPath);
	if (!EndsWith(lower, ".png"))
		return {};
	return Key(lower);
}

std::vector<std::string> AlphaCompanionKeys(std::string_view key) {
	const std::string k(key);
	const size_t slash = k.rfind('/');
	const size_t nameAt = slash == std::string::npos ? 0 : slash + 1;
	return {k.substr(0, nameAt) + "_" + k.substr(nameAt), k + "_"};
}

bool IsAlphaCompanionKey(std::string_view key) {
	const size_t slash = key.rfind('/');
	const std::string_view name = slash == std::string_view::npos ? key : key.substr(slash + 1);
	return !name.empty() && (name.front() == '_' || name.back() == '_');
}

int ScaleFor(int logicalW, int logicalH, int hdW, int hdH) {
	if (logicalW <= 0 || logicalH <= 0)
		return 0;
	for (int s : {2, 4})
		if (hdW == logicalW * s && hdH == logicalH * s)
			return s;
	return 0;
}

float HalfTexel(int pieceLogical, int scale) {
	return 0.5f / static_cast<float>(pieceLogical * std::max(scale, 1));
}

IntRect ScaleRect(const IntRect& r, int scale) {
	return {r.x * scale, r.y * scale, r.w * scale, r.h * scale};
}

int MaxLogicalPiece(int maxTextureSize, int scale) {
	return maxTextureSize / std::max(scale, 1);
}

bool OverlayServes(std::string_view key, const std::function<bool(const std::string&)>& exists) {
	std::vector<std::string> names = AlphaCompanionKeys(key);
	names.insert(names.begin(), std::string(key));
	for (const std::string& n : names)
		for (std::string_view ext : kImageExts)
			if (exists(n + std::string(ext)))
				return true;
	return false;
}

std::optional<PackInfo> ParsePackInfo(std::string_view text) {
	const nlohmann::json j = nlohmann::json::parse(text, nullptr, /*allow_exceptions=*/false);
	if (!j.is_object() || !j.contains("scale") || !j["scale"].is_number_integer())
		return std::nullopt;
	PackInfo info;
	info.scale = j["scale"].get<int>();
	if (info.scale != 2 && info.scale != 4)
		return std::nullopt;
	if (j.contains("count") && j["count"].is_number_integer())
		info.count = j["count"].get<int>();
	if (j.contains("source") && j["source"].is_string())
		info.source = j["source"].get<std::string>();
	if (j.contains("samePc") && j["samePc"].is_array())
		for (const auto& n : j["samePc"])
			if (n.is_string())
				info.samePc.insert(Key(n.get<std::string>()));
	if (j.contains("samePcEn") && j["samePcEn"].is_array())
		for (const auto& n : j["samePcEn"])
			if (n.is_string())
				info.samePcEn.insert(Key(n.get<std::string>()));
	return info;
}

UnnamedLookup LookUpUnnamed(const PackInfo& info, const std::string& key, bool fileInPack, bool chinese) {
	if (!chinese && info.samePcEn.count(key))
		return {false, true};
	return {fileInPack, info.samePc.count(key) > 0};
}

TwinFrom PickTwin(bool overlayServes, bool inUnnamedPack, bool samePc, bool inPcPack) {
	if (!overlayServes)
		return inPcPack ? TwinFrom::Pc : TwinFrom::None;
	if (inUnnamedPack)
		return TwinFrom::Unnamed;
	return samePc && inPcPack ? TwinFrom::Pc : TwinFrom::None;
}

bool HdWanted(std::optional<bool> setting, bool packPresent) {
	return packPresent && setting.value_or(true);
}

int64_t MipChainTexels(int w, int h) {
	int64_t total = 0;
	while (true) {
		total += static_cast<int64_t>(w) * h;
		if (w == 1 && h == 1)
			break;
		w = std::max(1, w / 2);
		h = std::max(1, h / 2);
	}
	return total;
}

int64_t PieceBytes(int pieceW, int pieceH, int scale, int bytesPerPixel, bool mipmaps) {
	const int w = pieceW * scale, h = pieceH * scale;
	const int64_t texels = mipmaps ? MipChainTexels(w, h) : static_cast<int64_t>(w) * h;
	return texels * bytesPerPixel;
}

bool BudgetAllows(int64_t liveBytes, int64_t addBytes, int64_t budgetBytes) {
	return budgetBytes <= 0 || liveBytes + addBytes <= budgetBytes;
}

std::optional<std::pair<int, int>> PngSize(const unsigned char* d, size_t size) {
	static const unsigned char kSig[8] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
	if (d == nullptr || size < 24 || !std::equal(kSig, kSig + 8, d) || !std::equal(d + 12, d + 16, "IHDR"))
		return std::nullopt;
	auto be32 = [](const unsigned char* p) {
		return static_cast<int64_t>(p[0]) << 24 | static_cast<int64_t>(p[1]) << 16 | static_cast<int64_t>(p[2]) << 8 | p[3];
	};
	const int64_t w = be32(d + 16), h = be32(d + 20);
	if (w <= 0 || h <= 0 || w > (1 << 16) || h > (1 << 16))
		return std::nullopt;
	return std::make_pair(static_cast<int>(w), static_cast<int>(h));
}

std::optional<std::pair<int, int>> ParseSize(std::string_view text) {
	const size_t x = text.find('x');
	if (x == std::string_view::npos)
		return std::nullopt;
	auto num = [](std::string_view s) -> int {
		if (s.empty() || s.size() > 6)
			return 0;
		int v = 0;
		for (char c : s) {
			if (c < '0' || c > '9')
				return 0;
			v = v * 10 + (c - '0');
		}
		return v;
	};
	const int w = num(text.substr(0, x)), h = num(text.substr(x + 1));
	if (w <= 0 || h <= 0)
		return std::nullopt;
	return std::make_pair(w, h);
}

} // namespace pvzu::hd
