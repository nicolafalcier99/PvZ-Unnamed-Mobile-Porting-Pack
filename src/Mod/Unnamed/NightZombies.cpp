// PVZU: Unnamed Night zombies and elite forms. Section numbers refer to the research notes (w2-night); exe addresses are
// listed per key in pvz-unnamed-pack/SOURCES-night-zombies.md.
#include "Mod/Unnamed/NightZombies.h"

#include "Mod/Unnamed/Rules.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace pvzu::unnamed {

namespace {

int R(const char* key, int vanilla) { return RuleInt(key, vanilla); }
float RF(const char* key, float vanilla) { return RuleFloat(key, vanilla); }
bool On(const char* key) { return R(key, 0) != 0; }

bool IsGreen(const NightForm& f) { return f.subVariant == nightsub::GREEN; }
bool IsGold(const NightForm& f) { return f.variant == nightelite::GOLD_FOOTBALL; }

} // namespace

bool IsNightType(ZombieType t) {
	return t == ZOMBIE_NEWSPAPER || t == ZOMBIE_DOOR || t == ZOMBIE_FOOTBALL || t == ZOMBIE_DANCER;
}

const char* NightEliteName(ZombieType type, int variant, int subVariant) {
	const NightForm f{variant, subVariant};
	switch (type) {
	case ZOMBIE_NEWSPAPER:
		if (variant == nightelite::KNIFE) return "knife";
		if (variant == nightelite::ALMANAC_PAPER) return "almanac-paper";
		if (subVariant == nightsub::FOOTBALL_HEAD) return "football-head";
		break;
	case ZOMBIE_DOOR:
		if (variant == nightelite::TALLNUT_DOOR) return "tallnut-door";
		break;
	case ZOMBIE_FOOTBALL:
		if (IsGold(f) && IsGreen(f)) return "green+gold";
		if (IsGold(f)) return "gold";
		if (IsGreen(f)) return "green";
		break;
	case ZOMBIE_DANCER:
		if (variant == nightelite::TROPHY) return "trophy";
		if (variant == nightelite::MOONWALKER) return "moonwalker";
		if (variant == nightelite::RANDOM_SUMMONER) return "random-summoner";
		break;
	default: break;
	}
	return "none";
}

// ---- spawn rolls ----

int NightRollRange(ZombieType type, bool isIZombieLevel, int eliteOption) {
	if (eliteOption == eliteoption::NONE)
		return 0;
	const bool all = eliteOption == eliteoption::ALL;
	switch (type) {
	case ZOMBIE_NEWSPAPER: // hook 502
		return isIZombieLevel ? 0 : R(all ? "nightZombies.newspaper.eliteRollRangeAllElite" : "nightZombies.newspaper.eliteRollRange", 0);
	case ZOMBIE_DOOR: return isIZombieLevel ? 0 : R("nightZombies.door.tallnutRollRange", 0);          // hook 486
	case ZOMBIE_FOOTBALL: return FootballGreenRange();                                               // hook 489
	case ZOMBIE_DANCER: return R(all ? "nightZombies.dancer.eliteRollRangeAllElite" : "nightZombies.dancer.eliteRollRange", 0); // hook 504
	default: return 0;
	}
}

NightForm RollNewspaper(int roll, bool almanacAllowed, int eliteOption) { // 0xC05770
	if (roll <= R("nightZombies.newspaper.footballHeadMaxRoll", -1)) return {0, nightsub::FOOTBALL_HEAD};
	if (roll == R("nightZombies.newspaper.almanacRoll", -1)) // 0x7a1a16: option 2 skips the level gate
		return almanacAllowed || eliteOption == eliteoption::ALL ? NightForm{nightelite::ALMANAC_PAPER, 0} : NightForm{};
	if (roll == R("nightZombies.newspaper.knifeRoll", -1)) return {nightelite::KNIFE, 0};
	return {};
}

