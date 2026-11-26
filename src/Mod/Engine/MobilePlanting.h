// PVZU: engine side of the mobile planting UX (2026-11-22; spec: the design notes (mobile-planting-animation)
// section 4; pure rules in Mod/TouchPlanting.h). Only where the HUD (Mod/Hud.h) is up (wide layout / phone) and not in
// demo recordings or replays; everywhere else every function here leaves vanilla alone.
#pragma once

namespace Sexy { class Graphics; }
class Board;
class SeedBank;

namespace pvzu::engine {

struct PacketRect { int x = 0, y = 0, w = 0, h = 0; };

// True where the touch planting rules and looks apply.
bool MobilePlantingActive(Board* theBoard);

// Board::MouseDown, first line: remember the press (finger down, where, what was held).
void MobileNoteDown(Board* theBoard, int x, int y);
// Board::MouseDown before vanilla's hit test: true when the press was handled here (Board::MouseDown returns).
bool MobileMouseDown(Board* theBoard, int x, int y, int theClickCount);
// Board::MouseUp, first line: plant / dig / put back where the finger lifts.
void MobileMouseUp(Board* theBoard, int x, int y);

// SeedPacket::Draw: the held packet is drawn as ready (full brightness, no recharge layer).
bool DrawPacketAsHeld(Board* theBoard, int theIndex);
// CursorPreview::Update: the see-through plant in the target tile shows only while a finger is down.
bool TilePreviewAllowed(Board* theBoard);
// Board::DrawUITop: false hides the held image; theLiftY is added to its y (the plant floats above the finger).
bool HeldImageShown(Board* theBoard, int& theLiftY);

// Board::DrawBackdrop, last: the crosshair (row + column wash) under the finger, over the ground.
void DrawPlantingCrosshair(Sexy::Graphics* g, Board* theBoard);
// Board::DrawUIBottom, after the seed bank and shovel: corner brackets around the held packet or shovel.
void DrawSelectionBrackets(Sexy::Graphics* g, Board* theBoard);

// Board-coordinate rect of seed packet theIndex as drawn (WideScreen's SeedPacketScreenRect, minus the board's
// position): a wide card or a tall packet.
PacketRect SelectedPacketRect(Board* theBoard, int theIndex);

} // namespace pvzu::engine
