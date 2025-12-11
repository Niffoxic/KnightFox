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
#include "engine/core.h"

namespace kfe
{
	class MapEditorContext;

	class KFE_API IKFECommand : public IKFEObject
	{
	public:
		virtual const char* GetCommandName() const noexcept = 0;
		
		virtual void Execute(MapEditorContext* ctx) = 0;
		virtual void Undo   (MapEditorContext* ctx) = 0;
	};
}
