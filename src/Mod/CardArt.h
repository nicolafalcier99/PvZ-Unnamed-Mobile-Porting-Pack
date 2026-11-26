// PVZU: procedural art for the seed card (Mod/PanelCard.h): the frame (soft shadow, hairline outline, bevelled white
// rim, pale aqua panel) and the white cost tab, rendered anti-aliased into ARGB pixels at `scale` device px per
// logical px. No EA art: shapes are signed-distance rounded rectangles, colours measured from the reference.
// SeedPacket.cpp turns them into cached images, so the vanilla greying/recharge (colourised drawing) still applies.
#pragma once

#include "Mod/PanelCard.h"

#include <cstdint>
#include <vector>

namespace pvzu {

struct ArgbImage {
	int w = 0, h = 0;
	std::vector<uint32_t> px; // 0xAARRGGBB, not premultiplied, row by row
};

// The frame, covering p.art (so drawn at p.art, p.art.w x p.art.h logical px).
ArgbImage RenderCardBody(const PanelCardParts& p, int scale);
// The cost tab, covering p.tab; transparent outside the frame's outline. Empty if p has no tab.
ArgbImage RenderCardTab(const PanelCardParts& p, int scale);

} // namespace pvzu
