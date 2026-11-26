#include "Mod/Engine/StringOverlay.h"
#include "Mod/Engine/Startup.h"
#include "Mod/Mode.h"
#include "Mod/TextDecode.h"

#include "PvzpLib/PvzpDebug.h"
#include "PvzpLib/PvzpStringFile.h"

#include <algorithm>
#include <fstream>
#include <vector>
#include <sstream>

namespace pvzu::engine {

namespace {
bool LoadOverlayFile(const std::filesystem::path& file) {
	std::ifstream in(file, std::ios::binary);
	if (!in) {
		PvzpLogLn("PVZU: text overlay not found: {}", file.string());
		return false;
	}
	std::stringstream buf;
	buf << in.rdbuf();
	const std::string text = DecodeTextFile(buf.str());
	const bool ok = PvzpStringListReadItems(text.c_str());
	PvzpLogLn("PVZU: text overlay {} {}", file.string(), ok ? "loaded" : "FAILED to parse");
	return ok;
}
}

std::filesystem::path UnnamedAssetsDir() { return UnnamedPackDir() / "assets"; }

void ApplyStringOverlays() {
	if (CurrentMode() != GameMode::Unnamed)
		return;
	if (StartupSettings().language == "zh")
		LoadOverlayFile(UnnamedAssetsDir() / "properties" / "LawnStrings.txt");
	else {
		LoadOverlayFile(UnnamedPackDir() / "strings" / "en.txt");
		// strings/en-corrections-*.txt: per-area fixes where Unnamed's code differs from its own Almanac (code wins)
		std::vector<std::filesystem::path> aFixes;
		std::error_code ec;
		for (const auto& e : std::filesystem::directory_iterator(UnnamedPackDir() / "strings", ec)) {
			const std::string fn = e.path().filename().string();
			if (fn.starts_with("en-corrections-") && fn.ends_with(".txt"))
				aFixes.push_back(e.path());
		}
		std::sort(aFixes.begin(), aFixes.end());
		for (const auto& f : aFixes)
			LoadOverlayFile(f);
	}
}

} // namespace pvzu::engine
