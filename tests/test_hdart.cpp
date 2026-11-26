#include "doctest/doctest.h"
#include "Mod/HdArt.h"
#include "Mod/Mode.h"

#include <filesystem>
#include <fstream>
#include <set>

using namespace pvzu;
namespace fs = std::filesystem;

TEST_CASE("HdKeyNormalisesGameNames") {
	// The game asks for names without extension, in any case, with either slash ("IMAGES\\Zombie.PNG" style).
	CHECK(hd::Key("reanim/PeaShooter_Head") == "reanim/peashooter_head");
	CHECK(hd::Key("IMAGES\\background1.jpg") == "images/background1");
	CHECK(hd::Key("./images/Sod1Row.PNG") == "images/sod1row");
	CHECK(hd::Key("images/editbox.gif") == "images/editbox");
	CHECK(hd::Key("particles/Doom") == "particles/doom");
	CHECK(hd::Key("images/some.name.v2") == "images/some.name.v2"); // not an image extension: kept
	CHECK(hd::Key("") == "");
}

TEST_CASE("HdPackFilesMapToKeys") {
	// The pack mirrors the PC paths as .png (jpg sources became png): images/background1.png <- images/background1.jpg
	CHECK(hd::KeyForPackFile("images/background1.png") == "images/background1");
	CHECK(hd::KeyForPackFile("reanim/PeaShooter_Head.png") == "reanim/peashooter_head");
	CHECK(hd::KeyForPackFile("hd.json") == "");
	CHECK(hd::KeyForPackFile("images/notes.txt") == "");
	// Alpha companions are composed into their colour image by the prepare script; never a twin of their own.
	CHECK(hd::IsAlphaCompanionKey("images/_fog"));
	CHECK(hd::IsAlphaCompanionKey("images/fog_"));
	CHECK_FALSE(hd::IsAlphaCompanionKey("images/fog"));
	CHECK_FALSE(hd::IsAlphaCompanionKey("reanim/zombie_body"));
}

TEST_CASE("HdAlphaCompanionNamesMatchTheEngine") {
	// ImageLib::GetImage looks for "<dir>/_<name>" then "<dir>/<name>_".
	auto names = hd::AlphaCompanionKeys("images/fog");
	REQUIRE(names.size() == 2);
	CHECK(names[0] == "images/_fog");
	CHECK(names[1] == "images/fog_");
	CHECK(hd::AlphaCompanionKeys("pool")[0] == "_pool");
}

TEST_CASE("HdScaleIsAWholeFactorOfTheLogicalSize") {
	CHECK(hd::ScaleFor(70, 65, 140, 130) == 2);   // logical size kept, HD twice as many pixels
	CHECK(hd::ScaleFor(70, 65, 280, 260) == 4);
	CHECK(hd::ScaleFor(70, 65, 70, 65) == 0);     // same size: nothing to gain
	CHECK(hd::ScaleFor(70, 65, 141, 130) == 0);   // not a whole factor
	CHECK(hd::ScaleFor(70, 65, 140, 195) == 0);   // different factors per axis
	CHECK(hd::ScaleFor(0, 65, 0, 130) == 0);
	CHECK(hd::ScaleFor(70, 65, 210, 195) == 0);   // 3x: textures must stay power-of-two pieces (mipmaps on GLES2)
	CHECK(hd::ScaleFor(70, 65, 560, 520) == 0);   // 8x: beyond what we support
}

TEST_CASE("HdUvMathMapsHdTexelsOntoTheSameQuad") {
	// A piece is laid out in logical pixels; its UVs (left / pieceWidth) stay the same, so the HD texture lands on
	// exactly the same quad. Only the clamp inset shrinks: half an HD texel instead of half a logical one.
	CHECK(hd::HalfTexel(64, 1) == doctest::Approx(0.5 / 64));
	CHECK(hd::HalfTexel(64, 2) == doctest::Approx(0.5 / 128));
	CHECK(hd::HalfTexel(256, 4) == doctest::Approx(0.5 / 1024));
	// The edge of logical pixel x is HD texel x*s: no half-pixel shift.
	CHECK(hd::HdTexelEdge(0, 2) == 0);
	CHECK(hd::HdTexelEdge(37, 2) == 74);
	hd::IntRect r = hd::ScaleRect({3, 5, 70, 65}, 2);
	CHECK(r.x == 6);
	CHECK(r.y == 10);
	CHECK(r.w == 140);
	CHECK(r.h == 130);
}

TEST_CASE("HdPieceSizeRespectsTheMaxTextureSize") {
	// A 2048-wide logical piece at 2x needs 4096 texels; with an 4096 limit the logical piece cap is 2048.
	CHECK(hd::MaxLogicalPiece(4096, 2) == 2048);
	CHECK(hd::MaxLogicalPiece(16384, 2) == 8192);
	CHECK(hd::MaxLogicalPiece(16384, 4) == 4096);
	CHECK(hd::MaxLogicalPiece(16384, 1) == 16384);
}

