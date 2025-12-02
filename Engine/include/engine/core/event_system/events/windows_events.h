#pragma once

#include <Windows.h>

typedef struct _KFE_WINDOW_RESIZED_EVENT
{
	UINT Width;
	UINT Height;
} KFE_WINDOW_RESIZED_EVENT;

typedef struct _KFE_WINDOW_PAUSE_EVENT
{
	BOOL Paused;
} KFE_WINDOW_PAUSE_EVENT;

typedef struct _KFE_FULL_SCREEN_EVENT
{
	UINT Width;
	UINT Height;
} KFE_FULL_SCREEN_EVENT;

typedef struct _KFE_WINDOWED_SCREEN_EVENT
{
	UINT Width;
	UINT Height;
} KFE_WINDOWED_SCREEN_EVENT;
