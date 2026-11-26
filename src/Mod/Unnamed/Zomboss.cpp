// PVZU: Unnamed Dr. Zomboss (default settings). Rule ids refer to the research notes (w5-roof) section 4.1; exe addresses
// are listed per key in pvz-unnamed-pack/SOURCES-zomboss.md.
#include "Mod/Unnamed/Zomboss.h"

#include "Mod/Unnamed/Rules.h"

#include <cmath>

namespace pvzu::unnamed {

namespace {

int R(const char* key, int vanilla) { return RuleInt(key, vanilla); }
bool On(const char* key) { return R(key, 0) != 0; }

bool InList(const char* key, int value) {
	const nlohmann::json* list = RuleArray(key);
	if (list == nullptr)
		return false;
	for (const auto& v : *list)
		if (v.is_number_integer() && v.get<int>() == value)
			return true;
	return false;
}

constexpr int kSeedIceshroom = 14, kSeedJalapeno = 20, kSeedImitater = 48;
constexpr int kStompFirstCol = 5, kStompLastCol = 9; // the vanilla stomp area (BossStompContact, col >= 5) on 9 columns

} // namespace

// ---- B1 ----

int BossStartHp(bool adventure, int vanilla) {
	return adventure ? R("boss.hpAdventure", vanilla) : R("boss.hpOther", vanilla);
}

// ---- B3 ----

int BossLateSpawnRollRange() { return R("boss.lateSpawnRoll", 0); }

bool BossLateSpawnAccepted(int roll, int targetRow, bool killedYeti) {
	if (roll < 0 || roll >= BossLateSpawnRollRange())
		return false;
	if (InList("boss.lateSpawnExclude", roll))
		return false;
	if (targetRow == 0 && InList("boss.lateSpawnExcludeRow0", roll)) // the top row has no room for a Gargantuar
		return false;
	if (killedYeti && InList("boss.lateSpawnExcludeYetiKilled", roll)) // Board+0x5760 mKilledYeti
		return false;
	return true;
}

ZombieType PickBossLateSpawn(int targetRow, bool killedYeti, const std::function<int(int)>& rand) {
	const int n = BossLateSpawnRollRange();
	int r = 0;
	do
		r = rand(n);
	while (!BossLateSpawnAccepted(r, targetRow, killedYeti));
	return static_cast<ZombieType>(r);
}

bool BossCopiesHypnosis() { return On("boss.copiesHypnosis"); }

// ---- B4 ----

bool BossEmptyStompHeals() { return R("boss.stompNothingHeal", 0) > 0; }

int BossHpAfterEmptyStomp(int hp) { return hp + R("boss.stompNothingHeal", 0); }

// ---- B5 ----

bool BossCrushesHypnotised() { return R("boss.crushHypnotisedDamage", 0) > 0; }

int BossCrushDamage() { return R("boss.crushHypnotisedDamage", 0); }

unsigned BossCrushDamageFlags(bool bossHypnotised) { return bossHypnotised ? 0xdu : 0x8du; }

int RectColumn(int px) { return static_cast<int>(std::lrint(px / 80.0)) - 1; }

bool StompHitsZombie(int zombieRow, int rectX, int rectWidth, int targetRow) { // 0x957ef0
	if (zombieRow < targetRow || zombieRow > targetRow + 1)
		return false;
	return RectColumn(rectX) >= kStompFirstCol && RectColumn(rectX + rectWidth) <= kStompLastCol;
}

bool RvHitsZombie(int zombieRow, int rectX, int rectWidth, int targetRow, int targetCol) { // 0x957bf0
	const int right = RectColumn(rectX + rectWidth);
	const int dRow = zombieRow - targetRow;
	return right >= targetCol && right <= targetCol + 2 && dRow >= 0 && dRow <= 1;
}

bool BossCrushOpensPots() { return On("boss.crushOpensPots"); }

bool StompOpensPot(int gridX, int gridY, int targetRow) { // 0x959be0
	return gridY >= targetRow && gridY <= targetRow + 1 && gridX > 4 && gridX < 10;
}

bool RvOpensPot(int gridX, int gridY, int targetRow, int targetCol) { // 0x959c50
	const int dRow = gridY - targetRow;
	return gridX >= targetCol && gridX <= targetCol + 2 && dRow >= 0 && dRow <= 1;
}

// ---- B6 ----

bool UmbrellaBlocksRv(int plantRow, int plantCol, int targetRow, int targetCol) {
	if (UmbrellaRvDamage() <= 0)
		return false;
	return plantRow >= targetRow - R("boss.umbrellaRvRowsBefore", 0) && plantRow <= targetRow + R("boss.umbrellaRvRowsAfter", 0) &&
		plantCol >= targetCol - R("boss.umbrellaRvColsBefore", 0) && plantCol <= targetCol + R("boss.umbrellaRvColsAfter", 0);
}

int UmbrellaRvDamage() { return R("boss.umbrellaRvDamage", 0); }

// ---- B7 ----

BossBall PickBossBall(const std::vector<BossPacket>& bank) {
	if (!On("boss.spitPicksHarderBall"))
		return BossBall::VANILLA_ROLL;
	int jalapenos = 0, iceshrooms = 0;
	for (const BossPacket& p : bank) {
		const int t = p.type == kSeedImitater ? p.imitaterType : p.type;
		jalapenos += t == kSeedJalapeno ? 1 : 0;
		iceshrooms += t == kSeedIceshroom ? 1 : 0;
	}
	if (jalapenos < iceshrooms) // a Jalapeno breaks the ice ball: fewer of them -> the ice ball
		return BossBall::ICE;
	if (jalapenos > iceshrooms)
		return BossBall::FIRE;
	return BossBall::VANILLA_ROLL;
}

// ---- B8 ----

bool BossBungeeJackForced() { return On("boss.bungeeJackForced"); }

int BossBungeeEliteRollRange() { return BossBungeeJackForced() ? R("boss.bungeeEliteRoll", 0) : 0; }

int BossBungeeVariant(int roll) {
	return roll >= 0 && roll < R("boss.bungeeEliteBelow", 0) ? zomboss::JACK_HEAD_BUNGEE : 0;
}

int JackBungeeGrabCs(int vanilla) { return R("boss.jackBungeeGrabCs", vanilla); }

int JackBungeeJackDx() { return R("boss.jackBungeeJackDx", 0); }

// ---- B9 ----

bool EveryBossBallsBreak() { return On("boss.everyBossBalls"); }

bool BossBallBreaks(bool bossHypnotised) { return !bossHypnotised; }

bool ZombossRefusesFreeze(int bodyHp) { return On("boss.oneHpRefusesFreeze") && bodyHp == 1; }

const std::vector<std::string>& ZombossRuleKeys() {
	static const std::vector<std::string> keys = {
		"boss.hpAdventure", "boss.hpOther",
		"boss.lateSpawnRoll", "boss.lateSpawnExclude", "boss.lateSpawnExcludeRow0", "boss.lateSpawnExcludeYetiKilled",
		"boss.stompNothingHeal", "boss.crushHypnotisedDamage", "boss.crushOpensPots",
		"boss.umbrellaRvDamage", "boss.umbrellaRvRowsBefore", "boss.umbrellaRvRowsAfter", "boss.umbrellaRvColsBefore",
		"boss.umbrellaRvColsAfter",
		"boss.spitPicksHarderBall", "boss.copiesHypnosis",
		"boss.bungeeJackForced", "boss.bungeeEliteRoll", "boss.bungeeEliteBelow", "boss.jackBungeeGrabCs",
		"boss.jackBungeeJackDx",
		"boss.everyBossBalls", "boss.oneHpRefusesFreeze",
	};
	return keys;
}

} // namespace pvzu::unnamed
