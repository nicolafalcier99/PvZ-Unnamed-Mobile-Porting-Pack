#include "Mod/Unnamed/ModAlmanac.h"

#include <algorithm>
#include <format>

namespace pvzu::unnamed {

namespace {
std::string_view TrimEnd(std::string_view s) {
	while (!s.empty() && (s.back() == '\r' || s.back() == '\n' || s.back() == ' ' || s.back() == '\t'))
		s.remove_suffix(1);
	return s;
}
} // namespace

bool ParseModAlmanacText(std::string_view theText, ModAlmanacTexts* out, std::string* err) {
	out->clear();
	std::string aKey, aValue;
	bool aHaveKey = false;
	auto flush = [&] {
		if (aHaveKey)
			(*out)[aKey] = std::string(TrimEnd(aValue));
		aValue.clear();
	};
	size_t aPos = 0;
	int aLineNo = 0;
	while (aPos <= theText.size()) {
		const size_t aEnd = std::min(theText.find('\n', aPos), theText.size());
		std::string_view aLine = theText.substr(aPos, aEnd - aPos);
		aPos = aEnd + 1;
		++aLineNo;
		if (!aLine.empty() && aLine.back() == '\r')
			aLine.remove_suffix(1);
		if (!aLine.empty() && aLine.front() == '[') {
			const std::string_view aTrimmed = TrimEnd(aLine);
			if (aTrimmed.size() < 3 || aTrimmed.back() != ']') {
				if (err)
					*err = std::format("line {}: a key line must look like [KEY]", aLineNo);
				return false;
			}
			flush();
			aKey = std::string(aTrimmed.substr(1, aTrimmed.size() - 2));
			aHaveKey = true;
			continue;
		}
		if (!aLine.empty() && aLine.front() == '#')
			continue; // a comment line, anywhere
		if (!aHaveKey) {
			if (aLine.empty())
				continue;
			if (err)
				*err = std::format("line {}: text before the first [KEY]", aLineNo);
			return false;
		}
		if (!aValue.empty() || !aLine.empty()) // skip blank lines right after the key
			aValue += std::string(aLine) + "\n";
		if (aEnd == theText.size())
			break;
	}
	flush();
	return true;
}

std::string ModPlantTextKey(int theSeed, std::string_view theWhat) { return std::format("MOD_PLANT_0x{:02x}_{}", theSeed, theWhat); }

std::string ModHintKey(std::string_view theKind, int theSeed) { return std::format("MOD_HINT_{}_0x{:02x}", theKind, theSeed); }

std::string ModAlmanacRechargeSeconds(int theRefreshCs) {
	const int aTenths = theRefreshCs / 10; // cs -> tenths of a second
	return aTenths % 10 == 0 ? std::format("{}", aTenths / 10) : std::format("{}.{}", aTenths / 10, aTenths % 10);
}

// --- layout -------------------------------------------------------------------------------------------------------
// Measured on IMAGE_ALMANAC_PLANTBACK (2600 x 1200 capture of the 1300 x 600 screen, dialog at x 250): the light
// inner frame of the packet grid spans dialog x 10..458, y 78..562; vanilla buttons from AlmanacDialog's constructor.
namespace {
constexpr int kCellW = 72, kCellH = 94, kGridX = 18, kGridY = 85;
constexpr float kPacketScale = 1.2f;
constexpr int kPacketW = 50, kPacketH = 70; // SEED_PACKET_WIDTH / HEIGHT
} // namespace

AlmanacRect ModAlmanacGridArea() { return {10, 78, 448, 484}; }

AlmanacRect ModAlmanacCell(int theIndex) {
	return {kGridX + theIndex % kModAlmanacColumns * kCellW, kGridY + theIndex / kModAlmanacColumns * kCellH, kCellW, kCellH};
}

AlmanacRect ModAlmanacPacket(int theIndex) {
	const AlmanacRect c = ModAlmanacCell(theIndex);
	const int w = static_cast<int>(kPacketW * kPacketScale + 0.5f), h = static_cast<int>(kPacketH * kPacketScale + 0.5f);
	return {c.x + (c.w - w) / 2, c.y + (c.h - h) / 2, w, h};
}

float ModAlmanacPacketScale() { return kPacketScale; }

int ModAlmanacCellAt(int x, int y, int theCount) {
	for (int i = 0; i < theCount && i < kModAlmanacColumns * kModAlmanacRows; ++i)
		if (ModAlmanacCell(i).Contains(x, y))
			return i;
	return -1;
}

AlmanacRect ModAlmanacButton() { return {318, 567, 164, 26}; }
AlmanacRect ModAlmanacButtonHit() { return {290, 556, 220, 44}; }
AlmanacRect ModAlmanacIndexButton() { return {32, 567, 164, 26}; }
AlmanacRect ModAlmanacCloseButton() { return {676, 567, 89, 26}; }
AlmanacRect ModAlmanacCard() { return {459, 86, 316, 470}; }

// --- hints --------------------------------------------------------------------------------------------------------
PlantedHint ChoosePlantedHint(bool theHasShadowText, int theShadowPoints, int theShadowThreshold, bool theHasPlantedText) {
	if (theHasShadowText && theShadowThreshold > 0 && theShadowPoints < theShadowThreshold)
		return PlantedHint::Shadow;
	return theHasPlantedText ? PlantedHint::Planted : PlantedHint::None;
}

bool CannonTapNeedsHint(int theModSeed, int theCobState) {
	return theModSeed == 0x7f && theCobState == 0x23; // STATE_COBCANNON_ARMING: the Loaded Cannon waits for a load
}

bool PrismTapNeedsHint(int theModSeed, bool theClickable, bool theCanPay) {
	return theModSeed == 0x5d && theClickable && !theCanPay;
}

bool RefusalHasOwnHint(int theCursorSeed, bool theNeedsSleepingReason) {
	return theNeedsSleepingReason && (theCursorSeed == 0x5b || theCursorSeed == 0x73);
}

bool HintMayShow(bool theMessageShown, bool theShownIsHint) { return !theMessageShown || theShownIsHint; }

bool HintLatch::Take(std::uintptr_t theLevelId, int theSeed) {
	if (theLevelId != mLevel) {
		mLevel = theLevelId;
		mShown.reset();
	}
	if (theSeed < 0 || theSeed >= static_cast<int>(mShown.size()) || mShown.test(theSeed))
		return false;
	mShown.set(theSeed);
	return true;
}

} // namespace pvzu::unnamed
