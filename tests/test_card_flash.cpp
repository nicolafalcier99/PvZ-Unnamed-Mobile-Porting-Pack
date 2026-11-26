// 2026-11-23: when a card finishes recharging, the whole card flashes (not a packet-sized strip inside it).
// Timing follows vanilla particles/SeedPacketFlash.xml: alpha 1 at the start, 0 at 40% of a 25 cs life.
#include "doctest/doctest.h"
#include "Mod/PanelCard.h"

using namespace pvzu;

TEST_CASE("CardFlashFadesLikeTheVanillaPacketFlash") {
	CHECK(CardFlashAlpha(0) == doctest::Approx(1.0f));
	CHECK(CardFlashAlpha(5) == doctest::Approx(0.5f));
	CHECK(CardFlashAlpha(10) == doctest::Approx(0.0f));
	CHECK(CardFlashAlpha(25) == doctest::Approx(0.0f));
	CHECK(CardFlashAlpha(-1) == doctest::Approx(0.0f)); // not started
	CHECK(kCardFlashCs == 10);
}
