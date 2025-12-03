#pragma once

#include <string>
#include <sal.h>
#include <functional>
#include <windows.h>

#include "key_generator/key_generator.h"

#ifndef SAFE_RELEASE
	#define SAFE_RELEASE(x) if(x){ x->Release(); x = nullptr; }
#endif

//~ States
enum class EScreenState : uint8_t
{
	FullScreen = 0,
	Windowed   = 1,
};

enum class EOccupiedState : uint8_t
{
	Free	 = 0,
	Occupied = 1,
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
	_In_					 std::string  WindowTitle{ "KnightEngine" };
	_Field_range_(100, 1920) UINT		  Width		 { 800u };
	_Field_range_(100, 1080) UINT		  Height	 { 800u };
	_Field_range_(0, 200)    UINT		  IconId	 { 0u };
	_In_					 EScreenState ScreenState{ EScreenState::Windowed };
} KFE_WINDOW_CREATE_DESC;

//~ Type Defines
using KFE_WinSizeU = KFE_WinSize<UINT>;
using KFE_WinSizeF = KFE_WinSize<float>;
using KFE_WinSizeI = KFE_WinSize<int>;

//~ Callbacks
using CallBack_IdBool	  = std::function<void(kfe::KID, bool)>;

struct OccupancyInfo
{
	kfe::KID	   Id;
	EOccupiedState State;
};
using CallBack_IdOccupied = std::function<void(const OccupancyInfo)>;