bool NewspaperAlmanacAllowed(int playerLevel, int finishedAdventures, bool isAdventure, int boardLevel) { // 0xC06230, 0x7AA100
	if (isAdventure && boardLevel == 70)
		return false;
	return playerLevel > R("nightZombies.newspaper.almanacAfterPlayerLevel", 1 << 30) || finishedAdventures > 0;
}

NightForm RollDoor(int roll, int eliteOption) { // 0x796163; option 2: `jne` -> `jo` at 0x796166 (0x7b2000)
	return roll == 0 || eliteOption == eliteoption::ALL ? NightForm{nightelite::TALLNUT_DOOR, 0} : NightForm{};
}

int FootballGreenRange() { return R("nightZombies.football.greenRollRange", 0); }
int FootballGoldRange() { return R("nightZombies.football.goldRollRange", 0); }
int FootballExtraRange() { return R("nightZombies.football.extraRollRange", 0); }

NightForm RollFootball(int greenRoll, int goldRoll) { // 0xC123E2 (Rand(4) == 0), 0x9DD37A (Rand(13) == 0)
	NightForm f;
	if (greenRoll == 0) f.subVariant = nightsub::GREEN;
	if (goldRoll == 0) f.variant = nightelite::GOLD_FOOTBALL;
	return f;
}

bool FootballDrawsExtraRoll(const NightForm& f) { return !IsGold(f); } // 0x9DD406 Rand(3), skipped for gold

NightForm RollDancer(int roll, bool isIZombieLevel, int eliteOption) { // 0x79682F .. 0x796885
	if (roll <= R("nightZombies.dancer.randomFollowerMaxRoll", -1)) return {nightelite::RANDOM_SUMMONER, 0};
	if (isIZombieLevel && eliteOption != eliteoption::ALL) return {}; // 0xc043eb: option 2 -> 0x79686e
	if (roll <= R("nightZombies.dancer.trophyMaxRoll", -1)) return {nightelite::TROPHY, 0};
	if (roll <= R("nightZombies.dancer.moonwalkMaxRoll", -1)) return {nightelite::MOONWALKER, 0};
	return {};
}

FootballAllEliteForm RollFootballAllElite(const std::function<int(int)>& rand, bool playerLevelAbove60) {
	FootballAllEliteForm out;
	auto greenThenGold = [&] { // 0xc123f1 green art, 0xc124d4 -> 0x9dd37a with bl = 0 (never black)
		out.form.subVariant = nightsub::GREEN;
		if (FootballGoldRange() > 0 && rand(FootballGoldRange()) == 0)
			out.form.variant = nightelite::GOLD_FOOTBALL;   // 0x9dd39d; 0xc12535 `test bl,bl` -> done
		else if (FootballExtraRange() > 0)
			rand(FootballExtraRange());                      // 0x9dd406, bl = 0: drawn, no effect
	};
	if (!playerLevelAbove60) { // 0xc12b40: Rand(16), ebx = 0
		const int n = R("nightZombies.football.allEliteRoll", 0);
		if (n <= 0) return out;
		if (rand(n) < R("nightZombies.football.allEliteGreenBelow", 0))
			greenThenGold();
		else
			out.form.variant = nightelite::GOLD_FOOTBALL;   // 0xc12c50 -> 0x9dd39d, bl = 0
		return out;
	}
	const int n = R("nightZombies.football.allEliteRollAbove60", 0); // 0xc12b1e: Rand(40)
	if (n <= 0) return out;
	const int r = rand(n);
	if (r > R("nightZombies.football.allEliteGreenMaxAbove60", 0)) { // 0xc12b2b -> 0x9dd37a with bl = 1
		if (FootballGoldRange() > 0 && rand(FootballGoldRange()) != 0) {
			out.black = true;                                // 0x9dd38f -> 0x9dd419 (0xc11df0)
		} else {
			out.form.variant = nightelite::GOLD_FOOTBALL;   // 0x9dd39d, then 0xc1253d -> 0x9dd406
			if (FootballExtraRange() > 0)
				out.black = rand(FootballExtraRange()) < R("nightZombies.football.allEliteBlackExtraBelow", 0);
		}
	} else if (r == 0) {
		out.form.variant = nightelite::GOLD_FOOTBALL;       // 0xc12b35 -> 0xc12c50, ebx = 0
	} else {
		greenThenGold();                                     // 0xc12b35 -> 0xc123f1
	}
	return out;
}

