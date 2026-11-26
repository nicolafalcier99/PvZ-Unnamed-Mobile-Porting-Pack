// PVZU: desktop-only helpers for scripted visual checks (-pvzu-shot, -pvzu-shot-frame, -pvzu-quit-frame,
// -pvzu-input). Frames are the app's update counter (100 per second of game time).
#pragma once

#include <string>
#include <vector>

namespace pvzu {

// "100,200" or "100-300/50" (range with step) or a mix; sorted, junk skipped.
std::vector<int> ParseFrameList(const std::string& text);

// Path for the shot taken at `frame`; with several frames "-<frame>" goes before ".png".
std::string ShotFileName(const std::string& base, int frame, bool manyFrames);

struct InputEvent {
	enum class Kind { Move, Down, Up, Text, Key };
	int frame = 0;
	Kind kind = Kind::Move;
	int x = 0, y = 0;   // logical coordinates
	std::string text;   // Text: characters; Key: key name ("return", "escape", "space", "up", "down", "left", "right", "a".."z")
	bool touch = false; // Down/Up from a finger (SDL's touch mouse) instead of a mouse
};

// One event per line: "<frame> move|down|up|click|tap|fdown|fmove|fup <x> <y>", "<frame> text <chars>", "<frame> key <name>".
// "click" is a press at <frame> and a release 5 frames later; "tap" is the same with a finger (touch-only behaviour,
// such as tooltips hidden on lift); "fdown" / "fmove" / "fup" <x> <y> hold, drag and lift a finger. '#' starts a comment. Sorted by frame (stable).
std::vector<InputEvent> ParseInputScript(const std::string& text, std::string* err);

} // namespace pvzu
