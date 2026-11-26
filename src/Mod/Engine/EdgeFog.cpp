#include "Mod/Engine/EdgeFog.h"

#include "Lawn/Board.h"
#include "LawnApp.h"
#include "Resources.h"
#include "misc/ResourceManager.h"
#include "graphics/Graphics.h"
#include "graphics/Image.h"
#include "graphics/TriVertex.h"

#include <algorithm>
#include <cmath>

namespace pvzu::engine {

namespace {
constexpr float kSliceW = 8.0f;  // board px per alpha step inside the ramp (alpha is interpolated between vertices)
constexpr int kMaxTris = 1024;   // triangles per batch (one GL draw per batch)

struct TriBatch {
	Sexy::Graphics* g;
	Sexy::Image* image;
	Sexy::TriVertex tris[kMaxTris][3];
	int count = 0;

	void Flush() {
		if (count > 0)
			g->DrawTrianglesTex(image, tris, count);
		count = 0;
	}
	// A textured quad x0..x1 / y0..y1 with alpha a0 on the left edge and a1 on the right.
	void Quad(float x0, float x1, float y0, float y1, float u0, float u1, float v0, float v1, uint32_t rgb, float a0, float a1) {
		if (count + 2 > kMaxTris)
			Flush();
		const uint32_t c0 = (static_cast<uint32_t>(a0 * 255.0f + 0.5f) << 24) | rgb;
		const uint32_t c1 = (static_cast<uint32_t>(a1 * 255.0f + 0.5f) << 24) | rgb;
		Sexy::TriVertex tl(x0, y0, u0, v0, c0), tr(x1, y0, u1, v0, c1), br(x1, y1, u1, v1, c1), bl(x0, y1, u0, v1, c0);
		Sexy::TriVertex* t = tris[count];
		t[0] = tl; t[1] = tr; t[2] = br;
		t = tris[count + 1];
		t[0] = tl; t[1] = br; t[2] = bl;
		count += 2;
	}
};
TriBatch gBatch; // render thread only; ~100 KB kept out of the stack

// The fog cels live in the DelayLoad_Background4 group, which vanilla loads on Fog levels only (Sexy::IMAGE_FOG is null
// elsewhere): load just this image (and its HD twin, swapped at upload) on first use. Resources are never released
// unless the build is LOW_MEMORY; then ask the manager each time instead of keeping a pointer.
Sexy::Image* FogImage(Board* theBoard) {
#ifdef LOW_MEMORY
	return static_cast<Sexy::Image*>(theBoard->mApp->mResourceManager->LoadImage("IMAGE_FOG"));
#else
	static Sexy::Image* sImage = nullptr;
	if (sImage == nullptr)
		sImage = static_cast<Sexy::Image*>(theBoard->mApp->mResourceManager->LoadImage("IMAGE_FOG"));
	return sImage;
#endif
}
} // namespace

void DrawEdgeFog(Sexy::Graphics* g, Board* theBoard, const StreetFogRamp& theRamp, float theShift, int theRightX,
	bool theNight) {
	if (!theRamp.active || theRamp.clearX + theShift >= theRightX)
		return;
	const float aRampL = theRamp.clearX + theShift;  // cover alpha 0 left of here
	const float aRampR = theRamp.opaqueX + theShift; // and full right of here
	auto aAlpha = [&](float x) { return EdgeFogCoverAlpha(x, theRamp, theShift); };

	// A wash in the clouds' tone first, so the cover is opaque where the clouds thin out; it trails the clouds so the
	// inner edge keeps their shape. 2 px strips inside the ramp, one rect beyond it.
	const EdgeFogRgb aWashRgb = EdgeFogWashColor(theNight);
	Sexy::Color aWash(aWashRgb.r, aWashRgb.g, aWashRgb.b);
	const int aTop = -theBoard->mY - 20, aHeight = 600 + 40;
	const int aWashL = static_cast<int>(std::floor(aRampL)), aWashR = static_cast<int>(std::ceil(aRampR));
	for (int x = aWashL; x < std::min(aWashR, theRightX); x += 2) {
		const float a = StreetFogWashAlpha(x + 1 - theShift, theRamp);
		if (a <= 0.0f)
			continue;
		aWash.mAlpha = static_cast<int>(a * 255.0f + 0.5f);
		g->SetColor(aWash);
		g->FillRect(x, aTop, std::min(2, theRightX - x), aHeight);
	}
	if (aWashR < theRightX) {
		aWash.mAlpha = 255;
		g->SetColor(aWash);
		g->FillRect(aWashR, aTop, theRightX - aWashR, aHeight);
	}

	// The clouds: vanilla Board::DrawFog's cels and colouring on its grid (x * 80 - 15, y * 85 + 20) with no blow
	// offset. Each cel is cut into vertical strips only inside the ramp, with per-vertex alpha, so the clouds fade with
	// the ramp; all of it goes to the GPU as one batch of triangles (about 800 for a 1300-wide phone view).
	Sexy::Image* aImageFog = FogImage(theBoard);
	if (aImageFog == nullptr)
		return;
	const int aCelW = aImageFog->GetCelWidth();
	const float aImgW = static_cast<float>(aImageFog->GetWidth()), aImgH = static_cast<float>(aImageFog->GetHeight());
	constexpr uint32_t kFogAnimPeriod = 4500; // as vanilla: lcm of the 900 and 500 frame periods
	const float aTime = static_cast<float>(theBoard->mMainCounter % kFogAnimPeriod) * PI * 2;
	const int aFirstCol = static_cast<int>(std::floor((theRamp.clearX + 15 - aCelW) / 80.0f));
	gBatch.g = g;
	gBatch.image = aImageFog;
	gBatch.count = 0;
	float aCuts[64];
	for (int x = aFirstCol; x * 80 - 15 + theShift < theRightX; ++x) {
		const int aLookCol = ((x % MAX_GRID_SIZE_X) + MAX_GRID_SIZE_X) % MAX_GRID_SIZE_X;
		const float aPosX = x * 80 - 15 + theShift;
		// Cut points (board x) across this column's cels: the cel's edges and every kSliceW inside the ramp. Cels end at
		// the screen edge (theRightX), so a cover slid right is clipped there.
		const float aCelR = std::min(aPosX + aCelW, static_cast<float>(theRightX));
		int aNumCuts = 0;
		aCuts[aNumCuts++] = aPosX;
		for (float c = std::max(aRampL, aPosX + 1); c < std::min(aRampR, aCelR) && aNumCuts < 62; c += kSliceW)
			aCuts[aNumCuts++] = c;
		if (aRampR > aPosX && aRampR < aCelR)
			aCuts[aNumCuts++] = aRampR;
		aCuts[aNumCuts++] = aCelR;
		for (int y = 0; y < MAX_GRID_SIZE_Y + 1; ++y) {
			const int aCelLook = theBoard->mGridCelLook[aLookCol][y % MAX_GRID_SIZE_Y];
			const int aCelCol = aCelLook % 8;
			const float aPhaseX = 6 * PI * x / MAX_GRID_SIZE_X;
			const float aPhaseY = 6 * PI * y / (MAX_GRID_SIZE_Y + 1);
			const float aMotion = 13 + 4 * std::sin(aTime / 900 + aPhaseY) + 8 * std::sin(aTime / 500 + aPhaseX);
			const EdgeFogRgb aRgb = EdgeFogCelColor(theNight, aCelLook, aMotion);
			const uint32_t aRgbBits = (static_cast<uint32_t>(std::clamp(aRgb.r, 1, 255)) << 16) |
				(static_cast<uint32_t>(std::clamp(aRgb.g, 0, 255)) << 8) | static_cast<uint32_t>(std::clamp(aRgb.b, 0, 255));
			const Sexy::Rect aCel = aImageFog->GetCelRect(aCelCol, 0);
			const float aPosY = static_cast<float>(y * 85 + 20);
			const float aV0 = aCel.mY / aImgH, aV1 = (aCel.mY + aCel.mHeight) / aImgH;
			for (int i = 0; i + 1 < aNumCuts; ++i) {
				const float aX0 = aCuts[i], aX1 = aCuts[i + 1];
				if (aX1 <= aX0)
					continue;
				const float a0 = aAlpha(aX0), a1 = aAlpha(aX1);
				if (a0 <= 0.0f && a1 <= 0.0f)
					continue;
				gBatch.Quad(aX0, aX1, aPosY, aPosY + aCel.mHeight, (aCel.mX + aX0 - aPosX) / aImgW,
					(aCel.mX + aX1 - aPosX) / aImgW, aV0, aV1, aRgbBits, a0, a1);
			}
		}
	}
	gBatch.Flush();
}

} // namespace pvzu::engine
