/*
 * Copyright (C) 2026 Zhou Qiankang <wszqkzqk@qq.com>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * This file is part of PvZ-Portable.
 *
 * PvZ-Portable is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PvZ-Portable is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with PvZ-Portable. If not, see <https://www.gnu.org/licenses/>.
 */

#include "Mod/Hud.h" // PVZU
#include "Mod/Engine/ModModes.h" // PVZU: mod-only modes 0x65..0x94
#include "Mod/SeedSlots.h" // PVZU
#include "Mod/CardArt.h" // PVZU
#include "Mod/PanelCard.h" // PVZU
#include "Mod/Touch.h" // PVZU
#include <vector>
#include "Mod/Engine/WideScreen.h" // PVZU
#include "Mod/Engine/MobilePlanting.h" // PVZU
#include "Mod/Engine/ModPlants.h" // PVZU Plan 8: mod-only plant packets
#include "Mod/Engine/ModPlantsSpecials.h" // PVZU Plan 8 part 3b: Golden Sun-shroom packet
#include "Mod/Engine/OptionsPanel.h" // PVZU: V2 conveyor step (double sun)
#include "Mod/Engine/SpecialLevelHooks.h" // PVZU: 7-5 / mode 0x89 Starfruit packet
#include <format>
#include <array> // PVZU: card art cache
#include <map> // PVZU
#include <memory> // PVZU
#include "Board.h"
#include "Cutscene.h"
#include "Challenge.h"
#include "SeedPacket.h"
#include "../LawnApp.h"
#include "CursorObject.h"
#include "../Resources.h"
#include "MessageWidget.h"
#include "graphics/Font.h"
#include "../PvzpLib/FilterEffect.h"
#include "../PvzpLib/PvzpParticle.h" // PVZU: OverrideScale on the packet flash
#include "misc/SexyMatrix.h"
#include "graphics/MemoryImage.h" // PVZU: card art
#include "Mod/Unnamed/ModVisuals.h" // PVZU: PacketPlantCelsUsed (0x4873e0)

constexpr const int SLOT_MACHINE_TIME = 400;
constexpr const int CONVEYOR_SPEED = 4;

SeedPacket::SeedPacket()
{
	mSlotMachiningPosition = 0.0f;
	mPacketType = SeedType::SEED_NONE;
	mImitaterType = SeedType::SEED_NONE;
	mIndex = -1;
	mSlotMachiningNextSeed = SeedType::SEED_NONE;
	mWidth = 50;
	mHeight = 70;
	mRefreshCounter = 0;
	mRefreshTime = 0;
	mRefreshing = false;
	mActive = true;
	mOffsetX = 0;
	mSlotMachineCountDown = 0;
	mSlotMachiningNextSeed = SeedType::SEED_NONE;
	mTimesUsed = 0;
}

void SeedPacket::PickNextSlotMachineSeed()
{
	int aPeasCount = mBoard->CountPlantByType(SeedType::SEED_PEASHOOTER);

	SeedType SLOT_SEED_TYPES[] = {
		SeedType::SEED_SUNFLOWER,
		SeedType::SEED_PEASHOOTER,
		SeedType::SEED_SNOWPEA,
		SeedType::SEED_WALLNUT,
		SeedType::SEED_SLOT_MACHINE_SUN,
		SeedType::SEED_SLOT_MACHINE_DIAMOND
	};

	int aSeedsCount = 0;
	PvzpWeightedArray aSeedWeightArray[SeedType::NUM_SEED_TYPES];
	for (size_t i = 0; i < LENGTH(SLOT_SEED_TYPES); i++)
	{
		SeedType aSeedType = SLOT_SEED_TYPES[i];

		int aWeight = 100;
		if (aSeedType == SeedType::SEED_PEASHOOTER)
		{
			aWeight = PvzpAnimateCurve(0, 5, aPeasCount, 200, 100, PvzpCurves::CURVE_LINEAR);
		}
		else if (aSeedType == SeedType::SEED_SLOT_MACHINE_DIAMOND)
		{
			aWeight = 30;
		}

		if (mIndex == 2 && aSeedType != SeedType::SEED_SLOT_MACHINE_DIAMOND)
		{
			if (aSeedType == mBoard->mSeedBank->mSeedPackets[0].mSlotMachiningNextSeed || aSeedType == mBoard->mSeedBank->mSeedPackets[1].mSlotMachiningNextSeed)
			{
				aWeight += aWeight / 2;
			}
		}

		aSeedWeightArray[aSeedsCount].mItem = static_cast<int>(aSeedType);
		aSeedWeightArray[aSeedsCount].mWeight = aWeight;
		aSeedsCount++;
	}

	mSlotMachiningNextSeed = static_cast<SeedType>(PvzpPickFromWeightedArray(aSeedWeightArray, aSeedsCount));
}

void SeedPacket::SlotMachineStart()
{
	mSlotMachineCountDown = 300;
	mSlotMachiningPosition = 0.0f;
	PickNextSlotMachineSeed();
}

// PVZU: when each HUD card's recharge-done flash began (Board::mMainCounter); not saved (a 0.1 s effect).
static std::map<const SeedPacket*, int>& CardFlashStarts()
{
	static auto& sStarts = *new std::map<const SeedPacket*, int>;
	return sStarts;
}

void SeedPacket::FlashIfReady()
{
	if (!CanPickUp() || mApp->mEasyPlantingCheat)
		return;

	if (!mBoard->HasConveyorBeltSeedBank())
	{
		int aRenderPosition = Board::MakeRenderOrder(RenderLayer::RENDER_LAYER_UI_BOTTOM, 0, 2);
		// PVZU: a scaled HUD packet gets a flash of its own size, centred on it; a landscape card gets it at the
		// card's height, centred on the card (vanilla size: unchanged).
		if (mWidth != SEED_PACKET_WIDTH || mHeight != SEED_PACKET_HEIGHT)
		{
			// PVZU: a HUD card flashes as a whole, in its own shape (Draw); the packet-sized particle showed as a
			// narrow strip inside the wide card (2026-11-23).
			CardFlashStarts()[this] = mBoard->mMainCounter;
		}
		else
		{
			PvzpParticleSystem* aFlash = mApp->AddPvzpParticle(mX + mBoard->mSeedBank->mX, mY + mBoard->mSeedBank->mY, aRenderPosition, ParticleEffect::PARTICLE_SEED_PACKET_FLASH);
			(void)aFlash;
		}
	}

	if (mBoard->mTutorialState == TutorialState::TUTORIAL_LEVEL_1_REFRESH_PEASHOOTER)
	{
		mBoard->SetTutorialState(TutorialState::TUTORIAL_LEVEL_1_PICK_UP_PEASHOOTER);
	}
	else if (mBoard->mTutorialState == TutorialState::TUTORIAL_LEVEL_2_REFRESH_SUNFLOWER && mPacketType == SeedType::SEED_SUNFLOWER)
	{
		mBoard->SetTutorialState(TutorialState::TUTORIAL_LEVEL_2_PICK_UP_SUNFLOWER);
	}
	else if (mBoard->mTutorialState == TutorialState::TUTORIAL_MORESUN_REFRESH_SUNFLOWER && mPacketType == SeedType::SEED_SUNFLOWER)
	{
		mBoard->SetTutorialState(TutorialState::TUTORIAL_MORESUN_PICK_UP_SUNFLOWER);
	}
}

void SeedPacket::Activate()
{
	PVZP_ASSERT(mPacketType != SeedType::SEED_NONE);
	mActive = true;
}

void SeedPacket::Deactivate()
{
	mActive = false;
	mRefreshCounter = 0;
	mRefreshTime = 0;
	mRefreshing = false;
}

void SeedPacket::SetActivate(bool theActive)
{
	if (theActive)
	{
		Activate();
	}
	else
	{
		Deactivate();
	}
}

void SeedPacket::Update()
{
	if (mApp->mGameScene != GameScenes::SCENE_PLAYING || mPacketType == SeedType::SEED_NONE)
	{
		return;
	}

	if (mBoard->mMainCounter == 0)
	{
		FlashIfReady();
	}

	if (!mActive && mRefreshing)
	{
		mRefreshCounter++;
		if (mRefreshCounter > mRefreshTime)
		{
			mRefreshCounter = 0;
			mRefreshing = false;
			Activate();
			FlashIfReady();
		}
	}

	if (mSlotMachineCountDown > 0)
	{
		mSlotMachineCountDown--;
		float aFlipsPerSecont = PvzpAnimateCurveFloat(SLOT_MACHINE_TIME, 0, mSlotMachineCountDown, 6.0f, 2.0f, PvzpCurves::CURVE_LINEAR);
		mSlotMachiningPosition += aFlipsPerSecont * 0.01f;

		if (mSlotMachiningPosition >= 1.0f)
		{
			mPacketType = mSlotMachiningNextSeed;
			if (mSlotMachineCountDown == 0)
			{
				Activate();
				mSlotMachiningPosition = 0.0f;
			}
			else
			{
				mSlotMachiningPosition -= 1.0f;
				PickNextSlotMachineSeed();
			}
		}
		else if (mSlotMachineCountDown == 0)
		{
			mSlotMachineCountDown = 1;
		}
	}
}

