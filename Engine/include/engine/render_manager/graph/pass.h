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

#include "types.h"
#include "context.h"

#include <functional>

namespace kfe::rg
{
	/// <summary>
	///  static metadata
	///  runtime callback
	/// </summary>
	struct RenderPass
	{
		RenderPassDesc                               Desc;
		std::function<void(RGExecutionContext& ctx)> Execute;

		NODISCARD bool IsValid() const noexcept
		{
			return Desc.IsValid() && static_cast<bool>(Execute);
		}

		void Reset() noexcept
		{
			Desc.Clear();
			Execute = nullptr;
		}
	};
} // namespace kfe::rg
