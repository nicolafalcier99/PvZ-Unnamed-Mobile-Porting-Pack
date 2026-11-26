// PVZU: Unnamed mod-only plant rules; see ModPlantRules.h and the research notes (mod-plants-behaviour).
#include "Mod/Unnamed/ModPlantRules.h"

#include "Mod/GameData.h"
#include "Mod/Unnamed/Rules.h"

#include <cmath>
#include <cstdio>

namespace pvzu::unnamed {

namespace {
// Rule value while the feature is on, else the "no effect" value.
int MP(const char* key, int off) {
	return ModPlantsOn() ? RuleInt(key, off) : off;
}
float MPF(const char* key, float off) {
	return ModPlantsOn() ? RuleFloat(key, off) : off;
}
}

bool ModPlantsOn() {
	return GameData::Get().Feature("modPlants");
}

bool IsModSeed(int seedType) {
	return seedType >= 0x50 && seedType <= 0x7f;
}

// ---------------------------------------------------------------------------------------------------------------
// Group A: shadow state, shadow family, poison (find-A)
// ---------------------------------------------------------------------------------------------------------------
namespace {
constexpr int kA_SeedShadowPea = 0x50, kA_SeedPrimalSun = 0x51, kA_SeedFireDragon = 0x56, kA_SeedToxic = 0x58,
	kA_SeedGoldenSun = 0x59, kA_SeedDead62 = 0x62, kA_SeedDead63 = 0x63, kA_SeedCharmedTallnut = 0x67,
	kA_SeedMoonflower = 0x69, kA_SeedDreamStar = 0x6d, kA_SeedDuskPult = 0x70;
constexpr int kA_ProjDusk = 0x21;

int AbsA(int v) {
	return v < 0 ? -v : v;
}
}

bool IsShadowPlant(int seedType) {
	if (MP("modPlants.shadow.on", 0) == 0)
		return false;
	switch (seedType) { // 0x7b0660, 0xc00dc0, 0xc06130
	case kA_SeedShadowPea: case kA_SeedPrimalSun: case kA_SeedFireDragon: case kA_SeedToxic: case kA_SeedDead62:
	case kA_SeedDead63: case kA_SeedCharmedTallnut: case kA_SeedMoonflower: case kA_SeedDreamStar:
	case kA_SeedDuskPult:
		return true;
	default:
		return false;
	}
}

bool ShadowUpdateRunsInMode(int gameMode) {
	if (MP("modPlants.shadow.on", 0) == 0)
		return false;
	return gameMode != MP("modPlants.shadow.skipModeA", -1) && gameMode != MP("modPlants.shadow.skipModeB", -1);
}

int ShadowNeighbourPoints(int selfSeed, int otherSeed, bool otherShadow, bool otherSquished, bool otherIsSelf,
	int dRow, int dCol) {
	const int range = MP("modPlants.shadow.rangeCells", 0);
	if (!IsShadowPlant(selfSeed) || otherIsSelf || otherSquished || AbsA(dRow) > range || AbsA(dCol) > range ||
		!IsShadowPlant(otherSeed))
		return 0;
	const int point = MP("modPlants.shadow.moonflowerPoint", 0);
	if (selfSeed == kA_SeedMoonflower) // 0x7b072c: a Moonflower counts every shadow neighbour
		return point;
	if (otherSeed != kA_SeedMoonflower) // 0xc12bf0: others count Moonflowers only
		return 0;
	return point + (otherShadow ? MP("modPlants.shadow.shadowedMoonflowerBonus", 0) : 0);
}

int ShadowThreshold(int seedType) {
	if (!IsShadowPlant(seedType))
		return 0;
	switch (seedType) { // 0xc11460
	case kA_SeedMoonflower: return MP("modPlants.shadow.thresholdMoonflower", 0);
	case kA_SeedFireDragon: return MP("modPlants.shadow.thresholdFireDragon", 0);
	case kA_SeedDreamStar: return MP("modPlants.shadow.thresholdDreamStarfruit", 0);
	case kA_SeedCharmedTallnut: return MP("modPlants.shadow.thresholdCharmedTallnut", 0);
	default: return MP("modPlants.shadow.thresholdDefault", 0);
	}
}

bool ShadowStateFromPoints(int seedType, int points) {
	const int t = ShadowThreshold(seedType);
	return t > 0 && points >= t; // 0x7b075b setge
}

ShadowTint ShadowPlantTint(int seedType, bool shadow) {
	ShadowTint t;
	if (!shadow || !IsShadowPlant(seedType))
		return t;
	t.apply = true; // 0x7a3ce8
	t.r = MP("modPlants.shadow.tintR", 255);
	t.g = MP("modPlants.shadow.tintG", 255);
	t.b = MP("modPlants.shadow.tintB", 255);
	t.a = MP("modPlants.shadow.tintA", 255);
	return t;
}

ShadowPeashooterShot ShadowPeashooterFire(int seedType, bool shadow, int row) {
	ShadowPeashooterShot s;
	if (seedType != kA_SeedShadowPea || MP("modPlants.shadowPeashooter.on", 0) == 0)
		return s;
	s.handled = true;
	if (!shadow) { // 0x7b00e0
		s.projectileType = MP("modPlants.shadowPeashooter.projectileType", 0);
		s.originDx = MP("modPlants.shadowPeashooter.originDx", 0);
		s.originDy = MP("modPlants.shadowPeashooter.originDy", 0);
		s.potDy = MP("modPlants.shadowPeashooter.potDy", 0);
		return s;
	}
	s.beam = true; // 0x7b0012
	s.beamDamage = MP("modPlants.shadowPeashooter.beamDamage", 0);
	s.beamDamageFlags = MP("modPlants.shadowPeashooter.beamDamageFlags", 0);
	const int extra = MP("modPlants.shadowPeashooter.beamPuffBase", 0) - row; // 0x7b004f: 7 - mRow
	s.beamPuffs = 1 + (extra > 0 ? extra : 0);
	s.beamPuffStepPx = MP("modPlants.shadowPeashooter.beamPuffStepPx", 0);
	return s;
}

bool ModSunPlantProduces(int seedType) {
	return (seedType == kA_SeedPrimalSun || seedType == kA_SeedMoonflower) && MP("modPlants.modSun.on", 0) != 0;
}

int ModSunPlantTimerRollRange() {
	const int j = MP("modPlants.modSun.timerJitter", -1);
	return j >= 0 ? j + 1 : 0;
}

int ModSunPlantNextTimer(int launchRate, int roll) {
	return launchRate - MP("modPlants.modSun.timerJitter", 0) + roll; // 0x7b02b2: [rate-150, rate]
}

int PrimalSunflowerPacketRollRange(bool shadow) {
	return shadow ? MP("modPlants.primalSunflower.packetRoll", 0) : 0;
}

ModSunDrops PrimalSunflowerProduce(bool shadow, int roll) {
	ModSunDrops d;
	if (MP("modPlants.modSun.on", 0) == 0)
		return d;
	d.second = {MP("modPlants.primalSunflower.extraCoin", 0), MP("modPlants.primalSunflower.extraMotion", 0), -1};
	d.first.motion = MP("modPlants.primalSunflower.mainMotion", 0);
	if (!shadow) { // 0x7b02db
		d.first.coinType = MP("modPlants.primalSunflower.sunCoin", 0);
		return d;
	}
	d.first.coinType = MP("modPlants.primalSunflower.shadowSunCoin", 0); // 0xc13213 push 6
	const nlohmann::json* seeds = RuleArray("modPlants.primalSunflower.packetSeeds");
	if (PrimalSunflowerPacketRollRange(true) > 0 && roll >= 0 && roll < MP("modPlants.primalSunflower.packetBelow", 0) &&
		seeds && roll < static_cast<int>(seeds->size()) && (*seeds)[roll].is_number_integer()) {
		d.first.coinType = MP("modPlants.primalSunflower.packetCoin", 0); // 0xc13226 coin 0x10
		d.first.usableSeed = (*seeds)[roll].get<int>();                    // table 0xc13254
	}
	return d;
}

ModSunDrops MoonflowerProduce(bool shadow) {
	ModSunDrops d;
	if (MP("modPlants.modSun.on", 0) == 0)
		return d;
	d.first.coinType = shadow ? MP("modPlants.moonflower.shadowSunCoin", 0) : MP("modPlants.moonflower.sunCoin", 0);
	d.first.motion = MP("modPlants.moonflower.coinMotion", 0); // 0x7b031d push 3
	return d;
}

ToxicPoisonSet ToxicShadowShroomEaten(int seedType, bool plantShadow, int currentKind) {
	ToxicPoisonSet p;
	const int shadowKind = MP("modPlants.toxicShadowShroom.shadowKind", 0);
	if (seedType != kA_SeedToxic || shadowKind == 0 || currentKind == shadowKind) // 0x7a65ba: kind 2 kept
		return p;
	p.apply = true;
	p.kind = plantShadow ? shadowKind : MP("modPlants.toxicShadowShroom.normalKind", 0);
	p.timer = MP("modPlants.toxicShadowShroom.poisonTimer", 0); // 0x7ffff
	return p;
}

ToxicPoisonStep ToxicPoisonTick(int kind, int timer, bool bossClass) {
	ToxicPoisonStep s{0, false, 0, kind, timer};
	const int normal = MP("modPlants.toxicShadowShroom.normalKind", 0);
	const int shadowKind = MP("modPlants.toxicShadowShroom.shadowKind", 0);
	if (bossClass || kind == 0 || (kind != normal && kind != shadowKind)) // 0x7a674d, 0x7a6761, 0x7a676b
		return s;
	const int period = MP("modPlants.toxicShadowShroom.poisonPeriod", 0);
	if (period > 0 && timer % period == 0) { // 0x7a677b: test ebx,0x3f
		s.damage = MP("modPlants.toxicShadowShroom.poisonDamage", 0);
		s.bodyOnly = kind == shadowKind;
		if (s.bodyOnly)
			s.flashCs = MP("modPlants.toxicShadowShroom.shadowFlashCs", 0);
	}
	s.timer = timer - 1;
	if (s.timer <= 0) // 0x7a67bf
		s.kind = 0;
	return s;
}

bool ModPlantDodgesZombieShots(int seedType) {
	if (MP("modPlants.dodgeZombieShots.on", 0) == 0)
		return false;
	return seedType == kA_SeedToxic || seedType == kA_SeedGoldenSun || seedType == kA_SeedDreamStar; // 0xc02c55..
}

RepeaterPoisonStep RepeaterPoisonTick(int counter) {
	RepeaterPoisonStep s{0, counter};
	if (counter <= 0)
		return s;
	const int dmg = MP("modPlants.repeaterPoison.damagePerTick", 0);
	if (dmg <= 0)
		return s;
	s.damage = dmg; // 0xc10d0e push 1; TakeDamage
	s.counter = counter - 1;
	return s;
}

int DuskPultProjectileType(int seedType, int vanillaType) {
	if (seedType != kA_SeedDuskPult)
		return vanillaType;
	return MP("modPlants.duskPult.projectileType", vanillaType); // 0x910411
}

bool DuskPultLooksInAdjacentRows(int seedType, bool shadow) {
	return seedType == kA_SeedDuskPult && shadow && MP("modPlants.duskPult.shadowRows", 0) != 0;
}

DuskPultVolley DuskPultExtraRows(int seedType, bool shadow) {
	DuskPultVolley v;
	if (!DuskPultLooksInAdjacentRows(seedType, shadow))
		return v;
	v.above = v.below = true; // 0x7a50c8 row-1, 0x7a5105 row+1
	v.aboveDy = MPF("modPlants.duskPult.aboveDy", 0.0f);
	v.aboveDz = MPF("modPlants.duskPult.aboveDz", 0.0f);
	return v;
}

bool DuskShardSplashReaches(int projectileType, int projRow, int projRectX, int projRectW, int zombieRow,
	int zombieRectX, int zombieRectW, bool zombieHypnotised) {
	if (projectileType != kA_ProjDusk || zombieHypnotised)
		return false;
	const int rows = MP("modPlants.duskPult.splashRows", -1);
	if (rows < 0 || AbsA(zombieRow - projRow) > rows)
		return false;
	const int x = projRectX - MP("modPlants.duskPult.splashBackPx", 0); // 0x910516
	const int w = projRectW + MP("modPlants.duskPult.splashWidenPx", 0); // 0x910523 (y 0, height 600: every y)
	return zombieRectX < x + w && x < zombieRectX + zombieRectW;
}

DuskSplash DuskShardSplash(int projectileType) {
	DuskSplash s;
	if (projectileType != kA_ProjDusk)
		return s;
	s.damage = MP("modPlants.duskPult.splashDamage", 0);
	s.damageFlags = MP("modPlants.duskPult.splashFlags", 0);
	s.pushPx = MPF("modPlants.duskPult.splashPushPx", 0.0f);
	s.weakenCs = MP("modPlants.duskPult.splashWeakenCs", 0);
	return s;
}

namespace {
constexpr int kB_SeedPeashooter = 0, kB_SeedSnowPea = 5, kB_SeedMysticIce = 0x55, kB_SeedElement = 0x7a;
constexpr int kB_ZombieNewspaper = 5, kB_ZombieBungee = 20, kB_ZombieBoss = 25;
constexpr int kB_NewspaperBossVariant = 0x7f;
constexpr int kB_ProjPea = 0, kB_ProjSnowPea = 1, kB_ProjCabbage = 2, kB_ProjStar = 7, kB_ProjPrism = 0x16;
constexpr int kB_MotionStar = 7, kB_MotionHoming = 9;

int B_ArrayInt(const char* key, int idx, int off) {
	if (!ModPlantsOn())
		return off;
	const nlohmann::json* a = RuleArray(key);
	if (!a || idx < 0 || idx >= static_cast<int>(a->size()) || !(*a)[idx].is_number())
		return off;
	return (*a)[idx].get<int>();
}
bool B_ArrayHas(const char* key, int v) {
	if (!ModPlantsOn())
		return false;
	const nlohmann::json* a = RuleArray(key);
	if (!a)
		return false;
	for (const auto& e : *a)
		if (e.is_number() && e.get<int>() == v)
			return true;
	return false;
}
} // namespace

// Element-shroom ------------------------------------------------------------------------------------

int ElementShroomState(int bits, int count) {
	if (!ModPlantsOn() || count == 0) // 0x7b14a8: count 0 -> none
		return kElementNone;
	bits &= 7;
	if (bits == 0)
		return kElementFire3;
	if (bits == 7)
		return kElementIce3;
	const int ones = (bits & 1) + ((bits >> 1) & 1) + ((bits >> 2) & 1); // 0x7b14ca test bl,7; jp (parity)
	return ones == 1 ? kElementFire2Ice1 : kElementIce2Fire1;
}

ElementGain ElementShroomGainElement(int bits, int mode, int count) {
	ElementGain g{bits, count};
	if (!ModPlantsOn())
		return g;
	int al = ((bits << 1) | (mode & 1)) & 0xff; // 0x7b1501 shl al,1; or al,bl
	if (count == 0)
		al = ((al << 3) - (mode & 1)) & 0xff;   // 0x7b150a: first gain fills all three slots
	g.bits = al & 7;
	if (count < 0x7fff)                         // 0x7b1517 add dx,1; jo
		g.count = count + 1;
	return g;
}

ElementTick ElementShroomTick(int stateCountdown, int count) {
	ElementTick t;
	if (!ModPlantsOn())
		return t;
	if (stateCountdown < 1) // 0x7b1461
		t.resetCountdownTo = count > 0 ? MP("modPlants.elementShroom.elementPeriodCs", 0)
		                               : MP("modPlants.elementShroom.firstElementCs", 0);
	t.gain = stateCountdown == 1; // 0x7b147e
	return t;
}

int ElementShroomToggleMode(int mode) {
	return ModPlantsOn() ? (mode ^ 1) : mode;
}

int ElementShroomCountdownDigit(int stateCountdown) {
	if (!ModPlantsOn())
		return 0;
	const int d = stateCountdown / 100 + 1; // 0x7b1536 idiv 100; +1; cmp 9; ja
	return (d >= 0 && d <= 9) ? d : 0;
}

int ElementShroomShootingCounter() {
	return MP("modPlants.elementShroom.shootingCounter", 0);
}

bool ElementShroomPulseAt(int shootingCounter) {
	return B_ArrayHas("modPlants.elementShroom.pulseCounters", shootingCounter);
}

bool ElementShroomPuffAt(int shootingCounter) {
	return B_ArrayHas("modPlants.elementShroom.puffCounters", shootingCounter);
}

int ElementShroomRowReach(int state) {
	if (state == kElementFire2Ice1)
		return MP("modPlants.elementShroom.wideRowReach", 1);
	return 1; // vanilla Gloom: rows -1..+1
}

ElementRect ElementShroomAttackRect(int state, int plantX, int plantY) {
	ElementRect r;
	const int half = MP("modPlants.elementShroom.wideRectHalf", 0);
	const int size = MP("modPlants.elementShroom.wideRectSize", 0);
	if (state != kElementFire2Ice1 || size <= 0)
		return r;
	r.own = true;
	r.x = plantX - half; // 0x7a91b4 sub edx,0xa0
	r.y = plantY - half;
	r.w = size;          // 0x7a91ba mov esi,0x190
	r.h = size;
	return r;
}

int ElementShroomIceRollRange(int state) {
	return state == kElementIce3 ? MP("modPlants.elementShroom.iceRoll", 0) : 0;
}

ElementHit ElementShroomHit(int state, int vanillaDamage, int vanillaFlags, int iceRoll, int zombieType, int zombieX,
	bool zombieBossClass) {
	ElementHit h;
	h.damage = vanillaDamage;
	h.damageFlags = vanillaFlags;
	if (!ModPlantsOn())
		return h;
	h.damage = MP("modPlants.elementShroom.pulseDamage", vanillaDamage); // 0x7a9088 mov edi,0x10
	switch (state) {
	case kElementFire2Ice1:
		h.damage = MP("modPlants.elementShroom.wideDamage", h.damage); // 0x7a9093
		break;
	case kElementFire3:
		h.damage = MP("modPlants.elementShroom.fireDamage", h.damage); // 0x7a90a6 mov edi,0x38
		h.thaw = true;
		if (!zombieBossClass) // 0xc05746 call 0x7aa320
			h.stripRollRange = MP("modPlants.elementShroom.fireStripRoll", 0);
		break;
	case kElementIce3: {
		h.damageFlags = MP("modPlants.elementShroom.iceDamageFlags", vanillaFlags); // 0x7a90e3
		if (MP("modPlants.elementShroom.iceRoll", 0) <= 0)
			break;
		if (iceRoll == 0) { // 0x7a9104
			h.freeze = true;
			h.damage = 0;
			h.iceTrapCs = MP("modPlants.elementShroom.iceFreezeCs", 0);
		} else if (iceRoll == 1 && zombieType != kB_ZombieBungee && zombieType != kB_ZombieBoss &&
		           zombieX < MP("modPlants.elementShroom.icePushMaxX", 0)) { // 0x7a911f
			h.pushPx = MP("modPlants.elementShroom.icePushPx", 0);
		}
		break;
	}
	default:
		break;
	}
	return h;
}

int ElementShroomFireStrip(int roll, bool hasHelm, bool hasShield) {
	if (!ModPlantsOn() || MP("modPlants.elementShroom.fireStripRoll", 0) <= 0 || roll != 0)
		return kElementStripNone;
	if (hasHelm)
		return kElementStripHelm;   // 0xc05624 DropHelm
	if (hasShield)
		return kElementStripShield; // 0xc0563e
	return kElementStripNone;
}

ElementHeal ElementShroomPulseHeal(int state, int hp, int maxHp) {
	ElementHeal r{hp, maxHp};
	const int add = MP("modPlants.elementShroom.healPerPulse", 0);
	if (state != kElementIce2Fire1 || add <= 0)
		return r;
	const int cap = MP("modPlants.elementShroom.healMaxCap", 0);
	r.maxHp = maxHp + add > cap ? cap : maxHp + add;  // 0x7b17a2..0x7b17b1
	r.hp = hp + add > r.maxHp ? r.maxHp : hp + add;   // 0x7b17b4..0x7b17c0
	return r;
}

bool ElementShroomIgnoresPlantern() {
	return MP("modPlants.elementShroom.noPlanternBoost", 0) != 0;
}

// Prism-shroom --------------------------------------------------------------------------------------

bool PrismShroomClickable(bool asleep, int shootingCounter) {
	return ModPlantsOn() && MP("modPlants.prismShroom.clickToFire", 0) != 0 && !asleep && shootingCounter == 0;
}

PrismClick PrismShroomClick(bool asleep, int shootingCounter, int sun, bool canTakeSun, bool freeShot) {
	PrismClick c;
	c.sun = sun;
	c.shootingCounter = shootingCounter;
	if (!PrismShroomClickable(asleep, shootingCounter)) // 0x7a6014..0x7a6028
		return c;
	if (!canTakeSun && !freeShot) { // 0x7a6060: buzzer, sun counter flashes
		c.buzzer = true;
		c.sunFlashCs = MP("modPlants.prismShroom.outOfSunFlashCs", 0);
		return c;
	}
	c.fires = true;
	c.shootingCounter = MP("modPlants.prismShroom.shootingCounter", 0); // 0x7a60c4
	c.sun = sun - PrismShroomShotCost();                               // 0x7a60d1
	if (c.sun < 0)
		c.sun = 0;
	return c;
}

int PrismShroomShotCost() {
	return MP("modPlants.prismShroom.shotCost", 0);
}

int PrismShroomProjectileType() {
	return MP("modPlants.prismShroom.projectileType", -1);
}

int PrismShroomShotDamage(int sunAtShot) {
	if (!ModPlantsOn())
		return 0;
	const int cap = MP("modPlants.prismShroom.damageSunCap", 0);
	const int div = MP("modPlants.prismShroom.damageSunDivisor", 1);
	int s = sunAtShot > cap ? cap : sunAtShot; // 0x7978f9
	if (s < 0)
		s = 0;
	return MP("modPlants.prismShroom.damageBase", 0) + (div > 0 ? s / div : 0); // sar edx,2; add 0x12c
}

// Dream Starfruit / star kinds ----------------------------------------------------------------------

int DreamStarfruitColourRollRange(int appCounter) {
	const int mask = MP("modPlants.dreamStarfruit.colourRerollMask", 0);
	if (mask <= 0 || (appCounter & mask) != 0) // 0x959df9 test ecx,0xc
		return 0;
	return MP("modPlants.dreamStarfruit.colours", 0);
}

int DreamStarfruitStarKind(int colourIndex) {
	return ModPlantsOn() ? colourIndex + 1 : 0; // 0x959e74 inc eax
}

int DreamStarfruitReturnTimer(bool shadow) {
	return shadow ? MP("modPlants.dreamStarfruit.shadowReturnCs", 0) : 0;
}

bool DreamStarfruitAlwaysTargets(bool shadow) {
	return shadow && MP("modPlants.dreamStarfruit.shadowAlwaysTargets", 0) != 0;
}

bool StarKindTurnsHoming(int projectileType, int motion, int kind, int clickBackoff) {
	return ModPlantsOn() && projectileType == kB_ProjStar && motion == kB_MotionStar && kind != 0 &&
	       (clickBackoff & 0xff) == 1; // 0xc00ec3 cmp al,1
}

uint32_t StarKindColour(int kind) {
	if (kind < 1 || kind > 5)
		return 0;
	return static_cast<uint32_t>(B_ArrayInt("modPlants.dreamStarfruit.palette", kind - 1, 0));
}

int StarKindDamageBonus(int kind) {
	if (kind < 1 || kind > 5) // 0x959fc0/0x959fcd: 1..5 only
		return 0;
	return B_ArrayInt("modPlants.dreamStarfruit.damageBonus", kind - 1, 0);
}

int StarKindHitFate(int projectileType, int kind, int hitsSoFar) {
	if (!ModPlantsOn() || projectileType != kB_ProjStar || kind <= 0 || kind == 1) // 0x959f3a, 0xc00f2e
		return kStarHitVanilla;
	int lives = 0;
	if (kind == 2)
		lives = MP("modPlants.dreamStarfruit.redPierceHits", 0);  // 0x959f55 cmp [+0x18],5
	else if (kind == 4)
		lives = MP("modPlants.dreamStarfruit.greyPierceHits", 0); // 0x959f92 cmp [+0x18],3
	else if (kind == 3 || kind == 5)
		return kStarHitDies;                                      // 0x959f76 / 0x959f83
	else
		return kStarHitVanilla;
	return hitsSoFar + 1 < lives ? kStarHitSurvives : kStarHitDies; // the counter (+0x18) starts at 1 (mVisible)
}

namespace {
bool B_FootballNewspaper(const StarZombie& z) {
	return z.type == kB_ZombieNewspaper && z.subVariant == 1; // 0xc05a20
}
bool B_SmallTarget(const StarZombie& z) {
	return z.scale <= 1.0f && z.bodyHp < MP("modPlants.dreamStarfruit.effectMaxHp", 0); // 0x95a8a0 / 0x95a8d0
}
} // namespace

int StarKindImpactRollRange(int kind, const StarZombie& z) {
	if (!ModPlantsOn() || kind < 1 || kind > 5)
		return 0;
	switch (kind) {
	case 1: // CanBeFrozen first, then Rand(7)
		return z.canBeFrozen ? MP("modPlants.dreamStarfruit.blueFreezeRoll", 0) : 0;
	case 2:
		return B_FootballNewspaper(z) ? 0 : MP("modPlants.dreamStarfruit.redHelmRoll", 0);
	case 3:
		if (!B_FootballNewspaper(z) && (z.type == kB_ZombieBoss || z.variant == kB_NewspaperBossVariant))
			return 0;
		if (!B_SmallTarget(z))
			return 0;
		return z.iZombie ? MP("modPlants.dreamStarfruit.purpleHypnoRollIZombie", 0)
		                 : MP("modPlants.dreamStarfruit.purpleHypnoRoll", 0);
	case 5:
		if (B_FootballNewspaper(z) || z.type == kB_ZombieBoss || z.variant == kB_NewspaperBossVariant)
			return 0;
		if (!B_SmallTarget(z))
			return 0;
		return z.iZombie ? MP("modPlants.dreamStarfruit.greenBlowRollIZombie", 0)
		                 : MP("modPlants.dreamStarfruit.greenBlowRoll", 0);
	default:
		return 0;
	}
}

StarImpact StarKindImpact(int kind, const StarZombie& z, int roll) {
	StarImpact s;
	if (!ModPlantsOn() || kind < 1 || kind > 5)
		return s;
	if (kind == 4) { // 0x95a095 add word [+0x1a],400 (also on the football Newspaper)
		s.weakenAddCs = MP("modPlants.dreamStarfruit.greyWeakenCs", 0);
		return s;
	}
	if (StarKindImpactRollRange(kind, z) <= 0 || roll != 0)
		return s;
	switch (kind) {
	case 1: // 0xc059d0
		if (B_FootballNewspaper(z))
			s.chill = true;
		else if (z.type != kB_ZombieBoss)
			s.freeze = true;
		break;
	case 2: // 0xc02d58 cmp [+0xcc],120000; jg skip
		s.dropHelm = z.bodyHp <= MP("modPlants.dreamStarfruit.redHelmMaxHp", 0);
		break;
	case 3:
		s.hypnotise = true; // 0x95a07a..0x95a085
		break;
	case 5:
		s.blowAway = true;  // 0x95a0ce mov byte [+0xb9],1
		break;
	default:
		break;
	}
	return s;
}

int ZombieWeakenRollRange(int weakenCs, int zombieType, int subVariant) {
	if (!ModPlantsOn() || weakenCs <= 0 || zombieType != kB_ZombieNewspaper || subVariant != 1)
		return 0;
	return MP("modPlants.dreamStarfruit.weakenFootballRoll", 0); // 0xc05a92 Rand(2)
}

int ZombieWeakenedDamage(int damage, int weakenCs, int zombieType, int subVariant, int roll) {
	if (!ModPlantsOn() || weakenCs <= 0)
		return damage;
	if (zombieType == kB_ZombieNewspaper && subVariant == 1) // 0xc05a70
		return roll == 0 ? damage : (damage >> 2);
	return damage * MP("modPlants.dreamStarfruit.weakenDamageMult", 1); // 0x95a12d shl ebp,1
}

int ZombieWeakenTick(int weakenCs) {
	return (ModPlantsOn() && weakenCs > 0) ? weakenCs - 1 : weakenCs; // 0x95a147 dec word
}

// Spirit-tail ---------------------------------------------------------------------------------------

int SpiritTailRollRange() {
	return MP("modPlants.spiritTail.roll", 0);
}

int SpiritTailProjectileType(int roll) {
	if (SpiritTailRollRange() <= 0)
		return 0;
	return B_ArrayHas("modPlants.spiritTail.projectileTypes", roll) ? roll : -1; // 0xc106c4..0xc106e9
}

int SpiritTailExtraRollRange(int projectileType) {
	if (projectileType == kB_ProjCabbage)
		return MP("modPlants.spiritTail.bigCabbageRoll", 0); // 0xc10614 Rand(2)
	if (projectileType == kB_ProjStar)
		return MP("modPlants.spiritTail.starKindRoll", 0);   // 0xc10635 Rand(6)
	return 0;
}

int SpiritTailProjectileField80(int projectileType, int roll) {
	if (!ModPlantsOn())
		return 0;
	if (projectileType == kB_ProjCabbage)
		return SpiritTailExtraRollRange(projectileType) > 0 && roll == 0 ? 1 : 0; // sete [+0x80]
	if (projectileType == kB_ProjStar)
		return SpiritTailExtraRollRange(projectileType) > 0 ? roll : 0;           // [+0x80] = Rand(6)
	if (projectileType == kB_ProjPrism)
		return MP("modPlants.spiritTail.prismDamage", 0);                          // 0xc10650 0x1db
	return 0;
}

int SpiritTailMotion() {
	return ModPlantsOn() ? MP("modPlants.spiritTail.motion", -1) : -1;
}

// Mystic-Ice Shooter --------------------------------------------------------------------------------

int MysticIceVelRollRange() {
	return MP("modPlants.mysticIce.velYRoll", 0);
}

MysticIceShot MysticIceTickShot(int plantX, int plantY, int roll) {
	MysticIceShot s;
	const int range = MysticIceVelRollRange();
	if (range <= 0)
		return s;
	s.fires = true;
	s.x = plantX + MP("modPlants.mysticIce.shotOffsetX", 0); // 0xc04772 add [esp],0x45
	s.y = plantY + MP("modPlants.mysticIce.shotOffsetY", 0); // 0xc0476b add [esp],0xa
	s.projectileType = MP("modPlants.mysticIce.shotType", 0);
	s.motion = MP("modPlants.mysticIce.shotMotion", 0);
	s.damageRangeFlags = MP("modPlants.mysticIce.shotRangeFlags", 0);
	s.velX = MPF("modPlants.mysticIce.shotVelX", 0.0f);
	const int div = MP("modPlants.mysticIce.velYDivisor", 1);
	s.velY = static_cast<float>(roll) / static_cast<float>(div > 0 ? div : 1) -
	         MPF("modPlants.mysticIce.velYOffset", 0.0f); // 0xc047a4..0xc047b4
	return s;
}

bool MysticIceConvertsProjectile(int projectileType, int hitTorchwoodCol, bool dead) {
	return ModPlantsOn() && MP("modPlants.mysticIce.convertPeas", 0) != 0 && projectileType == kB_ProjPea &&
	       hitTorchwoodCol == -1 && !dead; // 0xc04d90, 0xc047d4
}

int MysticIceConvertedType() {
	return MP("modPlants.mysticIce.convertTo", kB_ProjPea);
}

MysticIceLife MysticIceLifeTick(int launchRate) {
	MysticIceLife l{launchRate, false, -1};
	if (!ModPlantsOn() || MP("modPlants.mysticIce.lifeIsLaunchRate", 0) == 0)
		return l;
	l.launchRate = launchRate - 1; // 0xc047e7 dec [+0x5c]
	if (l.launchRate <= 0) {
		l.dies = true;
		l.deathParticle = MP("modPlants.mysticIce.deathParticle", -1);
	}
	return l;
}

int MysticIceUpgradeSeed(int seedType) {
	if (!ModPlantsOn() || seedType != MP("modPlants.mysticIce.upgradeFrom", -1))
		return -1;
	return MP("modPlants.mysticIce.upgradeTo", -1); // 0xc04853 push 5
}

bool ModPlantZomboniFreezeImmuneB(int seedType) {
	return (seedType == kB_SeedElement || seedType == kB_SeedMysticIce) &&
	       MP("modPlants.zomboniImmune.elementAndMysticIce", 0) != 0;
}

// ---------------------------------------------------------------------------------------------------- fork C
namespace {
constexpr int kC_ZombieBoss = 25, kC_ZombieNewspaper = 5, kC_NewspaperBossVariant = 0x7f;
constexpr int kC_ProjKnockback = 0x22, kC_ProjPoison = 0x23, kC_ProjButter = 12;
constexpr int kC_SeedCherry = 2, kC_SeedShrapnel = 0x52, kC_SeedDoom = 0xf, kC_SeedKernel = 0x22;
constexpr int kC_StateCobArming = 0x23;
constexpr int kC_VanillaBurnCap = 1800;
}

// Sniper ---------------------------------------------------------------------------------------------------
int SniperHpSum(const SniperCandidate& z) {
	int sum = z.bodyHp;
	if (z.helmType != 0) sum += z.helmHp;
	if (z.shieldType != 0) sum += z.shieldHp;
	if (z.flyingMaxHp != 0) sum += z.flyingHp;
	return sum;
}

static bool SniperSkipsBoss(const SniperCandidate& z) {
	// 0x7aa320: bodyMax > 120000 or Zomboss, or 0x7aa130 Newspaper boss form
	if (z.bodyMaxHp > MP("modPlants.sniper.bossMaxHp", 120000) || z.zombieType == kC_ZombieBoss)
		return true;
	return z.zombieType == kC_ZombieNewspaper && z.variant == kC_NewspaperBossVariant;
}

bool SniperCanPick(const SniperCandidate& z, int sniperRightX) {
	if (!ModPlantsOn())
		return false;
	return !SniperSkipsBoss(z) && z.hasHead && z.effectedByDamage && z.x >= sniperRightX;
}

int SniperPickTarget(const std::vector<SniperCandidate>& zombies, int sniperRightX) {
	int best = -1, bestHp = 0;
	for (int i = 0; i < (int)zombies.size(); i++) {
		if (!SniperCanPick(zombies[i], sniperRightX))
			continue;
		const int hp = SniperHpSum(zombies[i]);
		if (best < 0 || hp >= bestHp) { // 0x7b128f: jl skip -> a tie goes to the later zombie
			best = i;
			bestHp = hp;
		}
	}
	return best;
}

bool SniperKeepsTarget(const SniperCandidate& z, int sniperRightX) {
	if (!ModPlantsOn())
		return false;
	return sniperRightX < z.x && z.hasHead && z.effectedByDamage; // 0x7b136d: jge drop
}

int SniperCountdownAfterTick(int countdownAfterDecrement) {
	if (!ModPlantsOn() || countdownAfterDecrement >= 1)
		return countdownAfterDecrement;
	return MP("modPlants.sniper.periodCs", countdownAfterDecrement);
}

SniperTick SniperTimer(int countdown) {
	SniperTick t;
	if (!ModPlantsOn())
		return t;
	t.warn = countdown == MP("modPlants.sniper.warnAtCs", -1);
	if (t.warn)
		t.foley = MP("modPlants.sniper.warnFoley", 0); // 0x7b13a0
	if (countdown == MP("modPlants.sniper.fireAtCs", -1)) {
		t.damage = MP("modPlants.sniper.damage", 0);
		t.fire = t.damage > 0;
		t.foley = MP("modPlants.sniper.fireFoley", 0); // 0x7b13c2
	}
	return t;
}

// Primal Repeater ------------------------------------------------------------------------------------------
int PrimalRepeaterRollRange() {
	return MP("modPlants.primalRepeater.shotRoll", 0);
}

int PrimalRepeaterShotType(int roll, int vanillaType) {
	if (PrimalRepeaterRollRange() <= 0)
		return vanillaType;
	if (roll == 0) return kC_ProjKnockback; // 0xc10bc0: add eax, 0x22
	if (roll == 1) return kC_ProjPoison;
	return vanillaType;
}

bool PrimalRepeaterSecondPea() {
	return MP("modPlants.primalRepeater.secondPea", 0) != 0;
}

int PrimalKnockbackPx(int projectileType) {
	return projectileType == kC_ProjKnockback ? MP("modPlants.primalRepeater.knockbackPx", 0) : 0;
}

int PrimalPoisonAddCs(int projectileType) {
	return projectileType == kC_ProjPoison ? MP("modPlants.primalRepeater.poisonAddCs", 0) : 0;
}

// Shrapnel Cherry ------------------------------------------------------------------------------------------
std::vector<ShrapnelShard> ShrapnelCherryShards() {
	std::vector<ShrapnelShard> v;
	if (MP("modPlants.shrapnelCherry.shards", 0) == 0)
		return v;
	const float s = MPF("modPlants.shrapnelCherry.straightSpeed", 4.0f);  // 0x794080: -4.0 / 0x7940c1: 4.0
	const float d = MPF("modPlants.shrapnelCherry.diagonalSpeed", 2.8f);  // 0x40333333
	v.push_back({0.0f, 0.0f, 2});
	v.push_back({0.0f, -s, 1});
	v.push_back({0.0f, s, 1});
	v.push_back({d, d, 1});
	v.push_back({-d, -d, 1});
	v.push_back({d, -d, 1});
	v.push_back({-d, d, 1});
	return v;
}

int ShrapnelShardHitDamage(int kind) {
	if (kind != 1 && kind != 2)
		return 0;
	return MP("modPlants.shrapnelCherry.shardHitDamage", 0);
}

bool ShrapnelCoreReplants(int kind, int ageCs) {
	const int age = MP("modPlants.shrapnelCherry.coreReplantAgeCs", 0);
	return age > 0 && kind == 2 && ageCs == age;
}

int ShrapnelCoreReplantColumn(int starX) {
	const int w = MP("modPlants.shrapnelCherry.replantColumnPx", 80);
	return w > 0 ? starX / w : 0;
}

bool ShrapnelCherryExplodesWhenSquished(int seedType) {
	if (seedType == kC_SeedCherry)
		return true; // the day-world rule (hook 303), kept here so the site asks one function
	return seedType == kC_SeedShrapnel && MP("modPlants.shrapnelCherry.squishExplodes", 0) != 0;
}

// Loaded Cannon --------------------------------------------------------------------------------------------
bool LoadedCannonArmsByItself() {
	return MP("modPlants.loadedCannon.selfArms", 1) != 0;
}

CannonLoad LoadedCannonLoad(int cannonState, int plantedSeed) {
	if (MP("modPlants.loadedCannon.loadable", 0) == 0 || cannonState != kC_StateCobArming)
		return CannonLoad::None;
	if (plantedSeed == kC_SeedKernel) return CannonLoad::Kernel;
	if (plantedSeed == kC_SeedDoom) return CannonLoad::Doom;
	return CannonLoad::None;
}

int LoadedCannonPacketPenaltyCs(CannonLoad load) {
	return load == CannonLoad::None ? 0 : MP("modPlants.loadedCannon.packetPenaltyCs", 0);
}

int LoadedCannonCobBurnCap(int ammoSeed) {
	return ammoSeed == kC_SeedKernel ? MP("modPlants.loadedCannon.kernelBurnCap", 0) : 0;
}

int LoadedCannonDoomBurnCap() {
	return MP("modPlants.loadedCannon.doomBurnCap", 0);
}

bool LoadedCannonDoomClearsGraves() {
	return MP("modPlants.loadedCannon.doomClearsGraves", 0) != 0;
}

int BurnCapDamage(int armourAndBodySum, int cap) {
	const int c = cap > 0 ? cap : kC_VanillaBurnCap;
	return armourAndBodySum >= c ? c : -1;
}

// Butter-pult ----------------------------------------------------------------------------------------------
int ButterPultProjectile(int vanillaType) {
	return MP("modPlants.butterPult.alwaysButter", 0) != 0 ? kC_ProjButter : vanillaType;
}

int ButterPultButterStreak() {
	return 0; // 0xc10bc7: the streak carry runs for seed 0x22 only
}

int ButterPultDamageRangeFlags(int vanillaFlags) {
	return MP("modPlants.butterPult.damageRangeFlags", vanillaFlags);
}

// Bowling Wall-nut -----------------------------------------------------------------------------------------
bool BowlingWallnutRolls(bool isWallnutBowlingLevel) {
	return isWallnutBowlingLevel || MP("modPlants.bowlingWallnut.rollsAnywhere", 0) != 0;
}

bool BowlingWallnutEdible() {
	return MP("modPlants.bowlingWallnut.notEaten", 0) == 0;
}

bool BowlingWallnutRandomAnimRate() {
	return MP("modPlants.bowlingWallnut.fixedAnimRate", 0) == 0;
}

int BowlingRoofSlopeY(int x) { return x < 440 ? (440 - x) / 4 : 0; } // Board::GetPosYBasedOnRow's (440 - x) * 0.25

int BowlingLastRow(bool sixRows) { return sixRows ? 5 : 4; }

bool BowlingNutInItsSquare(int plantCol, int startRow, int row, int centreCol) {
	return row == startRow && centreCol == plantCol;
}

// --- Mod plants: shared rules (fork D) ----------------------------------------------------------------------
namespace {
constexpr int kD_SeedFirstMod = 0x50, kD_SeedLastMod = 0xff;
constexpr int kD_SeedBowlingNut = 0x53, kD_SeedDevourer = 0x5b, kD_SeedHallucination = 0x5c, kD_SeedLilyMod = 0x60,
	kD_SeedCharmedTallnut = 0x67, kD_SeedRewind = 0x6b, kD_SeedHolyPumpkin = 0x6e, kD_SeedPotMod = 0x71;
constexpr int kD_ZombieBungee = 20, kD_ZombieBoss = 25, kD_ZombieImp = 24;
constexpr int kD_PhaseDying = 1, kD_PhaseBurned = 2, kD_PhaseMowered = 3, kD_PhaseDancerDancingIn = 40,
	kD_PhaseNewspaperMad = 31;
constexpr int kD_StateGraveBusterEating = 9;
constexpr int kD_ModeRegenOff = 0x2b;

// x87 fistp after fild * float: round to nearest (even) of the exact product.
int RoundMul(int v, float f) {
	return static_cast<int>(std::nearbyint(static_cast<double>(v) * static_cast<double>(f)));
}
}

int ModPlantInitSeed(int seed) {
	if (!ModPlantsOn() || MP("modPlants.initAsBase", 0) == 0)
		return seed;
	return (seed >= kD_SeedFirstMod && seed <= kD_SeedLastMod) ? seed - kD_SeedFirstMod : seed; // 0x7a3e1a
}

int ModPlantLawnSlot(int seed) {
	if (!ModPlantsOn())
		return 0;
	switch (seed) { // 0x7a5040..0x7a5079
	case kD_SeedDevourer:
	case kD_SeedBowlingNut:
	case kD_SeedRewind:
		return 3;
	case kD_SeedHolyPumpkin:
		return 2;
	case kD_SeedLilyMod:
	case kD_SeedPotMod:
		return 1;
	default:
		return 0;
	}
}

bool ModPlantBossClass(int zombieType, int bodyMaxHp, bool newspaperBossForm) {
	return bodyMaxHp > MP("modPlants.bossClassMinMaxHp", 120000) || zombieType == kD_ZombieBoss || newspaperBossForm;
}

// Rewind Grass ----------------------------------------------------------------------------------------------

int RewindGrassBlowDelayCs() {
	return MP("modPlants.rewindGrass.blowDelayCs", 0);
}

float RewindGrassBlowAnimRate() {
	return MPF("modPlants.rewindGrass.blowAnimRate", 0.0f);
}

bool RewindGrassAffects(bool bossClass, int phase, int zombieX, bool hasHead) {
	if (RewindGrassBlowDelayCs() <= 0)
		return false;
	if (bossClass || phase == kD_PhaseDying || phase == kD_PhaseBurned || phase == kD_PhaseMowered)
		return false;
	return zombieX < MP("modPlants.rewindGrass.maxX", 800) && hasHead; // cmp [ebx+8],0x320; jge
}

bool RewindGrassSendsBack(int zombieType, bool mindControlled, bool blowingAway) {
	if (RewindGrassBlowDelayCs() <= 0)
		return false;
	return zombieType != kD_ZombieBungee && !mindControlled && !blowingAway; // 0x7b0b1d, word [+0xb8]
}

float RewindGrassBackX() {
	return MPF("modPlants.rewindGrass.backX", 0.0f);
}

bool RewindGrassResetsPhaseCounter(int phase) {
	return RewindGrassBlowDelayCs() > 0 && phase == kD_PhaseDancerDancingIn;
}

void RewindGrassHeal(ZombieHpSet& hp) {
	if (RewindGrassBlowDelayCs() <= 0)
		return;
	if (hp.body < hp.bodyMax) hp.body = hp.bodyMax;
	if (hp.helm < hp.helmMax) hp.helm = hp.helmMax;
	if (hp.shield < hp.shieldMax) hp.shield = hp.shieldMax;
	if (hp.flying < hp.flyingMax) hp.flying = hp.flyingMax;
}

bool RewindGrassUntouchable(int seed) {
	return seed == kD_SeedRewind && MP("modPlants.rewindGrass.untouchable", 0) != 0;
}

// Charmed Tall-nut ------------------------------------------------------------------------------------------

CharmedTallnutBite CharmedTallnutOnBite(int counter, int damage, int hpBefore, bool fromBite) {
	CharmedTallnutBite r{counter, false};
	const int first = MP("modPlants.charmedTallnut.firstSummonHp", 0);
	const int step = MP("modPlants.charmedTallnut.summonStepHp", 0);
	if (first <= 0 || step <= 0 || !fromBite)
		return r;
	r.counter += damage < hpBefore ? damage : hpBefore; // 0x9500be: min(damage, HP)
	const int q = r.counter / first;                     // idiv 0x5dc
	if (q >= 1) {
		r.counter -= q * step; // imul 0x4b0
		r.summon = true;       // one zombie, whatever q is
	}
	return r;
}

int CharmedTallnutSummonRollRange() {
	if (!ModPlantsOn())
		return 0;
	const nlohmann::json* a = RuleArray("modPlants.charmedTallnut.summonTypes");
	return a ? static_cast<int>(a->size()) : 0;
}

int CharmedTallnutSummonType(int roll) {
	if (!ModPlantsOn())
		return -1;
	const nlohmann::json* a = RuleArray("modPlants.charmedTallnut.summonTypes");
	if (!a || roll < 0 || roll >= static_cast<int>(a->size()) || !(*a)[roll].is_number_integer())
		return -1;
	return (*a)[roll].get<int>();
}

int CharmedTallnutSummonBodyHp(int bodyHp) {
	const float mul = MPF("modPlants.charmedTallnut.summonBodyHpMul", 1.0f);
	return RoundMul(bodyHp, mul);
}

int CharmedTallnutRiseCounterCs() {
	return MP("modPlants.charmedTallnut.riseCounterCs", 0);
}

int CharmedTallnutBiteDamage(int damage, bool shadow) {
	if (!shadow)
		return damage;
	return RoundMul(damage, MPF("modPlants.charmedTallnut.shadowBiteMul", 1.0f));
}

int CharmedTallnutShadowThreshold() {
	return MP("modPlants.charmedTallnut.shadowThreshold", 0);
}

CharmedTallnutGarg CharmedTallnutEnterShadow(int hp, int maxHp) {
	CharmedTallnutGarg g;
	g.plantHpAfter = hp;
	const float minFrac = MPF("modPlants.charmedTallnut.gargMinHpFrac", 0.0f);
	if (minFrac <= 0.0f)
		return g;
	if (hp < RoundMul(maxHp, minFrac)) // 0xc00fa9: jl skip
		return g;
	g.summon = true;
	g.gargBodyHp = hp >> 1;            // sar ebx,1
	g.plantHpAfter = hp - RoundMul(maxHp, MPF("modPlants.charmedTallnut.gargPlantCostFrac", 0.0f));
	return g;
}

NutRegen CharmedTallnutRegenTick(bool shadow, int gameMode, int hp, int maxHp, int lastHp, int counter) {
	NutRegen r{hp, counter, lastHp};
	const int delay = MP("modPlants.charmedTallnut.regenDelayCs", 0);
	if (!shadow || delay <= 0 || gameMode == kD_ModeRegenOff)
		return r;
	if (hp != lastHp) { // 0x651df6: damaged since last tick -> count restarts
		r.counter = 0;
		r.lastHp = hp;
		return r;
	}
	r.counter = counter + 1;
	if (r.counter < delay)
		return r;
	r.hp = hp + MP("modPlants.charmedTallnut.regenHp", 0);
	if (r.hp > maxHp)
		r.hp = maxHp;
	r.counter = 0;
	r.lastHp = r.hp;
	return r;
}

int CharmedTallnutBiteBackRollRange() {
	return MP("modPlants.charmedTallnut.biteBackCharmRoll", 0);
}

CharmedTallnutBiteBack CharmedTallnutOnEaten(bool shadow, int eaterPhase, bool eaterHasObject, int eaterVariant, int roll) {
	CharmedTallnutBiteBack b;
	if (!shadow || CharmedTallnutBiteBackRollRange() <= 0)
		return b;
	if (eaterPhase == kD_PhaseNewspaperMad && eaterHasObject &&
	    eaterVariant == MP("modPlants.charmedTallnut.instantCharmVariant", -1)) {
		b.charmEater = true; // 0xc021d2: straight to the charm
		return b;
	}
	b.damageToEater = MP("modPlants.charmedTallnut.biteBackDamage", 0);
	b.charmEater = roll == 0; // Rand(200) == 0
	return b;
}

bool ModSeedCountsAsTallnut(int seed) {
	return seed == kD_SeedCharmedTallnut && MP("modPlants.charmedTallnut.actsAsTallnut", 0) != 0;
}

// Hallucination-shroom --------------------------------------------------------------------------------------

bool HallucinationTurnsEaterToImp(int seed, int plantHpAfterBite, bool eaterBossClass) {
	if (seed != kD_SeedHallucination || MP("modPlants.hallucination.impMaxHp", 0) <= 0)
		return false;
	return plantHpAfterBite <= 0 && !eaterBossClass;
}

HallucinationImp HallucinationImpMorph(int bodyHp) {
	HallucinationImp m;
	const int maxHp = MP("modPlants.hallucination.impMaxHp", 0);
	if (maxHp <= 0)
		return m;
	m.zombieType = kD_ZombieImp;
	m.bodyMaxHp = maxHp;
	m.bodyHp = bodyHp < maxHp ? bodyHp : maxHp; // 0x7a6694: cap only
	m.rectX = 36;  // 0x7a6662..0x7a6680 (= vanilla Imp rects)
	m.rectW = 42;
	m.attackRectX = 50;
	m.attackRectW = 20;
	m.reanimType = 57; // push 0x39 = REANIM_IMP
	m.particle = 71;   // 0x47 = PARTICLE_IMITATER_MORPH
	return m;
}

// Magic Devourer --------------------------------------------------------------------------------------------

int MagicDevourerCountdownTick(int state, int countdown) {
	const int fastFrom = MP("modPlants.magicDevourer.fastCountdownFrom", 0);
	if (fastFrom <= 0 || state != kD_StateGraveBusterEating)
		return countdown;
	return countdown >= fastFrom ? countdown - 1 : countdown; // 0x7b07de
}

bool MagicDevourerSwallowsNow(int state, int countdown, int col, int row, bool bowlingLevel) {
	const int at = MP("modPlants.magicDevourer.swallowAtCountdown", 0);
	if (at <= 0 || state != kD_StateGraveBusterEating || countdown != at)
		return false;
	if (col < 0 || col > MP("modPlants.magicDevourer.maxCol", 8) || row < 0 || row > MP("modPlants.magicDevourer.maxRow", 5))
		return false;
	return !bowlingLevel; // 0x453840
}

int MagicDevourerCardCoinType() {
	return MP("modPlants.magicDevourer.cardCoinType", 0);
}

int MagicDevourerCardCoinMotion() {
	return MP("modPlants.magicDevourer.cardCoinMotion", 0);
}

// Holy-fire Pumpkin -----------------------------------------------------------------------------------------

bool HolyFirePumpkinActsAsPumpkin(int seed) {
	return seed == kD_SeedHolyPumpkin && MP("modPlants.holyFirePumpkin.actsAsPumpkin", 0) != 0;
}

bool HolyFirePumpkinGuardsSquare(int seedInSquare) {
	return seedInSquare == kD_SeedHolyPumpkin && MP("modPlants.holyFirePumpkin.freezeGuard", 0) != 0;
}

// ---- Fork E implementations ----------------------------------------------------------------------------------------
namespace {
constexpr int kE_ZombieBoss = 25;
constexpr int kE_StateDoingSpecial = 2, kE_StateSunshroomSmall = 23;
constexpr int kE_SeedCoffee = 0x23, kE_SeedImitater = 0x30, kE_SeedMagicDevourer = 0x5b, kE_SeedGoldenSun = 0x59,
	kE_SeedStrongCoffee = 0x73;
constexpr int kE_MotionStraight = 0, kE_MotionBackwards = 6, kE_MotionStar = 7;
constexpr int kE_ProjStar = 7, kE_ProjFirstMod = 14, kE_ProjField80 = 22;
constexpr int kE_PlantingOk = 0, kE_PlantingNotHere = 1, kE_PlantingNeedsSleeping = 13;

const nlohmann::json* E_Arr(const char* key) {
	return ModPlantsOn() ? RuleArray(key) : nullptr;
}
int E_ArrAt(const char* key, int index, int fallback) {
	const nlohmann::json* a = E_Arr(key);
	if (!a || index < 0 || index >= static_cast<int>(a->size()) || !(*a)[index].is_number_integer())
		return fallback;
	return (*a)[index].get<int>();
}
}

// Fire-dragon Grass ----------------------------------------------------------------------------------------------

bool FireDragonInRange(int zombieType, bool hypnotised, bool effectedByFlags, int zombieRow, int zombieX, int plantRow,
	int plantX, int plantCol) {
	if (FireDragonAttackCs() <= 0)
		return false;
	if (zombieType == kE_ZombieBoss) { // 0xc026c0: Zomboss only from column 7, no row/range test
		if (plantCol < MP("modPlants.fireDragon.bossMinCol", 99))
			return false;
	} else {
		const int reach = MP("modPlants.fireDragon.rowReach", 0);
		const int dRow = zombieRow - plantRow;
		if (dRow < -reach || dRow > reach || !effectedByFlags)
			return false;
		const int back = MP("modPlants.fireDragon.rangeBackPx", 0);
		const int front = MP("modPlants.fireDragon.rangeFrontPx", 0);
		const unsigned d = static_cast<unsigned>(zombieX - plantX + back); // 0x7b0532: add 0x28; cmp 0x118; ja
		if (d > static_cast<unsigned>(back + front))
			return false;
	}
	return !hypnotised; // 0xc061c0: cmp byte [zombie+0xb8], 1
}

int FireDragonDamageFlags() {
	return MP("modPlants.fireDragon.damageFlags", 0);
}

int FireDragonAttackCs() {
	return MP("modPlants.fireDragon.attackCs", 0);
}

int FireDragonHitAtCountdown() {
	return MP("modPlants.fireDragon.hitAtCountdown", -1);
}

int FireDragonFlameBudget() {
	return MP("modPlants.fireDragon.flameBudget", 0);
}

int FireDragonHitDamage(int zombieBodyMaxHp) {
	const int div = MP("modPlants.fireDragon.damageMaxHpDivisor", 0);
	if (div <= 0)
		return 0;
	const int base = MP("modPlants.fireDragon.damageBase", 0);
	const int cap = MP("modPlants.fireDragon.damageCap", 0);
	const int d = static_cast<int>(std::lround(base + static_cast<double>(zombieBodyMaxHp) / div)); // fistp, nearest
	return d > cap ? cap : d;
}

FireDragonHit FireDragonSpend(int damage, int budgetLeft, bool shadowState) {
	FireDragonHit h;
	if (FireDragonFlameBudget() <= 0)
		return h;
	int left = budgetLeft - damage; // 0xc060b3: sub [budget], eax
	h.damage = damage;
	if (left <= 0) {                // jg skip; else dmg += budget; budget = 0
		h.damage = damage + left;
		left = 0;
	}
	h.budgetLeft = left;
	h.poisonAdd = shadowState ? MP("modPlants.fireDragon.shadowPoison", 0) : 0; // 0xc060e9: add word [+0x1a], 400
	return h;
}

bool FireDragonClearsChillAndFreeze() {
	return MP("modPlants.fireDragon.clearsChill", 0) != 0;
}

bool FireDragonZomboniFreezeImmune() {
	return MP("modPlants.fireDragon.zomboniFreezeImmune", 0) != 0;
}

// Golden Sun-shroom ----------------------------------------------------------------------------------------------

GoldenSunShroomStep GoldenSunShroomUpdate(int gameMode, bool onBoard, bool notOnGround, int state, int timerCs) {
	GoldenSunShroomStep s;
	const int burst = MP("modPlants.goldenSunShroom.burstCs", 0);
	if (burst <= 0 || gameMode == MP("modPlants.goldenSunShroom.offMode", -1) || !onBoard || notOnGround)
		return s;
	if (state == kE_StateSunshroomSmall) { // 0x7b0d01: first tick -> doing special, vanish in 150
		s.state = kE_StateDoingSpecial;
		s.disappearCs = burst;
		s.timerCs = burst;
		return s;
	}
	if (state != kE_StateDoingSpecial)
		return s;
	const nlohmann::json* at = E_Arr("modPlants.goldenSunShroom.dropAtCs");
	if (!at)
		return s;
	for (const auto& v : *at) {
		if (v.is_number_integer() && v.get<int>() == timerCs) { // 0x7b0d2d: 125 / 75 / 25
			s.dropSun = true;
			s.coinType = MP("modPlants.goldenSunShroom.coinType", 0);
			s.coinMotion = timerCs == MP("modPlants.goldenSunShroom.coinMotionCoinAtCs", -1)
				? MP("modPlants.goldenSunShroom.coinMotionAlt", 0)
				: MP("modPlants.goldenSunShroom.coinMotion", 0);
			break;
		}
	}
	return s;
}

int GoldenSunShroomPacketStartCounter(int seedType, int imitaterType) {
	const bool golden = seedType == kE_SeedGoldenSun || (seedType == kE_SeedImitater && imitaterType == kE_SeedGoldenSun);
	return golden ? MP("modPlants.goldenSunShroom.packetStartCounter", -1) : -1;
}

// Strong Coffee Bean ---------------------------------------------------------------------------------------------

bool StrongCoffeeTriggers(int doSpecialCountdown, bool onBoard) {
	const int at = MP("modPlants.strongCoffee.triggerCountdown", 0);
	return at > 0 && onBoard && doSpecialCountdown == at;
}

bool StrongCoffeeWakes(int coffeeRow, int coffeeCol, int plantRow, int plantCol, bool plantNotOnGround) {
	if (StrongCoffeeWakeCs() <= 0 || plantNotOnGround)
		return false;
	const int r = MP("modPlants.strongCoffee.wakeRadius", 0);
	const int dr = plantRow - coffeeRow, dc = plantCol - coffeeCol;
	return dr >= -r && dr <= r && dc >= -r && dc <= r;
}

int StrongCoffeeWakeCs() {
	return MP("modPlants.strongCoffee.wakeCs", 0);
}

int CoffeeLikePlantingReason(int seedType, bool hasFlying, bool hasNormal, bool normalAsleep, int normalWakeUpCounter,
	bool normalGrabbedByBungee) {
	const bool modOn = ModPlantsOn();
	if (seedType != kE_SeedCoffee && !(modOn && (seedType == kE_SeedStrongCoffee || seedType == kE_SeedMagicDevourer)))
		return -1; // not a coffee-like seed: caller keeps its own rule
	if (hasFlying)
		return kE_PlantingNotHere;
	// 0x40e137: the mod made "must be asleep" a never-taken jo
	const bool needAsleep = MP("modPlants.strongCoffee.anyPlantOk", 0) == 0;
	if (!hasNormal || (needAsleep && !normalAsleep) || normalWakeUpCounter > 0 || normalGrabbedByBungee)
		return kE_PlantingNeedsSleeping;
	return kE_PlantingOk;
}

CoffeeHeal CoffeeBeanDeathHeal(int dyingSeed, int dyingState, int hp, int maxHp) {
	CoffeeHeal h{hp, maxHp};
	const int tenths = MP("modPlants.coffeeHeal.tenths", 0);
	if (tenths <= 0 || dyingState != kE_StateDoingSpecial ||
		(dyingSeed != kE_SeedCoffee && dyingSeed != kE_SeedStrongCoffee))
		return h;
	int heal = maxHp / 10 * tenths; // 0xc05db5: idiv 10; imul 3
	const int cap = MP("modPlants.coffeeHeal.cap", 0);
	if (heal >= cap) // jl keep; else cap
		heal = cap;
	if (hp + heal >= maxHp) // 0xc05dda: overheal raises the max
		h.maxHp = hp + heal;
	h.hp = hp + heal;
	return h;
}

// Leaf Shield ----------------------------------------------------------------------------------------------------

bool LeafShieldReflects(int projDamageRangeFlags, int projRow, int projX, int leafRow, int leafX, int leafWidth) {
	const int band = MP("modPlants.leafShield.bandPx", 0);
	if (band <= 0 || projDamageRangeFlags != 0) // 0x7b0de7: only shots that do not hurt zombies
		return false;
	const int reach = MP("modPlants.leafShield.rowReach", 0);
	const int dRow = projRow - leafRow;
	if (dRow < -reach || dRow > reach)
		return false;
	const int inside = MP("modPlants.leafShield.bandInsidePx", 0);
	const unsigned d = static_cast<unsigned>(projX - leafX - leafWidth + inside); // 0x7b0e03..0x7b0e12
	return d <= static_cast<unsigned>(band);
}

int LeafShieldHpCost(int projectileDamage) {
	const int shift = MP("modPlants.leafShield.hpCostShift", -1);
	if (shift < 0)
		return 0;
	return (projectileDamage >> shift) + MP("modPlants.leafShield.hpCostBase", 0);
}

LeafShieldBounce LeafShieldBounceOf(int motionType) {
	LeafShieldBounce b;
	b.motionType = motionType;
	if (MP("modPlants.leafShield.bandPx", 0) <= 0)
		return b;
	b.damageRangeFlags = MP("modPlants.leafShield.reflectedFlags", 0);
	b.flashCs = MP("modPlants.leafShield.flashCs", 0);
	if (motionType == kE_MotionBackwards)
		b.motionType = kE_MotionStraight; // 0x7b0e73
	else if (motionType == kE_MotionStar)
		b.negateVelX = true; // 0x7b0e80: fchs mVelX
	return b;
}

int ModProjectileDamage(int projectileType, int tableDamage, int field80, bool iZombieLevel) {
	if (!ModPlantsOn())
		return tableDamage;
	if (projectileType < kE_ProjFirstMod) {
		if (projectileType == kE_ProjStar && !iZombieLevel && MP("modPlants.projectileDamage.starDouble", 0) != 0)
			return tableDamage * 2; // 0xc10025: shl edx, 1
		return tableDamage;
	}
	const int v = E_ArrAt("modPlants.projectileDamage.byType", projectileType, 0);
	return projectileType == kE_ProjField80 && v < 0 ? field80 : v; // 0x7b08fb: type 22 = [proj+0x80]
}

// Boss Flower ----------------------------------------------------------------------------------------------------

int BossFlowerRollRange() {
	return MP("modPlants.bossFlower.rollRange", 0);
}

int BossFlowerSeedFromRoll(int roll) {
	if (BossFlowerRollRange() <= 0)
		return -1;
	return E_ArrAt("modPlants.bossFlower.seedTable", roll, 0) == 1 ? roll : -1; // 0xc000e0: cmp byte [r+0xc000fd], 1
}

int BossFlowerCoinType() {
	return MP("modPlants.bossFlower.coinType", 0);
}

int BossFlowerCoinMotion() {
	return MP("modPlants.bossFlower.coinMotion", 0);
}

} // namespace pvzu::unnamed