int NightBodyHp(ZombieType type, const NightForm& f, int vanilla) {
	switch (type) {
	case ZOMBIE_NEWSPAPER: // hook 517, 0x901D50
		return f.variant == nightelite::KNIFE ? R("nightZombies.newspaper.knifeBodyHp", vanilla)
		                                      : R("nightZombies.newspaper.bodyHp", vanilla);
	case ZOMBIE_FOOTBALL:
		if (IsGold(f)) return IsGreen(f) ? R("nightZombies.football.greenGoldBodyHp", vanilla) : R("nightZombies.football.goldBodyHp", vanilla);
		if (IsGreen(f)) return R("nightZombies.football.greenBodyHp", vanilla);
		return vanilla;
	case ZOMBIE_DANCER: { // 0x52352A, then the trophy's -500 (0x79689E)
		int hp = R("nightZombies.dancer.bodyHp", vanilla);
		if (f.variant == nightelite::TROPHY) hp -= R("nightZombies.dancer.trophyHpLoss", 0);
		return hp;
	}
	default: return vanilla;
	}
}

int NightHelmHp(ZombieType type, const NightForm& f, int vanilla) {
	if (type != ZOMBIE_FOOTBALL) return vanilla;
	if (IsGold(f)) return IsGreen(f) ? R("nightZombies.football.greenGoldHelmHp", vanilla) : R("nightZombies.football.goldHelmHp", vanilla);
	if (IsGreen(f)) return R("nightZombies.football.greenHelmHp", vanilla);
	return R("nightZombies.football.helmHp", vanilla); // 0x522BAA
}

int NightShieldHp(ZombieType type, const NightForm& f, int vanilla) {
	if (type == ZOMBIE_NEWSPAPER && f.variant == nightelite::ALMANAC_PAPER) // 0x7A1A41: at least 300
		return std::max(vanilla, R("nightZombies.newspaper.almanacPaperHp", vanilla));
	return vanilla;
}

int DancerStartPhaseCounter(const NightForm& f, int vanilla) { // 0xC052A0
	return f.variant == nightelite::RANDOM_SUMMONER ? R("nightZombies.dancer.randomFollowerDanceInCs", vanilla) : vanilla;
}

int DancerStartStored(const NightForm& f) { return f.variant == nightelite::RANDOM_SUMMONER ? -1 : 0; } // 0xC04616

// ---- movement ----

float NightWalkRateMultiplier(ZombieType type, const NightForm& f, int body, int bodyMax, int helm, int helmMax) {
	if (type == ZOMBIE_NEWSPAPER && f.subVariant == nightsub::FOOTBALL_HEAD) // 0xC058C0 -> 0xC034BA
		return RF("nightZombies.newspaper.footballHeadWalkMul", 1.0f);
	if (type != ZOMBIE_FOOTBALL) return 1.0f;
	float m = 1.0f;
	if (IsGreen(f)) { // 0xC12560
		const float k = RF("nightZombies.football.greenWalkHelmK", 1.0f);
		m *= (helmMax == 0 || helm == 0) ? k : k - static_cast<float>(helm) / static_cast<float>(helmMax);
		if (bodyMax != 0) {
			const float b = RF("nightZombies.football.greenWalkBodyK", 0.0f);
			m *= 1.0f + b * std::fabs(std::sin((1.0f - static_cast<float>(body) / static_cast<float>(bodyMax)) * b));
		}
		m *= RF("nightZombies.football.greenWalkBase", 1.0f);
	}
	if (IsGold(f)) m *= RF("nightZombies.football.goldWalkMul", 1.0f); // 0xC125E5 -> 0xC034B4
	return m;
}

