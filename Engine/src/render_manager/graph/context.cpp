// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

/*
 *  -----------------------------------------------------------------------------
 *  Project   : KnightFox (WMG Warwick - Module 2 WM9M2:Computer Graphics)
 *  Author    : Niffoxic (a.k.a Harsh Dubey)
 *  License   : MIT
 *  -----------------------------------------------------------------------------
 */

#include "pch.h"
#include "engine/render_manager/graph/context.h"
#include "engine/render_manager/api/buffer/buffer.h"
#include "engine/render_manager/api/texture/texture.h"
#include "engine/render_manager/api/commands/graphics_list.h"

#include <d3d12.h>


#pragma region Context_Implementation

kfe::rg::RGExecutionContext::RGExecutionContext() = default;

kfe::rg::RGExecutionContext::~RGExecutionContext() = default;
kfe::rg::RGExecutionContext::RGExecutionContext					   (RGExecutionContext&&) noexcept = default;
kfe::rg::RGExecutionContext& kfe::rg::RGExecutionContext::operator=(RGExecutionContext&&) noexcept = default;

kfe::KFEGraphicsCommandList* kfe::rg::RGExecutionContext::GetCommandList() const noexcept
{
	return CommandList;
}

kfe::KFETexture* kfe::rg::RGExecutionContext::GetTexture(RGTextureHandle handle) const noexcept
{
	if (!handle.IsValid())
	{
		return nullptr;
	}

	const auto index = handle.Index;
	if (index >= Textures.size())
	{
		return nullptr;
	}

	return Textures[index];
}

kfe::KFEBuffer* kfe::rg::RGExecutionContext::GetBuffer(RGBufferHandle handle) const noexcept
{
	if (!handle.IsValid())
	{
		return nullptr;
	}

	const auto index = handle.Index;
	if (index >= Buffers.size())
	{
		return nullptr;
	}

	return Buffers[index];
}

D3D12_CPU_DESCRIPTOR_HANDLE kfe::rg::RGExecutionContext::GetRTV(RGTextureHandle handle) const noexcept
{
	D3D12_CPU_DESCRIPTOR_HANDLE nullHandle{};
	if (!handle.IsValid())
	{
		return nullHandle;
	}

	const auto index = handle.Index;
	if (index >= RTVs.size())
	{
		return nullHandle;
	}

	return RTVs[index];
}

D3D12_CPU_DESCRIPTOR_HANDLE kfe::rg::RGExecutionContext::GetDSV(RGTextureHandle handle) const noexcept
{
	D3D12_CPU_DESCRIPTOR_HANDLE nullHandle{};
	if (!handle.IsValid())
	{
		return nullHandle;
	}

	const auto index = handle.Index;
	if (index >= DSVs.size())
	{
		return nullHandle;
	}

	return DSVs[index];
}

D3D12_GPU_DESCRIPTOR_HANDLE kfe::rg::RGExecutionContext::GetSRV(RGTextureHandle handle) const noexcept
{
	D3D12_GPU_DESCRIPTOR_HANDLE nullHandle{};
	nullHandle.ptr = 0;

	if (!handle.IsValid())
	{
		return nullHandle;
	}

	const auto index = handle.Index;
	if (index >= SRVs.size())
	{
		return nullHandle;
	}

	return SRVs[index];
}

D3D12_GPU_DESCRIPTOR_HANDLE kfe::rg::RGExecutionContext::GetUAV(RGTextureHandle handle) const noexcept
{
	D3D12_GPU_DESCRIPTOR_HANDLE nullHandle{};
	nullHandle.ptr = 0;

	if (!handle.IsValid())
	{
		return nullHandle;
	}

	const auto index = handle.Index;
	if (index >= UAVs.size())
	{
		return nullHandle;
	}

	return UAVs[index];
}

#pragma endregion
