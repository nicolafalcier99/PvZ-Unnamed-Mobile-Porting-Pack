#include "Mod/Unnamed/ModShots.h"

#include "Mod/Unnamed/CatapultShots.h"
#include "Mod/Unnamed/ModPlantRules.h"

namespace pvzu::unnamed {

bool IsModShotType(int theExeType) {
	return (theExeType >= 0x14 && theExeType <= 0x17) || (theExeType >= kShotDusk && theExeType <= kShotPoison) ||
	       IsCatapultShotType(theExeType); // Plan 9 R3: 0x12 vase, 0x20 bug spray
}

int ModShotCarrier(int theExeType) {
	if (theExeType >= 0 && theExeType <= 13)
		return theExeType;
	switch (theExeType) {
	case 0x14: return 4; // IMAGE_PUFFSHROOM_PUFF1 (0x7a0e1d)
	case 0x15: return 1; // IMAGE_PROJECTILESNOWPEA
	case 0x16: return 0; // drawn as IMAGE_POOLSPARKLY (0x7a0e5e); flies and hits like a pea
	case 0x17: return 7; // IMAGE_PROJECTILE_STAR
	case kShotDusk: return 2;      // hook 407 0x910428: drawn as a cabbage, lobbed like one
	case kShotKnockback: return 0; // hook 405: own image (not decoded) -> the pea
	case kShotPoison: return 0;
	case kShotVase:     // Plan 9 R3: the Catapult's vase and bug spray fly, collide and hit plants like the basketball
	case kShotBugSpray: // (hooks 390 / 391 / 392 treat 9, 0x12, 0x20 alike); drawn as IMAGE_SCARY_POT / IMAGE_BUG_SPRAY
		return 9;
	default: return -1;
	}
}

uint32_t ModShotTint(int theExeType, int theStarKind) {
	if (theExeType == 7 && theStarKind != 0)
		return StarKindColour(theStarKind);
	switch (theExeType) { // 0x959e90: every other shot is drawn with Color::White (0x72289c); the dusk shard too
	case kShotKnockback: return 0xffa040u; // stand-in when the pack lacks images/projectileprimitivepea: amber
	case kShotPoison: return 0xc060ffu;    // stand-in when the pack lacks images/ProjectilePoisonPea: violet
	default: return 0;
	}
}

std::string_view ModShotImageFile(int theExeType) {
	switch (theExeType) { // hook 405 0xc10b50 -> slots filled by the mod's loader 0xc10e50 (mod resources.xml 237-238)
	case kShotKnockback: return "images/projectileprimitivepea"; // [0xc10f4b] IMAGE_PROJECTILEPRIMITIVEPEA
	case kShotPoison: return "images/ProjectilePoisonPea";       // [0xc10f47] IMAGE_PROJECTILEPOISONPEA
	default: return {};
	}
}

bool RepeaterPoisonRecordFor(int theFromWave) {
	return theFromWave != -2 && theFromWave != -3;
}

} // namespace pvzu::unnamed
