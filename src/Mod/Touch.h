// PVZU: touch fit (spec item 5: larger hit areas for seed packets and shovel, drag-to-plant, taps that do not
// leave buttons looking pressed). Pure helpers; the engine uses them in Input.cpp, SeedPacket.cpp, Board.cpp.
#pragma once

#include <vector>

namespace pvzu {

struct HitRect { int x = 0, y = 0, w = 0, h = 0; };

// Grow a hit area to at least minSize x minSize logical px, centred on the original; never shrinks.
HitRect ExpandHitRect(HitRect r, int minSize);
// Grow each rect, then cut overlaps between neighbours at the midpoint of their gap, so taps between two
// packets go to the nearer one.
std::vector<HitRect> ExpandHitRects(const std::vector<HitRect>& rects, int minSize);

constexpr int kMinTouchSize = 64;      // logical px (about 9 mm on the S26 Ultra)
constexpr int kTouchCoinRadius = 40;   // sun and coins collect within this distance of the touch

// What the widget manager gets for one finger event.
enum class TouchPhase { Down, Move, Up };
struct MouseStep {
	enum class Kind { Move, Down, Up, Leave };
	Kind kind = Kind::Move;
	int x = 0, y = 0;
};
std::vector<MouseStep> TouchToMouse(TouchPhase phase, int x, int y);

// What sent a mouse event: SDL marks the mouse events it makes from fingers (SDL_TOUCH_MOUSEID) and pens
// (SDL_PEN_MOUSEID). Input.cpp: when HoverEndsOnLift, the release is followed by a window leave, so no widget stays
// hovered and no tooltip stays up (S26 bug lists 2026-11-22/23); a mouse keeps hovering.
enum class PointerSource { Mouse, Finger, Pen };
bool HoverEndsOnLift(PointerSource source);
// The source of the last pointer motion or press (Input.cpp notes it); Mouse until something is noted.
void NotePointer(PointerSource source);
PointerSource LastPointer();
// Follow-the-pointer modes (0x78, 0x8b): a mouse is followed always; a finger or pen only while a press that began on
// bare lawn is held (a tap on a sun or packet must not teleport the followed object).
bool FollowsPointer(PointerSource source, bool lawnPressHeld);

// Drag-to-plant: plant on release when this press picked up the packet and the finger travelled.
bool DragShouldPlantOnRelease(bool pickedThisPress, int downX, int downY, int upX, int upY, int minTravel);

} // namespace pvzu