TEST_CASE("HdOverlayWins") {
	// Unnamed's overlay replaces the PC image, so the PC-based twin would be wrong: no HD for any name it serves,
	// including only its alpha companion.
	std::set<std::string> overlay = {"images/zombienote.png", "reanim/_tree8.png", "particles/doom.png"};
	auto exists = [&](const std::string& n) { return overlay.count(n) > 0; };
	CHECK(hd::OverlayServes("images/zombienote", exists));
	CHECK(hd::OverlayServes("reanim/tree8", exists));        // alpha companion served
	CHECK(hd::OverlayServes("particles/doom", exists));
	CHECK_FALSE(hd::OverlayServes("reanim/peashooter_head", exists));
}

TEST_CASE("HdTwinComesFromThePackThatMatchesWhatIsShown") {
	// PVZU U1 (plan 9): art the Unnamed overlay serves takes its twin from the hd-unnamed pack; a key listed in that
	// pack's samePc (the Unnamed file is pixel-identical to PC) reuses the PC pack's twin; anything else the overlay
	// serves gets no twin (a PC-based twin would show the wrong art). Art the overlay does not serve: PC pack.
	using hd::TwinFrom;
	CHECK(hd::PickTwin(/*overlay*/ false, /*inUnnamed*/ false, /*samePc*/ false, /*inPc*/ true) == TwinFrom::Pc);
	CHECK(hd::PickTwin(false, true, false, true) == TwinFrom::Pc);  // overlay not serving it (English skip list)
	CHECK(hd::PickTwin(false, false, false, false) == TwinFrom::None);
	CHECK(hd::PickTwin(true, true, false, true) == TwinFrom::Unnamed);
	CHECK(hd::PickTwin(true, true, false, false) == TwinFrom::Unnamed); // mod-only art (no PC image at all)
	CHECK(hd::PickTwin(true, false, true, true) == TwinFrom::Pc);       // identical to PC
	CHECK(hd::PickTwin(true, false, true, false) == TwinFrom::None);    // identical, but the PC pack lacks it
	CHECK(hd::PickTwin(true, false, false, true) == TwinFrom::None);    // different art, not upscaled yet
	CHECK(hd::PickTwin(true, true, true, true) == TwinFrom::Unnamed);   // a file in the pack beats the list
}

TEST_CASE("HdTwinValidOnlyWhileBitsAreUnchanged") {
	// A twin matches the bits it was attached to; any later change (tint, CopyImageBits, procedural drawing)
	// makes the image use its own pixels again.
	CHECK(hd::TwinValid(/*attachedAt*/ 3, /*bitsChangedCount*/ 3));
	CHECK_FALSE(hd::TwinValid(3, 4));
	CHECK_FALSE(hd::TwinValid(-1, 0)); // never attached
}

TEST_CASE("HdPackInfoParses") {
	auto info = hd::ParsePackInfo(R"({"scale":2,"count":2604,"source":"realesrgan-x4plus-anime"})");
	REQUIRE(info.has_value());
	CHECK(info->scale == 2);
	CHECK(info->count == 2604);
	CHECK(info->source == "realesrgan-x4plus-anime");
	CHECK_FALSE(hd::ParsePackInfo("{ nope").has_value());
	CHECK_FALSE(hd::ParsePackInfo(R"({"count":3})").has_value());  // no scale
	CHECK_FALSE(hd::ParsePackInfo(R"({"scale":3})").has_value());  // unsupported scale
}

TEST_CASE("HdUnnamedPackInfoListsKeysIdenticalToPc") {
	auto info = hd::ParsePackInfo(R"({"scale":2,"count":120,"samePc":["reanim/peashooter_head","IMAGES\\Sod1Row.png"]})");
	REQUIRE(info.has_value());
	CHECK(info->samePc.size() == 2);
	CHECK(info->samePc.count("reanim/peashooter_head") == 1);
	CHECK(info->samePc.count("images/sod1row") == 1); // normalised like any game name
	auto plain = hd::ParsePackInfo(R"({"scale":2,"samePc":"nope"})");
	REQUIRE(plain.has_value()); // a broken list is ignored, the pack still works
	CHECK(plain->samePc.empty());
}

TEST_CASE("HdUnnamedKeysThatDifferOnlyByATranslatedPieceUseThePcTwinInEnglish") {
	// e.g. images/zombienoteblack1: its resources.xml alpha image ZombieNote1 is in overlay-en-skip.txt, so in
	// English the composed image is the PC one; in Chinese it is the Unnamed note (the pack's file).
	auto info = hd::ParsePackInfo(R"({"scale":2,"samePc":["a/x"],"samePcEn":["images/zombienoteblack1"]})");
	REQUIRE(info.has_value());
	CHECK(info->samePcEn.count("images/zombienoteblack1") == 1);
	auto en = hd::LookUpUnnamed(*info, "images/zombienoteblack1", /*fileInPack*/ true, /*chinese*/ false);
	CHECK_FALSE(en.inPack);
	CHECK(en.samePc);
	auto zh = hd::LookUpUnnamed(*info, "images/zombienoteblack1", true, /*chinese*/ true);
	CHECK(zh.inPack);
	CHECK_FALSE(zh.samePc);
	auto same = hd::LookUpUnnamed(*info, "a/x", false, true);
	CHECK_FALSE(same.inPack);
	CHECK(same.samePc);
	auto plain = hd::LookUpUnnamed(*info, "reanim/hurrikale_head", true, false);
	CHECK(plain.inPack);
	CHECK_FALSE(plain.samePc);
}

