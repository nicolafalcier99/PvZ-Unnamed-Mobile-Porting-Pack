// PVZU: text decoding for Unnamed's files (GBK / GB18030 Chinese) without iconv (absent on Android < 9).
#pragma once

#include <string>
#include <string_view>

namespace pvzu {

bool LooksLikeUtf8(std::string_view bytes);  // valid UTF-8 (ASCII counts)
// GBK/GB18030 two-byte text -> UTF-8. Invalid or 4-byte sequences become '?' per bad byte pair.
std::string GbkToUtf8(std::string_view bytes);
// Strips a UTF-8 BOM; keeps valid UTF-8; otherwise treats the bytes as GBK.
std::string DecodeTextFile(std::string_view bytes);

} // namespace pvzu