void SeedPacketDrawSeed(Graphics* g, float x, float y, SeedType theSeedType, SeedType theImitaterType, float theOffsetX, float theOffsetY, float theScale)
{
	if (pvzu::engine::ModAddPlantBase(theSeedType) != theSeedType) // PVZU: a mod seed: its base plant in the table's tint
	{
		Graphics aTintG(*g);
		pvzu::engine::ApplyModTint(&aTintG, theSeedType);
		SeedPacketDrawSeed(&aTintG, x, y, pvzu::engine::ModAddPlantBase(theSeedType), SeedType::SEED_NONE, theOffsetX, theOffsetY, theScale);
		return;
	}
	Image* aImage = IMAGE_PACKET_PLANTS;
	SeedType aSeedType = theSeedType;
	if (theSeedType == SeedType::SEED_IMITATER && theImitaterType != SeedType::SEED_NONE)
	{
		aSeedType = theImitaterType;
		FilterEffect aFilterEffect = FilterEffect::FILTER_EFFECT_WASHED_OUT;
		if (theImitaterType == SeedType::SEED_HYPNOSHROOM || theImitaterType == SeedType::SEED_SQUASH || theImitaterType == SeedType::SEED_POTATOMINE ||
			theImitaterType == SeedType::SEED_GARLIC || theImitaterType == SeedType::SEED_LILYPAD)
			aFilterEffect = FilterEffect::FILTER_EFFECT_LESS_WASHED_OUT;
		aImage = FilterEffectGetImage(aImage, aFilterEffect);
	}

	if (!pvzu::unnamed::PacketPlantCelsUsed()) // PVZU: Unnamed draws every packet's plant from its reanim (0x4873e0 jmp 0x487653)
	{
		Graphics aSeedG(*g);
		aSeedG.mScaleX = theScale * g->mScaleX;
		aSeedG.mScaleY = theScale * g->mScaleY;
		Plant::DrawSeedType(&aSeedG, theSeedType, theImitaterType, DrawVariation::VARIATION_NORMAL, x + theOffsetX, y + theOffsetY);
	}
	else if (aSeedType == SeedType::SEED_POTATOMINE && g->mScaleX <= 1.0f)
	{
		PvzpDrawImageCelScaledF(g, aImage, x, y, 0, 0, g->mScaleX, g->mScaleY);
	}
	else if (aSeedType == SeedType::SEED_CHOMPER && g->mScaleX <= 1.0f)
	{
		PvzpDrawImageCelScaledF(g, aImage, x, y, 1, 0, g->mScaleX, g->mScaleY);
	}
	else if (aSeedType == SeedType::SEED_HYPNOSHROOM && g->mScaleX <= 1.0f)
	{
		PvzpDrawImageCelScaledF(g, aImage, x, y, 2, 0, g->mScaleX, g->mScaleY);
	}
	else if (aSeedType == SeedType::SEED_TALLNUT && g->mScaleX <= 1.0f)
	{
		PvzpDrawImageCelScaledF(g, aImage, x, y, 3, 0, g->mScaleX, g->mScaleY);
	}
	else if (aSeedType == SeedType::SEED_BLOVER && g->mScaleX <= 1.0f)
	{
		PvzpDrawImageCelScaledF(g, aImage, x, y, 4, 0, g->mScaleX, g->mScaleY);
	}
	else if (aSeedType == SeedType::SEED_PUMPKINSHELL && g->mScaleX <= 1.0f)
	{
		PvzpDrawImageCelScaledF(g, aImage, x, y, 5, 0, g->mScaleX, g->mScaleY);
	}
	else if (aSeedType == SeedType::SEED_TWINSUNFLOWER && g->mScaleX <= 1.0f)
	{
		PvzpDrawImageCelScaledF(g, aImage, x, y, 6, 0, g->mScaleX, g->mScaleY);
	}
	else if (aSeedType == SeedType::SEED_COBCANNON && g->mScaleX <= 1.0f)
	{
		PvzpDrawImageCelScaledF(g, aImage, x, y, 7, 0, g->mScaleX, g->mScaleY);
	}
	else if (aSeedType == SeedType::SEED_CABBAGEPULT && g->mScaleX <= 1.0f)
	{
		PvzpDrawImageCelScaledF(g, aImage, x, y, 8, 0, g->mScaleX, g->mScaleY);
	}
	else if (aSeedType == SeedType::SEED_KERNELPULT && g->mScaleX <= 1.0f)
	{
		PvzpDrawImageCelScaledF(g, aImage, x, y, 9, 0, g->mScaleX, g->mScaleY);
	}
	else if (aSeedType == SeedType::SEED_MELONPULT && g->mScaleX <= 1.0f)
	{
		PvzpDrawImageCelScaledF(g, aImage, x, y, 10, 0, g->mScaleX, g->mScaleY);
	}
	else if (aSeedType == SeedType::SEED_WINTERMELON && g->mScaleX <= 1.0f)
	{
		PvzpDrawImageCelScaledF(g, aImage, x, y, 11, 0, g->mScaleX, g->mScaleY);
	}
	else if (aSeedType == SeedType::SEED_SPIKEROCK && g->mScaleX <= 1.0f)
	{
		PvzpDrawImageCelScaledF(g, aImage, x, y, 12, 0, g->mScaleX, g->mScaleY);
	}
	else
	{
		Graphics aSeedG(*g);
		aSeedG.mScaleX = theScale * g->mScaleX;
		aSeedG.mScaleY = theScale * g->mScaleY;
		Plant::DrawSeedType(&aSeedG, theSeedType, theImitaterType, DrawVariation::VARIATION_NORMAL, x + theOffsetX, y + theOffsetY);
	}
}

