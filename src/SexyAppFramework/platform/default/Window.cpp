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

#include <cstdlib>
#include <SDL3/SDL.h>

#include <memory>
#include <utility>

#include "SexyAppBase.h"
#include "graphics/GLInterface.h"
#include "graphics/GLImage.h"
#include "graphics/GLPlatform.h"
#include "widget/WidgetManager.h"
#include "Mod/CmdLine.h" // PVZU
#include "Mod/Engine/DebugHooks.h" // PVZU
#include "Mod/Screen.h" // PVZU
#include "Mod/HdArt.h" // PVZU

using namespace Sexy;

void SexyAppBase::MakeWindow()
{
#if !defined(__ANDROID__) && !defined(__IPHONEOS__) && !defined(__EMSCRIPTEN__) && !defined(__SWITCH__)
	if (pvzu::engine::DebugRunActive()) // PVZU: scripted checks never go full screen (no new desktop Space)
		mIsWindowed = true;
#endif
	if (mWindow)
	{
		SDL_SetWindowFullscreen(mWindow.get(), !mIsWindowed);
	}
	else
	{
		// For Wayland's icon support on the game window
		SDL_SetHint(SDL_HINT_APP_ID, "io.github.wszqkzqk.pvz-portable");

#if (defined(__ANDROID__) && !defined(__TERMUX__)) || defined(__IPHONEOS__)
		SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");
#endif

		if (!SDL_Init(SDL_INIT_VIDEO))
		{
			Sexy::LogErrorLn("Failed to initialize SDL video: {}", SDL_GetError());
			return;
		}

		// PVZU: a forced wide width (-pvzu-width) opens a big window (2x, or the largest that fits); scripted
		// checks open without taking focus from whatever the user is doing.
		int aWinW = mWidth, aWinH = mHeight;
#if !defined(__ANDROID__) && !defined(__IPHONEOS__) && !defined(__EMSCRIPTEN__) && !defined(__SWITCH__)
		if (pvzu::CmdFlag("-pvzu-width"))
		{
			SDL_Rect aUsable{};
			SDL_DisplayID aDisplay = SDL_GetPrimaryDisplay();
			float aDensity = 1.0f;
			if (const SDL_DisplayMode* aMode = SDL_GetDesktopDisplayMode(aDisplay))
				aDensity = aMode->pixel_density > 0 ? aMode->pixel_density : 1.0f;
			if (SDL_GetDisplayUsableBounds(aDisplay, &aUsable))
			{
				pvzu::WindowSize aPixels = pvzu::DesktopWindowSize(mWidth, mHeight,
					static_cast<int>(aUsable.w * aDensity), static_cast<int>((aUsable.h - 40) * aDensity));
				aWinW = static_cast<int>(aPixels.w / aDensity);
				aWinH = static_cast<int>(aPixels.h / aDensity);
			}
		}
		// PVZU: -pvzu-window-px <w>x<h>: a test window of that many pixels (2340x1080 = the S26 Ultra), for captures at
		// phone magnification. Only for hidden (automated) runs, so it never covers the screen.
		if (auto aPx = pvzu::hd::ParseSize(pvzu::CmdOption("-pvzu-window-px").value_or("")); aPx && pvzu::AutomatedRun(getenv("PVZU_HIDDEN")))
		{
			float aDensity = 1.0f;
			if (const SDL_DisplayMode* aMode = SDL_GetDesktopDisplayMode(SDL_GetPrimaryDisplay()))
				aDensity = aMode->pixel_density > 0 ? aMode->pixel_density : 1.0f;
			aWinW = static_cast<int>(aPx->first / aDensity + 0.5f);
			aWinH = static_cast<int>(aPx->second / aDensity + 0.5f);
		}
		if (pvzu::engine::DebugRunActive())
			SDL_SetHint(SDL_HINT_WINDOW_ACTIVATE_WHEN_SHOWN, "0");
		// PVZU: automated runs (screenshots, scripts, level/log flags, or PVZU_HIDDEN=1) never show a window and never
		// go full screen: full-screen test windows opened new desktops on the development Mac (2026-11-23).
		const bool aHiddenRun = pvzu::AutomatedRun(getenv("PVZU_HIDDEN"));
		if (aHiddenRun)
			mIsWindowed = true;
#else
		const bool aHiddenRun = false;
#endif

		SDL_WindowFlags winFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
			| SDL_WINDOW_HIGH_PIXEL_DENSITY | (!mIsWindowed ? SDL_WINDOW_FULLSCREEN : 0)
			| (aHiddenRun ? SDL_WINDOW_HIDDEN : 0);

		// Try OpenGL ES 2.0 first (Linux, most Windows drivers, ANGLE, etc.)
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

		SDLWindowPtr window(SDL_CreateWindow(mTitle.c_str(), aWinW, aWinH, winFlags) /* PVZU: size */, &SDL_DestroyWindow);
		SDLGLContextPtr context(nullptr, &SDL_GL_DestroyContext);

		if (window)
			context.reset(SDL_GL_CreateContext(window.get()));

#if defined(__ANDROID__) || defined(__IPHONEOS__)
		// EGL/EAGL surface may be transiently unavailable on mobile
		for (int retry = 0; !context && window && retry < 20; retry++)
		{
			SDL_Delay(100);
			SDL_PumpEvents();
			context.reset(SDL_GL_CreateContext(window.get()));
		}
		if (!context)
		{
			Sexy::LogErrorLn("Failed to create OpenGL ES context: {}", SDL_GetError());
			return;
		}
#else
		// Fallback: desktop GL 2.1 compatibility (macOS, old Windows drivers, etc.)
		if (!context)
		{
			window.reset();

			SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);

			window.reset(SDL_CreateWindow(mTitle.c_str(), aWinW, aWinH, winFlags) /* PVZU: size */);

			if (window)
				context.reset(SDL_GL_CreateContext(window.get()));

			if (!context)
			{
				Sexy::LogErrorLn("Failed to create any OpenGL context: {}", SDL_GetError());
				return;
			}

			gDesktopGLFallback = true;
		}
#endif

		SDL_GL_SetSwapInterval(1);

		mWindow = std::move(window);
		mContext = std::move(context);
	}

	if (mGLInterface == nullptr)
	{
		mGLInterface = std::make_unique<GLInterface>(this);
		if (!InitGLInterface())
		{
			mGLInterface = nullptr;
			return;
		}
	}

	bool isActive = mActive;
	mActive = !!(SDL_GetWindowFlags(mWindow.get()) & SDL_WINDOW_INPUT_FOCUS);
	if (pvzu::engine::DebugRunActive()) // PVZU: scripted checks run as if focused (focus events are ignored)
		mActive = true;

	mPhysMinimized = false;
	if (mMinimized)
	{
		if (mMuteOnLostFocus)
			Unmute(true);

		mMinimized = false;
		isActive = mActive; // set this here so we don't call RehupFocus again.
		RehupFocus();
	}

	if (isActive != mActive)
		RehupFocus();

	ReInitImages();

	mWidgetManager->mImage = mGLInterface->GetScreenImage();
	mWidgetManager->MarkAllDirty();

	mGLInterface->UpdateViewport();
	mWidgetManager->Resize(mScreenBounds, mGLInterface->mPresentationRect);
}
