// PVZU: "Import Unnamed ZIP" in the Options dialog (Mod/ZipImport.h): where the button shows, what the game says
// when the Android picker comes back, the hand-off between Android's UI thread and the game, the Mac dev flag.
#include "doctest/doctest.h"
#include "Mod/ZipImport.h"

#include <string>
#include <thread>

using namespace pvzu;

TEST_CASE("ImportButtonOnlyOnAndroidFromTheMainMenu") {
	// Android build, Options opened from the main menu: shown (in Vanilla and Unnamed alike)
	CHECK(ImportButtonShown(true, true));
	// in a level the dialog's three button slots (Almanac, Restart, Main Menu) are taken
	CHECK_FALSE(ImportButtonShown(true, false));
	// desktop builds never show it: there is no Android picker to open
	CHECK_FALSE(ImportButtonShown(false, true));
	CHECK_FALSE(ImportButtonShown(false, false));
}

TEST_CASE("ImportButtonGrowsToItsLabelInsideTheDialog") {
	// the dialog's stone buttons are 209 wide at x 107; "Import Unnamed ZIP" in the stone font is wider (~212 px),
	// so the button grows around the same centre, never past the dialog's inner panel (x 74..337)
	// the stone is drawn as left cap + whole middle tiles + right cap (36 + n x 46 + 35 in the PC art: 209 = 3 tiles),
	// so the width must be a whole number of tiles or the label runs past the drawn stone
	const StoneArt art{36, 46, 35};
	CHECK(ImportButtonRect(150, art).w == 209);
	CHECK(ImportButtonRect(150, art).x == 107);
	const ImportButtonBox wide = ImportButtonRect(212, art); // "Import Unnamed ZIP"
	CHECK(wide.w == 255);                        // 4 tiles
	CHECK(wide.w >= 212 + 2 * kImportButtonTextPad);
	CHECK(wide.x + wide.w / 2 == 107 + 209 / 2); // same centre as Almanac / Restart / Main Menu
	const ImportButtonBox huge = ImportButtonRect(400, art);
	CHECK(huge.w == 255);                        // never past the dialog's inner panel
	CHECK(ImportButtonRect(212, {0, 0, 0}).w == 209); // art not loaded: the stock size
	// from the main menu the checkbox rows sit 15-20 px lower, so the Almanac slot (241) would cover "Chinese Text":
	// the button takes the Restart slot (hidden there)
	CHECK(ImportButtonRect(150, art).y == 284);
}

TEST_CASE("CancelledPickerSaysNothing") {
	const ImportMessage m = ImportDoneMessage({ImportOutcome::Cancelled, 0, ""}, false);
	CHECK(m.reply == ImportReply::None);
	CHECK(m.body.empty());
}

TEST_CASE("FailedImportShowsTheReason") {
	const ImportMessage m = ImportDoneMessage({ImportOutcome::Failed, 0, "not an Unnamed zip"}, true);
	CHECK(m.reply == ImportReply::Error);
	CHECK(std::string(m.title) == "Import Failed");
	CHECK(m.body.find("not an Unnamed zip") != std::string::npos);
	// no reason given: still a plain sentence
	const ImportMessage n = ImportDoneMessage({ImportOutcome::Failed, 0, ""}, true);
	CHECK(n.reply == ImportReply::Error);
	CHECK_FALSE(n.body.empty());
	CHECK(n.body.find(":\n") == std::string::npos);
}

TEST_CASE("ImportedWithUnnamedOffSaysTurnItOnAndRestart") {
	const ImportMessage m = ImportDoneMessage({ImportOutcome::Imported, 4321, ""}, false);
	CHECK(m.reply == ImportReply::TurnOnAndRestart);
	CHECK(std::string(m.title) == "Unnamed Imported");
	CHECK(m.body.find("4321 files") != std::string::npos);
	CHECK(m.body.find("Turn on Unnamed Mode") != std::string::npos);
	CHECK(m.body.find("reopen the game") != std::string::npos);
}

TEST_CASE("ImportedWithUnnamedOnSaysRestartOnly") {
	// Unnamed on (running, or ticked this session): the art is read at start-up, so a restart is still needed
	const ImportMessage m = ImportDoneMessage({ImportOutcome::Imported, 12, ""}, true);
	CHECK(m.reply == ImportReply::Restart);
	CHECK(m.body.find("12 files") != std::string::npos);
	CHECK(m.body.find("Turn on") == std::string::npos);
	CHECK(m.body.find("reopen the game") != std::string::npos);
}

TEST_CASE("ImportedOneFileIsSingular") {
	CHECK(ImportDoneMessage({ImportOutcome::Imported, 1, ""}, true).body.find("1 file.") != std::string::npos);
}

TEST_CASE("ImportOutcomeFromTheJavaCode") {
	// PvZPortableActivity.nativePvzuImportDone(outcome, copied, error): 0 cancelled, 1 imported, 2 failed
	CHECK(ImportOutcomeFromCode(0) == ImportOutcome::Cancelled);
	CHECK(ImportOutcomeFromCode(1) == ImportOutcome::Imported);
	CHECK(ImportOutcomeFromCode(2) == ImportOutcome::Failed);
	CHECK(ImportOutcomeFromCode(-7) == ImportOutcome::Failed); // unknown code: never silently "imported"
}

TEST_CASE("MailboxHandsOverOneResultOnce") {
	ImportMailbox box;
	CHECK_FALSE(box.Take().has_value());
	box.Post({ImportOutcome::Imported, 5, ""});
	auto r = box.Take();
	REQUIRE(r.has_value());
	CHECK(r->outcome == ImportOutcome::Imported);
	CHECK(r->copied == 5);
	CHECK_FALSE(box.Take().has_value());
	// a newer result replaces one not taken yet
	box.Post({ImportOutcome::Failed, 0, "a"});
	box.Post({ImportOutcome::Imported, 9, ""});
	CHECK(box.Take()->copied == 9);
}

TEST_CASE("MailboxAcrossThreads") {
	ImportMailbox box;
	std::thread t([&] { box.Post({ImportOutcome::Imported, 77, ""}); }); // Android's UI thread
	t.join();
	CHECK(box.Take()->copied == 77); // the game thread
}

TEST_CASE("DevImportFlagFakesThePickerOnTheMac") {
	CHECK_FALSE(DevImportResult("").has_value());
	CHECK_FALSE(DevImportResult("bogus").has_value());
	CHECK(DevImportResult("cancelled")->outcome == ImportOutcome::Cancelled);
	auto ok = DevImportResult("imported");
	REQUIRE(ok.has_value());
	CHECK(ok->outcome == ImportOutcome::Imported);
	CHECK(ok->copied > 0);
	auto bad = DevImportResult("failed");
	REQUIRE(bad.has_value());
	CHECK(bad->outcome == ImportOutcome::Failed);
	CHECK_FALSE(bad->error.empty());
}
