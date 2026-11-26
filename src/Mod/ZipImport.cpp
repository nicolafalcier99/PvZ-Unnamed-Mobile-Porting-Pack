#include "Mod/ZipImport.h"

#include <algorithm>

namespace pvzu {

bool ImportButtonShown(bool theAndroid, bool theFromGameSelector) { return theAndroid && theFromGameSelector; }

ImportButtonBox ImportButtonRect(int theLabelWidth, const StoneArt& theArt) {
	constexpr int kX = 107, kY = 284, kW = 209, kH = 46; // NewOptionsDialog::Resize's Restart slot
	constexpr int kCentre = kX + kW / 2;
	constexpr int kMaxTiles = 4;
	int aW = kW;
	if (theArt.middle > 0) {
		const int aCaps = theArt.left + theArt.right;
		int aTiles = std::max(0, (kW - aCaps) / theArt.middle);
		while (aTiles < kMaxTiles && aCaps + aTiles * theArt.middle < theLabelWidth + 2 * kImportButtonTextPad)
			aTiles++;
		aW = std::max(kW, aCaps + aTiles * theArt.middle);
	}
	return {kCentre - aW / 2, kY, aW, kH};
}

ImportOutcome ImportOutcomeFromCode(int theCode) {
	switch (theCode) {
	case 0: return ImportOutcome::Cancelled;
	case 1: return ImportOutcome::Imported;
	default: return ImportOutcome::Failed;
	}
}

namespace {
void ReplaceAll(std::string& s, const std::string& from, const std::string& to) {
	for (size_t at = s.find(from); at != std::string::npos; at = s.find(from, at + to.size()))
		s.replace(at, from.size(), to);
}
}

std::string FillImportBody(const std::string& theTemplate, const ImportResult& theResult) {
	std::string s = theTemplate;
	ReplaceAll(s, "{files}", std::to_string(theResult.copied) + (theResult.copied == 1 ? " file" : " files"));
	ReplaceAll(s, "{error}", theResult.error);
	return s;
}

ImportMessage ImportDoneMessage(const ImportResult& theResult, bool theUnnamedOn) {
	ImportMessage m;
	switch (theResult.outcome) {
	case ImportOutcome::Cancelled:
		return m;
	case ImportOutcome::Failed:
		m.reply = ImportReply::Error;
		m.titleKey = "PVZU_IMPORT_FAILED_TITLE";
		m.title = "Import Failed";
		if (theResult.error.empty()) {
			m.bodyKey = "PVZU_IMPORT_FAILED";
			m.bodyTemplate = "Could not import that zip.\nPick the Unnamed zip\nand try again.";
		} else {
			m.bodyKey = "PVZU_IMPORT_FAILED_WHY";
			m.bodyTemplate = "Could not import that zip:\n{error}";
		}
		break;
	case ImportOutcome::Imported:
		m.titleKey = "PVZU_IMPORT_DONE_TITLE";
		m.title = "Unnamed Imported";
		if (theUnnamedOn) {
			m.reply = ImportReply::Restart;
			m.bodyKey = "PVZU_IMPORT_DONE_RESTART";
			m.bodyTemplate = "Imported {files}.\nClose and reopen the game\nto use them.";
		} else {
			m.reply = ImportReply::TurnOnAndRestart;
			m.bodyKey = "PVZU_IMPORT_DONE_TURN_ON";
			m.bodyTemplate = "Imported {files}.\nTurn on Unnamed Mode, then\nclose and reopen the game.";
		}
		break;
	}
	m.body = FillImportBody(m.bodyTemplate, theResult);
	return m;
}

void ImportMailbox::Post(const ImportResult& theResult) {
	std::lock_guard<std::mutex> aLock(mMutex);
	mResult = theResult;
}

std::optional<ImportResult> ImportMailbox::Take() {
	std::lock_guard<std::mutex> aLock(mMutex);
	std::optional<ImportResult> r;
	r.swap(mResult);
	return r;
}

std::optional<ImportResult> DevImportResult(const std::string& theValue) {
	if (theValue == "imported")
		return ImportResult{ImportOutcome::Imported, 4321, ""};
	if (theValue == "failed")
		return ImportResult{ImportOutcome::Failed, 0, "not an Unnamed zip (needs one top folder like [1.3beta2])"};
	if (theValue == "cancelled")
		return ImportResult{ImportOutcome::Cancelled, 0, ""};
	return std::nullopt;
}

} // namespace pvzu
