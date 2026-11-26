// PVZU: Plan 9 R4, Football gaps and elite-option-0 branches. Section numbers refer to
// the research notes (r4-football); exe addresses per key are in pvz-unnamed-pack/SOURCES-elite-gaps.md.
#include "Mod/Unnamed/EliteGaps.h"

#include "Mod/Unnamed/Rules.h"
#include "Mod/Unnamed/World7Zombies.h"

#include <cfenv>
#include <cmath>

namespace pvzu::unnamed {

namespace {

int R(const char* key, int vanilla) { return RuleInt(key, vanilla); }
float RF(const char* key, float vanilla) { return RuleFloat(key, vanilla); }
bool On(const char* key) { return R(key, 0) != 0; }

// fistp with the FPU's default rounding (nearest, ties to even) of int * float, computed wider than float like x87
int Fistp(int value, float mul) {
	return static_cast<int>(std::nearbyint(static_cast<double>(value) * static_cast<double>(mul)));
}

} // namespace

// ---- 1. black armour damage ----

int BlackArmourDamage(int damage, bool eating) { // 0x818069 .. 0x8180c0
	if (!HasRule("eliteGaps.black.walkMul"))
		return damage;
	int d = Fistp(damage, eating ? RF("eliteGaps.black.eatMul", 1.0f) : RF("eliteGaps.black.walkMul", 1.0f));
	if (d == 0) d = R("eliteGaps.black.zeroBecomes", 0); // `test ebp,ebp; jne; inc ebp`
	return d;
}

int BlackArmourSmallHitRollRange(int damage) { // 0xc00710 `cmp ebp,5; jge`
	return damage < R("eliteGaps.black.smallHitBelow", 0) ? R("eliteGaps.black.smallHitRoll", 0) : 0;
}

int BlackArmourSmallHit(int damage, int roll) { return damage <= roll ? 0 : damage; } // `cmp ebp,eax; jg`

bool BlackArmourRefusesFreeze(ZombieType type, int subVariant) { // hook 712, 0x81803b
	return type == ZOMBIE_FOOTBALL && subVariant == w7::BLACK_FOOTBALL && On("eliteGaps.black.refusesFreeze");
}

// ---- 3. the Football's option-0 roll ----

FootballDefaultRolls RollFootballDefault(const std::function<int(int)>& rand, bool above60, bool graveRise) {
	FootballDefaultRolls out;
	const int greenN = FootballGreenRange(), goldN = FootballGoldRange(), extraN = FootballExtraRange();
	if (greenN <= 0)
		return out;
	const bool grave = graveRise && On("eliteGaps.football.graveSkipsGold"); // [0x9dd48e] == 1
	const bool goldThird = On("eliteGaps.football.goldDrawsBlackRoll");
	auto extra = [&] { if (extraN > 0) out.extraRoll = rand(extraN); }; // 0x9dd406
	if (rand(greenN) == 0) { // 0xc123e2 -> 0xc123ef, ebx = 0
		out.form.subVariant = nightsub::GREEN;
		const bool gold = goldN > 0 && rand(goldN) == 0; // 0xc124d4 -> 0x9dd37a
		if (gold && !grave)
			out.form.variant = nightelite::GOLD_FOOTBALL; // 0xc12bc0, then 0xc12535 `test bl,bl` (0) -> done
		else
			extra();                                     // 0x9dd406 with bl = 0: never black
		return out;
	}
	out.blackEligible = above60;                        // 0x9dd33d `setg bl`
	if (grave) {                                        // 0x9dd346: al == 3 -> 0x9dd406, al == 2 -> done
		if (above60) extra();
		else out.blackEligible = false;
		return out;
	}
	if (goldN > 0 && rand(goldN) == 0) {                // 0x9dd395 `test eax,eax; jne`
		out.form.variant = nightelite::GOLD_FOOTBALL;   // 0xc12bc0 -> 0x9dd3a7 -> 0xc12500
		if (above60 && goldThird) extra();              // `test bl,bl` (bl = 3) -> 0x9dd406
		else out.blackEligible = false;
		return out;
	}
	extra();
	return out;
}

// ---- 4. green Football walking event, Gatling head ----

bool GreenWalkEventOn() { return On("eliteGaps.green.walkEvent"); }

GreenWalkEvent GreenFootballWalkEvent(ZombieType type, int variant, int subVariant, bool hasHelm, int age, int playerLevel) {
	GreenWalkEvent e;
	if (!GreenWalkEventOn() || type != ZOMBIE_FOOTBALL || subVariant != nightsub::GREEN || variant == gaps::GATLING_FOOTBALL)
		return e; // 0xc12b86, 0xc131c0
	if (!hasHelm || age < R("nightZombies.football.greenHelmDropAgeCs", 1 << 30))
		return e; // 0xc12caa `cmp [edi+0xc4],0`, `cmp [edi+0x60],0x320`
	e.fires = true;
	e.dropHelm = playerLevel <= R("nightZombies.football.greenHelmDropMaxPlayerLevel", -1); // 0xc13183 `cmp esi,0x46; jg`
	e.gatlingHead = playerLevel > R("eliteGaps.green.gatlingAbovePlayerLevel", 1 << 30);      // 0xc13195 `cmp esi,0x3c; jle`
	e.bodyBonus = R("nightZombies.football.greenChewBodyBonus", 0);                           // 0xc12e03, both paths
	return e;
}

bool GreenRefreshesAnimSpeed(ZombieType type, int variant, int subVariant) { // 0xc12b9d `call 0x52f050`
	return GreenWalkEventOn() && type == ZOMBIE_FOOTBALL && subVariant == nightsub::GREEN && variant != gaps::GATLING_FOOTBALL;
}

bool IsGatlingFootball(ZombieType type, int variant) { return type == ZOMBIE_FOOTBALL && variant == gaps::GATLING_FOOTBALL; }

HeadOffset GatlingFootballHeadOffset() { // 0xc12dc0 .. 0xc12de0
	return {RF("eliteGaps.gatling.headX", 0.0f), RF("eliteGaps.gatling.headY", 0.0f), RF("eliteGaps.gatling.headRot", 0.0f),
	        RF("eliteGaps.gatling.headSx", 1.0f), RF("eliteGaps.gatling.headSy", 1.0f)};
}

int GatlingFootballStartCounter() { return R("eliteGaps.gatling.startCounter", 150); } // 0xc12dee

bool GatlingFootballExtraShot(int phaseCounter) { // 0xc12e80 .. 0xc12ece
	const nlohmann::json* shots = HasRule("eliteGaps.gatling.extraShots") ? RuleArray("eliteGaps.gatling.extraShots") : nullptr;
	if (!shots)
		return false;
	for (const auto& s : *shots)
		if (s.is_number_integer() && s.get<int>() == phaseCounter)
			return true;
	return false;
}

bool GatlingFootballSpreads() { return R("eliteGaps.gatling.spreads", 1) != 0; } // 0xc12f00 / 0xc12f16

bool GatlingUpdateSkipped(ZombiePhase phase) { // 0xc03f70 `cmp [esi+0x28],0x32`
	return static_cast<int>(phase) == R("eliteGaps.gatling.skipPhase", -1);
}

// ---- 5. Newspaper at profile level > 70 ----

namespace {
bool Above70(int playerLevel) { return playerLevel > R("eliteGaps.newspaper.above70Level", 1 << 30); } // 0xc05791
}

NightForm NewspaperAbove70(const NightForm& rolled, int roll, int playerLevel, bool almanacAllowed, int eliteOption) {
	if (!Above70(playerLevel))
		return rolled;
	NightForm f{0, nightsub::FOOTBALL_HEAD}; // 0xc0579f, then 0xc0580b .. 0xc05859 for the variant
	if (roll == R("nightZombies.newspaper.almanacRoll", -1))
		f.variant = almanacAllowed || eliteOption == eliteoption::ALL ? nightelite::ALMANAC_PAPER : 0;
	else if (roll == R("nightZombies.newspaper.knifeRoll", -1))
		f.variant = nightelite::KNIFE;
	return f;
}

int NewspaperAbove70PaperHp(int playerLevel, int effectCounter, int vanilla) { // 0xc057b2 .. 0xc057e3
	if (!Above70(playerLevel) || effectCounter <= R("eliteGaps.newspaper.paperFromCounter", 1 << 30))
		return vanilla;
	if (effectCounter >= R("eliteGaps.newspaper.paperFullCounter", 1 << 30))
		return R("eliteGaps.newspaper.paperFullHp", vanilla);
	const int div = R("eliteGaps.newspaper.paperDivisor", 0);
	return div > 0 ? effectCounter / div : vanilla;
}

bool NewspaperRollsInIZombie() { return On("eliteGaps.newspaper.rollsInIZombie"); }

// ---- 6. football helmet, I, Zombie Normal ----

bool TakesFootballHelmCheck(ZombieType type, bool iZombieLevel, int eliteOption) {
	if (!HasRule("eliteGaps.helm.hp"))
		return false;
	if (type == ZOMBIE_NORMAL) return iZombieLevel;                                         // 0xc10990 (no option test)
	if (type == ZOMBIE_NEWSPAPER || type == ZOMBIE_DANCER) return eliteOption != eliteoption::NONE; // 0x7a1a0b, 0xc109ac
	return false;
}

int FootballHelmRollRange(bool iZombieLevel) { return iZombieLevel ? R("eliteGaps.helm.iZombieRoll", 0) : 0; } // 0xc109cf

bool GetsFootballHelm(bool iZombieLevel, int roll, int boardLevel, int currentWave) {
	if (!HasRule("eliteGaps.helm.hp"))
		return false;
	if (iZombieLevel) return roll == 0;                                                    // 0xc109d9 `test al,al; jne`
	return currentWave > R("eliteGaps.helm.waveAbove", 1 << 30) && boardLevel > R("eliteGaps.helm.boardLevelAbove", 1 << 30); // 0xc13621
}

int FootballHelmHp(int vanilla) { return R("eliteGaps.helm.hp", vanilla); } // 0xc109eb

int IZombieNormalCoinRollRange(bool footballHelm) { // 0xc111d5 .. 0xc111ee
	return footballHelm ? R("eliteGaps.iZombieNormal.coinRollHelm", 0) : R("eliteGaps.iZombieNormal.coinRoll", 0);
}

bool IZombieNormalIsCoin(int roll) { return roll + 1 <= R("eliteGaps.iZombieNormal.coinMaxRollPlusOne", 0); } // `add eax,ebx; cmp eax,4; jl`

const std::vector<std::string>& EliteGapsRuleKeys() {
	static const std::vector<std::string> keys = {
		"eliteGaps.black.walkMul", "eliteGaps.black.eatMul", "eliteGaps.black.zeroBecomes", "eliteGaps.black.smallHitBelow",
		"eliteGaps.black.smallHitRoll", "eliteGaps.black.refusesFreeze",
		"eliteGaps.football.goldDrawsBlackRoll", "eliteGaps.football.graveSkipsGold",
		"eliteGaps.green.walkEvent", "eliteGaps.green.gatlingAbovePlayerLevel",
		"eliteGaps.gatling.headX", "eliteGaps.gatling.headY", "eliteGaps.gatling.headRot", "eliteGaps.gatling.headSx",
		"eliteGaps.gatling.headSy", "eliteGaps.gatling.startCounter", "eliteGaps.gatling.extraShots", "eliteGaps.gatling.spreads",
		"eliteGaps.gatling.skipPhase",
		"eliteGaps.newspaper.above70Level", "eliteGaps.newspaper.paperFromCounter", "eliteGaps.newspaper.paperFullCounter",
		"eliteGaps.newspaper.paperFullHp", "eliteGaps.newspaper.paperDivisor", "eliteGaps.newspaper.rollsInIZombie",
		"eliteGaps.helm.iZombieRoll", "eliteGaps.helm.hp", "eliteGaps.helm.boardLevelAbove", "eliteGaps.helm.waveAbove",
		"eliteGaps.iZombieNormal.coinRoll", "eliteGaps.iZombieNormal.coinRollHelm", "eliteGaps.iZombieNormal.coinMaxRollPlusOne",
	};
	return keys;
}

} // namespace pvzu::unnamed
