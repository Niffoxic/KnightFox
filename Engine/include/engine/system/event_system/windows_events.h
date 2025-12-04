// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

/*
 *  -----------------------------------------------------------------------------
 *  Project   : KnightFox (WMG Warwick - Module 2 WM9M2:Computer Graphics)
 *  Author    : Niffoxic (a.k.a Harsh Dubey)
 *  License   : MIT
 *  -----------------------------------------------------------------------------
 */

#pragma once

#include <cstdint>

typedef struct _KFE_WINDOW_RESIZED_EVENT
{
	std::uint32_t Width;
	std::uint32_t Height;
} KFE_WINDOW_RESIZED_EVENT;

typedef struct _KFE_WINDOW_PAUSE_EVENT
{
	bool Paused;
} KFE_WINDOW_PAUSE_EVENT;

typedef struct _KFE_FULL_SCREEN_EVENT
{
	std::uint32_t Width;
	std::uint32_t Height;
} KFE_FULL_SCREEN_EVENT;

typedef struct _KFE_WINDOWED_SCREEN_EVENT
{
	std::uint32_t Width;
	std::uint32_t Height;
} KFE_WINDOWED_SCREEN_EVENT;
