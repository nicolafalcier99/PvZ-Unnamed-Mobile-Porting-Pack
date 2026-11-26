#include "doctest/doctest.h"
#include "Mod/CardArt.h"
#include "Mod/Hud.h"

using namespace pvzu;

namespace {
constexpr int kScale = 2; // device px per logical px in these checks
struct Rgba { int r, g, b, a; };
Rgba At(const ArgbImage& img, float lx, float ly, const Box& origin) {
	// Pixel at logical (lx, ly) in card coordinates; the image covers `origin`.
	const int x = static_cast<int>((lx - origin.x) * kScale), y = static_cast<int>((ly - origin.y) * kScale);
	REQUIRE(x >= 0);
	REQUIRE(y >= 0);
	REQUIRE(x < img.w);
	REQUIRE(y < img.h);
	const uint32_t c = img.px[static_cast<size_t>(y) * img.w + x];
	return {static_cast<int>((c >> 16) & 255), static_cast<int>((c >> 8) & 255), static_cast<int>(c & 255),
		static_cast<int>(c >> 24)};
}
int Lum(const Rgba& c) { return (c.r * 3 + c.g * 6 + c.b) / 10; }
PanelCardParts Card() { return PanelCardLayout(kCardW, kCardH, 18, 9); }
} // namespace

TEST_CASE("CardBodySizeCoversTheArtBox") {
	const PanelCardParts p = Card();
	const ArgbImage img = RenderCardBody(p, kScale);
	CHECK(img.w == p.art.w * kScale);
	CHECK(img.h == p.art.h * kScale);
	CHECK(img.px.size() == static_cast<size_t>(img.w) * img.h);
}

TEST_CASE("CardBodyRoundedCornersAndPaleAquaPanel") {
	const PanelCardParts p = Card();
	const ArgbImage img = RenderCardBody(p, kScale);
	const Box& f = p.frame;
	// The very corner of the frame is outside the rounded outline (only, at most, faint shadow).
	CHECK(At(img, f.x + 0.25f, f.y + 0.25f, p.art).a < 40);
	// Panel middle: opaque pale aqua (green and blue well above red, all light).
	const Rgba c = At(img, p.panel.x + p.panel.w / 2.0f, p.panel.y + p.panel.h / 2.0f, p.art);
	CHECK(c.a == 255);
	CHECK(c.g > 220);
	CHECK(c.b > 190);
	CHECK(c.r < c.g - 20);
	// Rim (between outline and panel, middle of the top edge): light, near white.
	const Rgba rim = At(img, f.x + f.w / 2.0f, f.y + kCardOutline + kCardRim / 2.0f, p.art);
	CHECK(rim.a == 255);
	CHECK(Lum(rim) > 225);
	// Outline: dark.
	const Rgba line = At(img, f.x + f.w / 2.0f, f.y + 0.5f, p.art);
	CHECK(line.a > 200);
	CHECK(Lum(line) < 110);
}

TEST_CASE("CardBodyRimHasABevel") {
	// The rim is lit like a rounded tube: its middle is lighter than its inner edge.
	const PanelCardParts p = Card();
	const ArgbImage img = RenderCardBody(p, kScale);
	const float x = p.frame.x + p.frame.w / 2.0f;
	const int aMid = Lum(At(img, x, p.frame.y + kCardOutline + kCardRim * 0.45f, p.art));
	const int aInner = Lum(At(img, x, p.frame.y + kCardOutline + kCardRim - 0.3f, p.art));
	CHECK(aMid > aInner);
}

TEST_CASE("CardBodySoftShadowBelow") {
	const PanelCardParts p = Card();
	const ArgbImage img = RenderCardBody(p, kScale);
	const float x = p.frame.x + p.frame.w / 2.0f;
	const float yBelow = p.frame.y + p.frame.h + 1.0f;
	const Rgba s = At(img, x, yBelow, p.art);
	CHECK(s.a > 40);
	CHECK(s.a < 200);
	CHECK(Lum(s) < 60);
	// Fades out further down.
	CHECK(At(img, x, p.art.y + p.art.h - 0.25f, p.art).a < s.a);
	// Nothing above the card.
	CHECK(At(img, x, p.art.y + 0.25f, p.art).a < 10);
}

TEST_CASE("CardTabWhiteWithRoundTopLeft") {
	const PanelCardParts p = Card();
	const ArgbImage img = RenderCardTab(p, kScale);
	const Box& t = p.tab;
	CHECK(img.w == t.w * kScale);
	CHECK(img.h == t.h * kScale);
	// The top-left corner is cut by the big radius.
	CHECK(At(img, t.x + 0.5f, t.y + 0.5f, t).a == 0);
	// Its inside (under the digits) is opaque, near white.
	const Rgba c = At(img, t.x + t.w * 0.6f, t.y + t.h * 0.6f, t);
	CHECK(c.a == 255);
	CHECK(Lum(c) > 235);
	// The frame's outline stays visible: the tab never covers the outermost px of the frame.
	CHECK(At(img, t.x + t.w - 0.25f, t.y + t.h * 0.5f, t).a == 0);
	CHECK(At(img, t.x + t.w * 0.6f, t.y + t.h - 0.25f, t).a == 0);
	// A hairline edge on the curve: darker than the fill, on the top edge right of the corner.
	const Rgba edge = At(img, t.x + t.w * 0.7f, t.y + 0.25f, t);
	CHECK(edge.a > 100);
	CHECK(Lum(edge) < Lum(c) - 60);
}
