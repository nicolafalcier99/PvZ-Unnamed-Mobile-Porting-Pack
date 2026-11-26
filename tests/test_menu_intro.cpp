// 2026-11-23: the zombie-hand scene after tapping Adventure (4.5 s) only the first time, or after a long break
// (a year without playing). Otherwise Adventure opens the level at once.
#include "doctest/doctest.h"
#include "Mod/MenuIntro.h"
#include "Mod/Mode.h"

#include <filesystem>

using namespace pvzu;
namespace fs = std::filesystem;

namespace {
constexpr int64_t kDay = 24 * 60 * 60;
constexpr int64_t kNow = 1'800'000'000;
}

TEST_CASE("AdventureIntroPlaysTheFirstTimeOnly") {
	CHECK(ShouldPlayAdventureIntro(std::nullopt, std::nullopt, kNow, false));              // never seen
	CHECK_FALSE(ShouldPlayAdventureIntro(kNow - kDay, kNow - kDay, kNow, false));           // seen, played yesterday
	CHECK_FALSE(ShouldPlayAdventureIntro(kNow - 400 * kDay, kNow - kDay, kNow, false));     // seen long ago, plays daily
	CHECK_FALSE(ShouldPlayAdventureIntro(kNow - kDay, std::nullopt, kNow, false));          // seen, no play record
}

TEST_CASE("AdventureIntroPlaysAgainAfterAYearAway") {
	CHECK(ShouldPlayAdventureIntro(kNow - 500 * kDay, kNow - 365 * kDay, kNow, false));
	CHECK_FALSE(ShouldPlayAdventureIntro(kNow - 500 * kDay, kNow - 364 * kDay, kNow, false));
	CHECK(kLongAbsenceSeconds == 365 * kDay);
}

TEST_CASE("AdventureIntroAtMostOncePerSession") {
	CHECK_FALSE(ShouldPlayAdventureIntro(std::nullopt, std::nullopt, kNow, true));
	CHECK_FALSE(ShouldPlayAdventureIntro(kNow - 500 * kDay, kNow - 400 * kDay, kNow, true));
}

TEST_CASE("AdventureIntroClockGoingBackwardsIsNotAnAbsence") {
	CHECK_FALSE(ShouldPlayAdventureIntro(kNow - kDay, kNow + 400 * kDay, kNow, false));
}

TEST_CASE("IntroAndLastPlayedTimesPersist") {
	fs::path dir = fs::temp_directory_path() / "pvzu-intro-test";
	fs::remove_all(dir);
	fs::create_directories(dir);
	Settings s;
	s.speed = 2;
	CHECK_FALSE(s.introSeenAt.has_value());
	CHECK_FALSE(s.lastPlayedAt.has_value());
	s.introSeenAt = kNow - 3;
	s.lastPlayedAt = kNow;
	REQUIRE(SaveSettings(dir / "pvzu-settings.json", s));
	Settings back = LoadSettings(dir / "pvzu-settings.json");
	CHECK(back.introSeenAt == kNow - 3);
	CHECK(back.lastPlayedAt == kNow);
	CHECK(back.speed == 2);
	fs::remove_all(dir);
}
