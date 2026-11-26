#include "Mod/Unnamed/Music.h"
#include "Mod/Unnamed/ModModeRules.h" // mode numbers
#include "Mod/Unnamed/Rules.h"

namespace pvzu::unnamed {

namespace {
constexpr int kTableTunes = 12; // gdi42 .rdata 0x103dbdf0: 12 (tune, N) pairs; 0x100188ab: tune index > 12 -> vanilla
}

bool ValidTrackNumber(int theN) {
	return theN == 1 || theN == 4 || (theN >= 5 && theN <= 14); // 0x10018848: cmp 1 / cmp 4 / lea -5, cmp 9, ja
}

int MusicTrackForTune(int theTune) {
	if (theTune < 1 || theTune > kTableTunes)
		return 0;
	const nlohmann::json* aTracks = RuleArray("music.tracks");
	if (aTracks == nullptr || static_cast<int>(aTracks->size()) < theTune)
		return 0;
	const nlohmann::json& aN = (*aTracks)[theTune - 1];
	if (!aN.is_number_integer())
		return 0;
	const int aTrack = aN.get<int>();
	return ValidTrackNumber(aTrack) ? aTrack : 0;
}

std::string MusicTrackFileName(int theN) {
	return "lawnbgm(" + std::to_string(theN) + ").mp3";
}

bool CerebrawlAdventureLevel(int theLevel) {
	const int aMin = RuleInt("music.cerebrawlLevelMin", 0);
	const int aMax = RuleInt("music.cerebrawlLevelMax", -1);
	return aMin > 0 && theLevel >= aMin && theLevel <= aMax;
}

namespace {
// MusicTune values (Lawn/System/Music.h)
enum : int { kNone = -1, kGrasswalk = 1, kMoongrains = 2, kWateryGraves = 3, kRigorMormist = 4, kGrazeTheRoof = 5,
	kZen = 8, kCerebrawl = 9, kLoonboon = 10, kConveyer = 11, kBrainiacManiac = 12 };

// Original only: the Unnamed levels with a vanilla kind (port ruling, Music.h). The mod modes are their twins'
// modes (Level55Shooter / Level65Snake / Level70Boss run them by the same code: 0x72 Lone Wolf, 0x89 Glutton Star,
// 0x83 Second Uncle).
int OriginalTuneOfKind(const GameTuneInput& in) {
	const int aLevel = in.adventure ? in.level : 0;
	if (aLevel == 70 || in.gameMode == kSecondUncle)
		return kBrainiacManiac;
	if (aLevel == 55 || aLevel == 65 || in.gameMode == kLoneWolf || in.gameMode == kGluttonStar)
		return kLoonboon;
	return 0;
}
} // namespace

int GameTune(const GameTuneInput& in, int theMusicChoice) {
	// Music::StartGameMusic (Lawn/System/Music.cpp, exe 0x45b770), in its order
	if (in.zenOrTree)
		return kZen;
	if (in.finalBoss)
		return kBrainiacManiac;
	if (in.miniGame)
		return kLoonboon;
	if (theMusicChoice == musicchoice::ORIGINAL)
		if (const int aTune = OriginalTuneOfKind(in); aTune != 0)
			return aTune;
	if (in.adventure && CerebrawlAdventureLevel(in.level)) // hook 258 (0x95c1f0)
		return kCerebrawl;
	if ((in.adventure && (in.level == 10 || in.level == 20 || in.level == 30)) || in.column)
		return kConveyer;
	if (in.stormy)
		return kNone;
	if (in.puzzle)
		return kCerebrawl;
	if (in.night)
		return in.pool ? kRigorMormist : kMoongrains;
	if (in.sixRows)
		return kWateryGraves;
	if (in.roof)
		return kGrazeTheRoof;
	return kGrasswalk;
}

} // namespace pvzu::unnamed