// PVZU: pieces of DrawSeedPacket shared with the seed panel card (vanilla behaviour unchanged).
namespace {
struct PacketPlantPlacement
{
	float mScale;
	float mOffsetX;
	float mOffsetY;
	bool mInMiddle;
};

PacketPlantPlacement GetPacketPlantPlacement(SeedType aSeedType)
{
	float aScale = 0.5f;
	bool aDrawSeedInMiddle = true;
	float aOffsetX = 5.0f;
	float aOffsetY = 8.0f;
	switch (aSeedType)
	{
	case SeedType::SEED_TALLNUT:
		aScale = 0.3f;
		aOffsetX = 12.0f;
		aOffsetY = 22.0f;
		break;

	case SeedType::SEED_INSTANT_COFFEE:
		aScale = 0.55f;
		aOffsetX = 0;
		aOffsetY = 9.0f;
		break;

	case SeedType::SEED_COBCANNON:
		aScale = 0.26f;
		aOffsetX = 6.0f;
		aOffsetY = 22.0f;
		break;

	case SeedType::SEED_CACTUS:
		aOffsetX = 9.0f;
		aOffsetY = 13.0f;
		break;

	case SeedType::SEED_POTATOMINE:
		aScale = 0.4f;
		aOffsetX = 8.0f;
		aOffsetY = 12.0f;
		break;

	case SeedType::SEED_MAGNETSHROOM:
		aOffsetY = 12.0f;
		break;

	case SeedType::SEED_FUMESHROOM:
	case SeedType::SEED_PUMPKINSHELL:
	case SeedType::SEED_CHOMPER:
	case SeedType::SEED_DOOMSHROOM:
	case SeedType::SEED_SQUASH:
	case SeedType::SEED_HYPNOSHROOM:
	case SeedType::SEED_SPIKEWEED:
	case SeedType::SEED_SPIKEROCK:
	case SeedType::SEED_PLANTERN:
	case SeedType::SEED_TORCHWOOD:
	case SeedType::SEED_TANGLEKELP:
		aScale = 0.4f;
		aOffsetX = 8.0f;
		aOffsetY = 12.0f;
		break;

	case SeedType::SEED_TWINSUNFLOWER:
	case SeedType::SEED_GLOOMSHROOM:
		aScale = 0.45f;
		aOffsetX = 7.0f;
		aOffsetY = 14.0f;
		break;

	case SeedType::SEED_CATTAIL:
		aScale = 0.45f;
		aOffsetX = 8.0f;
		aOffsetY = 13.0f;
		break;

	case SeedType::SEED_UMBRELLA:
		aScale = 0.5f;
		aOffsetX = 5.0f;
		aOffsetY = 10.0f;
		break;

	case SeedType::SEED_KERNELPULT:
		aScale = 0.4f;
		aOffsetX = 13.0f;
		aOffsetY = 14.0f;
		break;

	case SeedType::SEED_CABBAGEPULT:
		aScale = 0.4f;
		aOffsetX = 15.0f;
		aOffsetY = 14.0f;
		break;

	case SeedType::SEED_MELONPULT:
	case SeedType::SEED_WINTERMELON:
		aScale = 0.35f;
		aOffsetX = 18.0f;
		aOffsetY = 19.0f;
		break;

	case SeedType::SEED_GRAVEBUSTER:
		aScale = 0.4f;
		aOffsetX = 10.0f;
		aOffsetY = 15.0f;
		break;

	case SeedType::SEED_SPLITPEA:
		aScale = 0.45f;
		aOffsetX = 12.0f;
		aOffsetY = 12.0f;
		break;

	case SeedType::SEED_BLOVER:
		aScale = 0.4f;
		aOffsetX = 8.0f;
		aOffsetY = 17.0f;
		break;

	case SeedType::SEED_STARFRUIT:
		aScale = 0.5f;
		aOffsetX = 6.0f;
		aOffsetY = 8.0f;
		break;

	case SeedType::SEED_THREEPEATER:
		aScale = 0.5f;
		aOffsetX = 5.0f;
		aOffsetY = 10.0f;
		break;

	case SeedType::SEED_GATLINGPEA:
		aScale = 0.5f;
		aOffsetX = 2.0f;
		aOffsetY = 8.0f;
		break;

	case SeedType::SEED_ZOMBIE_NORMAL:
	case SeedType::SEED_ZOMBIE_TRAFFIC_CONE:
	case SeedType::SEED_ZOMBIE_PAIL:
		aScale = 0.35f;
		aOffsetX = -3.0f;
		aOffsetY = -7.0f;
		break;

	case SeedType::SEED_ZOMBIE_DANCER:
		aScale = 0.375f;
		aOffsetX = -19.0f;
		aOffsetY = -40.0f;
		break;

	case SeedType::SEED_ZOMBIE_POLEVAULTER:
		aScale = 0.35f;
		aOffsetX = -8.0f;
		aOffsetY = -12.0f;
		break;

	case SeedType::SEED_ZOMBIE_LADDER:
	case SeedType::SEED_ZOMBIE_DIGGER:
	case SeedType::SEED_ZOMBIE_SCREEN_DOOR:
	case SeedType::SEED_ZOMBIE_POGO:
		aScale = 0.35f;
		aOffsetX = -3.0f;
		aOffsetY = -10.0f;
		break;

	case SeedType::SEED_ZOMBIE_BUNGEE:
		aScale = 0.3f;
		aOffsetX = 1.0f;
		aOffsetY = -1.0f;
		break;

	case SeedType::SEED_ZOMBIE_FOOTBALL:
		aScale = 0.33f;
		aOffsetX = -7.0f;
		aOffsetY = -9.0f;
		break;

	case SeedType::SEED_ZOMBIE_BALLOON:
		aScale = 0.35f;
		aOffsetX = -3.0f;
		aOffsetY = -5.0f;
		break;

	case SeedType::SEED_ZOMBIE_IMP:
		aScale = 0.4f;
		aOffsetX = -12.0f;
		aOffsetY = -17.0f;
		break;

	case SeedType::SEED_ZOMBONI:
		aScale = 0.23f;
		aOffsetX = 12.0f;
		aOffsetY = 3.0f;
		break;

	case SeedType::SEED_ZOMBIE_GARGANTUAR:
		aScale = 0.23f;
		aOffsetX = 4.0f;
		aOffsetY = 3.0f;
		break;

	case SeedType::SEED_BEGHOULED_BUTTON_SHUFFLE:
	case SeedType::SEED_BEGHOULED_BUTTON_CRATER:
	case SeedType::SEED_SLOT_MACHINE_SUN:
	case SeedType::SEED_SLOT_MACHINE_DIAMOND:
	case SeedType::SEED_ZOMBIQUARIUM_SNORKLE:
	case SeedType::SEED_ZOMBIQUARIUM_TROPHY:
		aDrawSeedInMiddle = false;
		break;
	default:
		break;
	}
	if (((LawnApp*)gSexyAppBase)->mGameMode == GameMode::GAMEMODE_CHALLENGE_BIG_TIME)
	{
		if (aSeedType == SeedType::SEED_WALLNUT || aSeedType == SeedType::SEED_SUNFLOWER || aSeedType == SeedType::SEED_MARIGOLD)
		{
			aOffsetX = 16.0f;
			aOffsetY = 34.0f;
		}
	}
	if (aSeedType == SeedType::SEED_GIANT_WALLNUT)
	{
		aScale *= 0.75f;
		aOffsetX = 52.0f;
		aOffsetY = 58.0f;
	}
	return {aScale, aOffsetX, aOffsetY, aDrawSeedInMiddle};
}

int PacketBackgroundCel(SeedType theSeedType, SeedType aSeedType)
{
	return
		theSeedType == SeedType::SEED_IMITATER ? 0 :
		Plant::IsUpgrade(aSeedType) ? 1 :
		theSeedType == SeedType::SEED_BEGHOULED_BUTTON_CRATER ? 3 :
		theSeedType == SeedType::SEED_BEGHOULED_BUTTON_SHUFFLE ? 4 :
		theSeedType == SeedType::SEED_SLOT_MACHINE_SUN ? 5 :
		theSeedType == SeedType::SEED_SLOT_MACHINE_DIAMOND ? 6 :
		theSeedType == SeedType::SEED_ZOMBIQUARIUM_SNORKLE ? 7 :
		theSeedType == SeedType::SEED_ZOMBIQUARIUM_TROPHY ? 8 : 2;
}

std::string PacketCostString(SeedType theSeedType, SeedType theImitaterType, SeedType aSeedType, bool theUseCurrentCost)
{
	std::string aCostStr;
	if (gLawnApp->mBoard && gLawnApp->mBoard->PlantUsesAcceleratedPricing(aSeedType))
	{
		if (theUseCurrentCost)
		{
			aCostStr = std::to_string(gLawnApp->mBoard->GetCurrentPlantCost(theSeedType, theImitaterType));
		}
		else
		{
			aCostStr = std::format("{}+", Plant::GetCost(theSeedType, theImitaterType));
		}
	}
	else
	{
		aCostStr = std::to_string(Plant::GetCost(theSeedType, theImitaterType));
	}

	return aCostStr;
}
}

