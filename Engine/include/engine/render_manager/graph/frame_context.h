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

#include "engine/render_manager/api/components/device.h"
#include "engine/render_manager/api/commands/graphics_list.h"
#include "engine/render_manager/api/heap/heap_cbv_srv_uav.h"
#include "engine/render_manager/api/heap/heap_dsv.h"
#include "engine/render_manager/api/heap/heap_rtv.h"
#include "engine/render_manager/api/buffer/buffer.h"

#include <cstdint>

namespace kfe
{
	struct FrameContext
	{
		KFEGraphicsCommandList* CommandList{ nullptr };
		KFEDevice* Device{ nullptr };

		KFERTVHeap* RtvHeap{ nullptr };
		KFEDSVHeap* DsvHeap{ nullptr };
		KFEResourceHeap* CbvSrvUavHeap{ nullptr };

		KFEBuffer* FrameConstantBuffer{ nullptr };

		std::uint32_t FrameIndex{ 0u };

		NODISCARD KFEGraphicsCommandList* GetCommandList() const noexcept
		{
			return CommandList;
		}

		NODISCARD KFEDevice* GetDevice() const noexcept
		{
			return Device;
		}

		NODISCARD IKFEDescriptorHeap* GetRtvHeap() const noexcept
		{
			return RtvHeap;
		}

		NODISCARD IKFEDescriptorHeap* GetDsvHeap() const noexcept
		{
			return DsvHeap;
		}

		NODISCARD IKFEDescriptorHeap* GetCbvSrvUavHeap() const noexcept
		{
			return CbvSrvUavHeap;
		}

		NODISCARD KFEBuffer* GetFrameConstantBuffer() const noexcept
		{
			return FrameConstantBuffer;
		}

		NODISCARD std::uint32_t GetFrameIndex() const noexcept
		{
			return FrameIndex;
		}
	};
} // namespace kfe
