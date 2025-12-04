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

#include "engine/core.h"

#include <string>
#include <functional>


#ifndef SAFE_RELEASE
	#define SAFE_RELEASE(x) if(x){ x->Release(); x = nullptr; }
#endif

//~ States
enum class EScreenState : uint8_t
{
	FullScreen = 0,
	Windowed   = 1,
};

enum class EWorkState : uint8_t
{
	Free	 = 0,
	Working  = 1,
};

//~ Queries
typedef struct DIRECTORY_AND_FILE_NAME
{
	_In_ std::string DirectoryNames;
	_In_ std::string FileName;
} DIRECTORY_AND_FILE_NAME;

template<typename T>
struct KFE_WinSize 
{
	T Width {};
	T Height{};


	KFE_WinSize() = default;

	KFE_WinSize(T width, T height)
		: Width(width), Height(height)
	{}

	template<typename Type>
	_Check_return_ _NODISCARD
	Type As()
	{
		return KFE_WinSize<Type>(static_cast<Type>(Width),
								 static_cast<Type>(Height));
	}

	_Check_return_ _NODISCARD
	float AspectRatio() const
	{
		return (Height == 0)
			? 0.0f
			: static_cast<float>(Width) / static_cast<float>(Height);
	}
};

typedef struct _KFE_WINDOW_CREATE_DESC
{
	_In_					 std::string   WindowTitle{ "KnightEngine" };
	_Field_range_(100, 1920) std::uint32_t Width	  { 800u };
	_Field_range_(100, 1080) std::uint32_t Height	  { 800u };
	_Field_range_(0, 200)    std::uint32_t IconId	  { 0u };
	_In_					 EScreenState ScreenState { EScreenState::Windowed };
} KFE_WINDOW_CREATE_DESC;

typedef struct KFE_RECT
{
	std::uint32_t left;
	std::uint32_t right;
	std::uint32_t bottom;
	std::uint32_t top;
} KFE_RECT;

//~ Type Defines
using KFE_WinSizeU = KFE_WinSize<std::uint32_t>;
using KFE_WinSizeF = KFE_WinSize<float>;
using KFE_WinSizeI = KFE_WinSize<int>;

//~ Callbacks
using CallBack_IdBool	  = std::function<void(kfe::KID, bool)>;

struct OccupancyInfo
{
	kfe::KID	   Id;
	EWorkState State;
};
using CallBack_IdOccupied = std::function<void(const OccupancyInfo)>;
