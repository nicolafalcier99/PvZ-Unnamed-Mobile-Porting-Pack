#include "Mod/TextDecode.h"

#include <cstdint>

namespace pvzu {

namespace {

const uint16_t kGbkTable[] = {
#include "Mod/GbkTable.inc"
};

void AppendUtf8(std::string& out, uint32_t cp) {
	if (cp < 0x80) {
		out += static_cast<char>(cp);
	} else if (cp < 0x800) {
		out += static_cast<char>(0xC0 | (cp >> 6));
		out += static_cast<char>(0x80 | (cp & 0x3F));
	} else {
		out += static_cast<char>(0xE0 | (cp >> 12));
		out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
		out += static_cast<char>(0x80 | (cp & 0x3F));
	}
}

} // namespace

bool LooksLikeUtf8(std::string_view s) {
	size_t i = 0;
	while (i < s.size()) {
		const auto c = static_cast<unsigned char>(s[i]);
		int extra = 0;
		if (c < 0x80) extra = 0;
		else if ((c & 0xE0) == 0xC0 && c >= 0xC2) extra = 1;
		else if ((c & 0xF0) == 0xE0) extra = 2;
		else if ((c & 0xF8) == 0xF0 && c <= 0xF4) extra = 3;
		else return false;
		if (i + extra >= s.size() && extra > 0)
			return false; // sequence cut off at the end
		for (int k = 1; k <= extra; ++k)
			if ((static_cast<unsigned char>(s[i + k]) & 0xC0) != 0x80)
				return false;
		i += 1 + extra;
	}
	return true;
}

std::string GbkToUtf8(std::string_view s) {
	std::string out;
	out.reserve(s.size() * 3 / 2);
	size_t i = 0;
	while (i < s.size()) {
		const auto c = static_cast<unsigned char>(s[i]);
		if (c < 0x80) {
			out += static_cast<char>(c);
			++i;
			continue;
		}
		if (c >= 0x81 && c <= 0xFE && i + 1 < s.size()) {
			const auto t = static_cast<unsigned char>(s[i + 1]);
			if (t >= 0x40 && t <= 0xFE && t != 0x7F) {
				const uint16_t cp = kGbkTable[(c - 0x81) * 191 + (t - 0x40)];
				if (cp != 0) {
					AppendUtf8(out, cp);
					i += 2;
					continue;
				}
			}
			out += '?';
			i += 2;
			continue;
		}
		out += '?';
		++i;
	}
	return out;
}

std::string DecodeTextFile(std::string_view s) {
	if (s.size() >= 3 && s.substr(0, 3) == "\xEF\xBB\xBF")
		s.remove_prefix(3);
	if (LooksLikeUtf8(s))
		return std::string(s);
	return GbkToUtf8(s);
}

} // namespace pvzu