TEST_CASE("HdRenderedTwinScaleFollowsThePcPackElseTheUnnamedOne") {
	CHECK(hd::RenderScale(/*pcScale*/ 2, /*unnamedScale*/ 4) == 2);
	CHECK(hd::RenderScale(0, 2) == 2);
	CHECK(hd::RenderScale(0, 0) == 0);
}

TEST_CASE("HdSettingDefaultsOnWhenThePackExists") {
	CHECK(hd::HdWanted(std::nullopt, /*packPresent*/ true));
	CHECK_FALSE(hd::HdWanted(std::nullopt, false));
	CHECK_FALSE(hd::HdWanted(false, true));
	CHECK(hd::HdWanted(true, true));
	CHECK_FALSE(hd::HdWanted(true, false)); // nothing to show
}

TEST_CASE("HdSettingPersistsInPvzuSettings") {
	fs::path dir = fs::temp_directory_path() / "pvzu-hd-settings-test";
	fs::remove_all(dir);
	fs::create_directories(dir);
	CHECK_FALSE(Settings{}.hd.has_value());
	{ std::ofstream(dir / "a.json") << R"({"mode":"vanilla","hd":false})"; }
	CHECK(LoadSettings(dir / "a.json").hd == std::optional<bool>(false));
	{ std::ofstream(dir / "b.json") << R"({"hd":"yes"})"; }
	CHECK_FALSE(LoadSettings(dir / "b.json").hd.has_value());
	Settings s;
	s.hd = true;
	REQUIRE(SaveSettings(dir / "c.json", s));
	CHECK(LoadSettings(dir / "c.json").hd == std::optional<bool>(true));
	Settings unset;
	REQUIRE(SaveSettings(dir / "d.json", unset));
	CHECK_FALSE(LoadSettings(dir / "d.json").hd.has_value()); // not written when never chosen
	fs::remove_all(dir);
}

TEST_CASE("HdBudgetMath") {
	// Texture bytes of one piece at scale s, with the full mip chain (4x4 -> 16 + 4 + 1 texels).
	CHECK(hd::PieceBytes(2, 2, 2, 4, false) == 64);
	CHECK(hd::PieceBytes(2, 2, 2, 4, true) == (16 + 4 + 1) * 4);
	CHECK(hd::PieceBytes(64, 64, 1, 2, false) == 64 * 64 * 2);
	CHECK(hd::PieceBytes(1024, 512, 2, 2, true) == hd::MipChainTexels(2048, 1024) * 2);
	CHECK(hd::MipChainTexels(1, 1) == 1);
	CHECK(hd::MipChainTexels(4, 1) == 4 + 2 + 1);
	CHECK(hd::Megabytes(700LL * 1024 * 1024) == doctest::Approx(700.0));
	// The runtime budget: a twin is used only if it fits; 0 = no budget.
	CHECK(hd::BudgetAllows(600LL << 20, 50LL << 20, 700LL << 20));
	CHECK_FALSE(hd::BudgetAllows(680LL << 20, 50LL << 20, 700LL << 20));
	CHECK(hd::BudgetAllows(680LL << 20, 50LL << 20, 0));
}

TEST_CASE("HdPngHeaderGivesTheTwinSize") {
	// The engine checks a twin's size from its PNG header at load and decodes the pixels only when first used.
	const unsigned char png[24] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A, 0, 0, 0, 13, 'I', 'H', 'D', 'R',
	                               0, 0, 0x0A, 0xF0, 0, 0, 0x04, 0xB0};
	auto s = hd::PngSize(png, sizeof(png));
	REQUIRE(s.has_value());
	CHECK(s->first == 2800);
	CHECK(s->second == 1200);
	CHECK_FALSE(hd::PngSize(png, 20).has_value());
	const unsigned char jpg[24] = {0xFF, 0xD8};
	CHECK_FALSE(hd::PngSize(jpg, sizeof(jpg)).has_value());
}

TEST_CASE("HdWindowSizeOption") {
	// -pvzu-window-px 2340x1080: the drawable size of a hidden test window (phone-like captures on the Mac).
	auto s = hd::ParseSize("2340x1080");
	REQUIRE(s.has_value());
	CHECK(s->first == 2340);
	CHECK(s->second == 1080);
	CHECK_FALSE(hd::ParseSize("2340").has_value());
	CHECK_FALSE(hd::ParseSize("0x10").has_value());
	CHECK_FALSE(hd::ParseSize("ax10").has_value());
}
