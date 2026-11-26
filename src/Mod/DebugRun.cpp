#include "Mod/DebugRun.h"

#include <algorithm>
#include <charconv>
#include <sstream>
#include <string_view>

namespace pvzu {

namespace {
bool ToInt(std::string_view s, int* out) {
	while (!s.empty() && s.front() == ' ') s.remove_prefix(1);
	while (!s.empty() && s.back() == ' ') s.remove_suffix(1);
	if (s.empty())
		return false;
	auto [p, ec] = std::from_chars(s.data(), s.data() + s.size(), *out);
	return ec == std::errc() && p == s.data() + s.size();
}
} // namespace

std::vector<int> ParseFrameList(const std::string& text) {
	std::vector<int> out;
	std::string_view rest = text;
	while (!rest.empty()) {
		size_t comma = rest.find(',');
		std::string_view item = rest.substr(0, comma);
		rest = comma == std::string_view::npos ? std::string_view{} : rest.substr(comma + 1);
		int a = 0, b = 0, step = 1;
		size_t dash = item.find('-', 1);
		if (dash != std::string_view::npos) {
			std::string_view hi = item.substr(dash + 1);
			size_t slash = hi.find('/');
			if (slash != std::string_view::npos) {
				if (!ToInt(hi.substr(slash + 1), &step) || step <= 0)
					continue;
				hi = hi.substr(0, slash);
			}
			if (!ToInt(item.substr(0, dash), &a) || !ToInt(hi, &b) || b < a)
				continue;
			for (int f = a; f <= b && out.size() < 100000; f += step)
				out.push_back(f);
		} else if (ToInt(item, &a)) {
			out.push_back(a);
		}
	}
	std::sort(out.begin(), out.end());
	out.erase(std::unique(out.begin(), out.end()), out.end());
	return out;
}

std::string ShotFileName(const std::string& base, int frame, bool manyFrames) {
	std::string stem = base;
	bool hasPng = stem.size() >= 4 && stem.compare(stem.size() - 4, 4, ".png") == 0;
	if (hasPng)
		stem.resize(stem.size() - 4);
	if (!manyFrames)
		return stem + ".png";
	return stem + "-" + std::to_string(frame) + ".png";
}

std::vector<InputEvent> ParseInputScript(const std::string& text, std::string* err) {
	std::vector<InputEvent> out;
	std::istringstream in(text);
	std::string line;
	int lineNo = 0;
	auto bad = [&](const std::string& why) {
		if (err) {
			if (!err->empty()) *err += "\n";
			*err += "input line " + std::to_string(lineNo) + ": " + why;
		}
	};
	while (std::getline(in, line)) {
		++lineNo;
		if (size_t hash = line.find('#'); hash != std::string::npos)
			line.resize(hash);
		std::istringstream ls(line);
		std::string frameText, cmd;
		if (!(ls >> frameText))
			continue;  // blank
		InputEvent e;
		if (!ToInt(frameText, &e.frame) || !(ls >> cmd)) {
			bad("expected '<frame> <command> ...'");
			continue;
		}
		if (cmd == "move" || cmd == "down" || cmd == "up" || cmd == "click" || cmd == "tap" || cmd == "fdown" || cmd == "fmove" ||
			cmd == "fup") {
			if (!(ls >> e.x >> e.y)) {
				bad("expected x y");
				continue;
			}
			e.kind = cmd == "move" || cmd == "fmove" ? InputEvent::Kind::Move
			       : cmd == "up" || cmd == "fup"     ? InputEvent::Kind::Up
			                                          : InputEvent::Kind::Down;
			e.touch = cmd == "tap" || cmd[0] == 'f'; // fdown / fmove / fup: a finger
			out.push_back(e);
			if (cmd == "click" || cmd == "tap") {
				InputEvent up = e;
				up.kind = InputEvent::Kind::Up;
				up.frame = e.frame + 5;
				out.push_back(up);
			}
		} else if (cmd == "text" || cmd == "key") {
			std::string rest;
			std::getline(ls, rest);
			size_t start = rest.find_first_not_of(' ');
			e.text = start == std::string::npos ? std::string{} : rest.substr(start);
			e.kind = cmd == "text" ? InputEvent::Kind::Text : InputEvent::Kind::Key;
			if (e.text.empty()) {
				bad("expected text");
				continue;
			}
			out.push_back(e);
		} else {
			bad("unknown command '" + cmd + "'");
		}
	}
	std::stable_sort(out.begin(), out.end(), [](const InputEvent& a, const InputEvent& b) { return a.frame < b.frame; });
	return out;
}

} // namespace pvzu
