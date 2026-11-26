// PVZU: engine side of "Import Unnamed ZIP" in the Options dialog (rules Mod/ZipImport.h).
//
// Android: a tap calls PvZPortableActivity.pvzuOpenUnnamedImport() over JNI, which opens ResourceImportActivity with
// EXTRA_UNNAMED_ONLY: it goes straight to the Unnamed zip picker, imports with the Manage Data code
// (UnnamedImport.importZip), reports back through PvZPortableActivity.nativePvzuImportDone and returns to the game.
// The game thread is paused by SDL while that screen is in front; the Options dialog reads the result on its next
// update and shows what to do next.
//
// Dev flag (desktop): -pvzu-dev-import imported|failed|cancelled shows the button and a tap posts that result.
#pragma once

#include "Mod/ZipImport.h"

#include <optional>

namespace pvzu::engine {

// The Options dialog shows the button (Android, or the dev flag on desktop).
bool ImportButtonWanted(bool theFromGameSelector);
// Opens the picker (dev: posts the fake result). False when it could not be opened.
bool StartUnnamedZipImport();
// The picker's result, once.
std::optional<ImportResult> TakeUnnamedImportResult();

} // namespace pvzu::engine
