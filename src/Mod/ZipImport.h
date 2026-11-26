// PVZU: "Import Unnamed ZIP" in the game's Options dialog (Android). The button opens the same file picker and import
// as the launcher's long-press "Manage Data" screen (ResourceImportActivity + UnnamedImport.java, nothing copied here);
// when the picker comes back the game says what happened and what to do next. Pure rules; the engine side (JNI, the
// dialog) is Mod/Engine/ZipImport.h.
#pragma once

#include <mutex>
#include <optional>
#include <string>

namespace pvzu {

// Android only (desktop builds have no picker), and only in Options opened from the main menu: in a level the
// dialog's three button slots (Almanac, Restart, Main Menu) are taken. Shown in Vanilla and Unnamed alike.
bool ImportButtonShown(bool theAndroid, bool theFromGameSelector);

// Where it sits in the Options dialog (dialog coordinates). The Restart slot (x 107, y 284, 209 x 46; Restart is
// hidden from the main menu): the Almanac slot above would cover the "Chinese Text" row, which sits 20 px lower
// there. The label in the stone font can be wider than 209 px ("Import Unnamed ZIP" is ~212), so the button grows
// around the same centre to leave kImportButtonTextPad each side, by whole middle tiles of the stone art (DrawStoneButton
// draws left cap + whole tiles + right cap), at most 4 tiles (255 px in the PC art, the width of the inner panel).
constexpr int kImportButtonTextPad = 10;
struct StoneArt { int left, middle, right; }; // IMAGE_BUTTON_LEFT / _MIDDLE / _RIGHT widths
struct ImportButtonBox { int x, y, w, h; };
ImportButtonBox ImportButtonRect(int theLabelWidth, const StoneArt& theArt);

enum class ImportOutcome { Cancelled, Imported, Failed };
struct ImportResult {
	ImportOutcome outcome = ImportOutcome::Cancelled;
	int copied = 0;      // files copied (Imported)
	std::string error;   // Java's message (Failed), may be empty
};
// The code PvZPortableActivity passes: 0 cancelled, 1 imported, 2 failed; anything else counts as failed.
ImportOutcome ImportOutcomeFromCode(int theCode);

// What the game shows once the picker is back. The art and the zip's text are read at start-up, so every import
// ends with "close and reopen the game"; with Unnamed Mode off it also says to tick it (the row is on the same dialog).
enum class ImportReply { None, Error, Restart, TurnOnAndRestart };
struct ImportMessage {
	ImportReply reply = ImportReply::None;
	const char* titleKey = "";
	const char* title = "";
	const char* bodyKey = "";
	const char* bodyTemplate = ""; // "{files}" -> "12 files" / "1 file", "{error}" -> the reason
	std::string body;               // the template filled in
};
// theUnnamedOn: Unnamed Mode as wanted now (running, or ticked this session).
ImportMessage ImportDoneMessage(const ImportResult& theResult, bool theUnnamedOn);
std::string FillImportBody(const std::string& theTemplate, const ImportResult& theResult);

// The result arrives on Android's UI thread and is read by the game thread.
class ImportMailbox {
public:
	void Post(const ImportResult& theResult);
	std::optional<ImportResult> Take(); // once; a newer Post replaces one not taken yet

private:
	std::mutex mMutex;
	std::optional<ImportResult> mResult;
};

// Mac dev flag -pvzu-dev-import imported|failed|cancelled: shows the button on desktop and a tap posts this result
// instead of opening a picker. Anything else: no flag.
std::optional<ImportResult> DevImportResult(const std::string& theValue);

} // namespace pvzu
