#include "Mod/NineSlice.h"

#include <algorithm>

namespace pvzu {

std::vector<Piece> NineSlice(const Box& s, int k, const Box& d) {
	const int sx[3] = {s.x, s.x + k, s.x + s.w - k};
	const int sw[3] = {k, s.w - 2 * k, k};
	const int sy[3] = {s.y, s.y + k, s.y + s.h - k};
	const int sh[3] = {k, s.h - 2 * k, k};
	const int dx[3] = {d.x, d.x + k, d.x + d.w - k};
	const int dw[3] = {k, d.w - 2 * k, k};
	const int dy[3] = {d.y, d.y + k, d.y + d.h - k};
	const int dh[3] = {k, d.h - 2 * k, k};
	std::vector<Piece> out;
	for (int r = 0; r < 3; ++r)
		for (int c = 0; c < 3; ++c)
			out.push_back({Box{sx[c], sy[r], sw[c], sh[r]}, Box{dx[c], dy[r], dw[c], dh[r]}});
	return out;
}

std::vector<Piece> TileRects(const Box& s, const Box& d) {
	std::vector<Piece> out;
	if (s.w <= 0 || s.h <= 0)
		return out;
	for (int y = 0; y < d.h; y += s.h)
		for (int x = 0; x < d.w; x += s.w) {
			const int w = std::min(s.w, d.w - x);
			const int h = std::min(s.h, d.h - y);
			out.push_back({Box{s.x, s.y, w, h}, Box{d.x + x, d.y + y, w, h}});
		}
	return out;
}

} // namespace pvzu