float NightAnimRateMultiplier(ZombieType type, const NightForm& f, ZombiePhase phase) { // 0x901DB5
	if (type == ZOMBIE_NEWSPAPER && f.variant == nightelite::KNIFE && phase == PHASE_NEWSPAPER_MAD) {
		const float d = RF("nightZombies.knife.madAnimDiv", 1.0f);
		return d > 0.0f ? 1.0f / d : 1.0f;
	}
	return 1.0f;
}

bool DancerSkipsDancerUpdate(const NightForm& f) { return f.variant == nightelite::MOONWALKER; } // hook 625
float MoonwalkVelX() { return RF("nightZombies.dancer.moonwalkVelX", 0.5f); }
float MoonwalkAnimRate() { return RF("nightZombies.dancer.moonwalkAnimRate", 24.0f); }

// ---- dancers ----

bool DancerSnapsWithoutCountdown() { return On("nightZombies.dancer.snapWithoutCountdown"); }
bool DancerSkipsSummonCounterCheck(const NightForm& f) { return f.variant == nightelite::TROPHY; } // hook 594
bool DancerResummonsAll() { return On("nightZombies.dancer.resummonAll"); }
bool DancerCountsCarnival(const NightForm& f) { return f.variant != nightelite::RANDOM_SUMMONER; }
bool SummonsRandomFollowers(const NightForm& f) { return f.variant == nightelite::RANDOM_SUMMONER; }
int RandomFollowerRollRange() { return R("nightZombies.dancer.randomFollowerRollRange", 0); }

bool RandomFollowerAccepted(int r, int playerLevel) { // 0xC03A6F
	if (r == ZOMBIE_JALAPENO_HEAD)
		return playerLevel > R("nightZombies.dancer.randomFollowerJalapenoAfterPlayerLevel", 1 << 30);
	if (r == ZOMBIE_FLAG || r == ZOMBIE_LADDER || r == ZOMBIE_POGO || r == ZOMBIE_GATLING_HEAD) return false;
	if (r < ZOMBIE_DUCKY_TUBE) return true;
	if (r == ZOMBIE_BOSS) return false;
	if (r > ZOMBIE_BUNGEE) return true;
	return r == ZOMBIE_JACK_IN_THE_BOX;
}

bool DrawsWhite(ZombieType type, const NightForm& f) { // hook 649
	return type == ZOMBIE_DANCER && f.variant == nightelite::RANDOM_SUMMONER && On("nightZombies.dancer.randomSummonerWhite");
}

// ---- eating ----

std::optional<bool> NightBitesThisTick(ZombieType type, const NightForm& f, ZombiePhase phase, bool chilled, int rem) {
	if (phase == PHASE_NEWSPAPER_MAD) { // hook 672, 0x79C31A -> 0x79C340
		const int lo = R("nightZombies.newspaper.madBiteRemainderMin", 0), hi = R("nightZombies.newspaper.madBiteRemainderMax", 0);
		if (hi <= 0) return std::nullopt;
		return rem >= lo && rem <= hi;
	}
	if (type == ZOMBIE_FOOTBALL && f.subVariant != 0) { // 0x79C326 -> 0xC02170
		const int hi = R("nightZombies.football.greenBiteRemainderMax", -1);
		if (hi < 0) return std::nullopt;
		return chilled || rem <= hi;
	}
	return std::nullopt;
}

int CheckIfPreyCaughtCallsPerTick(ZombiePhase phase) { // hook 632
	return phase == PHASE_NEWSPAPER_MAD ? std::max(1, R("nightZombies.newspaper.madChecksPerTick", 1)) : 1;
}

bool SkipsNormalBite(ZombieType type, const NightForm& f) { // 0xC02730
	return type == ZOMBIE_NEWSPAPER && f.variant == nightelite::KNIFE && On("nightZombies.knife.skipsNormalBite");
}

