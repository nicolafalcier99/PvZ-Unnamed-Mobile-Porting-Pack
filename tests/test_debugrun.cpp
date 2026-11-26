#include "doctest/doctest.h"
#include "Mod/DebugRun.h"

using namespace pvzu;

TEST_CASE("FrameListParses") {
	auto f = ParseFrameList("300, 100,200");
	REQUIRE(f.size() == 3);
	CHECK(f[0] == 100);  // sorted
	CHECK(f[1] == 200);
	CHECK(f[2] == 300);
	CHECK(ParseFrameList("").empty());
	CHECK(ParseFrameList("abc,5").size() == 1);  // junk skipped
	auto r = ParseFrameList("100-130/10");       // range with step
	REQUIRE(r.size() == 4);
	CHECK(r[3] == 130);
}

TEST_CASE("ShotFileNameForFrame") {
	CHECK(ShotFileName("/tmp/a.png", 120, false) == "/tmp/a.png");
	CHECK(ShotFileName("/tmp/a.png", 120, true) == "/tmp/a-120.png");
	CHECK(ShotFileName("/tmp/a", 7, true) == "/tmp/a-7.png");
}

TEST_CASE("InputScriptParses") {
	std::string err;
	auto s = ParseInputScript(
		"# comment\n"
		"10 move 100 200\n"
		"20 click 400 300\n"
		"30 text Player\n"
		"40 key return\n"
		"bad line\n", &err);
	REQUIRE(s.size() == 5);  // click = down + up
	CHECK(s[0].frame == 10);
	CHECK(s[0].kind == InputEvent::Kind::Move);
	CHECK(s[0].x == 100);
	CHECK(s[0].y == 200);
	CHECK(s[1].kind == InputEvent::Kind::Down);
	CHECK(s[1].frame == 20);
	CHECK(s[2].kind == InputEvent::Kind::Up);
	CHECK(s[2].frame == 25);
	CHECK(s[3].kind == InputEvent::Kind::Text);
	CHECK(s[3].text == "Player");
	CHECK(s[4].kind == InputEvent::Kind::Key);
	CHECK(s[4].text == "return");
	CHECK(s[4].frame == 40);
	CHECK_FALSE(err.empty());  // the bad line is reported
}

TEST_CASE("InputScriptTapIsATouchClick") {
	// "tap": a finger press and lift (SDL's touch mouse, as on the phone), for touch-only behaviour on the Mac.
	std::string err;
	auto s = ParseInputScript("20 tap 400 300\n30 click 1 2\n", &err);
	REQUIRE(s.size() == 4);
	CHECK(s[0].kind == InputEvent::Kind::Down);
	CHECK(s[0].touch);
	CHECK(s[1].kind == InputEvent::Kind::Up);
	CHECK(s[1].frame == 25);
	CHECK(s[1].touch);
	CHECK(s[1].x == 400);
	CHECK_FALSE(s[2].touch); // click stays a mouse
	CHECK(err.empty());
}

TEST_CASE("InputScriptFingerDownMoveUp") {
	// "fdown" / "fmove" / "fup": a finger held and dragged (swipes, follow-the-finger modes) on the Mac.
	std::string err;
	auto s = ParseInputScript("10 fdown 100 200\n12 fmove 150 200\n14 fup 180 200\n16 move 1 1\n", &err);
	REQUIRE(s.size() == 4);
	CHECK(s[0].kind == InputEvent::Kind::Down);
	CHECK(s[1].kind == InputEvent::Kind::Move);
	CHECK(s[2].kind == InputEvent::Kind::Up);
	CHECK((s[0].touch && s[1].touch && s[2].touch));
	CHECK(s[2].x == 180);
	CHECK_FALSE(s[3].touch);
	CHECK(err.empty());
}

TEST_CASE("InputScriptIsSortedByFrame") {
	std::string err;
	auto s = ParseInputScript("50 move 1 1\n10 move 2 2\n", &err);
	REQUIRE(s.size() == 2);
	CHECK(s[0].frame == 10);
	CHECK(err.empty());
}