void DrawSeedPacket(Graphics* g, float x, float y, SeedType theSeedType, SeedType theImitaterType, float thePercentDark, int theGrayness, bool theDrawCost, bool theUseCurrentCost)
{
	SeedType aSeedType = theSeedType;
	if (aSeedType == SeedType::SEED_IMITATER && theImitaterType != SeedType::SEED_NONE)
	{
		aSeedType = theImitaterType;
	}
	aSeedType = pvzu::engine::ModAddPlantBase(aSeedType); // PVZU: a mod seed sits in the packet like its base plant
	Image* aPvzuModBack = pvzu::engine::ModPacketImage(theSeedType); // PVZU: mod packets use cell 12 (hook 465)

	if (theGrayness != 255)
	{
		g->SetColor(Color(theGrayness, theGrayness, theGrayness));
		g->SetColorizeImages(true);
	}
	else if (thePercentDark > 0)
	{
		g->SetColor(Color(128, 128, 128, 255));
		g->SetColorizeImages(true);
	}

	int aPacketBackground = PacketBackgroundCel(theSeedType, aSeedType); // PVZU: shared with the panel card

	if (g->mScaleX > 1 && !aPvzuModBack) // PVZU: a mod packet keeps its own back when enlarged (hold-for-info panel, mod Almanac grid)
	{
		PvzpDrawImageCelScaledF(g, Sexy::IMAGE_SEEDPACKET_LARGER, x, y, 0, 0, g->mScaleX * 0.5f, g->mScaleY * 0.5f);
	}
	else
	{
		if (aPvzuModBack) // PVZU
			PvzpDrawImageScaledF(g, aPvzuModBack, x, y, g->mScaleX, g->mScaleY);
		else
			PvzpDrawImageCelScaledF(g, Sexy::IMAGE_SEEDS, x, y, aPacketBackground, 0, g->mScaleX, g->mScaleY);
	}

	const PacketPlantPlacement aPlace = GetPacketPlantPlacement(aSeedType); // PVZU: shared with the panel card
	const float aScale = aPlace.mScale;
	const bool aDrawSeedInMiddle = aPlace.mInMiddle;
	float aOffsetX = aPlace.mOffsetX;
	float aOffsetY = aPlace.mOffsetY;
	aOffsetX = g->mScaleX * aOffsetX;
	aOffsetY = g->mScaleY * (aOffsetY + 1.0f);
	if (aDrawSeedInMiddle)
	{
		SeedPacketDrawSeed(g, x, y, theSeedType, theImitaterType, aOffsetX, aOffsetY, aScale);
	}

	if (thePercentDark > 0.0f)
	{
		int aDarknessHeight = FloatRoundToInt(68.0f * thePercentDark) + 2;
		Graphics aPlantG(*g);
		aPlantG.SetColor(Color(64, 64, 64, 255));
		aPlantG.SetColorizeImages(true);
		aPlantG.ClipRect(x, y, FloatRoundToInt(SEED_PACKET_WIDTH * g->mScaleX), FloatRoundToInt(aDarknessHeight * g->mScaleY)); // PVZU: scaled packets darken their own size
		if (aPvzuModBack) // PVZU
			PvzpDrawImageScaledF(&aPlantG, aPvzuModBack, x, y, aPlantG.mScaleX, aPlantG.mScaleY);
		else
			PvzpDrawImageCelScaledF(&aPlantG, Sexy::IMAGE_SEEDS, x, y, aPacketBackground, 0, aPlantG.mScaleX, aPlantG.mScaleY);
		if (aDrawSeedInMiddle)
		{
			SeedPacketDrawSeed(&aPlantG, x, y, theSeedType, theImitaterType, aOffsetX, aOffsetY, aScale);
		}
	}

	if (theDrawCost)
	{
		const std::string aCostStr = PacketCostString(theSeedType, theImitaterType, aSeedType, theUseCurrentCost); // PVZU: shared

		_Font* aTextFont = Sexy::FONT_PICO129;
		int aTextOffsetX = 32 - aTextFont->StringWidth(aCostStr);
		int aTextOffsetY = aTextFont->GetAscent() + 54;
		if (g->mScaleX == 1.0f && g->mScaleY == 1.0f)
		{
			PvzpDrawString(g, aCostStr, x + aTextOffsetX, y + aTextOffsetY, aTextFont, Color::Black, DS_ALIGN_LEFT);
		}
		else
		{
			SexyMatrix3 aMatrix;
			// PVZU: matrix drawing ignores the Graphics translation (as PvzpDrawImageCelScaledF adds it): add it here,
			// or a scaled packet's cost lands near the screen origin (panel column; award screen when not at x 0).
			PvzpScaleTransformMatrix(aMatrix, aTextOffsetX * g->mScaleX + x + g->mTransX, aTextOffsetY * g->mScaleY + y + g->mTransY, g->mScaleX, g->mScaleY);
			if (g->mScaleX > 1.8f)
			{
				g->SetLinearBlend(false);
			}
			PvzpDrawStringMatrix(g, aTextFont, aMatrix, aCostStr, Color::Black);
			g->SetLinearBlend(true);
		}
	}

	g->SetColorizeImages(false);
}

// PVZU: the HUD's landscape card, after the EA mobile card (2026-11-23; Mod/PanelCard.h): a procedural
// light frame (Mod/CardArt, cached per size), the packet's plant drawn large with a soft shadow, rising over the top
// and left rim and cut at the panel's bottom, then the white cost tab and dark digits (the vanilla packet's own
// font, larger). Greying, recharge darkening (from the top, as vanilla) and the held look follow DrawSeedPacket.
namespace {
constexpr int kCardArtScale = 3; // device px per logical px the card art is rendered at (Mac 2x, S26 Ultra 2.4x)

std::unique_ptr<MemoryImage> CardArtImage(const pvzu::ArgbImage& theArt)
{
	auto aImage = std::make_unique<MemoryImage>();
	const size_t aCount = theArt.px.size();
	aImage->mBits = std::make_unique<uint32_t[]>(aCount + 1);
	aImage->mWidth = theArt.w;
	aImage->mHeight = theArt.h;
	aImage->mHasTrans = true;
	aImage->mHasAlpha = true;
	std::copy(theArt.px.begin(), theArt.px.end(), aImage->mBits.get());
	aImage->mBits[aCount] = Sexy::MEMORYCHECK_ID;
	return aImage;
}

struct CardArt
{
	std::unique_ptr<MemoryImage> mBody;
	std::unique_ptr<MemoryImage> mTab;
};

// The frame and tab art for one layout (a level uses one or two: every card has the same size, tabs differ by
// the cost's width only when it has 4+ digits).
const CardArt& GetCardArt(const pvzu::PanelCardParts& theParts)
{
	// Never destroyed: a MemoryImage unregisters from the app when deleted, and the app is gone at static exit.
	static auto& sCache = *new std::map<std::array<int, 6>, CardArt>;
	const std::array<int, 6> aKey{theParts.frame.w, theParts.frame.h, theParts.frame.y, theParts.tab.w, theParts.tab.h, theParts.panel.h};
	auto aIt = sCache.find(aKey);
	if (aIt == sCache.end())
	{
		CardArt aArt;
		aArt.mBody = CardArtImage(pvzu::RenderCardBody(theParts, kCardArtScale));
		if (theParts.tab.w > 0)
			aArt.mTab = CardArtImage(pvzu::RenderCardTab(theParts, kCardArtScale));
		aIt = sCache.emplace(aKey, std::move(aArt)).first;
	}
	return aIt->second;
}

// Where the plant's pixels are in a vanilla packet (packet px), measured once per seed type by drawing it as
// DrawSeedPacket does into a scratch image (2x, with room around it: some plants reach out of the packet).
pvzu::PlantBounds PacketPlantBounds(SeedType theSeedType, SeedType theImitaterType, const PacketPlantPlacement& thePlace)
{
	static std::map<std::pair<int, int>, pvzu::PlantBounds> sCache;
	const std::pair<int, int> aKey{static_cast<int>(theSeedType), static_cast<int>(theImitaterType)};
	if (auto aIt = sCache.find(aKey); aIt != sCache.end())
		return aIt->second;

	constexpr int kScale = 2, kMargin = 40;
	const int aW = (SEED_PACKET_WIDTH + 2 * kMargin) * kScale, aH = (SEED_PACKET_HEIGHT + 2 * kMargin) * kScale;
	MemoryImage aImage;
	aImage.mBits = std::make_unique<uint32_t[]>(static_cast<size_t>(aW) * aH + 1);
	aImage.mWidth = aW;
	aImage.mHeight = aH;
	aImage.mHasTrans = true;
	aImage.mHasAlpha = true;
	std::fill_n(aImage.mBits.get(), static_cast<size_t>(aW) * aH, 0u);
	aImage.mBits[static_cast<size_t>(aW) * aH] = Sexy::MEMORYCHECK_ID;
	{
		Graphics aG(&aImage);
		aG.SetLinearBlend(true);
		aG.mScaleX = kScale;
		aG.mScaleY = kScale;
		SeedPacketDrawSeed(&aG, kMargin * kScale, kMargin * kScale, theSeedType, theImitaterType,
			kScale * thePlace.mOffsetX, kScale * (thePlace.mOffsetY + 1.0f), thePlace.mScale);
	}
	const uint32_t* aBits = aImage.GetBits();
	int x0 = aW, y0 = aH, x1 = -1, y1 = -1;
	for (int y = 0; y < aH; y++)
		for (int x = 0; x < aW; x++)
			if ((aBits[static_cast<size_t>(y) * aW + x] >> 24) > 24)
			{
				x0 = std::min(x0, x); x1 = std::max(x1, x);
				y0 = std::min(y0, y); y1 = std::max(y1, y);
			}
	pvzu::PlantBounds aBounds{static_cast<float>(pvzu::kPacketWindowSrc.x), static_cast<float>(pvzu::kPacketWindowSrc.y),
		static_cast<float>(pvzu::kPacketWindowSrc.x + pvzu::kPacketWindowSrc.w), static_cast<float>(pvzu::kPacketWindowSrc.y + pvzu::kPacketWindowSrc.h)};
	if (x1 > x0 && y1 > y0)
		aBounds = {static_cast<float>(x0) / kScale - kMargin, static_cast<float>(y0) / kScale - kMargin,
			static_cast<float>(x1 + 1) / kScale - kMargin, static_cast<float>(y1 + 1) / kScale - kMargin};
	sCache.emplace(aKey, aBounds);
	return aBounds;
}
} // namespace

