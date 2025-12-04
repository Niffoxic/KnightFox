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

#include "EngineAPI.h"
#include <string>
#include <sal.h>

namespace kfe
{
	class __declspec(novtable) KFE_API IManager
	{
	public:
		virtual ~IManager() = default;
		
		_NODISCARD _Check_return_
		virtual bool Initialize() = 0;
		virtual bool Release()	  = 0;

		virtual void OnFrameBegin(_In_ float deltaTime) = 0;
		virtual void OnFrameEnd()						= 0;

		virtual std::string GetName() const noexcept = 0;
	};
}
