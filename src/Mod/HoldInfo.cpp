// PVZU: see HoldInfo.h.
#include "Mod/HoldInfo.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <utility>

namespace pvzu {

// --- hold timing ---------------------------------------------------------------------------------------------------
void HoldPress::Down(int theTarget, int theFrame) {
	mTarget = theTarget;
	mDownFrame = theFrame;
	mShown = false;
	mLeft = false;
}

void HoldPress::Update(int theFrame, int theTargetUnder) {
	if (!Active() || mLeft || mShown) // once shown, the panel stays until the lift (2026-11-24)
		return;
	if (theTargetUnder != mTarget) {
		mLeft = true;
		return;
	}
	if (theFrame - mDownFrame >= kHoldInfoDelay)
		mShown = true;
}

HoldPress::Lift HoldPress::Up(int theFrame, int theTargetUnder) {
	if (!Active())
		return Lift::None;
	Update(theFrame, theTargetUnder);
	const bool aTap = !mLeft && !mShown;
	Reset();
	return aTap ? Lift::Tap : Lift::None;
}

void HoldPress::Reset() {
	mTarget = kHoldNoTarget;
	mShown = false;
	mLeft = false;
}

// --- panel layout --------------------------------------------------------------------------------------------------
namespace {
int PanelW(int theScreenW, int theSafeLeft, int theSafeRight) {
	return std::max(0, std::min(kInfoMaxW, theScreenW - theSafeLeft - theSafeRight - 2 * kInfoMargin));
}
} // namespace

int InfoBodyWidth(int theScreenW, int theSafeLeft, int theSafeRight) {
	return std::max(0, PanelW(theScreenW, theSafeLeft, theSafeRight) - kInfoPad - kInfoPadRight - kInfoPictureW - kInfoGap);
}

int InfoBodyMaxHeight(int theScreenH, bool theWarning) {
	return std::max(0, theScreenH - 2 * kInfoMargin - kInfoPad - kInfoTitleH - kInfoLedgeGap - kInfoLedgeH - (theWarning ? kInfoWarningH : 0));
}

InfoPanelLayout LayoutInfoPanel(int theScreenW, int theScreenH, int theSafeLeft, int theSafeRight, int theBodyH, bool theWarning) {
	InfoPanelLayout l;
	const int aW = PanelW(theScreenW, theSafeLeft, theSafeRight);
	const int aBodyH = std::clamp(theBodyH, 0, InfoBodyMaxHeight(theScreenH, theWarning));
	const int aWarningH = theWarning ? kInfoWarningH : 0;
	const int aColumnH = std::max(kInfoPictureH, aWarningH + aBodyH);
	const int aContentH = kInfoTitleH + aColumnH + kInfoLedgeGap;
	const int aH = kInfoPad + aContentH + kInfoLedgeH;
	const int aSafeW = theScreenW - theSafeLeft - theSafeRight;
	l.panel = {theSafeLeft + (aSafeW - aW) / 2, (theScreenH - aH) / 2, aW, aH};
	const int aInnerX = l.panel.x + kInfoPad;
	const int aInnerW = aW - kInfoPad - kInfoPadRight;
	const int aTop = l.panel.y + kInfoPad;
	l.content = {aInnerX, aTop, aInnerW, aContentH};
	l.title = {aInnerX, aTop, aInnerW, kInfoTitleH};
	l.picture = {aInnerX, aTop + kInfoTitleH, kInfoPictureW, kInfoPictureH};
	const int aTextX = aInnerX + kInfoPictureW + kInfoGap;
	const int aTextW = InfoBodyWidth(theScreenW, theSafeLeft, theSafeRight);
	int y = aTop + kInfoTitleH;
	if (theWarning) {
		l.warning = {aTextX, y, aTextW, kInfoWarningH};
		y += kInfoWarningH;
	}
	l.body = {aTextX, y, aTextW, aBodyH};
	const int aLedgeY = l.panel.y + aH - kInfoLedgeH;
	l.stats = {aInnerX, aLedgeY + (kInfoLedgeH - kInfoStatsH) / 2 - kInfoStatsLift, aInnerW, kInfoStatsH};
	return l;
}

// --- styled text ---------------------------------------------------------------------------------------------------
namespace {

struct Code {
	std::string_view name;
	InfoStyle style;
};
constexpr Code kStyleCodes[] = {
	{"NORMAL", InfoStyle::Normal}, {"FLAVOR", InfoStyle::Flavor}, {"KEYWORD", InfoStyle::Keyword},
	{"NOCTURNAL", InfoStyle::Nocturnal}, {"AQUATIC", InfoStyle::Aquatic}, {"STAT", InfoStyle::Stat},
	{"METAL", InfoStyle::Metal}, {"KEYMETAL", InfoStyle::KeyMetal},
};

// Length in bytes of the UTF-8 sequence starting with c (1 for stray bytes).
size_t Utf8Len(unsigned char c) {
	if (c >= 0xF0)
		return 4;
	if (c >= 0xE0)
		return 3;
	if (c >= 0xC0)
		return 2;
	return 1;
}

// Code point of the sequence at s[i] (enough for the CJK test).
uint32_t CodePoint(std::string_view s, size_t i, size_t n) {
	const auto b = [&](size_t k) { return static_cast<uint32_t>(static_cast<unsigned char>(s[i + k])); };
	if (n == 2)
		return ((b(0) & 0x1F) << 6) | (b(1) & 0x3F);
	if (n == 3)
		return ((b(0) & 0x0F) << 12) | ((b(1) & 0x3F) << 6) | (b(2) & 0x3F);
	if (n == 4)
		return ((b(0) & 0x07) << 18) | ((b(1) & 0x3F) << 12) | ((b(2) & 0x3F) << 6) | (b(3) & 0x3F);
	return b(0);
}

bool IsCjk(uint32_t cp) { return cp >= 0x2E80; } // CJK scripts and full-width forms: a line may break between any two

struct Token {
	enum class Kind { Word, Space, Newline, Gap } kind = Kind::Word;
	InfoStyle style = InfoStyle::Normal;
	std::string text;
};

std::vector<Token> Tokenize(std::string_view theText) {
	std::vector<Token> out;
	InfoStyle aStyle = InfoStyle::Normal;
	bool aJoinLines = false; // {FLAVOR}: a newline is a space (vanilla's IGNORE_NEWLINES flag, reset by any other code)
	bool aSkipNewline = false;
	auto word = [&](std::string_view w) {
		if (!out.empty() && out.back().kind == Token::Kind::Word && out.back().style == aStyle)
			out.back().text += w;
		else
			out.push_back({Token::Kind::Word, aStyle, std::string(w)});
	};
	size_t i = 0;
	while (i < theText.size()) {
		const char c = theText[i];
		if (c == '{') {
			const size_t aEnd = theText.find('}', i);
			if (aEnd != std::string_view::npos) {
				const std::string_view aName = theText.substr(i + 1, aEnd - i - 1);
				i = aEnd + 1;
				aJoinLines = false;
				if (aName == "SHORTLINE" || aName == "EXTRASHORTLINE") {
					out.push_back({Token::Kind::Gap, aStyle, {}});
					aSkipNewline = true; // the code sits on its own line
					continue;
				}
				for (const Code& k : kStyleCodes)
					if (aName == k.name)
						aStyle = k.style;
				aJoinLines = aName == "FLAVOR";
				continue;
			}
		}
		if (c == '\r') {
			++i;
			continue;
		}
		if (c == '\n') {
			++i;
			if (aSkipNewline) {
				aSkipNewline = false;
				continue;
			}
			if (aJoinLines)
				out.push_back({Token::Kind::Space, aStyle, " "});
			else
				out.push_back({Token::Kind::Newline, aStyle, {}});
			continue;
		}
		aSkipNewline = false;
		if (c == ' ' || c == '\t') {
			out.push_back({Token::Kind::Space, aStyle, " "});
			++i;
			continue;
		}
		const size_t n = std::min(Utf8Len(static_cast<unsigned char>(c)), theText.size() - i);
		const std::string_view aChar = theText.substr(i, n);
		if (IsCjk(CodePoint(theText, i, n))) {
			out.push_back({Token::Kind::Word, aStyle, std::string(aChar)}); // its own word: a break may follow
			out.push_back({Token::Kind::Space, aStyle, {}});                // a zero-width break point
		} else {
			if (!out.empty() && out.back().kind == Token::Kind::Space && out.back().text.empty())
				out.pop_back(); // Latin glued to the CJK character before it (a number after a word: "射手3")
			word(aChar);
		}
		i += n;
	}
	return out;
}

void AddRun(InfoLine& theLine, InfoStyle theStyle, std::string_view theText) {
	if (theText.empty())
		return;
	if (!theLine.runs.empty() && theLine.runs.back().style == theStyle)
		theLine.runs.back().text += theText;
	else
		theLine.runs.push_back({theStyle, std::string(theText)});
}

} // namespace

std::vector<InfoLine> WrapInfoText(std::string_view theText, int theMaxWidth, const InfoMeasure& theMeasure) {
	const std::vector<Token> aTokens = Tokenize(theText);
	std::vector<InfoLine> out;
	InfoLine aLine;
	int aLineW = 0;
	bool aLineOpen = false; // something (even a blank line) is being built
	const Token* aPendingSpace = nullptr;
	auto endLine = [&]() {
		out.push_back(std::move(aLine));
		aLine = InfoLine{};
		aLineW = 0;
		aLineOpen = false;
		aPendingSpace = nullptr;
	};
	for (const Token& t : aTokens) {
		switch (t.kind) {
		case Token::Kind::Newline:
			endLine(); // a blank line when nothing was on it
			break;
		case Token::Kind::Gap:
			if (aLineOpen && !aLine.runs.empty())
				endLine();
			out.push_back(InfoLine{{}, true});
			aLineOpen = false;
			aPendingSpace = nullptr;
			break;
		case Token::Kind::Space:
			if (!aLine.runs.empty())
				aPendingSpace = &t;
			break;
		case Token::Kind::Word: {
			std::string_view aWord = t.text;
			while (!aWord.empty()) {
				const int aSpaceW = aPendingSpace ? theMeasure(aPendingSpace->text) : 0;
				const int aWordW = theMeasure(aWord);
				if (aLine.runs.empty() || aLineW + aSpaceW + aWordW <= theMaxWidth) {
					if (aLine.runs.empty() && aWordW > theMaxWidth) {
						// too long for any line: cut it at the last code point that fits (at least one)
						size_t aCut = 0;
						while (aCut < aWord.size()) {
							const size_t n = std::min(Utf8Len(static_cast<unsigned char>(aWord[aCut])), aWord.size() - aCut);
							if (aCut > 0 && theMeasure(aWord.substr(0, aCut + n)) > theMaxWidth)
								break;
							aCut += n;
						}
						AddRun(aLine, t.style, aWord.substr(0, aCut));
						aWord.remove_prefix(aCut);
						endLine();
						continue;
					}
					if (aPendingSpace && !aLine.runs.empty())
						AddRun(aLine, aPendingSpace->style, aPendingSpace->text);
					AddRun(aLine, t.style, aWord);
					aLineW += aSpaceW + aWordW;
					aLineOpen = true;
					aPendingSpace = nullptr;
					aWord = {};
				} else {
					endLine();
				}
			}
			break;
		}
		}
	}
	if (!aLine.runs.empty())
		endLine();
	// no gaps or blank lines at the start or the end
	auto blank = [](const InfoLine& l) { return l.runs.empty(); };
	while (!out.empty() && blank(out.back()))
		out.pop_back();
	size_t aFirst = 0;
	while (aFirst < out.size() && blank(out[aFirst]))
		++aFirst;
	out.erase(out.begin(), out.begin() + static_cast<std::ptrdiff_t>(aFirst));
	return out;
}

int InfoTextHeight(const std::vector<InfoLine>& theLines, int theLineH) {
	int h = 0;
	for (const InfoLine& l : theLines)
		h += l.gap ? theLineH / 2 : theLineH;
	return h;
}

std::string DropFlavor(std::string_view theText) {
	const size_t at = theText.find("{FLAVOR}");
	return std::string(at == std::string_view::npos ? theText : theText.substr(0, at));
}

InfoFit FitInfoText(std::string_view theText, int theWidth, int theMaxHeight, int theLineH, const InfoMeasure& theMeasure) {
	const std::string aFull(theText);
	const std::string aShort = DropFlavor(theText);
	auto attempt = [&](const std::string& theBody, float theScale, bool theDropped) {
		InfoFit f;
		f.scale = theScale;
		f.flavorDropped = theDropped;
		f.lines = WrapInfoText(theBody, static_cast<int>(theWidth / theScale), theMeasure);
		f.height = static_cast<int>(InfoTextHeight(f.lines, theLineH) * theScale + 0.5f);
		return f;
	};
	const bool aHasStory = aShort.size() != aFull.size();
	if (InfoFit f = attempt(aFull, kInfoTextScale, false); f.height <= theMaxHeight)
		return f;
	if (aHasStory)
		if (InfoFit f = attempt(aShort, kInfoTextScale, true); f.height <= theMaxHeight)
			return f;
	InfoFit f = attempt(aShort, kInfoTextMinScale, aHasStory);
	if (f.height <= theMaxHeight)
		return f;
	while (!f.lines.empty() && f.height > theMaxHeight) {
		f.lines.pop_back();
		while (!f.lines.empty() && f.lines.back().runs.empty())
			f.lines.pop_back();
		f.height = static_cast<int>(InfoTextHeight(f.lines, theLineH) * f.scale + 0.5f);
	}
	f.clipped = true;
	return f;
}

} // namespace pvzu