static void DrawPanelCard(Graphics* g, float x, float y, int theWidth, int theHeight, SeedType theSeedType, SeedType theImitaterType, float thePercentDark, int theGrayness, bool theDrawCost)
{
	SeedType aSeedType = theSeedType;
	if (aSeedType == SeedType::SEED_IMITATER && theImitaterType != SeedType::SEED_NONE)
	{
		aSeedType = theImitaterType;
	}
	aSeedType = pvzu::engine::ModAddPlantBase(aSeedType); // PVZU: a mod seed sits on the card like its base plant
	const std::string aCostStr = theDrawCost ? PacketCostString(theSeedType, theImitaterType, aSeedType, true) : std::string();
	_Font* aCostFont = Sexy::FONT_PICO129; // the vanilla packet's cost font: thin, monospaced, like the reference's
	const int aCostH = aCostFont->GetAscent() - 3; // FONT_PICO129: ascent 10, its digits are 7 px tall on the baseline
	pvzu::PanelCardParts aParts = pvzu::PanelCardLayout(theWidth, theHeight, aCostStr.empty() ? 0 : aCostFont->StringWidth(aCostStr), aCostH);
	const CardArt& aArt = GetCardArt(aParts);
	const PacketPlantPlacement aPlace = GetPacketPlantPlacement(aSeedType);
	if (aPlace.mInMiddle)
		pvzu::FitPanelPlant(aParts, PacketPlantBounds(theSeedType, theImitaterType, aPlace));
	const int aX = FloatRoundToInt(x), aY = FloatRoundToInt(y);
	auto aDrawArt = [&](Graphics* theG, MemoryImage* theImage, const pvzu::Box& theDst)
	{
		if (theImage != nullptr && theDst.w > 0 && theDst.h > 0)
			theG->DrawImage(theImage, Rect(aX + theDst.x, aY + theDst.y, theDst.w, theDst.h), Rect(0, 0, theImage->mWidth, theImage->mHeight));
	};
	auto aDrawPlant = [&](Graphics* theG, float theDx, float theDy)
	{
		Graphics aPlantG(*theG);
		const pvzu::Box& c = aParts.plantClip;
		aPlantG.ClipRect(aX + c.x, aY + c.y, c.w, c.h);
		aPlantG.mScaleX = aParts.plantScale;
		aPlantG.mScaleY = aParts.plantScale;
		SeedPacketDrawSeed(&aPlantG, x + aParts.plantX + theDx, y + aParts.plantY + theDy, theSeedType, theImitaterType,
			aPlantG.mScaleX * aPlace.mOffsetX, aPlantG.mScaleY * (aPlace.mOffsetY + 1.0f), aPlace.mScale);
	};
	auto aDrawBody = [&](Graphics* theG)
	{
		aDrawArt(theG, aArt.mBody.get(), aParts.art);
		if (aPlace.mInMiddle)
		{
			// The plant's soft shadow: its silhouette, dark and see-through, a little down-right.
			Graphics aShadowG(*theG);
			aShadowG.SetColorizeImages(true);
			aShadowG.SetColor(Color(0, 0, 0, 60));
			aDrawPlant(&aShadowG, aParts.plantShadowDx, aParts.plantShadowDy);
			aDrawPlant(theG, 0.0f, 0.0f);
		}
		aDrawArt(theG, aArt.mTab.get(), aParts.tab);
	};

	if (theGrayness != 255)
	{
		g->SetColor(Color(theGrayness, theGrayness, theGrayness));
		g->SetColorizeImages(true);
	}
	else if (thePercentDark > 0)
	{
		g->SetColor(Color(128, 128, 128, 255));
		g->SetColorizeImages(true);
	}
	aDrawBody(g);

	if (thePercentDark > 0.0f)
	{
		// Vanilla darkens (68 * percent + 2) of the packet's 70 px from the top; the same share of the frame, and
		// everything above it (the plant rising over the rim).
		const int aTop = std::min(aParts.plantClip.y, aParts.art.y);
		const int aBottom = thePercentDark >= 1.0f ? aParts.art.y + aParts.art.h :
			aParts.frame.y + FloatRoundToInt(aParts.frame.h * (68.0f * thePercentDark + 2.0f) / SEED_PACKET_HEIGHT);
		Graphics aDarkG(*g);
		aDarkG.SetColor(Color(64, 64, 64, 255));
		aDarkG.SetColorizeImages(true);
		aDarkG.ClipRect(aX + aParts.plantClip.x, aY + aTop, aParts.art.x + aParts.art.w - aParts.plantClip.x, aBottom - aTop);
		aDrawBody(&aDarkG);
	}
	g->SetColorizeImages(false);

	// Cost: dark digits in the tab, drawn twice half a px apart (the reference's strokes are heavier than the pixel
	// font's at this size).
	if (aParts.tab.w > 0)
	{
		for (const float aNudge : {0.0f, 0.5f * aParts.k})
		{
			SexyMatrix3 aMatrix;
			PvzpScaleTransformMatrix(aMatrix, x + aParts.text.x - aNudge + g->mTransX, y + aParts.text.y + aParts.text.h + g->mTransY, aParts.textScale, aParts.textScale);
			PvzpDrawStringMatrix(g, aCostFont, aMatrix, aCostStr, Color(28, 30, 30));
		}
	}
}

void SeedPacket::Draw(Graphics* g)
{
	float aPercentDark = 0.0f;
	if (!mActive)
	{
		if (mRefreshTime == 0)
		{
			aPercentDark = 1.0f;
		}
		else
		{
			aPercentDark = static_cast<float>(mRefreshTime - mRefreshCounter) / static_cast<float>(mRefreshTime);
		}
	}

	if (mSlotMachineCountDown > 0)
	{
		int aOffsetY = FloatRoundToInt(-mHeight * mSlotMachiningPosition);

		Graphics aClipG(*g);
		aClipG.ClipRect(0, 0, mWidth, mHeight);

		DrawSeedPacket(&aClipG, 0.0f, aOffsetY, mPacketType, SeedType::SEED_NONE, 0.0f, 128, false, false);
		DrawSeedPacket(&aClipG, 0.0f, mHeight + aOffsetY, mSlotMachiningNextSeed, SeedType::SEED_NONE, 0.0f, 128, false, false);
	}
	else
	{
		SeedType aUseSeedType = mPacketType;
		if (mPacketType == SeedType::SEED_IMITATER && mImitaterType != SeedType::SEED_NONE)
		{
			aUseSeedType = mImitaterType;
		}

		bool aDrawCost = true;
		if (mBoard->HasConveyorBeltSeedBank() || mApp->IsSlotMachineLevel())
		{
			aDrawCost = false;
		}
		int aCost = mBoard->GetCurrentPlantCost(mPacketType, mImitaterType);

		int aGrayness = 255;
		if ((mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_BEGHOULED && !mActive) ||
			(mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_BEGHOULED_TWIST && !mActive))
		{
			aGrayness = 64;
		}
		else if (mApp->mGameScene != GameScenes::SCENE_PLAYING)
		{
			aGrayness = mBoard->mSeedBank->mCutSceneDarken;
			aPercentDark = 0.0f;
		}
		else if (mBoard->mTutorialState == TutorialState::TUTORIAL_LEVEL_1_PICK_UP_PEASHOOTER && mBoard->mTutorialTimer == -1 && mPacketType == SeedType::SEED_PEASHOOTER)
		{
			aGrayness = GetFlashingColor(mBoard->mMainCounter, 75).mRed;
		}
		else if (mBoard->mTutorialState == TutorialState::TUTORIAL_LEVEL_2_PICK_UP_SUNFLOWER && mPacketType == SeedType::SEED_SUNFLOWER)
		{
			aGrayness = GetFlashingColor(mBoard->mMainCounter, 75).mRed;
		}
		else if (mBoard->mTutorialState == TutorialState::TUTORIAL_MORESUN_PICK_UP_SUNFLOWER && mPacketType == SeedType::SEED_SUNFLOWER)
		{
			aGrayness = GetFlashingColor(mBoard->mMainCounter, 75).mRed;
		}
		else if (mBoard->mTutorialState == TutorialState::TUTORIAL_WHACK_A_ZOMBIE_PICK_SEED)
		{
			aGrayness = GetFlashingColor(mBoard->mMainCounter, 75).mRed;
		}
		else if (mApp->mEasyPlantingCheat)
		{
			aPercentDark = 0.0f;
		}
		else if ((!mBoard->CanTakeSunMoney(aCost) && aDrawCost) || aPercentDark > 1.0f || !mBoard->PlantingRequirementsMet(aUseSeedType))
		{
			aGrayness = 128;
		}
		if (pvzu::engine::DrawPacketAsHeld(mBoard, mIndex)) // PVZU: touch: the held packet stays bright (PC greys it)
		{
			aPercentDark = 0.0f;
			aGrayness = 255;
		}

		if (mWidth != SEED_PACKET_WIDTH || mHeight != SEED_PACKET_HEIGHT) // PVZU: a HUD card (ApplySeedBankLayout): wide (1-8 slots) or compact (9-15)
		{
			DrawPanelCard(g, mOffsetX, 0.0f, mWidth, mHeight, mPacketType, mImitaterType, aPercentDark, aGrayness, aDrawCost);
			auto& aStarts = CardFlashStarts();
			if (auto aIt = aStarts.find(this); aIt != aStarts.end())
			{
				const float aAlpha = pvzu::CardFlashAlpha(mBoard->mMainCounter - aIt->second);
				if (aAlpha <= 0.0f)
					aStarts.erase(aIt);
				else
				{
					// The same card again, white and added on top: the frame, plant and tab light up in their shape.
					Graphics aFlashG(*g);
					aFlashG.SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
					aFlashG.SetColorizeImages(true);
					aFlashG.SetColor(Color(255, 255, 255, FloatRoundToInt(255.0f * aAlpha)));
					DrawPanelCard(&aFlashG, mOffsetX, 0.0f, mWidth, mHeight, mPacketType, mImitaterType, 0.0f, 255, false);
				}
			}
		}
		else
		{
			DrawSeedPacket(g, mOffsetX, 0.0f, mPacketType, mImitaterType, aPercentDark, aGrayness, aDrawCost, true);
		}
	}
}

