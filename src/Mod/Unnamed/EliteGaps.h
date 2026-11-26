// PVZU: Plan 9 R4, the Football gaps and the elite-option-0 branches the earlier plans left out, as pure rules (rolls
// passed in). Evidence: the research notes (r4-football) (sections 1-6) and
// pvz-unnamed-pack/SOURCES-elite-gaps.md (exe addresses). Every constant comes from data/rules-elite-gaps.json (keys
// "eliteGaps.*") with the vanilla value (or "rule off") as fallback; the Night and World-7 ranges they extend are read
// through NightZombies.h / World7Zombies.h.
#pragma once

#include "ConstEnums.h"
#include "Mod/Unnamed/NightZombies.h"

#include <functional>

namespace pvzu::unnamed {

namespace gaps {
constexpr int GATLING_FOOTBALL = 0x6b;   // Football Zombie+0x134 once the green one grew a Gatling Pea head (0xc12df8)
}

// ---- 1. black armour damage (hook 705: 0x818069 .. 0x8180c0, 0xc00710) ----
// x0.85 walking / x1.2 eating (float, FPU round to nearest even), then at least 1.
int BlackArmourDamage(int damage, bool eating);
// A hit below 5 draws Rand(5) (0 = no roll) and is lost when damage <= roll.
int BlackArmourSmallHitRollRange(int damage);
int BlackArmourSmallHit(int damage, int roll);
// CanBeFrozen (hook 712, 0x81803b): a black Football cannot be frozen.
bool BlackArmourRefusesFreeze(ZombieType type, int subVariant);

// ---- 3. the Football's option-0 roll, every branch (hook 489, 0x9dd310 .. 0x9dd489) ----
// rand(n) = Rand(n) in the exe's order. above60 = profile level > 60. graveRise = the spawn flag 1 (a grave rise from
// a gold Football's death: no gold roll). blackEligible = the exe's `bl` (never for a green one); extraRoll = the
// Rand(3) when drawn, else -1. The black decision itself is World7Zombies FootballBecomesBlack (feature world7Zombies).
struct FootballDefaultRolls {
	NightForm form;
	int extraRoll = -1;
	bool blackEligible = false;
	bool operator==(const FootballDefaultRolls&) const = default;
};
FootballDefaultRolls RollFootballDefault(const std::function<int(int)>& rand, bool above60, bool graveRise);

// ---- 4. green Football walking event and the Gatling head (hooks 640, 628, 575) ----
// Zombie::Animate, not-eating branch, every tick: a green Football (sub 0x1A) not yet 0x6b with its helmet on and
// age >= 800 cs: helmet off at profile <= 70, Gatling head at profile > 60, +1000 body always.
struct GreenWalkEvent {
	bool fires = false;
	bool dropHelm = false;
	bool gatlingHead = false;
	int bodyBonus = 0;
	bool operator==(const GreenWalkEvent&) const = default;
};
bool GreenWalkEventOn();   // the rule sits in Animate (the Night port's chew site is then skipped)
GreenWalkEvent GreenFootballWalkEvent(ZombieType type, int variant, int subVariant, bool hasHelm, int age, int playerLevel);
// both green paths call UpdateAnimSpeed every not-eating tick (0xc12b9d), except once it is 0x6b
bool GreenRefreshesAnimSpeed(ZombieType type, int variant, int subVariant);
bool IsGatlingFootball(ZombieType type, int variant);
// the head's offset on the Football's anim_head1 track (TodScaleRotateTransformMatrix 0xc12de0)
struct HeadOffset {
	float x = 0, y = 0, rot = 0, sx = 1, sy = 1;
	bool operator==(const HeadOffset&) const = default;
};
HeadOffset GatlingFootballHeadOffset();
int GatlingFootballStartCounter();   // mPhaseCounter 150
// UpdateZombieGatlingHead: a Football also fires at these counters (0xc12e80), besides the vanilla 18 / 35 / 51 / 68
bool GatlingFootballExtraShot(int phaseCounter);
// 0xc12f00 / 0xc12f16: a Football's peas never spread
bool GatlingFootballSpreads();
// hook 628, 0xc03f70: no Gatling update while rising (phase 0x32)
bool GatlingUpdateSkipped(ZombiePhase phase);

// ---- 5. Newspaper at profile level > 70 (0xc05788 .. 0xc0585e) ----
// rolled = the form RollNewspaper gave for roll (Rand(5), or Rand(4) with ALL). Above 70: always the football head,
// roll 2 -> Almanac paper (when allowed), roll 3 -> knife, both with the head.
NightForm NewspaperAbove70(const NightForm& rolled, int roll, int playerLevel, bool almanacAllowed, int eliteOption);
// Board+0x556c (mEffectCounter) > 1200 -> paper HP counter / 10, 3000 from 30000; else unchanged.
int NewspaperAbove70PaperHp(int playerLevel, int effectCounter, int vanilla);
// 0xc05770: the Newspaper also rolls in I, Zombie (the Night port skipped it).
bool NewspaperRollsInIZombie();

// ---- 6. football helmet on Normal / Newspaper / Dancer (0xc109bf) and the I, Zombie Normal (0xc111af) ----
bool TakesFootballHelmCheck(ZombieType type, bool iZombieLevel, int eliteOption);   // which types run 0xc109bf
int FootballHelmRollRange(bool iZombieLevel);   // 3 in I, Zombie, else 0 (no roll)
// I, Zombie: roll == 0. Else board level > 70 and the board's wave > 25 (no roll).
bool GetsFootballHelm(bool iZombieLevel, int roll, int boardLevel, int currentWave);
int FootballHelmHp(int vanilla);   // 2800
// I, Zombie Normal, option 0: Rand(7), Rand(30) with the football helmet; roll + 1 <= 3 -> 0x105 (silver coin)
int IZombieNormalCoinRollRange(bool footballHelm);
bool IZombieNormalIsCoin(int roll);

// Every rules key these rules read (for the pack consistency test).
const std::vector<std::string>& EliteGapsRuleKeys();

} // namespace pvzu::unnamed
