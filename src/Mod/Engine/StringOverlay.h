// PVZU: Unnamed text on top of the game's LawnStrings (only in Unnamed mode).
#pragma once

#include <filesystem>

namespace pvzu::engine {

std::filesystem::path UnnamedAssetsDir(); // <pack dir>/assets (filled by the Unnamed zip import)

// en: <pack>/strings/en.txt (keys that differ from vanilla). zh: Unnamed's own
// assets/properties/LawnStrings.txt (GBK, decoded). Missing files are logged and skipped.
void ApplyStringOverlays();

} // namespace pvzu::engine