bool SeedPacket::CanPickUp()
{
	if (mBoard->mPaused || mApp->mGameScene != GameScenes::SCENE_PLAYING || mPacketType == SeedType::SEED_NONE)
	{
		return false;
	}

	SeedType aUseSeedType = mPacketType;
	if (mPacketType == SeedType::SEED_IMITATER && mImitaterType != SeedType::SEED_NONE)
	{
		aUseSeedType = mImitaterType;
	}

	if (mApp->IsSlotMachineLevel())
	{
		return false;
	}

	if (!mApp->mEasyPlantingCheat)
	{
		if (!mActive)
		{
			return false;
		}

		int aCost = mBoard->GetCurrentPlantCost(mPacketType, mImitaterType);
		if (!mBoard->CanTakeSunMoney(aCost) && !mBoard->HasConveyorBeltSeedBank())
		{
			return false;
		}

		if (!mBoard->PlantingRequirementsMet(aUseSeedType))
		{
			return false;
		}
	}

	return true;
}

void SeedPacket::MouseDown([[maybe_unused]] int x, [[maybe_unused]] int y, [[maybe_unused]] int theClickCount)
{
	if (mBoard->mPaused || mApp->mGameScene != GameScenes::SCENE_PLAYING || mPacketType == SeedType::SEED_NONE)
	{
		return;
	}
	if (mApp->IsSlotMachineLevel())
	{
		if (!mBoard->mAdvice->IsBeingDisplayed())
		{
			mBoard->DisplayAdvice("[ADVICE_SLOT_MACHINE_PULL]", MessageStyle::MESSAGE_STYLE_HINT_TALL_FAST, AdviceType::ADVICE_NONE);
		}
		mBoard->mChallenge->mSlotMachineRollCount = std::min(mBoard->mChallenge->mSlotMachineRollCount, 2);
		return;
	}

	SeedType aUseSeedType = mPacketType;
	if (mPacketType == SeedType::SEED_IMITATER && mImitaterType != SeedType::SEED_NONE)
	{
		aUseSeedType = mImitaterType;
	}

	if (!mApp->mEasyPlantingCheat)
	{
		if (!mActive)
		{
			mApp->PlaySample(SOUND_BUZZER);
			if (mApp->IsFirstTimeAdventureMode() && mBoard->mLevel == 1 && mBoard->mHelpDisplayed[AdviceType::ADVICE_CLICK_ON_SUN])
			{
				mBoard->DisplayAdvice("[ADVICE_SEED_REFRESH]", MessageStyle::MESSAGE_STYLE_TUTORIAL_LEVEL1, AdviceType::ADVICE_SEED_REFRESH);
			}
			return;
		}

		int aCost = mBoard->GetCurrentPlantCost(mPacketType, mImitaterType);
		if (!mBoard->CanTakeSunMoney(aCost) && !mBoard->HasConveyorBeltSeedBank())
		{
			mApp->PlaySample(SOUND_BUZZER);
			mBoard->mOutOfMoneyCounter = 70;
			if (mApp->IsFirstTimeAdventureMode() && mBoard->mLevel == 1 && mBoard->mHelpDisplayed[AdviceType::ADVICE_CLICK_ON_SUN])
			{
				mBoard->DisplayAdvice("[ADVICE_CANT_AFFORD_PLANT]", MessageStyle::MESSAGE_STYLE_TUTORIAL_LEVEL1, AdviceType::ADVICE_CANT_AFFORD_PLANT);
			}
			return;
		}

		if (!mBoard->PlantingRequirementsMet(aUseSeedType))
		{
			mApp->PlaySample(SOUND_BUZZER);
			switch (aUseSeedType)
			{
			case SeedType::SEED_GATLINGPEA:
				mBoard->DisplayAdvice("[ADVICE_PLANT_NEEDS_REPEATER]", MessageStyle::MESSAGE_STYLE_HINT_LONG, AdviceType::ADVICE_PLANT_NEEDS_REPEATER);
				break;
			case SeedType::SEED_WINTERMELON:
				mBoard->DisplayAdvice("[ADVICE_PLANT_NEEDS_MELONPULT]", MessageStyle::MESSAGE_STYLE_HINT_LONG, AdviceType::ADVICE_PLANT_NEEDS_MELONPULT);
				break;
			case SeedType::SEED_TWINSUNFLOWER:
				mBoard->DisplayAdvice("[ADVICE_PLANT_NEEDS_SUNFLOWER]", MessageStyle::MESSAGE_STYLE_HINT_LONG, AdviceType::ADVICE_PLANT_NEEDS_SUNFLOWER);
				break;
			case SeedType::SEED_SPIKEROCK:
				mBoard->DisplayAdvice("[ADVICE_PLANT_NEEDS_SPIKEWEED]", MessageStyle::MESSAGE_STYLE_HINT_LONG, AdviceType::ADVICE_PLANT_NEEDS_SPIKEWEED);
				break;
			case SeedType::SEED_COBCANNON:
				mBoard->DisplayAdvice("[ADVICE_PLANT_NEEDS_KERNELPULT]", MessageStyle::MESSAGE_STYLE_HINT_LONG, AdviceType::ADVICE_PLANT_NEEDS_KERNELPULT);
				break;
			case SeedType::SEED_GOLD_MAGNET:
				mBoard->DisplayAdvice("[ADVICE_PLANT_NEEDS_MAGNETSHROOM]", MessageStyle::MESSAGE_STYLE_HINT_LONG, AdviceType::ADVICE_PLANT_NEEDS_MAGNETSHROOM);
				break;
			case SeedType::SEED_GLOOMSHROOM:
				mBoard->DisplayAdvice("[ADVICE_PLANT_NEEDS_FUMESHROOM]", MessageStyle::MESSAGE_STYLE_HINT_LONG, AdviceType::ADVICE_PLANT_NEEDS_FUMESHROOM);
				break;
			case SeedType::SEED_CATTAIL:
				mBoard->DisplayAdvice("[ADVICE_PLANT_NEEDS_LILYPAD]", MessageStyle::MESSAGE_STYLE_HINT_LONG, AdviceType::ADVICE_PLANT_NEEDS_LILYPAD);
				break;
			default:
				PVZP_ASSERT(false);
				break;
			}

			return;
		}
	}

	mBoard->ClearAdvice(AdviceType::ADVICE_CANT_AFFORD_PLANT);
	mBoard->ClearAdvice(AdviceType::ADVICE_PLANT_NEEDS_REPEATER);
	mBoard->ClearAdvice(AdviceType::ADVICE_PLANT_NEEDS_MELONPULT);
	mBoard->ClearAdvice(AdviceType::ADVICE_PLANT_NEEDS_SUNFLOWER);
	mBoard->ClearAdvice(AdviceType::ADVICE_PLANT_NEEDS_KERNELPULT);
	mBoard->ClearAdvice(AdviceType::ADVICE_PLANT_NEEDS_SPIKEWEED);
	mBoard->ClearAdvice(AdviceType::ADVICE_PLANT_NEEDS_MAGNETSHROOM);
	mBoard->ClearAdvice(AdviceType::ADVICE_PLANT_NEEDS_FUMESHROOM);
	mBoard->ClearAdvice(AdviceType::ADVICE_PLANT_NEEDS_LILYPAD);

	if (pvzu::engine::ModModePacketClick(this)) // PVZU: mod mode 0x8d toggles the plant's form (0xc007d0), after the
	{                                          // recharge / sun / requirement checks, as the exe's jump at 0x488dae
		return;
	}
	if (mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_BEGHOULED || mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_BEGHOULED_TWIST)
	{
		mBoard->mChallenge->BeghouledPacketClicked(this);
	}
	else if (mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_ZOMBIQUARIUM)
	{
		mBoard->mChallenge->ZombiquariumPacketClicked(this);
	}
	else if (pvzu::engine::SpecialPacketClicked(this)) // PVZU: 7-5 / mode 0x89, the Starfruit packet grows the snake (hook 467)
	{
	}
	else
	{
		mBoard->mCursorObject->mType = mPacketType;
		mBoard->mCursorObject->mImitaterType = mImitaterType;
		mBoard->mCursorObject->mCursorType = CursorType::CURSOR_TYPE_PLANT_FROM_BANK;
		mBoard->mCursorObject->mSeedBankIndex = mIndex;
		mApp->PlaySample(SOUND_SEEDLIFT);

		if (mBoard->mTutorialState == TutorialState::TUTORIAL_LEVEL_1_PICK_UP_PEASHOOTER)
		{
			mBoard->SetTutorialState(TutorialState::TUTORIAL_LEVEL_1_PLANT_PEASHOOTER);
		}
		else if (mBoard->mTutorialState == TutorialState::TUTORIAL_LEVEL_2_PICK_UP_SUNFLOWER)
		{
			if (mPacketType == SeedType::SEED_SUNFLOWER)
			{
				mBoard->SetTutorialState(TutorialState::TUTORIAL_LEVEL_2_PLANT_SUNFLOWER);
			}
			else
			{
				mBoard->SetTutorialState(TutorialState::TUTORIAL_LEVEL_2_REFRESH_SUNFLOWER);
			}
		}
		else if (mBoard->mTutorialState == TutorialState::TUTORIAL_MORESUN_PICK_UP_SUNFLOWER)
		{
			if (mPacketType == SeedType::SEED_SUNFLOWER)
			{
				mBoard->SetTutorialState(TutorialState::TUTORIAL_MORESUN_PLANT_SUNFLOWER);
			}
			else
			{
				mBoard->SetTutorialState(TutorialState::TUTORIAL_MORESUN_REFRESH_SUNFLOWER);
			}
		}
		else if (mBoard->mTutorialState == TutorialState::TUTORIAL_WHACK_A_ZOMBIE_PICK_SEED || mBoard->mTutorialState == TutorialState::TUTORIAL_WHACK_A_ZOMBIE_BEFORE_PICK_SEED)
		{
			mBoard->SetTutorialState(TutorialState::TUTORIAL_WHACK_A_ZOMBIE_COMPLETED);
		}

		Deactivate();
	}
}

