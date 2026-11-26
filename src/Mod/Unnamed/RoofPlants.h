// PVZU: Unnamed 1.3beta2 Roof-plant rules (evidence: the research notes (w5-roof) section 1, and
// pvz-unnamed-pack/SOURCES-roof-plants.md for what was read again in the exe for this port).
// Pure functions: the engine rolls with its own RNG (Sexy::Rand(range)) and passes the roll in.
// Every constant is a pack rule (pvz-unnamed-pack/data/rules-roof-plants.json, keys "roofPlants.*") whose fallback is the
// vanilla behaviour, and every function also returns vanilla while the "roofPlants" feature is off (vanilla mode), so
// an engine site can call it without its own guard. Sites that roll check the roll range first, so vanilla never draws
// an extra Rand (deterministic replays).
// The exe's hooks never look at the world (only the Cabbage-pult looks at level 55), so in Unnamed mode these rules
// hold on every lawn. Table values (Flower Pot 500, Garlic 100 / 1200, Marigold 150 / 6000, Melon / Winter Melon launch
// 225) are in pvz-unnamed-pack/data/plants.json (Plan 1 tables), not here.
#pragma once

namespace pvzu::unnamed {

// Feature switch: GameData::Get().Feature("roofPlants") (off in vanilla mode).
bool RoofPlantsOn();

// --- Cabbage-pult (1.1: hook 352 -> 0xc05660 / 0xc10780; DoImpact 0xc05700; Draw hook 408 0xc056c0) -----------
int CabbageQuickRollRange(); // 0 = no roll (vanilla)
struct CabbageQuick {
	bool bigShot = false;   // projectile part: big cabbage (exe Projectile+0x80 = 1), velZ, accZ
	bool plantCost = false; // plant part: launch counter, HP cost, flash (not on level 55)
	int launchCounter = 0;
	int hpCost = 0;
	int flashCs = 0;        // Plant+0xbc mBeghouledFlashCountdown
	float velZ = 0.0f;
	float accZ = 0.0f;
};
CabbageQuick CabbageQuickThrow(int roll, int level);
int BigCabbageDamage(int projectileType, bool big, int tableDamage);
float BigCabbageDrawScale(int projectileType, bool big, float vanillaScale);

// --- Kernel-pult (1.3: hook 352 0x797643..0x79766c, 0x95a780; ApplyButter hook 720 0x95a700) -------------------
struct KernelShot {
	int projectileType = 0; // the shot's type after the pity rule (COBBIG after 15 kernels)
	int counter = 0;        // the plant's new kernels-in-a-row counter
	int carry = -1;         // streak the shot carries (exe Projectile+0x88) for the butter time; -1 = none
};
KernelShot KernelPityShot(int counter, int projectileType);
// Zombie::ApplyButter's new mButteredCounter. vanillaValue is what the caller would set (400, or another world's rule).
int ButterCs(int zombieType, int phase, int catapultBallsLeft, int gameMode, bool fromProjectile, int streak, int current,
	int vanillaValue);

// --- Garlic (1.5: PlantInitialize 900; aura hook 618 -> 0x7953c3 / 0x794320 / 0xc02680; bite cave 0x651374) ----
int GarlicHp(int vanilla);
int GarlicPoisonPeriodCs(); // 0 = no aura (vanilla)
bool GarlicPoisonTick(int zombieAge); // the zombie's age before this tick's ++
int GarlicPoisonDamage();
bool GarlicPoisonReaches(bool zombieIsBoss, int garlicCol, int garlicRow, int garlicX, int zombieRow, int zombieX);
int GarlicBiteChill(int chill); // the eater's new mChilledCounter
int GarlicHypnoRollRange();     // 0 = no roll
bool GarlicBiteRolls(int zombieType); // not Flag / Newspaper
bool GarlicBiteHypnotises(int roll);

// --- Umbrella Leaf (1.6: hook 731 0xc02b20): the RV block lives with Zomboss (Zomboss.h UmbrellaBlocksRv, called by
// ZombossRVLanding in Mod/Engine/ZombossSites.cpp; keys boss.umbrellaRv*).

// --- Marigold (1.7: patch 0x45fafa -> cave 0x6517b2..0x65197b, 0x7aa320, 0xc011f0; EatPlant hook 680) ----------
bool MarigoldShrinks();
bool MarigoldBiteImmune(int seedType);
bool MarigoldCanShrink(int zombieType, int variant, bool dead, bool hypnotised, int bodyMaxHp);
bool MarigoldInRange(int marigoldRow, int marigoldX, int marigoldWidth, int zombieRow, int zombieX, int zombieRectWidth);
struct ShrinkTarget {
	float scale;
	int body, bodyMax, helm, helmMax, shield, shieldMax, flying, flyingMax, chill;
};
void MarigoldShrink(ShrinkTarget& t);

// --- Melon-pult / Winter Melon (1.8: UpdateShooting hook 329 0x79751e) ------------------------------------------
bool MelonVolleyNeedsRoll(int seedType, int zombiesOnBoard); // false: no roll (not a melon, feature off, or always)
bool MelonVolleyFires(int seedType, int zombiesOnBoard, int roll);
bool MelonVolleyTarget(int zombieType, int phase, bool dead, bool hypnotised, bool sameRow, bool ahead);

// --- Flower Pot (1.2: FindCollisionTargetPlant hook 374 0xc02c30; CanAddGraveStoneAt hook 18 0xc113c0) ---------
bool ZombieShotSkipsPlant(int seedType, int plantState);
bool NoGraveOnFlowerPot();

} // namespace pvzu::unnamed
