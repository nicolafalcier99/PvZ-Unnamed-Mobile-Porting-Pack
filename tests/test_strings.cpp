#include "doctest/doctest.h"
#include "Mod/TextDecode.h"

using namespace pvzu;

TEST_CASE("Gb18030Detected") {
	// "向日葵" (Sunflower) in GBK, and "[KEY]" ASCII around it
	const std::string gbk = "[SUNFLOWER]\n\xCF\xF2\xC8\xD5\xBF\xFB\n";
	CHECK_FALSE(LooksLikeUtf8(gbk));
	CHECK(DecodeTextFile(gbk) == "[SUNFLOWER]\n\xE5\x90\x91\xE6\x97\xA5\xE8\x91\xB5\n");
}

TEST_CASE("Utf8PassesThroughAndBomIsStripped") {
	const std::string utf8 = "\xEF\xBB\xBF[A]\nPea \xE5\x90\x91\n";
	CHECK(LooksLikeUtf8(utf8));
	CHECK(DecodeTextFile(utf8) == "[A]\nPea \xE5\x90\x91\n");
	CHECK(DecodeTextFile("plain ascii") == "plain ascii");
}

TEST_CASE("GbkFileNamesDecode") {
	// "[quote not included]" in GBK
	CHECK(GbkToUtf8("\xD0\xE8\xD2\xAA\xB0\xEF\xD6\xFA.png") == "\xE9\x9C\x80\xE8\xA6\x81\xE5\xB8\xAE\xE5\x8A\xA9.png");
	CHECK(GbkToUtf8("[1.3beta2]/images/a.png") == "[1.3beta2]/images/a.png");
}

TEST_CASE("BrokenGbkDoesNotCrash") {
	CHECK(GbkToUtf8(std::string("\x81", 1)) == "?");      // lead byte at end
	CHECK(GbkToUtf8(std::string("\xFF\xFF", 2)) == "??");  // invalid pair
}

TEST_CASE("TruncatedUtf8IsNotUtf8") {
	CHECK_FALSE(LooksLikeUtf8(std::string("ab\xE5\x90", 4)));
	CHECK_FALSE(LooksLikeUtf8(std::string("\xC3", 1)));
	CHECK(LooksLikeUtf8(std::string("\xC3\xA9", 2)));
}