void SeedPacket::WasPlanted()
{
	PVZP_ASSERT(mPacketType != SeedType::SEED_NONE);
	pvzu::engine::ModModePacketPlanted(this); // PVZU: mod mode 0x73 re-rolls the packet (hook 69)

	if (mBoard->HasConveyorBeltSeedBank())
	{
		mBoard->mSeedBank->RemoveSeed(mIndex);
	}
	else if (mApp->IsSlotMachineLevel())
	{
		Deactivate();
	}
	else if (mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_LAST_STAND && mBoard->mChallenge->mChallengeState != ChallengeState::STATECHALLENGE_LAST_STAND_ONSLAUGHT)
	{
		mTimesUsed++;
		mActive = true;
		FlashIfReady();
	}
	else
	{
		mTimesUsed++;
		mRefreshing = true;
		mRefreshTime = Plant::GetRefreshTime(mPacketType, mImitaterType);
		pvzu::engine::ModModePacketRecharge(this); // PVZU: mod mode 0x80, one use per packet (hook 70)
	}
}

bool SeedPacket::MouseHitTest(int theX, int theY, HitResult* theHitResult)
{
	if (mSlotMachineCountDown > 0 || mPacketType == SeedType::SEED_NONE)
	{
		theHitResult->mObject = nullptr;
		theHitResult->mObjectType = GameObjectType::OBJECT_TYPE_NONE;
		return false;
	}

	if (theX >= mX + mOffsetX && theX < mX + mOffsetX + mWidth && theY >= mY && theY < mY + mHeight)
	{
		theHitResult->mObject = this;
		theHitResult->mObjectType = GameObjectType::OBJECT_TYPE_SEEDPACKET;
		return true;
	}

	theHitResult->mObject = nullptr;
	theHitResult->mObjectType = GameObjectType::OBJECT_TYPE_NONE;
	return false;
}

SeedBank::SeedBank()
{
	mWidth = IMAGE_SEEDBANK->GetWidth();
	mHeight = IMAGE_SEEDBANK->GetHeight();

	mNumPackets = 0;
	mConveyorBeltCounter = 0;
	mCutSceneDarken = 255;
}

void SeedBank::Draw(Graphics* g)
{
	if (mBoard->mCutScene && mBoard->mCutScene->IsBeforePreloading())
	{
		return;
	}

	if (mApp->mGameScene != GameScenes::SCENE_PLAYING)
	{
		g->mTransX -= mBoard->mX - pvzu::engine::Camera(mBoard).rest; // PVZU: seed bank x is relative to the board at rest
		g->mTransY -= mBoard->mY;
	}

	const bool aPanel = pvzu::engine::SeedPanelActive(mBoard); // PVZU: the HUD's cards (no backing: they float)
	const int aPanelShiftX = aPanel ? pvzu::engine::HudSlideOf(mBoard).cardsX : 0; // PVZU: a hidden bank slides the cards out left
	const int aPanelShiftY = aPanel ? -mY : 0; // instead of up
	g->mTransX += aPanelShiftX;
	g->mTransY += aPanelShiftY;
	if (aPanel)
	{
		// PVZU: the HUD's cards float over the house (no backing)
	}
	else if (mApp->IsSlotMachineLevel())
	{
		g->DrawImage(IMAGE_SUNBANK, 0, 0);
	}
	else if (mBoard->HasConveyorBeltSeedBank())
	{
		g->DrawImage(IMAGE_CONVEYORBELT_BACKDROP, 83, 0);
		g->DrawImageCel(IMAGE_CONVEYORBELT, 90, 63, mConveyorBeltCounter / 4 % 6);
		g->SetClipRect(90, 0, 501, BOARD_HEIGHT);
	}
	else
	{
		int aExtraWidth = mBoard->GetSeedBankExtraWidth();
		g->DrawImage(IMAGE_SEEDBANK, 0, 0);
		if (mNumPackets > pvzu::kVanillaSeedSlots) // PVZU: 11-15 slots (Unnamed): wood tiles, then vanilla's right end
		{
			for (const pvzu::BankPiece& aPiece : pvzu::TopBankExtensionPieces(IMAGE_SEEDBANK->mWidth, aExtraWidth))
				g->DrawImage(IMAGE_SEEDBANK, aPiece.dstX, 0, Rect(aPiece.srcX, 0, aPiece.w, IMAGE_SEEDBANK->mHeight));
		}
		else
		{
			Rect theSrcRect(IMAGE_SEEDBANK->mWidth - aExtraWidth - 12, 0, aExtraWidth + 12, IMAGE_SEEDBANK->mHeight);
			g->DrawImage(IMAGE_SEEDBANK, IMAGE_SEEDBANK->mWidth - 12, 0, theSrcRect);
		}
	}

	for (int i = 0; i < mNumPackets; i++)
	{
		SeedPacket* aSeedPacket = &mSeedPackets[i];
		if (aSeedPacket->mPacketType != SeedType::SEED_NONE && aSeedPacket->BeginDraw(g))
		{
			aSeedPacket->Draw(g);
			aSeedPacket->EndDraw(g);
		}
	}

	g->ClearClipRect();
	if (mApp->IsSlotMachineLevel() && mY > -IMAGE_SEEDBANK->GetHeight())
	{
		g->DrawImage(IMAGE_SLOTMACHINE_OVERLAY, 189, -2);
	}

	if (aPanel) // PVZU: the HUD's sun counter box (it slides up, not left)
	{
		g->mTransX -= aPanelShiftX;
		pvzu::engine::DrawHudSunBox(g, mBoard);
		g->mTransX += aPanelShiftX;
	}
	else if (!mBoard->HasConveyorBeltSeedBank())
	{
		std::string aMoneyLabel = std::to_string(std::max(mBoard->mSunMoney, 0));
		pvzu::engine::ModModeSunLabel(mBoard, aMoneyLabel); // PVZU: mod mode 0x91 "Unlimited" (hook 471)
		Color aMoneyColor(0, 0, 0);
		if (mBoard->mOutOfMoneyCounter > 0 && mBoard->mOutOfMoneyCounter % 20 < 10)
		{
			aMoneyColor = Color(255, 0, 0);
		}

		PvzpDrawString(g, aMoneyLabel, 34, 78, FONT_CONTINUUMBOLD14, aMoneyColor, DrawStringJustification::DS_ALIGN_CENTER);
	}

	g->mTransX -= aPanelShiftX; // PVZU
	g->mTransY -= aPanelShiftY;
	if (mApp->mGameScene != GameScenes::SCENE_PLAYING)
	{
		g->mTransX += mBoard->mX - pvzu::engine::Camera(mBoard).rest; // PVZU
		g->mTransY += mBoard->mY;
	}
}