int KnifeChewFlash(int flash, ZombieType type, const NightForm& f, ZombiePhase phase, bool mindControlled, SeedType seed) {
	const int delay = R("nightZombies.knife.killDelayCs", 0); // 0x901DEB
	if (delay <= 0) return flash;
	if (flash > kKnifePendingKill + 1) return kKnifePendingKill + 1;
	if (mindControlled || phase != PHASE_NEWSPAPER_MAD || type != ZOMBIE_NEWSPAPER || f.variant != nightelite::KNIFE ||
		seed == SEED_POTATOMINE)
		return flash;
	return kKnifePendingKill + delay;
}

bool KnifeKillsNow(int flashAfterDecrement) { return flashAfterDecrement == kKnifePendingKill; } // 0xC0270D

bool SkipsGarlic(ZombieType type, const NightForm& f) { // hook 637
	return type == ZOMBIE_NEWSPAPER && f.subVariant == nightsub::FOOTBALL_HEAD;
}

int MadButterCs(ZombieType type, ZombiePhase phase, int vanilla) { // hook 720
	return type == ZOMBIE_NEWSPAPER && phase == PHASE_NEWSPAPER_MAD ? R("nightZombies.newspaper.madButterCs", vanilla) : vanilla;
}

int EatZombieDamage(ZombieType eater, ZombiePhase eaterPhase, ZombieType target, int vanilla) { // hook 689
	if (eater != ZOMBIE_NEWSPAPER) return vanilla;
	if (eaterPhase != PHASE_NEWSPAPER_READING && target == ZOMBIE_BOSS) return R("nightZombies.newspaper.eatBossDamage", vanilla);
	return R("nightZombies.newspaper.eatZombieDamage", vanilla);
}

unsigned int EatZombieFlags(unsigned int vanilla) {
	return static_cast<unsigned int>(R("nightZombies.eatZombie.damageFlags", static_cast<int>(vanilla)));
}

bool GreenDropsHelmOnChew(const NightForm& f, bool hasHelm, int zombieAge, int playerLevel) { // 0xC12CAA, 0xC13180
	return IsGreen(f) && hasHelm && zombieAge >= R("nightZombies.football.greenHelmDropAgeCs", 1 << 30) &&
		playerLevel <= R("nightZombies.football.greenHelmDropMaxPlayerLevel", -1);
}

int GreenChewBodyBonus(int playerLevel) { // 0xC12E03 (level <= 60; above: Gatling-head form, world 7)
	return playerLevel <= R("nightZombies.football.greenChewBonusMaxPlayerLevel", -1) ? R("nightZombies.football.greenChewBodyBonus", 0) : 0;
}

// ---- damage ----

bool ReadingPaperSoaksBodyDamage(ZombieType type, const NightForm& f, ZombiePhase phase) { // hook 702, 0xC05970
	return type == ZOMBIE_NEWSPAPER && f.subVariant == nightsub::FOOTBALL_HEAD && phase == PHASE_NEWSPAPER_READING &&
		On("nightZombies.newspaper.footballHeadPaperSoaksBody");
}

bool ShieldAlwaysFirst(ZombieType type, const NightForm& f) { // hook 709, 0x796104 / 0x7961C0
	return type != ZOMBIE_BOSS && f.variant == nightelite::TALLNUT_DOOR && On("nightZombies.door.tallnutShieldFirst");
}

bool SkipsShieldDamageImages(ZombieType type, const NightForm& f) { // hook 696
	return type != ZOMBIE_BOSS && (f.variant == nightelite::TALLNUT_DOOR || f.variant == 0x18);
}

int AlmanacPaperBodyPerHit(ZombieType type, const NightForm& f) { // hook 695
	return type == ZOMBIE_NEWSPAPER && f.variant == nightelite::ALMANAC_PAPER ? R("nightZombies.newspaper.almanacBodyPerPaperHit", 0) : 0;
}

