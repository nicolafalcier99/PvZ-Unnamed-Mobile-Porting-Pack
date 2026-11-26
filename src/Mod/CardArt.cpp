#include "Mod/CardArt.h"

#include <algorithm>
#include <cmath>
#include <iterator>

namespace pvzu {

namespace {
struct Rgb { float r, g, b; };
struct Px4 { Rgb c{0, 0, 0}; float a = 0.0f; };

// Colours measured from the reference card (bright state; greying multiplies them as vanilla does).
constexpr Rgb kOutline{66, 80, 72};
constexpr Rgb kRimEdge{196, 207, 201};   // rim at its inner and outer edges
constexpr Rgb kRimLight{255, 255, 252};  // rim's lit middle
constexpr Rgb kHairline{120, 146, 136};  // between rim and panel
constexpr Rgb kShadow{18, 26, 20};
constexpr float kShadowAlpha = 0.45f;
constexpr float kHairlineW = 0.6f;
// Panel: pale aqua, a whiter band a third of the way down, a touch greener at the bottom.
struct Stop { float at; Rgb c; };
constexpr Stop kPanel[] = {
	{0.00f, {176, 246, 230}},
	{0.32f, {232, 254, 250}},
	{0.62f, {198, 251, 233}},
	{1.00f, {182, 238, 214}},
};
constexpr Rgb kTabTop{255, 255, 255};
constexpr Rgb kTabBottom{232, 238, 235};
constexpr Rgb kTabEdge{98, 116, 106};
constexpr float kTabEdgeW = 0.8f;

Rgb Mix(const Rgb& a, const Rgb& b, float t) {
	return {a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t};
}
Rgb Scale(const Rgb& a, float m) { return {a.r * m, a.g * m, a.b * m}; }
Rgb PanelAt(float v) {
	v = std::clamp(v, 0.0f, 1.0f);
	for (size_t i = 1; i < std::size(kPanel); ++i)
		if (v <= kPanel[i].at)
			return Mix(kPanel[i - 1].c, kPanel[i].c, (v - kPanel[i - 1].at) / (kPanel[i].at - kPanel[i - 1].at));
	return kPanel[std::size(kPanel) - 1].c;
}

// Signed distance (logical px, negative inside) to a box with per-corner radii.
struct RBox { float x0, y0, x1, y1, tl, tr, br, bl; };
RBox Rounded(const Box& b, float r, float inset = 0.0f) {
	const float ri = std::max(0.0f, r - inset);
	return {b.x + inset, b.y + inset, b.x + b.w - inset, b.y + b.h - inset, ri, ri, ri, ri};
}
float Sd(const RBox& b, float px, float py) {
	const float cx = (b.x0 + b.x1) * 0.5f, cy = (b.y0 + b.y1) * 0.5f;
	const float hx = (b.x1 - b.x0) * 0.5f, hy = (b.y1 - b.y0) * 0.5f;
	const float qx = px - cx, qy = py - cy;
	const float r = qx < 0 ? (qy < 0 ? b.tl : b.bl) : (qy < 0 ? b.tr : b.br);
	const float dx = std::fabs(qx) - hx + r, dy = std::fabs(qy) - hy + r;
	const float ox = std::max(dx, 0.0f), oy = std::max(dy, 0.0f);
	return std::min(std::max(dx, dy), 0.0f) + std::sqrt(ox * ox + oy * oy) - r;
}
float Smooth(float e0, float e1, float x) {
	const float t = std::clamp((x - e0) / (e1 - e0), 0.0f, 1.0f);
	return t * t * (3.0f - 2.0f * t);
}

void Over(Px4& dst, const Rgb& c, float a) {
	if (a <= 0.0f)
		return;
	const float aOut = a + dst.a * (1.0f - a);
	const float w = dst.a * (1.0f - a);
	dst.c = {(c.r * a + dst.c.r * w) / aOut, (c.g * a + dst.c.g * w) / aOut, (c.b * a + dst.c.b * w) / aOut};
	dst.a = aOut;
}
uint32_t Pack(const Px4& p) {
	auto aByte = [](float v) { return static_cast<uint32_t>(std::clamp(std::lround(v), 0L, 255L)); };
	return aByte(p.a * 255.0f) << 24 | aByte(p.c.r) << 16 | aByte(p.c.g) << 8 | aByte(p.c.b);
}

template <typename F>
ArgbImage Render(const Box& area, int scale, F&& shade) {
	ArgbImage img;
	if (area.w <= 0 || area.h <= 0 || scale <= 0)
		return img;
	img.w = area.w * scale;
	img.h = area.h * scale;
	img.px.resize(static_cast<size_t>(img.w) * img.h);
	const float s = static_cast<float>(scale);
	for (int y = 0; y < img.h; ++y)
		for (int x = 0; x < img.w; ++x)
			img.px[static_cast<size_t>(y) * img.w + x] = Pack(shade(area.x + (x + 0.5f) / s, area.y + (y + 0.5f) / s, s));
	return img;
}
// Anti-aliased coverage of a shape at distance d, one device px wide.
float Cover(float d, float s) { return std::clamp(0.5f - d * s, 0.0f, 1.0f); }
} // namespace

ArgbImage RenderCardBody(const PanelCardParts& p, int scale) {
	const float k = p.k;
	const float aRadius = kCardRadius * k;
	const float aOutline = kCardOutline * k;
	const float aRim = static_cast<float>(p.panel.y - p.frame.y) - aOutline;
	const float aBlur = 2.5f * k;
	const RBox aFrame = Rounded(p.frame, aRadius);
	const RBox aPanelIn = Rounded(p.panel, std::max(1.0f, aRadius - (p.panel.y - p.frame.y)));
	RBox aShadow = aFrame;
	aShadow.x0 += p.shadowDx; aShadow.x1 += p.shadowDx;
	aShadow.y0 += p.shadowDy; aShadow.y1 += p.shadowDy;

	return Render(p.art, scale, [&](float x, float y, float s) {
		Px4 o;
		// Soft shadow (only where the card does not cover it).
		const float ds = Sd(aShadow, x, y);
		Over(o, kShadow, kShadowAlpha * (1.0f - Smooth(-0.5f * aBlur, aBlur, ds)));
		const float d = Sd(aFrame, x, y);
		Over(o, kOutline, Cover(d, s));
		// Rim, lit like a rounded tube (lightest in its middle), a little darker towards the bottom of the card.
		const float aDepth = std::clamp((-d - aOutline) / std::max(aRim, 0.01f), 0.0f, 1.0f);
		const float aTube = std::pow(std::max(0.0f, std::sin(aDepth * 3.14159265f)), 0.7f); // sin(pi) < 0: NaN guard
		const float aDown = (y - p.frame.y) / std::max(1, p.frame.h);
		Over(o, Scale(Mix(kRimEdge, kRimLight, aTube), 1.0f - 0.07f * aDown), Cover(d + aOutline, s));
		// Panel with a hairline at its edge.
		const float dp = Sd(aPanelIn, x, y);
		const float aPanelCover = Cover(dp, s);
		Over(o, PanelAt((y - p.panel.y) / std::max(1, p.panel.h)), aPanelCover);
		const float aHair = aPanelCover - Cover(dp + kHairlineW * k, s);
		Over(o, kHairline, std::max(0.0f, aHair) * 0.85f);
		return o;
	});
}

ArgbImage RenderCardTab(const PanelCardParts& p, int scale) {
	if (p.tab.w <= 0 || p.tab.h <= 0)
		return {};
	const float k = p.k;
	const float aRadius = kCardRadius * k;
	const float aOutline = kCardOutline * k;
	const float aEdgeW = kTabEdgeW * k;
	// Inside of the frame's outline: the tab never covers the outline.
	const RBox aInside = Rounded(p.frame, aRadius, aOutline);
	// The tab: round top-left; its right and bottom run past the frame (cut by aInside).
	const RBox aTab{static_cast<float>(p.tab.x), static_cast<float>(p.tab.y), p.tab.x + p.tab.w + 20.0f,
		p.tab.y + p.tab.h + 20.0f, p.tabRadius, 0.0f, 0.0f, 0.0f};
	return Render(p.tab, scale, [&](float x, float y, float s) {
		Px4 o;
		const float dt = Sd(aTab, x, y), di = Sd(aInside, x, y);
		const float aCover = Cover(std::max(dt, di), s);
		const float aDown = (y - p.tab.y) / std::max(1, p.tab.h);
		Over(o, Mix(kTabTop, kTabBottom, aDown), aCover);
		// Hairline on the tab's edge over the panel only: where it meets the rim, tab and rim are one white shape.
		const float aOverPanel = Cover(y - (p.panel.y + p.panel.h), s) * Cover(x - (p.panel.x + p.panel.w), s);
		const float aEdge = (aCover - Cover(std::max(dt + aEdgeW, di), s)) * aOverPanel;
		Over(o, kTabEdge, std::max(0.0f, aEdge));
		return o;
	});
}

} // namespace pvzu