bool SeedBank::MouseHitTest(int x, int y, HitResult* theHitResult)
{
	if (pvzu::engine::SeedPanelActive(mBoard) && mNumPackets > 0)
	{
		// PVZU: seed panel: finger-sized hit areas (at least 64 px wide), split between neighbours.
		const std::vector<pvzu::HitRect> aRects = pvzu::SeedPanelHitRects(mNumPackets); // the drawn cards/packets, finger-sized
		for (int i = 0; i < mNumPackets; i++)
		{
			const pvzu::HitRect& r = aRects[i];
			const int lx = x - mX - pvzu::engine::HudSlideOf(mBoard).cardsX, ly = y; // drawn slid, never raised (SeedBank::Draw)
			if (mSeedPackets[i].mPacketType != SeedType::SEED_NONE && lx >= r.x && lx < r.x + r.w && ly >= r.y && ly < r.y + r.h)
			{
				theHitResult->mObject = &mSeedPackets[i];
				theHitResult->mObjectType = GameObjectType::OBJECT_TYPE_SEEDPACKET;
				return true;
			}
		}
		theHitResult->mObject = nullptr;
		theHitResult->mObjectType = GameObjectType::OBJECT_TYPE_NONE;
		return false;
	}
	if (x - mX <= mWidth - 5 && mNumPackets > 0)
	{
		for (SeedPacket& aSeedPacket : mSeedPackets)
		{
			if (aSeedPacket.MouseHitTest(x - mX, y - mY, theHitResult))
			{
				return true;
			}
		}
	}

	theHitResult->mObject = nullptr;
	theHitResult->mObjectType = GameObjectType::OBJECT_TYPE_NONE;
	return false;
}

bool SeedBank::ContainsPoint(int theX, int theY)
{
	return theX >= mX && theX < mX + mWidth && theY >= mY && theY < mY + mHeight;
}

void SeedBank::AddSeed(SeedType theSeedType, bool thePlaceOnLeft)
{
	PVZP_ASSERT(mBoard->HasConveyorBeltSeedBank());
	PVZP_ASSERT(theSeedType != SeedType::SEED_NONE);

	int aNumSeeds = GetNumSeedsOnConveyorBelt();
	if (aNumSeeds == mNumPackets)
	{
		return;
	}

	SeedPacket* aSeedPacket = &mSeedPackets[aNumSeeds];
	aSeedPacket->mPacketType = theSeedType;
	aSeedPacket->mRefreshCounter = 0;
	aSeedPacket->mRefreshTime = 0;
	aSeedPacket->mRefreshing = false;
	aSeedPacket->mActive = true;
	aSeedPacket->mOffsetX = 515 - (SEED_PACKET_WIDTH + 1) * aNumSeeds;
	if (thePlaceOnLeft)
	{
		aSeedPacket->mOffsetX = 0;
	}
	if (aNumSeeds > 0)
	{
		SeedPacket* aPrevPacket = &mSeedPackets[aNumSeeds - 1];
		if (aSeedPacket->mOffsetX < aPrevPacket->mOffsetX)
		{
			aSeedPacket->mOffsetX = aPrevPacket->mOffsetX + 40;
		}
	}
}

void SeedBank::RemoveSeed(int theIndex)
{
	PVZP_ASSERT(mBoard->HasConveyorBeltSeedBank());
	PVZP_ASSERT(theIndex >= 0 && theIndex < GetNumSeedsOnConveyorBelt());

	for (int i = theIndex; i < mNumPackets; i++)
	{
		SeedPacket* aSeedPacket = &mSeedPackets[i];
		if (aSeedPacket->mPacketType == SeedType::SEED_NONE)
		{
			break;
		}

		if (i == mNumPackets - 1)
		{
			aSeedPacket->mPacketType = SeedType::SEED_NONE;
			aSeedPacket->mOffsetX = 0;
		}
		else
		{
			SeedPacket* aNextPacket = &mSeedPackets[i + 1];
			aSeedPacket->mPacketType = aNextPacket->mPacketType;
			aSeedPacket->mOffsetX = aNextPacket->mOffsetX + SEED_PACKET_WIDTH + 1;
		}

		aSeedPacket->mRefreshCounter = 0;
		aSeedPacket->mRefreshTime = 0;
		aSeedPacket->mRefreshing = false;
		aSeedPacket->mActive = true;
	}
}

int SeedBank::GetNumSeedsOnConveyorBelt()
{
	for (int i = 0; i < mNumPackets; i++)
	{
		if (mSeedPackets[i].mPacketType == SeedType::SEED_NONE)
		{
			return i;
		}
	}
	return mNumPackets;
}

int SeedBank::CountOfTypeOnConveyorBelt(SeedType theSeedType)
{
	int aCount = 0;
	for (int i = 0; i < mNumPackets; i++)
	{
		if (mSeedPackets[i].mPacketType == theSeedType)
		{
			aCount++;
		}
	}
	return aCount;
}

void SeedPacket::SetPacketType(SeedType theSeedType, SeedType theImitaterType)
{
	mPacketType = theSeedType;
	mImitaterType = theImitaterType;
	mRefreshCounter = 0;
	mRefreshTime = 0;
	mRefreshing = false;
	mActive = true;

	SeedType aUseSeedType = theSeedType;
	if (theSeedType == SeedType::SEED_IMITATER && theImitaterType != SeedType::SEED_NONE)
	{
		aUseSeedType = theImitaterType;
	}

	if (mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_BEGHOULED || mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_BEGHOULED_TWIST ||
		mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_ZOMBIQUARIUM || mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_LAST_STAND ||
		mApp->IsIZombieLevel() || mApp->IsScaryPotterLevel() || mApp->IsWhackAZombieLevel() || (mApp->IsSurvivalMode() && mBoard->mChallenge->mSurvivalStage > 0))
		return;

	if ((Plant::IsUpgrade(aUseSeedType) && !gLawnApp->IsSurvivalMode()) || Plant::GetRefreshTime(mPacketType, mImitaterType) == 5000 ||
		pvzu::engine::ModPacketStartsRecharging(theSeedType, theImitaterType)) // PVZU: Golden Sun-shroom (hook 472, 0x7a6500)
	{
		mRefreshTime = 3500;
		mRefreshing = true;
		mActive = false;
	}
	else if (Plant::IsUpgrade(aUseSeedType) && gLawnApp->IsSurvivalMode())
	{
		mRefreshTime = 8000;
		mRefreshing = true;
		mActive = false;
	}
	else if (Plant::GetRefreshTime(mPacketType, mImitaterType) == 3000)
	{
		mRefreshTime = 2000;
		mRefreshing = true;
		mActive = false;
	}
}

void SeedBank::UpdateConveyorBelt()
{
	mConveyorBeltCounter++;
	if (mConveyorBeltCounter % CONVEYOR_SPEED == 0)
	{
		for (int i = 0; i < mNumPackets; i++)
		{
			SeedPacket* aSeedPacket = &mSeedPackets[i];
			if (aSeedPacket->mOffsetX > 0)
			{
				aSeedPacket->mOffsetX = pvzu::unnamed::ConveyorStepDown(aSeedPacket->mOffsetX, pvzu::engine::ConveyorStep(mBoard)); // PVZU: V2 double sun x2 (hook 473); vanilla max(x - 1, 0)
			}
		}

		mBoard->UpdateToolTip();
	}
}

void SeedBank::UpdateWidth()
{
	mNumPackets = mBoard->GetNumSeedsInBank();
	mWidth = IMAGE_SEEDBANK->GetWidth() + mBoard->GetSeedBankExtraWidth();
	for (int i = 0; i < mNumPackets; i++)
	{
		mSeedPackets[i].mX = mBoard->GetSeedPacketPositionX(i);
	}
}

void SeedBank::RefreshAllPackets()
{
	for (int i = 0; i < mNumPackets; i++)
	{
		SeedPacket* aSeedPacket = &mSeedPackets[i];
		if (aSeedPacket->mPacketType == SeedType::SEED_NONE)
		{
			break;
		}

		if (aSeedPacket->mRefreshing)
		{
			aSeedPacket->mRefreshCounter = 0;
			aSeedPacket->mRefreshing = false;
			aSeedPacket->mActive = true;
			aSeedPacket->FlashIfReady();
		}
	}
}
