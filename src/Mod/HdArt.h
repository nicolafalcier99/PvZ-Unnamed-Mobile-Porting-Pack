// PVZU: HD art. An "HD pack" (a folder with hd.json, made by scripts/pvzu-hd-prepare.py from AI-upscaled copies of
// the player's own PC images; private, never in git) holds a twin of each PC image at a whole factor (2x or 4x).
// The engine uploads the twin's pixels to the image's GL texture while the image keeps its logical size, cels and
// CPU bits, so layout, reanim transforms, hit tests and pixel reads behave as before. This file is the pure part
// (names, scale and UV math, rules, settings, budget); the engine glue is Mod/Engine/HdTextures.
#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

namespace pvzu::hd {

// Game image name -> pack key: lower case, '/' separators, no leading "./", image extension (.png .jpg .jpeg
// .gif .bmp .tga) dropped. "IMAGES\\Sod1Row.PNG" -> "images/sod1row".
std::string Key(std::string_view gameName);
// A file inside the pack (relative path) -> its key; "" when it is not a .png.
std::string KeyForPackFile(std::string_view relPath);
// "<dir>/_<name>" and "<dir>/<name>_": the separate alpha images ImageLib composes into "<dir>/<name>".
std::vector<std::string> AlphaCompanionKeys(std::string_view key);
bool IsAlphaCompanionKey(std::string_view key);

// The whole factor (2 or 4) when the twin is exactly that many times the logical size on both axes, else 0.
int ScaleFor(int logicalW, int logicalH, int hdW, int hdH);
// Half a texel of a piece logicalSize wide whose texture holds scale texels per logical pixel (UV clamp inset).
float HalfTexel(int pieceLogical, int scale);
// HD texel index of the left/top edge of logical pixel x.
inline int HdTexelEdge(int x, int scale) { return x * scale; }
struct IntRect { int x = 0, y = 0, w = 0, h = 0; };
IntRect ScaleRect(const IntRect& r, int scale);
// Largest logical piece edge whose HD texture still fits the GPU's max texture size.
int MaxLogicalPiece(int maxTextureSize, int scale);

// True when Unnamed's overlay serves the image or one of its alpha companions (any image extension).
// exists(lowerRelativePathWithExtension).
bool OverlayServes(std::string_view key, const std::function<bool(const std::string&)>& exists);
// Which pack an image's twin comes from. Art the Unnamed overlay serves: the hd-unnamed pack's file, else the PC
// pack's twin when hd-unnamed lists the key in samePc (the Unnamed image is pixel-identical to PC), else none (a
// PC-based twin would show the wrong art). Art the overlay does not serve: the PC pack.
enum class TwinFrom { None, Pc, Unnamed };
TwinFrom PickTwin(bool overlayServes, bool inUnnamedPack, bool samePc, bool inPcPack);
// GPU-rendered twins (reanim caches) use the PC pack's scale, else the hd-unnamed pack's; 0 = none.
inline int RenderScale(int pcScale, int unnamedScale) { return pcScale ? pcScale : unnamedScale; }
// A twin attached when the image's bits-changed count was attachedAt matches only while the count is unchanged.
inline bool TwinValid(int attachedAt, int bitsChangedCount) { return attachedAt >= 0 && attachedAt == bitsChangedCount; }

struct PackInfo {
	int scale = 0;
	int count = 0;
	std::string source;
	std::unordered_set<std::string> samePc;   // hd-unnamed only: keys whose Unnamed image equals the PC one
	std::unordered_set<std::string> samePcEn; // hd-unnamed only: equal to PC outside Chinese mode (a piece of the
	                                          // image is on overlay-en-skip.txt); the pack's file is the Chinese one
};
// The hd-unnamed pack's answer for key in the current language (feeds PickTwin's inUnnamedPack and samePc).
struct UnnamedLookup {
	bool inPack = false;
	bool samePc = false;
};
UnnamedLookup LookUpUnnamed(const PackInfo& info, const std::string& key, bool fileInPack, bool chinese);
// hd.json: {"scale":2|4, "count":N, "source":"...", "samePc":[names]}; nullopt when broken or the scale is
// unsupported. samePc names are normalised with Key(); a samePc that is not a list is ignored.
std::optional<PackInfo> ParsePackInfo(std::string_view json);
// pvzu-settings.json "hd": unset -> on when a pack is present; false -> off; true -> on if a pack is present.
bool HdWanted(std::optional<bool> setting, bool packPresent);

// Texels of a full mip chain from w x h down to 1 x 1.
int64_t MipChainTexels(int w, int h);
// GPU bytes of one texture piece laid out pieceW x pieceH logical pixels, at scale, bytesPerPixel, with mipmaps.
int64_t PieceBytes(int pieceW, int pieceH, int scale, int bytesPerPixel, bool mipmaps);
inline double Megabytes(int64_t bytes) { return static_cast<double>(bytes) / (1024.0 * 1024.0); }
// Whether adding addBytes to liveBytes stays within budgetBytes (0 = no budget).
bool BudgetAllows(int64_t liveBytes, int64_t addBytes, int64_t budgetBytes);

// Width and height from a PNG file's first 24 bytes (signature + IHDR); nullopt when not a PNG.
std::optional<std::pair<int, int>> PngSize(const unsigned char* data, size_t size);

// "2340x1080" -> {2340, 1080}; nullopt unless both are positive integers.
std::optional<std::pair<int, int>> ParseSize(std::string_view text);

} // namespace pvzu::hd
