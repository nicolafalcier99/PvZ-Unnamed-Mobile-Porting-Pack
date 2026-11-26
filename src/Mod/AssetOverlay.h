// PVZU: an asset folder (imported Unnamed files) that takes precedence over main.pak and loose files.
// Never serves properties/* (Unnamed's 1051-era resources.xml and Chinese LawnStrings would break GOTY).
// Lookups are case-insensitive like the game's own (it asks for "IMAGES/zombie.PNG" style names).
#pragma once

#include <filesystem>
#include <optional>
#include <set>
#include <string>
#include <string_view>

namespace pvzu {

// Sets (and indexes) the overlay root; nullopt turns the overlay off (vanilla).
// skipLowerPaths: lower-case relative paths never served (English mode skips Unnamed's translated UI art).
void SetOverlayRoot(std::optional<std::filesystem::path> root, const std::set<std::string>& skipLowerPaths = {});
// One relative path per line ('#' comments, blank lines ignored); returns lower-case paths. Missing file -> empty.
std::set<std::string> LoadOverlaySkipList(const std::filesystem::path& file);
// The pack's data folder: overlay-skip-always.txt (art made for Unnamed's own 1067-wide exe layout) in every
// language, merged with overlay-en-skip.txt (translated UI art) unless language is "zh".
std::set<std::string> LoadOverlaySkipLists(const std::filesystem::path& dataDir, std::string_view language);
const std::optional<std::filesystem::path>& OverlayRoot();

// root/<normalised name> or nullopt (no overlay, empty name, absolute path, or ".." escape).
std::optional<std::filesystem::path> OverlayCandidate(std::string_view gameName);
// The real file in the overlay matching gameName case-insensitively, if any.
std::optional<std::filesystem::path> OverlayResolve(std::string_view gameName);
bool OverlayFileExists(std::string_view gameName);

} // namespace pvzu
