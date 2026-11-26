// PVZU: cut an image region into 9 parts (corners keep size, edges/centre stretch) or into 1:1 tiles.
// Used to build the vertical seed panel from the original horizontal SeedBank art.
#pragma once

#include <vector>

namespace pvzu {

struct Box { int x = 0, y = 0, w = 0, h = 0; };
struct Piece { Box src; Box dst; };

// TL, T, TR, L, C, R, BL, B, BR. `inset` = corner/edge thickness (same in src and dst).
std::vector<Piece> NineSlice(const Box& src, int inset, const Box& dst);
// Tiles `src` over `dst` 1:1 (no stretching); the last row/column is cropped.
std::vector<Piece> TileRects(const Box& src, const Box& dst);

} // namespace pvzu
