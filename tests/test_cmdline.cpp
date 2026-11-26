#include "doctest/doctest.h"
#include "Mod/CmdLine.h"

using namespace pvzu;

TEST_CASE("PvzuParamsAreRecognised") {
	CHECK(IsPvzuParam("-pvzu-width"));
	CHECK(IsPvzuParam("-pvzu-dump-tables"));
	CHECK_FALSE(IsPvzuParam("-resdir"));
	CHECK_FALSE(IsPvzuParam("pvzu-width"));
}

TEST_CASE("CmdOptionsStoreValuesAndFlags") {
	ClearCmdOptions();
	SetCmdOption("-pvzu-width", "1300");
	SetCmdOption("-pvzu-quit", "");
	CHECK(CmdOption("-pvzu-width").value() == "1300");
	CHECK(CmdFlag("-pvzu-quit"));
	CHECK(CmdFlag("-pvzu-width"));
	CHECK_FALSE(CmdOption("-pvzu-missing").has_value());
	CHECK_FALSE(CmdFlag("-pvzu-missing"));
	CHECK(CmdOptionInt("-pvzu-width", 800) == 1300);
	CHECK(CmdOptionInt("-pvzu-missing", 800) == 800);
	SetCmdOption("-pvzu-bad", "abc");
	CHECK(CmdOptionInt("-pvzu-bad", 7) == 7);
}

TEST_CASE("AutomatedRunsAreDetected") {
	ClearCmdOptions();
	CHECK_FALSE(AutomatedRun(nullptr));
	CHECK(AutomatedRun("1"));
	CHECK_FALSE(AutomatedRun("0"));
	for (const char* flag : {"-pvzu-shot", "-pvzu-input", "-pvzu-quit-frame", "-pvzu-level", "-pvzu-quit-after",
	                         "-pvzu-quit-after-waves", "-pvzu-dump-tables", "-pvzu-autoplay", "-pvzu-hidden"}) {
		ClearCmdOptions();
		SetCmdOption(flag, "1");
		CAPTURE(flag);
		CHECK(AutomatedRun(nullptr));
	}
	ClearCmdOptions();
	SetCmdOption("-pvzu-width", "1300");  // a manual wide window stays visible
	CHECK_FALSE(AutomatedRun(nullptr));
	ClearCmdOptions();
}