bool DoorReflects(ShieldType shield, unsigned int flags, ProjectileType projectile, ProjectileMotion motion) { // 0x7960A6
	if (!On("nightZombies.door.reflect") || shield != SHIELDTYPE_DOOR || (flags & 0xBu) != 0)
		return false;
	return projectile != PROJECTILE_FIREBALL && projectile != PROJECTILE_STAR && motion != MOTION_HOMING;
}

int DoorReflectShieldBonus(int damage) { return damage - R("nightZombies.door.reflectCostHp", 0); } // 0xC10DF0

Reflection ReflectedShot(bool doorWalksBackwards) { // 0x955E90
	return doorWalksBackwards ? Reflection{MOTION_STRAIGHT, 1} : Reflection{MOTION_BACKWARDS, 0};
}

bool GoldSquishes(ZombieType type, const NightForm& f) { // 0xC034F9
	return type == ZOMBIE_FOOTBALL && IsGold(f) && On("nightZombies.football.goldSquishes");
}

bool GoldRaisesGraveFootballs(ZombieType type, const NightForm& f) { return type == ZOMBIE_FOOTBALL && IsGold(f); } // 0xC03520

// ---- art ----

NightArt NightArtFor(ZombieType type, const NightForm& f) { // study 7
	NightArt a;
	if (type == ZOMBIE_NEWSPAPER) {
		if (f.subVariant == nightsub::FOOTBALL_HEAD) a.images.push_back({"anim_head1", "particles/ZombieFootballHead"});
		if (f.variant == nightelite::KNIFE) a.showTracks.push_back("Zombie_knife");
		if (f.variant == nightelite::ALMANAC_PAPER) a.images.push_back({"Zombie_paper_paper", "images/SelectorScreen_AlmanacHighlight"});
	} else if (type == ZOMBIE_DOOR) {
		if (f.variant == nightelite::TALLNUT_DOOR) a.images.push_back({"anim_screendoor", "reanim/Tallnut_cracked2"});
	} else if (type == ZOMBIE_FOOTBALL) {
		if (IsGreen(f)) // 0xC123F1: 8 tracks (names at 0x90173E..)
			for (const char* part : {"helmet", "leftarm_hand", "leftleg_lower", "rightleg_lower", "rightarm_hand", "upperbody",
			                         "upperbody2", "upperbody3"})
				a.images.push_back({std::string("zombie_football_") + part, std::string("images/green_football_") + part});
		if (IsGold(f)) a.images.push_back({"anim_head1", "reanim/coin_gold_dollar"});
	} else if (type == ZOMBIE_DANCER) {
		if (f.variant == nightelite::TROPHY) {
			a.images.push_back({"anim_head1", "images/trophy"});
			a.hidePrefixes = {"anim_hair", "anim_head2"};
		} else if (f.variant == nightelite::MOONWALKER) {
			a.images.push_back({"anim_head1", "reanim/Zombie_paper_madhead"});
			a.hidePrefixes = {"anim_hair"};
		}
	}
	return a;
}

NightArt ResolveNightArt(ZombieType type, const NightForm& f, const std::function<bool(const std::string&)>& available) {
	NightArt a = NightArtFor(type, f);
	for (const auto& img : a.images)
		if (!available(img.file)) {
			NightArt kept;
			kept.showTracks = a.showTracks;   // a merged track carries its own image
			return kept;
		}
	return a;
}

const std::vector<std::string>& UnnamedOnlyNewspaperTracks() {
	static const std::vector<std::string> tracks = {"Zombie_knife", "anim_football"};
	return tracks;
}

std::string NewspaperMadHeadFile(const NightForm& f) { // hook 552, 0xC05880
	return f.subVariant == nightsub::FOOTBALL_HEAD ? "particles/ZombieFootballHead" : "";
}

std::string GreenHelmImage(int damageIndex) {
	if (damageIndex == 1) return "images/green_football_helmet2";
	if (damageIndex == 2) return "images/green_football_helmet3";
	return "";
}

