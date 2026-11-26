#include "Mod/Touch.h"

#include <algorithm>
#include <cstdlib>

namespace pvzu {

HitRect ExpandHitRect(HitRect r, int minSize) {
	if (r.w < minSize) {
		r.x -= (minSize - r.w) / 2;
		r.w = minSize;
	}
	if (r.h < minSize) {
		r.y -= (minSize - r.h) / 2;
		r.h = minSize;
	}
	return r;
}

namespace {
bool Overlap(const HitRect& a, const HitRect& b) {
	return a.x < b.x + b.w && b.x < a.x + a.w && a.y < b.y + b.h && b.y < a.y + a.h;
}
// Cut grown rects a/b apart at the midpoint of the gap between their originals oa/ob.
void Separate(HitRect& a, HitRect& b, const HitRect& oa, const HitRect& ob) {
	if (oa.x + oa.w <= ob.x || ob.x + ob.w <= oa.x) {
		HitRect& l = oa.x < ob.x ? a : b;
		HitRect& r = oa.x < ob.x ? b : a;
		const HitRect& ol = oa.x < ob.x ? oa : ob;
		const HitRect& orr = oa.x < ob.x ? ob : oa;
		const int mid = (ol.x + ol.w + orr.x) / 2;
		l.w = std::max(ol.x + ol.w, mid) - l.x;
		const int right = r.x + r.w;
		r.x = std::min(orr.x, mid);
		r.w = right - r.x;
	} else {
		HitRect& t = oa.y < ob.y ? a : b;
		HitRect& u = oa.y < ob.y ? b : a;
		const HitRect& ot = oa.y < ob.y ? oa : ob;
		const HitRect& ou = oa.y < ob.y ? ob : oa;
		const int mid = (ot.y + ot.h + ou.y) / 2;
		t.h = std::max(ot.y + ot.h, mid) - t.y;
		const int bottom = u.y + u.h;
		u.y = std::min(ou.y, mid);
		u.h = bottom - u.y;
	}
}
} // namespace

std::vector<HitRect> ExpandHitRects(const std::vector<HitRect>& rects, int minSize) {
	std::vector<HitRect> out;
	out.reserve(rects.size());
	for (const HitRect& r : rects)
		out.push_back(ExpandHitRect(r, minSize));
	for (size_t i = 0; i < out.size(); ++i)
		for (size_t j = i + 1; j < out.size(); ++j)
			if (Overlap(out[i], out[j]))
				Separate(out[i], out[j], rects[i], rects[j]);
	return out;
}

std::vector<MouseStep> TouchToMouse(TouchPhase phase, int x, int y) {
	switch (phase) {
	case TouchPhase::Down:
		return {{MouseStep::Kind::Move, x, y}, {MouseStep::Kind::Down, x, y}};
	case TouchPhase::Move:
		return {{MouseStep::Kind::Move, x, y}};
	case TouchPhase::Up:
		return {{MouseStep::Kind::Move, x, y}, {MouseStep::Kind::Up, x, y}, {MouseStep::Kind::Leave, x, y}};
	}
	return {};
}

bool HoverEndsOnLift(PointerSource source) {
	return source == PointerSource::Finger || source == PointerSource::Pen;
}

namespace {
PointerSource gLastPointer = PointerSource::Mouse;
}

void NotePointer(PointerSource source) { gLastPointer = source; }

PointerSource LastPointer() { return gLastPointer; }

bool FollowsPointer(PointerSource source, bool lawnPressHeld) {
	return source == PointerSource::Mouse || lawnPressHeld;
}

bool DragShouldPlantOnRelease(bool pickedThisPress, int downX, int downY, int upX, int upY, int minTravel) {
	if (!pickedThisPress)
		return false;
	return std::abs(upX - downX) + std::abs(upY - downY) >= minTravel;
}

} // namespace pvzu
