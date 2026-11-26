// PVZU: Unnamed 1.3beta2 Dr. Zomboss with default settings (5-10), as pure rules (rolls are passed in). Evidence per
// rule: the research notes (w5-roof) section 4.1 (B1-B10) and pvz-unnamed-pack/SOURCES-zomboss.md (exe
// addresses). Every constant comes from data/rules-zomboss.json (keys "boss.*") with the vanilla value as fallback, so
// a pack without a key plays that rule like vanilla. The "Legendary Zomboss" option (study 4.2) is not ported: it is
// off by default and its spawn picker is broken in 1.3beta2.
#pragma once

#include "ConstEnums.h"

#include <functional>
#include <string>
#include <vector>

namespace pvzu::unnamed {

namespace zomboss {
constexpr int JACK_HEAD_BUNGEE = 0x17;   // Bungee variant (exe Zombie+0x134) of the boss's 1-in-4 bungees (B8)
}

// ---- B1: hit points (hook 505, .text 0x523618) ----
int BossStartHp(bool adventure, int vanilla);   // Adventure 50000, any other mode 80000

// ---- B3: the late spawn list (hook 735 0x9579b0), once the boss is 12500 cs old ----
int BossLateSpawnRollRange();                                       // n of Rand(n); 0 = the vanilla list pick
bool BossLateSpawnAccepted(int roll, int targetRow, bool killedYeti);
// Rand(n) re-rolled until accepted (21 types uniform; 19 in row 0; no Yeti once one was killed).
ZombieType PickBossLateSpawn(int targetRow, bool killedYeti, const std::function<int(int)>& rand);
bool BossCopiesHypnosis();   // spawned zombies and bungees take the boss's mMindControlled (0x957970, hook 741)

// ---- B4: an empty stomp heals (hook 737 0xc02800) ----
bool BossEmptyStompHeals();
int BossHpAfterEmptyStomp(int hp);   // + 1800, never clamped (the exe has no cap)

// ---- B5: the stomp and the RV also crush hypnotised zombies and open pots (hooks 738, 739, 731) ----
bool BossCrushesHypnotised();
int BossCrushDamage();                         // TakeDamage(1800, 1)
unsigned BossCrushDamageFlags(bool bossHypnotised); // EffectedByDamage 0x8d (only hypnotised) / 0xd for a hypnotised boss
int RectColumn(int px);                        // round(px / 80) - 1, round half to even (FPU, 0x957680)
bool StompHitsZombie(int zombieRow, int rectX, int rectWidth, int targetRow);
bool RvHitsZombie(int zombieRow, int rectX, int rectWidth, int targetRow, int targetCol);
bool BossCrushOpensPots();
bool StompOpensPot(int gridX, int gridY, int targetRow);
bool RvOpensPot(int gridX, int gridY, int targetRow, int targetCol);

// ---- B6: an Umbrella Leaf near the landing square stops the RV (hook 731 0xc02b20) ----
bool UmbrellaBlocksRv(int plantRow, int plantCol, int targetRow, int targetCol);
int UmbrellaRvDamage();

// ---- B7: the head spit picks the harder ball (hook 745 0x957a50) ----
enum class BossBall { VANILLA_ROLL, FIRE, ICE };
struct BossPacket {
	int type = -1;        // SeedType of the packet
	int imitaterType = -1; // its copy when type is the Imitater
};
BossBall PickBossBall(const std::vector<BossPacket>& bank);

// ---- B8: boss bungees (hooks 488 + 741, flag 0xc04476) ----
bool BossBungeeJackForced();          // a bungee whose elite roll passes while the boss drops it is always 0x17
int BossBungeeEliteRollRange();       // Rand(12)
int BossBungeeVariant(int roll);      // roll < 3 -> JACK_HEAD_BUNGEE, else 0
int JackBungeeGrabCs(int vanilla);    // phase counter at the bottom: 0 = grabs at once (vanilla 300)
int JackBungeeJackDx();               // the Jack lands at GridToPixelX(plant col) + this

// ---- B9: freezing ----
bool EveryBossBallsBreak();           // Ice-shroom / Jalapeno handle every boss, not only the first (hooks 340, 341)
bool BossBallBreaks(bool bossHypnotised); // ... except a hypnotised one
bool ZombossRefusesFreeze(int bodyHp);    // hook 713: a boss at 1 HP cannot be frozen

// Every rules key the boss reads (for the pack consistency test).
const std::vector<std::string>& ZombossRuleKeys();

} // namespace pvzu::unnamed