const char* KnifeAnim(const char* vanillaAnim) { // 0x901D6F, 0x901D92
	if (std::strcmp(vanillaAnim, "anim_gasp") == 0) return "anim_gasp1";
	if (std::strcmp(vanillaAnim, "anim_eat_nopaper") == 0) return "anim_eat_nopaper1";
	return vanillaAnim;
}

const std::vector<std::string>& NightZombieRuleKeys() {
	static const std::vector<std::string> keys = {
		"nightZombies.newspaper.bodyHp", "nightZombies.newspaper.knifeBodyHp", "nightZombies.newspaper.eliteRollRange",
		"nightZombies.newspaper.footballHeadMaxRoll", "nightZombies.newspaper.almanacRoll", "nightZombies.newspaper.knifeRoll",
		"nightZombies.newspaper.almanacAfterPlayerLevel", "nightZombies.newspaper.almanacPaperHp",
		"nightZombies.newspaper.almanacBodyPerPaperHit", "nightZombies.newspaper.footballHeadWalkMul",
		"nightZombies.newspaper.footballHeadPaperSoaksBody", "nightZombies.newspaper.madChecksPerTick",
		"nightZombies.newspaper.madBiteRemainderMin", "nightZombies.newspaper.madBiteRemainderMax",
		"nightZombies.newspaper.madButterCs", "nightZombies.newspaper.eatZombieDamage", "nightZombies.newspaper.eatBossDamage",
		"nightZombies.knife.madAnimDiv", "nightZombies.knife.killDelayCs", "nightZombies.knife.skipsNormalBite",
		"nightZombies.eatZombie.damageFlags",
		"nightZombies.door.tallnutRollRange", "nightZombies.door.reflect", "nightZombies.door.reflectCostHp",
		"nightZombies.door.tallnutShieldFirst",
		"nightZombies.football.helmHp", "nightZombies.football.greenRollRange", "nightZombies.football.greenBodyHp",
		"nightZombies.football.greenHelmHp", "nightZombies.football.goldRollRange", "nightZombies.football.goldBodyHp",
		"nightZombies.football.goldHelmHp", "nightZombies.football.greenGoldBodyHp", "nightZombies.football.greenGoldHelmHp",
		"nightZombies.football.extraRollRange", "nightZombies.football.goldWalkMul", "nightZombies.football.goldSquishes",
		"nightZombies.football.greenBiteRemainderMax", "nightZombies.football.greenWalkBase",
		"nightZombies.football.greenWalkHelmK", "nightZombies.football.greenWalkBodyK",
		"nightZombies.football.greenHelmDropAgeCs", "nightZombies.football.greenHelmDropMaxPlayerLevel",
		"nightZombies.football.greenChewBodyBonus", "nightZombies.football.greenChewBonusMaxPlayerLevel",
		"nightZombies.dancer.bodyHp", "nightZombies.dancer.eliteRollRange", "nightZombies.dancer.randomFollowerMaxRoll",
		"nightZombies.dancer.trophyMaxRoll", "nightZombies.dancer.moonwalkMaxRoll", "nightZombies.dancer.trophyHpLoss",
		"nightZombies.dancer.randomFollowerDanceInCs", "nightZombies.dancer.randomFollowerRollRange",
		"nightZombies.dancer.randomFollowerJalapenoAfterPlayerLevel", "nightZombies.dancer.snapWithoutCountdown",
		"nightZombies.dancer.resummonAll", "nightZombies.dancer.moonwalkVelX", "nightZombies.dancer.moonwalkAnimRate",
		"nightZombies.dancer.randomSummonerWhite",
		"nightZombies.newspaper.eliteRollRangeAllElite", "nightZombies.dancer.eliteRollRangeAllElite",
		"nightZombies.football.allEliteRoll", "nightZombies.football.allEliteGreenBelow",
		"nightZombies.football.allEliteRollAbove60", "nightZombies.football.allEliteGreenMaxAbove60",
		"nightZombies.football.allEliteBlackExtraBelow",
	};
	return keys;
}

} // namespace pvzu::unnamed
