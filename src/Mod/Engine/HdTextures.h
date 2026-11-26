// PVZU: HD art in the engine (rules in Mod/HdArt.h). An image keeps its logical size, cels and CPU bits; only the
// pixels uploaded to its GL texture come from its HD twin. Twins are attached when a PC image is loaded from file
// (SexyAppBase::GetImage), built for reanim atlases from their parts' twins, and rendered on the GPU for the
// reanim caches (seed packet plants, almanac zombies). Any later change to an image's bits (BitsChanged) makes the
// twin stale, so modified images use their own pixels.
//
// Flags: -pvzu-hd <dir> (else <app data>/hd/ when it holds hd.json), -pvzu-hd-unnamed <dir> (Unnamed mode: twins of
// the overlay's own art, else <app data>/hd-unnamed/; see Mod/HdArt.h PickTwin), -pvzu-log-hd (memory per group, live/peak),
// -pvzu-hd-budget-mb <n> (default 700: once all live textures reach it, further images upload their PC pixels;
// 0 = no limit). pvzu-settings.json "hd": false turns HD off; unset = on whenever a pack is found.
#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace Sexy {
class Image;
class MemoryImage;
class Graphics;
}

namespace pvzu::hd {

struct Twin {
	std::string file;                   // pack file, "" for generated twins (atlas, GPU render)
	std::string key;
	int scale = 0;                      // HD texels per logical pixel (2 or 4)
	int attachedAt = -1;                // the image's mBitsChangedCount the twin matches
	int hdW = 0, hdH = 0;
	std::unique_ptr<uint32_t[]> bits;   // ARGB like MemoryImage::mBits; dropped once uploaded
	std::mutex mutex;
};

// Start-up (after the overlay is set): finds and indexes the pack, reads the setting, logs.
void Init();
bool Active();

// SexyAppBase::GetImage: a file image was just loaded with its final bits (ImageLib composed "_" alpha files).
void OnImageLoaded(Sexy::MemoryImage* theImage, const std::string& theFileName);
// After a load step that changed the bits into what the twin already shows (resources.xml alpha files, sanding):
// keeps the twin if it matched the bits before that step (bitsChangedCount was theCountBefore).
void KeepIfMatchedBefore(Sexy::MemoryImage* theImage, int theCountBefore);
// The load group being loaded (for -pvzu-log-hd); "" when outside a group.
void SetLoadGroup(const std::string& theGroup);

// The twin to upload for theImage now (valid, bits decoded, scale fits the max texture size), else nullptr.
Twin* TwinForUpload(Sexy::MemoryImage* theImage, int theMaxTextureSize);
// After the upload: frees the HD pixels (reloaded from the file if a texture must be made again).
void UploadDone(Twin* theTwin);

struct AtlasPart {
	Sexy::Image* image;
	int x, y, w, h;
};
// ReanimAtlas: composes the atlas twin from its parts' twins (parts without one are upscaled from their own bits).
void OnAtlasCreated(Sexy::MemoryImage* theAtlas, const std::vector<AtlasPart>& theParts);

// Reanim caches (ReanimatorCache): theDraw is the drawing that just filled theImage's bits; it is done again by the
// GPU at the pack's scale and kept as theImage's twin. Main thread with GL only; otherwise nothing happens.
void AttachRenderedTwin(Sexy::MemoryImage* theImage, const std::function<void(Sexy::Graphics*)>& theDraw);

// Texture accounting (GLInterface): bytes created/released; hd = the texture holds twin pixels.
void NoteTextureBytes(int64_t theBytes, bool theHd);
void LogSummary(const char* theWhen);

} // namespace pvzu::hd
