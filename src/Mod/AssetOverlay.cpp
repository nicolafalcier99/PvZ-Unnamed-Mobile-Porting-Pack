#include "Mod/AssetOverlay.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <string>
#include <unordered_map>
#include <utility>

namespace pvzu {
namespace fs = std::filesystem;

namespace {

std::optional<fs::path> gRoot;
std::unordered_map<std::string, fs::path> gIndex; // lower-case relative path -> real path

std::string Lower(std::string s) {
	std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	return s;
}

// "images\\a.png" / "./images/a.png" -> "images/a.png"; empty optional for unsafe names.
std::optional<std::string> Normalise(std::string_view name) {
	std::string s(name);
	std::replace(s.begin(), s.end(), '\\', '/');
	if (s.empty() || s[0] == '/' || (s.size() > 1 && s[1] == ':'))
		return std::nullopt;
	std::string out;
	size_t i = 0;
	while (i <= s.size()) {
		size_t j = s.find('/', i);
		if (j == std::string::npos) j = s.size();
		std::string part = s.substr(i, j - i);
		if (part == "..")
			return std::nullopt;
		if (!part.empty() && part != ".") {
			if (!out.empty()) out += '/';
			out += part;
		}
		i = j + 1;
	}
	if (out.empty())
		return std::nullopt;
	return out;
}

} // namespace

void SetOverlayRoot(std::optional<fs::path> root, const std::set<std::string>& skipLowerPaths) {
	gRoot = std::move(root);
	gIndex.clear();
	if (!gRoot)
		return;
	std::error_code ec;
	if (!fs::is_directory(*gRoot, ec))
		return;
	for (auto it = fs::recursive_directory_iterator(*gRoot, ec); !ec && it != fs::recursive_directory_iterator(); it.increment(ec)) {
		if (!it->is_regular_file(ec))
			continue;
		std::string rel = Lower(fs::relative(it->path(), *gRoot, ec).generic_string());
		if (rel.starts_with("properties/") || skipLowerPaths.count(rel))
			continue; // Unnamed's 1051-era resources.xml/LawnStrings would break GOTY; text goes via StringOverlay
		gIndex.emplace(std::move(rel), it->path());
	}
}

const std::optional<fs::path>& OverlayRoot() { return gRoot; }

std::optional<fs::path> OverlayCandidate(std::string_view gameName) {
	if (!gRoot)
		return std::nullopt;
	auto n = Normalise(gameName);
	if (!n)
		return std::nullopt;
	return *gRoot / fs::path(*n);
}

std::optional<fs::path> OverlayResolve(std::string_view gameName) {
	if (!gRoot)
		return std::nullopt;
	auto n = Normalise(gameName);
	if (!n)
		return std::nullopt;
	auto it = gIndex.find(Lower(*n));
	if (it == gIndex.end())
		return std::nullopt;
	return it->second;
}

std::set<std::string> LoadOverlaySkipList(const fs::path& file) {
	std::set<std::string> out;
	std::ifstream in(file);
	std::string line;
	while (std::getline(in, line)) {
		const auto b = line.find_first_not_of(" \t\r");
		if (b == std::string::npos || line[b] == '#')
			continue;
		const auto e = line.find_last_not_of(" \t\r");
		std::string p = line.substr(b, e - b + 1);
		std::replace(p.begin(), p.end(), '\\', '/');
		out.insert(Lower(p));
	}
	return out;
}

std::set<std::string> LoadOverlaySkipLists(const fs::path& dataDir, std::string_view language) {
	std::set<std::string> out = LoadOverlaySkipList(dataDir / "overlay-skip-always.txt");
	if (language != "zh")
		out.merge(LoadOverlaySkipList(dataDir / "overlay-en-skip.txt"));
	return out;
}

bool OverlayFileExists(std::string_view gameName) { return OverlayResolve(gameName).has_value(); }

} // namespace pvzu
