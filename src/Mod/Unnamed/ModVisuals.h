// PVZU: Plan 9 L3, Unnamed's small visual details (pure rules; the engine side is Mod/Engine/ModVisuals.*).
//   * the right-hold info tooltip (hook 57, Board::UpdateToolTip 0x40f351 -> 0x955281; its last line 0x95a1b0 is the
//     zombie's weakened time): zombie HP / armour / statuses, plant health. Feature "modVisuals".
//   * the Peashooter "layers" number (hook 121 chain 0xc05e7c). Feature "modVisuals".
//   * the Sniper 0x78 head aim (hook 13, Attachment::SetMatrix -> 0x7a7e60 -> 0x7aa500) and the IMAGE_TARGET mark on its
//     target (hook 121 0xc10f70 + zombie draw 0xc10fd0). Feature "modPlants".
//   * the Element-shroom 0x7a element icons and countdown digit (0x7a8e21 -> 0x7b1536, 0x7b158f). Feature "modPlants".
// Evidence: the research notes (l3-visual-details). Pack keys: data/rules-mod-visuals.json (missing key = not drawn).
#pragma once

#include <array>
#include <string>

namespace pvzu::unnamed {

bool ModVisualsOn(); // feature "modVisuals"

// ---- info tooltip (hook 57) ----
// The exe: mGameScene == SCENE_PLAYING, mGameMode != 21 (Invisighoul), WidgetManager mDownButtons == 2 (right only).
// Port addition for the phone: the left button (a finger) held for modVisuals.infoHoldTicks updates.
bool InfoToolTipShown(int theGameScene, int theGameMode, int theDownButtons, int theHeldTicks);

struct ZombieInfo {
	int body = 0, bodyMax = 0;                    // +0xc8 / +0xcc
	int helmType = 0, helm = 0, helmMax = 0;      // +0xc4 / +0xd0 / +0xd4
	int shieldType = 0, shield = 0, shieldMax = 0; // +0xd8 / +0xdc / +0xe0
	int flying = 0, flyingMax = 0;                // +0xe4 / +0xe8
	int butterCs = 0, chillCs = 0, iceCs = 0;     // +0xb0 / +0xac / +0xb4
	int weakenCs = 0;                             // word +0x1a (0x95a1b0)
};
struct InfoLabels { // the exe's 一类： 二类： 气球： HP： 黄油： 减速： 冰冻： 易伤: and "cs"
	std::string helm, shield, balloon, hp, butter, chill, frozen, weakened, cs;
};
// The tooltip's "HP:" sum (0x9555ce..0x9555f2: the helm and shield only while worn) and its maximum (all four maxima).
int ZombieInfoTotal(const ZombieInfo& z);
int ZombieInfoTotalMax(const ZombieInfo& z);
// The label text of the zombie tooltip: fields joined by ' ' and '\n' alternately (two per line, 0x9558c0).
std::string ZombieInfoLabel(const ZombieInfo& z, const InfoLabels& l);
// The plant tooltip's label "生命:%d/%d" (0xc05fea).
std::string PlantInfoLabel(int theHp, int theMaxHp, const std::string& theHealthLabel);

// ---- Peashooter layers (0xc05e7c): raw seed 0 only; "%d" of the layers at plant (2, 70), magenta ----
bool PeashooterLayersDrawn(int theRawSeed);
inline constexpr int kLayersX = 2, kLayersY = 70;

// ---- Sniper (hook 13 / 121) ----
// Aiming: seed 0x78, state != 0, a target id, and the plant alive, unsquished, awake, on the board.
bool SniperAims(int theSeed, int theState, int theTargetId, bool theAliveOnBoard);
float SniperHeadAngle(int thePlantX, int thePlantY, int theZombieX, int theZombieY); // atan2(py - zy, |zx - px|)
struct AimMatrix { float m00, m01, m02, m10, m11, m12; };
AimMatrix SniperHeadMatrix(float theAngle); // rotation about (40, 40) (0x7aa500)
bool SniperMarkDrawn(); // modVisuals.sniperMark
int SniperMarkOffsetX(); // 20 (zombie-local, 0xc10fff push 20.0f)
int SniperMarkOffsetY(); // 0

// ---- Seed packets (0x4873e0) ----
// false: every packet draws its plant from the reanim (the exe jumps over the static packet_plants cels: 0x4873e0
// `jmp 0x487653`); true = vanilla (Potato Mine, Chomper, Blover... use their packet_plants.png cel at scale <= 1).
bool PacketPlantCelsUsed(); // modVisuals.packetPlantCels

// ---- Element-shroom (0x7b158f / 0x7b1536) ----
bool ElementIconsDrawn(int theSeed, int theElementCount, bool theSquished);
struct ElementIcon { bool ice; int x, y; }; // plant-local top-left of the image
std::array<ElementIcon, 3> ElementShroomIcons(int theBits, int theWidth, int theHeight);
int ElementDigitX(int theWidth); // width/2 - 6
int ElementDigitY();             // 25

} // namespace pvzu::unnamed
