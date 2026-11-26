// PVZU: Unnamed Roof zombies and elite forms. Section numbers refer to the research notes (w5-roof); exe addresses are
// listed per key in pvz-unnamed-pack/SOURCES-roof-zombies.md.
#include "Mod/Unnamed/RoofZombies.h"

#include "Mod/Unnamed/FogZombies.h"
#include "Mod/Unnamed/Rules.h"

#include <cmath>

namespace pvzu::unnamed {

namespace {

int R(const char* key, int vanilla) { return RuleInt(key, vanilla); }
float RF(const char* key, float vanilla) { return RuleFloat(key, vanilla); }
bool On(const char* key) { return R(key, 0) != 0; }

constexpr int kAttackChew = 0;     // ZombieAttackType (Lawn/Zombie.h)
constexpr int kAttackLadder = 3;

bool IsGarg(ZombieType t) { return t == ZOMBIE_GARGANTUAR || t == ZOMBIE_REDEYE_GARGANTUAR; }

} // namespace

bool IsRoofType(ZombieType t) {
	return t == ZOMBIE_BUNGEE || t == ZOMBIE_LADDER || t == ZOMBIE_CATAPULT || IsGarg(t);
}

const char* RoofEliteName(ZombieType type, int variant) {
	switch (type) {
	case ZOMBIE_BUNGEE:
		return variant == roofelite::WHEELBARROW ? "wheelbarrow" : variant == roofelite::JACK_BUNGEE ? "jack-head" : "none";
	case ZOMBIE_LADDER:
		return variant == roofelite::SHOVEL ? "shovel" : variant == roofelite::LADDER_PILE ? "ladder-pile"
			: variant == roofelite::GHOST_LADDER ? "ghost" : "none";
	case ZOMBIE_CATAPULT: return variant == roofelite::POLE_HEAD ? "pole-head" : "none";
	case ZOMBIE_GARGANTUAR:
	case ZOMBIE_REDEYE_GARGANTUAR: return variant == roofelite::STAFF ? "staff" : "none";
	default: return "none";
	}
}

// ---- spawn rolls ----

int BungeeRollRange(int setting) { // 2.1, hook 488 0x796eb2
	if (setting == eliteoption::NONE) return 0;
	return R(setting == eliteoption::ALL ? "roofZombies.bungee.eliteRollAllElite" : "roofZombies.bungee.eliteRoll", 0);
}

int RollBungee(int roll, bool forceJack) { // 0xc04440: (roll & 1) + 0x16; forced 0x17 in I, Zombie / Zomboss bungees
	if (roll < 0 || roll >= R("roofZombies.bungee.eliteBelow", 0))
		return 0;
	if (forceJack)
		return roofelite::JACK_BUNGEE;
	return (roll & 1) ? roofelite::JACK_BUNGEE : roofelite::WHEELBARROW;
}

int LadderShovelRollRange(int setting) { // 2.2, hook 487 0x796d1e
	return setting == eliteoption::NONE ? 0 : R("roofZombies.ladder.shovelRoll", 0);
}

int RollLadderShovel(int roll, int setting) {
	if (setting == eliteoption::ALL) // 0x7b2000: `jne` -> `jge` at 0x796d2c
		return roll >= 0 && roll < R("roofZombies.ladder.shovelBelowAllElite", 0) ? roofelite::SHOVEL : 0;
	return roll == R("roofZombies.ladder.shovelHit", -1) ? roofelite::SHOVEL : 0;
}

int LadderSecondRollRange(int value, int setting) { // 0x95aa10: skipped with option 1 ("no elites")
	if (value == eliteoption::NONE) return 0;
	return R(setting == eliteoption::ALL ? "roofZombies.ladder.secondRollAllElite" : "roofZombies.ladder.secondRoll", 0);
}

int RollLadderSecond(int roll, int plantCount) {
	if (LadderSecondRollRange(eliteoption::DEFAULT) <= 0)
		return 0;
	if (roll == 1 && plantCount >= R("roofZombies.ladder.ghostMinPlants", 5)) // Board+0xbc >= 5
		return roofelite::GHOST_LADDER;
	return roll == 0 || roll == 1 ? roofelite::LADDER_PILE : 0;
}

int CatapultRollRange(int setting) { // 2.3, hook 498 0x796e09
	return setting == eliteoption::NONE ? 0 : R("roofZombies.catapult.poleHeadRoll", 0);
}

int RollCatapult(int roll, int value, int setting) {
	if (CatapultRollRange(setting) <= 0 || value == eliteoption::NONE)
		return 0;
	const bool all = setting == eliteoption::ALL;
	if (roll >= 0 && roll < R(all ? "roofZombies.catapult.poleHeadBelowAllElite" : "roofZombies.catapult.poleHeadBelow", 0))
		return roofelite::POLE_HEAD;
	// up to 0x910000's bound: the world-7 vase (PlayerInfo level > 60, World7Zombies), else the pole head when the
	// value is "all elites" (0x91009b)
	if (value == eliteoption::ALL &&
		roll < R(all ? "roofZombies.catapult.poleHeadAllEliteBelowAllElite" : "roofZombies.catapult.poleHeadAllEliteBelow", 0))
		return roofelite::POLE_HEAD;
	return 0;
}

bool GargStaffHit(int poleHit, int eliteOption) { // 2.4, hook 513 0xc116a0
	const int mod = R("roofZombies.garg.staffMod", 0);
	if (mod <= 0 || eliteOption == eliteoption::NONE)
		return false;
	return eliteOption == eliteoption::ALL || poleHit % mod == R("roofZombies.garg.staffHit", -1);
}

// ---- hit points ----

int RoofBodyHp(ZombieType type, int variant, int vanilla) {
	if (type == ZOMBIE_BUNGEE && variant == roofelite::WHEELBARROW)
		return vanilla + R("roofZombies.bungee.wheelbarrowHpBonus", 0);   // 0xc04440 +0x2ee
	if (type == ZOMBIE_CATAPULT) return R("roofZombies.catapult.hp", vanilla);              // patch 0x522e87
	if (type == ZOMBIE_REDEYE_GARGANTUAR) return R("roofZombies.garg.gigaHp", vanilla);     // hook 514 / 0x523e44
	return vanilla;
}

int RoofShieldHp(ZombieType type, int variant, int vanilla) {
	return type == ZOMBIE_LADDER && variant == roofelite::SHOVEL ? vanilla + R("roofZombies.ladder.shovelShieldBonus", 0) : vanilla;
}

// ---- Bungee ----

int BungeeBottomCounter(int variant, int vanilla) { // hook 527 0x796ffd
	if (BungeeRollRange() <= 0) return vanilla;
	if (variant == roofelite::WHEELBARROW) return R("roofZombies.bungee.wheelbarrowBottomCs", vanilla);
	if (variant == roofelite::JACK_BUNGEE) return R("roofZombies.bungee.jackBottomCs", vanilla);
	return vanilla;
}

bool WheelbarrowBlocksCell(int bungeeRow, int bungeeTargetCol, int col, int row) { // hook 46 0x796f8b
	const int r = R("roofZombies.bungee.noPlantRadius", -1);
	if (r < 0) return false;
	return std::abs(row - bungeeRow) <= r && std::abs(col - bungeeTargetCol) <= r;
}

bool JackBungeeLeavesJack(int variant) { return variant == roofelite::JACK_BUNGEE && BungeeRollRange() > 0; }

float JackBungeeJackX(float plantCellPixelX) { return plantCellPixelX - static_cast<float>(R("roofZombies.bungee.jackOffsetPx", 0)); }

bool BungeeCarriesZombies() { return On("roofZombies.bungee.carryZombies"); }

bool BungeeCanCarry(bool bungeeHypnotised, ZombieType type, int bodyMaxHp, bool zombieHypnotised, bool zombieDead) { // 0xc02530
	if (!BungeeCarriesZombies() || zombieDead || type == ZOMBIE_BUNGEE || type == ZOMBIE_BOSS)
		return false;
	if (bodyMaxHp > R("roofZombies.bungee.carryMaxHp", 0))
		return false;
	return bungeeHypnotised != zombieHypnotised;   // EffectedByDamage(0x81) / (1)
}

bool ZombieCoversColumn(int rectX, int rectWidth, int col) { // 0x957680 (ROUND = round half to even, as the x87 FPU)
	const int first = static_cast<int>(std::nearbyint(rectX / 80.0)) - 1;
	const int last = static_cast<int>(std::nearbyint((rectX + rectWidth) / 80.0)) - 1;
	return col >= first && col <= last;
}

// ---- Ladder ----

bool LadderTargetsAnyPlant(int variant) { // hook 655 0x796dd6 (+ the pile's own block 0xc02c90)
	if (LadderShovelRollRange() <= 0) return false;
	return variant == roofelite::SHOVEL || variant == roofelite::LADDER_PILE;
}

std::optional<bool> PileCanTarget(int variant, int attackType, bool cellHasLadder) { // 0xc02c90
	if (variant != roofelite::LADDER_PILE || LadderSecondRollRange(eliteoption::DEFAULT) <= 0)
		return std::nullopt;
	if (attackType == kAttackChew)
		return cellHasLadder ? std::nullopt : std::optional<bool>(false);
	if (attackType == kAttackLadder)
		return !cellHasLadder;
	return std::nullopt;
}

bool ShovelDigsUpPlant(int variant) { return variant == roofelite::SHOVEL && LadderShovelRollRange() > 0; }   // hook 600

bool PileCallsBungee(int variant) { return variant == roofelite::LADDER_PILE && LadderSecondRollRange(eliteoption::DEFAULT) > 0; }

bool KeepsShieldOnLadder(int variant) { return variant == roofelite::GHOST_LADDER && LadderSecondRollRange(eliteoption::DEFAULT) > 0; }

int PileSpares(int playerLevel) { // 0xc02cc0: PlayerInfo+0x24 < 0x3c -> 5, else 11
	return playerLevel < R("roofZombies.ladder.pileSparesLateLevel", 0) ? R("roofZombies.ladder.pileSpares", 0)
	                                                                     : R("roofZombies.ladder.pileSparesLate", 0);
}

int PileLadderHp() { return R("roofZombies.ladder.pileLadderHp", 0); }   // hook 722

int PileBodyToShield(int variant, bool hasLadder, int bodyDamage) { // hook 702 0xc02d0f: `sar dmg, 1`
	return variant == roofelite::LADDER_PILE && hasLadder && PileSpares(0) > 0 ? bodyDamage >> 1 : 0;
}

bool PileOverflowsIntoNextLadder(int variant) { return variant == roofelite::LADDER_PILE && PileSpares(0) > 0; }   // hook 710

bool PileBites(int variant, bool hasLadder) { // hook 672 0x79c300
	return !(variant == roofelite::LADDER_PILE && hasLadder && PileSpares(0) > 0);
}

float LadderAnimRateMultiplier(int variant, bool hasLadder, bool eating) { // hook 662 -> 0x95c380 / 0x7a6be3
	if (variant == roofelite::LADDER_PILE)
		return hasLadder ? RF("roofZombies.ladder.pileCarrySpeed", 1.0f) : RF("roofZombies.ladder.pileFreeSpeed", 1.0f);
	if (variant == roofelite::GHOST_LADDER && !eating)
		return RF("roofZombies.ladder.ghostSpeed", 1.0f);
	return 1.0f;
}

int GhostTeleportAge() { return R("roofZombies.ladder.ghostTeleportAge", -1); }   // hook 619 0x95c2a0

float GhostTeleportX(int maxColumnWithoutLadder) {
	return static_cast<float>((maxColumnWithoutLadder + 1) * R("roofZombies.ladder.ghostTeleportStepPx", 0));
}

int GhostBiteDamage(int variant, int plantMaxHp, int plantHp, int vanilla) { // hook 790 0x95c3b0
	if (variant != roofelite::GHOST_LADDER || GhostTeleportAge() < 0)
		return vanilla;
	return plantMaxHp <= R("roofZombies.ladder.ghostOneBiteMaxHp", 0) ? plantHp + 1 : R("roofZombies.ladder.ghostToughBite", vanilla);
}

int GhostAlpha(int variant) { return variant == roofelite::GHOST_LADDER ? R("roofZombies.ladder.ghostAlpha", 255) : 255; }   // 0x95c230

bool SkipsShieldDamageImages(int variant) { return variant == roofelite::SHOVEL && LadderShovelRollRange() > 0; }   // hook 696

// ---- Catapult ----

bool CatapultPrefers(int variant, int candidateColumn, int currentColumn) { // hook 544 0x796e70
	if (variant == roofelite::POLE_HEAD && CatapultRollRange() > 0)
		return candidateColumn > currentColumn;
	return candidateColumn < currentColumn;
}

FaithHit CatapultFaith(int bodyHp, int damage, int ballsLeft, bool wallnutBowling) { // hook 705 -> 0xc00000
	const int lock = R("roofZombies.catapult.faithLockHp", -1);
	if (lock < 0 || wallnutBowling || ballsLeft == 0 || bodyHp - damage > lock)
		return {damage, bodyHp};
	return {damage & 1, lock};
}

bool CatapultTooToughToAsh(int ballsLeft) { return ballsLeft != 0 && R("roofZombies.catapult.faithLockHp", -1) >= 0; }   // hook 793

// ---- Gargantuar / Giga ----

bool GargThrowsAtAnyDistance() { return On("roofZombies.garg.throwAnyDistance"); }   // patch 0x527216 `jp` -> `jo`

float GargThrowDistance(float computed, float minimum) { // patch 0x527056 (`test ah, 0x41; nop; nop`)
	return On("roofZombies.garg.throwMinDistanceAlways") ? minimum : computed;
}

int StaffThrowRollRange() { return R("roofZombies.garg.staffThrowRoll", 0); }   // hook 565 0xc11500

bool StaffThrowTypeAccepted(int r) { // re-rolled while 1, or > 9 and not 12 / 15 and (< 17 or 19 or 25)
	if (r < 0 || r >= 33 || r == 1)
		return false;
	if (r <= 9 || r == 12 || r == 15)
		return true;
	return r >= 17 && r != 19 && r != 25;
}

bool StaffKeepsThrowing(int boardZombies, bool hypnotised) { // hook 568 0xc11700: Board+0xa0 < 0x33
	const int cap = R("roofZombies.garg.staffMaxBoardZombies", -1);
	return cap >= 0 && boardZombies <= cap && !hypnotised;
}

// ---- thrown Imp ----

bool ThrownImpSkipsHouse(ZombiePhase phase) { // hook 630 0x7a0281: phases 0x47 / 0x48
	return On("roofZombies.imp.thrownSkipsHouse") && (phase == PHASE_IMP_GETTING_THROWN || phase == PHASE_IMP_LANDING);
}

// ---- art ----

std::vector<RoofArtImage> RoofArtFor(ZombieType type, int variant) {
	if (type == ZOMBIE_BUNGEE && variant == roofelite::WHEELBARROW) return {{"anim_head1", "images/Zen_WheelBarrow"}};
	if (type == ZOMBIE_BUNGEE && variant == roofelite::JACK_BUNGEE) return {{"anim_head1", "reanim/Zombie_jackbox_box"}};
	if (type == ZOMBIE_LADDER && variant == roofelite::SHOVEL) return {{"Zombie_ladder_1", "images/Shovel_hi_res"}};
	if (type == ZOMBIE_CATAPULT && variant == roofelite::POLE_HEAD)
		return {{"Zombie_catapult_driver_head", "particles/ZombiePoleVaulterHead"}};
	if (IsGarg(type) && variant == roofelite::STAFF)
		return {{"Zombie_gargantuar_telephonepole", "reanim/Zombie_gargantuar_staff"}};
	return {};
}

std::vector<RoofArtImage> ResolveRoofArt(ZombieType type, int variant, const std::function<bool(const std::string&)>& available) {
	auto a = RoofArtFor(type, variant);
	for (const auto& img : a)
		if (!available(img.file))
			return {};
	return a;
}

const std::vector<std::string>& RoofZombieRuleKeys() {
	static const std::vector<std::string> keys = {
		"roofZombies.bungee.eliteRollAllElite", "roofZombies.ladder.shovelBelowAllElite",
		"roofZombies.ladder.secondRollAllElite", "roofZombies.catapult.poleHeadBelowAllElite",
		"roofZombies.catapult.poleHeadAllEliteBelowAllElite",
		"roofZombies.bungee.eliteRoll", "roofZombies.bungee.eliteBelow", "roofZombies.bungee.wheelbarrowHpBonus",
		"roofZombies.bungee.wheelbarrowBottomCs", "roofZombies.bungee.jackBottomCs", "roofZombies.bungee.noPlantRadius",
		"roofZombies.bungee.jackOffsetPx", "roofZombies.bungee.carryZombies", "roofZombies.bungee.carryMaxHp",
		"roofZombies.ladder.shovelRoll", "roofZombies.ladder.shovelHit", "roofZombies.ladder.shovelShieldBonus",
		"roofZombies.ladder.secondRoll", "roofZombies.ladder.ghostMinPlants", "roofZombies.ladder.pileSpares",
		"roofZombies.ladder.pileSparesLate", "roofZombies.ladder.pileSparesLateLevel", "roofZombies.ladder.pileLadderHp",
		"roofZombies.ladder.pileCarrySpeed", "roofZombies.ladder.pileFreeSpeed", "roofZombies.ladder.ghostSpeed",
		"roofZombies.ladder.ghostTeleportAge", "roofZombies.ladder.ghostTeleportStepPx", "roofZombies.ladder.ghostOneBiteMaxHp",
		"roofZombies.ladder.ghostToughBite", "roofZombies.ladder.ghostAlpha",
		"roofZombies.catapult.hp", "roofZombies.catapult.poleHeadRoll", "roofZombies.catapult.poleHeadBelow",
		"roofZombies.catapult.poleHeadAllEliteBelow", "roofZombies.catapult.faithLockHp",
		"roofZombies.garg.gigaHp", "roofZombies.garg.staffMod", "roofZombies.garg.staffHit", "roofZombies.garg.staffThrowRoll",
		"roofZombies.garg.staffMaxBoardZombies", "roofZombies.garg.throwAnyDistance", "roofZombies.garg.throwMinDistanceAlways",
		"roofZombies.imp.thrownSkipsHouse",
	};
	return keys;
}

} // namespace pvzu::unnamed
